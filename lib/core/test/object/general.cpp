
#include <togo/core/error/assert.hpp>
#include <togo/core/string/string.hpp>
#include <togo/support/test.hpp>

#include <quanta/core/object/object.hpp>

using namespace quanta;

signed main() {
	memory_init();

	{
		Object a;
		TOGO_ASSERTE(!is_named(a));
		TOGO_ASSERTE(string::compare_equal(name(a), ""));
		TOGO_ASSERTE(name_hash(a) == OBJECT_NAME_NULL);
		TOGO_ASSERTE(source(a) == 0);
		TOGO_ASSERTE(sub_source(a) == 0);
		TOGO_ASSERTE(!has_source(a));
		TOGO_ASSERTE(is_null(a));
		TOGO_ASSERTE(!has_tags(a));
		TOGO_ASSERTE(!has_children(a));
		TOGO_ASSERTE(!has_quantity(a));
		TOGO_ASSERTE(!find_child(a, ""));
		TOGO_ASSERTE(!find_child(a, "a"));
		TOGO_ASSERTE(!find_tag(a, ""));
		TOGO_ASSERTE(!find_tag(a, "a"));
	}

	{
		Object a;
		set_string(a, "brains");
		TOGO_ASSERTE(string::compare_equal(object::string(a), "brains"));
		set_source(a, 1);
		set_sub_source(a, 2);
		TOGO_ASSERTE(has_source(a) && source(a) == 1 && sub_source(a) == 2);

		auto& q = make_quantity(a);
		set_integer(q, 42, "g");
		TOGO_ASSERTE(integer(q) == 42);
		TOGO_ASSERTE(string::compare_equal(unit(q), "g"));

		auto& c = push_back_inplace(children(a));
		set_name(c, "d");
		TOGO_ASSERTE(string::compare_equal(name(c), "d"));
		set_string(c, "yum");
		TOGO_ASSERTE(&c == find_child(a, "d"));

		auto& t = push_back_inplace(tags(a));
		set_name(t, "leftover");
		TOGO_ASSERTE(&t == find_tag(a, "leftover"));
	}
	return 0;
}
