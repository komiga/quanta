#line 2 "quanta/core/chrono/types.hpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.

@file
@brief Chronometry types.
@ingroup lib_core_types
@ingroup lib_core_chrono
*/

#pragma once

#include <quanta/core/config.hpp>
#include <quanta/core/types.hpp>

#include <togo/core/lua/types.hpp>

namespace quanta {
namespace time {

/**
	@addtogroup lib_core_chrono
	@{
*/

/// A span of time.
using Duration = s64;

/// A point in time relative to 0001-01-01T00:00:00Z in the Gregorian calendar.
struct Time {
	TOGO_LUA_MARK_USERDATA(quanta::time::Time);

	s64 sec; // seconds relative to epoch in UTC
	s32 zone_offset; // timezone offset in seconds (sec_local = sec + zone_offset)
};

/** @} */ // end of doc-group lib_core_chrono

} // namespace time

using time::Duration;
using time::Time;

namespace date {

/// A date in a traditionally-structured calendar.
struct Date {
	s32 year;
	s32 month;
	s32 day;
	s32 year_day;
};

} // namespace date

using date::Date;

} // namespace quanta
