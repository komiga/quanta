#line 2 "quanta/core/chrono/chrono_time_li.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/chrono/time.hpp>
#include <quanta/core/lua/lua.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>

namespace quanta {

namespace time {

static Time li_temporary_store[64]{};
static unsigned li_temporary_store_index = 0;

static Time& TOGO_LI_FUNC(next_temporary)() {
	auto& t = li_temporary_store[li_temporary_store_index];
	if (++li_temporary_store_index == array_extent(li_temporary_store)) {
		li_temporary_store_index = 0;
	}
	return t;
}

TOGO_LI_FUNC_DEF(temporary) {
	auto& t = li_next_temporary();
	auto from = luaL_opt(L, lua::get_lightuserdata_typed<Time>, 1, nullptr);
	if (from) {
		t = *from;
	}
	lua::push_lightuserdata(L, &t);
	return 1;
}

TOGO_LI_FUNC_DEF(clear) {
	auto t = lua::get_lightuserdata_typed<Time>(L, 1);
	*t = {};
	return 0;
}

TOGO_LI_FUNC_DEF(set_zone_offset) {
	auto t = lua::get_lightuserdata_typed<Time>(L, 1);
	time::set_zone_offset(*t, luaL_checkinteger(L, 2));
	return 0;
}

TOGO_LI_FUNC_DEF(set_zone_clock) {
	auto t = lua::get_lightuserdata_typed<Time>(L, 1);
	time::set_zone_clock(*t, luaL_checkinteger(L, 2), luaL_opt(L, luaL_checkinteger, 3, 0));
	return 0;
}

TOGO_LI_FUNC_DEF(set_zone_utc) {
	auto t = lua::get_lightuserdata_typed<Time>(L, 1);
	time::set_zone_utc(*t);
	return 0;
}

TOGO_LI_FUNC_DEF(adjust_zone_offset) {
	auto t = lua::get_lightuserdata_typed<Time>(L, 1);
	time::adjust_zone_offset(*t, luaL_checkinteger(L, 2));
	return 0;
}

TOGO_LI_FUNC_DEF(adjust_zone_clock) {
	auto t = lua::get_lightuserdata_typed<Time>(L, 1);
	time::adjust_zone_clock(*t, luaL_checkinteger(L, 2), luaL_opt(L, luaL_checkinteger, 3, 0));
	return 0;
}

TOGO_LI_FUNC_DEF(adjust_zone_utc) {
	auto t = lua::get_lightuserdata_typed<Time>(L, 1);
	time::adjust_zone_utc(*t);
	return 0;
}

TOGO_LI_FUNC_DEF(as_utc) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	auto utc = &li_next_temporary();
	*utc = time::as_utc(*t);
	lua::push_lightuserdata(L, utc);
	return 1;
}

TOGO_LI_FUNC_DEF(as_utc_adjusted) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	auto utc = &li_next_temporary();
	*utc = time::as_utc_adjusted(*t);
	lua::push_lightuserdata(L, utc);
	return 1;
}

TOGO_LI_FUNC_DEF(difference) {
	auto l = lua::get_lightuserdata_typed<Time const>(L, 1);
	auto r = lua::get_lightuserdata_typed<Time const>(L, 2);
	lua_pushinteger(L, time::difference(*l, *r));
	return 1;
}

TOGO_LI_FUNC_DEF(add) {
	auto t = lua::get_lightuserdata_typed<Time>(L, 1);
	auto d = luaL_checkinteger(L, 2);
	time::add(*t, d);
	return 0;
}

TOGO_LI_FUNC_DEF(sub) {
	auto t = lua::get_lightuserdata_typed<Time>(L, 1);
	auto d = luaL_checkinteger(L, 2);
	time::sub(*t, d);
	return 0;
}

TOGO_LI_FUNC_DEF(compare_equal) {
	auto l = lua::get_lightuserdata_typed<Time const>(L, 1);
	auto r = lua::get_lightuserdata_typed<Time const>(L, 2);
	lua_pushboolean(L, time::compare_equal(*l, *r));
	return 1;
}

TOGO_LI_FUNC_DEF(hour_utc) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	lua_pushinteger(L, time::hour_utc(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(minute_utc) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	lua_pushinteger(L, time::minute_utc(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(second_utc) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	lua_pushinteger(L, time::second_utc(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(hour) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	lua_pushinteger(L, time::hour(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(minute) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	lua_pushinteger(L, time::minute(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(second) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	lua_pushinteger(L, time::second(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(clock_utc) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	signed h = 0, m = 0, s = 0;
	time::clock_utc(*t, h, m, s);
	lua_pushinteger(L, h);
	lua_pushinteger(L, m);
	lua_pushinteger(L, s);
	return 3;
}

TOGO_LI_FUNC_DEF(clock) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	signed h = 0, m = 0, s = 0;
	time::clock(*t, h, m, s);
	lua_pushinteger(L, h);
	lua_pushinteger(L, m);
	lua_pushinteger(L, s);
	return 3;
}

TOGO_LI_FUNC_DEF(clock_seconds_utc) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	lua_pushinteger(L, time::clock_seconds_utc(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(clock_seconds) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	lua_pushinteger(L, time::clock_seconds(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(date_seconds_utc) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	lua_pushinteger(L, time::date_seconds_utc(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(date_seconds) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	lua_pushinteger(L, time::date_seconds(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(set_utc) {
	auto t = lua::get_lightuserdata_typed<Time>(L, 1);
	auto h = luaL_checkinteger(L, 2);
	auto m = luaL_checkinteger(L, 3);
	auto s = luaL_checkinteger(L, 4);
	time::set_utc(*t, h, m, s);
	return 0;
}

TOGO_LI_FUNC_DEF(set) {
	auto t = lua::get_lightuserdata_typed<Time>(L, 1);
	auto h = luaL_checkinteger(L, 2);
	auto m = luaL_checkinteger(L, 3);
	auto s = luaL_checkinteger(L, 4);
	time::set(*t, h, m, s);
	return 0;
}

static luaL_reg const li_funcs[]{
	TOGO_LI_FUNC_REF(time, temporary)
	TOGO_LI_FUNC_REF(time, clear)

	TOGO_LI_FUNC_REF(time, set_zone_offset)
	TOGO_LI_FUNC_REF(time, set_zone_clock)
	TOGO_LI_FUNC_REF(time, set_zone_utc)
	TOGO_LI_FUNC_REF(time, adjust_zone_offset)
	TOGO_LI_FUNC_REF(time, adjust_zone_clock)
	TOGO_LI_FUNC_REF(time, adjust_zone_utc)

	TOGO_LI_FUNC_REF(time, as_utc)
	TOGO_LI_FUNC_REF(time, as_utc_adjusted)

	TOGO_LI_FUNC_REF(time, difference)
	TOGO_LI_FUNC_REF(time, add)
	TOGO_LI_FUNC_REF(time, sub)
	TOGO_LI_FUNC_REF(time, compare_equal)

	TOGO_LI_FUNC_REF(time, hour_utc)
	TOGO_LI_FUNC_REF(time, minute_utc)
	TOGO_LI_FUNC_REF(time, second_utc)
	TOGO_LI_FUNC_REF(time, hour)
	TOGO_LI_FUNC_REF(time, minute)
	TOGO_LI_FUNC_REF(time, second)

	TOGO_LI_FUNC_REF(time, clock_utc)
	TOGO_LI_FUNC_REF(time, clock)

	TOGO_LI_FUNC_REF(time, clock_seconds_utc)
	TOGO_LI_FUNC_REF(time, clock_seconds)

	TOGO_LI_FUNC_REF(time, date_seconds_utc)
	TOGO_LI_FUNC_REF(time, date_seconds)

	TOGO_LI_FUNC_REF(time, set_utc)
	TOGO_LI_FUNC_REF(time, set)

	{nullptr, nullptr}
};

namespace gregorian {

TOGO_LI_FUNC_DEF(date_utc) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	Date d = time::gregorian::date_utc(*t);
	lua_pushinteger(L, d.year);
	lua_pushinteger(L, d.month);
	lua_pushinteger(L, d.day);
	return 3;
}

TOGO_LI_FUNC_DEF(year_utc) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	lua_pushinteger(L, time::gregorian::year_utc(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(month_utc) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	lua_pushinteger(L, time::gregorian::month_utc(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(day_utc) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	lua_pushinteger(L, time::gregorian::day_utc(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(is_leap_year_utc) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	lua_pushboolean(L, time::gregorian::is_leap_year_utc(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(date) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	Date d = time::gregorian::date(*t);
	lua_pushinteger(L, d.year);
	lua_pushinteger(L, d.month);
	lua_pushinteger(L, d.day);
	return 3;
}

TOGO_LI_FUNC_DEF(year) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	lua_pushinteger(L, time::gregorian::year(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(month) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	lua_pushinteger(L, time::gregorian::month(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(day) {
	auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
	lua_pushinteger(L, time::gregorian::day(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(is_leap_year) {
	if (lua_type(L, 1) == LUA_TLIGHTUSERDATA) {
		auto t = lua::get_lightuserdata_typed<Time const>(L, 1);
		lua_pushboolean(L, time::gregorian::is_leap_year(*t));
	} else {
		auto year = luaL_checkinteger(L, 1);
		lua_pushboolean(L, time::gregorian::is_leap_year(year));
	}
	return 1;
}

TOGO_LI_FUNC_DEF(set_utc) {
	auto t = lua::get_lightuserdata_typed<Time>(L, 1);
	auto year = luaL_checkinteger(L, 2);
	auto month = luaL_checkinteger(L, 3);
	auto day = luaL_checkinteger(L, 4);
	if (lua_gettop(L) == 4) {
		time::gregorian::set_utc(*t, year, month, day);
	} else {
		auto clock_h = luaL_checkinteger(L, 5);
		auto clock_m = luaL_checkinteger(L, 6);
		auto clock_s = luaL_checkinteger(L, 7);
		time::gregorian::set_utc(*t, year, month, day, clock_h, clock_m, clock_s);
	}
	return 0;
}

TOGO_LI_FUNC_DEF(set) {
	auto t = lua::get_lightuserdata_typed<Time>(L, 1);
	auto year = luaL_checkinteger(L, 2);
	auto month = luaL_checkinteger(L, 3);
	auto day = luaL_checkinteger(L, 4);
	if (lua_gettop(L) == 4) {
		time::gregorian::set(*t, year, month, day);
	} else {
		auto clock_h = luaL_checkinteger(L, 5);
		auto clock_m = luaL_checkinteger(L, 6);
		auto clock_s = luaL_checkinteger(L, 7);
		time::gregorian::set(*t, year, month, day, clock_h, clock_m, clock_s);
	}
	return 0;
}

static luaL_reg const li_funcs[]{
	TOGO_LI_FUNC_REF(time::gregorian, date_utc)
	TOGO_LI_FUNC_REF(time::gregorian, year_utc)
	TOGO_LI_FUNC_REF(time::gregorian, month_utc)
	TOGO_LI_FUNC_REF(time::gregorian, day_utc)
	TOGO_LI_FUNC_REF(time::gregorian, is_leap_year_utc)

	TOGO_LI_FUNC_REF(time::gregorian, date)
	TOGO_LI_FUNC_REF(time::gregorian, year)
	TOGO_LI_FUNC_REF(time::gregorian, month)
	TOGO_LI_FUNC_REF(time::gregorian, day)
	TOGO_LI_FUNC_REF(time::gregorian, is_leap_year)

	TOGO_LI_FUNC_REF(time::gregorian, set_utc)
	TOGO_LI_FUNC_REF(time::gregorian, set)

	{nullptr, nullptr}
};

} // namespace gregorian

} // namespace time

/// Register the Lua interface.
void time::register_lua_interface(lua_State* L) {
	luaL_register(L, "Quanta.Time", time::li_funcs);

	luaL_register(L, "Quanta.Time.Gregorian", time::gregorian::li_funcs);
	lua::table_set_copy_raw(L, -4, "G", -1); // Quanta.Time.G = Quanta.Time.Gregorian
	lua_pop(L, 1);

	lua_pop(L, 1); // Quanta.Time
}

} // namespace quanta
