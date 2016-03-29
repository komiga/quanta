
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
	u1 = U{E1 = {a}, P1 = {E1}}
	u1{P1}
	u1{P2}
]], {
	true,
	true,
	true,
	true,
	true,
	false,
}),
}

function do_test(t, implicit_scope, resolver)
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

	resolver:do_tree(unit)
	check_unit(unit)
end

local function searcher_wrapper(name, searcher)
	return function(resolver, parent, unit)
		local thing, variant, terminate = searcher(resolver, parent, unit)
		U.print(
			"%10s %-5s %s => %s, %s, %s%s",
			name,
			parent ~= nil,
			unit.id,
			thing ~= nil,
			variant ~= nil,
			not not terminate,
			thing ~= nil and "\n" or ""
		)
		return thing, variant, terminate
	end
end

local function select_searcher(part)
	if part.type ~= Unit.Type.reference then
		return searcher_wrapper("child", Unit.Resolver.searcher_unit_child(part))
	else
		return searcher_wrapper("selector", Unit.Resolver.searcher_unit_selector(part))
	end
end

function main()
	Vessel.init("vessel_data")
	local universe = Entity.read_universe(O.create_mv(universe_text))
	U.assert(universe)

	local resolver = Unit.Resolver(select_searcher)
	resolver:push_searcher(function()
		U.print("")
		return nil, nil
	end)
	resolver:push_searcher(searcher_wrapper("universe", Unit.Resolver.searcher_universe(universe, nil, nil)))

	local implicit_scope = make_time("2016-01-01Z")
	for _, t in pairs(tests) do
		do_test(t, implicit_scope, resolver)
	end

	return 0
end

return main()
