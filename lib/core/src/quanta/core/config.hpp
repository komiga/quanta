#line 2 "quanta/core/config.hpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.

@file
@brief Core configuration.
@ingroup lib_core_config

@defgroup lib_core_config Configuration
@ingroup lib_core
@details
*/

#pragma once

#include <togo/core/config.hpp>

namespace quanta {

/**
	@addtogroup lib_core_config
	@{
*/

#if !defined(QUANTA_DEBUG) && (defined(DEBUG) || !defined(NDEBUG))
	#define QUANTA_DEBUG
#endif

using namespace togo;

/** @} */ // end of doc-group lib_core_config

} // namespace quanta
