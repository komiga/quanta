
#include <quanta/core/object/object.hpp>

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
	lua_close(L);
	return 0;
}
