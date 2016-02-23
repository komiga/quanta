#line 2 "quanta/core/entity/entity_li.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/entity/entity.hpp>
#include <quanta/core/lua/lua.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>

namespace quanta {

namespace entity {

static LuaModuleRef const li_module{
	"Quanta.Entity",
	"quanta/core/entity/Entity.lua",
	null_ref_tag{},
	#include <quanta/core/entity/Entity.lua>
};

} // namespace entity

/// Register the Lua interface.
void entity::register_lua_interface(lua_State* L) {
	lua::preload_module(L, entity::li_module);
}

} // namespace quanta
