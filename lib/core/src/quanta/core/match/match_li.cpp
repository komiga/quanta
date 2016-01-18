#line 2 "quanta/core/match/match_li.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/match/match.hpp>
#include <quanta/core/lua/lua.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>

namespace quanta {

namespace match {

static LuaModuleRef const li_module{
	"Quanta.Match",
	"quanta/core/match/Match.lua",
	null_ref_tag{},
	#include <quanta/core/match/Match.lua>
};

} // namespace match

/// Register the Lua interface.
void match::register_lua_interface(lua_State* L) {
	lua::preload_module(L, match::li_module);
}

} // namespace quanta
