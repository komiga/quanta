#line 2 "quanta/core/object/object_li.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/object/object.hpp>
#include <quanta/core/lua/lua.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>
#include <togo/core/io/memory_stream.hpp>

namespace quanta {

namespace object {

TOGO_LI_FUNC_DEF(__module_init__) {
	lua::table_set_raw(L, "NAME_NULL", unsigned_cast(OBJECT_NAME_NULL));
	lua::table_set_raw(L, "VALUE_NULL", unsigned_cast(OBJECT_VALUE_NULL));

	lua_createtable(L, 0, 8);
	lua::table_set_copy_raw(L, -4, "Type", -1);
	lua::table_set_raw(L, "null", unsigned_cast(ObjectValueType::null));
	lua::table_set_raw(L, "boolean", unsigned_cast(ObjectValueType::boolean));
	lua::table_set_raw(L, "integer", unsigned_cast(ObjectValueType::integer));
	lua::table_set_raw(L, "decimal", unsigned_cast(ObjectValueType::decimal));
	lua::table_set_raw(L, "time", unsigned_cast(ObjectValueType::time));
	lua::table_set_raw(L, "string", unsigned_cast(ObjectValueType::string));
	lua::table_set_raw(L, "identifier", unsigned_cast(ObjectValueType::identifier));
	lua::table_set_raw(L, "expression", unsigned_cast(ObjectValueType::expression));
	lua_pop(L, 1);

	lua_createtable(L, 0, 5);
	lua::table_set_copy_raw(L, -4, "Operator", -1);
	lua::table_set_raw(L, "none", unsigned_cast(ObjectOperator::none));
	lua::table_set_raw(L, "add", unsigned_cast(ObjectOperator::add));
	lua::table_set_raw(L, "sub", unsigned_cast(ObjectOperator::sub));
	lua::table_set_raw(L, "mul", unsigned_cast(ObjectOperator::mul));
	lua::table_set_raw(L, "div", unsigned_cast(ObjectOperator::div));
	lua_pop(L, 1);

	lua_createtable(L, 0, 3);
	lua::table_set_copy_raw(L, -4, "TimeType", -1);
	lua::table_set_raw(L, "date_and_clock", unsigned_cast(ObjectTimeType::date_and_clock));
	lua::table_set_raw(L, "date", unsigned_cast(ObjectTimeType::date));
	lua::table_set_raw(L, "clock", unsigned_cast(ObjectTimeType::clock));
	lua_pop(L, 1);

	return 0;
}

TOGO_LI_FUNC_DEF(hash_name) {
	auto name = lua::get_string(L, 1);
	lua::push_value(L, object::hash_name(name));
	return 1;
}

TOGO_LI_FUNC_DEF(hash_value) {
	auto value = lua::get_string(L, 1);
	lua::push_value(L, object::hash_value(value));
	return 1;
}

static signed li_create_base(lua_State* L, bool single_value) {
	Object* obj = TOGO_CONSTRUCT_DEFAULT(memory::default_allocator(), Object);
	if (lua_type(L, 1) == LUA_TSTRING) {
		if (!object::read_text_string(*obj, lua::get_string(L, 1), single_value)) {
			TOGO_DESTROY(memory::default_allocator(), obj);
			obj = nullptr;
		}
	}
	lua::push_lightuserdata(L, obj);
	return 1;
}

// text, single_value = true
TOGO_LI_FUNC_DEF(create) {
	return li_create_base(L, luaL_opt(L, lua::get_boolean, 3, true));
}

TOGO_LI_FUNC_DEF(create_mv) {
	return li_create_base(L, false);
}

TOGO_LI_FUNC_DEF(destroy) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	TOGO_DESTROY(memory::default_allocator(), obj);
	return 0;
}

TOGO_LI_FUNC_DEF(type) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, unsigned_cast(object::type(*obj)));
	return 1;
}

TOGO_LI_FUNC_DEF(set_type) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	auto type = static_cast<ObjectValueType>(luaL_checkinteger(L, 2));
	lua::push_value(L, object::set_type(*obj, type));
	return 1;
}

TOGO_LI_FUNC_DEF(is_type) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	auto type = static_cast<ObjectValueType>(luaL_checkinteger(L, 2));
	lua::push_value(L, object::is_type(*obj, type));
	return 1;
}

TOGO_LI_FUNC_DEF(is_type_any) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	auto type = static_cast<ObjectValueType>(luaL_checkinteger(L, 2));
	lua::push_value(L, object::is_type_any(*obj, type));
	return 1;
}

TOGO_LI_FUNC_DEF(is_null) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::is_null(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(is_boolean) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::is_boolean(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(is_integer) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::is_integer(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(is_decimal) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::is_decimal(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(is_numeric) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::is_numeric(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(is_time) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::is_time(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(is_string) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::is_string(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(is_identifier) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::is_identifier(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(is_textual) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::is_textual(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(is_expression) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::is_expression(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(name) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::name(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(name_hash) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::name_hash(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(is_named) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::is_named(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(set_name) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_name(*obj, lua::get_string(L, 2));
	return 0;
}

TOGO_LI_FUNC_DEF(clear_name) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::clear_name(*obj);
	return 0;
}

TOGO_LI_FUNC_DEF(op) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, unsigned_cast(object::op(*obj)));
	return 1;
}

TOGO_LI_FUNC_DEF(set_op) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_op(*obj, static_cast<ObjectOperator>(luaL_checkinteger(L, 2)));
	return 0;
}

TOGO_LI_FUNC_DEF(source) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::source(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(marker_source_uncertain) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::marker_source_uncertain(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(sub_source) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::sub_source(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(marker_sub_source_uncertain) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::marker_sub_source_uncertain(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(has_source) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::has_source(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(has_sub_source) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::has_sub_source(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(source_certain) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::source_certain(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(source_certain_or_unspecified) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::source_certain_or_unspecified(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(clear_source_uncertainty) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::clear_source_uncertainty(*obj);
	return 0;
}

TOGO_LI_FUNC_DEF(set_source_certain) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_source_certain(*obj, lua::get_boolean(L, 2));
	return 0;
}

TOGO_LI_FUNC_DEF(set_source) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_source(*obj, luaL_checkinteger(L, 2));
	return 0;
}

TOGO_LI_FUNC_DEF(set_sub_source_certain) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_sub_source_certain(*obj, lua::get_boolean(L, 2));
	return 0;
}

TOGO_LI_FUNC_DEF(set_sub_source) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_sub_source(*obj, luaL_checkinteger(L, 2));
	return 0;
}

TOGO_LI_FUNC_DEF(clear_source) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::clear_source(*obj);
	return 0;
}

TOGO_LI_FUNC_DEF(marker_value_uncertain) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::marker_value_uncertain(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(marker_value_guess) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::marker_value_guess(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(value_approximation) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::value_approximation(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(value_certain) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::value_certain(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(set_value_certain) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_value_certain(*obj, lua::get_boolean(L, 2));
	return 0;
}

TOGO_LI_FUNC_DEF(set_value_guess) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_value_guess(*obj, lua::get_boolean(L, 2));
	return 0;
}

TOGO_LI_FUNC_DEF(set_value_approximation) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_value_approximation(*obj, luaL_checkinteger(L, 2));
	return 0;
}

TOGO_LI_FUNC_DEF(clear_value_markers) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::clear_value_markers(*obj);
	return 0;
}

TOGO_LI_FUNC_DEF(clear_value) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::clear_value(*obj);
	return 0;
}

TOGO_LI_FUNC_DEF(clear) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::clear(*obj);
	return 0;
}

TOGO_LI_FUNC_DEF(copy) {
	auto a = lua::get_lightuserdata_typed<Object>(L, 1);
	auto b = lua::get_lightuserdata_typed<Object>(L, 2);
	object::copy(*a, *b);
	return 0;
}

TOGO_LI_FUNC_DEF(set_null) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_null(*obj);
	return 0;
}

TOGO_LI_FUNC_DEF(boolean) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::boolean(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(set_boolean) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_boolean(*obj, lua::get_boolean(L, 2));
	return 0;
}

TOGO_LI_FUNC_DEF(integer) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::integer(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(set_integer) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	auto value = luaL_checkinteger(L, 2);
	if (lua_isnone(L, 3)) {
		object::set_integer(*obj, value);
	} else {
		auto unit = lua::get_string(L, 3);
		object::set_integer(*obj, value, unit);
	}
	return 0;
}

TOGO_LI_FUNC_DEF(decimal) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::decimal(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(set_decimal) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	auto value = luaL_checknumber(L, 2);
	if (lua_isnone(L, 3)) {
		object::set_decimal(*obj, value);
	} else {
		auto unit = lua::get_string(L, 3);
		object::set_decimal(*obj, value, unit);
	}
	return 0;
}

TOGO_LI_FUNC_DEF(numeric) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	if (object::is_decimal(*obj)) {
		lua::push_value(L, object::decimal(*obj));
	} else {
		// type check in integer()
		lua::push_value(L, object::integer(*obj));
	}
	return 1;
}

TOGO_LI_FUNC_DEF(unit) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::unit(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(unit_hash) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::unit_hash(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(has_unit) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::has_unit(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(set_unit) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_unit(*obj, lua::get_string(L, 2));
	return 0;
}

TOGO_LI_FUNC_DEF(time) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_lightuserdata(L, &object::time_value(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(time_type) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, unsigned_cast(object::time_type(*obj)));
	return 1;
}

TOGO_LI_FUNC_DEF(has_date) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::has_date(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(has_clock) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::has_clock(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(is_zoned) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::is_zoned(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(is_year_contextual) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::is_year_contextual(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(is_month_contextual) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::is_month_contextual(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(set_zoned) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_zoned(*obj, lua::get_boolean(L, 2), /*adjust = */luaL_opt(L, lua::get_boolean, 3, true));
	return 0;
}

TOGO_LI_FUNC_DEF(set_year_contextual) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_year_contextual(*obj, lua::get_boolean(L, 2));
	return 0;
}

TOGO_LI_FUNC_DEF(set_month_contextual) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_month_contextual(*obj, lua::get_boolean(L, 2));
	return 0;
}

TOGO_LI_FUNC_DEF(set_time_type) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_time_type(*obj, static_cast<ObjectTimeType>(luaL_checkinteger(L, 2)));
	return 0;
}

TOGO_LI_FUNC_DEF(set_time_value) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	auto t = lua::get_pointer<Time const>(L, 2);
	object::set_time_value(*obj, *t);
	return 0;
}

TOGO_LI_FUNC_DEF(set_time) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	auto t = lua::get_pointer<Time const>(L, 2);
	object::set_time(*obj, *t);
	return 0;
}

TOGO_LI_FUNC_DEF(set_time_date) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	auto t = lua::get_pointer<Time const>(L, 2);
	object::set_time_date(*obj, *t);
	return 0;
}

TOGO_LI_FUNC_DEF(set_time_clock) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	auto t = lua::get_pointer<Time const>(L, 2);
	object::set_time_clock(*obj, *t);
	return 0;
}

TOGO_LI_FUNC_DEF(resolve_time) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	auto t = lua::get_pointer<Time const>(L, 2);
	object::resolve_time(*obj, *t);
	return 0;
}

TOGO_LI_FUNC_DEF(string) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::string(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(set_string) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_string(*obj, lua::get_string(L, 2));
	return 0;
}

TOGO_LI_FUNC_DEF(identifier) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::identifier(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(identifier_hash) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::identifier_hash(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(set_identifier) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_identifier(*obj, lua::get_string(L, 2));
	return 0;
}

TOGO_LI_FUNC_DEF(text) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::text(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(set_expression) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::set_expression(*obj);
	return 0;
}

TOGO_LI_FUNC_DEF(array_iter) {
	// 1-based index
	auto& a = *lua::get_lightuserdata_typed<Array<Object>>(L, 1);
	auto i = luaL_checkinteger(L, 2);
	++i;
	if (i <= signed_cast(array::size(a))) {
		lua::push_value(L, i);
		lua::push_lightuserdata(L, &a[i - 1]);
		return 2;
	}
	return 0;
}

// returns nil on error
static signed TOGO_LI_FUNC(push_sub)(lua_State* L, Object* obj, Array<Object>& a, bool sv_default) {
	Object* sub = nullptr;
	if (lua_isnone(L, 2)) {
		sub = &array::push_back_inplace(a);
	} else if (lua_isstring(L, 2)) {
		auto text = lua::get_string(L, 2);
		bool single_value = luaL_opt(L, lua::get_boolean, 3, sv_default);
		auto parsed = TOGO_CONSTRUCT_DEFAULT(memory::default_allocator(), Object);
		if (object::read_text_string(*parsed, text, single_value)) {
			sub = &array::push_back_inplace(a, rvalue_ref(*parsed));
		}
		TOGO_DESTROY(memory::default_allocator(), parsed);
	} else {
		auto sub = lua::get_lightuserdata_typed<Object>(L, 2);
		TOGO_DEBUG_ASSERTE(obj != sub);
		sub = &array::push_back_inplace(a, *sub);
	}
	lua::push_lightuserdata(L, sub);
	return 1;
}

static signed TOGO_LI_FUNC(remove_sub)(lua_State* L, Object* /*obj*/, Array<Object>& a) {
	auto i = luaL_checkinteger(L, 2);
	luaL_argcheck(L, i >= 1 && i <= signed_cast(array::size(a)), 2, "index out of bounds");
	array::remove(a, i - 1);
	return 0;
}

static signed TOGO_LI_FUNC(sub_at)(lua_State* L, Object* /*obj*/, Array<Object>& a) {
	auto i = luaL_checkinteger(L, 2);
	luaL_argcheck(L, i >= 1 && i <= signed_cast(array::size(a)), 2, "index out of bounds");
	lua::push_lightuserdata(L, &a[i - 1]);
	return 1;
}

TOGO_LI_FUNC_DEF(children) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, TOGO_LI_FUNC(array_iter));
	lua::push_lightuserdata(L, &object::children(*obj));
	lua::push_value(L, 0);
	return 3;
}

TOGO_LI_FUNC_DEF(num_children) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, array::size(object::children(*obj)));
	return 1;
}

TOGO_LI_FUNC_DEF(has_children) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::has_children(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(clear_children) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::clear_children(*obj);
	return 0;
}

TOGO_LI_FUNC_DEF(push_child) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	return li_push_sub(L, obj, object::children(*obj), true);
}

TOGO_LI_FUNC_DEF(push_child_mv) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	return li_push_sub(L, obj, object::children(*obj), false);
}

TOGO_LI_FUNC_DEF(pop_child) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	array::pop_back(object::children(*obj));
	return 0;
}

TOGO_LI_FUNC_DEF(remove_child) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	return li_remove_sub(L, obj, object::children(*obj));
}

TOGO_LI_FUNC_DEF(child_at) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	return li_sub_at(L, obj, object::children(*obj));
}

TOGO_LI_FUNC_DEF(find_child) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	Object* result = nullptr;
	if (lua_type(L, 2) == LUA_TSTRING) {
		result = object::find_child(*obj, lua::get_string(L, 2));
	} else {
		result = object::find_child(*obj, static_cast<ObjectNameHash>(luaL_checkinteger(L, 2)));
	}
	lua::push_lightuserdata(L, result);
	return 1;
}

TOGO_LI_FUNC_DEF(tags) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, li_array_iter);
	lua::push_lightuserdata(L, &object::tags(*obj));
	lua::push_value(L, 0);
	return 3;
}

TOGO_LI_FUNC_DEF(num_tags) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, array::size(object::tags(*obj)));
	return 1;
}

TOGO_LI_FUNC_DEF(has_tags) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::has_tags(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(clear_tags) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::clear_tags(*obj);
	return 0;
}

TOGO_LI_FUNC_DEF(push_tag) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	return li_push_sub(L, obj, object::tags(*obj), true);
}

TOGO_LI_FUNC_DEF(push_tag_mv) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	return li_push_sub(L, obj, object::tags(*obj), false);
}

TOGO_LI_FUNC_DEF(pop_tag) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	array::pop_back(object::tags(*obj));
	return 0;
}

TOGO_LI_FUNC_DEF(remove_tag) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	return li_remove_sub(L, obj, object::tags(*obj));
}

TOGO_LI_FUNC_DEF(tag_at) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	return li_sub_at(L, obj, object::tags(*obj));
}

TOGO_LI_FUNC_DEF(find_tag) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	Object* result = nullptr;
	if (lua_type(L, 2) == LUA_TSTRING) {
		result = object::find_tag(*obj, lua::get_string(L, 2));
	} else {
		result = object::find_tag(*obj, static_cast<ObjectNameHash>(luaL_checkinteger(L, 2)));
	}
	lua::push_lightuserdata(L, result);
	return 1;
}

TOGO_LI_FUNC_DEF(quantity) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_lightuserdata(L, object::quantity(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(has_quantity) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_value(L, object::has_quantity(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(clear_quantity) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::clear_quantity(*obj);
	return 0;
}

TOGO_LI_FUNC_DEF(make_quantity) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	lua::push_lightuserdata(L, &object::make_quantity(*obj));
	return 1;
}

TOGO_LI_FUNC_DEF(release_quantity) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	object::release_quantity(*obj);
	return 0;
}

// obj, path, single_value = false
TOGO_LI_FUNC_DEF(read_text_file) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	auto path = lua::get_string(L, 2);
	bool single_value = luaL_opt(L, lua::get_boolean, 3, false);
	lua::push_value(L, object::read_text_file(*obj, path, single_value));
	return 1;
}

// obj, text, single_value = false
TOGO_LI_FUNC_DEF(read_text_string) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	auto text = lua::get_string(L, 2);
	bool single_value = luaL_opt(L, lua::get_boolean, 3, false);
	lua::push_value(L, object::read_text_string(*obj, text, single_value));
	return 1;
}

// obj, path, single_value = false
TOGO_LI_FUNC_DEF(write_text_file) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	auto path = lua::get_string(L, 2);
	bool single_value = luaL_opt(L, lua::get_boolean, 3, false);
	lua::push_value(L, object::write_text_file(*obj, path, single_value));
	return 1;
}

// obj, single_value = false, capacity = 512
TOGO_LI_FUNC_DEF(write_text_string) {
	auto obj = lua::get_lightuserdata_typed<Object>(L, 1);
	bool single_value = luaL_opt(L, lua::get_boolean, 2, false);
	auto size = luaL_opt(L, luaL_checkinteger, 3, 512);
	MemoryStream stream{memory::scratch_allocator(), static_cast<unsigned>(size)};
	if (object::write_text(*obj, stream, single_value)) {
		lua::push_value(L, {
			reinterpret_cast<char*>(array::begin(stream.data())),
			static_cast<unsigned>(stream.size())
		});
	} else {
		lua::push_value(L, null_tag{});
	}
	return 1;
}

} // namespace object

namespace {

static LuaModuleFunctionArray const li_funcs{
	TOGO_LI_FUNC_REF(object, __module_init__)

	TOGO_LI_FUNC_REF(object, hash_name)
	TOGO_LI_FUNC_REF(object, hash_value)

	TOGO_LI_FUNC_REF(object, create)
	TOGO_LI_FUNC_REF(object, create_mv)
	TOGO_LI_FUNC_REF(object, destroy)

	TOGO_LI_FUNC_REF(object, type)
	TOGO_LI_FUNC_REF(object, set_type)
	TOGO_LI_FUNC_REF(object, is_type)
	TOGO_LI_FUNC_REF(object, is_type_any)
	TOGO_LI_FUNC_REF(object, is_null)
	TOGO_LI_FUNC_REF(object, is_boolean)
	TOGO_LI_FUNC_REF(object, is_integer)
	TOGO_LI_FUNC_REF(object, is_decimal)
	TOGO_LI_FUNC_REF(object, is_numeric)
	TOGO_LI_FUNC_REF(object, is_time)
	TOGO_LI_FUNC_REF(object, is_string)
	TOGO_LI_FUNC_REF(object, is_identifier)
	TOGO_LI_FUNC_REF(object, is_textual)
	TOGO_LI_FUNC_REF(object, is_expression)

	TOGO_LI_FUNC_REF(object, name)
	TOGO_LI_FUNC_REF(object, name_hash)
	TOGO_LI_FUNC_REF(object, is_named)
	TOGO_LI_FUNC_REF(object, set_name)
	TOGO_LI_FUNC_REF(object, clear_name)

	TOGO_LI_FUNC_REF(object, op)
	TOGO_LI_FUNC_REF(object, set_op)

	TOGO_LI_FUNC_REF(object, source)
	TOGO_LI_FUNC_REF(object, marker_source_uncertain)
	TOGO_LI_FUNC_REF(object, sub_source)
	TOGO_LI_FUNC_REF(object, marker_sub_source_uncertain)
	TOGO_LI_FUNC_REF(object, has_source)
	TOGO_LI_FUNC_REF(object, has_sub_source)
	TOGO_LI_FUNC_REF(object, source_certain)
	TOGO_LI_FUNC_REF(object, source_certain_or_unspecified)
	TOGO_LI_FUNC_REF(object, clear_source_uncertainty)
	TOGO_LI_FUNC_REF(object, set_source_certain)
	TOGO_LI_FUNC_REF(object, set_source)
	TOGO_LI_FUNC_REF(object, set_sub_source_certain)
	TOGO_LI_FUNC_REF(object, set_sub_source)
	TOGO_LI_FUNC_REF(object, clear_source)

	TOGO_LI_FUNC_REF(object, marker_value_uncertain)
	TOGO_LI_FUNC_REF(object, marker_value_guess)
	TOGO_LI_FUNC_REF(object, value_approximation)
	TOGO_LI_FUNC_REF(object, value_certain)
	TOGO_LI_FUNC_REF(object, set_value_certain)
	TOGO_LI_FUNC_REF(object, set_value_guess)
	TOGO_LI_FUNC_REF(object, set_value_approximation)
	TOGO_LI_FUNC_REF(object, clear_value_markers)
	TOGO_LI_FUNC_REF(object, clear_value)
	TOGO_LI_FUNC_REF(object, clear)
	TOGO_LI_FUNC_REF(object, copy)

	TOGO_LI_FUNC_REF(object, set_null)

	TOGO_LI_FUNC_REF(object, boolean)
	TOGO_LI_FUNC_REF(object, set_boolean)

	TOGO_LI_FUNC_REF(object, integer)
	TOGO_LI_FUNC_REF(object, set_integer)
	TOGO_LI_FUNC_REF(object, decimal)
	TOGO_LI_FUNC_REF(object, set_decimal)
	TOGO_LI_FUNC_REF(object, numeric)
	TOGO_LI_FUNC_REF(object, unit)
	TOGO_LI_FUNC_REF(object, unit_hash)
	TOGO_LI_FUNC_REF(object, has_unit)
	TOGO_LI_FUNC_REF(object, set_unit)

	TOGO_LI_FUNC_REF(object, time)
	TOGO_LI_FUNC_REF(object, time_type)
	TOGO_LI_FUNC_REF(object, has_date)
	TOGO_LI_FUNC_REF(object, has_clock)
	TOGO_LI_FUNC_REF(object, is_zoned)
	TOGO_LI_FUNC_REF(object, is_year_contextual)
	TOGO_LI_FUNC_REF(object, is_month_contextual)
	TOGO_LI_FUNC_REF(object, set_zoned)
	TOGO_LI_FUNC_REF(object, set_year_contextual)
	TOGO_LI_FUNC_REF(object, set_month_contextual)
	TOGO_LI_FUNC_REF(object, set_time_type)
	TOGO_LI_FUNC_REF(object, set_time_value)
	TOGO_LI_FUNC_REF(object, set_time)
	TOGO_LI_FUNC_REF(object, set_time_date)
	TOGO_LI_FUNC_REF(object, set_time_clock)
	TOGO_LI_FUNC_REF(object, resolve_time)

	TOGO_LI_FUNC_REF(object, string)
	TOGO_LI_FUNC_REF(object, set_string)

	TOGO_LI_FUNC_REF(object, identifier)
	TOGO_LI_FUNC_REF(object, identifier_hash)
	TOGO_LI_FUNC_REF(object, set_identifier)

	TOGO_LI_FUNC_REF(object, text)

	TOGO_LI_FUNC_REF(object, set_expression)

	TOGO_LI_FUNC_REF(object, children)
	TOGO_LI_FUNC_REF(object, num_children)
	TOGO_LI_FUNC_REF(object, has_children)
	TOGO_LI_FUNC_REF(object, clear_children)
	TOGO_LI_FUNC_REF(object, push_child)
	TOGO_LI_FUNC_REF(object, push_child_mv)
	TOGO_LI_FUNC_REF(object, pop_child)
	TOGO_LI_FUNC_REF(object, remove_child)
	TOGO_LI_FUNC_REF(object, child_at)
	TOGO_LI_FUNC_REF(object, find_child)

	TOGO_LI_FUNC_REF(object, tags)
	TOGO_LI_FUNC_REF(object, num_tags)
	TOGO_LI_FUNC_REF(object, has_tags)
	TOGO_LI_FUNC_REF(object, clear_tags)
	TOGO_LI_FUNC_REF(object, push_tag)
	TOGO_LI_FUNC_REF(object, push_tag_mv)
	TOGO_LI_FUNC_REF(object, pop_tag)
	TOGO_LI_FUNC_REF(object, remove_tag)
	TOGO_LI_FUNC_REF(object, tag_at)
	TOGO_LI_FUNC_REF(object, find_tag)

	TOGO_LI_FUNC_REF(object, quantity)
	TOGO_LI_FUNC_REF(object, has_quantity)
	TOGO_LI_FUNC_REF(object, clear_quantity)
	TOGO_LI_FUNC_REF(object, make_quantity)
	TOGO_LI_FUNC_REF(object, release_quantity)

	TOGO_LI_FUNC_REF(object, read_text_file)
	TOGO_LI_FUNC_REF(object, read_text_string)
	TOGO_LI_FUNC_REF(object, write_text_file)
	TOGO_LI_FUNC_REF(object, write_text_string)
};

static LuaModuleRef const li_module{
	"Quanta.Object",
	"quanta/core/object/Object.lua",
	li_funcs,
	#include <quanta/core/object/Object.lua>
};

} // anonymous namespace

/// Register the Lua interface.
void object::register_lua_interface(lua_State* L) {
	lua::preload_module(L, li_module);
}

} // namespace quanta
