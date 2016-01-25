#line 2 "quanta/core/director/director_li.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/director/director.hpp>
#include <quanta/core/lua/lua.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>

namespace quanta {

namespace director {

static LuaModuleRef const li_module{
	"Quanta.Director",
	"quanta/core/director/Director.lua",
	null_ref_tag{},
	#include <quanta/core/director/Director.lua>
};

} // namespace director

/// Register the Lua interface.
void director::register_lua_interface(lua_State* L) {
	lua::preload_module(L, director::li_module);
}

} // namespace quanta
