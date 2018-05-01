#line 2 "quanta/core/object/io/writer.ipp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/object/object.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>
#include <togo/core/collection/array.hpp>
#include <togo/core/io/types.hpp>
#include <togo/core/io/io.hpp>

#include <cstdio>

namespace quanta {
namespace object {

namespace {

static constexpr char const TABS[]{
	"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
};

#define RETURN_ERROR(x) if (!(x)) { return false; }

static bool write_tabs(IWriter& stream, unsigned tabs) {
	while (tabs > 0) {
		unsigned const amount = min(tabs, array_extent(TABS));
		if (!io::write(stream, TABS, amount)) {
			return false;
		}
		tabs -= amount;
	}
	return true;
}

#if defined(TOGO_COMPILER_CLANG) || \
	defined(TOGO_COMPILER_GCC)
	__attribute__((__format__ (__printf__, 2, 3)))
#endif
static bool writef(IWriter& stream, char const* const format, ...) {
	char buffer[256];
	va_list va;
	va_start(va, format);
	signed const size = std::vsnprintf(buffer, array_extent(buffer), format, va);
	va_end(va);
	if (size < 0) {
		return false;
	}
	return io::write(stream, buffer, static_cast<unsigned>(size));
}

inline static unsigned quote_level(StringRef const& str) {
	auto const it_end = end(str);
	for (auto it = begin(str); it != it_end; ++it) {
		switch (*it) {
		case '\\':
		case '\"':
		case '\n':
			return 1;
		}
	}
	return 0;
}

inline static bool write_quote(IWriter& stream, unsigned level) {
	switch (level) {
	case 0: RETURN_ERROR(io::write_value(stream, '\"')); break;
	case 1: RETURN_ERROR(io::write(stream, "```", 3)); break;
	}
	return true;
}

inline static bool write_identifier(IWriter& stream, StringRef const& str) {
	RETURN_ERROR(io::write(stream, str.data, str.size));
	return true;
}

inline static bool write_string(IWriter& stream, StringRef const& str) {
	unsigned const level = quote_level(str);
	RETURN_ERROR(
		write_quote(stream, level) &&
		io::write(stream, str.data, str.size) &&
		write_quote(stream, level)
	);
	return true;
}

inline static bool write_source(IWriter& stream, unsigned source, bool uncertain) {
	RETURN_ERROR(
		io::write(stream, "$?", 1 + uncertain) &&
		(source == 0 || writef(stream, "%u", source))
	);
	return true;
}

inline static bool write_markers(IWriter& stream, Object const& obj) {
	if (object::marker_value_uncertain(obj)) {
		RETURN_ERROR(io::write_value(stream, '?'));
	} else if (object::marker_value_guess(obj)) {
		RETURN_ERROR(io::write(stream, "G~", 2));
	}
	auto approximation = object::value_approximation(obj);
	if (approximation < 0) {
		RETURN_ERROR(io::write(stream, "~~~", unsigned_cast(-approximation)));
	} else if (approximation > 0) {
		RETURN_ERROR(io::write(stream, "^^^", unsigned_cast(approximation)));
	}
	return true;
}

static bool write_object(
	IWriter& stream,
	Object const& obj,
	unsigned tabs,
	bool named = true
);

inline char const* op_string(ObjectOperator const op) {
	switch (op) {
	case ObjectOperator::add: return " + ";
	case ObjectOperator::sub: return " - ";
	case ObjectOperator::mul: return " * ";
	case ObjectOperator::div: return " / ";
	}
}

static bool write_expression(
	IWriter& stream,
	Object const& obj,
	unsigned tabs,
	bool scoped
) {
	scoped |= array::size(object::expression(obj)) <= 1;
	if (scoped) {
		RETURN_ERROR(io::write_value(stream, '('));
	}
	if (object::has_operands(obj)) {
		auto& operands = object::expression(obj);
		auto it = begin(operands);
		auto end = array::end(operands);
		RETURN_ERROR(write_object(stream, *it, tabs, true));
		for (++it; it != end; ++it) {
			RETURN_ERROR(
				io::write(stream, op_string(object::op(*it)), 3) &&
				write_object(stream, *it, tabs, false)
			);
		}
	}
	if (scoped) {
		RETURN_ERROR(io::write_value(stream, ')'));
	}
	return true;
}

inline bool would_write_value(Object const& obj, bool const is_tag) {
	return !object::is_null(obj) || (// if we would write a null
		!is_tag &&
		!(obj.properties & object::M_VALUE_MARKERS) &&
		!object::has_children(obj) &&
		!object::has_tags(obj)
	);
}

static bool write_value(
	IWriter& stream,
	Object const& obj,
	unsigned tabs,
	bool would_write,
	bool const is_tag
) {
	if (would_write) {
		if (is_tag) {
			RETURN_ERROR(io::write_value(stream, '='));
		} else if (object::is_named(obj)) {
			RETURN_ERROR(io::write(stream, " = ", 3));
		}
		RETURN_ERROR(write_markers(stream, obj));
	} else if (!is_tag) {
		RETURN_ERROR(write_markers(stream, obj));
	}

	switch (object::type(obj)) {
	case ObjectValueType::null:
		if (would_write) {
			RETURN_ERROR(io::write(stream, "null", 4));
		}
		break;

	case ObjectValueType::boolean:
		if (obj.value.boolean) {
			RETURN_ERROR(io::write(stream, "true", 4));
		} else {
			RETURN_ERROR(io::write(stream, "false", 5));
		}
		break;

	case ObjectValueType::integer:
		RETURN_ERROR(writef(stream, "%ld", obj.value.numeric.integer));
		goto l_write_unit;

	case ObjectValueType::decimal:
		RETURN_ERROR(writef(stream, "%.6lg", obj.value.numeric.decimal));
		goto l_write_unit;

	case ObjectValueType::currency: {
		RETURN_ERROR(io::write(stream, "\xC2\xA4", 2)); // ¤ in UTF-8
		s64 value = 0;
		if (obj.value.numeric.c.value < 0) {
			RETURN_ERROR(io::write_value(stream, '-'));
			value = -obj.value.numeric.c.value;
		} else {
			value = obj.value.numeric.c.value;
		}
		if (obj.value.numeric.c.exponent == 0) {
			RETURN_ERROR(writef(stream, "%li", value));
		} else if (obj.value.numeric.c.exponent < 0) {
			RETURN_ERROR(writef(stream, "0.%0*li", -obj.value.numeric.c.exponent, value));
		} else {
			u64 scale = object::pow_int(10, obj.value.numeric.c.exponent);
			s64 minor = value % scale;
			RETURN_ERROR(writef(
				stream, "%li.%0*li",
				(value - minor) / scale,
				obj.value.numeric.c.exponent, minor
			));
		}
	}	goto l_write_unit;

	l_write_unit:
		if (object::has_unit(obj)) {
			RETURN_ERROR(write_identifier(stream, object::unit(obj)));
		} else if (object::is_currency(obj)) {
			RETURN_ERROR(io::write(stream, "unknown", 7));
		}
		break;

	case ObjectValueType::time: {
		signed h, m, s;
		Time const& t = object::time_value(obj);
		if (object::has_date(obj)) {
			Date date = time::gregorian::date(t);
			if (!object::is_year_contextual(obj)) {
				RETURN_ERROR(writef(stream, "%04d-", date.year));
			}
			if (!object::is_month_contextual(obj)) {
				RETURN_ERROR(writef(stream, "%02d-", date.month));
			}
			RETURN_ERROR(writef(stream, "%02d", date.day));
			if (
				object::has_clock(obj) || (
					object::is_zoned(obj)
					? t.zone_offset != 0
					: object::is_month_contextual(obj)
				)
			) {
				RETURN_ERROR(io::write_value(stream, 'T'));
			}
		}
		if (object::has_clock(obj)) {
			time::clock(t, h, m, s);
			RETURN_ERROR(writef(stream, "%02d:%02d:%02d", h, m, s));
		}
		if (object::is_zoned(obj)) {
			if (t.zone_offset == 0) {
				RETURN_ERROR(io::write(stream, "Z", 1));
			} else {
				bool negative = t.zone_offset < 0;
				time::clock(Time{negative ? -t.zone_offset : t.zone_offset, 0}, h, m, s);
				if (m == 0) {
					RETURN_ERROR(writef(stream, "%c%02d", negative ? '-' : '+', h));
				} else {
					RETURN_ERROR(writef(stream, "%c%02d:%02d", negative ? '-' : '+', h, m));
				}
			}
		}
	}	break;

	case ObjectValueType::string:
		if (object::has_string_type(obj)) {
			RETURN_ERROR(write_identifier(stream, object::string_type(obj)));
		}
		RETURN_ERROR(write_string(stream, object::string(obj)));
		break;

	case ObjectValueType::identifier:
		RETURN_ERROR(write_identifier(stream, object::identifier(obj)));
		break;

	case ObjectValueType::expression:
		RETURN_ERROR(write_expression(
			stream, obj, tabs,
			is_tag ||
			(obj.properties & object::M_VALUE_MARKERS) ||
			object::has_source(obj) ||
			object::has_tags(obj) ||
			object::has_children(obj) ||
			object::has_quantity(obj)
		));
		break;
	}
	return true;
}

static bool write_tag(
	IWriter& stream,
	Object const& obj,
	unsigned tabs,
	bool scope_childless
) {
	bool const would_write = would_write_value(obj, true);

	RETURN_ERROR(io::write_value(stream, ':'));
	if (!would_write) {
		RETURN_ERROR(write_markers(stream, obj));
	}
	if (object::is_named(obj)) {
		RETURN_ERROR(write_identifier(stream, object::name(obj)));
	}
	RETURN_ERROR(write_value(stream, obj, tabs, would_write, true));

	if (object::has_children(obj)) {
		RETURN_ERROR(io::write_value(stream, '('));
		auto& last_child = array::back(object::children(obj));
		for (auto& child : object::children(obj)) {
			RETURN_ERROR(
				write_object(stream, child, tabs) &&
				(&child == &last_child || io::write(stream, ", ", 2))
			);
		}
		RETURN_ERROR(io::write_value(stream, ')'));
	} else if (scope_childless || (
		!would_write &&
		!object::is_named(obj) &&
		!(obj.properties & object::M_VALUE_MARKERS)
	)) {
		RETURN_ERROR(io::write(stream, "()", 2));
	}
	return true;
}

static bool write_object(
	IWriter& stream,
	Object const& obj,
	unsigned tabs,
	bool named
) {
	if (named && object::is_named(obj)) {
		RETURN_ERROR(write_identifier(stream, object::name(obj)));
	}
	RETURN_ERROR(write_value(stream, obj, tabs, would_write_value(obj, false), false));

	if (object::has_source(obj) || object::marker_source_uncertain(obj)) {
		RETURN_ERROR(write_source(stream, object::source(obj), object::marker_source_uncertain(obj)));
		if (object::has_sub_source(obj) || object::marker_sub_source_uncertain(obj)) {
			RETURN_ERROR(write_source(stream, object::sub_source(obj), object::marker_sub_source_uncertain(obj)));
		}
	}

	// TODO: split pre- and post-tags
	for (auto& tag : object::tags(obj)) {
		RETURN_ERROR(write_tag(stream, tag, tabs, false));
	}
	if (object::has_children(obj)) {
		RETURN_ERROR(io::write(stream, "{\n", 2));
		++tabs;
		for (auto& child : object::children(obj)) {
			RETURN_ERROR(
				write_tabs(stream, tabs) &&
				write_object(stream, child, tabs) &&
				io::write_value(stream, '\n')
			);
		}
		--tabs;
		RETURN_ERROR(
			write_tabs(stream, tabs) &&
			io::write_value(stream, '}')
		);
	}
	if (object::has_quantity(obj)) {
		auto& quantity = *object::quantity(obj);
		RETURN_ERROR(io::write_value(stream, '['));
		if (object::is_null(quantity) && object::has_children(quantity)) {
			auto& last_child = array::back(object::children(quantity));
			for (auto& child : object::children(quantity)) {
				RETURN_ERROR(
					write_object(stream, child, tabs) &&
					(&child == &last_child || io::write(stream, ", ", 2))
				);
			}
		} else {
			RETURN_ERROR(write_object(stream, quantity, tabs));
		}
		RETURN_ERROR(io::write_value(stream, ']'));
	}
	return true;
}

#undef RETURN_ERROR

} // anonymous namespace

} // namespace object
} // namespace quanta
