#line 2 "quanta/core/object/io_text.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <togo/core/error/assert.hpp>
#include <togo/core/log/log.hpp>
#include <togo/core/memory/memory.hpp>
#include <togo/core/memory/temp_allocator.hpp>
#include <togo/core/collection/array.hpp>
#include <togo/core/string/string.hpp>
#include <togo/core/io/types.hpp>
#include <togo/core/io/io.hpp>
#include <togo/core/io/file_stream.hpp>

#include <quanta/core/object/object.hpp>
#include <quanta/core/object/io/parser.ipp>

#include <cstdio>

namespace quanta {

/// Read text-format object from stream.
///
/// Returns false if a parser error occurred. pinfo will have the
/// position and error message of the parser.
bool object::read_text(Object& root, IReader& stream, ObjectParserInfo& pinfo) {
	TempAllocator<4096> allocator{};
	ObjectParser p{stream, pinfo, allocator};
	array::reserve(p.stack, 32);
	array::reserve(p.buffer, 4096 - (1 * sizeof(void*)) - (32 * sizeof(ObjectParser::Branch)));

	object::clear(root);
	parser_init(p, root);
	return parser_read(p);
}

/// Read text-format object from stream (sans parser info).
bool object::read_text(Object& root, IReader& stream) {
	ObjectParserInfo pinfo{};
	if (!object::read_text(root, stream, pinfo)) {
		TOGO_LOG_ERRORF(
			"failed to read object: [%2u,%2u]: %s\n",
			pinfo.line, pinfo.column, pinfo.message
		);
		return false;
	}
	return true;
}

/// Read text-format object from file.
bool object::read_text_file(Object& root, StringRef const& path) {
	FileReader stream{};
	if (!stream.open(path)) {
		TOGO_LOG_ERRORF(
			"failed to read object from '%.*s': failed to open file\n",
			path.size, path.data
		);
		return false;
	}

	ObjectParserInfo pinfo{};
	bool const success = object::read_text(root, stream, pinfo);
	if (!success) {
		TOGO_LOG_ERRORF(
			"failed to read object from '%.*s': [%2u,%2u]: %s\n",
			path.size, path.data,
			pinfo.line, pinfo.column, pinfo.message
		);
	}
	stream.close();
	return success;
}

// write

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
	unsigned level = str.empty() ? 1 : 0;
	if (str.size >= 1) {
		char const c = str.data[0];
		if ((c >= '0' && c <= '9') || c == '-' || c == '+') {
			level = 1;
		}
	}
	auto const it_end = end(str);
	for (auto it = begin(str); it != it_end && level < 2; ++it) {
		switch (*it) {
		case '\t':
		case ' ':
		case ',': case ';':
		case ':':
		case '=':
		case '{': case '}':
		case '[': case ']':
		case '(': case ')':
		case '/':
		case '`':
			level = 1;
			break;

		case '\"': // fall-through
		case '\n':
			level = 2;
			break;
		}
	}
	return level;
}

inline static bool write_quote(IWriter& stream, unsigned level) {
	switch (level) {
	case 0: break;
	case 1: RETURN_ERROR(io::write_value(stream, '\"')); break;
	case 2: RETURN_ERROR(io::write(stream, "```", 3)); break;
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
	unsigned tabs
);

static bool write_tag(
	IWriter& stream,
	Object const& obj,
	unsigned tabs
) {
	RETURN_ERROR(
		io::write_value(stream, ':') &&
		write_markers(stream, obj) &&
		write_identifier(stream, object::name(obj))
	);
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
	}
	return true;
}

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
	unsigned tabs
) {
	if (object::has_children(obj)) {
		auto& children = object::children(obj);
		auto it = begin(children);
		auto end = array::end(children);
		RETURN_ERROR(write_object(stream, *it, tabs));
		for (++it; it != end; ++it) {
			RETURN_ERROR(
				io::write(stream, op_string(object::op(*it)), 3) &&
				write_object(stream, *it, tabs)
			);
		}
	}
	return true;
}

static bool write_object(
	IWriter& stream,
	Object const& obj,
	unsigned tabs
) {
	if (object::is_expression(obj)) {
		return write_expression(stream, obj, tabs);
	} else if (object::is_named(obj)) {
		RETURN_ERROR(
			write_identifier(stream, object::name(obj)) &&
			io::write(stream, " = ", 3)
		);
	}
	RETURN_ERROR(write_markers(stream, obj));
	switch (object::type(obj)) {
	case ObjectValueType::null:
		if (
			!(obj.properties & object::M_VALUE_MARKERS) &&
			!object::has_children(obj) &&
			!object::has_tags(obj)
		) {
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
		break;

	case ObjectValueType::decimal:
		RETURN_ERROR(writef(stream, "%.6lg", obj.value.numeric.decimal));
		break;

	case ObjectValueType::string:
		RETURN_ERROR(write_string(stream, object::string(obj)));
		break;

	case ObjectValueType::expression:
		break;
	}
	if (object::has_unit(obj)) {
		RETURN_ERROR(write_identifier(stream, object::unit(obj)));
	}
	if (object::has_source(obj) || object::marker_source_uncertain(obj)) {
		RETURN_ERROR(write_source(stream, object::source(obj), object::marker_source_uncertain(obj)));
		if (object::has_sub_source(obj) || object::marker_sub_source_uncertain(obj)) {
			RETURN_ERROR(write_source(stream, object::sub_source(obj), object::marker_sub_source_uncertain(obj)));
		}
	}
	// TODO: split pre- and post-tags
	for (auto& tag : object::tags(obj)) {
		RETURN_ERROR(write_tag(stream, tag, tabs));
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
		if (object::has_children(quantity)) {
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

// unsigned object::prewrite_fix() // TODO
// bool object::prewrite_validate() // TODO
// tag:
/*
	if (unmanaged_string::empty(name)) {
		TOGO_LOG_ERROR("unnamed tag\n");
		return false;
	} else if (!object::is_null(obj)) {
		TOGO_LOG_ERRORF("tag '%.*s' is non-null\n", name.size, name.data);
		return false;
	} else if (object::has_tags(obj)) {
		TOGO_LOG_ERRORF("tag '%.*s' has tags itself\n", name.size, name.data);
		return false;
	} else if (object::has_quantity(obj)) {
		TOGO_LOG_ERRORF("tag '%.*s' has quantity\n", name.size, name.data);
		return false;
	}
*/

/// Write text-format object to stream.
///
/// Returns true if the write succeeded.
bool object::write_text(Object const& obj, IWriter& stream) {
	for (auto& child : object::children(obj)) {
		if (!(
			write_object(stream, child, 0) &&
			io::write_value(stream, '\n')
		)) {
			return false;
		}
	}
	return io::status(stream).ok();
}

/// Write text-format object to file.
bool object::write_text_file(Object const& obj, StringRef const& path) {
	FileWriter stream{};
	if (!stream.open(path, false)) {
		TOGO_LOG_ERRORF(
			"failed to write object to '%.*s': failed to open file\n",
			path.size, path.data
		);
		return false;
	}
	bool const success = object::write_text(obj, stream);
	stream.close();
	return success;
}

} // namespace quanta
