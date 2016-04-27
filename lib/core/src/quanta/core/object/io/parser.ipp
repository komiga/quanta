#line 2 "quanta/core/object/io/parser.ipp"
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

#include <cstdlib>
#include <cstdio>
#include <cstring>

namespace quanta {
namespace object {

namespace {

enum : unsigned {
	PC_EOF = ~0u,
	PC_PEEK_EMPTY = PC_EOF - 1u,
};

enum : unsigned {
	PF_NONE				= 0,
	PF_ERROR			= 1 << 0, // error met
	PF_CARRY			= 1 << 1, // carry the current parser char forward
};

enum ParserBufferType : unsigned {
	PB_NONE,
	PB_SOURCE,
	PB_MARKER_UNCERTAINTY,
	PB_MARKER_GUESS,
	PB_MARKER_APPROXIMATION,
	PB_NULL,
	PB_FALSE,
	PB_TRUE,
	PB_INTEGER,
	PB_DECIMAL,
	PB_STRING,
	PB_IDENTIFIER,
	PB_TIME,
	PB_CURRENCY,
};

enum class ApplyBufferAs : unsigned {
	value,
	name,
	unit,
	source,
	sub_source,
};

enum : unsigned {
	TSP_D_YYYY		= 1 << 0,
	TSP_D_MM		= 1 << 1,
	TSP_D_DD		= 1 << 2,

	TSP_T_MARKER	= 1 << 3,
	TSP_T_HH		= 1 << 4,
	TSP_T_MM		= 1 << 5,
	TSP_T_SS		= 1 << 6,

	TSP_Z_UTC		= 1 << 7,
	TSP_Z_SIGN		= 1 << 8,
	TSP_Z_HH		= 1 << 9,
	TSP_Z_MM		= 1 << 10,

	TSP_D_ELEMENTS
		= TSP_D_YYYY
		| TSP_D_MM
		| TSP_D_DD
	,
	TSP_D_ALL
		= TSP_D_ELEMENTS
	,

	TSP_T_ELEMENTS
		= TSP_T_HH
		| TSP_T_MM
		| TSP_T_SS
	,
	TSP_T_ALL
		= TSP_T_MARKER
		| TSP_T_ELEMENTS
	,

	TSP_Z_ELEMENTS
		= TSP_Z_HH
		| TSP_Z_MM
	,
	TSP_Z_ALL
		= TSP_Z_SIGN
		| TSP_Z_UTC
		| TSP_Z_ELEMENTS
	,
};

enum : unsigned {
	BF_NONE						= 0,
	BF_S_ROOT					= 1 << 0,
	BF_TAGS						= 1 << 1,
	BF_S_CHILDREN				= 1 << 2,
	BF_S_EXPRESSION_SCOPE		= 1 << 3,
	BF_S_TAG_CHILDREN			= 1 << 4,
	BF_QUANTITY					= 1 << 5,
	BF_S_QUANTITY_COLLECTION	= 1 << 6,
	BF_EXPRESSION				= 1 << 7,
	BF_SINGLE_VALUE				= 1 << 8,

	BF_M_SCOPE
		= BF_S_ROOT
		| BF_S_CHILDREN
		| BF_S_EXPRESSION_SCOPE
		| BF_S_TAG_CHILDREN
		| BF_S_QUANTITY_COLLECTION
	,
	BF_M_EXPRESSION
		= BF_S_EXPRESSION_SCOPE
		| BF_EXPRESSION
	,

	BF_M_FILTER_NEWLINE
		= BF_M_SCOPE
	,
	BF_M_FILTER_COMPLETER
		= BF_M_SCOPE
		& ~BF_S_EXPRESSION_SCOPE
	,
	BF_M_FILTER_COMMENT
		= BF_M_SCOPE
	,
};

enum class Response {
	error,
	complete,
	eof,
	loopback_exit,
	jump,
	jump_sub,
	pass,
	next,
	next_gobble,
	exit,
	exit_sub,
};

struct ObjectParser;

using StageFunction = Response(ObjectParser&);

struct Stage {
	StringRef name;
	unsigned flags;
	StageFunction* enter;
	StageFunction* exit;
};


struct ObjectParser {
	struct Branch {
		Object* obj;
		Stage const** sequence_pos;
		bool any_part;
	};

	IReader& stream;
	ObjectParserInfo& info;
	Array<Branch> stack;
	Array<char> buffer;
	unsigned line;
	unsigned column;
	unsigned c;
	unsigned nc;
	unsigned flags;
	ParserBufferType buffer_type;
	unsigned hop_count;
	unsigned time_parts;
	struct {
		unsigned point;
	} currency_parts;
	Branch* branch;

	ObjectParser(IReader& stream, ObjectParserInfo& info, Allocator& allocator)
		: stream(stream)
		, info(info)
		, stack(allocator)
		, buffer(allocator)
	{}
};

#if defined(TOGO_COMPILER_CLANG) || \
	defined(TOGO_COMPILER_GCC)
	__attribute__((__format__ (__printf__, 2, 3)))
#endif
static bool parser_error(
	ObjectParser& p,
	char const* const format,
	...
) {
	va_list va;
	va_start(va, format);
	std::vsnprintf(p.info.message, array_extent(&ObjectParserInfo::message), format, va);
	va_end(va);
	p.info.line = p.line;
	p.info.column = p.column;
	p.flags |= PF_ERROR;
	return false;
}

#define PARSER_ERROR(p, format) \
	parser_error(p, "(@ %4d) " format " [last: %s]", __LINE__, p.branch ? (*p.branch->sequence_pos)->name.data : "none")

#define PARSER_ERRORF(p, format, ...) \
	parser_error(p, "(@ %4d) " format " [last: %s]", __LINE__, __VA_ARGS__, p.branch ? (*p.branch->sequence_pos)->name.data : "none")

#define PARSER_ERROR_EXPECTED(p, what) \
	PARSER_ERRORF(p, "expected %s, got '%c' (%02x)", what, static_cast<char>(p.c), p.c)

#define PARSER_ERROR_UNEXPECTED(p, what) \
	PARSER_ERRORF(p, "unexpected %s; got '%c' (%02x)", what, static_cast<char>(p.c), p.c)

#define PARSER_ERROR_STREAM(p, what) \
	PARSER_ERRORF(p, "%s: stream read failure", what)

inline static void parser_buffer_add(ObjectParser& p) {
	TOGO_DEBUG_ASSERTE(p.c != PC_EOF);
	array::push_back(p.buffer, static_cast<char>(p.c));
}

inline static void parser_buffer_clear(ObjectParser& p) {
	array::clear(p.buffer);
	p.buffer_type = PB_NONE;
}

inline static unsigned parser_buffer_size(ObjectParser const& p) {
	return static_cast<unsigned>(array::size(p.buffer));
}

inline static StringRef parser_buffer_ref(ObjectParser const& p) {
	return StringRef{array::begin(p.buffer), parser_buffer_size(p)};
}

inline static void parser_push(ObjectParser& p, Object& obj, Stage const** sequence_pos) {
	p.branch = &array::push_back(p.stack, {&obj, sequence_pos, false});
	// TOGO_LOGF("push: %2lu %s\n", array::size(p.stack), (*sequence_pos)->name.data);
}

inline static void parser_pop(ObjectParser& p) {
	TOGO_DEBUG_ASSERTE(array::any(p.stack));
	// TOGO_LOGF("pop : %2lu %s\n", array::size(p.stack), (*p.branch->sequence_pos)->name.data);
	array::pop_back(p.stack);
	p.branch = array::any(p.stack) ? &array::back(p.stack) : nullptr;
}

inline static unsigned parser_parent_flags(ObjectParser const& p) {
	TOGO_DEBUG_ASSERTE(array::size(p.stack) > 1);
	return (*p.stack[array::size(p.stack) - 2].sequence_pos)->flags;
}

static void parser_move_object_into_child(Object& obj) {
	auto& children = object::children(obj);
	{
	auto& last = array::push_back_inplace(children);
	auto name = obj.name;
	obj.name = {};
	object::copy(last, obj, false);
	obj.name = name;
	}
	// Copy children
	if (array::size(children) > 1) {
		auto last = end(children) - 1;
		auto& last_children = object::children(*last);
		for (auto it = begin(children); it != last; ++it) {
			array::push_back_inplace(last_children, rvalue_ref(*it));
		}
		array::remove_over(children, 0);
		array::resize(children, 1);
	}
	object::set_null(obj);
	object::clear_value_markers(obj);
	object::clear_source(obj);
	object::clear_tags(obj);
	object::release_quantity(obj);
}

static bool parser_next(ObjectParser& p) {
	if (p.flags & PF_CARRY) {
		p.flags &= ~PF_CARRY;
		return true;
	} else if (p.nc != PC_PEEK_EMPTY) {
		p.c = p.nc;
		p.nc = PC_PEEK_EMPTY;
		goto l_return;
	}

	{u8 c = '\0';
	do {} while (io::read_value(p.stream, c) && c == '\r');
	p.c = c;
	IOStatus const& status = io::status(p.stream);
	if (status.eof()) {
		p.c = PC_EOF;
	} else if (status.fail()) {
		return PARSER_ERROR_STREAM(p, "parser_next()");
	}}

l_return:
	if (p.c == '\n') {
		++p.line;
		p.column = 0;
	} else {
		++p.column;
	}
	return true;
}

static bool parser_peek(ObjectParser& p) {
	if (p.nc != PC_PEEK_EMPTY) {
		return p.nc;
	}
	u8 c = '\0';
	do {} while (io::read_value(p.stream, c) && c == '\r');
	p.nc = c;
	IOStatus const& status = io::status(p.stream);
	if (status.eof()) {
		p.nc = PC_EOF;
	} else if (status.fail()) {
		return PARSER_ERROR_STREAM(p, "parser_peek()");
	}
	return true;
}

inline static bool is_identifier_terminator(unsigned const c) {
	switch (c) {
	case PC_EOF:
	case '\t':
	case '\n':
	case ' ':
	case ',': case ';':
	case '=': case ':': case '$':
	case '}': case ']': case ')':
	case '{': case '[': case '(':
	case '+':
	case '\"': case '`':
	case '*': case '/':
	case '\\':
		return true;
	}
	return false;
}

inline static bool parser_is_currency_lead(ObjectParser& p) {
	// ¤ in UTF-8: C2 A4
	if (p.c == 0xC2) {
		return parser_peek(p) && p.nc == 0xA4;
	}
	return false;
}

inline static bool parser_is_identifier_lead(ObjectParser const& p) {
	return false
		|| (p.c >= 'a' && p.c <= 'z')
		|| (p.c >= 'A' && p.c <= 'Z')
		||  p.c == '_'
		||  p.c == '.'
		// UTF-8 sequence lead byte.. but not EOF
		|| (p.c >= 0xC0 && p.c != PC_EOF)
	;
}

inline static bool parser_is_number_lead(ObjectParser const& p) {
	return false
		|| (p.c >= '0' && p.c <= '9')
		||  p.c == '+'
		||  p.c == '-'
	;
}

static bool parser_skip_junk(ObjectParser& p, bool stage_exit) {
	auto flags = (*p.branch->sequence_pos)->flags;
	bool filter_newline = false;
	bool filter_completers = false;
	bool filter_comments = false;
	if (stage_exit) {
		filter_newline = flags & BF_M_FILTER_NEWLINE;
		filter_completers = flags & BF_M_FILTER_COMPLETER;
		filter_comments = flags & BF_M_FILTER_COMMENT;
	}

	do {
	switch (p.c) {
	default:
		return true;

	case '\n':
		if (!filter_newline) {
			return true;
		}

	case '\t':
	case ' ':
		break;

	case ',': case ';':
		if (!filter_completers) {
			return true;
		}
		break;

	case '\\':
		if (!filter_comments) {
			return true;
		}
		if (!parser_next(p)) {
			return false;
		}
		if (p.c == '\\') {
			while (parser_next(p)) {
				if (p.c == '\n' || p.c == PC_EOF) {
					goto l_continue;
				}
			}
			return false;
		} else if (p.c == '*') {
			bool head = false;
			bool tail = false;
			unsigned count = 1;
			while (parser_next(p)) {
				switch (p.c) {
				case PC_EOF:
					return PARSER_ERROR(p, "expected end of block comment, got EOF");

				case '*':
					if (head) {
						++count;
						head = false;
					} else {
						tail = true;
					}
					break;

				case '\\':
					if (tail) {
						--count;
						tail = false;
						if (count == 0) {
							goto l_continue;
						}
					} else {
						head = true;
					}
					break;

				default:
					head = false;
					tail = false;
					break;
				}
			}
			return false;
		} else if (p.c == PC_EOF) {
			return PARSER_ERROR(p, "expected '\\' or '*' to continue comment lead, got EOF");
		} else {
			return PARSER_ERROR_EXPECTED(p, "'\\' or '*' to continue comment lead");
		}
		break;
	}

	l_continue:
		(void)0;
	} while (parser_next(p));
	return false;
}

static bool parser_read_time(ObjectParser& p) {
	// day:
	//   DD
	// date:
	//   [YYYY-]MM-(day)
	// daytime:
	//   HH:MM[:SS]
	// zz (UTC zone offset):
	//   Z
	// zo (zone offset):
	//   (+|-)HH[:MM]
	// complete:
	//   (day)T[zo]
	//   (date)
	//   (day|date)(zz)
	//   (day|date)[T(zo)]
	//   [(day|date)T](daytime)[zz|zo]

	// init state from parser_read_number():
	//   YYYY- or MM-
	//   DDT or DDZ
	//   HH:

	if (!parser_peek(p)) {
		return false;
	}
	if ((p.nc < '0' || p.nc > '9') && p.nc != '-' && p.nc != '+') {
		if ((p.c == 'T' || p.c == 'Z') && is_identifier_terminator(p.nc)) {
			p.time_parts = TSP_D_DD | (p.c == 'T' ? TSP_T_MARKER : TSP_Z_UTC);
			p.buffer_type = PB_TIME;
			parser_next(p);
		} else if (p.c == '-') {
			parser_next(p);
			return PARSER_ERROR_EXPECTED(p, "date part following date lead");
		} else {
			// T/Z must be a unit
			// : must specify a tag
			p.buffer_type = PB_INTEGER;
		}
		return true;
	}

#define SET_PART(part) do { \
	parts |= part; last = now; \
} while (false)

#define SET_PART_ADD(part) do { \
	parser_buffer_add(p); \
	parts |= part; last = now + 1; \
} while (false)

	unsigned now;
	unsigned part_length;
	unsigned last = 0;
	unsigned parts = 0;
	p.buffer_type = PB_TIME;
	do {
		now = static_cast<unsigned>(parser_buffer_size(p));
		part_length = now - last;
		switch (p.c) {
		case '-':
			if (parts & TSP_Z_ALL) {
				return PARSER_ERROR_UNEXPECTED(p, "zone offset sign in zone offset segment");
			} else if (parts & TSP_T_ELEMENTS) {
				if (part_length != 2) {
					return PARSER_ERROR(p, "daytime part must be 2 numerals long");
				} else if (parts & TSP_T_MM) { // HH:MM:SS-
					SET_PART(TSP_T_SS);
				} else if (parts & TSP_T_HH) { // HH:MM-
					SET_PART(TSP_T_MM);
				}
				SET_PART_ADD(TSP_Z_SIGN);
			} else if (part_length == 0 && parts & TSP_T_MARKER) { // T-
				// no daytime part
				SET_PART_ADD(TSP_Z_SIGN);
			} else if (part_length == 4) { // YYYY-
				if (!parts) {
					SET_PART(TSP_D_YYYY);
				} else {
					return PARSER_ERROR_UNEXPECTED(p, "non-leading year in date segment");
				}
			} else if (part_length == 2) {
				if (~parts & TSP_D_MM) { // [YYYY-]MM-
					SET_PART(TSP_D_MM);
				} else { // YYYY-MM-DD-
					return PARSER_ERROR_UNEXPECTED(p, "date part separator after day in date segment");
				}
			} else {
				return PARSER_ERROR(p, "date part must be 4 (YYYY) or 2 (MM, DD) numerals long");
			}
			if (!parser_peek(p)) {
				return false;
			}
			if (p.nc < '0' || p.nc > '9') {
				parser_next(p);
				return PARSER_ERROR_EXPECTED(p, "numeral following separator");
			}
			break;

		case '+':
			if (parts & TSP_Z_ALL) {
				return PARSER_ERROR_UNEXPECTED(p, "zone offset sign in zone offset segment");
			} else if (parts & TSP_T_ELEMENTS) {
				if (part_length != 2) {
					return PARSER_ERROR(p, "daytime part must be 2 numerals long");
				} else if (parts & TSP_T_MM) { // HH:MM:SS+
					SET_PART(TSP_T_SS);
				} else if (parts & TSP_T_HH) { // HH:MM+
					SET_PART(TSP_T_MM);
				}
				SET_PART_ADD(TSP_Z_SIGN);
			} else if (part_length == 0 && parts & TSP_T_MARKER) { // T+
				// no daytime part
				SET_PART_ADD(TSP_Z_SIGN);
			} else { // [YYYY-][MM-]DD+
				return PARSER_ERROR_UNEXPECTED(p, "ambiguous positive sign immediately following date part");
			}
			break;

		case 'T':
			if (parts & TSP_T_ALL) {
				return PARSER_ERROR_UNEXPECTED(p, "daytime marker in daytime segment");
			} else if (part_length == 0) { // [YYYY-][MM][-]T
				return PARSER_ERROR(p, "day must be present in date segment for daytime segment to be specified");
			} else if (part_length != 2) {
				return PARSER_ERROR(p, "day must be 2 numerals long");
			} else if ((parts & (TSP_D_YYYY | TSP_D_MM)) == TSP_D_YYYY) { // YYYY-XXT
				return PARSER_ERROR(p, "day unspecified in date segment before daytime marker");
			} else { // [[YYYY-]MM-]DDT
				SET_PART(TSP_D_DD | TSP_T_MARKER);
			}
			break;

		case 'Z':
			if ((parts & TSP_T_ALL) == TSP_T_MARKER) { // TZ, TX...Z
				return PARSER_ERROR(p, "UTC zone offset marker following daytime segment must come after minute part");
			} else if (parts & TSP_Z_ALL) {
				return PARSER_ERROR_UNEXPECTED(p, "UTC zone offset marker in zone offset segment");
			} else if (part_length != 2) {
				if (parts & TSP_T_ALL) {
					return PARSER_ERROR(p, "daytime part must be 2 numerals long");
				} else {
					return PARSER_ERROR(p, "date part must be 2 numerals long (MM, DD)");
				}
			} else if (parts & TSP_T_MM) { // HH:MM:SSZ
				SET_PART(TSP_T_SS);
			} else if (parts & TSP_T_HH) { // HH:MMZ
				SET_PART(TSP_T_MM);
			} else if (parts & TSP_D_MM) { // [[YYYY-]MM-]DDZ
				SET_PART(TSP_D_DD);
			} else { // YYYY-Z or MM-Z
				return PARSER_ERROR(p, "day unspecified in date segment before UTC zone offset marker");
			}
			SET_PART(TSP_Z_UTC);
			p.time_parts = parts;
			return parser_next(p);

		case ':':
			if (parts && !(parts & ~TSP_D_ALL)) {
				return PARSER_ERROR_UNEXPECTED(p, "time part separator in date segment");
			} else if (part_length != 2) {
				return PARSER_ERROR(p, "time part must be 2 numerals long");
			} else if (parts & TSP_Z_ALL) {
				if (parts & TSP_Z_HH) { // HH:MM:
					SET_PART(TSP_Z_MM);
					// assume tag
					p.time_parts = parts;
					return true;
				} else { // HH:
					SET_PART(TSP_Z_HH);
				}
			} else if (parts & TSP_T_MM) { // HH:MM:SS:
				SET_PART(TSP_T_SS);
				// assume tag
				p.time_parts = parts;
				return true;
			} else if (parts & TSP_T_HH) { // HH:MM:
				SET_PART(TSP_T_MM);
			} else { // HH:
				SET_PART(TSP_T_HH);
			}
			if (!parser_peek(p)) {
				return false;
			}
			if (p.nc < '0' || p.nc > '9') {
				// assume tag
				p.time_parts = parts;
				return true;
			}
			break;

		case PC_EOF:
		case '\t':
		case '\n':
		case ' ':
		case ',': case ';':
		case '=': case '$':
		case '}': case ']': case ')':
		case '{': case '[': case '(':
		case '*': case '/':
		case '\\':
			if (part_length == 0) {
				if ((parts & TSP_Z_ALL) == TSP_Z_SIGN) {
					return PARSER_ERROR_EXPECTED(p, "time part after zone offset sign");
				}
				// complete
			} else if (part_length != 2) {
				return PARSER_ERROR(p, "trailing part must be 2 numerals long");
			} else if (parts & TSP_Z_ALL) {
				if (parts & TSP_Z_HH) { // (+|-)HH:MM
					SET_PART(TSP_Z_MM);
				} else { // (+|-)HH
					SET_PART(TSP_Z_HH);
				}
			} else if (parts & TSP_T_ALL) {
				if (parts & TSP_T_MM) { // HH:MM:SS
					SET_PART(TSP_T_SS);
				} else if (parts & TSP_T_HH) { // HH:MM
					SET_PART(TSP_T_MM);
				} else { // HH
					return PARSER_ERROR(p, "missing minute part in daytime segment");
				}
			} else {
				if (parts & TSP_D_MM) { // [YYYY-]MM-DD
					SET_PART(TSP_D_DD);
				} else /*if (parts & TSP_D_YYYY)*/ { // YYYY-XX
					return PARSER_ERROR(p, "missing day part in date segment");
				}
			}
			p.time_parts = parts;
			return true;

		default:
			if (p.c < '0' || p.c > '9') {
				return PARSER_ERROR_EXPECTED(p, "time part or terminator");
			} else {
				parser_buffer_add(p);
			}
			break;
		}
	} while (parser_next(p));
	return false;

#undef SET_PART
}

static bool parser_read_number(ObjectParser& p) {
	enum : unsigned {
		PART_SIGN				= 1 << 0,
		PART_NUMERAL			= 1 << 1,
		PART_DECIMAL			= 1 << 2,
		PART_DECIMAL_NUMERAL	= 1 << 3,
		PART_EXPONENT			= 1 << 4,
		PART_EXPONENT_SIGN		= 1 << 5,
		PART_EXPONENT_NUMERAL	= 1 << 6,
	};
	unsigned parts = 0;
	do {
		switch (p.c) {
		// Completers
		case PC_EOF:
		case '\t':
		case '\n':
		case ' ':
		case ',': case ';':
		case '=': case '$':
		case '}': case ']': case ')':
		case '{': case '[': case '(':
		case '*': case '/':
		case '\\':
			goto l_complete;

		case '-': // YYYY- or MM-
			if (parts == PART_NUMERAL && (parser_buffer_size(p) == 4 || parser_buffer_size(p) == 2)) {
				return parser_read_time(p);
			}

		case '+':
			if (
				parts & PART_EXPONENT_SIGN ||
				(~parts & PART_EXPONENT && parts & PART_SIGN)
			) {
				return PARSER_ERROR(p, "sign already specified for number part");
			} else if (parts & PART_EXPONENT_NUMERAL) {
				return PARSER_ERROR_UNEXPECTED(p, "non-leading sign in number exponent");
			}
			if (parts & PART_EXPONENT) {
				parts |= PART_EXPONENT_SIGN;
			} else if (parts == 0) {
				parts |= PART_SIGN;
			} else {
				return PARSER_ERROR_UNEXPECTED(p, "non-leading sign in number");
			}
			break;

		case '.':
			if (parts & PART_EXPONENT) {
				return PARSER_ERROR_UNEXPECTED(p, "decimal point in number exponent");
			} else if (parts & PART_DECIMAL) {
				return PARSER_ERROR(p, "decimal point in number specified twice");
			}
			parts |= PART_DECIMAL;
			break;

		case 'e': case 'E':
			if (parts & PART_EXPONENT) {
				return PARSER_ERROR(p, "exponent in number specified twice");
			}
			if (!parser_peek(p)) {
				return false;
			}
			if (
				(p.nc < '0' || p.nc > '9') &&
				p.nc != '-' && p.nc != '+' &&
				p.nc != '.'
			) {
				goto l_complete;
			}
			parts |= PART_EXPONENT;
			break;

		case ':': // HH:
		case 'T': // DDT
		case 'Z': // DDZ
			if (parts == PART_NUMERAL && parser_buffer_size(p) == 2) {
				return parser_read_time(p);
			} else {
				goto l_complete;
			}

		default:
			if (p.c >= '0' && p.c <= '9') {
				if (parts & PART_EXPONENT) {
					parts |= PART_EXPONENT_NUMERAL;
				} else if (parts & PART_DECIMAL) {
					parts |= PART_NUMERAL | PART_DECIMAL_NUMERAL;
				} else {
					parts |= PART_NUMERAL;
				}
			} else {
				goto l_complete;
			}
			break;
		}
		parser_buffer_add(p);
	} while (parser_next(p));
	return false;

l_complete:
	if (~parts & PART_NUMERAL) {
		return PARSER_ERROR(p, "missing numeral part in number");
	} else if (parts & PART_DECIMAL && ~parts & PART_DECIMAL_NUMERAL) {
		return PARSER_ERROR(p, "missing numeral part after decimal in number");
	} else if (parts & PART_EXPONENT && ~parts & PART_EXPONENT_NUMERAL) {
		return PARSER_ERROR(p, "missing numeral part after number exponent");
	}
	if (parts & (PART_DECIMAL | PART_EXPONENT)) {
		p.buffer_type = PB_DECIMAL;
	} else {
		p.buffer_type = PB_INTEGER;
	}
	return true;
}

static bool parser_read_identifier(
	ObjectParser& p,
	bool const conversion = true
) {
	do {
		if (is_identifier_terminator(p.c)) {
			p.buffer_type = PB_IDENTIFIER;
			if (conversion) {
				if (array::size(p.buffer) == 4) {
					if (std::memcmp(array::begin(p.buffer), "null", 4) == 0) {
						p.buffer_type = PB_NULL;
					} else if (std::memcmp(array::begin(p.buffer), "true", 4) == 0) {
						p.buffer_type = PB_TRUE;
					}
				} else if (array::size(p.buffer) == 5) {
					if (std::memcmp(array::begin(p.buffer), "false", 5) == 0) {
						p.buffer_type = PB_FALSE;
					}
				}
			}
			return true;
		}
		parser_buffer_add(p);
	} while (parser_next(p));
	return false;
}

static bool parser_read_currency(ObjectParser& p) {
	enum : unsigned {
		PART_SIGN				= 1 << 0,
		PART_NUMERAL			= 1 << 1,
		PART_DECIMAL			= 1 << 2,
		PART_DECIMAL_NUMERAL	= 1 << 3,
	};
	// skip lead sign (¤)
	if (!(parser_next(p) && parser_next(p))) {
		return false;
	}
	unsigned parts = 0;
	do {
		switch (p.c) {
		// Completers
		case PC_EOF:
		case '\t':
		case '\n':
		case ' ':
		case ',': case ';':
		case '=': case ':': case '$':
		case '}': case ']': case ')':
		case '{': case '[': case '(':
		case '*': case '/':
		case '\\':
			return PARSER_ERROR_EXPECTED(p, "currency value part or currency unit");

		case '-':
		case '+':
			if (parts & PART_SIGN) {
				return PARSER_ERROR(p, "sign already specified for currency value");
			} else if (parts != 0) {
				return PARSER_ERROR_UNEXPECTED(p, "non-leading sign in currency value");
			}
			parts |= PART_SIGN;
			break;

		case '.':
			if (parts & PART_DECIMAL) {
				return PARSER_ERROR(p, "decimal point already specified for currency value");
			}
			parts |= PART_DECIMAL;
			p.currency_parts.point = parser_buffer_size(p);
			break;

		default:
			if (p.c >= '0' && p.c <= '9') {
				parts |= (parts & PART_DECIMAL)
					? PART_DECIMAL_NUMERAL
					: PART_NUMERAL
				;
			} else {
				goto l_complete;
			}
			break;
		}
		parser_buffer_add(p);
	} while (parser_next(p));
	return false;

l_complete:
	if (~parts & PART_NUMERAL) {
		return PARSER_ERROR(p, "missing numeral part in currency value");
	} else if (parts & PART_DECIMAL && ~parts & PART_DECIMAL_NUMERAL) {
		return PARSER_ERROR(p, "missing numeral part after decimal in currency value");
	}
	if (~parts & PART_DECIMAL) {
		p.currency_parts.point = parser_buffer_size(p);
	}
	p.buffer_type = PB_CURRENCY;
	return true;
}

static bool parser_read_string_quote(ObjectParser& p) {
	bool escaped = false;
	while (parser_next(p)) {
		switch (p.c) {
		case PC_EOF:
			return PARSER_ERROR(p, "expected completer for double-quote bounded string, got EOF");

		case '\n':
			return PARSER_ERROR(p, "unexpected newline in double-quote bounded string");

		case '"':
			if (!escaped) {
				if (!parser_next(p)) {
					return false;
				}
				p.buffer_type = PB_STRING;
				return true;
			}
			break;

		case '\\':
			escaped = true;
			continue;
		}
		if (escaped) {
			switch (p.c) {
			case 't': p.c = '\t'; break;
			case 'n': p.c = '\n'; break;
			}
			escaped = false;
		}
		parser_buffer_add(p);
	}
	return false;
}

static bool parser_read_string_block(ObjectParser& p) {
	unsigned count = 1;
	while (parser_next(p)) {
		if (p.c == '`') {
			if (++count == 3) {
				goto l_parse;
			}
		} else {
			return PARSER_ERROR(p, "incomplete lead block-quote for expected block-quote bounded string");
		}
	}
	return false;

l_parse:
	count = 0;
	while (parser_next(p)) {
		switch (p.c) {
		case PC_EOF:
			return PARSER_ERROR(p, "expected completer for block-quote bounded string, got EOF");

		case '`':
			if (++count == 3) {
				if (!parser_next(p)) {
					return false;
				}
				p.buffer_type = PB_STRING;
				array::resize(p.buffer, array::size(p.buffer) - 2);
				return true;
			}
			break;

		default:
			count = 0;
			break;
		}
		parser_buffer_add(p);
	}
	return false;
}

static bool parser_read_uncertainty_marker(ObjectParser& p) {
	// FIXME: legacy: multiple '?' markers in a row were valid
	parser_buffer_add(p);
	while (parser_next(p)) {
		if (p.c != '?') {
			p.buffer_type = PB_MARKER_UNCERTAINTY;
			return true;
		}
		parser_buffer_add(p);
	}
	return false;
}

static bool parser_read_guess_marker(ObjectParser& p) {
	parser_buffer_add(p);
	if (!parser_next(p)) {
		return false;
	}
	if (p.c != '~') {
		return parser_read_identifier(p);
	} else {
		if (!parser_next(p)) {
			return false;
		}
	}
	p.buffer_type = PB_MARKER_GUESS;
	return true;
}

static bool parser_read_approximation_marker(ObjectParser& p, unsigned const lead) {
	unsigned count = 1;
	parser_buffer_add(p);
	while (parser_next(p)) {
		if (p.c == lead) {
			if (++count > 3) {
				return PARSER_ERROR(p, "max approximation marker count exceeded");
			}
		} else {
			p.buffer_type = PB_MARKER_APPROXIMATION;
			array::resize(p.buffer, count);
			return true;
		}
	}
	return false;
}

static bool parser_read_source(ObjectParser& p) {
	bool uncertainty_marker = false;
	bool id = false;
	do {
		if (p.c == '?') {
			if (id) {
				return PARSER_ERROR_UNEXPECTED(p, "uncertainty marker after source ID part");
			} else if (!uncertainty_marker) {
				parser_buffer_add(p);
				uncertainty_marker = true;
			}
		} else if (p.c >= '0' && p.c <= '9') {
			parser_buffer_add(p);
			id = true;
		} else if (uncertainty_marker || id) {
			p.buffer_type = PB_SOURCE;
			return true;
		} else {
			return PARSER_ERROR_EXPECTED(p, "(\"?\" [0-9]*) in source");
		}
	} while (parser_next(p));
	return false;
}

static void parser_apply(ObjectParser& p, ApplyBufferAs const apply_as = ApplyBufferAs::value) {
	/*TOGO_LOGF(
		"apply %u, [%.*s]\n",
		unsigned_cast(p.buffer_type),
		parser_buffer_size(p),
		array::begin(p.buffer)
	);*/
	TOGO_DEBUG_ASSERTE(p.buffer_type != PB_NONE);
	auto& obj = *p.branch->obj;
	switch (apply_as) {
	case ApplyBufferAs::value:
		switch (p.buffer_type) {
		case PB_NONE:
		case PB_SOURCE:
			TOGO_DEBUG_ASSERTE(false);
			break;

		case PB_MARKER_UNCERTAINTY:
			object::set_value_certain(obj, false);
			break;

		case PB_MARKER_GUESS:
			object::set_value_guess(obj, true);
			break;

		case PB_MARKER_APPROXIMATION: {
			signed value = signed_cast(array::size(p.buffer));
			value = p.buffer[0] == '~' ? -value : value;
			object::set_value_approximation(obj, value);
		}	break;

		case PB_NULL:
			object::set_null(obj);
			break;

		case PB_FALSE:
			object::set_boolean(obj, false);
			break;

		case PB_TRUE:
			object::set_boolean(obj, true);
			break;

		case PB_INTEGER:
			array::push_back(p.buffer, '\0');
			object::set_integer(obj, parse_s64(array::begin(p.buffer), 10));
			break;

		case PB_DECIMAL:
			array::push_back(p.buffer, '\0');
			object::set_decimal(obj, parse_f64(array::begin(p.buffer)));
			break;

		case PB_STRING:
			object::set_string(obj, parser_buffer_ref(p));
			break;

		case PB_IDENTIFIER:
			object::set_identifier(obj, parser_buffer_ref(p));
			break;

		case PB_TIME: {
			bool has_date = false;
			bool has_clock = false;
			Date date{};
			signed h, m, s;
			h = m = s = 0;

			object::set_time_value(obj, {});
			object::set_zoned(obj, false);
			auto it = begin(parser_buffer_ref(p));
			switch (p.time_parts & TSP_D_ELEMENTS) {
			case TSP_D_YYYY | TSP_D_MM | TSP_D_DD:
				date.year = parse_integer_unsigned(it, it + 4);
			case TSP_D_MM | TSP_D_DD:
				date.month = parse_integer_unsigned(it, it + 2);
			case TSP_D_DD:
				date.day = parse_integer_unsigned(it, it + 2);
				has_date = true;
			}
			if (~p.time_parts & TSP_D_MM) {
				object::set_month_contextual(obj, true);
			} else if (~p.time_parts & TSP_D_YYYY) {
				object::set_year_contextual(obj, true);
			}
			if (p.time_parts & TSP_T_ELEMENTS) {
				h = parse_integer_unsigned(it, it + 2);
				m = parse_integer_unsigned(it, it + 2);
				if (p.time_parts & TSP_T_SS) {
					s = parse_integer_unsigned(it, it + 2);
				}
				has_clock = true;
			}
			if (has_date && has_clock) {
				object::set_time_type(obj, ObjectTimeType::date_and_clock);
				time::gregorian::set_utc(object::time_value(obj), date, h, m, s);
			} else if (has_date) {
				object::set_time_type(obj, ObjectTimeType::date);
				time::gregorian::set_utc(object::time_value(obj), date);
			} else if (has_clock) {
				object::set_time_type(obj, ObjectTimeType::clock);
				time::set_utc(object::time_value(obj), h, m, s);
			}
			if (p.time_parts & TSP_Z_UTC) {
				object::set_zoned(obj, true);
			} else if (p.time_parts & TSP_Z_ELEMENTS) {
				bool is_negative = false;
				if (p.time_parts & TSP_Z_SIGN) {
					is_negative = *it++ == '-';
				}
				object::set_zoned(obj, true);
				h = parse_integer_unsigned(it, it + 2);
				if (p.time_parts & TSP_Z_MM) {
					m = parse_integer_unsigned(it, it + 2);
				} else {
					m = 0;
				}
				time::adjust_zone_clock(object::time_value(obj), is_negative ? -h : h, m);
			}
			TOGO_DEBUG_ASSERTE(it == end(parser_buffer_ref(p)));
		}	break;

		case PB_CURRENCY: {
			auto head = begin(parser_buffer_ref(p));
			auto it = head;
			bool negative = *it == '-';
			if (negative || *it == '+') {
				++it;
			}
			u64 value = parse_integer_unsigned(it, head + p.currency_parts.point);
			s32 exponent = parser_buffer_size(p) - p.currency_parts.point;
			if (exponent > 0) {
				if (--exponent > 0) {
					if (value > 0) {
						value *= pow_int(10, exponent);
					}
					++it;
					value += parse_integer_unsigned(it, it + exponent);
				}
			}
			object::set_currency(
				obj,
				negative ? -static_cast<s64>(value) : static_cast<s64>(value),
				exponent,
				""
			);
		}	break;
		}
		break;

	case ApplyBufferAs::name:
		object::set_name(obj, parser_buffer_ref(p));
		break;

	case ApplyBufferAs::unit:
		object::set_unit(obj, parser_buffer_ref(p));
		break;

	// FIXME: ugh
	case ApplyBufferAs::source: {
		TOGO_DEBUG_ASSERTE(p.buffer_type == PB_SOURCE);
		bool uncertain = p.buffer[0] == '?';
		object::set_source_certain(obj, !uncertain);
		if (!uncertain || array::size(p.buffer) > 1) {
			array::push_back(p.buffer, '\0');
			auto id = static_cast<unsigned>(parse_s64(array::begin(p.buffer) + uncertain, 10));
			obj.source = static_cast<u16>(min(id, 0xFFFFu));
		}
	}	break;

	case ApplyBufferAs::sub_source: {
		TOGO_DEBUG_ASSERTE(p.buffer_type == PB_SOURCE);
		bool uncertain = p.buffer[0] == '?';
		object::set_sub_source_certain(obj, !uncertain);
		if (!uncertain || array::size(p.buffer) > 1) {
			array::push_back(p.buffer, '\0');
			auto id = static_cast<unsigned>(parse_s64(array::begin(p.buffer) + uncertain, 10));
			obj.sub_source = static_cast<u16>(min(id, 0xFFFFu));
		}
	}	break;
	}
	parser_buffer_clear(p);
	p.branch->any_part = true;
}

extern Stage const
* sequence_root[],
* sequence_root_single_value[],
* sequence_base[],
** jump_base_unnamed,
** jump_base_after_marker_guess,
** jump_base_after_value,
** jump_base_expression_scope,
** jump_base_quantity,
** jump_base_complete,

* sequence_tag[],
** jump_tag_after_name,

* sub_assign,
* sub_unit,
* sub_sub_source,
* sub_quantity_children,
* sub_expression,
* sub_expression_scope_lead
;

#define STAGE(name, flags, enter, exit) \
	Stage const name { \
		#name, flags, enter, exit \
	}

#define RESP(resp) do { /*TOGO_LOGF_TRACED("RESP(%-9s)  ", #resp);*/ return Response::resp; } while (false)
#define RESP_IF(expr, resp) if (expr) { RESP(resp); }
#define RESP_IF_ELSE(expr, t_resp, f_resp) RESP_IF(expr, t_resp) else { RESP(f_resp); }

#define RESP_SEQ(resp, seq_pos) do { /*TOGO_LOGF_TRACED("SEQ(%s)\n", #seq_pos);*/ p.branch->sequence_pos = seq_pos; RESP(resp); } while (false)
#define RESP_SEQ_IF(expr, resp, seq_pos) if (expr) { RESP_SEQ(resp, seq_pos); }

STAGE(stage_root, BF_S_ROOT,
nullptr,
[](ObjectParser& p) -> Response {
	if (p.c == PC_EOF) {
		RESP(eof);
	} else {
		parser_push(p, array::push_back_inplace(object::children(*p.branch->obj)), sequence_base);
		RESP(jump);
	}
});

// To skip lead junk
STAGE(stage_sv_leader, BF_S_ROOT | BF_SINGLE_VALUE,
nullptr,
[](ObjectParser& /*p*/) -> Response {
	RESP(next);
});

STAGE(stage_sv, BF_S_ROOT | BF_SINGLE_VALUE,
[](ObjectParser& p) -> Response {
	RESP_IF(p.c == PC_EOF, eof)
	else {
		parser_push(p, *p.branch->obj, sequence_base);
		RESP(jump);
	}
},
[](ObjectParser& p) -> Response {
	RESP_IF(p.c == PC_EOF, eof)
	else {
		PARSER_ERROR_EXPECTED(p, "EOF (single-value parse)");
		RESP(error);
	}
});

STAGE(stage_lead, BF_NONE,
[](ObjectParser& p) -> Response {
	switch (p.c) {
	case ',': case ';':
		PARSER_ERROR_EXPECTED(p, "object part");
		RESP(error);

	case 'G':
		// might be a guess marker
		RESP_IF(!parser_read_guess_marker(p), error)
		if (p.buffer_type == PB_MARKER_GUESS) {
			parser_apply(p);
			RESP_SEQ(jump, jump_base_after_marker_guess);
		}
		break;

	default:
		if (parser_is_identifier_lead(p) && !parser_is_currency_lead(p)) {
			RESP_IF(!parser_read_identifier(p), error)
		} else {
			RESP(pass);
		}
		break;
	}

	if (p.buffer_type == PB_IDENTIFIER) {
		// expecting assignment or some right-hand part
		RESP_SEQ(jump_sub, &sub_assign);
	} else { // null, true, false
		parser_apply(p);
		RESP_SEQ(jump, jump_base_after_value);
	}
},
nullptr
);

STAGE(stage_assign, BF_NONE,
[](ObjectParser& p) -> Response {
	if (p.c == '=') {
		parser_apply(p, ApplyBufferAs::name);
		// expecting right-hand part, but might not get it!
		RESP(exit_sub);
	} else {
		parser_apply(p);
		RESP_SEQ(jump, jump_base_after_value);
	}
},
[](ObjectParser& p) -> Response {
	switch (p.c) {
	case PC_EOF:
	case '*': case '/':
	case '}': case ')': case ']':
	case ',': case ';': case '\n':
		PARSER_ERROR_EXPECTED(p, "right-hand part following assignment");
		RESP(error);
	}
	RESP(next);
});

STAGE(stage_marker_uncertainty, BF_NONE,
[](ObjectParser& p) -> Response {
	if (p.c == '?') {
		RESP_IF(!parser_read_uncertainty_marker(p), error)
		else {
			parser_apply(p);
			RESP(next);
		}
	}
	RESP(pass);
},
nullptr
);

STAGE(stage_marker_guess, BF_NONE,
[](ObjectParser& p) -> Response {
	RESP_IF(p.c != 'G', pass)
	else RESP_IF(!parser_read_guess_marker(p), error)
	else if (!object::value_certain(*p.branch->obj)) {
		PARSER_ERROR(p, "lead marker '?' was already met; guess marker invalid here");
		RESP(error);
	} else if (p.buffer_type != PB_MARKER_GUESS) {
		parser_apply(p);
		RESP_SEQ(jump, jump_base_after_value);
	} else {
		parser_apply(p);
		RESP(next);
	}
},
nullptr
);

STAGE(stage_tag_marker_guess, BF_NONE,
[](ObjectParser& p) -> Response {
	RESP_IF(p.c != 'G', pass)
	else RESP_IF(!parser_read_guess_marker(p), error)
	else if (!object::value_certain(*p.branch->obj)) {
		PARSER_ERROR(p, "lead marker '?' was already met; guess marker invalid here");
		RESP(error);
	} else if (p.buffer_type == PB_MARKER_GUESS) {
		parser_apply(p);
		RESP(next);
	} else {
		parser_apply(p, ApplyBufferAs::name);
		RESP_SEQ(jump, jump_tag_after_name);
	}
},
nullptr
);

STAGE(stage_marker_approximation, BF_NONE,
[](ObjectParser& p) -> Response {
	if (p.c == '~' || p.c == '^') {
		RESP_IF(!parser_read_approximation_marker(p, p.c), error)
		else {
			parser_apply(p);
			RESP(next);
		}
	}
	RESP(pass);
},
nullptr
);

STAGE(stage_value, BF_NONE,
[](ObjectParser& p) -> Response {
	bool is_unit_carrier = false;
	if (p.c == '\"') {
		parser_read_string_quote(p);
	} else if (p.c == '`') {
		parser_read_string_block(p);
	} else if (parser_is_currency_lead(p)) {
		parser_read_currency(p);
		is_unit_carrier = true;
	} else if (parser_is_identifier_lead(p)) {
		parser_read_identifier(p);
	} else if (parser_is_number_lead(p)) {
		parser_read_number(p);
		is_unit_carrier = p.buffer_type == PB_INTEGER || p.buffer_type == PB_DECIMAL;
	} else {
		RESP(pass);
	}
	RESP_IF(p.flags & PF_ERROR, error)
	else {
		parser_apply(p);
		RESP_SEQ_IF(is_unit_carrier, jump_sub, &sub_unit)
		else RESP(next);
	}
},
nullptr
);

STAGE(stage_unit, BF_NONE,
[](ObjectParser& p) -> Response {
	if (parser_is_identifier_lead(p)) {
		RESP_IF(!parser_read_identifier(p, false), error)
		else {
			parser_apply(p, ApplyBufferAs::unit);
			RESP(next);
		}
	} else if (object::is_currency(*p.branch->obj)) {
		PARSER_ERROR_EXPECTED(p, "unit following currency value");
		RESP(error);
	}
	RESP(pass);
},
nullptr
);

STAGE(stage_typed_string, BF_NONE,
[](ObjectParser& p) -> Response {
	auto& obj = *p.branch->obj;
	RESP_IF(!object::is_identifier(obj), pass)
	else if (p.c == '\"') {
		parser_read_string_quote(p);
	} else if (p.c == '`') {
		parser_read_string_block(p);
	} else {
		RESP(pass);
	}
	RESP_IF(p.flags & PF_ERROR, error)
	else {
		auto type = obj.value.identifier;
		obj.value.identifier = {};
		parser_apply(p);
		obj.value.string.type = type;
		RESP(next);
	}
},
nullptr
);

STAGE(stage_source, BF_NONE,
[](ObjectParser& p) -> Response {
	RESP_IF_ELSE(p.c == '$', exit, pass)
},
[](ObjectParser& p) -> Response {
	RESP_IF(!parser_read_source(p), error)
	else {
		parser_apply(p, ApplyBufferAs::source);
		RESP_SEQ(jump_sub, &sub_sub_source);
	}
});

STAGE(stage_sub_source, BF_NONE,
[](ObjectParser& p) -> Response {
	RESP_IF_ELSE(p.c == '$', exit_sub, next)
},
[](ObjectParser& p) -> Response {
	RESP_IF(!parser_read_source(p), error)
	else {
		parser_apply(p, ApplyBufferAs::sub_source);
		// resume sequence_base
		RESP(next);
	}
});

STAGE(stage_tags, BF_TAGS,
[](ObjectParser& p) -> Response {
	if (p.c == ':') {
		p.flags |= PF_CARRY;
		RESP(exit);
	} else {
		RESP(pass);
	}
},
[](ObjectParser& p) -> Response {
	RESP_IF(p.c != ':', next)

	RESP_IF(!parser_next(p), error)
	else {
		parser_push(p, array::push_back_inplace(object::tags(*p.branch->obj)), sequence_tag);
		RESP(jump);
	}
});

STAGE(stage_children, BF_S_CHILDREN,
[](ObjectParser& p) -> Response {
	RESP_IF_ELSE(p.c == '{', exit, pass)
},
[](ObjectParser& p) -> Response {
	switch (p.c) {
	case '}':
		// hop over stage_expression_scope
		p.hop_count = 1;
		RESP(next_gobble);

	case PC_EOF:
		PARSER_ERROR(p, "expected sub-object or block close, got EOF");
		RESP(error);

	case ')': case ']':
		PARSER_ERRORF(p, "unbalanced block close: '%c'", static_cast<char>(p.c));
		RESP(error);

	default:
		parser_push(p, array::push_back_inplace(object::children(*p.branch->obj)), sequence_base);
		RESP(jump);
	}
});

// Wacky business ahead. Using the exit sequence of a sub-stage to read a lead
// value (if any) while skipping junk.
STAGE(stage_expression_scope, BF_S_EXPRESSION_SCOPE,
[](ObjectParser& p) -> Response {
	RESP_IF(p.c != '(', pass);

	if (!object::is_null(*p.branch->obj)) {
		PARSER_ERROR_EXPECTED(p, "scoped expression is a value (must not have preceding value)");
		RESP(error);
	}
	object::set_expression(*p.branch->obj);
	RESP_SEQ(exit_sub, &sub_expression_scope_lead);
},
[](ObjectParser& p) -> Response {
	auto op = ObjectOperator::none;
	switch (p.c) {
	case PC_EOF:
		PARSER_ERROR(p, "expected sub-object or block close, got EOF");
		RESP(error);

	case '}': case ']':
		PARSER_ERRORF(p, "unbalanced block close: '%c'", static_cast<char>(p.c));
		RESP(error);

	case ')':
		RESP(next_gobble);

	case '+': op = ObjectOperator::add; goto l_op;
	case '-': op = ObjectOperator::sub; goto l_op;
	case '*': op = ObjectOperator::mul; goto l_op;
	case '/': op = ObjectOperator::div; goto l_op;

	default:
		break;

	l_op:
		RESP_IF(!parser_next(p), error)
		else {
			auto& obj = array::push_back_inplace(object::children(*p.branch->obj));
			object::set_op(obj, op);
			parser_push(p, obj, jump_base_unnamed);
			RESP(jump);
		}
	}
	PARSER_ERROR_EXPECTED(p, "operator or scope terminator");
	RESP(error);
});

STAGE(stage_expression_scope_lead, BF_S_EXPRESSION_SCOPE,
nullptr,
[](ObjectParser& p) -> Response {
	RESP_SEQ_IF(p.c == ')', loopback_exit, jump_base_expression_scope)
	else {
		// hack: return to stage_expression_scope on Response::complete
		p.branch->sequence_pos = jump_base_expression_scope;
		auto& lead = array::push_back_inplace(object::children(*p.branch->obj));
		object::set_op(lead, ObjectOperator::none);
		parser_push(p, lead, jump_base_unnamed);
		RESP(jump);
	}
});

STAGE(stage_tag_name, BF_NONE,
[](ObjectParser& p) -> Response {
	if (parser_is_identifier_lead(p)) {
		RESP_IF(!parser_read_identifier(p), error)
		else {
			parser_apply(p, ApplyBufferAs::name);
			RESP(next);
		}
	}
	PARSER_ERROR_EXPECTED(p, "tag name");
	RESP(error);
},
nullptr
);

STAGE(stage_tag_children, BF_S_TAG_CHILDREN,
[](ObjectParser& p) -> Response {
	RESP_IF_ELSE(p.c == '(', exit, pass);
},
[](ObjectParser& p) -> Response {
	switch (p.c) {
	case ')':
		RESP(complete);

	case PC_EOF:
		PARSER_ERROR(p, "expected sub-object or block close, got EOF");
		RESP(error);

	case '}': case ']':
		PARSER_ERRORF(p, "unbalanced block close: '%c'", static_cast<char>(p.c));
		RESP(error);

	default:
		parser_push(p, array::push_back_inplace(object::children(*p.branch->obj)), sequence_base);
		RESP(jump);
	}
});

STAGE(stage_quantity, BF_QUANTITY,
[](ObjectParser& p) -> Response {
	RESP_IF(p.c != '[', pass)

	RESP_IF(!parser_next(p), error)
	RESP_IF(p.c == ']', next_gobble)
	else {
		// hack: nothing keeps track of the current position since we haven't reached exit
		p.branch->sequence_pos = jump_base_quantity;
		parser_push(p, object::make_quantity(*p.branch->obj), sequence_base);
		RESP(jump);
	}
},
[](ObjectParser& p) -> Response {
	switch (p.c) {
	case PC_EOF:
		PARSER_ERROR(p, "expected sub-object or block close, got EOF");
		RESP(error);

	case '}': case ')':
		PARSER_ERRORF(p, "unbalanced block close: '%c'", static_cast<char>(p.c));
		RESP(error);

	case ']':
		RESP(next_gobble);

	case ',': case ';': case '\n':
		parser_push(p, *object::quantity(*p.branch->obj), &sub_quantity_children);
		RESP(jump);
	}
	// FIXME: Can this ever be reached?
	RESP(error);
});

STAGE(stage_quantity_children, BF_S_QUANTITY_COLLECTION,
[](ObjectParser& p) -> Response {
	if (p.c == ']') {
		PARSER_ERROR_EXPECTED(p, "sub-object");
		RESP(error);
	}
	parser_move_object_into_child(*p.branch->obj);
	p.flags |= PF_CARRY;
	RESP(exit);
},
[](ObjectParser& p) -> Response {
	switch (p.c) {
	case PC_EOF:
		PARSER_ERROR(p, "expected sub-object or block close, got EOF");
		RESP(error);

	case '}': case ')':
		PARSER_ERRORF(p, "unbalanced block close: '%c'", static_cast<char>(p.c));
		RESP(error);

	case ']':
		p.flags |= PF_CARRY;
		RESP(complete);

	default:
		parser_push(p, array::push_back_inplace(object::children(*p.branch->obj)), sequence_base);
		RESP(jump);
	}
});

STAGE(stage_tag_complete, BF_NONE,
[](ObjectParser& p) -> Response {
	p.flags |= PF_CARRY;
	RESP(complete);
},
nullptr
);

STAGE(stage_complete, BF_NONE,
[](ObjectParser& p) -> Response {
	if (!p.branch->any_part) {
		PARSER_ERROR_EXPECTED(p, "object part");
		RESP(error);
	}
	switch (p.c) {
	case '+': case '-':
	case '*': case '/': {
		auto flags = parser_parent_flags(p);
		if (flags & BF_M_EXPRESSION) {
			// do nothing (common case)
		} else if (flags & (BF_SINGLE_VALUE | BF_QUANTITY)) {
			parser_pop(p);
			auto& obj = *((flags & BF_QUANTITY) ? object::quantity(*p.branch->obj) : p.branch->obj);
			parser_move_object_into_child(obj);
			object::set_expression(obj);
			object::set_op(array::back(object::children(obj)), ObjectOperator::none);
			parser_push(p, obj, &sub_expression);
			RESP(jump);
		} else {
			parser_pop(p);
			auto& scope = object::children(*p.branch->obj);
			auto& shim = array::push_back_inplace(scope);
			auto lead = end(scope) - 2;
			object::set_expression(shim);
			shim.name = lead->name;
			lead->name = {};
			object::set_op(*lead, ObjectOperator::none);
			array::push_back_inplace(object::children(shim), rvalue_ref(*lead));
			array::remove_over(scope, lead);
			parser_push(p, array::back(scope), &sub_expression);
			RESP(jump);
		}
	}

	case PC_EOF:
	case '\\':
	case ':':
	case '}': case ')': case ']':
	case ',': case ';': case '\n':
		p.flags |= PF_CARRY;
		RESP(complete);
	}
	PARSER_ERROR_EXPECTED(p, "object part or terminator");
	RESP(error);
},
nullptr
);

STAGE(stage_expression, BF_EXPRESSION,
[](ObjectParser& p) -> Response {
	p.flags |= PF_CARRY;
	RESP(exit);
},
[](ObjectParser& p) -> Response {
	auto op = ObjectOperator::none;
	switch (p.c) {
	case PC_EOF:
	case '\\':
	case '}': case ')': case ']':
	case ',': case ';': case '\n':
		/*if (array::size(object::children(*p.branch->obj)) == 1) {
			PARSER_ERROR_EXPECTED(p, "operand following operator");
			RESP(error);
		}*/
		p.flags |= PF_CARRY;
		RESP(complete);

	case '+': op = ObjectOperator::add; goto l_op;
	case '-': op = ObjectOperator::sub; goto l_op;
	case '*': op = ObjectOperator::mul; goto l_op;
	case '/': op = ObjectOperator::div; goto l_op;

	default:
		break;

	l_op:
		RESP_IF(!parser_next(p), error)
		else {
			auto& obj = array::push_back_inplace(object::children(*p.branch->obj));
			object::set_op(obj, op);
			parser_push(p, obj, jump_base_unnamed);
			RESP(jump);
		}
	}
	PARSER_ERROR_EXPECTED(p, "operator or terminator");
	RESP(error);
});

STAGE(stage_sequence_end, BF_NONE,
[](ObjectParser& p) -> Response {
	(void)p;
	RESP(error);
},
[](ObjectParser& p) -> Response {
	(void)p;
	RESP(error);
}
);

#undef STAGE

#undef RESP
#undef RESP_IF
#undef RESP_IF_ELSE

#undef RESP_SEQ
#undef RESP_SEQ_IF

inline static constexpr Stage const**
find_stage(Stage const** i, Stage const* stage) {
	return
		*i == stage
			? i
		: *i == &stage_sequence_end
			? throw "stage not found"
		: find_stage(i + 1, stage)
	;
}

Stage const
* sequence_root[]{
	&stage_root,
	&stage_sequence_end,
},
* sequence_root_single_value[]{
	&stage_sv_leader,
	&stage_sv,
	&stage_sequence_end,
},
* sequence_base[]{
	&stage_lead, // -> stage_assign
	&stage_marker_uncertainty,
	&stage_marker_guess,
	&stage_marker_approximation,
	&stage_value, // -> stage_unit
	&stage_typed_string,
	&stage_source, // -> stage_sub_source
	&stage_tags, // pre
	&stage_children,
	&stage_expression_scope, // -> stage_expression_scope_lead
	&stage_tags, // post
	&stage_quantity, // -> stage_quantity_children
	&stage_complete,
	&stage_sequence_end,
},
** jump_base_unnamed = find_stage(sequence_base, &stage_lead) + 1,
** jump_base_after_marker_guess = find_stage(sequence_base, &stage_marker_uncertainty) + 1,
** jump_base_after_value = find_stage(sequence_base, &stage_value) + 1,
** jump_base_expression_scope = find_stage(sequence_base, &stage_expression_scope),
** jump_base_quantity = find_stage(sequence_base, &stage_quantity),
** jump_base_complete = find_stage(sequence_base, &stage_complete),

* sequence_tag[]{
	&stage_marker_uncertainty,
	&stage_tag_marker_guess,
	&stage_marker_approximation,
	&stage_tag_name,
	&stage_tag_children,
	&stage_tag_complete,
	&stage_sequence_end,
},
** jump_tag_after_name = find_stage(sequence_tag, &stage_tag_name) + 1,

* sub_assign = &stage_assign,
* sub_unit = &stage_unit,
* sub_sub_source = &stage_sub_source,
* sub_quantity_children = &stage_quantity_children,
* sub_expression = &stage_expression,
* sub_expression_scope_lead = &stage_expression_scope_lead
;

static void parser_init(ObjectParser& p, Object& root, bool single_value) {
	p.info.line = 0;
	p.info.column = 0;
	p.info.message[0] = '\0';

	p.line = 1;
	p.column = 0;
	p.c = PC_EOF;
	p.nc = PC_PEEK_EMPTY;
	p.flags = PF_NONE;
	p.buffer_type  = PB_NONE;
	p.hop_count = 0;
	p.time_parts = 0;
	p.currency_parts = {};
	p.branch = nullptr;
	array::clear(p.stack);
	array::clear(p.buffer);
	parser_push(p, root, single_value ? sequence_root_single_value : sequence_root);
}

static bool parser_read(ObjectParser& p) {
	enum StagePart : unsigned {
		enter,
		exit,
	};

	// NB: last stage in a sequence will always error
	Stage const** base_pos = p.branch->sequence_pos;
	StagePart stage_part = StagePart::exit;
	Response response = Response::error;

l_exec:
	p.hop_count = 0;
	if (!parser_next(p)) {
		return false;
	}
	if (!parser_skip_junk(p, stage_part == StagePart::exit)) {
		return false;
	}
	if (object::source_line(*p.branch->obj) == 0) {
		object::set_source_line(*p.branch->obj, p.line);
	}

	switch (stage_part) {
	case StagePart::enter: response = (*p.branch->sequence_pos)->enter(p); break;
	case StagePart::exit : response = (*p.branch->sequence_pos)->exit(p); break;
	}
	/*char c = static_cast<char>(p.c);
	TOGO_LOGF("FROM(%-26s) [%.*s %02x] <%.*s>\n",
		(*base_pos)->name.data,
		p.c == PC_EOF ? 3 : 1,
		p.c == PC_EOF ? "EOF" : &c,
		unsigned_cast(p.c),
		static_cast<unsigned>(array::size(p.buffer)),
		array::begin(p.buffer)
	);*/

	p.branch->any_part |= unsigned_cast(response) > unsigned_cast(Response::pass);
	switch (response) {
	case Response::error:
		if (~p.flags & PF_ERROR) {
			PARSER_ERROR(p, "unknown error");
		}
		return false;

	case Response::complete:
		parser_pop(p);
		base_pos = p.branch->sequence_pos;
		stage_part = StagePart::exit;
		goto l_exec;

	case Response::eof:
		return true;

	case Response::loopback_exit:
		stage_part = StagePart::exit;
		p.flags |= PF_CARRY;
		goto l_exec;

	case Response::jump:
		base_pos = p.branch->sequence_pos;

	case Response::jump_sub:
		stage_part = StagePart::enter;
		p.flags |= PF_CARRY;
		goto l_exec;

	case Response::pass:
	case Response::next:
		p.flags |= PF_CARRY;

	case Response::next_gobble:
		p.branch->sequence_pos = base_pos += (p.hop_count + 1);
		stage_part = StagePart::enter;
		goto l_exec;

	case Response::exit:
		base_pos = p.branch->sequence_pos;

	case Response::exit_sub:
		TOGO_DEBUG_ASSERTE(stage_part == StagePart::enter);
		stage_part = StagePart::exit;
		goto l_exec;
	}
}

} // anonymous namespace

} // namespace object
} // namespace quanta
