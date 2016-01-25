#line 2 "quanta/core/tracker/tracker_li.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/tracker/tracker.hpp>
#include <quanta/core/lua/lua.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>

namespace quanta {

namespace tracker {

static LuaModuleRef const li_module{
	"Quanta.Tracker",
	"quanta/core/tracker/Tracker.lua",
	null_ref_tag{},
	#include <quanta/core/tracker/Tracker.lua>
};

} // namespace tracker

/// Register the Lua interface.
void tracker::register_lua_interface(lua_State* L) {
	lua::preload_module(L, tracker::li_module);
}

} // namespace quanta
