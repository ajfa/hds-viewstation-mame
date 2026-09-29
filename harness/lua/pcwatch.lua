-- samples the i960 PC and reports every change of 4 KB page, plus a final register dump
-- at exit also writes vram.bin (2 MB of VRAM) and dram.bin (the first MB of DRAM) to the current directory
local cpu = manager.machine.devices[":maincpu"]
local last = -1
local hist = {}
local function pc() return cpu.state["CURPC"].value end

emu.register_periodic(function()
	local p = pc()
	local page = p & 0xfffff000
	hist[page] = (hist[page] or 0) + 1
	if page ~= last then
		print(string.format("t=%.4f pc=%08x", manager.machine.time:as_double(), p))
		last = page
	end
end)

stop_sub = emu.add_machine_stop_notifier(function()
	manager.machine.video:snapshot()
	print("--- final state")
	for _, r in ipairs({"CURPC", "pc", "ac", "prcb", "ipnd", "imsk", "imap0", "imap1", "imap2", "icon", "pfp", "sp", "rip", "fp", "g0", "g1", "g8", "g13", "g14"}) do
		print(string.format("%-6s %08x", r, cpu.state[r].value))
	end
	local sp = cpu.spaces["program"]
	print(string.format("iram 10: %08x %08x %08x %08x", sp:read_u32(0x10), sp:read_u32(0x14), sp:read_u32(0x18), sp:read_u32(0x1c)))
	local prcb = cpu.state["prcb"].value
	local it = sp:read_u32(prcb + 16)
	local imap = {cpu.state["imap0"].value, cpu.state["imap1"].value}
	for pin = 0, 7 do
		local v = (((imap[pin // 4 + 1] >> ((pin % 4) * 4)) & 0xf) << 4) | 2
		print(string.format("xint%d vector %02x handler %08x", pin, v, sp:read_u32(it + 4 + 4 * v)))
	end
	local cv = {}
	for _, r in ipairs({0x121, 0x122, 0x123, 0x124, 0x125, 0x126, 0x127, 0x128, 0x129, 0x12a, 0x12b, 0x12c, 0x140, 0x160, 0x180}) do
		cv[#cv + 1] = string.format("%03x=%x", r, sp:read_u32(0xc0000000 + r * 4))
	end
	print("cvc " .. table.concat(cv, " "))
	local pal = {}
	for i = 0, 15 do pal[#pal + 1] = string.format("%06x", sp:read_u32(0xc0000000 + i * 4)) end
	print("pal " .. table.concat(pal, " "))
	local s = ""
	for a = 0x3006f470, 0x3006f570 do
		local c = sp:read_u8(a)
		if c == 0 then break end
		s = s .. string.char(c)
	end
	print("panic buffer: " .. s)
	local nz = 0
	for a = 0x21800000, 0x219ffffc, 4 do
		if sp:read_u32(a) ~= 0 then nz = nz + 1 end
	end
	print(string.format("vram nonzero words: %d", nz))
	local fv = io.open("vram.bin", "wb")
	for a = 0x21800000, 0x219ffffc, 4 do
		local v = sp:read_u32(a)
		fv:write(string.char(v & 0xff, (v >> 8) & 0xff, (v >> 16) & 0xff, (v >> 24) & 0xff))
	end
	fv:close()
	local f = io.open("dram.bin", "wb")
	for a = 0x30000000, 0x300ffffc, 4 do
		local v = sp:read_u32(a)
		f:write(string.char(v & 0xff, (v >> 8) & 0xff, (v >> 16) & 0xff, (v >> 24) & 0xff))
	end
	f:close()
	print("--- pc pages by samples")
	local t = {}
	for k, v in pairs(hist) do t[#t + 1] = {k, v} end
	table.sort(t, function(a, b) return a[2] > b[2] end)
	for i = 1, math.min(#t, 12) do print(string.format("%08x %d", t[i][1], t[i][2])) end
end)
