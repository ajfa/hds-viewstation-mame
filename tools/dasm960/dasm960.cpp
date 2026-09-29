// license:BSD-3-Clause
// copyright-holders:
// dasm960 file.bin load_address [start end]: disassemble i960 code with MAME's i960dis
#include "disasmintf.h"
#include "i960dis.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <iostream>

using u8 = uint8_t; using u16 = uint16_t; using u32 = uint32_t; using u64 = uint64_t; using offs_t = util::disasm_interface::offs_t;

struct buf : util::disasm_interface::data_buffer {
	std::vector<u8> d; u32 base;
	u8 g(u32 pc) const { u32 o = pc - base; return o < d.size() ? d[o] : 0; }
	u8 r8(offs_t pc) const override { return g(pc); }
	u16 r16(offs_t pc) const override { return g(pc) | g(pc+1) << 8; }
	u32 r32(offs_t pc) const override { return r16(pc) | u32(r16(pc+2)) << 16; }
	u64 r64(offs_t pc) const override { return r32(pc) | u64(r32(pc+4)) << 32; }
};

int main(int argc, char **argv)
{
	if (argc < 3) { fprintf(stderr, "usage: dasm960 file base [start end]\n"); return 1; }
	FILE *f = fopen(argv[1], "rb");
	buf b; b.base = strtoul(argv[2], nullptr, 0);
	int c; while ((c = fgetc(f)) != EOF) b.d.push_back(c);
	u32 s = argc > 3 ? strtoul(argv[3], nullptr, 0) : b.base;
	u32 e = argc > 4 ? strtoul(argv[4], nullptr, 0) : b.base + b.d.size();
	i960_disassembler dis;
	for (u32 pc = s; pc < e; ) {
		std::ostringstream os;
		u32 r = dis.disassemble(os, pc, b, b);
		u32 len = r & util::disasm_interface::LENGTHMASK; if (!len) len = 4;
		printf("%08x: %08x%s  %s\n", pc, b.r32(pc), len == 8 ? " " : "         ", os.str().c_str());
		if (len == 8) printf("          %08x\n", b.r32(pc+4));
		pc += len;
	}
}
