#line 2 "quanta/app_script_host/main.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/app_script_host/config.hpp>
#include <quanta/app_script_host/types.hpp>
#include <togo/core/utility/args.hpp>
#include <togo/core/memory/memory.hpp>
#include <togo/core/kvs/kvs.hpp>
#include <quanta/app_script_host/interface.hpp>

using namespace quanta;
using namespace quanta::app_script_host;

signed main(signed argc, char* argv[]) {
	signed ec = 0;
	memory::init();

	{
		KVS k_options{};
		KVS k_command_options{};
		KVS k_command{};
		parse_args(k_options, k_command_options, k_command, argc, argv);

		Interface interface{};
		if (!interface::read_options(interface, k_options)) {
			ec = -1;
			goto l_exit;
		}
		interface::init(interface);
		if (!interface::run(interface, k_command_options, k_command)) {
			ec = -2;
			goto l_exit;
		}
	}

l_exit:
	memory::shutdown();
	return ec;
}
