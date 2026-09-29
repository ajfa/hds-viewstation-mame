-- clickseq plus typing: TYPES="time=text;time=text" posted through the natural keyboard
-- ("\n" in the text is Enter)
local todo = {}
for t, s in string.gmatch(os.getenv("TYPES") or "", "([%d%.]+)=([^;]*)") do
	todo[#todo + 1] = { tonumber(t), (s:gsub("\\n", "\n")) }
end
emu.register_periodic(function()
	local t = manager.machine.time:as_double()
	if todo[1] and t >= todo[1][1] then
		manager.machine.natkeyboard:post(todo[1][2])
		table.remove(todo, 1)
	end
end)
local here = debug.getinfo(1, "S").source:match("^@(.*[/\\])") or ""
dofile(here .. "clickseq.lua")
