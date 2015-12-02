
local U = require "Quanta.Util"
local O = require "Quanta.Object"
local Measurement = require "Quanta.Measurement"
local Instance = require "Quanta.Instance"
local Composition = require "Quanta.Composition"
local Entity = require "Quanta.Entity"

function check_modifier_equal(x, y)
	U.assert(x.name == y.name)
	U.assert(x.controller == y.controller)
	if x.controller then
		x.controller.check_equal(x, y)
	end
end

function check_instance_equal(x, y)
	U.assert(x.name == y.name)
	U.assert(x.name_hash == y.name_hash)
	U.assert(x.item == y.item)
	U.assert(x.source == y.source)
	U.assert(x.sub_source == y.sub_source)
	U.assert(x.source_certain == y.source_certain)
	U.assert(x.sub_source_certain == y.sub_source_certain)
	U.assert(x.variant_certain == y.variant_certain)
	U.assert(x.presence_certain == y.presence_certain)

	U.assert(#x.measurements == #y.measurements)
	for mi = 1, #x.measurements do
		U.assert(x.measurements[mi] == y.measurements[mi])
	end

	U.assert(#x.selection == #y.selection)
	for si = 1, #x.selection do
		check_instance_equal(x.selection[si], y.selection[si])
	end

	U.assert(#x.modifiers == #y.modifiers)
	for mi = 1, #x.modifiers do
		check_modifier_equal(x.modifiers[mi], y.modifiers[mi])
	end
end

function make_modifier(name, controller)
	local m = Instance.Modifier()
	m.name = name
	m.controller = controller
	return m
end

function make_instance(
	name, item,
	source, sub_source,
	source_certain, sub_source_certain,
	variant_certain, presence_certain,
	measurements, selection, modifiers
)
	local i = Instance()
	i.name = name
	i.name_hash = O.hash_name(name)
	i.item = item
	i.source = source
	i.sub_source = sub_source
	i.source_certain = source_certain
	i.sub_source_certain = sub_source_certain
	i.variant_certain = variant_certain
	i.presence_certain = presence_certain
	i.measurements = measurements
	i.selection = selection
	i.modifiers = modifiers
	return i
end

function make_test(text, items)
	local comp = Composition()
	comp.items = items

	return {
		text = text,
		comp = comp,
	}
end

local translation_tests = {
	make_test("x", {
		make_instance("x", nil, 0, 0, true, true, true, true, {}, {}, {}),
	}),
	make_test("x$1", {
		make_instance("x", nil, 1, 0, true, true, true, true, {}, {}, {}),
	}),
	make_test("x$1$2", {
		make_instance("x", nil, 1, 2, true, true, true, true, {}, {}, {}),
	}),
	make_test("{x, y}", {
		make_instance("x", nil, 0, 0, true, true, true, true, {}, {}, {}),
		make_instance("y", nil, 0, 0, true, true, true, true, {}, {}, {}),
	}),

	make_test("{x[2], x[2ml], x[2kg]}", {
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(2)}, {}, {}),
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(2, "ml")}, {}, {}),
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(2, "kg")}, {}, {}),
	}),
	make_test("{x[?1], x[G~1]}", {
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(1, "", 0, 0, false)}, {}, {}),
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(1, "", 0, 0, false)}, {}, {}),
	}),
	make_test("{x[~~1], x[^^1]}", {
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(1, "", 0, -2)}, {}, {}),
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(1, "", 0,  2)}, {}, {}),
	}),
	make_test("x[1, 2]", {
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(1), Measurement(2)}, {}, {}),
	}),
	make_test("x[1/2g]", {
		make_instance("x", nil, 0, 0, true, true, true, true, {Measurement(2, "g", 1)}, {}, {}),
	}),

	make_test("{x$?, x$?$?, x$?1, x$?1$?2}", {
		make_instance("x", nil, 0, 0, false, true, true, true, {}, {}, {}),
		make_instance("x", nil, 0, 0, false, false, true, true, {}, {}, {}),
		make_instance("x", nil, 1, 0, false, true, true, true, {}, {}, {}),
		make_instance("x", nil, 1, 2, false, false, true, true, {}, {}, {}),
	}),
	make_test("{x_¿, x¿}", {
		make_instance("x", nil, 0, 0, true, true, false, true, {}, {}, {}),
		make_instance("x", nil, 0, 0, true, true, false, true, {}, {}, {}),
	}),
	make_test("?x", {
		make_instance("x", nil, 0, 0, true, true, true, false, {}, {}, {}),
	}),

	make_test("x:m", {
		make_instance("x", nil, 0, 0, true, true, true, true, {}, {}, {
			make_modifier("m", nil, nil),
		}),
	}),
	make_test(":m", {
		make_instance("", nil, 0, 0, true, true, true, true, {}, {}, {
			make_modifier("m", nil, nil),
		}),
	}),
}

function do_translation_test(t, search_in, controllers)
	local o = O.create(t.text)
	U.assert(o ~= nil)

	local comp = Composition(o, search_in, controllers)
	U.assert(#comp.items == #t.comp.items)
	for i = 1, #comp.items do
		check_instance_equal(comp.items[i], t.comp.items[i])
	end

	comp:to_object(o)
	local text = O.write_text_string(o, #comp.items > 1)
	if #comp.items > 1 then
		text = "{" .. string.sub(text, 4, -3) .. "}"
	end
	text = string.gsub(text, "\t", " ")
	text = string.gsub(text, "\n", ",")
	U.print("%s  =>\n%s", t.text, text)

	O.destroy(o)
end

function main()
	local search_in = {}
	local controllers = {}
	for _, t in pairs(translation_tests) do
		do_translation_test(t, search_in, controllers)
	end

	return 0
end

return main()
