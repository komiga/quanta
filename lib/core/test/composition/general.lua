
local U = require "togo.utility"
local O = require "Quanta.Object"
local Measurement = require "Quanta.Measurement"
local Instance = require "Quanta.Instance"
local Composition = require "Quanta.Composition"
local Entity = require "Quanta.Entity"

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
		make_instance("x", nil, 0, 0, true, true, true, true, {}, {}, {}),
	}),
	make_test(
		"x$1",
		Measurement(), {
		make_instance("x", nil, 1, 0, true, true, true, true, {}, {}, {}),
	}),
	make_test(
		"x$1$2",
		Measurement(), {
		make_instance("x", nil, 1, 2, true, true, true, true, {}, {}, {}),
	}),
	make_test(
		"{x, y}",
		Measurement(), {
		make_instance("x", nil, 0, 0, true, true, true, true, {}, {}, {}),
		make_instance("y", nil, 0, 0, true, true, true, true, {}, {}, {}),
	}),
	make_test(
		"x + y",
		Measurement(), {
		make_instance("x", nil, 0, 0, true, true, true, true, {}, {}, {}),
		make_instance("y", nil, 0, 0, true, true, true, true, {}, {}, {}),
	}),
	make_test(
		"{x[1], y[2]}[3]",
		Measurement(3), {
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(1)}, {}, {}),
		make_instance("y", nil, 0, 0, true, true, true, true, {Measurement(2)}, {}, {}),
	}),

	make_test(
		"{x[2], x[2ml], x[2kg]}",
		Measurement(), {
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(2)}, {}, {}),
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(2, "ml")}, {}, {}),
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(2, "kg")}, {}, {}),
	}),
	make_test(
		"{x[?1], x[G~1]}",
		Measurement(), {
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(1, "", 0, 0, false)}, {}, {}),
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(1, "", 0, 0, false)}, {}, {}),
	}),
	make_test(
		"{x[~~1], x[^^1]}",
		Measurement(), {
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(1, "", 0, -2)}, {}, {}),
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(1, "", 0,  2)}, {}, {}),
	}),
	make_test(
		"x[1, 2]",
		Measurement(), {
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(1), Measurement(2)}, {}, {}),
	}),
	make_test(
		"x[1/2g]",
		Measurement(), {
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(2, "g", 1)}, {}, {}),
	}),

	make_test(
		"{x$?, x$?$?, x$?1, x$?1$?2}",
		Measurement(), {
		make_instance("x", nil, 0, 0, false, true, true, true, {}, {}, {}),
		make_instance("x", nil, 0, 0, false, false, true, true, {}, {}, {}),
		make_instance("x", nil, 1, 0, false, true, true, true, {}, {}, {}),
		make_instance("x", nil, 1, 2, false, false, true, true, {}, {}, {}),
	}),
	make_test(
		"{x_¿, x¿}",
		Measurement(), {
		make_instance("x", nil, 0, 0, true, true, false, true, {}, {}, {}),
		make_instance("x", nil, 0, 0, true, true, false, true, {}, {}, {}),
	}),
	make_test(
		"?x",
		Measurement(), {
		make_instance("x", nil, 0, 0, true, true, true, false, {}, {}, {}),
	}),

	make_test(
		"x:m",
		Measurement(), {
		make_instance("x", nil, 0, 0, true, true, true, true, {}, {}, {
			make_modifier("m", nil, nil),
		}),
	}),
	make_test(
		":m",
		Measurement(), {
		make_instance("", nil, 0, 0, true, true, true, true, {}, {}, {
			make_modifier("m", nil, nil),
		}),
	}),
}

function do_test(t, search_in, controllers)
	local o = O.create(t.text)
	U.assert(o ~= nil)

	local comp = Composition(o, search_in, controllers)
	check_composition_equal(comp, t.comp)

	comp:to_object(o)
	local text = O.write_text_string(o, #comp.items > 1)
	if #comp.items > 1 then
		if comp.measurement:is_empty() then
			text = "{" .. string.sub(text, 4, -3) .. "}"
		else
			local bi
			local i = 0
			while i ~= nil do
				bi = i
				i = string.find(text, "%[", i + 1)
			end
			text = "{" .. string.sub(text, 4, bi - 3) .. "}" .. string.sub(text, bi)
		end
	end
	text = string.gsub(text, "\t", " ")
	text = string.gsub(text, "\n", ",")
	U.print("%s  =>\n%s", t.text, text)
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
