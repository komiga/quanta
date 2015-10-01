#line 2 "quanta/core/scripting/scripting.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/scripting/scripting.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>
#include <togo/core/memory/memory.hpp>
#include <togo/core/string/string.hpp>

#include <cstring>

namespace quanta {

static void* alloc_func(void* ud, void* ptr, size_t osize, size_t nsize) {
	auto a = static_cast<Allocator*>(ud);
	if (nsize == 0) {
		a->deallocate(ptr);
		return nullptr;
	}

	void* new_ptr = a->allocate(nsize);
	// TODO: does Lua do resize blocks a lot? maybe this should be a nop
	// (togo allocators don't do reallocs)
	if (osize > 0) {
		std::memcpy(new_ptr, ptr, nsize);
	}
	a->deallocate(ptr);
	return new_ptr;
}

/// Create a new state with the given allocator.
///
/// allocator must exist as long as the state.
lua_State* lua::new_state(Allocator& allocator) {
	return lua_newstate(alloc_func, &allocator);
}

/// Create a new state with Lua's allocator.
lua_State* lua::new_state() {
	return luaL_newstate();
}

} // namespace quanta
