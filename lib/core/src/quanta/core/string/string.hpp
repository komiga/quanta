#line 2 "quanta/core/string/string.hpp"
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
#include <togo/core/string/string.hpp>
#include <togo/core/hash/hash.hpp>

#include <quanta/core/string/types.hpp>
#include <quanta/core/string/string.gen_interface>

namespace quanta {
namespace string {

/**
	@addtogroup lib_core_string
	@{
*/

/// String size (not including NUL terminator).
inline u32 size(String const& s) { return s.size; }

/// Whether the string is empty.
inline bool empty(String const& s) { return s.size == 0; }

/// Whether the string is non-empty.
inline bool any(String const& s) { return s.size > 0; }

template<class H>
void set(HashedString<H>& s, StringRef value, Allocator& a) {
	set(static_cast<String&>(s), value, a);
	s.hash = hash::calc_generic<H>(s.data, s.size);
}

/// Free string.
template<class H>
void clear(HashedString<H>& s, Allocator& a) {
	clear(static_cast<String&>(s), a);
	s.hash = hash::traits<H>::identity;
}

/** @} */ // end of doc-group lib_core_string

} // namespace string
} // namespace quanta
