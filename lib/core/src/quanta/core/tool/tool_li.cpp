#line 2 "quanta/core/tool/tool_li.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/tool/tool.hpp>
#include <quanta/core/lua/lua.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>

namespace quanta {

namespace tool {

static LuaModuleRef const li_module{
	"Quanta.Tool",
	"quanta/core/tool/Tool.lua",
	null_ref_tag{},
	#include <quanta/core/tool/Tool.lua>
};

} // namespace tool

/// Register the Lua interface.
void tool::register_lua_interface(lua_State* L) {
	lua::preload_module(L, tool::li_module);
}

} // namespace quanta
