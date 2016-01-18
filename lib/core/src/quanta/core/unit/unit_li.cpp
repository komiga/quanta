#line 2 "quanta/core/unit/unit_li.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/unit/unit.hpp>
#include <quanta/core/lua/lua.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>

namespace quanta {

namespace unit {

static LuaModuleRef const li_module{
	"Quanta.Unit",
	"quanta/core/unit/Unit.lua",
	null_ref_tag{},
	#include <quanta/core/unit/Unit.lua>
};

} // namespace unit

/// Register the Lua interface.
void unit::register_lua_interface(lua_State* L) {
	lua::preload_module(L, unit::li_module);
}

} // namespace quanta
