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

namespace togo {
	class Allocator;
} // namespace togo

namespace quanta {
namespace string {

/**
	@addtogroup lib_core_string
	@{
*/

/// String.
struct String {
	char* data;
	u32 size;
};

/// Hashed string.
template<class H>
struct HashedString
	: String
{
	static_assert(
		is_same<H, hash32>::value ||
		is_same<H, hash64>::value,
		"H must be a hash value type"
	);

	H hash;
};

/** @} */ // end of doc-group lib_core_string

} // namespace string

using string::String;
using string::HashedString;

} // namespace quanta
