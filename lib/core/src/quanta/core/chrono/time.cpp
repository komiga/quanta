#line 2 "quanta/core/chrono/time.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/types.hpp>
#include <quanta/core/chrono/internal.hpp>
#include <quanta/core/chrono/time.hpp>

#include <togo/core/utility/utility.hpp>

namespace quanta {

namespace time {

namespace {

static signed const days_before[]{
	0,
	31,
	31 + 28,
	31 + 28 + 31,
	31 + 28 + 31 + 30,
	31 + 28 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30,
	31 + 28 + 31 + 30 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30, // 01 - 12
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31, // 0001-01 - 0002-01
};

} // anonymous namespace

IGEN_PRIVATE
Date gregorian::date_internal(u64 abs, bool const full) {
	Date date{};
	{
	u64 n;
	u64 y = 0;
	u64 d = abs / SECS_PER_DAY;

	// 400-year cycles
	n = d / DAYS_PER_400_YEARS;
	y += 400 * n;
	d -= DAYS_PER_400_YEARS * n;

	// 100-year cycles
	n = d / DAYS_PER_100_YEARS;
	n -= n >> 2;
	y += 100 * n;
	d -= DAYS_PER_100_YEARS * n;

	// 4-year cycles
	n = d / DAYS_PER_4_YEARS;
	y += 4 * n;
	d -= DAYS_PER_4_YEARS * n;

	// within 4-year cycle
	n = d / 365;
	n -= n >> 2;
	y += n;
	d -= 365 * n;

	date.year = static_cast<signed>(static_cast<s64>(y) + YEAR_ABSOLUTE);
	date.year_day = static_cast<signed>(d) + 1;
	}

	if (!full) {
		return date;
	}

	date.day = date.year_day - 1;
	if (gregorian::is_leap_year(date.year)) {
		// NB: -1 because day is currently 0-based
		if (date.day > 31 + 29 - 1) {
			// after leap day (act like it doesn't exist)
			--date.day;
		} else if (date.day == 31 + 29 - 1) {
			// leap day
			date.month = 2;
			date.day = 29;
			return date;
		}
	}

	// estimate and adjust
	date.month = date.day / 31;
	signed end = days_before[date.month + 1];
	if (date.day >= end) {
		// low estimate
		++date.month;
		date.day -= end;
	} else {
		date.day -= days_before[date.month];
	}

	++date.month; // month is 1-based
	++date.day; // day is 1-based
	return date;
}

/// Set the Gregorian calendar date (UTC).
void gregorian::set_utc(Time& t, signed year, signed month, signed day) {
	--month;
	internal::normalize(year, month, 12);
	++month;

	// accumulate days
	u64 n;
	u64 y = static_cast<u64>(static_cast<s64>(year) - YEAR_ABSOLUTE);
	u64 d = 0;

	// 400-year cycles
	n = y / 400;
	y -= 400 * n;
	d += DAYS_PER_400_YEARS * n;

	// 100-year cycles
	n = y / 100;
	y -= 100 * n;
	d += DAYS_PER_100_YEARS * n;

	// 4-year cycles
	n = y / 4;
	y -= 4 * n;
	d += DAYS_PER_4_YEARS * n;

	// non-leap years
	d += 365 * y;

	// days preceding the month
	d += static_cast<u64>(days_before[month - 1]);
	if (gregorian::is_leap_year(year) && month >= 3) {
		++d; // leap day
	}

	// days from month
	d += static_cast<u64>(day - 1);

	t.sec = time::clock_seconds_utc(t);
	t.sec += d * SECS_PER_DAY + ABSOLUTE_TO_QUANTA;
}

/// Set the Gregorian calendar date.
void gregorian::set(Time& t, signed year, signed month, signed day) {
	t.sec += t.zone_offset;
	gregorian::set_utc(t, year, month, day);
	t.sec -= t.zone_offset;
}

/// Set the Gregorian calendar date and clock time (UTC).
void gregorian::set_utc(
	Time& t,
	signed year, signed month, signed day,
	signed clock_h, signed clock_m, signed clock_s
) {
	internal::normalize(clock_m, clock_s, 60);
	internal::normalize(clock_h, clock_m, 60);
	internal::normalize(day, clock_h, 24);
	gregorian::set_utc(t, year, month, day);
	time::set_utc(t, clock_h, clock_m, clock_s);
}

/// Set the Gregorian calendar date and clock time (zone-local).
void gregorian::set(
	Time& t,
	signed year, signed month, signed day,
	signed clock_h, signed clock_m, signed clock_s
) {
	t.sec += t.zone_offset;
	gregorian::set_utc(t, year, month, day, clock_h, clock_m, clock_s);
	t.sec -= t.zone_offset;
}

/// Construct a time point from a Gregorian calendar date (UTC).
void gregorian::set_utc(Time& t, Date const& date) {
	gregorian::set_utc(t, date.year, date.month, date.day);
}

/// Construct a time point from a Gregorian calendar date.
void gregorian::set(Time& t, Date const& date) {
	gregorian::set(t, date.year, date.month, date.day);
}

/// Construct a time point from a Gregorian calendar date and clock time (UTC).
void gregorian::set_utc(Time& t, Date const& date, signed h, signed m, signed s) {
	gregorian::set_utc(t, date.year, date.month, date.day, h, m, s);
}

/// Construct a time point from a Gregorian calendar date and clock time (zone-local).
void gregorian::set(Time& t, Date const& date, signed h, signed m, signed s) {
	gregorian::set(t, date.year, date.month, date.day, h, m, s);
}

} // namespace time

} // namespace quanta
