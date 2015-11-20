
local U = require "Quanta.Util"
local O = require "Quanta.Object"
local M = U.module("Quanta.Measurement")

U.class(M)

M.Quantity = {
{
	name = "dimensionless",
	translation_preference = 0,
	convert = function(self, m)
		m.qindex = self.index
		m.magnitude = 0
	end,
},
{
	name = "mass",
	translation_preference = 3,
	convert = function(self, m)
		-- TODO: 1u/cm³ density in measurement?
		m.qindex = self.index
	end,
},
{
	name = "volume",
	translation_preference = 2,
	convert = function(self, m)
		-- TODO
		m.qindex = self.index
	end,
},
{
	name = "transitional",
	translation_preference = 1,
	convert = function(self, m)
		-- TODO
		m.qindex = self.index
	end,
},
}
M.QuantityIndex = {}
M.Unit = {}

do
	for i, q in ipairs(M.Quantity) do
		q.index = i
		q.UnitByMagnitude = {}
		M.Quantity[q.name] = q
		M.QuantityIndex[q.name] = i
	end

	local function define_units(quantity, orders)
		U.assert(quantity)
		for _, order in pairs(orders) do
			local names = order[1]
			names = U.is_type(names, "string") and {names} or names

			local def = {
				name = names[1],
				qindex = quantity.index,
				quantity = quantity,
				magnitude = U.is_type(order[2], "number") and order[2] or 0,
				convert = U.is_type(order[2], "function") and order[2] or nil,
			}
			def.is_si = def.convert == nil
			if def.is_si and not quantity.UnitByMagnitude[def.magnitude] then
				quantity.UnitByMagnitude[def.magnitude] = def
			end

			for _, name in pairs(names) do
				local name_hash = O.hash_value(name)
				U.assert(not M.Unit[name_hash])
				M.Unit[name_hash] = def
			end
		end
	end

	define_units(M.Quantity.dimensionless, {
		{"" ,  0},
		{"u",  0}, -- FIXME: this should probably not be allowed in the future
	})

	define_units(M.Quantity.mass, {
		-- SI
		{"kg",  3},
		{"g" ,  0},
		{"mg", -3},
		{"µg", -6},
		{"ng", -9},

		-- U.S. customary units
		{"oz", function(value)
			return value * 28.349523
		end},
		{"lb", function(value)
			return value * 453.592370
		end},
	})

	define_units(M.Quantity.volume, {
		-- SI
		{{"l" , "L" },  3},
		{{"ml", "mL"},  0},
		{{"µl", "µL"}, -3},
		{{"nl", "nL"}, -6},

		-- U.S. customary units
		{"fl_oz", function(value)
			return value * 29.573529
		end},
		{"gal", function(value)
			return value * 3785.411784
		end},
	})

	define_units(M.Quantity.transitional, {
		-- "International Unit". used in pharmacology. rather annoying..
		{"IU",  0},
	})
end

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

function M:__init(obj_or_value, unit, approximation, certain)
	self.of = 0
	self.value = 0
	self.qindex = M.QuantityIndex.dimensionless
	self.magnitude = 0
	self.approximation = 0
	self.certain = true

	if U.is_type(obj_or_value, "number") then
		self:set(obj_or_value, unit or "", approximation, certain)
	elseif obj_or_value ~= nil then
		self:from_object(obj_or_value)
	end
end

function M:from_object(obj)
	U.type_assert(obj, "userdata")

	if O.is_numeric(obj) then
		self:set(
			O.numeric(obj),
			O.unit_hash(obj),
			O.value_approximation(obj),
			not (O.marker_value_uncertain(obj) or O.marker_value_guess(obj))
		)
		return
	elseif O.is_expression(obj) then
		local of, best, best_unit, unit
		for i, sub in O.children(obj) do
			U.assert(O.op(sub) == O.Operator.div or i == 1)
			if O.is_numeric(sub) then
				unit = M.get_unit(O.unit_hash(sub))
				if unit and (not best or measurement_less(best, best_unit, sub, unit)) then
					best = sub
					best_unit = unit
					if unit.qindex == M.QuantityIndex.dimensionless then
						of = O.numeric(sub)
					end
				end
			end
		end
		if best then
			self:from_object(best)
			self.of = of
			return
		end
	end
	U.assert(false, "unmeasureable object formula")
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
	O.set_value_approximation(obj, self.approximation)
	O.set_value_certain(obj, self.certain)
	O.set_decimal(obj, value)
	if unit then
		O.set_unit(obj, unit.name)
	else
		O.set_unit(obj, quantity.name .. "_unknown")
	end
end

function M:set(value, unit, approximation, certain)
	U.type_assert(value, "number")
	unit = M.get_unit(unit)
	U.type_assert(unit, "table")
	U.type_assert(approximation, "number", true)
	U.type_assert(certain, "boolean", true)

	self.value = value
	self.qindex = unit.qindex
	self.magnitude = unit.magnitude
	self.approximation = U.optional(approximation, 0)
	self.certain = U.optional(certain, true)

	if unit.convert then
		self.value = unit.convert(self.value)
	end
end

function M:rebase(unit)
	unit = M.get_unit(unit)
	U.type_assert(unit, "table")
	U.assert(unit.is_si, "rebase unit must be SI")

	if self.qindex ~= unit.qindex then
		unit.quantity.convert(unit.quantity, self)
	end
	if self.magnitude ~= unit.magnitude then
		self.value = self.value * (10 ^ (self.magnitude - unit.magnitude))
		self.magnitude = unit.magnitude
	end
end

function M:unit()
	return M.Quantity[self.qindex].UnitByMagnitude[self.magnitude]
end

function M:is_empty()
	return self.value == 0
end

function M:__eq(y)
	return (
		self.value == y.value and
		self.qindex == y.qindex and
		self.magnitude == y.magnitude and
		self.approximation == y.approximation and
		self.certain == y.certain
	)
end

return M
