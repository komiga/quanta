
#include <togo/core/error/assert.hpp>
#include <togo/support/test.hpp>

#include <quanta/core/chrono/time.hpp>

using namespace quanta;

#define ASSERT_DATE(t, y_, m_, d_, yd_) do {\
	Date const date = time::gregorian::date(t); \
	TOGO_ASSERTE(date.year == y_ && date.month == m_ && date.day == d_ && date.year_day == yd_); \
} while (false)

#define ASSERT_CLOCK(t, h_, m_, s_) \
	TOGO_ASSERTE(hour(t) == h_ && minute(t) == m_ && second(t) == s_)

#define ASSERT_BOTH(t, y_, m_, d_, yd_, ch_, cm_, cs_) \
	ASSERT_DATE(t, y_, m_, d_, yd_); \
	ASSERT_CLOCK(t, ch_, cm_, cs_)

signed main() {
	{
		Time t{};
		TOGO_ASSERTE(t.sec == 0 && t.zone_offset == 0);
		ASSERT_BOTH(t, 1,1,1, 1, 0,0,0);
	}

	{
		Time t{24 * 60 * 60, 0};
		ASSERT_BOTH(t, 1,1,2, 2, 0,0,0);

		add(t, 1 * 60 * 60 + 2 * 60 + 30);
		ASSERT_BOTH(t, 1,1,2, 2, 1,2,30);

		sub(t, 60);
		ASSERT_CLOCK(t, 1,1,30);
	}

	{
		Time t{};
		set(t, 12,15,45);
		ASSERT_BOTH(t, 1,1,1, 1, 12,15,45);

		set(t, 25,-59,-60);
		ASSERT_BOTH(t, 1,1,2, 2, 0,0,0);
	}

	{
		Time t{};
		set(t, 18,0,0);
		adjust_zone_offset(t, -6 * 60 * 60);
		ASSERT_BOTH(t, 1,1,1, 1, 18,0,0);

		adjust_zone_utc(t);
		set_zone_offset(t, -6 * 60 * 60);
		ASSERT_BOTH(t, 1,1,1, 1, 12,0,0);
	}

	{
		Time t{};
		adjust_zone_clock(t, +1);
		time::gregorian::set(t, 2,3,4);
		ASSERT_BOTH(t, 2,3,4, 31+28+4, 0,0,0);

		set(t, 11,22,33);
		ASSERT_BOTH(t, 2,3,4, 31+28+4, 11,22,33);

		time::gregorian::set(t, 9,6,7);
		ASSERT_BOTH(t, 9,6,7, 31+28+31+30+31+7, 11,22,33);

		time::gregorian::set(t, 2,4,14, 2,3,1);
		ASSERT_BOTH(t, 2,4,14, 31+28+31+14, 2,3,1);
	}
	return 0;
}
