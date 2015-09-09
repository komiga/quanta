#line 2 "quanta/core/object/io/parser.ipp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <togo/core/error/assert.hpp>
#include <togo/core/collection/array.hpp>
#include <togo/core/io/types.hpp>
#include <togo/core/io/io.hpp>

#include <quanta/core/object/object.hpp>

#include <cstdlib>
#include <cstdio>
#include <cstring>

namespace quanta {
namespace object {

namespace {

enum : unsigned {
	PC_EOF = ~0u,
};

enum : unsigned {
	PF_NONE				= 0,
	PF_ERROR			= 1 << 0, // error met
	PF_CARRY			= 1 << 1, // carry the current parser char forward
	PF_HELD				= 1 << 2, // held identifier, waiting for equality sign or sub part
	PF_ASSIGN			= 1 << 3, // found equality sign and set name
	PF_TAG_LEAD			= 1 << 4, // found colon, expecting name
	PF_SOURCE			= 1 << 5, // found equality sign and set name
	PF_SOURCE_PARTIAL	= 1 << 6, // found equality sign and set name
	PF_ACCEPT_UNIT		= 1 << 7, // numeric value preceded (can accept subsequent identifier as unit)
};

enum ParserBufferType : unsigned {
	PB_NONE,
	PB_MARKER_UNCERTAINTY,
	PB_MARKER_GUESS,
	PB_MARKER_APPROXIMATION,
	PB_NULL,
	PB_TRUE,
	PB_FALSE,
	PB_INTEGER,
	PB_DECIMAL,
	PB_IDENTIFIER,
	PB_STRING,
};

enum : unsigned {
	PN_NONE								= 0,
	PN_TAG								= 1 << 0, // is a tag (parent has PN_S_TAGS)

	// segments
	PN_S_NAME							= 1 << 1, // name part (left side; right side following)
	PN_S_MARKER_UNCERTAINTY_OR_GUESS	= 1 << 2, // ? or G~
	PN_S_MARKER_APPROXIMATION			= 1 << 3, // approximation marker
	PN_S_VALUE							= 1 << 4, // value
	PN_S_UNIT							= 1 << 5, // numeric unit
	PN_S_SOURCE							= 1 << 6, // source
	PN_S_SUB_SOURCE						= 1 << 7, // sub source
	PN_S_TAGS							= 1 << 8, // pre-children tags
	PN_S_CHILDREN						= 1 << 9, // children
	PN_S_QUANTITY						= 1 << 10, // quantity

	// [16,31]
	PN_CURRENT_SCOPE_SHIFT = 16u,
	PN_CURRENT_SCOPE_MASK = ((1u << PN_CURRENT_SCOPE_SHIFT) - 1) << PN_CURRENT_SCOPE_SHIFT,

	PN_M_PAST_SOURCES
		= PN_S_TAGS
		| PN_S_CHILDREN
		| PN_S_QUANTITY
	,
};

struct ObjectParser {
	struct Branch {
		Object* obj;
		unsigned flags;
	};

	Object& root;
	IReader& stream;
	ObjectParserInfo& info;
	Array<Branch> stack;
	Array<char> buffer;
	unsigned line;
	unsigned column;
	unsigned c;
	unsigned flags;
	ParserBufferType buffer_type;

	Branch* branch;
};

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
	parser_error(p, "(@ %4d) " format, __LINE__)

#define PARSER_ERRORF(p, format, ...) \
	parser_error(p, "(@ %4d) " format, __LINE__, __VA_ARGS__)

#define PARSER_ERROR_EXPECTED(p, what) \
	PARSER_ERRORF(p, "expected %s, got '%c'", what, p.c)

#define PARSER_ERROR_UNEXPECTED(p, what) \
	PARSER_ERRORF(p, "unexpected %s: '%c'", what, p.c)

#define PARSER_ERROR_STREAM(p, what) \
	PARSER_ERRORF(p, "%s: stream read failure", what)

inline static bool parser_is_identifier_lead(ObjectParser const& p) {
	return false
		|| (p.c >= 'a' && p.c <= 'z')
		|| (p.c >= 'A' && p.c <= 'Z')
		||  p.c == '_'
		||  p.c == '.'
		// UTF-8 sequence lead byte
		||  p.c >= 0xC0
	;
}

inline static bool parser_is_number_lead(ObjectParser const& p) {
	return false
		|| (p.c >= '0' && p.c <= '9')
		||  p.c == '-'
		||  p.c == '+'
		// ||  p.c == '.'
	;
}

inline static unsigned parser_branch_scope(ObjectParser::Branch const& branch) {
	return (branch.flags & PN_CURRENT_SCOPE_MASK) >> PN_CURRENT_SCOPE_SHIFT;
}

inline static unsigned parser_scope(ObjectParser const& p) {
	return parser_branch_scope(*p.branch);
}

inline static bool parser_scope_is_open(ObjectParser const& p) {
	return p.branch->flags & PN_CURRENT_SCOPE_MASK;
}

static void parser_buffer_add(ObjectParser& p) {
	TOGO_DEBUG_ASSERTE(p.c != PC_EOF);
	array::push_back(p.buffer, static_cast<char>(p.c));
}

inline static void parser_buffer_clear(ObjectParser& p) {
	array::clear(p.buffer);
	p.buffer_type = PB_NONE;
}

inline static StringRef parser_buffer_ref(ObjectParser const& p) {
	return StringRef{
		array::begin(p.buffer),
		static_cast<unsigned>(array::size(p.buffer))
	};
}

inline static void parser_pop(ObjectParser& p) {
	TOGO_DEBUG_ASSERTE(array::size(p.stack) > 1);
	array::pop_back(p.stack);
	p.branch = &array::back(p.stack);
	p.flags &= ~(PF_ASSIGN | PF_TAG_LEAD | PF_SOURCE | PF_SOURCE_PARTIAL | PF_ACCEPT_UNIT);
}

inline static void parser_push(ObjectParser& p, Object& object, unsigned const flags) {
	p.branch = &array::push_back(p.stack, {&object, flags});
}

static bool parser_next(ObjectParser& p) {
	if (p.flags & PF_CARRY) {
		p.flags &= ~PF_CARRY;
		return true;
	}
	char c = '\0';
	do {} while (io::read_value(p.stream, c) && c == '\r');
	p.c = c;
	IOStatus const& status = io::status(p.stream);
	if (status.eof()) {
		p.c = PC_EOF;
	} else if (status.fail()) {
		return false;
	}
	if (p.c == '\n') {
		++p.line;
		p.column = 0;
	} else {
		++p.column;
	}
	return true;
}

static bool parser_skip_whitespace(ObjectParser& p, bool const newline) {
	while (p.c == ' ' || p.c == '\t' || (newline && p.c == '\n')) {
		if (!parser_next(p)) {
			return false;
		}
	}
	return true;
}

static bool parser_skip_comment(ObjectParser& p) {
	if (!parser_next(p)) {
		return false;
	}
	if (p.c == '/') {
		while (parser_next(p)) {
			if (p.c == '\n' || p.c == PC_EOF) {
				return true;
			}
		}
		return PARSER_ERROR_STREAM(p, "in comment");
	} else if (p.c == '*') {
		bool tail = false;
		while (parser_next(p)) {
			switch (p.c) {
			case PC_EOF:
				return PARSER_ERROR(p, "expected end of block comment, got EOF");

			case '*':
				tail = true;
				break;

			case '/':
				if (tail) {
					return true;
				}
				break;

			default: tail = false; break;
			}
		}
		return PARSER_ERROR_STREAM(p, "in comment block");
	} else if (p.c == PC_EOF) {
		return PARSER_ERROR(p, "expected '/' or '*' to continue comment lead, got EOF");
	} else {
		return PARSER_ERROR_EXPECTED(p, "'/' or '*' to continue comment lead");
	}
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
		case ':': case '$':
		case '}': case ']': case ')':
		case '{': case '[': case '(':
		case '/':
			goto l_assign_value;

		case '-': case '+':
			if (
				parts & PART_EXPONENT_SIGN ||
				(~parts & PART_EXPONENT && parts & PART_SIGN)
			) {
				return PARSER_ERROR(p, "sign already specified for number part");
			} else if (parts & PART_EXPONENT_NUMERAL) {
				return PARSER_ERROR(p, "unexpected non-leading sign in number exponent");
			}
			if (parts & PART_EXPONENT) {
				parts |= PART_EXPONENT_SIGN;
			} else {
				parts |= PART_SIGN;
			}
			break;

		case '.':
			if (parts & PART_EXPONENT) {
				return PARSER_ERROR(p, "unexpected decimal point in number exponent");
			} else if (parts & PART_DECIMAL) {
				return PARSER_ERROR(p, "decimal point in number specified twice");
			}
			parts |= PART_DECIMAL;
			break;

		case 'e': case 'E':
			if (parts & PART_EXPONENT) {
				return PARSER_ERROR(p, "exponent in number specified twice");
			}
			parts |= PART_EXPONENT;
			break;

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
				goto l_assign_value;
			}
			break;
		}
		parser_buffer_add(p);
	} while (parser_next(p));
	return PARSER_ERROR_STREAM(p, "in number");

l_assign_value:
	p.flags |= PF_CARRY;
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

static bool parser_read_identifier(ObjectParser& p) {
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
		case '/':
			p.flags |= PF_CARRY;
			p.buffer_type = PB_IDENTIFIER;
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
			return true;

		case '\\':
		case '\'':
		case '"':
		case '`':
			return PARSER_ERROR_UNEXPECTED(p, "symbol in identifier");
		}
		parser_buffer_add(p);
	} while (parser_next(p));
	return PARSER_ERROR_STREAM(p, "in identifier");
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
	return PARSER_ERROR_STREAM(p, "in double-quote bounded string");
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
	goto l_stream_error;

l_parse:
	count = 0;
	while (parser_next(p)) {
		switch (p.c) {
		case PC_EOF:
			return PARSER_ERROR(p, "expected completer for block-quote bounded string, got EOF");

		case '`':
			if (++count == 3) {
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

l_stream_error:
	return PARSER_ERROR_STREAM(p, "in block-quote bounded string");
}

static bool parser_read_uncertainty_marker(ObjectParser& p) {
	// FIXME: legacy: multiple '?' markers in a row were valid
	parser_buffer_add(p);
	while (parser_next(p)) {
		if (p.c != '?') {
			p.flags |= PF_CARRY;
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
			parser_buffer_add(p);
		} else {
			p.flags |= PF_CARRY;
			p.buffer_type = PB_MARKER_APPROXIMATION;
			return true;
		}
	}
	return false;
}

inline static bool parser_read_approximation_marker_less(ObjectParser& p) {
	return parser_read_approximation_marker(p, '~');
}

inline static bool parser_read_approximation_marker_more(ObjectParser& p) {
	return parser_read_approximation_marker(p, '^');
}

static bool parser_read_source_id(ObjectParser& p) {
	do {
		if (p.c >= '0' && p.c <= '9') {
			parser_buffer_add(p);
		} else {
			p.flags |= PF_CARRY;
			p.buffer_type = PB_INTEGER;
			return true;
		}
	} while (parser_next(p));
	return false;
}

static void parser_push_new(ObjectParser& p) {
	Object* obj = nullptr;
	// TODO: set object flag if tag is pre/post
	switch (parser_scope(p) & ~PN_TAG) {
	case PN_S_TAGS:
		obj = &array::push_back_inplace(object::tags(*p.branch->obj));
		break;
	case PN_S_CHILDREN:
		obj = &array::push_back_inplace(object::children(*p.branch->obj));
		break;
	case PN_S_QUANTITY:
		obj = &object::make_quantity(*p.branch->obj);
		break;
	}
	TOGO_DEBUG_ASSERTE(obj);
	parser_push(p, *obj, PN_NONE);
}

static void parser_apply(ObjectParser& p) {
	TOGO_DEBUG_ASSERTE(p.buffer_type != PB_NONE);
	auto& obj = *p.branch->obj;
	if (p.flags & PF_TAG_LEAD) {
		p.flags &= ~PF_TAG_LEAD;
		p.branch->flags |= PN_TAG | PN_S_NAME;
		object::set_name(obj, parser_buffer_ref(p));
	} else if ((~p.branch->flags & PN_S_NAME) && (p.flags & PF_ASSIGN)) {
		p.flags &= ~PF_ASSIGN;
		p.branch->flags |= PN_S_NAME;
		object::set_name(obj, parser_buffer_ref(p));
	} else if (p.flags & PF_ACCEPT_UNIT) {
		p.flags &= ~PF_ACCEPT_UNIT;
		p.branch->flags |= PN_S_UNIT;
		object::set_unit(obj, parser_buffer_ref(p));
	} else if (p.flags & PF_SOURCE) {
		if (p.buffer_type == PB_MARKER_UNCERTAINTY) {
			if (~p.branch->flags & PN_S_SOURCE) {
				object::set_source_certain(obj, false);
			} else {
				object::set_sub_source_certain(obj, false);
			}
			p.flags |= PF_SOURCE_PARTIAL;
		} else if (p.buffer_type == PB_INTEGER) {
			array::push_back(p.buffer, '\0');
			auto id = unsigned_cast(parse_s64(array::begin(p.buffer), 10));
			// NB: assigning directly because the interface wipes uncertainty
			if (~p.branch->flags & PN_S_SOURCE) {
				obj.source = id;
				p.branch->flags |= PN_S_SOURCE;
			} else {
				if (obj.source != 0) {
					obj.sub_source = id;
				}
				p.branch->flags |= PN_S_SUB_SOURCE;
			}
			p.flags &= ~PF_SOURCE;
		} else {
			TOGO_DEBUG_ASSERTE(false);
		}
	} else {
		p.flags &= ~(PF_ASSIGN | PF_SOURCE_PARTIAL);
		p.branch->flags |= PN_S_VALUE;
		switch (p.buffer_type) {
		case PB_NONE:
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

		case PB_TRUE: // fall-through
		case PB_FALSE:
			object::set_boolean(obj, p.buffer_type == PB_TRUE);
			break;

		case PB_INTEGER:
			array::push_back(p.buffer, '\0');
			object::set_integer(obj, parse_s64(array::begin(p.buffer), 10));
			p.flags |= PF_ACCEPT_UNIT;
			break;

		case PB_DECIMAL:
			array::push_back(p.buffer, '\0');
			object::set_decimal(obj, parse_f64(array::begin(p.buffer)));
			p.flags |= PF_ACCEPT_UNIT;
			break;

		case PB_IDENTIFIER:
		case PB_STRING:
			object::set_string(obj, parser_buffer_ref(p));
			break;
		}
	}
	parser_buffer_clear(p);
}

static void parser_unhold(ObjectParser& p) {
	TOGO_DEBUG_ASSERTE((p.flags & PF_HELD) && p.buffer_type == PB_IDENTIFIER);
	p.flags &= ~PF_HELD;
	parser_push_new(p);
	parser_apply(p);
}

static bool parser_open_scope(ObjectParser& p, unsigned segment) {
	TOGO_DEBUG_ASSERTE(segment & (PN_S_TAGS | PN_S_CHILDREN | PN_S_QUANTITY));
	if ((p.branch->flags & PN_TAG) && (~segment & PN_TAG)) {
		parser_pop(p);
	}
	auto scope = parser_scope(p);
	if (!scope) {
		if (p.branch->flags & (~segment & PN_TAG)) {
			return PARSER_ERROR_UNEXPECTED(p, "block was already defined");
		} else if (segment == PN_S_QUANTITY && (p.branch->flags & ~(PN_TAG | PN_S_QUANTITY))) {
			return PARSER_ERROR_UNEXPECTED(p, "quantity block without value part");
		}
	} else if (p.flags & PF_HELD) {
		parser_unhold(p);
	} else if (segment == PN_S_QUANTITY) {
		return PARSER_ERROR_UNEXPECTED(p, "quantity block without value part");
	} else {
		parser_push_new(p);
	}
	p.branch->flags &= ~PN_CURRENT_SCOPE_MASK;
	p.branch->flags |= (segment & ~PN_TAG) | (segment << PN_CURRENT_SCOPE_SHIFT);
	return true;
}

static bool parser_close_scope(ObjectParser& p, unsigned segment) {
	if (p.flags & PF_HELD) {
		parser_unhold(p);
		parser_pop(p);
	}
	if (parser_scope(p) == segment) {
		// nothing to do; clears current scope
	} else if (parser_scope(p)) {
		return PARSER_ERROR_UNEXPECTED(p, "scope close in different scope");
	} else if (array::size(p.stack) == 1) {
		return PARSER_ERROR_UNEXPECTED(p, "scope close at root");
	/*} else if (parser_branch_scope(p.stack[array::size(p.stack) - 2]) != segment) {
		return PARSER_ERROR_UNEXPECTED(p, "unmatched scope close");*/
	} else {
		if (p.branch->flags & PN_TAG) {
			parser_pop(p);
		}
		parser_pop(p);
		if (parser_scope(p) != segment) {
			return PARSER_ERROR_UNEXPECTED(p, "unmatched scope close");
		}
	}
	p.branch->flags &= ~PN_CURRENT_SCOPE_MASK;
	return true;
}

static void parser_read_value_part(ObjectParser& p) {
	if (p.flags & PF_HELD) {
		if (p.flags & PF_ASSIGN) {
			parser_unhold(p);
		} else {
			PARSER_ERROR_UNEXPECTED(p, "value part following identifier (wanted sub parts or equality sign)");
			return;
		}
	}
	bool (*func_read)(ObjectParser&) = nullptr;
	switch (p.c) {
	case '?':
		if (p.branch->flags & PN_S_MARKER_UNCERTAINTY_OR_GUESS) {
			PARSER_ERROR(p, "a lead marker (? or G~) was already met");
		} else {
			func_read = parser_read_uncertainty_marker;
		}
		break;

	case 'G':
		if (p.branch->flags & PN_S_MARKER_UNCERTAINTY_OR_GUESS) {
			PARSER_ERROR(p, "a lead marker (? or G~) was already met");
		} else {
			func_read = parser_read_guess_marker;
		}
		break;

	case '~':
		if (p.branch->flags & PN_S_MARKER_APPROXIMATION) {
			PARSER_ERROR(p, "approximation marker was already met");
		} else {
			func_read = parser_read_approximation_marker_less;
		}
		break;

	case '^':
		if (p.branch->flags & PN_S_MARKER_APPROXIMATION) {
			PARSER_ERROR(p, "approximation marker was already met");
		} else {
			func_read = parser_read_approximation_marker_more;
		}
		break;

	case '"':
		if (p.flags & PF_TAG_LEAD) {
			PARSER_ERROR_EXPECTED(p, "tag name after ':' (identifier)");
		} else {
			func_read = parser_read_string_quote;
		}
		break;

	case '`':
		if (p.flags & PF_TAG_LEAD) {
			PARSER_ERROR_EXPECTED(p, "tag name after ':' (identifier)");
		} else {
			func_read = parser_read_string_block;
		}
		break;

	default:
		if (parser_is_identifier_lead(p)) {
			func_read = parser_read_identifier;
		} else if (parser_is_number_lead(p)) {
			if (p.flags & PF_TAG_LEAD) {
				PARSER_ERROR_EXPECTED(p, "tag name after ':' (identifier)");
			} else {
				func_read = parser_read_number;
			}
		} else {
			PARSER_ERROR_EXPECTED(p, "value");
		}
		break;
	}
	if (!func_read) {
		return;
	}
	if (p.flags & PF_SOURCE) {
		if (!(p.c == '?' || (p.c >= '0' && p.c <= '9'))) {
			PARSER_ERROR_EXPECTED(p, "uncertainty marker and/or source ID after '$' ('?' or unsigned number)");
			return;
		} else if (p.c != '?') {
			func_read = parser_read_source_id;
		}
	}
	if (func_read(p)) {
		if (
			p.buffer_type == PB_IDENTIFIER &&
			!(p.flags & (PF_TAG_LEAD | PF_ACCEPT_UNIT)) &&
			parser_scope_is_open(p)
		) {
			/*TOGO_LOG_DEBUGF(
				"held identifier: '%.*s'\n",
				static_cast<unsigned>(array::size(p.buffer)),
				array::begin(p.buffer)
			);*/
			p.flags |= PF_HELD;
		} else {
			if (parser_scope_is_open(p)) {
				parser_push_new(p);
			}
			parser_apply(p);
		}
	}
}

static bool parser_step(ObjectParser& p) {
#define CHECK_UNEXPECTED_TERMINATION											\
	if (p.flags & PF_ASSIGN) {													\
		PARSER_ERROR_EXPECTED(p, "value part after '='");						\
	} else if (p.flags & PF_TAG_LEAD) {											\
		PARSER_ERROR_EXPECTED(p, "tag name after ':' (identifier)");			\
	}

	parser_next(p);
	if (!parser_skip_whitespace(
		p, !(p.flags & (PF_HELD | PF_ASSIGN | PF_TAG_LEAD)) && parser_scope_is_open(p)
	)) {
		return false;
	}
	switch (p.c) {
	case PC_EOF:
		if (p.flags & PF_ASSIGN) {
			PARSER_ERROR(p, "expected value after '=', got EOF");
		} else if (p.flags & PF_TAG_LEAD) {
			PARSER_ERROR(p, "expected tag name after ':' (identifier), got EOF");
		} else if (
			(p.flags & (PF_SOURCE | PF_SOURCE_PARTIAL)) == PF_SOURCE &&
			p.buffer_type == PB_NONE
		) {
			PARSER_ERROR(p, "expected source ID or uncertainty marker after '$' ('?' or unsigned number), got EOF");
		} else if (array::size(p.stack) > 1 && parser_scope_is_open(p)) {
			PARSER_ERRORF(p, "%u unclosed scope(s) at EOF", array::size(p.stack) - 1);
		} else {
			if (p.flags & PF_HELD) {
				parser_unhold(p);
			} else if (p.buffer_type != PB_NONE) {
				parser_apply(p);
			}
			if (array::size(p.stack) > 1) {
				parser_pop(p);
			}
		}
		return false;

	case '/':
		// TODO: allow single-line block comment between any part of the expression
		CHECK_UNEXPECTED_TERMINATION else {
			if (p.flags & PF_HELD) {
				parser_unhold(p);
			}
			parser_skip_comment(p);
		}
		break;

	case '=':
		if (~p.flags & PF_HELD) {
			PARSER_ERROR_EXPECTED(p, "preceding name (identifier)");
		} else CHECK_UNEXPECTED_TERMINATION else {
			p.flags |= PF_ASSIGN;
		}
		break;

	case ':':
		if (p.flags & PF_TAG_LEAD) {
			PARSER_ERROR_EXPECTED(p, "tag name after ':' (identifier)");
		} else if (p.branch->flags & PN_S_QUANTITY) {
			PARSER_ERROR_EXPECTED(p, "end of object after quantity scope");
		} else {
			if (p.branch->flags & PN_TAG) {
				parser_pop(p);
			} else {
				parser_open_scope(p, PN_S_TAGS);
			}
			parser_push_new(p);
			p.flags |= PF_TAG_LEAD;
		}
		break;

	case '$':
		if (p.flags & PF_HELD) {
			parser_unhold(p);
			p.flags |= PF_SOURCE;
		} else if (parser_scope_is_open(p)) {
			PARSER_ERROR(p, "source cannot be defined here");
		} else if (p.branch->flags & PN_S_SUB_SOURCE) {
			PARSER_ERROR(p, "sources have already been defined");
		} else CHECK_UNEXPECTED_TERMINATION else {
			// FIXME: ugh
			if (p.flags & PF_SOURCE_PARTIAL) {
				p.branch->flags |= ((~p.branch->flags & PN_S_SOURCE) ? PN_S_SOURCE : PN_S_SUB_SOURCE);
			}
			p.flags &= ~PF_SOURCE_PARTIAL;
			p.flags |= PF_SOURCE;
		}
		break;

	case '{':
		if (p.flags & PF_TAG_LEAD) {
			PARSER_ERROR_EXPECTED(p, "tag name after ':' (identifier)");
		} else {
			parser_open_scope(p, PN_S_CHILDREN);
		}
		break;

	case '(':
		if (p.flags & PF_TAG_LEAD) {
			PARSER_ERROR_EXPECTED(p, "tag name after ':' (identifier)");
		} else if (~p.branch->flags & PN_TAG) {
			PARSER_ERROR_UNEXPECTED(p, "tag child opener outside of tag");
		} else {
			parser_open_scope(p, PN_S_CHILDREN | PN_TAG);
		}
		break;

	case '[':
		if (p.flags & PF_TAG_LEAD) {
			PARSER_ERROR_EXPECTED(p, "tag name after ':' (identifier)");
		} else if (p.branch->flags & PN_S_QUANTITY) {
			PARSER_ERROR_UNEXPECTED(p, "quantity opener after block has been opened");
		} else {
			parser_open_scope(p, PN_S_QUANTITY);
		}
		break;
 
	case '}':
		CHECK_UNEXPECTED_TERMINATION else {
			parser_close_scope(p, PN_S_CHILDREN);
		}
		break;

	case ')':
		CHECK_UNEXPECTED_TERMINATION else {
			parser_close_scope(p, PN_S_CHILDREN | PN_TAG);
		}
		break;

	case ']':
		CHECK_UNEXPECTED_TERMINATION else {
			parser_close_scope(p, PN_S_QUANTITY);
		}
		break;

	case ',':
	case ';':
		// TODO: morph quantity object if in PN_S_QUANTITY

	case '\n':
		CHECK_UNEXPECTED_TERMINATION else {
			if (p.flags & PF_HELD) {
				parser_unhold(p);
			}
			parser_pop(p);
		}
		break;

	default:
		parser_read_value_part(p);
		break;
	}
#undef CHECK_UNEXPECTED_TERMINATION

	return !(p.flags & PF_ERROR);
}

} // anonymous namespace

} // namespace object
} // namespace quanta
