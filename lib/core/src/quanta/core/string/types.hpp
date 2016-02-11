#line 2 "quanta/core/string/types.hpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.

@file
@brief String types.
@ingroup lib_core_types
@ingroup lib_core_string
*/

#pragma once

#include <quanta/core/config.hpp>
#include <quanta/core/types.hpp>
#include <togo/core/utility/utility.hpp>
#include <togo/core/string/types.hpp>

namespace togo {
	class Allocator;
} // namespace togo

namespace quanta {

/**
	@addtogroup lib_core_string
	@{
*/

namespace unmanaged_string {

/**
	@addtogroup lib_core_unmanaged_string
	@{
*/

/// Unmanaged string.
struct UnmanagedString {
	char* data;
	u32 size;

	operator StringRef() const {
		return StringRef{data, size};
	}
};

/// Hashed unmanaged string.
template<class H>
struct HashedUnmanagedString
	: UnmanagedString
{
	typename H::Value hash;

	operator StringRef() const {
		return StringRef{data, size};
	}
};

/** @} */ // end of doc-group lib_core_unmanaged_string

} // namespace unmanaged_string

using unmanaged_string::UnmanagedString;
using unmanaged_string::HashedUnmanagedString;

/** @} */ // end of doc-group lib_core_string

} // namespace quanta
