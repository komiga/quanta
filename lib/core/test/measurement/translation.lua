
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
	make_test("1ratio"  , 0, 1  , "ratio", 0),
	make_test("0.5ratio", 0, 0.5, "ratio", 0),
	make_test("100pct", 0, 1, "ratio", 0),
	make_test("20pct" , 0, 0.2, "ratio", 0),

	make_test("2"  , 0, 0, ""  , 2),
	make_test("2g" , 0, 2, "g" , 0),
	make_test("2ml", 0, 2, "ml", 0),

	make_test("1/2g", 2, 2, "g", 1),
	make_test("1/2ml/3g", 3, 3, "g", 1),
	make_test("1/2ml/3g/4mg", 3, 3, "g", 1),

	make_test("1µg/2mg", 2, 2, "mg"),
	make_test("2mg/1µg", 1, 2, "mg"),

	make_test("?1/2", 2, 0, "", 2),
	make_test("~1/2", 2, 0, "", 2),
	make_test("~~1/~2", 2, 0, "", 2, -1),
	make_test("?1/~~2", 2, 0, "", 2, -2),
	make_test("G~1/~~~2", 2, 0, "", 2, -3),
	make_test("G~^1/?2", 2, 0, "", 2, 0, false),
}

function do_translation_test(t)
	local obj = O.create(t.text)
	U.assert(obj ~= nil)
	U.print("%s  =>", O.write_text_string(obj, true))

	local value = t.value
	local unit = Measurement.get_unit(t.unit)
	U.assert(unit)

	local item = t.item_index > 0 and O.operand_at(obj, t.item_index) or obj
	-- U.assert(Measurement.get_unit(O.unit_hash(item)) == unit)

	local m = Measurement(obj)
	U.assert(m:unit() == unit)
	O.clear(obj)
	m:to_object(obj)
	print(O.write_text_string(obj, true))

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

		m:convert("g", true)
		U.assert(
			m.value == 1 and
			m.qindex == Measurement.QuantityIndex.mass and
			m.magnitude == 0
		)

		m:convert("mg")
		U.assert(m.value == 1000 and m.magnitude == -3)
		m:convert("kg")
		U.assert(m.value == 0.001 and m.magnitude == 3)
		m:convert("g")
		U.assert(m.value == 1 and m.magnitude == 0)

		m:convert("ml")
		U.assert(m.value == 1 and m.magnitude == 0)

		m:set(12, "µg")
		U.assert(m.value == 12 and m.magnitude == -6)

		m:convert("g")
		U.assert(m.value == 0.000012 and m.magnitude == 0)
	end

	for _, t in pairs(translation_tests) do
		do_translation_test(t)
	end

	return 0
end

return main()
