
#include <quanta/core/object/object.hpp>

#include <togo/core/log/log.hpp>
#include <togo/core/error/assert.hpp>
#include <togo/core/string/string.hpp>
#include <togo/core/memory/memory.hpp>
#include <togo/support/test.hpp>

using namespace quanta;

signed main() {
	memory_init();

	lua_State* L = lua::new_state();
	luaL_openlibs(L);
	object::register_lua_interface(L);
	if (luaL_dofile(L, "object/lua.lua")) {
		auto error = lua::get_string(L, -1);
		TOGO_LOG_ERRORF("script error: %.*s\n", error.size, error.data);
		lua_pop(L, 1);
	}
	lua_close(L);
	return 0;
}
