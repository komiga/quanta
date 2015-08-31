#line 2 "quanta/core/object/object.hpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.

@file
@brief Object interface.
@ingroup lib_core_object
*/

#pragma once

#include <quanta/core/config.hpp>
#include <quanta/core/types.hpp>
#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>
#include <togo/core/memory/memory.hpp>
#include <togo/core/collection/array.hpp>
#include <togo/core/string/string.hpp>
#include <togo/core/hash/hash.hpp>

#include <quanta/core/string/unmanaged_string.hpp>
#include <quanta/core/object/types.hpp>
#include <quanta/core/object/object.gen_interface>

namespace quanta {
namespace object {

/**
	@addtogroup lib_core_object
	@{
*/

namespace {
	static constexpr ObjectValueType const type_mask_numeric
		= ObjectValueType::integer
		| ObjectValueType::decimal
	;
} // anonymous namespace

namespace internal {

inline unsigned get_property(Object const& obj, unsigned mask, unsigned shift) {
	return (obj.properties & mask) >> shift;
}

inline void set_property(Object& obj, unsigned mask, unsigned shift, unsigned value) {
	obj.properties = (obj.properties & ~mask) | (value << shift);
}

inline void clear_property(Object& obj, unsigned mask) {
	obj.properties &= ~mask;
}

} // namespace internal

/// Calculate object name hash of string.
inline ObjectNameHash hash_name(StringRef const& name) {
	return hash::calc_generic<ObjectNameHash>(name);
}

/// Value type.
inline ObjectValueType type(Object const& obj) {
	return static_cast<ObjectValueType>(internal::get_property(obj, Object::M_TYPE, 0));
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

/// Whether type is ObjectValueType::string.
inline bool is_string(Object const& obj) { return object::is_type(obj, ObjectValueType::string); }

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

/// Source.
inline unsigned source(Object const& obj) {
	return obj.source;
}

/// Whether source uncertainty marker is set.
inline bool marker_source_uncertain(Object const& obj) {
	return internal::get_property(obj, Object::M_SOURCE_UNCERTAIN, Object::S_SOURCE_UNCERTAIN);
}

/// Sub-source.
inline unsigned sub_source(Object const& obj) {
	return obj.sub_source;
}

/// Whether sub-source uncertainty marker is set.
inline bool marker_sub_source_uncertain(Object const& obj) {
	return internal::get_property(obj, Object::M_SUB_SOURCE_UNCERTAIN, Object::S_SUB_SOURCE_UNCERTAIN);
}

/// Whether a source is specified.
inline bool has_source(Object const& obj) {
	return obj.source != 0;
}

/// Whether the source and sub-source are certain.
inline bool source_certain(Object const& obj) {
	return object::has_source(obj) && !(obj.properties & Object::M_BOTH_SOURCE_UNCERTAIN);
}

/// Whether the source and sub-source are certain or unspecified.
inline bool source_certain_or_unspecified(Object const& obj) {
	return !(obj.properties & Object::M_BOTH_SOURCE_UNCERTAIN);
}

/// Whether the value uncertain marker is set.
inline bool marker_value_uncertain(Object const& obj) {
	return internal::get_property(obj, Object::M_VALUE_UNCERTAIN, Object::S_VALUE_UNCERTAIN);
}

/// Whether the value guess marker is set.
inline bool marker_value_guess(Object const& obj) {
	return internal::get_property(obj, Object::M_VALUE_GUESS, Object::S_VALUE_GUESS);
}

/// Value approximation marker value.
inline signed value_approximation(Object const& obj) {
	unsigned const value = internal::get_property(obj, Object::M_VALUE_APPROXIMATE, Object::S_VALUE_APPROXIMATE);
	return (value & (1 << 2)) ? -(signed_cast(value) & 3) : signed_cast(value);
}

/// Whether value is certain.
///
/// If the value guess or uncertainty marker are set or the approximation marker
/// is non-zero, this returns false.
/// If the value is null, the value uncertainty and approximation markers can
/// still be set.
inline bool value_certain(Object const& obj) {
	return !(obj.properties & Object::M_VALUE_MARKERS);
}

/// Boolean value.
inline bool boolean(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::boolean));
	return obj.value.boolean;
}

/// Integer value.
inline s64 integer(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::integer));
	return obj.value.numeric.integer;
}

/// Decimal value.
inline f64 decimal(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::decimal));
	return obj.value.numeric.decimal;
}

/// Numeric value unit.
inline StringRef unit(Object const& obj) {
	TOGO_ASSERTE(object::is_type_any(obj, type_mask_numeric));
	return obj.value.numeric.unit;
}

/// Numeric value unit hash.
inline ObjectNumericUnitHash unit_hash(Object const& obj) {
	TOGO_ASSERTE(object::is_type_any(obj, type_mask_numeric));
	return obj.value.numeric.unit.hash;
}

/// String value.
inline StringRef string(Object const& obj) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::string));
	return obj.value.string;
}

/// Clear source and sub-source uncertainty markers.
inline void clear_source_uncertainty(Object& obj) {
	internal::clear_property(obj, Object::M_SOURCE_UNCERTAIN | Object::M_SUB_SOURCE_UNCERTAIN);
}

/// Set source certainty marker.
inline void set_source_certain(Object& obj, bool const certain) {
	internal::set_property(obj, Object::M_SOURCE_UNCERTAIN, Object::S_SOURCE_UNCERTAIN, !certain);
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
	internal::set_property(obj, Object::M_SUB_SOURCE_UNCERTAIN, Object::S_SUB_SOURCE_UNCERTAIN, !certain);
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

/// Set value certainty.
///
/// This clears the value guess marker.
inline void set_value_certain(Object& obj, bool const certain) {
	internal::set_property(obj, Object::M_VALUE_UNCERTAIN_AND_GUESS, Object::S_VALUE_UNCERTAIN, !certain);
}

/// Set value guess marker.
///
/// This clears the value uncertainty marker.
inline void set_value_guess(Object& obj, bool const guess) {
	internal::set_property(obj, Object::M_VALUE_UNCERTAIN_AND_GUESS, Object::S_VALUE_GUESS, guess);
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
	internal::set_property(obj, Object::M_VALUE_APPROXIMATE, Object::S_VALUE_APPROXIMATE, property_value);
}

/// Clear value uncertainty, guess, and approximation markers.
inline void clear_value_markers(Object& obj) {
	internal::clear_property(obj, Object::M_VALUE_MARKERS);
}

/// Set value to null.
inline void set_null(Object& obj) {
	object::set_type(obj, ObjectValueType::null);
}

/// Set boolean value.
inline void set_boolean(Object& obj, bool const value) {
	object::set_type(obj, ObjectValueType::boolean);
	obj.value.boolean = value;
}

/// Set integer value.
inline void set_integer(Object& obj, s64 const value) {
	object::set_type(obj, ObjectValueType::integer);
	obj.value.numeric.integer = value;
}

/// Set decimal value.
inline void set_decimal(Object& obj, f64 const value) {
	object::set_type(obj, ObjectValueType::decimal);
	obj.value.numeric.decimal = value;
}

/// Set numeric value unit.
///
/// Type must be numeric.
inline void set_unit(Object& obj, StringRef const unit) {
	TOGO_ASSERTE(object::is_type_any(obj, type_mask_numeric));
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

/// Set string value.
inline void set_string(Object& obj, StringRef const value) {
	object::set_type(obj, ObjectValueType::string);
	unmanaged_string::set(obj.value.string, value, memory::default_allocator());
}

/// Children.
inline Array<Object>& children(Object& obj) { return obj.children; }
inline Array<Object> const& children(Object const& obj) { return obj.children; }

/// Tags.
inline Array<Object>& tags(Object& obj) { return obj.tags; }
inline Array<Object> const& tags(Object const& obj) { return obj.tags; }

/// Quantity.
inline Object* quantity(Object& obj) { return obj.quantity; }
inline Object const* quantity(Object const& obj) { return obj.quantity; }

/// Whether the object has tags.
inline bool has_tags(Object const& obj) {
	return array::any(obj.tags);
}

/// Whether the object has children.
inline bool has_children(Object const& obj) {
	return array::any(obj.children);
}

/// Whether the object has a quantity.
inline bool has_quantity(Object const& obj) {
	return obj.quantity;
}

/// Clear tags.
inline void clear_tags(Object& obj) {
	array::clear(obj.tags);
}

/// Clear children.
inline void clear_children(Object& obj) {
	array::clear(obj.children);
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

/// Destruct.
inline Object::~Object() {
	object::clear_name(*this);
	object::set_null(*this);
	object::release_quantity(*this);
}

/// Construct null.
inline Object::Object()
	: properties(unsigned_cast(ObjectValueType::null))
	, source(0)
	, sub_source(0)
	, name()
	, value()
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

inline Object& Object::operator=(Object const& other) {
	object::copy(*this, other);
	return *this;
}

/** @} */ // end of doc-group lib_core_object

} // namespace object
} // namespace quanta
