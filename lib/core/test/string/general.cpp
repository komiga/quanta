
#include <togo/core/error/assert.hpp>
#include <togo/core/memory/memory.hpp>
#include <togo/core/string/string.hpp>
#include <togo/core/hash/hash.hpp>
#include <togo/support/test.hpp>

#include <quanta/core/string/unmanaged_string.hpp>

using namespace quanta;

signed main() {
	memory_init();
	auto& a = memory::default_allocator();

	{
		UnmanagedString s{};
		unmanaged_string::set(s, "", a);
		TOGO_ASSERTE(s.data == nullptr && s.size == 0);

		unmanaged_string::set(s, "xyz", a);
		TOGO_ASSERTE(s.data != nullptr && s.size == 3 && string::compare_equal(s, "xyz"));

		unmanaged_string::clear(s, a);
	}
	{
		HashedUnmanagedString<hash::Default32> s{};
		unmanaged_string::set(s, "", a);
		TOGO_ASSERTE(s.data == nullptr && s.size == 0);
		TOGO_ASSERTE(s.hash == hash::IDENTITY32);

		unmanaged_string::set(s, "xyz", a);
		TOGO_ASSERTE(s.data != nullptr && s.size == 3 && string::compare_equal(s, "xyz"));
		TOGO_ASSERTE(s.hash == "xyz"_hash32);

		unmanaged_string::clear(s, a);
	}
	return 0;
}
