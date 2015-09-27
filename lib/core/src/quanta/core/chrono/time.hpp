#line 2 "quanta/core/chrono/time.hpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.

@file
@brief Time interface.
@ingroup lib_core_chrono
*/

#pragma once

#include <quanta/core/config.hpp>
#include <quanta/core/types.hpp>
#include <togo/core/utility/utility.hpp>

#include <quanta/core/chrono/types.hpp>
#include <quanta/core/chrono/internal.hpp>
#include <quanta/core/chrono/time.gen_interface>

namespace quanta {
namespace time {

/**
	@addtogroup lib_core_chrono
	@{
*/

/// Set zone offset (in seconds).
///
/// Does not adjust the time referred to, only its offset from UTC.
inline void set_zone_offset(Time& t, s32 zone_offset) {
	t.zone_offset = zone_offset;
}

/// Set zone offset (clock).
///
/// Negative hour implies negative minute.
/// Does not adjust the time referred to, only its offset from UTC.
inline void set_zone_clock(Time& t, signed h, signed m = 0) {
	if (h < 0) {
		m = -m;
	}
	time::set_zone_offset(t, h * SECS_PER_HOUR + m * SECS_PER_MINUTE);
}

/// Set zone offset to UTC.
inline void set_zone_utc(Time& t) {
	time::set_zone_offset(t, 0);
}

/// Adjust zone offset (in seconds).
///
/// Applies the difference between the current and new zone offsets.
inline void adjust_zone_offset(Time& t, s32 zone_offset) {
	t.sec += t.zone_offset - zone_offset;
	time::set_zone_offset(t, zone_offset);
}

/// Adjust zone offset (clock).
///
/// Applies the difference between the current and new zone offsets.
inline void adjust_zone_clock(Time& t, signed h, signed m = 0) {
	if (h < 0) {
		m = -m;
	}
	time::adjust_zone_offset(t, h * SECS_PER_HOUR + m * SECS_PER_MINUTE);
}

/// Adjust zone offset to UTC.
inline void adjust_zone_utc(Time& t) {
	time::adjust_zone_offset(t, 0);
}

/// Make a copy in UTC.
inline Time as_utc(Time const& t) {
	return {t.sec, 0};
}

/// Make a copy in UTC (zone-adjusted).
inline Time as_utc_adjusted(Time const& t) {
	return {t.sec + t.zone_offset, 0};
}

/// The difference between two time points.
inline Duration difference(Time l, Time r) {
	return l.sec - r.sec;
}

/// Add a duration.
inline void add(Time& t, Duration d) {
	t.sec += d;
}

/// Subtract a duration.
inline void sub(Time& t, Duration d) {
	t.sec -= d;
}

/// Compare two times for equality (UTC).
inline bool compare_equal(Time& l, Time& r) {
	return l.sec == r.sec;
}

/// Seconds relative to the POSIX epoch.
inline s64 posix(Time const& t) {
	return t.sec + QUANTA_TO_POSIX;
}

/// Hour on clock (UTC).
inline signed hour_utc(Time const& t) {
	return (internal::abs_utc(t) % SECS_PER_DAY) / SECS_PER_HOUR;
}

/// Minute on clock (UTC).
inline signed minute_utc(Time const& t) {
	return (internal::abs_utc(t) % SECS_PER_HOUR) / SECS_PER_MINUTE;
}

/// Second on clock (UTC).
inline signed second_utc(Time const& t) {
	return (internal::abs_utc(t) % SECS_PER_MINUTE);
}

/// Hour on clock.
inline signed hour(Time const& t) {
	return (internal::abs(t) % SECS_PER_DAY) / SECS_PER_HOUR;
}

/// Minute on clock.
inline signed minute(Time const& t) {
	return (internal::abs(t) % SECS_PER_HOUR) / SECS_PER_MINUTE;
}

/// Second on clock.
inline signed second(Time const& t) {
	return (internal::abs(t) % SECS_PER_MINUTE);
}

/// Clock time (UTC).
inline void clock_utc(Time const& t, signed& h, signed& m, signed& s) {
	u64 abs = internal::abs_utc(t);
	h = (abs % SECS_PER_DAY) / SECS_PER_HOUR;
	m = (abs % SECS_PER_HOUR) / SECS_PER_MINUTE;
	s = (abs % SECS_PER_MINUTE);
}

/// Clock time.
inline void clock(Time const& t, signed& h, signed& m, signed& s) {
	u64 abs = internal::abs(t);
	h = (abs % SECS_PER_DAY) / SECS_PER_HOUR;
	m = (abs % SECS_PER_HOUR) / SECS_PER_MINUTE;
	s = (abs % SECS_PER_MINUTE);
}

/// Clock time in seconds (UTC).
inline Duration clock_seconds_utc(Time const& t) {
	return internal::abs_utc(t) % SECS_PER_DAY;
}

/// Clock time in seconds.
inline Duration clock_seconds(Time const& t) {
	return internal::abs(t) % SECS_PER_DAY;
}

/// Non-clock part in seconds (UTC).
inline Duration date_seconds_utc(Time const& t) {
	u64 abs = internal::abs_utc(t);
	abs -= abs % SECS_PER_DAY;
	return static_cast<s64>(abs) + ABSOLUTE_TO_QUANTA;
}

/// Non-clock part in seconds.
inline Duration date_seconds(Time const& t) {
	u64 abs = internal::abs(t);
	abs -= abs % SECS_PER_DAY;
	return static_cast<s64>(abs) + ABSOLUTE_TO_QUANTA;
}

/// Set the clock time (UTC).
///
/// This sets the clock time independent of the zone offset (input is UTC).
inline void set_utc(Time& t, signed h, signed m, signed s) {
	t.sec += QUANTA_TO_ABSOLUTE;
	t.sec -= t.sec % SECS_PER_DAY;
	t.sec += h * SECS_PER_HOUR + m * SECS_PER_MINUTE + s;
	t.sec += ABSOLUTE_TO_QUANTA;
}

/// Set the clock time (zone-local time).
inline void set(Time& t, signed h, signed m, signed s) {
	t.sec += t.zone_offset;
	time::set_utc(t, h, m, s - t.zone_offset);
}

namespace gregorian {

/// Whether a Gregorian year is a leap year.
inline bool is_leap_year(signed const year) {
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}



/// Gregorian calendar date.
inline Date date(Time const& t) {
	return gregorian::date_internal(internal::abs(t), true);
}

/// Gregorian calendar year.
inline signed year(Time const& t) {
	return gregorian::date_internal(internal::abs(t), false).year;
}

/// Gregorian calendar month.
inline signed month(Time const& t) {
	return gregorian::date(t).month;
}

/// Gregorian calendar day.
inline signed day(Time const& t) {
	return gregorian::date(t).day;
}

/// Whether the time specified is in a Gregorian leap year.
inline bool is_leap_year(Time const& t) {
	return gregorian::is_leap_year(gregorian::year(t));
}

/// Gregorian calendar date (UTC).
inline Date date_utc(Time const& t) {
	return gregorian::date_internal(internal::abs_utc(t), true);
}

/// Gregorian calendar year (UTC).
inline signed year_utc(Time const& t) {
	return gregorian::date_internal(internal::abs_utc(t), false).year;
}

/// Gregorian calendar month (UTC).
inline signed month_utc(Time const& t) {
	return gregorian::date_utc(t).month;
}

/// Gregorian calendar day (UTC).
inline signed day_utc(Time const& t) {
	return gregorian::date_utc(t).day;
}

/// Whether the time specified is in a Gregorian leap year (UTC).
inline bool is_leap_year_utc(Time const& t) {
	return gregorian::is_leap_year(gregorian::year_utc(t));
}

} // namespace gregorian

/** @} */ // end of doc-group lib_core_chrono

} // namespace time
} // namespace quanta
