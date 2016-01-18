#line 2 "quanta/core/composition/composition_li.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/composition/composition.hpp>
#include <quanta/core/lua/lua.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>

namespace quanta {

namespace composition {

static LuaModuleRef const li_module{
	"Quanta.Composition",
	"quanta/core/composition/Composition.lua",
	null_ref_tag{},
	#include <quanta/core/composition/Composition.lua>
};

} // namespace composition

/// Register the Lua interface.
void composition::register_lua_interface(lua_State* L) {
	lua::preload_module(L, composition::li_module);
}

} // namespace quanta
