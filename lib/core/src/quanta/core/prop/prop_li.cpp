#line 2 "quanta/core/prop/prop_li.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/prop/prop.hpp>
#include <quanta/core/lua/lua.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>

namespace quanta {

namespace prop {

static LuaModuleRef const li_module{
	"Quanta.Prop",
	"quanta/core/prop/Prop.lua",
	null_ref_tag{},
	#include <quanta/core/prop/Prop.lua>
};

} // namespace prop

/// Register the Lua interface.
void prop::register_lua_interface(lua_State* L) {
	lua::preload_module(L, prop::li_module);
}

} // namespace quanta
