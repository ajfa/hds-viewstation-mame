// license:BSD-3-Clause
// copyright-holders:
/*
    HDS ViewStation FX / Neoware @workStation (Intel 80960CA)

    Main board, from the HDS ViewStation System Administrator's Guide:
      - Intel 80960CA at 16, 25 or 33 MHz
      - Intel 82596CA Ethernet
      - 1 or 2 MB video memory, up to 1280x1024 at 8 bits
      - 8 KB non-volatile memory, 2 Mbit PLCC boot PROM
      - PC keyboard and mouse, parallel port, RS-232

    Memory map, as used by the boot PROM and netOS 3.2:
      00000000-000003ff  80960CA internal data RAM
      10000000/10000004  82596 PORT / channel attention
      21800000-219fffff  video memory, 2048 byte lines
      21a00000-21bfffff  off-screen memory
      21c00000-21dfffff  video memory block write: each word is a 16 pixel mask
      21e00000/21fffffc  block write colour registers
      30000000-          DRAM, 8 MB; SIMM banks at 40000000-7fffffff (each repeats in its
                         window); -ram 24m (default), 40m or 72m sets the one at 40000000
      a8000000/ac000000  board control registers (16 bit)
      c0000000-          G300-style colour video controller (palette words 000-0ff), or on
                         the FX colour board a TLC34075 palette DAC in bytes 0-f; both
                         also answer at c8000000
      d0000000/d0000001  8042-style keyboard and mouse controller
      d2000000-d200000f  SCN2681 DUART; the output port drives LEDs and the speaker, the
                         input port reads four switch banks at d2/d28/d30/d38000000
      d6000000-d6000003  8254 timer, 3.6864 MHz
      d8000000-          setup memory
      da000000-          Ethernet address PROM
      e00001f0/e00005f0  IDE data, 8-bit path with a latch for the high byte
      e02001f0-e02001f7  IDE task file
      fefc0000-feffffff  boot PROM, also at fffc0000

    Interrupts: XINT0 DUART, XINT2 keyboard, XINT3 mouse, XINT5/7 watchdog, XINT6 100 Hz clock.

    The video mode follows the monitor type and video mode switches, so each system here is
    the same board set up for one of the HDS monitors.

    TODO: Ethernet interrupt, vertical retrace, PCMCIA, flash board, sound.
*/

#include "emu.h"

#include "cpu/i960/i960.h"
#include "machine/i82586.h"
#include "machine/mc68681.h"
#include "machine/nvram.h"
#include "machine/pckeybrd.h"
#include "machine/pit8253.h"
#include "machine/ram.h"
#include "video/tlc34076.h"

#include "bus/ata/ataintf.h"
#include "bus/ata/hdd.h"
#include "bus/rs232/rs232.h"

#include "endianness.h"
#include <deque>
#include "screen.h"

#define LOG_UNKNOWN (1U << 1)
#define LOG_DUART   (1U << 2)
#define LOG_DAC     (1U << 3)
#define LOG_ATA     (1U << 4)
#define LOG_KBC     (1U << 5)

#define VERBOSE (0)
#include "logmacro.h"


namespace {

class viewstation_state : public driver_device
{
public:
	viewstation_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_iram(*this, "iram")
		, m_dram(*this, "dram")
		, m_vram(*this, "vram")
		, m_pit(*this, "pit")
		, m_eth(*this, "eth")
		, m_duart(*this, "duart")
		, m_atkbd(*this, "at_keyboard")
		, m_mouse_x(*this, "MOUSEX")
		, m_mouse_y(*this, "MOUSEY")
		, m_mouse_btn(*this, "MOUSEBTN")
		, m_ata(*this, "ata")
		, m_ram(*this, "ram")
		, m_ramdac(*this, "ramdac")
		, m_config(*this, "CONFIG")
	{
	}

	void hdsfx(machine_config &config) ATTR_COLD;
	void hdsfx_v16c(machine_config &config) ATTR_COLD;
	void hdsfx_v14c(machine_config &config) ATTR_COLD;
	void hdsfx_vesa(machine_config &config) ATTR_COLD;

private:
	required_device<i80960ca_device> m_maincpu;
	required_shared_ptr<uint32_t> m_iram;
	required_shared_ptr<uint32_t> m_dram;
	required_shared_ptr<uint32_t> m_vram;
	required_device<pit8254_device> m_pit;
	required_device<i82596_device> m_eth;
	required_device<scn2681_device> m_duart;
	required_device<at_keyboard_device> m_atkbd;
	required_ioport m_mouse_x;
	required_ioport m_mouse_y;
	required_ioport m_mouse_btn;
	required_device<ata_interface_device> m_ata;
	required_device<ram_device> m_ram;
	required_device<tlc34076_device> m_ramdac;
	required_ioport m_config;
	bool m_colour = false;
	uint8_t m_monitor_sw = 0, m_mode_sw = 0;

	void mem_map(address_map &map) ATTR_COLD;
	void eth_map(address_map &map) ATTR_COLD;

	uint8_t mac_r(offs_t offset);
	void blockwrite_w(offs_t offset, uint32_t data, uint32_t mem_mask);
	uint32_t m_blkcolor[2] = { };

	// 8042-style keyboard and mouse controller
	uint8_t kbc_data_r();
	uint8_t kbc_status_r();
	void kbc_data_w(uint8_t data);
	void kbc_command_w(uint8_t data);
	void kbc_refill();
	void kbc_update_irq();
	void mouse_command(uint8_t data);
	TIMER_CALLBACK_MEMBER(mouse_poll);
	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;

	std::deque<uint8_t> m_kbc_q;    // controller responses
	std::deque<uint8_t> m_aux_q;    // mouse bytes
	uint8_t m_kbc_cmd = 0;
	uint8_t m_kbc_out = 0;
	bool m_kbc_obf = false;
	bool m_kbc_aux = false;
	bool m_kbc_last_cmd = false;
	uint8_t m_kbc_expect = 0;
	int m_kbd_line = 0;
	bool m_mouse_on = false;
	uint8_t m_mouse_expect = 0;
	uint16_t m_mouse_lastx = 0, m_mouse_lasty = 0;
	uint8_t m_mouse_lastbtn = 0;
	emu_timer *m_mouse_timer = nullptr;
	uint16_t ata_r(offs_t offset, uint16_t mem_mask);
	void ata_w(offs_t offset, uint16_t data, uint16_t mem_mask);
	uint8_t m_ata_regs[8] = { };
	uint8_t m_ata_latch = 0;
	uint8_t ata8_r(offs_t offset);
	void ata8_w(offs_t offset, uint8_t data);
	uint8_t duart_r(offs_t offset);
	void duart_w(offs_t offset, uint8_t data);
	void dac_w(offs_t offset, uint8_t data);
	uint8_t dac_r(offs_t offset);

	uint8_t m_dac_test = 0;
	uint32_t m_cvc_regs[0x400] = { };

	uint32_t screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

	uint8_t unk_r(offs_t offset);
	void unk_w(offs_t offset, uint8_t data);
};

uint8_t viewstation_state::unk_r(offs_t offset)
{
	if(!machine().side_effects_disabled())
		LOGMASKED(LOG_UNKNOWN, "%s: read %08x\n", machine().describe_context(), 0xa0000000 + offset);
	return 0;
}

void viewstation_state::unk_w(offs_t offset, uint8_t data)
{
	LOGMASKED(LOG_UNKNOWN, "%s: write %08x = %02x\n", machine().describe_context(), 0xa0000000 + offset, data);
}

// Ethernet address PROM; 00:80:96 is the HDS prefix used in the netOS bootptab
uint8_t viewstation_state::mac_r(offs_t offset)
{
	static const uint8_t mac[6] = { 0x00, 0x80, 0x96, 0x00, 0x12, 0x34 };
	return (offset < 6) ? mac[offset] : 0xff;
}

// FX colour board: a TLC34075 palette DAC in the low bytes of the video controller, also
// answering at c8000000. Writing 3 to the test register makes it return its ID.
uint8_t viewstation_state::dac_r(offs_t offset)
{
	if(offset == 0xe && m_dac_test == 3)
		return 0x75;
	return m_ramdac->read(offset);
}

void viewstation_state::dac_w(offs_t offset, uint8_t data)
{
	LOGMASKED(LOG_DAC, "%s: dac w %x = %02x\n", machine().describe_context(), offset, data);
	if(offset == 0xe)
		m_dac_test = data;
	m_ramdac->write(offset, data);
}

// The DUART input port reads one of four 4-bit switch banks on IP3-IP6, chosen by A23-A24:
// the monitor type, the video mode (inverted) and two jumper banks (inverted).
uint8_t viewstation_state::duart_r(offs_t offset)
{
	const int bank = (offset >> 23) & 3;
	offset &= 0xf;
	uint8_t data = m_duart->read(offset);
	if(offset == 0xd) {
		uint8_t nib = 0;
		switch(bank) {
		case 0: nib = m_monitor_sw; break;
		case 1: nib = ~m_mode_sw; break;
		}
		data = (data & 0x87) | ((nib & 0xf) << 3);
	}
	return data;
}

void viewstation_state::duart_w(offs_t offset, uint8_t data)
{
	offset &= 0xf;
	if(offset == 3 || offset == 11)
		LOGMASKED(LOG_DUART, "duart tx%c %02x '%c'\n", offset == 3 ? 'a' : 'b', data, (data >= 0x20 && data < 0x7f) ? data : '.');
	else
		LOGMASKED(LOG_DUART, "%s: duart w %x = %02x\n", machine().describe_context(), offset, data);
	m_duart->write(offset, data);
}

// IDE task file at ISA-style ports 1f0-1f7 behind e0200000; byte registers, 16-bit data
uint16_t viewstation_state::ata_r(offs_t offset, uint16_t mem_mask)
{
	if(offset == 0 && mem_mask == 0xffff)
		return m_ata->cs0_r(0);
	const int reg = offset * 2 + ((mem_mask & 0xff) ? 0 : 1);
	const uint8_t data = m_ata->cs0_r(reg);
	if(!machine().side_effects_disabled() && reg != 7)
		LOGMASKED(LOG_ATA, "%s: ata r %d = %02x\n", machine().describe_context(), reg, data);
	return (mem_mask & 0xff) ? data : data << 8;
}

void viewstation_state::ata_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	if(offset == 0 && mem_mask == 0xffff) {
		m_ata->cs0_w(0, data);
		return;
	}
	const int reg = offset * 2 + ((mem_mask & 0xff) ? 0 : 1);
	const uint8_t val = (mem_mask & 0xff) ? data : data >> 8;
	m_ata_regs[reg] = val;
	if(reg == 7)
		LOGMASKED(LOG_ATA, "ata cmd %02x lba %02x%02x%02x%02x count %d\n", val, m_ata_regs[6] & 0xf, m_ata_regs[5], m_ata_regs[4], m_ata_regs[3], m_ata_regs[2]);
	m_ata->cs0_w(reg, val);
}

// 8-bit path to the same drive: the data word goes through a latch, low byte at 1f0 and
// high byte at 5f0 (A10)
uint8_t viewstation_state::ata8_r(offs_t offset)
{
	if(offset == 0x1f0) {
		if(machine().side_effects_disabled())
			return 0;
		const uint16_t data = m_ata->cs0_r(0);
		m_ata_latch = data >> 8;
		return data & 0xff;
	}
	if(offset == 0x5f0)
		return m_ata_latch;
	if(offset >= 0x1f1 && offset <= 0x1f7)
		return m_ata->cs0_r(offset - 0x1f0);
	if(offset == 0x3f6)
		return m_ata->cs1_r(6);
	if(!machine().side_effects_disabled())
		LOGMASKED(LOG_UNKNOWN, "%s: read %08x\n", machine().describe_context(), 0xe0000000 + offset);
	return 0;
}

void viewstation_state::ata8_w(offs_t offset, uint8_t data)
{
	if(offset == 0x5f0)
		m_ata_latch = data;
	else if(offset == 0x1f0)
		m_ata->cs0_w(0, data | (m_ata_latch << 8));
	else if(offset >= 0x1f1 && offset <= 0x1f7) {
		m_ata_regs[offset - 0x1f0] = data;
		if(offset == 0x1f7)
			LOGMASKED(LOG_ATA, "ata cmd %02x lba %02x%02x%02x%02x count %d\n", data, m_ata_regs[6] & 0xf, m_ata_regs[5], m_ata_regs[4], m_ata_regs[3], m_ata_regs[2]);
		m_ata->cs0_w(offset - 0x1f0, data);
	}
	else if(offset == 0x3f6)
		m_ata->cs1_w(6, data);
	else
		LOGMASKED(LOG_UNKNOWN, "%s: write %08x = %02x\n", machine().describe_context(), 0xe0000000 + offset, data);
}

void viewstation_state::machine_start()
{
	// the 8 MB on the main board are fixed; the rest of -ram is SIMM memory at 40000000,
	// which repeats through its 128 MB window like the main board memory
	const u32 simm = m_ram->size() - 0x800000;
	m_maincpu->space(AS_PROGRAM).install_ram(0x40000000, 0x40000000 + simm - 1, 0x07ffffff & ~(simm - 1), m_ram->pointer());

	// attach the Ethernet controller to the first host network device
	m_eth->set_interface(0);
	m_mouse_timer = timer_alloc(FUNC(viewstation_state::mouse_poll), this);
}

void viewstation_state::machine_reset()
{
	m_colour = BIT(m_config->read(), 0);
	m_kbc_q.clear();
	m_aux_q.clear();
	m_kbc_cmd = 0;
	m_kbc_obf = m_kbc_aux = false;
	m_kbc_expect = 0;
	m_mouse_on = false;
	m_mouse_expect = 0;
	m_mouse_timer->adjust(attotime::from_hz(100), 0, attotime::from_hz(100));
	kbc_update_irq();
}

// fill the output buffer: controller replies first, then the mouse, then the keyboard
void viewstation_state::kbc_refill()
{
	if(m_kbc_obf)
		return;
	if(!m_kbc_q.empty()) {
		m_kbc_out = m_kbc_q.front();
		m_kbc_q.pop_front();
		m_kbc_obf = true;
		m_kbc_aux = false;
	} else if(!m_aux_q.empty() && !BIT(m_kbc_cmd, 5)) {
		m_kbc_out = m_aux_q.front();
		m_aux_q.pop_front();
		m_kbc_obf = true;
		m_kbc_aux = true;
	} else if(m_kbd_line && !BIT(m_kbc_cmd, 4)) {
		// the keyboard never drops its key-ready line; an empty queue reads as 0
		const uint8_t data = m_atkbd->read();
		if(data) {
			m_kbc_out = data;
			m_kbc_obf = true;
			m_kbc_aux = false;
		} else
			m_kbd_line = 0;
	}
	kbc_update_irq();
}

void viewstation_state::kbc_update_irq()
{
	m_maincpu->set_input_line(I960CA_XINT2, (m_kbc_obf && !m_kbc_aux && BIT(m_kbc_cmd, 0)) ? ASSERT_LINE : CLEAR_LINE);
	m_maincpu->set_input_line(I960CA_XINT3, (m_kbc_obf && m_kbc_aux && BIT(m_kbc_cmd, 1)) ? ASSERT_LINE : CLEAR_LINE);
}

uint8_t viewstation_state::kbc_status_r()
{
	if(!machine().side_effects_disabled())
		kbc_refill();
	return (m_kbc_obf ? 0x01 : 0) | 0x04 | (m_kbc_last_cmd ? 0x08 : 0) | 0x10 | ((m_kbc_obf && m_kbc_aux) ? 0x20 : 0);
}

uint8_t viewstation_state::kbc_data_r()
{
	const uint8_t data = m_kbc_out;
	if(!machine().side_effects_disabled()) {
		m_kbc_obf = false;
		kbc_refill();
	}
	return data;
}

void viewstation_state::kbc_command_w(uint8_t data)
{
	LOGMASKED(LOG_KBC, "%s: 8042 command %02x\n", machine().describe_context(), data);
	m_kbc_last_cmd = true;
	m_kbc_expect = 0;
	switch(data) {
	case 0x20: m_kbc_q.push_back(m_kbc_cmd); break;
	case 0xa7: m_kbc_cmd |= 0x20; break;
	case 0xa8: m_kbc_cmd &= ~0x20; break;
	case 0xa9: m_kbc_q.push_back(0x00); break;
	case 0xaa: m_kbc_q.push_back(0x55); break;
	case 0xab: m_kbc_q.push_back(0x00); break;
	case 0xad: m_kbc_cmd |= 0x10; break;
	case 0xae: m_kbc_cmd &= ~0x10; break;
	case 0xc0: m_kbc_q.push_back(0xbf); break;
	case 0xd0: m_kbc_q.push_back(0xdf); break;
	case 0x60: case 0xd1: case 0xd2: case 0xd3: case 0xd4: m_kbc_expect = data; break;
	default:
		LOGMASKED(LOG_UNKNOWN, "%s: 8042 command %02x ignored\n", machine().describe_context(), data);
		break;
	}
	kbc_refill();
}

void viewstation_state::kbc_data_w(uint8_t data)
{
	m_kbc_last_cmd = false;
	const uint8_t expect = m_kbc_expect;
	m_kbc_expect = 0;
	switch(expect) {
	case 0x60: m_kbc_cmd = data; break;
	case 0xd1: break;
	case 0xd2: m_kbc_q.push_back(data); break;
	case 0xd3: m_aux_q.push_back(data); break;
	case 0xd4: mouse_command(data); break;
	default:
		// scan code set 3 key type commands are only acknowledged
		if(data >= 0xf7 && data <= 0xfd)
			m_kbc_q.push_back(0xfa);
		else {
			m_atkbd->write(data);
			m_kbd_line = 1;
		}
		break;
	}
	kbc_refill();
}

void viewstation_state::mouse_command(uint8_t data)
{
	LOGMASKED(LOG_KBC, "%s: mouse command %02x\n", machine().describe_context(), data);
	if(m_mouse_expect) {
		m_mouse_expect = 0;
		m_aux_q.push_back(0xfa);
		return;
	}
	switch(data) {
	case 0xff: m_mouse_on = false; m_aux_q.insert(m_aux_q.end(), { 0xfa, 0xaa, 0x00 }); break;
	case 0xf6: case 0xf5: m_mouse_on = false; m_aux_q.push_back(0xfa); break;
	case 0xf4: m_mouse_on = true; m_aux_q.push_back(0xfa); break;
	case 0xf3: case 0xe8: m_mouse_expect = data; m_aux_q.push_back(0xfa); break;
	case 0xf2: m_aux_q.insert(m_aux_q.end(), { 0xfa, 0x00 }); break;
	case 0xe9: m_aux_q.insert(m_aux_q.end(), { 0xfa, 0x00, 0x02, 0x64 }); break;
	case 0xee: m_aux_q.push_back(0xee); break;
	default: m_aux_q.push_back(0xfa); break;
	}
}

TIMER_CALLBACK_MEMBER(viewstation_state::mouse_poll)
{
	const uint16_t x = m_mouse_x->read(), y = m_mouse_y->read();
	const uint8_t btn = m_mouse_btn->read() & 7;
	int dx = int16_t(x - m_mouse_lastx), dy = int16_t(m_mouse_lasty - y);
	if(!m_mouse_on) {
		m_mouse_lastx = x;
		m_mouse_lasty = y;
		return;
	}
	// while the guest is behind, keep the movement for a later packet rather than lose it
	if(m_aux_q.size() > 12 || (!dx && !dy && btn == m_mouse_lastbtn))
		return;
	m_mouse_lastbtn = btn;
	LOGMASKED(LOG_KBC, "mouse packet %d %d %x\n", dx, dy, btn);
	dx = std::clamp(dx, -255, 255);
	dy = std::clamp(dy, -255, 255);
	m_mouse_lastx += dx;
	m_mouse_lasty -= dy;
	m_aux_q.push_back(0x08 | btn | (dx < 0 ? 0x10 : 0) | (dy < 0 ? 0x20 : 0));
	m_aux_q.push_back(dx & 0xff);
	m_aux_q.push_back(dy & 0xff);
	kbc_refill();
}

void viewstation_state::blockwrite_w(offs_t offset, uint32_t data, uint32_t mem_mask)
{
	// each word writes the block colour to the 16 pixels whose mask bits are set; the
	// software reaches the second 16 pixels of a 32-pixel group at +4 (in a burst), +0x10
	// or +0x14, and the words of a quad store step 16 pixels each
	data &= mem_mask;
	const offs_t w = offset << 2;
	const offs_t base = ((w & ~0x1f) + (BIT(w, 3) << 5) + ((BIT(w, 2) | BIT(w, 4)) << 4)) & 0x1fffff;
	uint8_t *vram = reinterpret_cast<uint8_t *>(m_vram.target());
	for(int i = 0; i < 16; i++)
		if(BIT(data, i)) {
			const offs_t a = base + i;
			vram[BYTE4_XOR_LE(a)] = m_blkcolor[BIT(a, 2)] >> (8 * (a & 3));
		}
}

uint32_t viewstation_state::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	// INMOS G300 style colour video controller: palette words 000-0ff hold 0x00bbggrr,
	// 140 is the pixel mask, 180 the top of screen; lines are 2048 bytes apart
	const uint8_t *vram = reinterpret_cast<const uint8_t *>(m_vram.target());
	const uint32_t tos = m_cvc_regs[0x180] & 0x1fffff;
	if(m_colour) {
		const pen_t *pens = m_ramdac->pens();
		const uint8_t mask = m_ramdac->read(2);
		for(int y = cliprect.min_y; y <= cliprect.max_y; y++) {
			uint32_t *dst = &bitmap.pix(y);
			for(int x = cliprect.min_x; x <= cliprect.max_x; x++)
				dst[x] = pens[vram[BYTE4_XOR_LE((tos + y * 2048 + x) & 0x1fffff)] & mask];
		}
		return 0;
	}
	// G300 palette words hold red in the low byte
	const uint8_t mask = m_cvc_regs[0x140];
	for(int y = cliprect.min_y; y <= cliprect.max_y; y++) {
		uint32_t *dst = &bitmap.pix(y);
		for(int x = cliprect.min_x; x <= cliprect.max_x; x++) {
			const uint32_t c = m_cvc_regs[vram[BYTE4_XOR_LE((tos + y * 2048 + x) & 0x1fffff)] & mask];
			dst[x] = rgb_t(c & 0xff, (c >> 8) & 0xff, (c >> 16) & 0xff);
		}
	}
	return 0;
}

void viewstation_state::mem_map(address_map &map)
{
	map(0x00000000, 0x000003ff).ram().share(m_iram);
	map(0x21800000, 0x219fffff).ram().share(m_vram);
	// VRAM block write: colour registers for the two interleaved banks, and a window
	// where each 32-bit word is a mask for 32 pixels at the same offset in VRAM
	map(0x21a00000, 0x21bfffff).ram();
	map(0x21c00000, 0x21dfffff).w(FUNC(viewstation_state::blockwrite_w));
	map(0x21e00000, 0x21ffffff).lw32(NAME([this] (offs_t offset, uint32_t data, uint32_t mem_mask) {
		COMBINE_DATA(&m_blkcolor[(offset == 0x7ffff) ? 1 : 0]);
	}));
	// banks do not decode the address lines above their size, so they repeat through their 128 MB window
	map(0x30000000, 0x307fffff).ram().share(m_dram).mirror(0x07800000);
	map(0xa0000000, 0xefffffff).rw(FUNC(viewstation_state::unk_r), FUNC(viewstation_state::unk_w));
	map(0xc0000000, 0xc0000fff).lrw32(
		NAME([this] (offs_t offset, uint32_t mem_mask) {
			if(!machine().side_effects_disabled()) LOGMASKED(LOG_DAC, "%s: cvc r %03x & %08x\n", machine().describe_context(), offset, mem_mask);
			if(m_colour && offset < 4) {
				uint32_t data = 0;
				for(int i = 0; i < 4; i++)
					if((mem_mask >> (8 * i)) & 0xff)
						data |= (machine().side_effects_disabled() ? 0 : dac_r(offset * 4 + i)) << (8 * i);
				return data;
			}
			return m_cvc_regs[offset & 0x3ff];
		}),
		NAME([this] (offs_t offset, uint32_t data, uint32_t mem_mask) {
			LOGMASKED(LOG_DAC, "%s: cvc w %03x = %08x & %08x\n", machine().describe_context(), offset, data, mem_mask);
			if(m_colour && offset < 4) {
				for(int i = 0; i < 4; i++)
					if((mem_mask >> (8 * i)) & 0xff)
						dac_w(offset * 4 + i, data >> (8 * i));
				return;
			}
			COMBINE_DATA(&m_cvc_regs[offset & 0x3ff]);
		})).mirror(0x08000000);
	map(0xd6000000, 0xd6000003).rw(m_pit, FUNC(pit8254_device::read), FUNC(pit8254_device::write));
	map(0xda000000, 0xda00003f).r(FUNC(viewstation_state::mac_r));
	map(0xd8000000, 0xd8007fff).ram().share("nvram");
	map(0xe0000000, 0xe00007ff).rw(FUNC(viewstation_state::ata8_r), FUNC(viewstation_state::ata8_w));
	map(0xe02001f0, 0xe02001f7).rw(FUNC(viewstation_state::ata_r), FUNC(viewstation_state::ata_w));
	map(0xe02003f6, 0xe02003f6).lrw8(
		NAME([this] () { return uint8_t(m_ata->cs1_r(6)); }),
		NAME([this] (uint8_t data) { m_ata->cs1_w(6, data); }));
	map(0xd0000000, 0xd0000000).rw(FUNC(viewstation_state::kbc_data_r), FUNC(viewstation_state::kbc_data_w));
	map(0xd0000001, 0xd0000001).rw(FUNC(viewstation_state::kbc_status_r), FUNC(viewstation_state::kbc_command_w));
	map(0xd2000000, 0xd3ffffff).rw(FUNC(viewstation_state::duart_r), FUNC(viewstation_state::duart_w));
	map(0x10000000, 0x10000003).lw32(NAME([this] (uint32_t data) { m_eth->port(data); }));
	map(0x10000004, 0x10000007).lw32(NAME([this] (uint32_t data) { m_eth->ca(1); m_eth->ca(0); }));
	map(0xfefc0000, 0xfeffffff).rom().region("bootprom", 0);
	map(0xfffc0000, 0xffffffff).rom().region("bootprom", 0);
}

// the 82596 masters the main CPU bus
void viewstation_state::eth_map(address_map &map)
{
	map(0x00000000, 0xffffffff).lrw32(
		NAME([this] (offs_t offset, uint32_t mem_mask) { return m_maincpu->space(AS_PROGRAM).read_dword(offset << 2, mem_mask); }),
		NAME([this] (offs_t offset, uint32_t data, uint32_t mem_mask) { m_maincpu->space(AS_PROGRAM).write_dword(offset << 2, data, mem_mask); }));
}

void viewstation_state::hdsfx(machine_config &config)
{
	I80960CA(config, m_maincpu, 33_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &viewstation_state::mem_map);

	// 8 MB on the main board plus a 16, 32 or 64 MB SIMM
	RAM(config, m_ram).set_default_size("24M").set_extra_options("40M,72M");

	screen_device &screen(SCREEN(config, "screen"));
	screen.set_refresh_hz(60);
	screen.set_size(1280, 1024);
	screen.set_visarea_full();
	screen.set_screen_update(FUNC(viewstation_state::screen_update));
	// vertical retrace on XINT1 (edge triggered); netOS updates the palette there
	//screen.screen_vblank().set_inputline(m_maincpu, I960CA_XINT1);

	TLC34076(config, m_ramdac, tlc34076_device::TLC34076_8_BIT);

	// system clock on XINT6 (active low); the boot PROM programs 36864 counts for 100 Hz
	PIT8254(config, m_pit);
	m_pit->set_clk<0>(3.6864_MHz_XTAL);
	m_pit->set_clk<1>(3.6864_MHz_XTAL);
	m_pit->set_clk<2>(3.6864_MHz_XTAL);
	m_pit->out_handler<0>().set_inputline(m_maincpu, I960CA_XINT6).invert();

	// serial ports; the output port drives the LEDs and the speaker
	SCN2681(config, m_duart, 3.6864_MHz_XTAL);
	m_duart->irq_cb().set_inputline(m_maincpu, I960CA_XINT0);
	m_duart->a_tx_cb().set("serial0", FUNC(rs232_port_device::write_txd));
	m_duart->b_tx_cb().set("serial1", FUNC(rs232_port_device::write_txd));
	rs232_port_device &serial0(RS232_PORT(config, "serial0", default_rs232_devices, nullptr));
	serial0.rxd_handler().set(m_duart, FUNC(scn2681_device::rx_a_w));
	rs232_port_device &serial1(RS232_PORT(config, "serial1", default_rs232_devices, nullptr));
	serial1.rxd_handler().set(m_duart, FUNC(scn2681_device::rx_b_w));

	// keyboard behind the 8042-style controller, scan code set 3
	AT_KEYB(config, m_atkbd, pc_keyboard_device::KEYBOARD_TYPE::AT, 3);
	m_atkbd->keypress().set([this] (int state) { m_kbd_line = state; if(state) kbc_refill(); });

	// setup memory; the boot PROM sizes it by writing 8 at 0 and 2 at 0x1800
	NVRAM(config, "nvram", nvram_device::DEFAULT_ALL_0);

	ATA_INTERFACE(config, m_ata).options(ata_devices, "hdd", nullptr, false);

	I82596_LE32(config, m_eth, 25_MHz_XTAL);
	// TODO: interrupt pin not identified yet
	m_eth->set_addrmap(0, &viewstation_state::eth_map);
}

// The monitor switches select the video mode: the boot PROM takes the first entry of its mode
// table for the monitor type and the clock that the mode switch picks. netOS follows it.
void viewstation_state::hdsfx_v16c(machine_config &config)
{
	hdsfx(config);
	subdevice<screen_device>("screen")->set_visarea(0, 1152 - 1, 0, 900 - 1);
	m_monitor_sw = 3;
}

void viewstation_state::hdsfx_v14c(machine_config &config)
{
	hdsfx(config);
	subdevice<screen_device>("screen")->set_visarea(0, 1024 - 1, 0, 768 - 1);
	m_monitor_sw = 5;
}

void viewstation_state::hdsfx_vesa(machine_config &config)
{
	hdsfx(config);
	subdevice<screen_device>("screen")->set_visarea(0, 800 - 1, 0, 600 - 1);
	m_monitor_sw = 9;
	m_mode_sw = 8;
}

static INPUT_PORTS_START(hdsfx)
	PORT_START("MOUSEX")
	PORT_BIT(0xffff, 0, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(1)
	PORT_START("MOUSEY")
	PORT_BIT(0xffff, 0, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(1)
	PORT_START("MOUSEBTN")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("Mouse Left")
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("Mouse Right")
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("Mouse Middle")
	PORT_START("CONFIG")
	PORT_CONFNAME(0x01, 0x01, "Video board")
	PORT_CONFSETTING(0x00, "G300")
	PORT_CONFSETTING(0x01, "FX with TLC34075 (colour)")
INPUT_PORTS_END

ROM_START(hdsfx)
	ROM_REGION32_LE(0x40000, "bootprom", 0)
	ROM_SYSTEM_BIOS(0, "v32", "netOS 3.2 (Build 563)")
	ROMX_LOAD("v32-bootprom.bin", 0x00000, 0x40000, CRC(2462f9c9) SHA1(a0fa0b9dd67e79da477dae80327aefd11007d9b6), ROM_BIOS(0))
ROM_END

#define rom_hdsfxv16 rom_hdsfx
#define rom_hdsfxv14 rom_hdsfx
#define rom_hdsfxvesa rom_hdsfx

} // anonymous namespace

//    YEAR  NAME   PARENT  COMPAT  MACHINE  INPUT  CLASS              INIT        COMPANY                     FULLNAME                               FLAGS
COMP( 1997, hdsfx, 0,      0,      hdsfx,   hdsfx, viewstation_state, empty_init, "HDS Network Systems / Neoware", "ViewStation FX / @workStation (i960), V19C monitor 1280x1024", MACHINE_NO_SOUND | MACHINE_SUPPORTS_SAVE )
COMP( 1997, hdsfxv16,  hdsfx, 0,  hdsfx_v16c, hdsfx, viewstation_state, empty_init, "HDS Network Systems / Neoware", "ViewStation FX / @workStation (i960), V16C monitor 1152x900", MACHINE_NO_SOUND | MACHINE_SUPPORTS_SAVE )
COMP( 1997, hdsfxv14,  hdsfx, 0,  hdsfx_v14c, hdsfx, viewstation_state, empty_init, "HDS Network Systems / Neoware", "ViewStation FX / @workStation (i960), V14C monitor 1024x768", MACHINE_NO_SOUND | MACHINE_SUPPORTS_SAVE )
COMP( 1997, hdsfxvesa, hdsfx, 0,  hdsfx_vesa, hdsfx, viewstation_state, empty_init, "HDS Network Systems / Neoware", "ViewStation FX / @workStation (i960), VESA monitor 800x600", MACHINE_NO_SOUND | MACHINE_SUPPORTS_SAVE )
