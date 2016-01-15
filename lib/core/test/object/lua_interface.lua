
local O = require "Quanta.Object"
local T = require "Quanta.Time"
require "Quanta.Time.Gregorian"

function print_table(t, level)
	local function table_to_string(t, level)
		local ws = string.rep("  ", level)
		if t == nil then
			return ws .. "nil"
		end
		local output = "{\n"
		for k, v in pairs(t) do
			output = output .. ws .. "  \"" .. tostring(k) .. "\" : "
			if type(v) == "table" then
				output = output .. table_to_string(v, level + 1) .. ",\n"
			else
				output = output .. tostring(v) .. ",\n"
			end
		end
		return output .. ws .. "}"
	end
	print(table_to_string(t, level or 0))
end

-- print_table(O)

do
	local a = O.create_mv [[
		a, b
	]]

	for i, v in O.children(a) do
		print(i, O.write_text_string(v, true), v)
		assert(
			(i == 1 and O.identifier(v) == "a") or
			(i == 2 and O.identifier(v) == "b") or
			false
		)
	end

	assert(O.push_child(a, "c") ~= nil)
	assert(O.num_children(a) == 3 and O.identifier(O.child_at(a, 3)) == "c")
	O.remove_child(a, 2)
	assert(O.num_children(a) == 2 and O.identifier(O.child_at(a, 2)) == "c")
	O.remove_child(a, 1)
	assert(O.num_children(a) == 1 and O.identifier(O.child_at(a, 1)) == "c")
	O.pop_child(a)
	assert(O.num_children(a) == 0)
	assert(O.push_child(a, "x=") == nil)
	O.destroy(a)
end

do
	local a = O.create()
	assert(
		not O.is_named(a) and
		O.name_hash(a) == O.NAME_NULL and
		O.name(a) == ""
	)
	assert(O.op(a) == O.Operator.none)
	assert(
		not O.has_source(a) and O.source(a) == 0 and O.sub_source(a) == 0 and
		not O.source_certain(a) and O.source_certain_or_unspecified(a) and
		not O.marker_source_uncertain(a) and not O.marker_sub_source_uncertain(a)
	)
	assert(
		O.value_certain(a) and O.value_approximation(a) == 0 and
		not O.marker_value_uncertain(a) and not O.marker_value_guess(a)
	)
	assert(O.is_null(a))
	assert(not O.has_tags(a))
	assert(not O.has_children(a))
	assert(not O.has_quantity(a))
	assert(not O.find_child(a, O.NAME_NULL))
	assert(not O.find_child(a, ""))
	assert(not O.find_child(a, "a"))
	assert(not O.find_tag(a, O.NAME_NULL))
	assert(not O.find_tag(a, ""))
	assert(not O.find_tag(a, "a"))
	O.destroy(a)
end

do
	local a = O.create()
	O.set_source(a, 1)
	O.set_sub_source(a, 2)
	assert(
		O.has_source(a) and O.source(a) == 1 and O.sub_source(a) == 2 and
		O.source_certain(a) and O.source_certain_or_unspecified(a) and
		not O.marker_source_uncertain(a) and not O.marker_sub_source_uncertain(a)
	)
	O.set_source_certain(a, false)
	assert(
		not O.source_certain(a) and not O.source_certain_or_unspecified(a) and
		O.marker_source_uncertain(a) and not O.marker_sub_source_uncertain(a)
	)
	O.set_sub_source_certain(a, false)
	assert(
		not O.source_certain(a) and not O.source_certain_or_unspecified(a) and
		O.marker_source_uncertain(a) and O.marker_sub_source_uncertain(a)
	)
	O.clear_source(a)
	assert(
		not O.has_source(a) and O.source(a) == 0 and O.sub_source(a) == 0 and
		not O.source_certain(a) and O.source_certain_or_unspecified(a) and
		not O.marker_source_uncertain(a) and not O.marker_sub_source_uncertain(a)
	)
	O.destroy(a)
end

do
	local a = O.create()
	O.set_value_certain(a, false)
	assert(not O.value_certain(a) and O.marker_value_uncertain(a) and not O.marker_value_guess(a))
	O.set_value_guess(a, true)
	assert(not O.value_certain(a) and not O.marker_value_uncertain(a) and O.marker_value_guess(a))
	O.set_value_certain(a, true)
	assert(O.value_certain(a) and not O.marker_value_uncertain(a) and not O.marker_value_guess(a))

	O.set_value_approximation(a, -3)
	assert(not O.value_certain(a) and O.value_approximation(a) == -3)
	O.set_value_approximation(a, 0)
	assert(O.value_certain(a) and O.value_approximation(a) == 0)
	O.set_value_approximation(a, 3)
	assert(not O.value_certain(a) and O.value_approximation(a) == 3)

	O.clear_value_markers(a)
	assert(
		O.value_certain(a) and
		not O.marker_value_uncertain(a) and not O.marker_value_guess(a) and
		O.value_approximation(a) == 0
	)
	O.destroy(a)
end

do
	local a = O.create()
	O.set_name(a, "name")
	assert(O.is_named(a) and O.name(a) == "name" and O.name_hash(a) == O.hash_name("name"))
	O.clear_name(a)
	assert(not O.is_named(a) and O.name(a) == "" and O.name_hash(a) == O.NAME_NULL)
	O.destroy(a)
end

do
	local a = O.create()
	O.set_identifier(a, "brains")
	assert(O.identifier(a) == "brains" and O.identifier_hash(a) == O.hash_value("brains"))

	local q = O.make_quantity(a)
	O.set_integer(q, 42, "g")
	assert(O.integer(q) == 42)
	assert(O.unit(q) == "g")

	local c = O.push_child(a)
	O.set_name(c, "d")
	assert(O.name(c) == "d")
	O.set_string(c, "yum")
	assert(O.string(c) == "yum")
	assert(O.text(c) == "yum")

	assert(c == O.find_child(a, "d"))

	local t = O.push_tag(a)
	O.set_name(t, "leftover")
	assert(t == O.find_tag(a, "leftover"))
	O.destroy(a)
end

do
	local wow = T.temporary()
	-- treated as local when resolve_time() is called (from unzoned to -04:00)
	T.set(wow, 10,16,0)

	local a = O.create()
	O.set_time_value(a, wow)
	assert(O.is_zoned(a))
	assert(not O.is_year_contextual(a) and not O.is_month_contextual(a))
	assert(O.has_date(a) and O.has_clock(a) and O.time_type(a) == O.TimeType.date_and_clock)

	O.set_zoned(a, false)
	assert(not O.is_zoned(a))
	O.set_time_type(a, O.TimeType.date)
	assert(O.has_date(a) and not O.has_clock(a) and O.time_type(a) == O.TimeType.date)
	O.set_time_type(a, O.TimeType.clock)
	assert(not O.has_date(a) and O.has_clock(a) and O.time_type(a) == O.TimeType.clock)

	T.adjust_zone_clock(wow, -4)
	T.G.set(wow, 1977,8,15)
	O.resolve_time(a, wow)
	assert(O.is_zoned(a))
	assert(O.has_date(a) and O.has_clock(a) and O.time_type(a) == O.TimeType.date_and_clock)

	assert(T.compare_equal(O.time(a), wow))
	O.destroy(a)
end

do
	local y, m, d
	local t = T.temporary()
	T.G.set(t, 1,10,15)

	local a = O.create()
	O.set_time_value(a, t)

	T.clear(t)
	O.set_year_contextual(a, true)
	T.G.set(t, 2,3,10)
	O.resolve_time(a, t)
	y, m, d = T.G.date(O.time(a))
	assert(y == 2 and m == 10 and d == 15)

	O.set_month_contextual(a, true) -- implies contextual year
	T.G.set(t, 4,6,20)
	O.resolve_time(a, t)
	y, m, d = T.G.date(O.time(a))
	assert(y == 4 and m == 6 and d == 15)
	O.destroy(a)
end

do
	local a = O.create()
	O.set_expression(a)
	assert(O.is_expression(a))

	local e1 = O.push_child(a)
	O.set_integer(e1, 1)
	assert(O.op(e1) == O.Operator.none)

	local e2 = O.push_child(a)
	O.set_integer(e2, 2)
	O.set_op(e2, O.Operator.div)
	assert(O.op(e2) == O.Operator.div)

	O.destroy(a)
end
