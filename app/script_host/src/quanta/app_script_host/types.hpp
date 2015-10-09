#line 2 "quanta/app_script_host/types.hpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.

@file
@brief app_script_host types.
@ingroup app_script_host_types

@defgroup app_script_host_types Types
@ingroup app_script_host
@details
*/

#pragma once

#include <quanta/app_script_host/config.hpp>
#include <quanta/core/types.hpp>
#include <togo/core/memory/types.hpp>
#include <togo/core/collection/types.hpp>
#include <togo/core/string/types.hpp>
#include <togo/core/hash/hash.hpp>
#include <togo/core/io/types.hpp>
#include <togo/core/serialization/types.hpp>

namespace quanta {
namespace app_script_host {

/**
	@addtogroup app_script_host_interface
	@{
*/

/// App interface.
struct Interface {
	Interface(Interface const&) = delete;
	Interface(Interface&&) = delete;
	Interface& operator=(Interface const&) = delete;
	Interface& operator=(Interface&&) = delete;

	~Interface();
	Interface();
};

/** @} */ // end of doc-group app_script_host_interface

} // namespace app_script_host
} // namespace quanta
