#line 2 "quanta/core/vessel/vessel_li.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/vessel/vessel.hpp>
#include <quanta/core/lua/lua.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>

namespace quanta {

namespace vessel {

static LuaModuleRef const li_module{
	"Quanta.Vessel",
	"quanta/core/vessel/Vessel.lua",
	null_ref_tag{},
	#include <quanta/core/vessel/Vessel.lua>
};

} // namespace vessel

/// Register the Lua interface.
void vessel::register_lua_interface(lua_State* L) {
	lua::preload_module(L, vessel::li_module);
}

} // namespace quanta
