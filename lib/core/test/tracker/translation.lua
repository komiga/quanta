
local U = require "togo.utility"
local O = require "Quanta.Object"
local Tracker = require "Quanta.Tracker"
local Director = require "Quanta.Director"

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
	Entry{range = 01:00 - 02:00, actions = {
		ETODO
		ETODO{"x"}
		ETODO{d = "y"}
	}};
}}]==],
"2016-01-01Z", {
	make_tracker_entry(
		false,
		make_tracker_entry_time("2016-01-01T01:00:00Z", 0, true),
		make_tracker_entry_time("2016-01-01T02:00:00Z", 0, true),
		{},
		{},
		nil, nil,
		1, {
			make_tracker_action("ETODO", Tracker.PlaceholderAction()),
			make_tracker_action("ETODO", Tracker.PlaceholderAction("x")),
			make_tracker_action("ETODO", Tracker.PlaceholderAction("y")),
		}
	),
}),

make_test(
[==[Tracker{date = 2016-01-01Z, entries = {
	Entry{range = 01:00 - 02:00, actions = {
		ETODO
		ETODO:action_primary
		ETODO
	}};
}}]==],
"2016-01-01Z", {
	make_tracker_entry(
		false,
		make_tracker_entry_time("2016-01-01T01:00:00Z", 0, true),
		make_tracker_entry_time("2016-01-01T02:00:00Z", 0, true),
		{},
		{},
		nil, nil,
		2, {
			make_tracker_action("ETODO", Tracker.PlaceholderAction()),
			make_tracker_action("ETODO", Tracker.PlaceholderAction()),
			make_tracker_action("ETODO", Tracker.PlaceholderAction()),
		}
	),
}),
make_test(
[==[Tracker{date = 2016-01-01Z, entries = {
	Entry{range = 01:00 - 02:00, actions = {
		ETODO
		ETODO
		ETODO:action_primary
	}};
}}]==],
"2016-01-01Z", {
	make_tracker_entry(
		false,
		make_tracker_entry_time("2016-01-01T01:00:00Z", 0, true),
		make_tracker_entry_time("2016-01-01T02:00:00Z", 0, true),
		{},
		{},
		nil, nil,
		3, {
			make_tracker_action("ETODO", Tracker.PlaceholderAction()),
			make_tracker_action("ETODO", Tracker.PlaceholderAction()),
			make_tracker_action("ETODO", Tracker.PlaceholderAction()),
		}
	),
}),

make_test_fail(
[==[Tracker{date = 2016-01-01Z, entries = {
	Entry{range = 01:00 - 02:00, actions = {
		ETODO:action_primary
		ETODO:action_primary
	}};
}}]==]
),

make_test(
[==[Tracker{date = 2016-01-01Z, entries = {
	Entry{range = 01:00:00 - ENEXT, actions = {ETODO}};
	Entry{range = 02:00:00 - 04:00, actions = {ETODO}};
}}]==],
"2016-01-01Z", {
	make_tracker_entry(
		false,
		make_tracker_entry_time("2016-01-01T01:00:00Z", 0, true),
		make_tracker_entry_time("2016-01-01T02:00:00Z", 0, true),
		{},
		{},
		nil, nil,
		1, {
			make_tracker_action("ETODO", Tracker.PlaceholderAction()),
		}
	),
	make_tracker_entry(
		false,
		make_tracker_entry_time("2016-01-01T02:00:00Z", 0, true),
		make_tracker_entry_time("2016-01-01T04:00:00Z", 0, true),
		{},
		{},
		nil, nil,
		1, {
			make_tracker_action("ETODO", Tracker.PlaceholderAction()),
		}
	),
}),
make_test(
[==[Tracker{date = 2016-01-01Z, entries = {
	Entry{range = 01:00:00 - ENEXT, actions = {ETODO}};
	Entry:ool{range = EPREV - ENEXT, actions = {ETODO}};
	Entry{range = 02:00:00 - 04:00, actions = {ETODO}};
	Entry:ool{range = EPREV:ool - 03:00, actions = {ETODO}};
}}]==],
"2016-01-01Z", {
	make_tracker_entry(
		false,
		make_tracker_entry_time("2016-01-01T01:00:00Z", 0, true),
		make_tracker_entry_time("2016-01-01T02:00:00Z", 0, true),
		{},
		{},
		nil, nil,
		1, {
			make_tracker_action("ETODO", Tracker.PlaceholderAction()),
		}
	),
	make_tracker_entry(
		true,
		make_tracker_entry_time("2016-01-01T01:00:00Z", 0, true),
		make_tracker_entry_time("2016-01-01T02:00:00Z", 0, true),
		{},
		{},
		nil, nil,
		1, {
			make_tracker_action("ETODO", Tracker.PlaceholderAction()),
		}
	),
	make_tracker_entry(
		false,
		make_tracker_entry_time("2016-01-01T02:00:00Z", 0, true),
		make_tracker_entry_time("2016-01-01T04:00:00Z", 0, true),
		{},
		{},
		nil, nil,
		1, {
			make_tracker_action("ETODO", Tracker.PlaceholderAction()),
		}
	),
	make_tracker_entry(
		true,
		make_tracker_entry_time("2016-01-01T01:00:00Z", 0, true),
		make_tracker_entry_time("2016-01-01T03:00:00Z", 0, true),
		{},
		{},
		nil, nil,
		1, {
			make_tracker_action("ETODO", Tracker.PlaceholderAction()),
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
	local obj = O.create(t.text)
	U.assert(obj ~= nil)

	local tracker = Tracker()
	local success, msg = tracker:from_object(obj, director)
	if not success then
		U.print("translation error: %s", msg)
	end
	U.assert(success == not not t.tracker, "unexpected success value: %s", success)
	if t.tracker then
		--tracker:to_object(obj)
		--U.print("%s", O.write_text_string(obj, true))
		check_tracker_equal(tracker, t.tracker)
	else
		U.print("(expected)")
	end
end

function main()
	Director.debug = true

	local director = Director()
	director:register_action("ETODO", Tracker.PlaceholderAction)

	for i, t in ipairs(translation_tests) do
		do_test(t, director)
	end

	return 0
end

return main()
