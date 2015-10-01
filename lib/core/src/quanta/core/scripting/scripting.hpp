#line 2 "quanta/core/scripting/scripting.hpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.

@file
@brief Scripting interface.
@ingroup lib_core_scripting
*/

#pragma once

#include <quanta/core/config.hpp>
#include <quanta/core/types.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>
#include <togo/core/string/types.hpp>

extern "C" {
	#include <luajit-2.0/lua.h>
	#include <luajit-2.0/lauxlib.h>
	#include <luajit-2.0/lualib.h>
}

namespace quanta {
namespace lua {

/**
	@addtogroup lua
	@{
*/

/// Get an argument from the stack as light userdata.
inline void* arg_lud(lua_State* L, signed narg, bool require = true) {
	luaL_checktype(L, narg, LUA_TLIGHTUSERDATA);
	auto p = lua_touserdata(L, narg);
	TOGO_ASSERTE(!require || p);
	return p;
}

/// Get an argument from the stack (typed).
template<class T>
inline T* arg_lud_t(lua_State* L, signed narg, bool require = true) {
	return static_cast<T*>(arg_lud(L, narg, require));
}

/// Get an argument from the stack as a boolean.
inline bool arg_boolean(lua_State* L, signed narg) {
	luaL_checktype(L, narg, LUA_TBOOLEAN);
	return lua_toboolean(L, narg);
}

/// Get a string from the stack as a string reference.
inline StringRef arg_string(lua_State* L, signed narg, bool require = true) {
	size_t size = 0;
	auto data = luaL_checklstring(L, narg, &size);
	TOGO_ASSERTE(!require || data);
	return {data, static_cast<unsigned>(size)};
}

/// Push a string reference to the stack.
inline void push_string_ref(lua_State* L, StringRef str) {
	lua_pushlstring(L, str.data ? str.data : "", unsigned_cast(str.size));
}

/** @} */ // end of doc-group lua

} // namespace lua
} // namespace quanta
