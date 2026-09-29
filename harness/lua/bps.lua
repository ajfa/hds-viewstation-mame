-- runs the debugger commands in the file named by BPS (one per line) at startup; needs -debug
-- (with -debugger none), and samples the PC like pcwatch.lua
local f = assert(io.open(os.getenv("BPS")), "set BPS to a file of debugger commands")
local dbg = manager.machine.debugger
local cmds = {}
for l in f:lines() do cmds[#cmds + 1] = l end
f:close()
local done = false
emu.register_periodic(function()
	if done then return end
	done = true
	for _, c in ipairs(cmds) do dbg:command(c) end
end)
local here = debug.getinfo(1, "S").source:match("^@(.*[/\\])") or ""
dofile(here .. "pcwatch.lua")
