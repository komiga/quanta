u8R""__RAW_STRING__(

local U = require "togo.utility"
local O = require "Quanta.Object"
local Match = require "Quanta.Match"
local M = U.module(...)

U.class(M)

M.Quantity = {}
M.QuantityIndex = {}
M.Unit = {}

function M.define_quantity(quantity, units)
	local existing_quantity = M.Quantity[quantity.name]
	if existing_quantity then
		U.assert(false, "name collision: %s : %s",
			existing_quantity.name,
			quantity.name
		)
	end

	table.insert(M.Quantity, quantity)
	quantity.index = #M.Quantity
	quantity.__is_quantity = true
	quantity.UnitByMagnitude = quantity.UnitByMagnitude or {}
	quantity.CompatibleWith = quantity.CompatibleWith or {}
	quantity.CompatibleWith[quantity.index] = true

	M.Quantity[quantity.name] = quantity
	M.QuantityIndex[quantity.name] = quantity.index

	if units then
		M.define_units(quantity, units)
	end
end

function M.define_units(quantity, units)
	U.assert(quantity)
	for _, order in pairs(units) do
		local names = order[1]
		names = U.is_type(names, "string") and {names} or names

		local def = {
			__is_unit = true,
			name = names[1],
			qindex = quantity.index,
			quantity = quantity,
			magnitude = order[2],
			convert = U.is_type(order[3], "function") and order[3] or nil,
		}
		def.is_si = def.convert == nil
		if def.is_si and not quantity.UnitByMagnitude[def.magnitude] then
			quantity.UnitByMagnitude[def.magnitude] = def
		end

		for _, name in pairs(names) do
			local name_hash = O.hash_value(name)
			local existing_unit = M.Unit[name_hash]
			if existing_unit then
				U.assert(false, "name collision: %s (%d) : %s (%d)",
					existing_unit.name, existing_unit.name_hash,
					name, name_hash
				)
			end
			M.Unit[name_hash] = def
		end
	end
end

function M.resolve_quantity_references()
	for _, quantity in ipairs(M.Quantity) do
		for _, name in ipairs(quantity.CompatibleWith) do
			if type(name) == "string" then
				local ref_quantity = M.Quantity[name]
				U.assert(ref_quantity, "quantity does not exist: %s", name)
				quantity.CompatibleWith[ref_quantity.index] = true
			end
		end
	end
end

M.define_quantity({
	name = "dimensionless",
	translation_preference = 0,
	convert = function(self, m)
		m.qindex = self.index
		m.magnitude = 0
	end,
}, {
	{"" , 0},
})

M.define_quantity({
	name = "ratio",
	translation_preference = 0,
	convert = function(self, m)
		-- TODO
		m.qindex = self.index
		m.magnitude = 0
	end,
}, {
	{"ratio",  0},
})

M.define_quantity({
	name = "joule",
	translation_preference = 0,
	convert = function(self, m)
		U.assert(false)
	end,
}, {
	-- SI
	{"GJ",  9},
	{"MJ",  6},
	{"kJ",  3},
	{"J" ,  0},
	{"mJ", -3},
	{"µJ", -6},
	{"nJ", -9},

	-- customary
	{"cal", 0, function(value)
		return value * 0.239
	end},
	{"kcal", 3, function(value)
		return value * 0.000239
	end},
})

M.define_quantity({
	name = "mass",
	tangible = true,
	translation_preference = 3,
	CompatibleWith = {"volume"},
	convert = function(self, m)
		-- TODO: 1u/cm³ density in measurement?
		m.qindex = self.index
	end,
}, {
	-- SI
	{"kg",  3},
	{"g" ,  0},
	{"mg", -3},
	{"µg", -6},
	{"ng", -9},

	-- U.S. customary units
	{"oz", 0, function(value)
		return value * 28.349523
	end},
	{"lb", 3, function(value)
		return value * 0.453592370
	end},
})

M.define_quantity({
	name = "volume",
	tangible = true,
	translation_preference = 2,
	CompatibleWith = {"mass"},
	convert = function(self, m)
		-- TODO
		m.qindex = self.index
	end,
}, {
	-- SI
	{{"l" , "L" },  3},
	{{"ml", "mL"},  0},
	{{"µl", "µL"}, -3},
	{{"nl", "nL"}, -6},

	-- U.S. customary units
	{"fl_oz", 0, function(value)
		return value * 29.573529
	end},
	{"gal", 3, function(value)
		return value * 3.785411784
	end},
})

M.define_quantity({
	name = "transitional",
	translation_preference = 1,
	convert = function(self, m)
		-- TODO
		m.qindex = self.index
	end,
}, {
	-- "International Unit". used in pharmacology. rather annoying..
	{"IU",  0},
})

M.resolve_quantity_references()

function M.get_unit(unit)
	if U.is_type(unit, "string") then
		return M.Unit[O.hash_value(unit)]
	elseif U.is_type(unit, "number") then
		return M.Unit[unit]
	elseif U.is_type(unit, "table") and unit.quantity ~= nil then
		return unit
	end
	return nil
end

local function numeric_bool(v)
	return v and 1 or 0
end

local function measurement_less(lobj, lunit, robj, runit)
	if
		lunit.quantity.translation_preference < runit.quantity.translation_preference or
		math.abs(lunit.magnitude) > math.abs(runit.magnitude) or
		(
			(numeric_bool(O.marker_value_uncertain(lobj)) * 10) +
			(numeric_bool(O.marker_value_guess(lobj)) * 10) +
			math.abs(O.value_approximation(lobj)) >

			(numeric_bool(O.marker_value_uncertain(robj)) * 10) +
			(numeric_bool(O.marker_value_guess(robj)) * 10) +
			math.abs(O.value_approximation(robj))
		)
	then
		return true
	end
	return false
end

function M:__init(value, unit, of, approximation, certain)
	self.of = 0
	self.value = 0
	self.qindex = M.QuantityIndex.dimensionless
	self.magnitude = 0
	self.approximation = 0
	self.certain = true

	if U.is_type(value, "number") then
		self:set(value, unit or "", of, approximation, certain)
	elseif value ~= nil then
		self:from_object(value)
	end
end

function M:make_copy()
	return U.make_empty_object(M):copy(self)
end

function M:copy(measurement)
	self.of = measurement.of
	self.value = measurement.value
	self.qindex = measurement.qindex
	self.magnitude = measurement.magnitude
	self.approximation = measurement.approximation
	self.certain = measurement.certain
	return self
end

function M:from_object(obj)
	U.type_assert(obj, "userdata")

	if O.is_numeric(obj) then
		local unit = M.get_unit(O.unit_hash(obj))
		if not unit then
			return false
		end
		local value = O.numeric(obj)
		local of = 0
		if unit.qindex == M.QuantityIndex.dimensionless then
			of = value
			value = 0
		end
		self:set(
			value, unit, of,
			O.value_approximation(obj),
			not (O.marker_value_uncertain(obj) or O.marker_value_guess(obj))
		)
		return true
	elseif O.is_expression(obj) then
		local of, best, best_unit, unit
		for i, sub in O.children(obj) do
			U.assert(i == 1 or O.op(sub) == O.Operator.div)
			if O.is_numeric(sub) then
				unit = M.get_unit(O.unit_hash(sub))
				if unit and (not best or measurement_less(best, best_unit, sub, unit)) then
					best = sub
					best_unit = unit
					if unit.qindex == M.QuantityIndex.dimensionless then
						of = sub
					end
				end
			end
		end
		if best then
			self:from_object(best)
			if of and of ~= best then
				self.of = O.numeric(of)
			end
			return true
		end
	end

	-- U.assert(false, "unmeasureable object formula")
	self:__init()
	return false
end

function M:to_object(obj, no_rebase)
	U.type_assert(obj, "userdata")
	U.type_assert(no_rebase, "boolean", true)

	local quantity = M.Quantity[self.qindex]
	local value = self.value
	local unit = quantity.UnitByMagnitude[self.magnitude]
	if not unit and not no_rebase then
		value = value * (10 ^ self.magnitude)
		unit = quantity.UnitByMagnitude[0]
	end

	if self.of > 0 then
		if value > 0 then
			O.set_expression(obj)
			O.clear_children(obj)
			O.set_decimal(O.push_child(obj), self.of)
			obj = O.push_child(obj)
			O.set_op(obj, O.Operator.div)
		else
			O.set_decimal(obj, self.of)
		end
	end

	O.set_value_approximation(obj, self.approximation)
	O.set_value_certain(obj, self.certain)
	if value > 0 then
		O.set_decimal(obj, value)
		if unit then
			O.set_unit(obj, unit.name)
		else
			O.set_unit(obj, quantity.name .. "_unknown")
		end
	end
end

function M:set(value, unit, of, approximation, certain)
	U.type_assert(value, "number")
	unit = M.get_unit(unit)
	U.type_assert(unit, "table")
	U.type_assert(of, "number", true)
	U.type_assert(approximation, "number", true)
	U.type_assert(certain, "boolean", true)

	self.of = U.optional(of, 0)
	self.value = value
	self.qindex = unit.qindex
	self.magnitude = unit.magnitude
	self.approximation = U.optional(approximation, 0)
	self.certain = U.optional(certain, true)

	if unit.convert then
		self.value = unit.convert(self.value)
	end
end

function M:rebase(unit, force)
	unit = M.get_unit(unit)
	U.type_assert(unit, "table")
	U.assert(
		force or unit.quantity.CompatibleWith[self.qindex] == true,
		"units must be compatible"
	)

	if self.qindex ~= unit.qindex then
		unit.quantity.convert(unit.quantity, self)
	end
	if self.magnitude ~= unit.magnitude then
		self.value = self.value * (10 ^ (self.magnitude - unit.magnitude))
		self.magnitude = unit.magnitude
	end
end

function M:add(measurement)
	U.assert(U.is_instance(measurement, M))
	if measurement.qindex ~= self.qindex or measurement.magnitude ~= self.magnitude then
		U.assert(M.Quantity[self.qindex].tangible, "lhs must be a tangible quantity if the rhs is not of the same quantity")
		measurement = measurement:make_copy()
		measurement:rebase(self:unit())
	end
	self.value = self.value + measurement.value
	self.of = self.of + measurement.of
	self.approximation = U.clamp(self.approximation + measurement.approximation, -3, 3)
	self.certain = self.certain and measurement.certain
end

function M:sub(measurement)
	U.assert(U.is_instance(measurement, M))
	measurement.value = -measurement.value
	self:add(measurement)
	measurement.value = -measurement.value
end

function M:quantity()
	return M.Quantity[self.qindex]
end

function M:unit()
	return M.Quantity[self.qindex].UnitByMagnitude[self.magnitude]
end

function M:is_exact()
	return self.approximation == 0 and self.certain
end

function M:is_empty()
	return self.of == 0 and self.value == 0
end

function M:__eq(y)
	return (
		self.of == y.of and
		self.value == y.value and
		self.qindex == y.qindex and
		self.magnitude == y.magnitude and
		self.approximation == y.approximation and
		self.certain == y.certain
	)
end

function M.struct_list(list)
	U.type_assert(list, "table")
	return list
end

function M.struct_list_to_quantity(list, obj)
	if #list == 0 or list[1]:is_empty() then
		O.release_quantity(obj)
	elseif #list == 1 then
		list[1]:to_object(O.make_quantity(obj))
	elseif #list > 1 then
		local quantity = O.make_quantity(obj)
		for _, m in pairs(list) do
			if not m:is_empty() then
				m:to_object(O.push_child(quantity))
			end
		end
		if not O.has_children(quantity) then
			O.release_quantity(quantity)
		end
	end
end

-- TODO: use a pattern instead of passively ignoring non-matching whole quantities
function M.adapt_struct_list(property_name)
	local p_element = Match.Pattern{
		any = true,
		acceptor = function(context, thing, obj)
			local m = M(obj)
			if not m:is_empty() then
				table.insert(thing[property_name], m)
			end
		end,
	}

	local t_head = Match.Tree({
	-- [..., ...]
	Match.Pattern{
		children = {
			p_element,
		},
	},
	-- [...]
	p_element,
	})
	t_head:build()

	return M.struct_list_to_quantity, t_head
end

_, M.t_struct_list_head = M.adapt_struct_list("measurements")

return M

)"__RAW_STRING__"