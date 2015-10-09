#line 2 "quanta/app_script_host/interface/command_run.ipp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/scripting/scripting.hpp>

#include <togo/core/filesystem/filesystem.hpp>

namespace quanta {
namespace app_script_host {

static signed pcall_message_handler(lua_State* L) {
	if (lua_isstring(L, 1)) {
		luaL_traceback(L, L, lua_tostring(L, 1), 1);
	}
	return 1;
}

/// Run 'run' command.
///
/// Executes a script.
bool interface::command_run(
	Interface const& /*interface*/,
	StringRef const& script_path
) {
	if (!filesystem::is_file(script_path)) {
		TOGO_LOG("error: path is either not a file or does not exist\n");
		return false;
	}
	lua_State* L = lua::new_state();
	luaL_openlibs(L);
	lua::register_interfaces(L);
	lua_pushcfunction(L, pcall_message_handler);
	if (luaL_loadfile(L, script_path.data)) {
		auto error = lua::get_string(L, -1);
		TOGO_LOGF("failed to load script: %.*s\n", error.size, error.data);
		lua_pop(L, 1);
		return false;
	}
	if (lua_pcall(L, 0, 0, -2)) {
		auto error = lua::get_string(L, -1);
		TOGO_LOGF("script error: %.*s\n", error.size, error.data);
		lua_pop(L, 1);
		return false;
	}
	lua_close(L);
	return true;
}

/// Run 'run' command with KVS.
///
/// Specification:
/// @verbatim run [script_path] @endverbatim
bool interface::command_run(
	Interface const& interface,
	KVS const& /*k_command_options*/,
	KVS const& k_command
) {
	if (kvs::any(k_command) && kvs::is_string(kvs::back(k_command))) {
		return command_run(interface, kvs::string_ref(kvs::back(k_command)));
	} else {
		TOGO_LOG("error: argument must be a string\n");
		return false;
	}
}

} // namespace app_script_host
} // namespace quanta
