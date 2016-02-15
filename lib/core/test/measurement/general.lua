
local U = require "togo.utility"
local O = require "Quanta.Object"
local Measurement = require "Quanta.Measurement"

function make_test(text, item_index, value, unit, of, approximation, certain)
	return {
		text = text,
		item_index = item_index,
		value = value,
		unit = unit,
		of = U.optional(of, 0),
		approximation = U.optional(approximation, 0),
		certain = U.optional(certain, true),
	}
end

local translation_tests = {
	make_test("2"  , 0, 2, ""  ),
	make_test("2u" , 0, 2, "u" ),
	make_test("2g" , 0, 2, "g" ),
	make_test("2ml", 0, 2, "ml"),

	make_test("1/2g", 2, 2, "g", 1),
	make_test("1/2ml/3g", 3, 3, "g", 1),
	make_test("1/2ml/3g/4mg", 3, 3, "g", 1),

	make_test("1µg/2mg", 2, 2, "mg"),
	make_test("2mg/1µg", 1, 2, "mg"),

	make_test("?1/2", 2, 2, ""),
	make_test("~1/2", 2, 2, ""),
	make_test("~~1/~2", 2, 2, "", 0, -1),
	make_test("?1/~~2", 2, 2, "", 0, -2),
	make_test("G~1/~~~2", 2, 2, "", 0, -3),
	make_test("G~^1/?2", 2, 2, "", 0, 0, false),
}

function do_translation_test(t)
	local o = O.create(t.text)
	U.assert(o ~= nil)

	local value = t.value
	local unit = Measurement.get_unit(t.unit)
	U.assert(unit)

	local item = t.item_index > 0 and O.child_at(o, t.item_index) or o
	U.assert(Measurement.get_unit(O.unit_hash(item)) == unit)

	local m = Measurement(o)
	U.assert(m.value == value)
	U.assert(m.qindex == unit.quantity.index)
	U.assert(m.magnitude == unit.magnitude)
	U.assert(m.of == t.of)
	U.assert(m.approximation == t.approximation)
	U.assert(m.certain == t.certain)

	U.assert(m == Measurement(t.value, t.unit, t.of, t.approximation, t.certain))
end

function main()
	do
		local m = Measurement(1)
		U.assert(
			m.value == 1 and
			m.qindex == Measurement.QuantityIndex.dimensionless and
			m.magnitude == 0
		)

		m:rebase("g")
		U.assert(
			m.value == 1 and
			m.qindex == Measurement.QuantityIndex.mass and
			m.magnitude == 0
		)

		m:rebase("mg")
		U.assert(m.value == 1000 and m.magnitude == -3)
		m:rebase("kg")
		U.assert(m.value == 0.001 and m.magnitude == 3)
		m:rebase("g")
		U.assert(m.value == 1 and m.magnitude == 0)

		m:rebase("ml")
		U.assert(m.value == 1 and m.magnitude == 0)

		m:set(12, "µg")
		U.assert(m.value == 12 and m.magnitude == -6)

		m:rebase("g")
		U.assert(m.value == 0.000012 and m.magnitude == 0)
	end

	for _, t in pairs(translation_tests) do
		do_translation_test(t)
	end

	return 0
end

return main()
