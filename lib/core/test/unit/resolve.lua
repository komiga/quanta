
require "common"

local U = require "togo.utility"
local O = require "Quanta.Object"
local Prop = require "Quanta.Prop"
local Measurement = require "Quanta.Measurement"
local Unit = require "Quanta.Unit"
local Entity = require "Quanta.Entity"
local Vessel = require "Quanta.Vessel"

function make_test(text, items)
	return {
		text = text,
		items = items,
	}
end

local universe_text = [[
	a = Generic{}
	b = GenericCategory{children = {
		c = Generic{children = {
			d = Generic{}
		}}
	}}
]]

Vessel.init("vessel_data")
local universe = Entity.read_universe(O.create_mv(universe_text))
U.assert(universe)

local function resolver(parent, unit)
	if parent then
		unit.thing = parent.parts[unit.id] or parent.items[unit.id]
	end
	if not unit.thing then
		unit.thing = universe:search(nil, unit.id)
	end
	U.print("%-5s %s => %s", parent ~= nil, unit.id, unit.thing ~= nil)
end

local tests = {
make_test([[
	a
	b
	b.c
	b.c.d
	n
]], {
	true,
	true,
	true,
	true,
	false,
}),

make_test([[
	U{a}
	U{P1 = {a}}
	U{P1 = {RS1{a}}}
]], {
	true,
	true,
	true,
}),

make_test([[
	U{E1 = {a}, P1 = {E1}}
]], {
	true,
	true,
}),
}

function do_test(t, implicit_scope)
	local obj = O.create_mv(t.text)
	U.assert(obj ~= nil)

	local unit = Unit.Composition()
	local success, msg = unit:from_object_by_type(obj, implicit_scope)
	U.assert(success, "translation error: %s", msg)
	-- U.print("%s", O.write_text_string(unit:to_object(), true))

	local i = 1
	local function check_unit(unit)
		if U.is_instance(unit, Unit.Element) then
			for _, step in ipairs(unit.steps) do
				for _, item in ipairs(step.composition.items) do
					check_unit(item)
				end
			end
		else
			if unit.type == Unit.Type.reference then
				U.assert(i <= #t.items)
				if U.is_type(t.items[i], "boolean") then
					U.assert((unit.thing ~= nil) == t.items[i], O.write_text_string(unit:to_object(), true))
				else
					U.assert(unit.thing == t.items[i], O.write_text_string(unit:to_object(), true))
				end
				i = i + 1
			end
			for _, item in ipairs(unit.items) do
				check_unit(item)
			end
			for _, part in ipairs(unit.parts) do
				check_unit(part)
			end
		end
	end

	unit:resolve(nil, resolver)
	check_unit(unit)
end

function main()
	local implicit_scope = make_time("2016-01-01Z")
	for _, t in pairs(tests) do
		do_test(t, implicit_scope)
	end

	return 0
end

return main()
