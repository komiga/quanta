#line 2 "quanta/core/string/unmanaged_string.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>
#include <togo/core/memory/memory.hpp>
#include <togo/core/string/string.hpp>

#include <quanta/core/string/unmanaged_string.hpp>

namespace quanta {

/// Set value.
void unmanaged_string::set(UnmanagedString& s, StringRef value, Allocator& a) {
	unmanaged_string::clear(s, a);
	if (value.empty()) {
		return;
	}
	s.size = value.size;
	s.data = static_cast<char*>(a.allocate(s.size + 1));
	string::copy(s.data, s.size + 1, value);
}

/// Free data.
void unmanaged_string::clear(UnmanagedString& s, Allocator& a) {
	a.deallocate(s.data);
	s.data = nullptr;
	s.size = 0;
}

} // namespace quanta
