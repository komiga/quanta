
local U = require "Quanta.Util"
local O = require "Quanta.Object"
local Entity = require "Quanta.Entity"
local Measurement = require "Quanta.Measurement"
local Instance = require "Quanta.Instance"
local Composition = require "Quanta.Composition"
local Unit = require "Quanta.Unit"

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
	"", {"X%Y"}, {},
	{}, {}
),
make_test(
	[[{author = {"X%Y"}}]],
	Unit.Type.none,
	"",
	"", {"X%Y"}, {},
	{}, {}
),
make_test(
	[[{author = {"X%Y", "Z%W"}}]],
	Unit.Type.none,
	"",
	"", {"X%Y", "Z%W"}, {},
	{}, {}
),

make_test(
	[[{note = "x"}]],
	Unit.Type.none,
	"",
	"", {}, {Unit.Note("x")},
	{}, {}
),
make_test(
	[[{note = {01T00:00, "x"}}]],
	Unit.Type.none,
	"",
	"", {}, {Unit.Note("x", nil)},
	{}, {}
),
make_test(
	[[{note = {{01T00:00, "x"}, "y"}}]],
	Unit.Type.none,
	"",
	"", {}, {
		Unit.Note("x", nil),
		Unit.Note("y", nil),
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
				make_instance("x", nil, 0, 0, true, true, true, true, {}, {}, {}),
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
				make_instance("x", nil, 0, 0, true, true, true, true, {}, {}, {}),
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
				make_instance("x", nil, 0, 0, true, true, true, true, {}, {}, {}),
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
				make_instance("x", nil, 0, 0, true, true, true, true, {}, {}, {}),
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

function do_test(t, search_in, controllers)
	local o = O.create(t.text)
	U.assert(o ~= nil)
	local text_rewrite = O.write_text_string(o, true)
	U.print("%s  =>", text_rewrite)

	local unit = Unit()
	U.assert(unit:from_object(o, search_in, controllers) or not t.unit)
	if t.unit then
		check_unit_equal(unit, t.unit)
		unit:to_object(o)
		U.print("%s", O.write_text_string(o, true))
	else
		U.print("(expected)")
	end

	O.destroy(o)
end

function main()
	local search_in = {}
	local controllers = {}
	for _, t in pairs(translation_tests) do
		do_test(t, search_in, controllers)
	end

	return 0
end

return main()
