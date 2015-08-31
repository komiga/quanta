
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
		TOGO_ASSERTE(
			!has_source(a) && source(a) == 0 && sub_source(a) == 0 &&
			!source_certain(a) && source_certain_or_unspecified(a) &&
			!marker_source_uncertain(a) && !marker_sub_source_uncertain(a)
		);

		set_source(a, 1);
		set_sub_source(a, 2);
		TOGO_ASSERTE(
			has_source(a) && source(a) == 1 && sub_source(a) == 2 &&
			source_certain(a) && source_certain_or_unspecified(a) &&
			!marker_source_uncertain(a) && !marker_sub_source_uncertain(a)
		);
		set_source_certain(a, false);
		TOGO_ASSERTE(
			!source_certain(a) && !source_certain_or_unspecified(a) &&
			marker_source_uncertain(a) && !marker_sub_source_uncertain(a)
		);
		set_sub_source_certain(a, false);
		TOGO_ASSERTE(
			!source_certain(a) && !source_certain_or_unspecified(a) &&
			marker_source_uncertain(a) && marker_sub_source_uncertain(a)
		);
		clear_source(a);
		TOGO_ASSERTE(
			!has_source(a) && source(a) == 0 && sub_source(a) == 0 &&
			!source_certain(a) && source_certain_or_unspecified(a) &&
			!marker_source_uncertain(a) && !marker_sub_source_uncertain(a)
		);
	}

	{
		Object a;
		TOGO_ASSERTE(
			value_certain(a) &&
			!marker_value_uncertain(a) && !marker_value_guess(a) &&
			value_approximation(a) == 0
		);

		set_value_certain(a, false);
		TOGO_ASSERTE(!value_certain(a) && marker_value_uncertain(a) && !marker_value_guess(a));
		set_value_guess(a, true);
		TOGO_ASSERTE(!value_certain(a) && !marker_value_uncertain(a) && marker_value_guess(a));
		set_value_certain(a, true);
		TOGO_ASSERTE(value_certain(a) && !marker_value_uncertain(a) && !marker_value_guess(a));

		set_value_approximation(a, -3);
		TOGO_ASSERTE(!value_certain(a) && value_approximation(a) == -3);
		set_value_approximation(a, 0);
		TOGO_ASSERTE(value_certain(a) && value_approximation(a) == 0);
		set_value_approximation(a, +3);
		TOGO_ASSERTE(!value_certain(a) && value_approximation(a) == +3);

		clear_value_markers(a);
		TOGO_ASSERTE(
			value_certain(a) &&
			!marker_value_uncertain(a) && !marker_value_guess(a) &&
			value_approximation(a) == 0
		);
	}

	{
		Object a;
		set_string(a, "brains");
		TOGO_ASSERTE(string::compare_equal(object::string(a), "brains"));

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
