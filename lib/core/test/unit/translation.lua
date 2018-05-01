
require "common"

local U = require "togo.utility"
local O = require "Quanta.Object"
local Prop = require "Quanta.Prop"
local Measurement = require "Quanta.Measurement"
local Unit = require "Quanta.Unit"
local Vessel = require "Quanta.Vessel"

function make_test(text, unit)
	return {
		text = text,
		unit = unit,
	}
end

function make_test_typed(text, unit)
	return {
		text = text,
		unit = unit,
		typed = true,
	}
end

function make_test_fail(text)
	return {
		text = text,
		unit = nil,
	}
end

local translation_tests = {
make_test_typed(
	"x",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
})),
make_test_typed(
	"x$1",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, nil, 1, 0, true, true, true, true, {}, {}, {}),
})),
make_test_typed(
	"x$1$2",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, nil, 1, 2, true, true, true, true, {}, {}, {}),
})),
make_test_typed(
	"{x, y}",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
	make_unit_ref(nil, "y", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
})),
make_test_typed(
	"x + y",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
	make_unit_ref(nil, "y", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
})),
make_test_typed(
	"{x[1], y[2]}[3]",
make_unit_comp(nil, {Measurement(3)}, {}, {
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {Measurement(1)}, {}, {}),
	make_unit_ref(nil, "y", nil, nil, 0, 0, true, true, true, true, {Measurement(2)}, {}, {}),
})),

make_test_typed(
	"x{P1[1], P2[1]}[2]",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {Measurement(2)}, {}, {
		make_unit_ref(nil, "P1", nil, nil, 0, 0, true, true, true, true, {Measurement(1)}, {}, {}),
		make_unit_ref(nil, "P2", nil, nil, 0, 0, true, true, true, true, {Measurement(1)}, {}, {}),
	}),
})),

make_test_typed(
	"{x[2], x[2ml], x[2kg]}",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {Measurement(2)}, {}, {}),
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {Measurement(2, "ml")}, {}, {}),
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {Measurement(2, "kg")}, {}, {}),
})),
make_test_typed(
	"{x[?1], x[G~1]}",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {Measurement(1, "", 0, 0, false)}, {}, {}),
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {Measurement(1, "", 0, 0, false)}, {}, {}),
})),
make_test_typed(
	"{x[~~1], x[^^1]}",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {Measurement(1, "", 0, -2)}, {}, {}),
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {Measurement(1, "", 0,  2)}, {}, {}),
})),
make_test_typed(
	"x[1, 2]",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {Measurement(1), Measurement(2)}, {}, {}),
})),
make_test_typed(
	"x[1/2g]",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {Measurement(2, "g", 1)}, {}, {}),
})),

make_test_typed(
	"{x$?, x$?$?, x$?1, x$?1$?2}",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, nil, 0, 0, false, true, true, true, {}, {}, {}),
	make_unit_ref(nil, "x", nil, nil, 0, 0, false, false, true, true, {}, {}, {}),
	make_unit_ref(nil, "x", nil, nil, 1, 0, false, true, true, true, {}, {}, {}),
	make_unit_ref(nil, "x", nil, nil, 1, 2, false, false, true, true, {}, {}, {}),
})),
make_test_typed(
	"{x_¿, x¿}",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, false, true, {}, {}, {}),
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, false, true, {}, {}, {}),
})),
make_test_typed(
	"?x",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, false, {}, {}, {}),
})),

make_test_typed(
	"x:m",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {}, {make_modifier("m", Unit.UnknownModifier())}, {}),
})),
make_test_typed(
	":m",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, nil, nil, nil, 0, 0, true, true, true, true, {}, {make_modifier("m", Unit.UnknownModifier())}, {}),
})),

make_test_typed(
	"03T{x}",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, "2016-01-03Z", 0, 0, true, true, true, true, {}, {}, {}),
})),
make_test_typed(
	"03T{x, y}",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, "2016-01-03Z", 0, 0, true, true, true, true, {}, {}, {}),
	make_unit_ref(nil, "y", nil, "2016-01-03Z", 0, 0, true, true, true, true, {}, {}, {}),
})),

make_test_typed(
	"02-03T{x}",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, "2016-02-03Z", 0, 0, true, true, true, true, {}, {}, {}),
})),
make_test_typed(
	"02-03T{x, y}",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, "2016-02-03Z", 0, 0, true, true, true, true, {}, {}, {}),
	make_unit_ref(nil, "y", nil, "2016-02-03Z", 0, 0, true, true, true, true, {}, {}, {}),
})),

make_test_typed(
	"2015-02-03{x}",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, "2015-02-03Z", 0, 0, true, true, true, true, {}, {}, {}),
})),
make_test_typed(
	"2015-02-03{x, y}",
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, "2015-02-03Z", 0, 0, true, true, true, true, {}, {}, {}),
	make_unit_ref(nil, "y", nil, "2015-02-03Z", 0, 0, true, true, true, true, {}, {}, {}),
})),

make_test_typed(
	[[U{}]],
make_unit_comp(nil, {}, {}, {
	make_unit_def(
		Unit.DefinitionType.none,
		"",
		"", {}, {},
		{}, {},
		{}
	),
})),
make_test_typed(
	[[{x, U{}}]],
make_unit_comp(nil, {}, {}, {
	make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
	make_unit_def(
		Unit.DefinitionType.none,
		"",
		"", {}, {},
		{}, {},
		{}
	),
})),

make_test_typed(
	[[U{
		d = "root"
		author = "X%Y",
		P1 = {
			d = "P1"
			RS1{x}
		}
	}]],
make_unit_comp(nil, {}, {}, {
	make_unit_def(
		Unit.DefinitionType.none,
		"",
		"root", {Prop.Author("X%Y", true, nil, true)}, {},
		{}, {},
		{
			make_element_primary(1, "P1", {}, {}, {
				make_step(1, false, {}, {}, {
					make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
				}),
			}),
		}
	),
})),

make_test(
	[[U{}]],
make_unit_def(
	Unit.DefinitionType.none,
	"",
	"", {}, {},
	{}, {},
	{}
)),
make_test(
	[[S{}]],
make_unit_def(
	Unit.DefinitionType.self,
	"",
	"", {}, {},
	{}, {},
	{}
)),

make_test(
	[[name = U{}]],
make_unit_def(
	Unit.DefinitionType.none,
	"name",
	"", {}, {},
	{}, {},
	{}
)),

make_test(
	[[U{d = "x"}]],
make_unit_def(
	Unit.DefinitionType.none,
	"",
	"x", {}, {},
	{}, {},
	{}
)),

make_test(
	[[U{author = "X%Y"}]],
make_unit_def(
	Unit.DefinitionType.none,
	"",
	"", {Prop.Author("X%Y", true, nil, true)}, {},
	{}, {},
	{}
)),
make_test(
	[[U{author = {"X%Y"}}]],
make_unit_def(
	Unit.DefinitionType.none,
	"",
	"", {Prop.Author("X%Y", true, nil, true)}, {},
	{}, {},
	{}
)),
make_test(
	[[U{author = {"X%Y", "Z%W"}}]],
make_unit_def(
	Unit.DefinitionType.none,
	"",
	"", {
		Prop.Author("X%Y", true, nil, true),
		Prop.Author("Z%W", true, nil, true),
	}, {},
	{}, {},
	{}
)),

make_test(
	[[U{note = "x"}]],
make_unit_def(
	Unit.DefinitionType.none,
	"",
	"", {}, {Prop.Note("x")},
	{}, {},
	{}
)),
make_test(
	[[U{note = {01:23, "x"}}]],
make_unit_def(
	Unit.DefinitionType.none,
	"",
	"", {}, {Prop.Note("x", make_time("2016-01-01T01:23Z"))},
	{}, {},
	{}
)),
make_test(
	[[U{note = {{02T01:23, "x"}, "y"}}]],
make_unit_def(
	Unit.DefinitionType.none,
	"",
	"", {}, {
		Prop.Note("x", make_time("2016-01-02T01:23Z")),
		Prop.Note("y"),
	},
	{}, {},
	{}
)),

make_test(
	[[U{
		P1 = {
			d = "blah"
			x
		}
	}]],
make_unit_def(
	Unit.DefinitionType.none,
	"",
	"", {}, {},
	{}, {},
	{
		make_element_primary(1, "blah", {}, {}, {
			make_step(1, false, {}, {}, {
				make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
			}),
		}),
	}
)),

make_test(
	[[U{
		x
	}]],
make_unit_def(
	Unit.DefinitionType.none,
	"",
	"", {}, {},
	{}, {},
	{
		make_element_primary(1, "", {}, {}, {
			make_step(1, false, {}, {}, {
				make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
			}),
		}),
	}
)),

make_test(
	[[U{
		x
		y
		:z
	}]],
make_unit_def(
	Unit.DefinitionType.none,
	"",
	"", {}, {},
	{}, {},
	{
		make_element_primary(1, "", {}, {}, {
			make_step(1, false, {}, {}, {
				make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
				make_unit_ref(nil, "y", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
				make_unit_ref(nil, nil, nil, nil, 0, 0, true, true, true, true, {}, {
					make_modifier("z", Unit.UnknownModifier())
				}, {}),
			}),
		}),
	}
)),

make_test(
	[[U{
		P1 = {x}
	}]],
make_unit_def(
	Unit.DefinitionType.none,
	"",
	"", {}, {},
	{}, {},
	{
		make_element_primary(1, "", {}, {}, {
			make_step(1, false, {}, {}, {
				make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
			}),
		}),
	}
)),
make_test(
	[[U{
		P1 = x
	}]],
make_unit_def(
	Unit.DefinitionType.none,
	"",
	"", {}, {},
	{}, {},
	{
		make_element_primary(1, "", {}, {}, {
			make_step(1, false, {}, {}, {
				make_unit_ref("prototype", "x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
			}),
		}),
	}
)),

make_test(
	[[U{
		P1 = {
			RS1{x}
		}
	}]],
make_unit_def(
	Unit.DefinitionType.none,
	"",
	"", {}, {},
	{}, {},
	{
		make_element_primary(1, "", {}, {}, {
			make_step(1, false, {}, {}, {
				make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
			}),
		}),
	}
)),
make_test(
	[[U{
		P1 = {
			RS1{
				FH{d = "stuff", author = "X%Y", x}
			}
			RS2{:cook}
		}
	}]],
make_unit_def(
	Unit.DefinitionType.none,
	"",
	"", {}, {},
	{}, {},
	{
		make_element_primary(1, "", {}, {}, {
			make_step(1, false, {}, {}, {
				make_unit_def(
					Unit.DefinitionType.familiar,
					"",
					"stuff", {Prop.Author("X%Y", true, nil, true)}, {},
					{}, {},
					{
						make_element_primary(1, "", {}, {}, {
							make_step(1, false, {}, {}, {
								make_unit_ref(nil, "x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
							}),
						}),
					}
				),
			}),
			make_step(2, false, {}, {}, {
				make_unit_ref(nil, nil, nil, nil, 0, 0, true, true, true, true, {}, {
					make_modifier("cook", Unit.UnknownModifier()),
				}, {}),
			}),
		}),
	}
)),

make_test_fail(
	[[U{
		P1 = {}
	}]]
),
make_test_fail(
	[[U{
		P1 = {
			RS1{}
		}
	}]]
),
}

function do_test(t, implicit_scope)
	local obj = O.create(t.text)
	U.assert(obj ~= nil)
	local text_rewrite = O.write_text_string(obj, true)
	U.print("%s  =>", text_rewrite)

	local unit = Unit()
	local translator = Unit.from_object
	if t.typed then
		U.assert(t.unit)
		unit.type = t.unit.type
		translator = Unit.from_object_by_type
	end
	local success, msg = translator(unit, obj, implicit_scope)
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
	Vessel.init("vessel_data")

	local implicit_scope = make_time("2016-01-01Z")
	for _, t in pairs(translation_tests) do
		do_test(t, implicit_scope)
	end

	return 0
end

return main()
