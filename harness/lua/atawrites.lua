-- clicktype plus a log of IDE write commands (0x30) with their time and LBA
local sp = manager.machine.devices[":maincpu"].spaces["program"]
local tf = {}
wtap = sp:install_write_tap(0xe02001f0, 0xe02001f7, "ataw", function(o, d, m)
	local lane = 0
	while m ~= 0 and (m & 0xff) == 0 do m = m >> 8; lane = lane + 1 end
	local reg = (o & 7) + lane
	local v = (d >> (8 * lane)) & 0xff
	tf[reg] = v
	if reg == 7 and v == 0x30 then
		print(string.format("t=%.2f WRITE lba %02x%02x%02x%02x cnt %d", manager.machine.time:as_double(),
			(tf[6] or 0) & 15, tf[5] or 0, tf[4] or 0, tf[3] or 0, tf[2] or 0))
	end
end)
local here = debug.getinfo(1, "S").source:match("^@(.*[/\\])") or ""
dofile(here .. "clicktype.lua")
