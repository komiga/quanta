
local U = require "togo.utility"
local O = require "Quanta.Object"
local Measurement = require "Quanta.Measurement"
local Instance = require "Quanta.Instance"
local Composition = require "Quanta.Composition"
local Director = require "Quanta.Director"

require "common"

function make_test(text, measurement, items)
	return {
		text = text,
		comp = make_composition(measurement, items),
	}
end

local translation_tests = {
make_test(
	"x",
	Measurement(), {
	make_instance("x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
}),
make_test(
	"x$1",
	Measurement(), {
	make_instance("x", nil, nil, 1, 0, true, true, true, true, {}, {}, {}),
}),
make_test(
	"x$1$2",
	Measurement(), {
	make_instance("x", nil, nil, 1, 2, true, true, true, true, {}, {}, {}),
}),
make_test(
	"{x, y}",
	Measurement(), {
	make_instance("x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
	make_instance("y", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
}),
make_test(
	"x + y",
	Measurement(), {
	make_instance("x", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
	make_instance("y", nil, nil, 0, 0, true, true, true, true, {}, {}, {}),
}),
make_test(
	"{x[1], y[2]}[3]",
	Measurement(3), {
	make_instance("x", nil, nil, 0, 0, true, true, true, true, {Measurement(1)}, {}, {}),
	make_instance("y", nil, nil, 0, 0, true, true, true, true, {Measurement(2)}, {}, {}),
}),

make_test(
	"x{P1[1], P2[1]}[2]",
	Measurement(), {
	make_instance("x", nil, nil, 0, 0, true, true, true, true, {Measurement(2)}, {
		make_instance("P1", nil, nil, 0, 0, true, true, true, true, {Measurement(1)}, {}, {}),
		make_instance("P2", nil, nil, 0, 0, true, true, true, true, {Measurement(1)}, {}, {}),
	}, {}),
}),

make_test(
	"{x[2], x[2ml], x[2kg]}",
	Measurement(), {
	make_instance("x", nil, nil, 0, 0, true, true, true, true, {Measurement(2)}, {}, {}),
	make_instance("x", nil, nil, 0, 0, true, true, true, true, {Measurement(2, "ml")}, {}, {}),
	make_instance("x", nil, nil, 0, 0, true, true, true, true, {Measurement(2, "kg")}, {}, {}),
}),
make_test(
	"{x[?1], x[G~1]}",
	Measurement(), {
	make_instance("x", nil, nil, 0, 0, true, true, true, true, {Measurement(1, "", 0, 0, false)}, {}, {}),
	make_instance("x", nil, nil, 0, 0, true, true, true, true, {Measurement(1, "", 0, 0, false)}, {}, {}),
}),
make_test(
	"{x[~~1], x[^^1]}",
	Measurement(), {
	make_instance("x", nil, nil, 0, 0, true, true, true, true, {Measurement(1, "", 0, -2)}, {}, {}),
	make_instance("x", nil, nil, 0, 0, true, true, true, true, {Measurement(1, "", 0,  2)}, {}, {}),
}),
make_test(
	"x[1, 2]",
	Measurement(), {
	make_instance("x", nil, nil, 0, 0, true, true, true, true, {Measurement(1), Measurement(2)}, {}, {}),
}),
make_test(
	"x[1/2g]",
	Measurement(), {
	make_instance("x", nil, nil, 0, 0, true, true, true, true, {Measurement(2, "g", 1)}, {}, {}),
}),

make_test(
	"{x$?, x$?$?, x$?1, x$?1$?2}",
	Measurement(), {
	make_instance("x", nil, nil, 0, 0, false, true, true, true, {}, {}, {}),
	make_instance("x", nil, nil, 0, 0, false, false, true, true, {}, {}, {}),
	make_instance("x", nil, nil, 1, 0, false, true, true, true, {}, {}, {}),
	make_instance("x", nil, nil, 1, 2, false, false, true, true, {}, {}, {}),
}),
make_test(
	"{x_¿, x¿}",
	Measurement(), {
	make_instance("x", nil, nil, 0, 0, true, true, false, true, {}, {}, {}),
	make_instance("x", nil, nil, 0, 0, true, true, false, true, {}, {}, {}),
}),
make_test(
	"?x",
	Measurement(), {
	make_instance("x", nil, nil, 0, 0, true, true, true, false, {}, {}, {}),
}),

make_test(
	"x:m",
	Measurement(), {
	make_instance("x", nil, nil, 0, 0, true, true, true, true, {}, {}, {
		make_modifier("m", Instance.UnknownModifier()),
	}),
}),
make_test(
	":m",
	Measurement(), {
	make_instance("", nil, nil, 0, 0, true, true, true, true, {}, {}, {
		make_modifier("m", Instance.UnknownModifier()),
	}),
}),

make_test(
	"03T{x}",
	Measurement(), {
	make_instance("x", nil, "2016-01-03Z", 0, 0, true, true, true, true, {}, {}, {}),
}),
make_test(
	"03T{x, y}",
	Measurement(), {
	make_instance("x", nil, "2016-01-03Z", 0, 0, true, true, true, true, {}, {}, {}),
	make_instance("y", nil, "2016-01-03Z", 0, 0, true, true, true, true, {}, {}, {}),
}),

make_test(
	"02-03T{x}",
	Measurement(), {
	make_instance("x", nil, "2016-02-03Z", 0, 0, true, true, true, true, {}, {}, {}),
}),
make_test(
	"02-03T{x, y}",
	Measurement(), {
	make_instance("x", nil, "2016-02-03Z", 0, 0, true, true, true, true, {}, {}, {}),
	make_instance("y", nil, "2016-02-03Z", 0, 0, true, true, true, true, {}, {}, {}),
}),

make_test(
	"2015-02-03{x}",
	Measurement(), {
	make_instance("x", nil, "2015-02-03Z", 0, 0, true, true, true, true, {}, {}, {}),
}),
make_test(
	"2015-02-03{x, y}",
	Measurement(), {
	make_instance("x", nil, "2015-02-03Z", 0, 0, true, true, true, true, {}, {}, {}),
	make_instance("y", nil, "2015-02-03Z", 0, 0, true, true, true, true, {}, {}, {}),
}),
}

function do_test(t, implicit_scope, director)
	local o = O.create(t.text)
	U.assert(o ~= nil)
	local text_rewrite = O.write_text_string(o, true)
	U.print("%s  =>", text_rewrite)

	local comp = Composition()
	local success, msg = comp:from_object(o, implicit_scope, director)
	if not success then
		U.print("translation error: %s", msg)
	end
	U.assert(success == not not t.comp, "unexpected success value: %s", success)
	check_composition_equal(comp, t.comp)
	if t.comp then
		check_composition_equal(comp, t.comp)
		comp:to_object(o)
		U.print("%s", O.write_text_string(o, true))
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
