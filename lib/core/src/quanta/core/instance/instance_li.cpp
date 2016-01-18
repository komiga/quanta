#line 2 "quanta/core/instance/instance_li.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/instance/instance.hpp>
#include <quanta/core/lua/lua.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>

namespace quanta {

namespace instance {

static LuaModuleRef const li_module{
	"Quanta.Instance",
	"quanta/core/instance/Instance.lua",
	null_ref_tag{},
	#include <quanta/core/instance/Instance.lua>
};

} // namespace instance

/// Register the Lua interface.
void instance::register_lua_interface(lua_State* L) {
	lua::preload_module(L, instance::li_module);
}

} // namespace quanta
