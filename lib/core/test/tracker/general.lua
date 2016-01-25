
local U = require "togo.utility"
local O = require "Quanta.Object"
local Director = require "Quanta.Director"
local Tracker = require "Quanta.Tracker"

require "common"

function make_test(text, date, entries)
	return {
		text = text,
		tracker = make_tracker(date, entries),
	}
end

function make_test_fail(text)
	return {
		text = text,
		tracker = nil,
	}
end

local translation_tests = {
make_test(
[==[Tracker{date = 2016-01-01Z}]==],
"2016-01-01Z", {}
),
make_test(
[==[Tracker{date = 2016-01-01Z, entries = {}}]==],
"2016-01-01Z", {}
),

make_test_fail(
[==[Tracker{date = 2016-01-01}]==]
),
make_test_fail(
[==[Tracker{date = 01-01T}]==]
),
make_test_fail(
[==[Tracker{date = 01T}]==]
),
make_test_fail(
[==[Tracker{date = 12:00}]==]
),
}

function do_test(t, director)
	local o = O.create(t.text)
	U.assert(o ~= nil)

	local tracker = Tracker()
	local success, msg = tracker:from_object(o, director)
	if not success then
		U.print("translation error: %s", msg)
	end
	U.assert(success == not not t.tracker, "unexpected success value: %s", success)
	if t.tracker then
		check_tracker_equal(tracker, t.tracker)
		--tracker:to_object(o)
		--U.print("%s", O.write_text_string(o, true))
	else
		U.print("(expected)")
	end
end

function main()
	local director = Director()
	for i, t in ipairs(translation_tests) do
		do_test(t, director)
	end

	return 0
end

return main()
