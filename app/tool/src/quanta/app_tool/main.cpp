#line 2 "quanta/app_tool/main.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/app_tool/config.hpp>
#include <quanta/app_tool/types.hpp>

#include <quanta/core/lua/lua.hpp>

#include <togo/core/utility/utility.hpp>
#include <togo/core/log/log.hpp>
#include <togo/core/memory/memory.hpp>
#include <togo/core/system/system.hpp>
#include <togo/core/filesystem/filesystem.hpp>
#include <togo/core/io/io.hpp>

using namespace togo;
using namespace quanta;

signed main(signed argc, char* argv[]) {
	signed ec = 0;
	memory::init();

	lua_State* L = lua::new_state();
	luaL_openlibs(L);

	lua::register_core(L);
	io::register_lua_interface(L);
	filesystem::register_lua_interface(L);
	lua::register_quanta_core(L);

	lua::push_value(L, lua::pcall_error_message_handler);
	lua::load_module(L, "Quanta.Tool", true);
	lua::table_get_raw(L, "main");
	lua_remove(L, -2);

	lua_createtable(L, 0, argc);
	for (signed i = 0; i < argc; ++i) {
		lua::table_set_index_raw(L, i + 1, StringRef{argv[i], cstr_tag{}});
	}
	if (lua_pcall(L, 1, 1, -3)) {
		auto error = lua::get_string(L, -1);
		TOGO_LOGF("error: %.*s\n", error.size, error.data);
		ec = 1;
	} else {
		if (lua_isboolean(L, -1) && !lua::get_boolean(L, -1)) {
			ec = 2;
		}
	}
	lua_pop(L, 2);
	lua_close(L);

	memory::shutdown();
	return ec;
}
