#line 2 "quanta/core/lua/lua.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/chrono/time.hpp>
#include <quanta/core/object/object.hpp>

#include <quanta/core/vessel/vessel.hpp>
#include <quanta/core/match/match.hpp>
#include <quanta/core/measurement/measurement.hpp>
#include <quanta/core/prop/prop.hpp>
#include <quanta/core/entity/entity.hpp>
#include <quanta/core/unit/unit.hpp>

#include <quanta/core/tracker/tracker.hpp>
#include <quanta/core/director/director.hpp>
#include <quanta/core/tool/tool.hpp>

#include <quanta/core/lua/lua.hpp>

namespace togo {

/// Register core Quanta interfaces.
void lua::register_quanta_core(lua_State* L) {
	quanta::time::register_lua_interface(L);
	quanta::object::register_lua_interface(L);

	quanta::vessel::register_lua_interface(L);
	quanta::match::register_lua_interface(L);
	quanta::measurement::register_lua_interface(L);
	quanta::prop::register_lua_interface(L);
	quanta::entity::register_lua_interface(L);
	quanta::unit::register_lua_interface(L);

	quanta::tracker::register_lua_interface(L);
	quanta::director::register_lua_interface(L);
	quanta::tool::register_lua_interface(L);
}

} // namespace togo
