-- move the pointer through CLICKS (x,y,time;... in screen pixels, starting from the centre
-- at 640,515) in unaccelerated single steps, clicking at each point; snapshot at SHOTS.
-- A point "x,y,timem" only moves there, without clicking.
-- A point "H,time" re-anchors: it pushes the pointer into the top left corner (0,0), which
-- corrects any movement the guest dropped while busy.
local fx, fy, fb
for _, f in pairs(manager.machine.ioport.ports[":MOUSEX"].fields) do fx = f end
for _, f in pairs(manager.machine.ioport.ports[":MOUSEY"].fields) do fy = f end
for n, f in pairs(manager.machine.ioport.ports[":MOUSEBTN"].fields) do if n == "Mouse Left" then fb = f end end
local points = {}
for item in string.gmatch(os.getenv("CLICKS") or "963,480,200", "[^;]+") do
	local t = item:match("^H,([%d%.]+)$")
	if t then
		points[#points + 1] = { home = true, t = tonumber(t) }
	else
		local x, y, tt, m = item:match("(%d+),(%d+),([%d%.]+)(m?)")
		points[#points + 1] = { sx = tonumber(x), sy = tonumber(y), t = tonumber(tt), move = (m == "m") }
	end
end
local shots = {}
for t in string.gmatch(os.getenv("SHOTS") or "240", "([%d%.]+)") do shots[#shots + 1] = tonumber(t) end
-- field value = offset + screen coordinate
local ox, oy = -640, -515
local x, y, idx, hold = 0, 0, 1, 0
emu.register_frame_done(function()
	local t = manager.machine.time:as_double()
	local p = points[idx]
	if p and t >= p.t then
		if p.home then
			if hold == 0 then
				x = x - 3000; y = y - 3000
				fx:set_value(x & 0xffff); fy:set_value(y & 0xffff)
			end
			hold = hold + 1
			if hold == 30 then ox, oy = x, y; idx = idx + 1; hold = 0 end
		else
			local tx, ty = ox + p.sx, oy + p.sy
			if x ~= tx then x = x + (tx > x and 1 or -1) end
			if y ~= ty then y = y + (ty > y and 1 or -1) end
			fx:set_value(x & 0xffff)
			fy:set_value(y & 0xffff)
			if x == tx and y == ty then
				hold = hold + 1
				if hold == 10 and not p.move then fb:set_value(1) end
				if hold == 16 then fb:set_value(0); idx = idx + 1; hold = 0 end
			end
		end
	end
	if shots[1] and t >= shots[1] then
		table.remove(shots, 1)
		manager.machine.video:snapshot()
	end
end)
