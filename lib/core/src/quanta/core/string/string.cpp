#line 2 "quanta/core/string/string.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/types.hpp>
#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>
#include <togo/core/memory/memory.hpp>
#include <togo/core/string/string.hpp>

#include <quanta/core/string/string.hpp>

#include <cstring>

namespace quanta {

/// Set value.
void string::set(String& s, StringRef value, Allocator& a) {
	if (value.size > s.size) {
		a.deallocate(s.data);
		s.data = static_cast<char*>(a.allocate(value.size));
	}
	s.size = value.size;
	std::memcpy(s.data, value.data, s.size);
}

/// Clear string.
void string::clear(String& s, Allocator& a) {
	a.deallocate(s.data);
	s.data = nullptr;
	s.size = 0;
}

} // namespace quanta
