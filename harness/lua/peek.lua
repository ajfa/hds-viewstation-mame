-- print words at PEEK="addr addr ..." every PEEK_EVERY seconds (default 30)
local sp = manager.machine.devices[":maincpu"].spaces["program"]
local addrs = {}
for a in string.gmatch(os.getenv("PEEK") or "302bc970", "(%x+)") do addrs[#addrs + 1] = tonumber(a, 16) end
local every = tonumber(os.getenv("PEEK_EVERY") or "30")
local nextt = every
emu.register_periodic(function()
	local t = manager.machine.time:as_double()
	if t < nextt then return end
	nextt = nextt + every
	local s = string.format("t=%.0f PEEK", t)
	for _, a in ipairs(addrs) do s = s .. string.format(" %08x=%08x", a, sp:read_u32(a)) end
	print(s)
end)
