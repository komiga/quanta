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

/// Seconds relative to the POSIX epoch.
inline s64 posix(Time const& t) {
	return t.sec + QUANTA_TO_POSIX;
}

/// Hour on clock.
inline signed hour(Time const& t) {
	return (time::abs(t) % SECS_PER_DAY) / SECS_PER_HOUR;
}

/// Minute on clock.
inline signed minute(Time const& t) {
	return (time::abs(t) % SECS_PER_HOUR) / SECS_PER_MINUTE;
}

/// Second on clock.
inline signed second(Time const& t) {
	return time::abs(t) % SECS_PER_MINUTE;
}

/// Set the clock time (UTC).
///
/// This sets the clock time independent of the zone offset.
/// If the current time is negative, it is retained when setting the clock time.
inline void set_utc(Time& t, signed h, signed m, signed s) {
	s64 ct = h * SECS_PER_HOUR + m * SECS_PER_MINUTE + s;
	if (t.sec < 0) {
		t.sec += -t.sec % SECS_PER_DAY;
		t.sec -= ct;
	} else {
		t.sec -= t.sec % SECS_PER_DAY;
		t.sec += ct;
	}
}

/// Set the clock time (zone-local time).
inline void set(Time& t, signed h, signed m, signed s) {
	t.sec += t.zone_offset;
	time::set_utc(t, h, m, s);
	t.sec -= t.zone_offset;
}

namespace gregorian {

/// Whether a Gregorian year is a leap year.
inline bool is_leap_year(signed const year) {
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

/// Whether the time specified is in a Gregorian leap year.
inline bool is_leap_year(Time const& t) {
	return gregorian::is_leap_year(gregorian::date_internal(t, false).year);
}

/// Gregorian calendar date.
inline Date date(Time const& t) {
	return gregorian::date_internal(t, true);
}

/// Gregorian calendar year.
inline signed year(Time const& t) {
	return gregorian::date_internal(t, false).year;
}

/// Gregorian calendar month.
inline signed month(Time const& t) {
	return gregorian::date_internal(t, true).month;
}

/// Gregorian calendar day.
inline signed day(Time const& t) {
	return gregorian::date_internal(t, true).day;
}

} // namespace gregorian

/** @} */ // end of doc-group lib_core_chrono

} // namespace time
} // namespace quanta
