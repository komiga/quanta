#line 2 "quanta/app_script_host/interface.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/app_script_host/config.hpp>
#include <quanta/app_script_host/types.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>
#include <togo/core/log/log.hpp>
#include <togo/core/string/string.hpp>
#include <togo/core/kvs/kvs.hpp>

#include <quanta/app_script_host/interface.hpp>

namespace quanta {
namespace app_script_host {

Interface::~Interface() {}

Interface::Interface() {}

/// Read options.
///
/// This should only be called before init().
bool interface::read_options(
	Interface& /*interface*/,
	KVS const& k_options
) {
	TOGO_ASSERTE(kvs::is_node(k_options));
	for (KVS const& k_opt : k_options) {
		switch (kvs::name_hash(k_opt)) {
		/*case "--project-path"_kvs_name:
			if (!kvs::is_string(k_opt) || kvs::string_size(k_opt) == 0) {
				TOGO_LOG("error: --project-path expected a non-empty string\n");
				return false;
			}
			interface::set_project_path(interface, kvs::string_ref(k_opt));
			break;*/

		default:
			TOGO_LOGF(
				"error: option '%.*s' not recognized\n",
				kvs::name_size(k_opt), kvs::name(k_opt)
			);
			return false;
		}
	}
	return true;
}

/// Initialize.
void interface::init(Interface& interface) {
	(void)interface;
}

/// Run with main() arguments.
bool interface::run(
	Interface& interface,
	KVS const& k_command_options,
	KVS const& k_command
) {
	TOGO_ASSERTE(
		kvs::is_node(k_command_options) &&
		kvs::is_node(k_command)
	);

	#define CMD_CASE(cmd)										\
		case #cmd ## _kvs_name:									\
		success = interface::command_ ## cmd(					\
			interface, k_command_options, k_command				\
		);														\
		break

	bool success = false;
	switch (kvs::name_hash(k_command)) {
		CMD_CASE(help);
		CMD_CASE(run);

	default:
		if (kvs::is_named(k_command)) {
			TOGO_LOGF(
				"error: command '%.*s' not recognized\n\n",
				kvs::name_size(k_command), kvs::name(k_command)
			);
		} else {
			TOGO_LOG("error: expected command\n\n");
		}
		TOGO_LOG(
			QUANTA_APP_SCRIPT_HOST_USAGE_TEXT "\n"
			"use \"script_host help [command_name]\" for help\n"
		);
		success = false;
		break;
	}
	#undef CMD_CASE

	return success;
}

} // namespace app_script_host
} // namespace quanta

#include <quanta/app_script_host/interface/command_help.ipp>
#include <quanta/app_script_host/interface/command_run.ipp>
