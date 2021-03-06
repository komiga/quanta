#line 2 "quanta/core/object/object.hpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.

@file
@brief Object interface.
@ingroup lib_core_object
*/

#pragma once

// igen-source: object/io_text.cpp
// igen-source: object/object_li.cpp

#include <quanta/core/config.hpp>
#include <quanta/core/types.hpp>
#include <quanta/core/string/unmanaged_string.hpp>
#include <quanta/core/chrono/time.hpp>
#include <quanta/core/object/types.hpp>
#include <quanta/core/object/internal.hpp>
#include <quanta/core/lua/lua.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>
#include <togo/core/memory/memory.hpp>
#include <togo/core/collection/array.hpp>
#include <togo/core/string/string.hpp>
#include <togo/core/hash/hash.hpp>
#include <togo/core/io/types.hpp>

#include <quanta/core/object/object.gen_interface>

namespace quanta {
namespace object {

/**
	@addtogroup lib_core_object
	@{
*/

/// Calculate object name hash of a string.
inline ObjectNameHash hash_name(StringRef const& name) {
	return hash::calc<ObjectNameHasher>(name);
}

/// Calculate value hash of a string.
inline ObjectValueHash hash_value(StringRef const& name) {
	return hash::calc<ObjectValueHasher>(name);
}

/// Value type.
inline ObjectValueType type(Object const& obj) {
	return static_cast<ObjectValueType>(internal::get_property(obj, M_TYPE, 0));
}

/// Whether type is type.
inline bool is_type(Object const& obj, ObjectValueType const type) {
	return object::type(obj) == type;
}

/// Whether type is any in type.
inline bool is_type_any(Object const& obj, ObjectValueType const type) {
	return enum_bool(object::type(obj) & type);
}

/// Whether type is ObjectValueType::null.
inline bool is_null(Object const& obj) { return object::is_type(obj, ObjectValueType::null); }

/// Whether type is ObjectValueType::boolean.
inline bool is_boolean(Object const& obj) { return object::is_type(obj, ObjectValueType::boolean); }

/// Whether type is ObjectValueType::integer.
inline bool is_integer(Object const& obj) { return object::is_type(obj, ObjectValueType::integer); }

/// Whether type is ObjectValueType::decimal.
inline bool is_decimal(Object const& obj) { return object::is_type(obj, ObjectValueType::decimal); }

/// Whether type is ObjectValueType::integer or ObjectValueType::decimal.
inline bool is_numeric(Object const& obj) {
	return object::is_type_any(obj, type_mask_numeric);
}

/// Whether type is ObjectValueType::currency.
inline bool is_currency(Object const& obj) { return object::is_type(obj, ObjectValueType::currency); }

/// Whether type is ObjectValueType::time.
inline bool is_time(Object const& obj) { return object::is_type(obj, ObjectValueType::time); }

/// Whether type is ObjectValueType::string.
inline bool is_string(Object const& obj) { return object::is_type(obj, ObjectValueType::string); }

/// Whether type is ObjectValueType::identifier.
inline bool is_identifier(Object const& obj) { return object::is_type(obj, ObjectValueType::identifier); }

/// Whether type is ObjectValueType::string or ObjectValueType::identifier.
inline bool is_textual(Object const& obj) { return object::is_type_any(obj, type_mask_textual); }

/// Whether type is ObjectValueType::expression.
inline bool is_expression(Object const& obj) { return object::is_type(obj, ObjectValueType::expression); }

/// Line in source file.
inline unsigned source_line(Object const& obj) {
	return obj.source_line;
}

/// Set line in source file.
///
/// A 0 means unknown/unprovided
inline void set_source_line(Object& obj, unsigned source_line) {
	obj.source_line = source_line;
}

/// Name.
inline StringRef name(Object const& obj) {
	return obj.name;
}

/// Name hash.
inline ObjectNameHash name_hash(Object const& obj) {
	return obj.name.hash;
}

/// Whether name is non-empty.
inline bool is_named(Object const& obj) {
	return unmanaged_string::any(obj.name);
}

/// Set name.
inline void set_name(Object& obj, StringRef name) {
	unmanaged_string::set(obj.name, name, memory::default_allocator());
}

/// Clear name.
inline void clear_name(Object& obj) {
	unmanaged_string::clear(obj.name, memory::default_allocator());
}

/// Operator.
inline ObjectOperator op(Object const& obj) {
	return static_cast<ObjectOperator>(internal::get_property(obj, M_OP, S_OP));
}

/// Set operator.
///
/// This only has significance when the object is part of an expression.
inline void set_op(Object& obj, ObjectOperator const op) {
	internal::set_property(obj, M_OP, S_OP, unsigned_cast(op));
}

/// Source.
inline unsigned source(Object const& obj) {
	return obj.source;
}

/// Whether source uncertainty marker is set.
inline bool marker_source_uncertain(Object const& obj) {
	return internal::get_property(obj, M_SOURCE_UNCERTAIN, 0);
}

/// Sub-source.
inline unsigned sub_source(Object const& obj) {
	return obj.sub_source;
}

/// Whether sub-source uncertainty marker is set.
inline bool marker_sub_source_uncertain(Object const& obj) {
	return internal::get_property(obj, M_SUB_SOURCE_UNCERTAIN, 0);
}

/// Whether a source is specified.
inline bool has_source(Object const& obj) {
	return obj.source != 0;
}

/// Whether a sub-source is specified.
inline bool has_sub_source(Object const& obj) {
	return obj.sub_source != 0;
}

/// Whether the source and sub-source are certain.
inline bool source_certain(Object const& obj) {
	return object::has_source(obj) && !(obj.properties & M_BOTH_SOURCE_UNCERTAIN);
}

/// Whether the source and sub-source are certain or unspecified.
inline bool source_certain_or_unspecified(Object const& obj) {
	return obj.source == 0 || !(obj.properties & M_BOTH_SOURCE_UNCERTAIN);
}

/// Clear source and sub-source uncertainty markers.
inline void clear_source_uncertainty(Object& obj) {
	internal::clear_property(obj, M_SOURCE_UNCERTAIN | M_SUB_SOURCE_UNCERTAIN);
}

/// Set source certainty marker.
inline void set_source_certain(Object& obj, bool const certain) {
	internal::set_property(obj, M_SOURCE_UNCERTAIN, S_SOURCE_UNCERTAIN, !certain);
}

/// Set source.
///
/// If source is 0, sub-source is also set to 0 (same effect as clear_source()).
/// This clears the source uncertainty marker.
inline void set_source(Object& obj, unsigned const source) {
	if (source == 0) {
		obj.source = 0;
		obj.sub_source = 0;
		object::clear_source_uncertainty(obj);
	} else {
		obj.source = static_cast<u16>(min(source, 0xFFFFu));
		object::set_source_certain(obj, true);
	}
}

/// Set sub-source certainty marker.
inline void set_sub_source_certain(Object& obj, bool const certain) {
	internal::set_property(obj, M_SUB_SOURCE_UNCERTAIN, S_SUB_SOURCE_UNCERTAIN, !certain);
}

/// Set sub-source.
///
/// The source must be non-0 for the sub-source to take a non-0 value.
inline void set_sub_source(Object& obj, unsigned const sub_source) {
	if (object::has_source(obj)) {
		obj.sub_source = static_cast<u16>(min(sub_source, 0xFFFFu));
	}
}

/// Clear source and sub-source and their uncertainty markers.
inline void clear_source(Object& obj) {
	object::set_source(obj, 0);
	object::clear_source_uncertainty(obj);
}

/// Whether the value uncertain marker is set.
inline bool marker_value_uncertain(Object const& obj) {
	return internal::get_property(obj, M_VALUE_UNCERTAIN, 0);
}

/// Whether the value guess marker is set.
inline bool marker_value_guess(Object const& obj) {
	return internal::get_property(obj, M_VALUE_GUESS, 0);
}

/// Value approximation marker value.
inline signed value_approximation(Object const& obj) {
	unsigned const value = internal::get_property(obj, M_VALUE_APPROXIMATE, S_VALUE_APPROXIMATE);
	return (value & (1 << 2)) ? -(signed_cast(value) & 3) : signed_cast(value);
}

/// Whether value is certain.
///
/// If the value guess or uncertainty marker are set or the approximation marker
/// is non-zero, this returns false.
/// If the value is null, the value uncertainty and approximation markers can
/// still be set.
inline bool value_certain(Object const& obj) {
	return !(obj.properties & M_VALUE_MARKERS);
}

/// Set value certainty.
///
/// This clears the value guess marker.
inline void set_value_certain(Object& obj, bool const certain) {
	internal::set_property(obj, M_VALUE_UNCERTAIN_AND_GUESS, S_VALUE_UNCERTAIN, !certain);
}

/// Set value guess marker.
///
/// If the value is null, this has no effect.
/// This clears the value uncertainty marker.
inline void set_value_guess(Object& obj, bool const guess) {
	if (object::is_null(obj)) {
		internal::set_property(obj, M_VALUE_UNCERTAIN_AND_GUESS, S_VALUE_GUESS, guess);
	}
}

/// Set value approximation value.
///
/// value is bound to [-3, 3]. value of:
///
/// - <0 = approximately less than
/// -  0 = absolute
/// - >0 = approximately more than
inline void set_value_approximation(Object& obj, signed const value) {
	unsigned property_value = value < 0;
	property_value = unsigned_cast(min(property_value ? -value : value, 3)) | (property_value << 2);
	internal::set_property(obj, M_VALUE_APPROXIMATE, S_VALUE_APPROXIMATE, property_value);
}

/// Clear value uncertainty, guess, and approximation markers.
inline void clear_value_markers(Object& obj) {
	internal::clear_property(obj, M_VALUE_MARKERS);
}

/// Set value to null.
inline void set_null(Object& obj) {
	object::set_type(obj, ObjectValueType::null);
}

/// Boolean value.
inline bool boolean(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::boolean));
	return obj.value.boolean;
}

/// Set boolean value.
inline void set_boolean(Object& obj, bool const value) {
	object::set_type(obj, ObjectValueType::boolean);
	obj.value.boolean = value;
}

/// Integer value.
inline s64 integer(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::integer));
	return obj.value.numeric.integer;
}

/// Set integer value.
inline void set_integer(Object& obj, s64 const value) {
	object::set_type(obj, ObjectValueType::integer);
	obj.value.numeric.integer = value;
}

/// Decimal value.
inline f64 decimal(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::decimal));
	return obj.value.numeric.decimal;
}

/// Set decimal value.
inline void set_decimal(Object& obj, f64 const value) {
	object::set_type(obj, ObjectValueType::decimal);
	obj.value.numeric.decimal = value;
}

/// Numeric/currency unit.
inline StringRef unit(Object const& obj) {
	TOGO_ASSERTE(object::is_type_any(obj, type_mask_unit_carrier));
	return obj.value.numeric.unit;
}

/// Numeric/currency unit hash.
inline ObjectValueHash unit_hash(Object const& obj) {
	TOGO_ASSERTE(object::is_type_any(obj, type_mask_unit_carrier));
	return obj.value.numeric.unit.hash;
}

/// Whether the numeric/currency value has a unit.
inline bool has_unit(Object const& obj) {
	return object::is_type_any(obj, type_mask_unit_carrier) && unmanaged_string::any(obj.value.numeric.unit);
}

/// Set numeric/currency unit.
///
/// Type must be numeric or currency.
inline void set_unit(Object& obj, StringRef const unit) {
	TOGO_ASSERTE(object::is_type_any(obj, type_mask_unit_carrier));
	unmanaged_string::set(obj.value.numeric.unit, unit, memory::default_allocator());
}

/// Set integer value and unit.
inline void set_integer(Object& obj, s64 const value, StringRef const unit) {
	object::set_type(obj, ObjectValueType::integer);
	obj.value.numeric.integer = value;
	object::set_unit(obj, unit);
}

/// Set decimal value and unit.
inline void set_decimal(Object& obj, f64 const value, StringRef const unit) {
	object::set_type(obj, ObjectValueType::decimal);
	obj.value.numeric.decimal = value;
	object::set_unit(obj, unit);
}

/// Currency value.
inline s64 currency(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::currency));
	return obj.value.numeric.c.value;
}

/// Currency exponent.
inline s32 currency_exponent(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::currency));
	return obj.value.numeric.c.exponent;
}

/// Set currency value, exponent, and unit.
inline void set_currency(Object& obj, s64 const value, s32 const exponent, StringRef const unit) {
	object::set_type(obj, ObjectValueType::currency);
	obj.value.numeric.c.value = value;
	obj.value.numeric.c.exponent = exponent;
	object::set_unit(obj, unit);
}

/// Set currency value.
///
/// Type must currency.
inline void set_currency(Object& obj, s64 const value) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::currency));
	obj.value.numeric.c.value = value;
}

/// Set currency exponent.
///
/// Type must currency.
inline void set_currency_exponent(Object& obj, s64 const exponent) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::currency));
	obj.value.numeric.c.value = exponent;
}

// NB: calling the accessors "time" makes them ambiguous with the time
// namespace >_>

/// Time value.
inline Time& time_value(Object& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::time));
	return obj.value.time;
}

/// Time value.
inline Time const& time_value(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::time));
	return obj.value.time;
}

/// Time type.
inline ObjectTimeType time_type(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::time));
	return static_cast<ObjectTimeType>(internal::get_property(obj, M_TM_TYPE, S_TM_TYPE));
}

/// Whether the time value specifies a date (by time type).
inline bool has_date(Object const& obj) {
	return object::time_type(obj) != ObjectTimeType::clock;
}

/// Whether the time value specifies a clock (by time type).
inline bool has_clock(Object const& obj) {
	return object::time_type(obj) != ObjectTimeType::date;
}

/// Whether the time value specifies a zone offset (UTC or relative to UTC).
///
/// The zoned object property is overridden if the time value has a zone offset.
inline bool is_zoned(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::time));
	return obj.value.time.zone_offset != 0 || !internal::get_property(obj, M_TM_UNZONED, 0);
}

/// Whether the time value is context-relative to year.
inline bool is_year_contextual(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::time));
	return
		object::has_date(obj) &&
		internal::get_property(obj, M_TM_CONTEXTUAL_YEAR | M_TM_CONTEXTUAL_MONTH, 0)
	;
}

/// Whether the time value is context-relative to month.
///
/// This implies is_year_contextual().
inline bool is_month_contextual(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::time));
	return
		object::has_date(obj) &&
		internal::get_property(obj, M_TM_CONTEXTUAL_MONTH, 0)
	;
}

/// Whether the time value is context-relative to year or month.
inline bool is_date_contextual(Object const& obj) {
	return object::is_year_contextual(obj);
}

/// Set whether the time value specifies a zone offset (UTC or relative to UTC).
///
/// If zoned is false and the value has a zone offset, it is adjusted to UTC
/// unless adjust is false.
inline void set_zoned(Object& obj, bool zoned, bool adjust = true) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::time));
	if (!zoned && adjust && obj.value.time.zone_offset != 0) {
		time::adjust_zone_utc(obj.value.time);
	}
	internal::set_property(obj, M_TM_UNZONED, S_TM_UNZONED, !zoned);
}

/// Set whether the time value is context-relative to year.
///
/// Does nothing if the time type does not include date.
inline void set_year_contextual(Object& obj, bool year_contextual) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::time));
	if (object::has_date(obj)) {
		internal::set_property(obj, M_TM_CONTEXTUAL_YEAR, S_TM_CONTEXTUAL_YEAR, year_contextual);
	}
}

/// Set whether the time value is context-relative to month.
///
/// This implies context-relative to year.
/// Does nothing if the time type does not include date.
inline void set_month_contextual(Object& obj, bool month_contextual) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::time));
	if (object::has_date(obj)) {
		internal::set_property(obj, M_TM_CONTEXTUAL_MONTH, S_TM_CONTEXTUAL_MONTH, month_contextual);
	}
}

/// Set time type.
inline void set_time_type(Object& obj, ObjectTimeType const type) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::time));
	internal::set_property(obj, M_TM_TYPE, S_TM_TYPE, unsigned_cast(type));
}

/// Set time value without changing time type.
///
/// If the new value has a zone offset, the time is zoned.
inline void set_time_value(Object& obj, Time const& value) {
	object::set_type(obj, ObjectValueType::time);
	obj.value.time = value;
	if (value.zone_offset != 0) {
		object::set_zoned(obj, true);
	}
}

/// Set time value as date and clock.
inline void set_time(Object& obj, Time const& value) {
	object::set_time_value(obj, value);
	object::set_time_type(obj, ObjectTimeType::date_and_clock);
}

/// Set time value as date.
inline void set_time_date(Object& obj, Time const& value) {
	object::set_time_value(obj, value);
	object::set_time_type(obj, ObjectTimeType::date);
}

/// Set time value as clock.
inline void set_time_clock(Object& obj, Time const& value) {
	object::set_time_value(obj, value);
	object::set_time_type(obj, ObjectTimeType::clock);
}

/// Resolve time value from context.
///
/// See object::time_resolved().
inline void resolve_time(Object& obj, Time context) {
	obj.value.time = object::time_resolved(obj, context);
	if (!object::has_date(obj)) {
		object::set_time_type(obj, ObjectTimeType::date_and_clock);
	}
	internal::clear_property(obj, M_TM_UNZONED | M_TM_CONTEXTUAL_MONTH | M_TM_CONTEXTUAL_YEAR);
}

/// String value.
inline StringRef string(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::string));
	return obj.value.string.value;
}

/// Set string value.
inline void set_string(Object& obj, StringRef const value) {
	object::set_type(obj, ObjectValueType::string);
	unmanaged_string::set(obj.value.string.value, value, memory::default_allocator());
}

/// String type.
inline StringRef string_type(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::string));
	return obj.value.string.type;
}

/// String type hash.
inline ObjectValueHash string_type_hash(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::string));
	return obj.value.string.type.hash;
}

/// Whether the string value is typed.
inline bool has_string_type(Object const& obj) {
	return object::is_type(obj, ObjectValueType::string) && unmanaged_string::any(obj.value.string.type);
}

/// Set string type.
inline void set_string_type(Object& obj, StringRef const type) {
	object::set_type(obj, ObjectValueType::string);
	unmanaged_string::set(obj.value.string.type, type, memory::default_allocator());
}

/// Identifier value.
inline StringRef identifier(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::identifier));
	return obj.value.identifier;
}

/// Identifier value hash.
inline ObjectValueHash identifier_hash(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::identifier));
	return obj.value.identifier.hash;
}

/// Set identifier value.
inline void set_identifier(Object& obj, StringRef const value) {
	object::set_type(obj, ObjectValueType::identifier);
	unmanaged_string::set(obj.value.identifier, value, memory::default_allocator());
}

/// String or identifier value.
inline StringRef text(Object const& obj) {
	TOGO_ASSERTE(object::is_type_any(obj, type_mask_textual));
	return object::is_type(obj, ObjectValueType::string) ? obj.value.string.value : obj.value.identifier;
}

/// Set type to expression.
inline void set_expression(Object& obj) {
	object::set_type(obj, ObjectValueType::expression);
}

/// Expression value.
inline Array<Object>& expression(Object& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::expression));
	return obj.expression;
}
inline Array<Object> const& expression(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::expression));
	return obj.expression;
}

/// Whether the object has any operands (is an expression).
inline bool has_operands(Object const& obj) {
	return array::any(obj.expression);
}

/// Clear expression.
inline void clear_expression(Object& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::expression));
	array::clear(obj.expression);
}

/// Children.
inline Array<Object>& children(Object& obj) { return obj.children; }
inline Array<Object> const& children(Object const& obj) { return obj.children; }

/// Whether the object has children.
inline bool has_children(Object const& obj) {
	return array::any(obj.children);
}

/// Clear children.
inline void clear_children(Object& obj) {
	array::clear(obj.children);
}

/// Copy children.
inline void copy_children(Object& dst, Object const& src) {
	array::copy(dst.children, src.children);
}

/// Tags.
inline Array<Object>& tags(Object& obj) { return obj.tags; }
inline Array<Object> const& tags(Object const& obj) { return obj.tags; }

/// Whether the object has tags.
inline bool has_tags(Object const& obj) {
	return array::any(obj.tags);
}

/// Clear tags.
inline void clear_tags(Object& obj) {
	array::clear(obj.tags);
}

/// Copy tags.
inline void copy_tags(Object& dst, Object const& src) {
	array::copy(dst.tags, src.tags);
}

/// Quantity.
inline Object* quantity(Object& obj) { return obj.quantity; }
inline Object const* quantity(Object const& obj) { return obj.quantity; }

/// Whether the object has a quantity.
inline bool has_quantity(Object const& obj) {
	return obj.quantity;
}

/// Clear quantity.
inline void clear_quantity(Object& obj) {
	if (object::has_quantity(obj)) {
		object::clear(*obj.quantity);
	}
}

/// Release quantity.
inline void release_quantity(Object& obj) {
	TOGO_DESTROY(memory::default_allocator(), obj.quantity);
	obj.quantity = nullptr;
}

/// Copy quantity.
inline void copy_quantity(Object& dst, Object const& src) {
	if (object::has_quantity(src)) {
		if (object::has_quantity(dst)) {
			object::copy(*dst.quantity, *src.quantity);
		} else {
			dst.quantity = TOGO_CONSTRUCT(memory::default_allocator(), Object, *src.quantity);
		}
	} else {
		object::release_quantity(dst);
	}
}

/// Destruct.
inline Object::~Object() {
	object::clear_name(*this);
	object::set_null(*this);
	object::release_quantity(*this);
}

/// Construct null.
inline Object::Object()
	: properties(unsigned_cast(ObjectValueType::null))
	, source_line(0)
	, source(0)
	, sub_source(0)
	, name()
	, value()
	, expression(memory::default_allocator())
	, tags(memory::default_allocator())
	, children(memory::default_allocator())
	, quantity(nullptr)
{}

/// Construct copy.
inline Object::Object(Object const& other)
	: Object()
{
	object::copy(*this, other);
}

/// Move-construct.
inline Object::Object(Object&& other)
	: properties(other.properties)
	, source(other.source)
	, sub_source(other.sub_source)
	, name(other.name)
	, value(other.value)
	, expression(rvalue_ref(other.expression))
	, tags(rvalue_ref(other.tags))
	, children(rvalue_ref(other.children))
	, quantity(other.quantity)
{
	switch (object::type(other)) {
	case ObjectValueType::null:
		break;
	case ObjectValueType::boolean:
		other.value.boolean = false;
		break;
	case ObjectValueType::integer:
		other.value.numeric.integer = 0;
		other.value.numeric.unit = {};
		break;
	case ObjectValueType::decimal:
		other.value.numeric.decimal = 0.0f;
		other.value.numeric.unit = {};
		break;
	case ObjectValueType::currency:
		other.value.numeric.c = {};
		other.value.numeric.unit = {};
		break;
	case ObjectValueType::time:
		other.value.time = {};
		break;
	case ObjectValueType::string:
		other.value.string.value = {};
		other.value.string.type = {};
		break;
	case ObjectValueType::identifier:
		other.value.identifier = {};
		break;
	case ObjectValueType::expression:
		break;
	}
	other.properties = unsigned_cast(ObjectValueType::null);
	other.source = 0;
	other.sub_source = 0;
	other.name = {};
	other.quantity = nullptr;
}

inline Object& Object::operator=(Object const& other) {
	object::copy(*this, other);
	return *this;
}

/** @} */ // end of doc-group lib_core_object

} // namespace object
} // namespace quanta
