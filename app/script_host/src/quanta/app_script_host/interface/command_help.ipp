#line 2 "quanta/app_script_host/interface/command_help.ipp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

namespace quanta {
namespace app_script_host {

/// Run help command.
///
/// Prints command help information.
bool interface::command_help(
	Interface const& /*interface*/,
	StringRef const& command_name
) {
	#define CASE_DESCRIBE_COMMAND(cmd, args, desc)				\
		case cmd ## _hash32:									\
		TOGO_LOG(cmd " " args "\n" desc);						\
		if (!do_all) {											\
			break;												\
		} else {												\
			TOGO_LOG("\n");										\
		}

	bool do_all = false;
	switch (hash::calc32(command_name)) {
		case ""_hash32:
			do_all = true;
			TOGO_LOG(
				QUANTA_APP_SCRIPT_HOST_INFO_TEXT "\n"
				"\n"
				QUANTA_APP_SCRIPT_HOST_USAGE_TEXT "\n"
				"\n"
			);

		CASE_DESCRIBE_COMMAND(
			"help", "[command_name]",
			"  prints script_host help\n"
		);
		CASE_DESCRIBE_COMMAND(
			"run", "<script_path>",
			"  run script\n"
		);

	default:
		if (!do_all) {
			TOGO_LOGF(
				"error: command '%.*s' not recognized\n",
				command_name.size, command_name.data
			);
			return false;
		}
	}
	#undef CASE_DESCRIBE_COMMAND
	return true;
}

/// Run help command with KVS.
///
/// Specification:
/// @verbatim help [command_name] @endverbatim
bool interface::command_help(
	Interface const& interface,
	KVS const& /*k_command_options*/,
	KVS const& k_command
) {
	if (kvs::empty(k_command)) {
		return command_help(interface, "");
	} else if (kvs::is_string(kvs::back(k_command))) {
		return command_help(interface, kvs::string_ref(kvs::back(k_command)));
	} else {
		TOGO_LOG("error: argument must be a string\n");
		return false;
	}
}

} // namespace app_script_host
} // namespace quanta
