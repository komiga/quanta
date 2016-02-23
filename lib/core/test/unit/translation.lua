
local U = require "togo.utility"
local O = require "Quanta.Object"
local Prop = require "Quanta.Prop"
local Measurement = require "Quanta.Measurement"
local Instance = require "Quanta.Instance"
local Unit = require "Quanta.Unit"
local Director = require "Quanta.Director"

require "common"

function make_test(text, type, name, description, author, note, elements_generic, elements_primary)
	return {
		text = text,
		unit = make_unit(type, name, description, author, note, elements_generic, elements_primary),
	}
end

function make_test_fail(text)
	return {
		text = text,
		unit = nil,
	}
end

local translation_tests = {
make_test(
	[[{}]],
	Unit.Type.none,
	"",
	"", {}, {},
	{}, {}
),
make_test(
	[[S{}]],
	Unit.Type.self,
	"",
	"", {}, {},
	{}, {}
),

make_test(
	[[name = {}]],
	Unit.Type.none,
	"name",
	"", {}, {},
	{}, {}
),

make_test(
	[[{d = "x"}]],
	Unit.Type.none,
	"",
	"x", {}, {},
	{}, {}
),

make_test(
	[[{author = "X%Y"}]],
	Unit.Type.none,
	"",
	"", {Prop.Author("X%Y", true, nil, true)}, {},
	{}, {}
),
make_test(
	[[{author = {"X%Y"}}]],
	Unit.Type.none,
	"",
	"", {Prop.Author("X%Y", true, nil, true)}, {},
	{}, {}
),
make_test(
	[[{author = {"X%Y", "Z%W"}}]],
	Unit.Type.none,
	"",
	"", {
		Prop.Author("X%Y", true, nil, true),
		Prop.Author("Z%W", true, nil, true),
	}, {},
	{}, {}
),

make_test(
	[[{note = "x"}]],
	Unit.Type.none,
	"",
	"", {}, {Prop.Note("x")},
	{}, {}
),
make_test(
	[[{note = {01:23, "x"}}]],
	Unit.Type.none,
	"",
	"", {}, {Prop.Note("x", make_time("2016-01-01T01:23Z"))},
	{}, {}
),
make_test(
	[[{note = {{02T01:23, "x"}, "y"}}]],
	Unit.Type.none,
	"",
	"", {}, {
		Prop.Note("x", make_time("2016-01-02T01:23Z")),
		Prop.Note("y"),
	},
	{}, {}
),

make_test(
	[[{
		P1 = {
			d = "blah"
			x
		}
	}]],
	Unit.Type.none,
	"",
	"", {}, {},
	{}, {
		make_element_primary(1, "blah", {}, {}, {
			make_step(1, Measurement(), {
				make_instance("x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
			}),
		}),
	}
),

make_test(
	[[{
		x
	}]],
	Unit.Type.none,
	"",
	"", {}, {},
	{}, {
		make_element_primary(1, "", {}, {}, {
			make_step(1, Measurement(), {
				make_instance("x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
			}),
		}),
	}
),

make_test(
	[[{
		P1 = {x}
	}]],
	Unit.Type.none,
	"",
	"", {}, {},
	{}, {
		make_element_primary(1, "", {}, {}, {
			make_step(1, Measurement(), {
				make_instance("x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
			}),
		}),
	}
),
make_test(
	[[{
		P1 = {
			RS1{x}
		}
	}]],
	Unit.Type.none,
	"",
	"", {}, {},
	{}, {
		make_element_primary(1, "", {}, {}, {
			make_step(1, Measurement(), {
				make_instance("x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
			}),
		}),
	}
),
make_test_fail(
	[[{
		P1 = {}
	}]]
),
make_test_fail(
	[[{
		P1 = {
			RS1{}
		}
	}]]
),
}

function do_test(t, implicit_scope, director)
	local obj = O.create(t.text)
	U.assert(obj ~= nil)
	local text_rewrite = O.write_text_string(obj, true)
	U.print("%s  =>", text_rewrite)

	local unit = Unit()
	local success, msg = unit:from_object(obj, implicit_scope, director)
	if not success then
		U.print("translation error: %s", msg)
	end
	U.assert(success == not not t.unit, "unexpected success value: %s", success)
	if t.unit then
		unit:to_object(obj)
		U.print("%s", O.write_text_string(obj, true))
		check_unit_equal(unit, t.unit)
	else
		U.print("(expected)")
	end
end

function main()
	Director.debug = true

	local implicit_scope = make_time("2016-01-01Z")
	local director = Director()
	for _, t in pairs(translation_tests) do
		do_test(t, implicit_scope, director)
	end

	return 0
end

return main()
