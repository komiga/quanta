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

/// Calculate object name hash of string.
inline ObjectNameHash hash_name(StringRef const& name) {
	return hash::calc_generic<ObjectNameHash>(name);
}

/// Value type.
inline ObjectValueType type(Object const& obj) {
	return static_cast<ObjectValueType>(obj.properties & Object::M_TYPE);
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

/// Sub-source.
inline unsigned sub_source(Object const& obj) {
	return obj.sub_source;
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

/// Set source.
inline unsigned set_source(Object& obj, unsigned source) {
	return obj.source = static_cast<u16>(max(source, 0xFFFFu));
}

/// Set sub-source.
inline unsigned set_sub_source(Object& obj, unsigned sub_source) {
	return obj.sub_source = static_cast<u16>(max(sub_source, 0xFFFFu));
}

/// Clear source and sub-source.
inline void clear_source(Object& obj) {
	object::set_source(obj, 0);
	object::set_sub_source(obj, 0);
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
