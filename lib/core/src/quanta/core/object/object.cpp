#line 2 "quanta/core/object/object.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>
#include <togo/core/memory/memory.hpp>

#include <quanta/core/string/string.hpp>
#include <quanta/core/object/object.hpp>

namespace quanta {

/// Set type.
///
/// Returns true if type changed.
bool object::set_type(Object& obj, ObjectValueType const type) {
	if (object::type(obj) == type) {
		return false;
	}
	object::clear_value(obj);
	obj.properties = (obj.properties & ~Object::M_TYPE) | unsigned_cast(type);
	return true;
}

/// Reset value to type default.
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
		string::clear(obj.value.numeric.unit, a);
		break;
	case ObjectValueType::decimal:
		obj.value.numeric.decimal = 0.0f;
		string::clear(obj.value.numeric.unit, a);
		break;
	case ObjectValueType::string:
		string::clear(obj.value.string, a);
		break;
	}
}

/// Clear tags.
void object::clear_tags(Object& obj) {
	array::clear(obj.tags);
}

/// Clear children.
void object::clear_children(Object& obj) {
	array::clear(obj.children);
}

/// Copy an object.
void object::copy(Object& dst, Object const& src) {
	auto& a = memory::default_allocator();
	dst.source = src.source;
	dst.sub_source = src.sub_source;
	dst.properties = src.properties;
	string::set(dst.name, src.name, a);
	object::set_type(dst, object::type(src));
	switch (object::type(dst)) {
	case ObjectValueType::null:
		break;
	case ObjectValueType::boolean:
		dst.value.boolean = src.value.boolean;
		break;
	case ObjectValueType::integer:
		dst.value.numeric.integer = src.value.numeric.integer;
		string::set(dst.value.numeric.unit, src.value.numeric.unit, a);
		break;
	case ObjectValueType::decimal:
		dst.value.numeric.decimal = src.value.numeric.decimal;
		string::set(dst.value.numeric.unit, src.value.numeric.unit, a);
		break;
	case ObjectValueType::string:
		string::set(dst.value.string, src.value.string, a);
		break;
	}
	array::copy(dst.tags, src.tags);
	array::copy(dst.children, src.children);
	if (src.quantity) {
		if (!dst.quantity) {
			dst.quantity = TOGO_CONSTRUCT(a, Object, *src.quantity);
		} else {
			object::copy(*dst.quantity, *src.quantity);
		}
	}
}

namespace object {

Object& Object::operator=(Object const& other) {
	object::copy(*this, other);
	return *this;
}

} // namespace object

} // namespace quanta
