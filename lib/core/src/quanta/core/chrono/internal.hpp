#line 2 "quanta/core/chrono/internal.hpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.

@file
@brief Chronometry internals.
*/

#pragma once

#include <quanta/core/config.hpp>
#include <quanta/core/chrono/types.hpp>

namespace quanta {
namespace time {

namespace {

// NB: modeled after Go
enum : s64 {
	SECS_PER_MINUTE = 60,
	SECS_PER_HOUR = 60 * 60,
	SECS_PER_DAY = 24 * 60 * 60,
	SECS_PER_WEEK = 7 * SECS_PER_DAY,
	DAYS_PER_400_YEARS = 365 * 400 + 97,
	DAYS_PER_100_YEARS = 365 * 100 + 24,
	DAYS_PER_4_YEARS   = 365 * 4 + 1,

	YEAR_QUANTA = 1,
	YEAR_POSIX = 1970,
	YEAR_ABSOLUTE = -292277022399,
	// ABSOLUTE_TO_QUANTA = static_cast<s64>((YEAR_ABSOLUTE - YEAR_QUANTA) * 365.2425 * SECS_PER_DAY),
	ABSOLUTE_TO_QUANTA = -9223371966579724800ll,
	QUANTA_TO_ABSOLUTE = -ABSOLUTE_TO_QUANTA,

	POSIX_TO_QUANTA = (1969*365 + 1969/4 - 1969/100 + 1969/400) * SECS_PER_DAY,
	QUANTA_TO_POSIX = -POSIX_TO_QUANTA,
};

namespace internal {

// normalize value into higher-order multiples
inline void normalize(signed& h, signed& l, signed m) {
	signed n;
	if (l < 0) {
		n = (-l - 1) / m + 1;
		h -= n;
		l += n * m;
	}
	if (l >= m) {
		n = l / m;
		h += n;
		l -= n * m;
	}
}

inline u64 abs_utc(Time const& t) {
	return static_cast<u64>(t.sec + QUANTA_TO_ABSOLUTE);
}

inline u64 abs(Time const& t) {
	return static_cast<u64>((t.sec + t.zone_offset) + QUANTA_TO_ABSOLUTE);
}

} // namespace internal
} // anonymous namespace

} // namespace time
} // namespace quanta
