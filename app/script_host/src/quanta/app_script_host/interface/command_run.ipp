#line 2 "quanta/app_script_host/interface/command_run.ipp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/lua/lua.hpp>

#include <togo/core/filesystem/filesystem.hpp>
#include <togo/core/io/io.hpp>

namespace quanta {
namespace app_script_host {

static void copy_kvs_to_table(
	lua_State* L,
	KVS const& k_list,
	unsigned start_index
) {
	lua_createtable(L, 1, kvs::size(k_list));
	lua::table_set_raw(L, "name", kvs::name_ref(k_list));
	signed ti = 1;
	for (unsigned li = start_index; li < kvs::size(k_list); ++li, ++ti) {
		auto const& k_value = k_list[li];
		lua_createtable(L, 2, 0);
		lua::table_set_raw(L, "name", kvs::name_ref(k_value));
		switch (kvs::type(k_value)) {
		case KVSType::boolean: lua::table_set_raw(L, "value", kvs::boolean(k_value)); break;
		case KVSType::string: lua::table_set_raw(L, "value", kvs::string_ref(k_value)); break;
		default:
			TOGO_ASSERT(false, "missing type handler");
		}
		lua_rawseti(L, -2, ti);
	}
}

/// Run 'run' command.
///
/// Executes a script.
bool interface::command_run(
	Interface const& /*interface*/,
	StringRef const& script_path,
	KVS const& k_command_options,
	KVS const& k_command
) {
	if (!filesystem::is_file(script_path)) {
		TOGO_LOG("error: path is either not a file or does not exist\n");
		return false;
	}

	lua_State* L = lua::new_state();
	luaL_openlibs(L);
	lua::register_core(L);
	io::register_lua_interface(L);
	filesystem::register_lua_interface(L);
	lua::register_quanta_core(L);

	lua::push_value(L, lua::pcall_error_message_handler);
	if (luaL_loadfile(L, script_path.data)) {
		auto error = lua::get_string(L, -1);
		TOGO_LOGF("failed to load script: %.*s\n", error.size, error.data);
		lua_pop(L, 1);
		return false;
	}

	copy_kvs_to_table(L, k_command_options, 0);
	copy_kvs_to_table(L, k_command, 1);
	if (lua_pcall(L, 2, 0, -4)) {
		auto error = lua::get_string(L, -1);
		TOGO_LOGF("script error: %.*s\n", error.size, error.data);
		lua_close(L);
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
	KVS const& k_command_options,
	KVS const& k_command
) {
	if (kvs::any(k_command) && kvs::is_string(k_command[0])) {
		return command_run(
			interface,
			kvs::string_ref(k_command[0]),
			k_command_options,
			k_command
		);
	} else {
		TOGO_LOG("error: argument must be a string\n");
		return false;
	}
}

} // namespace app_script_host
} // namespace quanta
