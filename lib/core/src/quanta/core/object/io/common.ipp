#line 2 "quanta/core/object/io/common.ipp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/object/types.hpp>

namespace quanta {
namespace object {

namespace {

inline u64 pow_int(unsigned base, unsigned exponent) {
	unsigned value = 1;
	while (exponent--) {
		value *= base;
	}
	return value;
}

static u64 parse_integer_unsigned(char const*& it, char const* end) {
	u64 value = 0;
	for (; it != end && *it == '0'; ++it) {}
	for (; it != end; ++it) {
		value *= 10;
		value += *it - '0';
	}
	return value;
}

/*static s64 parse_integer_signed(char const*& it, char const* end) {
	bool negative = *it == '-';
	if (negative || *it == '+') {
		++it;
	}
	u64 value = parse_integer_unsigned(it, end);
	TOGO_LOGF_TRACED("%d %ld\n", negative, negative ? -static_cast<s64>(value) : static_cast<s64>(value));
	return negative ? -static_cast<s64>(value) : static_cast<s64>(value);
}*/

template<bool>
struct parse_s64_adaptor;

template<>
struct parse_s64_adaptor<true> {
	inline static s64 parse(char const* const cstr, unsigned const base) {
		return std::strtol(cstr, nullptr, base);
	}
};

template<>
struct parse_s64_adaptor<false> {
	inline static s64 parse(char const* const cstr, unsigned const base) {
		return std::strtoll(cstr, nullptr, base);
	}
};

inline static s64 parse_s64(char const* const cstr, unsigned const base) {
	return parse_s64_adaptor<sizeof(long) == sizeof(s64)>::parse(cstr, base);
}

template<bool>
struct parse_f64_adaptor;

template<>
struct parse_f64_adaptor<true> {
	inline static f64 parse(char const* const cstr) {
		return std::strtod(cstr, nullptr);
	}
};

template<>
struct parse_f64_adaptor<false> {
	inline static f64 parse(char const* const cstr) {
		return std::strtold(cstr, nullptr);
	}
};

inline static f64 parse_f64(char const* const cstr) {
	return parse_f64_adaptor<sizeof(double) == sizeof(f64)>::parse(cstr);
}

} // anonymous namespace

} // namespace object
} // namespace quanta
