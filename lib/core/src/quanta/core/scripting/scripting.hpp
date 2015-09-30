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

#include <luajit-2.0/lua.hpp>

namespace quanta {

// Forward declarations
namespace object {
	struct Object;
}

namespace lua {

/**
	@addtogroup lua
	@{
*/

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

/// Get an object from the stack as an Object (LUD).
inline object::Object* arg_object(lua_State* L, signed narg, bool require = true) {
	luaL_checktype(L, narg, LUA_TLIGHTUSERDATA);
	auto obj = static_cast<object::Object*>(lua_touserdata(L, narg));
	TOGO_ASSERTE(!require || obj);
	return obj;
}

/** @} */ // end of doc-group lua

} // namespace lua
} // namespace quanta
