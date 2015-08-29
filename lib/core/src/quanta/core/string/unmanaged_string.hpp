#line 2 "quanta/core/string/unmanaged_string.hpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.

@file
@brief String interface.
@ingroup lib_core_string
*/

#pragma once

#include <quanta/core/config.hpp>
#include <quanta/core/types.hpp>
#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>
#include <togo/core/memory/types.hpp>
#include <togo/core/hash/hash.hpp>

#include <quanta/core/string/types.hpp>
#include <quanta/core/string/unmanaged_string.gen_interface>

namespace quanta {
namespace unmanaged_string {

/**
	@addtogroup lib_core_unmanaged_string
	@{
*/

/// String size (not including NUL terminator).
inline u32 size(UnmanagedString const& s) { return s.size; }

/// Whether the string is empty.
inline bool empty(UnmanagedString const& s) { return s.size == 0; }

/// Whether the string is non-empty.
inline bool any(UnmanagedString const& s) { return s.size > 0; }

template<class H>
void set(HashedUnmanagedString<H>& s, StringRef value, Allocator& a) {
	unmanaged_string::set(static_cast<UnmanagedString&>(s), value, a);
	s.hash = hash::calc_generic<H>(s.data, s.size);
}

template<class H>
void clear(HashedUnmanagedString<H>& s, Allocator& a) {
	unmanaged_string::clear(static_cast<UnmanagedString&>(s), a);
	s.hash = hash::traits<H>::identity;
}

/** @} */ // end of doc-group lib_core_unmanaged_string

} // namespace unmanaged_string
} // namespace quanta
