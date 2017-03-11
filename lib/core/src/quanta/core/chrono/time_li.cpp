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

TOGO_LI_FUNC_DEF(__mm_ctor) {
	auto t = lua::new_userdata<Time>(L);
	if (lua_isnumber(L, 1)) {
		t->sec = lua::get_integer(L, 1);
	} else if (!lua_isnoneornil(L, 1)) {
		*t = *lua::get_pointer<Time>(L, 1);
	}
	if (lua_isnumber(L, 2)) {
		t->zone_offset = lua::get_integer(L, 2);
	}
	return 1;
}

TOGO_LI_FUNC_DEF(from_posix) {
	auto t = lua::new_userdata<Time>(L);
	time::set_posix(*t, lua::get_integer(L, 1));
	if (lua_isnumber(L, 2)) {
		t->zone_offset = lua::get_integer(L, 2);
	}
	return 1;
}

TOGO_LI_FUNC_DEF(set_zone_offset) {
	auto t = lua::get_pointer<Time>(L, 1);
	time::set_zone_offset(*t, lua::get_integer(L, 2));
	return 0;
}

TOGO_LI_FUNC_DEF(set_zone_clock) {
	auto t = lua::get_pointer<Time>(L, 1);
	time::set_zone_clock(*t, lua::get_integer(L, 2), luaL_opt(L, lua::get_integer, 3, 0));
	return 0;
}

TOGO_LI_FUNC_DEF(set_zone_utc) {
	auto t = lua::get_pointer<Time>(L, 1);
	time::set_zone_utc(*t);
	return 0;
}

TOGO_LI_FUNC_DEF(adjust_zone_offset) {
	auto t = lua::get_pointer<Time>(L, 1);
	time::adjust_zone_offset(*t, lua::get_integer(L, 2));
	return 0;
}

TOGO_LI_FUNC_DEF(adjust_zone_clock) {
	auto t = lua::get_pointer<Time>(L, 1);
	time::adjust_zone_clock(*t, lua::get_integer(L, 2), luaL_opt(L, lua::get_integer, 3, 0));
	return 0;
}

TOGO_LI_FUNC_DEF(adjust_zone_utc) {
	auto t = lua::get_pointer<Time>(L, 1);
	time::adjust_zone_utc(*t);
	return 0;
}

TOGO_LI_FUNC_DEF(as_utc) {
	auto t = lua::get_pointer<Time const>(L, 1);
	auto utc = lua::new_userdata<Time>(L);
	*utc = time::as_utc(*t);
	return 1;
}

TOGO_LI_FUNC_DEF(as_utc_adjusted) {
	auto t = lua::get_pointer<Time const>(L, 1);
	auto utc = lua::new_userdata<Time>(L);
	*utc = time::as_utc_adjusted(*t);
	return 1;
}

TOGO_LI_FUNC_DEF(difference) {
	auto l = lua::get_pointer<Time const>(L, 1);
	auto r = lua::get_pointer<Time const>(L, 2);
	lua::push_value(L, time::difference(*l, *r));
	return 1;
}

TOGO_LI_FUNC_DEF(add) {
	auto t = lua::get_pointer<Time>(L, 1);
	auto d = lua::get_integer(L, 2);
	time::add(*t, d);
	return 0;
}

TOGO_LI_FUNC_DEF(sub) {
	auto t = lua::get_pointer<Time>(L, 1);
	auto d = lua::get_integer(L, 2);
	time::sub(*t, d);
	return 0;
}

TOGO_LI_FUNC_DEF(compare_equal) {
	auto l = lua::get_pointer<Time const>(L, 1);
	auto r = lua::get_pointer<Time const>(L, 2);
	lua::push_value(L, time::compare_equal(*l, *r));
	return 1;
}

TOGO_LI_FUNC_DEF(posix) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, time::posix(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(set_posix) {
	auto t = lua::get_pointer<Time>(L, 1);
	auto posix_sec = lua::get_integer(L, 2);
	time::set_posix(*t, posix_sec);
	return 0;
}

TOGO_LI_FUNC_DEF(hour_utc) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, time::hour_utc(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(minute_utc) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, time::minute_utc(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(second_utc) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, time::second_utc(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(hour) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, time::hour(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(minute) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, time::minute(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(second) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, time::second(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(clock_utc) {
	auto t = lua::get_pointer<Time const>(L, 1);
	signed h = 0, m = 0, s = 0;
	time::clock_utc(*t, h, m, s);
	lua::push_value(L, h);
	lua::push_value(L, m);
	lua::push_value(L, s);
	return 3;
}

TOGO_LI_FUNC_DEF(clock) {
	auto t = lua::get_pointer<Time const>(L, 1);
	signed h = 0, m = 0, s = 0;
	time::clock(*t, h, m, s);
	lua::push_value(L, h);
	lua::push_value(L, m);
	lua::push_value(L, s);
	return 3;
}

TOGO_LI_FUNC_DEF(clock_seconds_utc) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, time::clock_seconds_utc(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(clock_seconds) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, time::clock_seconds(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(date_seconds_utc) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, time::date_seconds_utc(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(date_seconds) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, time::date_seconds(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(set_utc) {
	auto t = lua::get_pointer<Time>(L, 1);
	auto h = lua::get_integer(L, 2);
	auto m = lua::get_integer(L, 3);
	auto s = lua::get_integer(L, 4);
	time::set_utc(*t, h, m, s);
	return 0;
}

TOGO_LI_FUNC_DEF(set) {
	auto t = lua::get_pointer<Time>(L, 1);
	if (lua_gettop(L) == 2) {
		if (lua_type(L, 2) == LUA_TNUMBER) {
			t->sec = lua::get_integer(L, 2);
		} else {
			*t = *lua::get_pointer<Time>(L, 2);
		}
	} else {
		auto h = lua::get_integer(L, 2);
		auto m = lua::get_integer(L, 3);
		auto s = lua::get_integer(L, 4);
		time::set(*t, h, m, s);
	}
	return 0;
}

TOGO_LI_FUNC_DEF(set_now) {
	auto t = lua::get_pointer<Time>(L, 1);
	time::set_now(*t);
	return 0;
}

TOGO_LI_FUNC_DEF(set_clock_now) {
	auto t = lua::get_pointer<Time>(L, 1);
	time::set_clock_now(*t);
	return 0;
}

TOGO_LI_FUNC_DEF(set_date_now) {
	auto t = lua::get_pointer<Time>(L, 1);
	time::set_date_now(*t);
	return 0;
}

TOGO_LI_FUNC_DEF(clear) {
	auto t = lua::get_pointer<Time>(L, 1);
	*t = {};
	return 0;
}

TOGO_LI_FUNC_DEF(clear_clock) {
	auto t = lua::get_pointer<Time>(L, 1);
	time::clear_clock(*t);
	return 0;
}

TOGO_LI_FUNC_DEF(clear_date) {
	auto t = lua::get_pointer<Time>(L, 1);
	time::clear_date(*t);
	return 0;
}

TOGO_LI_FUNC_DEF(value) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, t->sec);
	return 1;
}

TOGO_LI_FUNC_DEF(zone_offset) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, t->zone_offset);
	return 1;
}

static LuaModuleFunctionArray const li_funcs{
	TOGO_LI_FUNC_REF(time, __mm_ctor)
	TOGO_LI_FUNC_REF(time, from_posix)

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

	TOGO_LI_FUNC_REF(time, posix)
	TOGO_LI_FUNC_REF(time, set_posix)

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
	TOGO_LI_FUNC_REF(time, set_now)
	TOGO_LI_FUNC_REF(time, set_clock_now)
	TOGO_LI_FUNC_REF(time, set_date_now)

	TOGO_LI_FUNC_REF(time, clear)
	TOGO_LI_FUNC_REF(time, clear_clock)
	TOGO_LI_FUNC_REF(time, clear_date)

	TOGO_LI_FUNC_REF(time, value)
	TOGO_LI_FUNC_REF(time, zone_offset)
};

static LuaModuleRef const li_module{
	"Quanta.Time",
	"quanta/core/chrono/Time.lua",
	li_funcs,
	#include <quanta/core/chrono/Time.lua>
};

namespace gregorian {

TOGO_LI_FUNC_DEF(date_utc) {
	auto t = lua::get_pointer<Time const>(L, 1);
	Date d = time::gregorian::date_utc(*t);
	lua::push_value(L, d.year);
	lua::push_value(L, d.month);
	lua::push_value(L, d.day);
	return 3;
}

TOGO_LI_FUNC_DEF(year_utc) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, time::gregorian::year_utc(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(month_utc) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, time::gregorian::month_utc(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(day_utc) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, time::gregorian::day_utc(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(is_leap_year_utc) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, time::gregorian::is_leap_year_utc(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(date) {
	auto t = lua::get_pointer<Time const>(L, 1);
	Date d = time::gregorian::date(*t);
	lua::push_value(L, d.year);
	lua::push_value(L, d.month);
	lua::push_value(L, d.day);
	return 3;
}

TOGO_LI_FUNC_DEF(year) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, time::gregorian::year(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(month) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, time::gregorian::month(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(day) {
	auto t = lua::get_pointer<Time const>(L, 1);
	lua::push_value(L, time::gregorian::day(*t));
	return 1;
}

TOGO_LI_FUNC_DEF(is_leap_year) {
	if (lua_type(L, 1) == LUA_TLIGHTUSERDATA) {
		auto t = lua::get_pointer<Time const>(L, 1);
		lua::push_value(L, time::gregorian::is_leap_year(*t));
	} else {
		auto year = lua::get_integer(L, 1);
		lua::push_value(L, time::gregorian::is_leap_year(year));
	}
	return 1;
}

TOGO_LI_FUNC_DEF(set_utc) {
	auto t = lua::get_pointer<Time>(L, 1);
	auto year = lua::get_integer(L, 2);
	auto month = lua::get_integer(L, 3);
	auto day = lua::get_integer(L, 4);
	if (lua_gettop(L) == 4) {
		time::gregorian::set_utc(*t, year, month, day);
	} else {
		auto clock_h = lua::get_integer(L, 5);
		auto clock_m = lua::get_integer(L, 6);
		auto clock_s = lua::get_integer(L, 7);
		time::gregorian::set_utc(*t, year, month, day, clock_h, clock_m, clock_s);
	}
	return 0;
}

TOGO_LI_FUNC_DEF(set) {
	auto t = lua::get_pointer<Time>(L, 1);
	auto year = lua::get_integer(L, 2);
	auto month = lua::get_integer(L, 3);
	auto day = lua::get_integer(L, 4);
	if (lua_gettop(L) == 4) {
		time::gregorian::set(*t, year, month, day);
	} else {
		auto clock_h = lua::get_integer(L, 5);
		auto clock_m = lua::get_integer(L, 6);
		auto clock_s = lua::get_integer(L, 7);
		time::gregorian::set(*t, year, month, day, clock_h, clock_m, clock_s);
	}
	return 0;
}

static LuaModuleFunctionArray const li_funcs{
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
};

static LuaModuleRef const li_module{
	"Quanta.Time.Gregorian",
	"quanta/core/chrono/Time.Gregorian.lua",
	li_funcs,
	#include <quanta/core/chrono/Time.Gregorian.lua>
};

} // namespace gregorian

} // namespace time

/// Register the Lua interface.
void time::register_lua_interface(lua_State* L) {
	lua::register_userdata<time::Time>(L, nullptr);
	lua::preload_module(L, time::li_module);
	lua::preload_module(L, time::gregorian::li_module);
}

} // namespace quanta
