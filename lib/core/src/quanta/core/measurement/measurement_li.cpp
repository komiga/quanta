#line 2 "quanta/core/measurement/measurement_li.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/measurement/measurement.hpp>
#include <quanta/core/lua/lua.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>

namespace quanta {

namespace measurement {

static LuaModuleRef const li_module{
	"Quanta.Measurement",
	"quanta/core/measurement/Measurement.lua",
	null_ref_tag{},
	#include <quanta/core/measurement/Measurement.lua>
};

} // namespace measurement

/// Register the Lua interface.
void measurement::register_lua_interface(lua_State* L) {
	lua::preload_module(L, measurement::li_module);
}

} // namespace quanta
