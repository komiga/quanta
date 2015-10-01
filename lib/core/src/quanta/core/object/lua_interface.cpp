#line 2 "quanta/core/object/lua_binding.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/object/object.hpp>
#include <quanta/core/scripting/scripting.hpp>

#include <togo/core/utility/utility.hpp>
#include <togo/core/io/memory_stream.hpp>

namespace quanta {

namespace object {

#define LI_FUNC(name) li_##name
#define LI_FUNC_DEF(name) static signed LI_FUNC(name)(lua_State* L)

LI_FUNC_DEF(hash_name) {
	auto name = lua::get_string(L, 1, false);
	lua_pushinteger(L, object::hash_name(name));
	return 1;
}

LI_FUNC_DEF(hash_unit) {
	auto name = lua::get_string(L, 1, false);
	lua_pushinteger(L, object::hash_unit(name));
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
	lua::push_lud(L, obj);
	return 1;
}

// text, single_value = true
LI_FUNC_DEF(create) {
	return li_create_base(L, luaL_opt(L, lua::get_boolean, 3, true));
}

LI_FUNC_DEF(create_mv) {
	return li_create_base(L, false);
}

LI_FUNC_DEF(destroy) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	TOGO_DESTROY(memory::default_allocator(), obj);
	return 0;
}

LI_FUNC_DEF(type) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushinteger(L, unsigned_cast(object::type(*obj)));
	return 1;
}

LI_FUNC_DEF(set_type) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	auto type = static_cast<ObjectValueType>(luaL_checkinteger(L, 2));
	lua_pushboolean(L, object::set_type(*obj, type));
	return 1;
}

LI_FUNC_DEF(is_type) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	auto type = static_cast<ObjectValueType>(luaL_checkinteger(L, 2));
	lua_pushboolean(L, object::is_type(*obj, type));
	return 1;
}

LI_FUNC_DEF(is_type_any) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	auto type = static_cast<ObjectValueType>(luaL_checkinteger(L, 2));
	lua_pushboolean(L, object::is_type_any(*obj, type));
	return 1;
}

LI_FUNC_DEF(is_null) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::is_null(*obj));
	return 1;
}

LI_FUNC_DEF(is_boolean) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::is_boolean(*obj));
	return 1;
}

LI_FUNC_DEF(is_integer) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::is_integer(*obj));
	return 1;
}

LI_FUNC_DEF(is_decimal) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::is_decimal(*obj));
	return 1;
}

LI_FUNC_DEF(is_numeric) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::is_numeric(*obj));
	return 1;
}

LI_FUNC_DEF(is_time) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::is_time(*obj));
	return 1;
}

LI_FUNC_DEF(is_string) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::is_string(*obj));
	return 1;
}

LI_FUNC_DEF(is_expression) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::is_expression(*obj));
	return 1;
}

LI_FUNC_DEF(name) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua::push_string(L, object::name(*obj));
	return 1;
}

LI_FUNC_DEF(name_hash) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushinteger(L, object::name_hash(*obj));
	return 1;
}

LI_FUNC_DEF(is_named) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::is_named(*obj));
	return 1;
}

LI_FUNC_DEF(set_name) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_name(*obj, lua::get_string(L, 2));
	return 0;
}

LI_FUNC_DEF(clear_name) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::clear_name(*obj);
	return 0;
}

LI_FUNC_DEF(op) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushinteger(L, unsigned_cast(object::op(*obj)));
	return 1;
}

LI_FUNC_DEF(set_op) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_op(*obj, static_cast<ObjectOperator>(luaL_checkinteger(L, 2)));
	return 0;
}

LI_FUNC_DEF(source) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushinteger(L, object::source(*obj));
	return 1;
}

LI_FUNC_DEF(marker_source_uncertain) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::marker_source_uncertain(*obj));
	return 1;
}

LI_FUNC_DEF(sub_source) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushinteger(L, object::sub_source(*obj));
	return 1;
}

LI_FUNC_DEF(marker_sub_source_uncertain) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::marker_sub_source_uncertain(*obj));
	return 1;
}

LI_FUNC_DEF(has_source) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::has_source(*obj));
	return 1;
}

LI_FUNC_DEF(has_sub_source) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::has_sub_source(*obj));
	return 1;
}

LI_FUNC_DEF(source_certain) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::source_certain(*obj));
	return 1;
}

LI_FUNC_DEF(source_certain_or_unspecified) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::source_certain_or_unspecified(*obj));
	return 1;
}

LI_FUNC_DEF(clear_source_uncertainty) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::clear_source_uncertainty(*obj);
	return 0;
}

LI_FUNC_DEF(set_source_certain) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_source_certain(*obj, lua::get_boolean(L, 2));
	return 0;
}

LI_FUNC_DEF(set_source) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_source(*obj, luaL_checkinteger(L, 2));
	return 0;
}

LI_FUNC_DEF(set_sub_source_certain) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_sub_source_certain(*obj, lua::get_boolean(L, 2));
	return 0;
}

LI_FUNC_DEF(set_sub_source) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_sub_source(*obj, luaL_checkinteger(L, 2));
	return 0;
}

LI_FUNC_DEF(clear_source) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::clear_source(*obj);
	return 0;
}

LI_FUNC_DEF(marker_value_uncertain) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::marker_value_uncertain(*obj));
	return 1;
}

LI_FUNC_DEF(marker_value_guess) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::marker_value_guess(*obj));
	return 1;
}

LI_FUNC_DEF(value_approximation) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushinteger(L, object::value_approximation(*obj));
	return 1;
}

LI_FUNC_DEF(value_certain) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::value_certain(*obj));
	return 1;
}

LI_FUNC_DEF(set_value_certain) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_value_certain(*obj, lua::get_boolean(L, 2));
	return 0;
}

LI_FUNC_DEF(set_value_guess) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_value_guess(*obj, lua::get_boolean(L, 2));
	return 0;
}

LI_FUNC_DEF(set_value_approximation) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_value_approximation(*obj, luaL_checkinteger(L, 2));
	return 0;
}

LI_FUNC_DEF(clear_value_markers) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::clear_value_markers(*obj);
	return 0;
}

LI_FUNC_DEF(clear_value) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::clear_value(*obj);
	return 0;
}

LI_FUNC_DEF(clear) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::clear(*obj);
	return 0;
}

LI_FUNC_DEF(copy) {
	auto a = lua::get_lud_t<Object>(L, 1);
	auto b = lua::get_lud_t<Object>(L, 2);
	object::copy(*a, *b);
	return 0;
}

LI_FUNC_DEF(set_null) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_null(*obj);
	return 0;
}

LI_FUNC_DEF(boolean) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::boolean(*obj));
	return 1;
}

LI_FUNC_DEF(set_boolean) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_boolean(*obj, lua::get_boolean(L, 2));
	return 0;
}

LI_FUNC_DEF(integer) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushinteger(L, object::integer(*obj));
	return 1;
}

LI_FUNC_DEF(set_integer) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_integer(*obj, luaL_checkinteger(L, 2));
	return 0;
}

LI_FUNC_DEF(decimal) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushnumber(L, object::decimal(*obj));
	return 1;
}

LI_FUNC_DEF(set_decimal) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_decimal(*obj, luaL_checknumber(L, 2));
	return 0;
}

LI_FUNC_DEF(numeric) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	if (object::is_decimal(*obj)) {
		lua_pushnumber(L, object::decimal(*obj));
	} else {
		// type check in integer()
		lua_pushinteger(L, object::integer(*obj));
	}
	return 1;
}

LI_FUNC_DEF(unit) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua::push_string(L, object::unit(*obj));
	return 1;
}

LI_FUNC_DEF(unit_hash) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushinteger(L, object::unit_hash(*obj));
	return 1;
}

LI_FUNC_DEF(has_unit) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::has_unit(*obj));
	return 1;
}

LI_FUNC_DEF(set_unit) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_unit(*obj, lua::get_string(L, 2));
	return 0;
}

LI_FUNC_DEF(time) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua::push_lud(L, &object::time_value(*obj));
	return 1;
}

LI_FUNC_DEF(time_type) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushinteger(L, unsigned_cast(object::time_type(*obj)));
	return 1;
}

LI_FUNC_DEF(has_date) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::has_date(*obj));
	return 1;
}

LI_FUNC_DEF(has_clock) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::has_clock(*obj));
	return 1;
}

LI_FUNC_DEF(is_zoned) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::is_zoned(*obj));
	return 1;
}

LI_FUNC_DEF(is_year_contextual) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::is_year_contextual(*obj));
	return 1;
}

LI_FUNC_DEF(is_month_contextual) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::is_month_contextual(*obj));
	return 1;
}

LI_FUNC_DEF(set_zoned) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_zoned(*obj, lua::get_boolean(L, 2), /*adjust = */luaL_opt(L, lua::get_boolean, 3, true));
	return 0;
}

LI_FUNC_DEF(set_year_contextual) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_year_contextual(*obj, lua::get_boolean(L, 2));
	return 0;
}

LI_FUNC_DEF(set_month_contextual) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_month_contextual(*obj, lua::get_boolean(L, 2));
	return 0;
}

LI_FUNC_DEF(set_time_type) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_time_type(*obj, static_cast<ObjectTimeType>(luaL_checkinteger(L, 2)));
	return 0;
}

LI_FUNC_DEF(set_time_value) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	auto t = lua::get_lud_t<Time const>(L, 2);
	object::set_time_value(*obj, *t);
	return 0;
}

LI_FUNC_DEF(set_time) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	auto t = lua::get_lud_t<Time const>(L, 2);
	object::set_time(*obj, *t);
	return 0;
}

LI_FUNC_DEF(set_time_date) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	auto t = lua::get_lud_t<Time const>(L, 2);
	object::set_time_date(*obj, *t);
	return 0;
}

LI_FUNC_DEF(set_time_clock) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	auto t = lua::get_lud_t<Time const>(L, 2);
	object::set_time_clock(*obj, *t);
	return 0;
}

LI_FUNC_DEF(resolve_time) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	auto t = lua::get_lud_t<Time const>(L, 2);
	object::resolve_time(*obj, *t);
	return 0;
}

LI_FUNC_DEF(string) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua::push_string(L, object::string(*obj));
	return 1;
}

LI_FUNC_DEF(set_string) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_string(*obj, lua::get_string(L, 2));
	return 0;
}

LI_FUNC_DEF(set_expression) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::set_expression(*obj);
	return 0;
}

LI_FUNC_DEF(has_children) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::has_children(*obj));
	return 1;
}

LI_FUNC_DEF(clear_children) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::clear_children(*obj);
	return 0;
}

LI_FUNC_DEF(find_child) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	Object* result = nullptr;
	if (lua_type(L, 2) == LUA_TSTRING) {
		result = object::find_child(*obj, lua::get_string(L, 2));
	} else {
		result = object::find_child(*obj, static_cast<ObjectNameHash>(luaL_checkinteger(L, 2)));
	}
	lua::push_lud(L, result);
	return 1;
}

LI_FUNC_DEF(has_tags) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::has_tags(*obj));
	return 1;
}

LI_FUNC_DEF(clear_tags) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::clear_tags(*obj);
	return 0;
}

LI_FUNC_DEF(find_tag) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	Object* result = nullptr;
	if (lua_type(L, 2) == LUA_TSTRING) {
		result = object::find_tag(*obj, lua::get_string(L, 2));
	} else {
		result = object::find_tag(*obj, static_cast<ObjectNameHash>(luaL_checkinteger(L, 2)));
	}
	lua::push_lud(L, result);
	return 1;
}

LI_FUNC_DEF(quantity) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua::push_lud(L, object::quantity(*obj));
	return 1;
}

LI_FUNC_DEF(has_quantity) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua_pushboolean(L, object::has_quantity(*obj));
	return 1;
}

LI_FUNC_DEF(clear_quantity) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::clear_quantity(*obj);
	return 0;
}

LI_FUNC_DEF(make_quantity) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	lua::push_lud(L, &object::make_quantity(*obj));
	return 1;
}

LI_FUNC_DEF(release_quantity) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	object::release_quantity(*obj);
	return 0;
}

// obj, path, single_value = false
LI_FUNC_DEF(read_text_file) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	auto path = lua::get_string(L, 2);
	bool single_value = luaL_opt(L, lua::get_boolean, 3, false);
	lua_pushboolean(L, object::read_text_file(*obj, path, single_value));
	return 1;
}

// obj, text, single_value = false
LI_FUNC_DEF(read_text_string) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	auto text = lua::get_string(L, 2);
	bool single_value = luaL_opt(L, lua::get_boolean, 3, false);
	lua_pushboolean(L, object::read_text_string(*obj, text, single_value));
	return 1;
}

// obj, path, single_value = false
LI_FUNC_DEF(write_text_file) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	auto path = lua::get_string(L, 2);
	bool single_value = luaL_opt(L, lua::get_boolean, 3, false);
	lua_pushboolean(L, object::write_text_file(*obj, path, single_value));
	return 1;
}

// obj, single_value = false, capacity = 512
LI_FUNC_DEF(write_text_string) {
	auto obj = lua::get_lud_t<Object>(L, 1);
	bool single_value = luaL_opt(L, lua::get_boolean, 2, false);
	auto size = luaL_opt(L, luaL_checkinteger, 3, 512);
	MemoryStream stream{memory::scratch_allocator(), static_cast<unsigned>(size)};
	if (object::write_text(*obj, stream, single_value)) {
		lua::push_string(L, {
			reinterpret_cast<char*>(array::begin(stream.data())),
			static_cast<unsigned>(stream.size())
		});
	} else {
		lua_pushnil(L);
	}
	return 1;
}

#define LI_FUNC_REF(name) {#name, object:: LI_FUNC(name)},

static luaL_reg const lua_interface[]{
	LI_FUNC_REF(hash_name)
	LI_FUNC_REF(hash_unit)

	LI_FUNC_REF(create)
	LI_FUNC_REF(create_mv)
	LI_FUNC_REF(destroy)

	LI_FUNC_REF(type)
	LI_FUNC_REF(set_type)
	LI_FUNC_REF(is_type)
	LI_FUNC_REF(is_type_any)
	LI_FUNC_REF(is_null)
	LI_FUNC_REF(is_boolean)
	LI_FUNC_REF(is_integer)
	LI_FUNC_REF(is_decimal)
	LI_FUNC_REF(is_numeric)
	LI_FUNC_REF(is_time)
	LI_FUNC_REF(is_string)
	LI_FUNC_REF(is_expression)

	LI_FUNC_REF(name)
	LI_FUNC_REF(name_hash)
	LI_FUNC_REF(is_named)
	LI_FUNC_REF(set_name)
	LI_FUNC_REF(clear_name)

	LI_FUNC_REF(op)
	LI_FUNC_REF(set_op)

	LI_FUNC_REF(source)
	LI_FUNC_REF(marker_source_uncertain)
	LI_FUNC_REF(sub_source)
	LI_FUNC_REF(marker_sub_source_uncertain)
	LI_FUNC_REF(has_source)
	LI_FUNC_REF(has_sub_source)
	LI_FUNC_REF(source_certain)
	LI_FUNC_REF(source_certain_or_unspecified)
	LI_FUNC_REF(clear_source_uncertainty)
	LI_FUNC_REF(set_source_certain)
	LI_FUNC_REF(set_source)
	LI_FUNC_REF(set_sub_source_certain)
	LI_FUNC_REF(set_sub_source)
	LI_FUNC_REF(clear_source)

	LI_FUNC_REF(marker_value_uncertain)
	LI_FUNC_REF(marker_value_guess)
	LI_FUNC_REF(value_approximation)
	LI_FUNC_REF(value_certain)
	LI_FUNC_REF(set_value_certain)
	LI_FUNC_REF(set_value_guess)
	LI_FUNC_REF(set_value_approximation)
	LI_FUNC_REF(clear_value_markers)
	LI_FUNC_REF(clear_value)
	LI_FUNC_REF(clear)
	LI_FUNC_REF(copy)

	LI_FUNC_REF(set_null)

	LI_FUNC_REF(boolean)
	LI_FUNC_REF(set_boolean)

	LI_FUNC_REF(integer)
	LI_FUNC_REF(set_integer)
	LI_FUNC_REF(decimal)
	LI_FUNC_REF(set_decimal)
	LI_FUNC_REF(numeric)
	LI_FUNC_REF(unit)
	LI_FUNC_REF(set_unit)
	LI_FUNC_REF(unit_hash)
	LI_FUNC_REF(has_unit)

	LI_FUNC_REF(time)
	LI_FUNC_REF(time_type)
	LI_FUNC_REF(has_date)
	LI_FUNC_REF(has_clock)
	LI_FUNC_REF(is_zoned)
	LI_FUNC_REF(is_year_contextual)
	LI_FUNC_REF(is_month_contextual)
	LI_FUNC_REF(set_zoned)
	LI_FUNC_REF(set_year_contextual)
	LI_FUNC_REF(set_month_contextual)
	LI_FUNC_REF(set_time_type)
	LI_FUNC_REF(set_time_value)
	LI_FUNC_REF(set_time)
	LI_FUNC_REF(set_time_date)
	LI_FUNC_REF(set_time_clock)
	LI_FUNC_REF(resolve_time)

	LI_FUNC_REF(string)
	LI_FUNC_REF(set_string)

	LI_FUNC_REF(set_expression)

	LI_FUNC_REF(has_children)
	LI_FUNC_REF(clear_children)
	LI_FUNC_REF(find_child)

	LI_FUNC_REF(has_tags)
	LI_FUNC_REF(clear_tags)
	LI_FUNC_REF(find_tag)

	LI_FUNC_REF(quantity)
	LI_FUNC_REF(has_quantity)
	LI_FUNC_REF(clear_quantity)
	LI_FUNC_REF(make_quantity)
	LI_FUNC_REF(release_quantity)

	LI_FUNC_REF(read_text_file)
	LI_FUNC_REF(read_text_string)
	LI_FUNC_REF(write_text_file)
	LI_FUNC_REF(write_text_string)

	{nullptr, nullptr}
};

#undef LI_FUNC_REF
#undef LI_FUNC_DEF
#undef LI_FUNC

} // namespace object

/// Register the Lua interface.
void object::register_lua_interface(lua_State* L) {
	luaL_register(L, "Object", object::lua_interface);

#define SET_INT(name_, value_) do { \
	lua_pushliteral(L, name_); \
	lua_pushinteger(L, value_); \
	lua_rawset(L, -3); \
	} while (false)

	SET_INT("NAME_NULL", OBJECT_NAME_NULL);

	lua_createtable(L, 0, 7);
	lua_pushliteral(L, "Type");
	lua_pushvalue(L, -2);
	lua_rawset(L, -4);
	SET_INT("null", unsigned_cast(ObjectValueType::null));
	SET_INT("boolean", unsigned_cast(ObjectValueType::boolean));
	SET_INT("integer", unsigned_cast(ObjectValueType::integer));
	SET_INT("decimal", unsigned_cast(ObjectValueType::decimal));
	SET_INT("time", unsigned_cast(ObjectValueType::time));
	SET_INT("string", unsigned_cast(ObjectValueType::string));
	SET_INT("expression", unsigned_cast(ObjectValueType::expression));
	lua_pop(L, 1);

	lua_createtable(L, 0, 5);
	lua_pushliteral(L, "Operator");
	lua_pushvalue(L, -2);
	lua_rawset(L, -4);
	SET_INT("none", unsigned_cast(ObjectOperator::none));
	SET_INT("add", unsigned_cast(ObjectOperator::add));
	SET_INT("sub", unsigned_cast(ObjectOperator::sub));
	SET_INT("mul", unsigned_cast(ObjectOperator::mul));
	SET_INT("div", unsigned_cast(ObjectOperator::div));
	lua_pop(L, 1);

	lua_createtable(L, 0, 3);
	lua_pushliteral(L, "TimeType");
	lua_pushvalue(L, -2);
	lua_rawset(L, -4);
	SET_INT("date_and_clock", unsigned_cast(ObjectTimeType::date_and_clock));
	SET_INT("date", unsigned_cast(ObjectTimeType::date));
	SET_INT("clock", unsigned_cast(ObjectTimeType::clock));
	lua_pop(L, 1);

	lua_pop(L, 1); // module table

#undef SET_INT
}

} // namespace quanta
