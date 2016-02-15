
local U = require "togo.utility"
local O = require "Quanta.Object"
local Entity = require "Quanta.Entity"
require "Quanta.Entity.Match"

require "common"

function make_search_test(text, branches_func, items)
	return {
		text = text,
		branches_func = branches_func,
		items = items,
	}
end

function make_search_test_item(expected, ref, full_ref)
	return {
		expected = expected,
		ref = ref,
		full_ref = full_ref,
	}
end

local search_tests = {
make_search_test([[
	x = Entity{}
	a = EntityCategory{children = {
		y = Entity{}
		b = EntityCategory{children = {
			z = Entity{}
		}}
	}}
	c = EntityCategory{children = {
		u = Entity{}
		d = EntityCategory{children = {
			v = Entity{}
		}}
	}}
]],
function(universe)
	return {
		Entity.make_search_branch(universe:search(nil, "a"), 0),
		Entity.make_search_branch(universe:search(nil, "a.b"), 0),
		Entity.make_search_branch(universe:search(nil, "c"), math.huge),
		Entity.make_search_branch(universe, 0),
	}
end, {
	make_search_test_item(true, ".x", "x"),
	make_search_test_item(true, ".a.y", "a.y"),
	make_search_test_item(true, ".a.b", "a.b"),
	make_search_test_item(true, ".a.b.z", "a.b.z"),
	make_search_test_item(true, ".c.u", "c.u"),
	make_search_test_item(true, ".c.d", "c.d"),
	make_search_test_item(true, ".c.d.v", "c.d.v"),

	make_search_test_item(true, "x", "x"),
	make_search_test_item(true, "a.y", "a.y"),
	make_search_test_item(true, "a.b", "a.b"),
	make_search_test_item(true, "a.b.z", "a.b.z"),
	make_search_test_item(true, "c.u", "c.u"),
	make_search_test_item(true, "c.d", "c.d"),
	make_search_test_item(true, "c.d.v", "c.d.v"),

	make_search_test_item(true, "x", "x"),
	make_search_test_item(true, "a", "a"),
	make_search_test_item(true, "y", "a.y"),
	make_search_test_item(true, "b", "a.b"),
	make_search_test_item(true, "z", "a.b.z"),

	make_search_test_item(true, "c", "c"),
	make_search_test_item(true, "u", "c.u"),
	make_search_test_item(true, "d", "c.d"),
	make_search_test_item(true, "v", "c.d.v"),

	make_search_test_item(false, ".n"),
	make_search_test_item(false, ".a.n"),
	make_search_test_item(false, "n"),
	make_search_test_item(false, "a.n"),
}),
}

function do_search_test(t)
	local obj = O.create_mv(t.text)

	local universe = Entity.Match.read_universe(obj)
	U.assert(universe)

	local branches = t.branches_func(universe)
	for _, item in pairs(t.items) do
		local entity = universe:search(branches, item.ref)
		local real_ref = entity and entity:ref() or "<not found>"
		U.log("%s => %s", item.ref, real_ref)
		U.assert((entity ~= nil) == item.expected)
		if entity then
			U.assert(real_ref == item.full_ref)
		end
	end
end

function main()
	for _, t in pairs(search_tests) do
		do_search_test(t)
	end

	return 0
end

return main()
