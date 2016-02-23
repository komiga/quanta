
local U = require "togo.utility"
local T = require "Quanta.Time"
local O = require "Quanta.Object"
local Vessel = require "Quanta.Vessel"
local Tracker = require "Quanta.Tracker"
local Director = require "Quanta.Director"

Director.debug = true

function do_tracker_file(director, path)
	local obj = O.create()
	if not O.read_text_file(obj, path, true) then
		U.log("error: failed to read file: %s", path)
		return false
	end
	local tracker = Tracker()
	local success, msg = tracker:from_object(obj, director)
	if success then
		tracker:to_object(obj)
		print(O.write_text_string(obj, true))
	else
		U.log("error: %s", msg)
		U.log("error: failed to translate file: %s", path)
	end
	return success
end

function main(argv)
	table.remove(argv, 1)
	Vessel.init(nil, false)

	if #argv == 0 then
		return do_tracker_file(director, Vessel.tracker_path(nil))
	else
		local t_now = T()
		T.set_date_now(t_now)
		local obj = O.create()
		for _, arg in ipairs(argv) do
			if O.read_text_string(obj, arg, true) and O.is_time(obj) then
				O.resolve_time(obj, t_now)
				if not do_tracker_file(director, Vessel.tracker_path(O.time(obj))) then
					return false
				end
			else
				U.log("error: failed to parse date: %s", arg)
				return false
			end
		end
	end
	return true
end

return main(...)
