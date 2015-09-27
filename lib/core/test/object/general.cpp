
#include <togo/core/error/assert.hpp>
#include <togo/core/string/string.hpp>
#include <togo/support/test.hpp>

#include <quanta/core/chrono/time.hpp>
#include <quanta/core/object/object.hpp>

using namespace quanta;

signed main() {
	memory_init();

	{
		Object a;
		TOGO_ASSERTE(
			!is_named(a) &&
			name_hash(a) == OBJECT_NAME_NULL &&
			string::compare_equal(name(a), "")
		);
		TOGO_ASSERTE(op(a) == ObjectOperator::none);
		TOGO_ASSERTE(
			!has_source(a) && source(a) == 0 && sub_source(a) == 0 &&
			!source_certain(a) && source_certain_or_unspecified(a) &&
			!marker_source_uncertain(a) && !marker_sub_source_uncertain(a)
		);
		TOGO_ASSERTE(
			value_certain(a) && value_approximation(a) == 0 &&
			!marker_value_uncertain(a) && !marker_value_guess(a)
		);
		TOGO_ASSERTE(is_null(a));
		TOGO_ASSERTE(!has_tags(a));
		TOGO_ASSERTE(!has_children(a));
		TOGO_ASSERTE(!has_quantity(a));
		TOGO_ASSERTE(!find_child(a, OBJECT_NAME_NULL));
		TOGO_ASSERTE(!find_child(a, ""));
		TOGO_ASSERTE(!find_child(a, "a"));
		TOGO_ASSERTE(!find_tag(a, OBJECT_NAME_NULL));
		TOGO_ASSERTE(!find_tag(a, ""));
		TOGO_ASSERTE(!find_tag(a, "a"));
	}

	{
		Object a;
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
		set_name(a, "name");
		TOGO_ASSERTE(is_named(a) && string::compare_equal(name(a), "name") && name_hash(a) == "name"_object_name);
		clear_name(a);
		TOGO_ASSERTE(!is_named(a) && string::compare_equal(name(a), "") && name_hash(a) == OBJECT_NAME_NULL);
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

	{
		Time wow{};
		// treated as local when resolve_time() is called (from unzoned to -04:00)
		time::set(wow, 10,16,0);

		Object a;
		set_time_value(a, wow);
		TOGO_ASSERTE(is_zoned(a));
		TOGO_ASSERTE(!is_year_contextual(a) && !is_month_contextual(a));
		TOGO_ASSERTE(has_date(a) && has_clock(a) && time_type(a) == ObjectTimeType::date_and_clock);

		set_zoned(a, false);
		TOGO_ASSERTE(!is_zoned(a));
		set_time_type(a, ObjectTimeType::date);
		TOGO_ASSERTE(has_date(a) && !has_clock(a) && time_type(a) == ObjectTimeType::date);
		set_time_type(a, ObjectTimeType::clock);
		TOGO_ASSERTE(!has_date(a) && has_clock(a) && time_type(a) == ObjectTimeType::clock);

		time::adjust_zone_clock(wow, -4);
		time::gregorian::set(wow, 1977,8,15);
		resolve_time(a, wow);
		TOGO_ASSERTE(is_zoned(a));
		TOGO_ASSERTE(has_date(a) && has_clock(a) && time_type(a) == ObjectTimeType::date_and_clock);

		TOGO_ASSERTE(time::compare_equal(time_value(a), wow));
	}

	{
		Date d;
		Time t{};
		time::gregorian::set(t, 1,10,15);

		Object a;
		set_time_value(a, t);

		t = {};
		set_year_contextual(a, true);
		time::gregorian::set(t, 2,3,10);
		resolve_time(a, t);
		d = time::gregorian::date(time_value(a));
		TOGO_ASSERTE(d.year == 2 && d.month == 10 && d.day == 15);

		set_month_contextual(a, true); // implies contextual year
		time::gregorian::set(t, 4,6,20);
		resolve_time(a, t);
		d = time::gregorian::date(time_value(a));
		TOGO_ASSERTE(d.year == 4 && d.month == 6 && d.day == 15);
	}

	{
		Object a;
		set_expression(a);
		TOGO_ASSERTE(is_expression(a));
		auto& e1 = push_back_inplace(children(a));
		set_integer(e1, 1);
		TOGO_ASSERTE(op(e1) == ObjectOperator::none);
		auto& e2 = push_back_inplace(children(a));
		set_integer(e2, 2);
		set_op(e2, ObjectOperator::div);
		TOGO_ASSERTE(op(e2) == ObjectOperator::div);
	}
	return 0;
}
