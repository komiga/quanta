#line 2 "quanta/core/object/object.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/string/unmanaged_string.hpp>
#include <quanta/core/object/internal.hpp>
#include <quanta/core/object/object.hpp>

#include <togo/core/log/log.hpp>
#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>
#include <togo/core/memory/memory.hpp>

namespace quanta {

/// Set type.
///
/// Returns true if type changed.
bool object::set_type(Object& obj, ObjectValueType const type) {
	if (object::type(obj) == type) {
		return false;
	}
	object::clear_value(obj);
	internal::set_property(obj, M_TYPE, 0, unsigned_cast(type));
	if (type == ObjectValueType::null) {
		internal::clear_property(obj, M_VALUE_GUESS);
	} else if (type == ObjectValueType::expression) {
		object::clear_value_markers(obj);
		object::clear_source(obj);
		object::clear_tags(obj);
		object::clear_quantity(obj);
	}
	return true;
}

/// Reset value to type default.
///
/// This does not clear children if the object is an expression.
void object::clear_value(Object& obj) {
	auto& a = memory::default_allocator();
	switch (object::type(obj)) {
	case ObjectValueType::null:
		break;
	case ObjectValueType::boolean:
		obj.value.boolean = false;
		break;
	case ObjectValueType::integer:
		obj.value.numeric.integer = 0;
		unmanaged_string::clear(obj.value.numeric.unit, a);
		break;
	case ObjectValueType::decimal:
		obj.value.numeric.decimal = 0.0f;
		unmanaged_string::clear(obj.value.numeric.unit, a);
		break;
	case ObjectValueType::time:
		obj.value.time = {};
		internal::clear_property(obj, M_TM_PROPERTIES);
		break;
	case ObjectValueType::string:
		unmanaged_string::clear(obj.value.string, a);
		break;
	case ObjectValueType::expression:
		break;
	}
}

/// Copy an object.
void object::copy(Object& dst, Object const& src, bool const children IGEN_DEFAULT(true)) {
	auto& a = memory::default_allocator();
	object::clear_value(dst);
	dst.properties = src.properties;
	dst.source = src.source;
	dst.sub_source = src.sub_source;
	unmanaged_string::set(dst.name, src.name, a);
	switch (object::type(src)) {
	case ObjectValueType::null:
		break;
	case ObjectValueType::boolean:
		dst.value.boolean = src.value.boolean;
		break;
	case ObjectValueType::integer:
		dst.value.numeric.integer = src.value.numeric.integer;
		unmanaged_string::set(dst.value.numeric.unit, src.value.numeric.unit, a);
		break;
	case ObjectValueType::decimal:
		dst.value.numeric.decimal = src.value.numeric.decimal;
		unmanaged_string::set(dst.value.numeric.unit, src.value.numeric.unit, a);
		break;
	case ObjectValueType::time:
		dst.value.time = src.value.time;
		break;
	case ObjectValueType::string:
		unmanaged_string::set(dst.value.string, src.value.string, a);
		break;
	case ObjectValueType::expression:
		break;
	}
	array::copy(dst.tags, src.tags);
	if (children) {
		array::copy(dst.children, src.children);
	}
	if (object::has_quantity(src)) {
		if (object::has_quantity(dst)) {
			object::copy(*dst.quantity, *src.quantity);
		} else {
			dst.quantity = TOGO_CONSTRUCT(a, Object, *src.quantity);
		}
	}
}

/// Create quantity or clear existing quantity.
Object& object::make_quantity(Object& obj) {
	if (object::has_quantity(obj)) {
		object::clear(*obj.quantity);
	} else {
		obj.quantity = TOGO_CONSTRUCT_DEFAULT(memory::default_allocator(), Object);
	}
	return *obj.quantity;
}

/// Set to null and clear all properties.
void object::clear(Object& obj) {
	object::clear_name(obj);
	object::set_null(obj);
	object::clear_value_markers(obj);
	object::clear_source(obj);
	object::clear_tags(obj);
	object::clear_children(obj);
	object::clear_quantity(obj);
}

/// Resolve time value from context.
///
/// Relative date parts are taken from the context time.
/// If the time value does not specify a zone offset, it is adjusted to the
/// zone offset of the context (time assumed to be zone-local).
void object::resolve_time(Object& obj, Time context) {
	TOGO_ASSERTE(object::is_type(obj, ObjectValueType::time));
	if (!object::is_zoned(obj)) {
		time::adjust_zone_offset(obj.value.time, context.zone_offset);
	}
	// context is only used as a date, so we don't want the zone offset to shove
	// our value into another date as we apply it
	time::adjust_zone_utc(context);
	auto ct = time::clock_seconds_utc(obj.value.time);
	unsigned rel_parts = internal::get_property(obj, M_TM_CONTEXTUAL_YEAR | M_TM_CONTEXTUAL_MONTH, 0);
	if (!object::has_date(obj)) {
		obj.value.time.sec = time::date_seconds_utc(context);
		if (object::has_clock(obj)) {
			obj.value.time.sec += ct;
			object::set_time_type(obj, ObjectTimeType::date_and_clock);
		} else {
			object::set_time_type(obj, ObjectTimeType::date);
		}
	} else if (rel_parts) {
		Date date = time::gregorian::date_utc(obj.value.time);
		Date const context_date = time::gregorian::date_utc(context);
		if (rel_parts & M_TM_CONTEXTUAL_MONTH) {
			date.month = context_date.month;
			date.year = context_date.year; // implied
		} else/* if (rel_parts & M_TM_CONTEXTUAL_YEAR)*/ {
			date.year = context_date.year;
		}
		time::gregorian::set_utc(obj.value.time, date);
		if (object::has_clock(obj)) {
			obj.value.time.sec = time::date_seconds_utc(obj.value.time) + ct;
		}
	}
	internal::clear_property(obj, M_TM_UNZONED | M_TM_CONTEXTUAL_MONTH | M_TM_CONTEXTUAL_YEAR);
}

IGEN_PRIVATE
Object const* object::find_impl(
	Array<Object> const& collection,
	StringRef const& name,
	ObjectNameHash const name_hash
) {
	if (name_hash == OBJECT_NAME_NULL || array::empty(collection)) {
		return nullptr;
	}
	for (Object const& item : collection) {
		if (name_hash == item.name.hash) {
			#if defined(QUANTA_DEBUG)
			if (name.valid() && !string::compare_equal(name, object::name(item))) {
				TOGO_LOG_DEBUGF(
					"hashes matched, but names mismatched: '%.*s' != '%.*s' (lookup_name != name)\n",
					name.size, name.data,
					item.name.size, item.name.data
				);
			}
			#else
				(void)name;
			#endif
			return &item;
		}
	}
	return nullptr;
}

/// Find tag by name.
Object* object::find_tag(Object& obj, StringRef const& name) {
	ObjectNameHash const name_hash = object::hash_name(name);
	return const_cast<Object*>(object::find_impl(obj.tags, name, name_hash));
}

/// Find tag by name.
Object const* object::find_tag(Object const& obj, StringRef const& name) {
	ObjectNameHash const name_hash = object::hash_name(name);
	return object::find_impl(obj.tags, name, name_hash);
}

/// Find tag by name hash.
Object* object::find_tag(Object& obj, ObjectNameHash const name_hash) {
	return const_cast<Object*>(
		object::find_impl(obj.tags, StringRef{}, name_hash)
	);
}

/// Find tag by name hash.
Object const* object::find_tag(Object const& obj, ObjectNameHash const name_hash) {
	return object::find_impl(obj.tags, StringRef{}, name_hash);
}

/// Find child by name.
Object* object::find_child(Object& obj, StringRef const& name) {
	ObjectNameHash const name_hash = object::hash_name(name);
	return const_cast<Object*>(object::find_impl(obj.children, name, name_hash));
}

/// Find child by name.
Object const* object::find_child(Object const& obj, StringRef const& name) {
	ObjectNameHash const name_hash = object::hash_name(name);
	return object::find_impl(obj.children, name, name_hash);
}

/// Find child by name hash.
Object* object::find_child(Object& obj, ObjectNameHash const name_hash) {
	return const_cast<Object*>(
		object::find_impl(obj.children, StringRef{}, name_hash)
	);
}

/// Find child by name hash.
Object const* object::find_child(Object const& obj, ObjectNameHash const name_hash) {
	return object::find_impl(obj.children, StringRef{}, name_hash);
}

} // namespace quanta
