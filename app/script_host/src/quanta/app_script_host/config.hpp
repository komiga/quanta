#line 2 "quanta/app_script_host/config.hpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.

@file
@brief Core configuration.
@ingroup app_script_host_config

@defgroup app_script_host_config Configuration
@ingroup app_script_host
@details
*/

#pragma once

#include <quanta/core/config.hpp>

namespace quanta {
namespace app_script_host {

/**
	@addtogroup app_script_host_config
	@{
*/

/// app_script_host information text.
#define QUANTA_APP_SCRIPT_HOST_INFO_TEXT \
	"Quanta script_host 0.00"

/// app_script_host usage text.
#define QUANTA_APP_SCRIPT_HOST_USAGE_TEXT \
	"usage: script_host [options] <command> [command_arguments]"

/** @} */ // end of doc-group app_script_host_config

} // namespace app_script_host
} // namespace quanta
