
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

make_test(
[==[Tracker{date = 2016-01-01Z, entries = {
	Entry{range = 01:00:00 - 02:30:00, actions = {
		ETODO
		ETODO{"x"}
		ETODO{d = "y"}
	}};
}}]==],
"2016-01-01Z", {
	make_tracker_entry(
		false,
		make_tracker_entry_time("2016-01-01T01:00:00Z", 0, true),
		make_tracker_entry_time("2016-01-01T02:30:00Z", 0, true),
		{},
		{},
		nil, nil,
		{
			make_tracker_action("ETODO", {description = ""}),
			make_tracker_action("ETODO", {description = "x"}),
			make_tracker_action("ETODO", {description = "y"}),
		}
	),
}),
make_test(
[==[Tracker{date = 2016-01-01Z, entries = {
	Entry{range = 01:00:00 - ENEXT, actions = {ETODO}};
	Entry{range = 02:30:00 - 04:00, actions = {ETODO}};
}}]==],
"2016-01-01Z", {
	make_tracker_entry(
		false,
		make_tracker_entry_time("2016-01-01T01:00:00Z", 0, true),
		make_tracker_entry_time("2016-01-01T02:30:00Z", 0, true),
		{},
		{},
		nil, nil,
		{
			make_tracker_action("ETODO", {description = ""}),
		}
	),
	make_tracker_entry(
		false,
		make_tracker_entry_time("2016-01-01T02:30:00Z", 0, true),
		make_tracker_entry_time("2016-01-01T04:00:00Z", 0, true),
		{},
		{},
		nil, nil,
		{
			make_tracker_action("ETODO", {description = ""}),
		}
	),
}),
make_test(
[==[Tracker{date = 2016-01-01Z, entries = {
	Entry{range = 01:00:00 - ENEXT, actions = {ETODO}};
	Entry:ool{range = EPREV - ENEXT, actions = {ETODO}};
	Entry{range = 02:30:00 - 04:00, actions = {ETODO}};
	Entry:ool{range = EPREV:ool - 03:00, actions = {ETODO}};
}}]==],
"2016-01-01Z", {
	make_tracker_entry(
		false,
		make_tracker_entry_time("2016-01-01T01:00:00Z", 0, true),
		make_tracker_entry_time("2016-01-01T02:30:00Z", 0, true),
		{},
		{},
		nil, nil,
		{
			make_tracker_action("ETODO", {description = ""}),
		}
	),
	make_tracker_entry(
		true,
		make_tracker_entry_time("2016-01-01T01:00:00Z", 0, true),
		make_tracker_entry_time("2016-01-01T02:30:00Z", 0, true),
		{},
		{},
		nil, nil,
		{
			make_tracker_action("ETODO", {description = ""}),
		}
	),
	make_tracker_entry(
		false,
		make_tracker_entry_time("2016-01-01T02:30:00Z", 0, true),
		make_tracker_entry_time("2016-01-01T04:00:00Z", 0, true),
		{},
		{},
		nil, nil,
		{
			make_tracker_action("ETODO", {description = ""}),
		}
	),
	make_tracker_entry(
		true,
		make_tracker_entry_time("2016-01-01T01:00:00Z", 0, true),
		make_tracker_entry_time("2016-01-01T03:00:00Z", 0, true),
		{},
		{},
		nil, nil,
		{
			make_tracker_action("ETODO", {description = ""}),
		}
	),
}),

make_test_fail(
[==[Tracker{date = 2016-01-01Z, entries = {
	Entry{};
}}]==]
),
make_test_fail(
[==[Tracker{date = 2016-01-01Z, entries = {
	Entry{range = 01:00 - 02:00};
}}]==]
),
make_test_fail(
[==[Tracker{date = 2016-01-01Z, entries = {
	Entry:ool{range = 01:00 - 02:00, actions = {ETODO}};
	Entry{range = EPREV - 03:00, actions = {ETODO}};
}}]==]
),
make_test_fail(
[==[Tracker{date = 2016-01-01Z, entries = {
	Entry{range = 01:00 - ENEXT, actions = {ETODO}};
	Entry:ool{range = 02:00 - 03:00, actions = {ETODO}};
}}]==]
),
make_test_fail(
[==[Tracker{date = 2016-01-01Z, entries = {
	Entry{range = 01:00 - 02:00, actions = {ETODO}};
	Entry{range = EPREV:ool - 03:00, actions = {ETODO}};
}}]==]
),
make_test_fail(
[==[Tracker{date = 2016-01-01Z, entries = {
	Entry{range = 01:00 - ENEXT:ool, actions = {ETODO}};
	Entry{range = 02:00 - 03:00, actions = {ETODO}};
}}]==]
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
