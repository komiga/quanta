#line 2 "quanta/core/lua/lua.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/chrono/time.hpp>
#include <quanta/core/object/object.hpp>
#include <quanta/core/lua/lua.hpp>

namespace togo {

/// Register core Quanta interfaces.
void lua::register_quanta_core(lua_State* L) {
	quanta::time::register_lua_interface(L);
	quanta::object::register_lua_interface(L);
}

} // namespace togo
