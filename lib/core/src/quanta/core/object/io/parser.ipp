#line 2 "quanta/core/object/io/parser.ipp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>
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
	PB_IDENTIFIER,
	PB_STRING,
};

enum class ApplyBufferAs : unsigned {
	value,
	name,
	unit,
	source,
	sub_source,
};

enum : unsigned {
	BF_NONE					= 0,
	BF_S_ROOT				= 1 << 0,
	BF_S_TAGS				= 1 << 1,
	BF_S_CHILDREN			= 1 << 2,
	BF_S_TAG_CHILDREN		= 1 << 3,
	BF_S_QUANTITY			= 1 << 4,

	BF_M_SCOPE
		= BF_S_ROOT
		| BF_S_TAGS
		| BF_S_CHILDREN
		| BF_S_TAG_CHILDREN
		| BF_S_QUANTITY
	,
};

enum class Response {
	next,
	pass,
	jump,
	jump_sub,
	exit,
	exit_sub,
	complete,
	error,
	eof,
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
	};

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

	ObjectParser(IReader& stream, ObjectParserInfo& info, Allocator& allocator)
		: stream(stream)
		, info(info)
		, stack(allocator)
		, buffer(allocator)
	{}
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
	PARSER_ERRORF(p, "expected %s, got '%c' (%x)", what, static_cast<char>(p.c), p.c)

#define PARSER_ERROR_UNEXPECTED(p, what) \
	PARSER_ERRORF(p, "unexpected %s; got '%c' (%x)", what, static_cast<char>(p.c), p.c)

#define PARSER_ERROR_STREAM(p, what) \
	PARSER_ERRORF(p, "%s: stream read failure", what)

inline static bool parser_is_identifier_lead(ObjectParser const& p) {
	return false
		|| (p.c >= 'a' && p.c <= 'z')
		|| (p.c >= 'A' && p.c <= 'Z')
		||  p.c == '_'
		||  p.c == '.'
		// UTF-8 sequence lead byte.. but not EOF
		||  (p.c >= 0xC0 && p.c != PC_EOF)
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

inline static void parser_push(ObjectParser& p, Object& obj, Stage const** sequence_pos) {
	p.branch = &array::push_back(p.stack, {&obj, sequence_pos});
	// TOGO_LOGF("push: %2lu %s\n", array::size(p.stack), (*sequence_pos)->name.data);
}

inline static void parser_pop(ObjectParser& p) {
	TOGO_DEBUG_ASSERTE(array::any(p.stack));
	// TOGO_LOGF("pop : %2lu %s\n", array::size(p.stack), (*p.branch->sequence_pos)->name.data);
	array::pop_back(p.stack);
	p.branch = array::any(p.stack) ? &array::back(p.stack) : nullptr;
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

static bool parser_skip_junk(ObjectParser& p, bool const filter_completers) {
	do {
		switch (p.c) {
		default:
			return true;

		case '\n':
		case ',': case ';':
			if (!filter_completers) {
				return true;
			}

		case '\t':
		case ' ':
			break;

		case '/':
			if (!parser_next(p)) {
				return false;
			}
			if (p.c == '/') {
				while (parser_next(p)) {
					if (p.c == '\n' || p.c == PC_EOF) {
						goto l_continue;
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
							goto l_continue;
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
			break;
		}
		l_continue:
		(void)0;
	} while (parser_next(p));
	return false;
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
		case '=': case ':': case '$':
		case '}': case ']': case ')':
		case '{': case '[': case '(':
		case '/':
			goto l_complete;

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
				goto l_complete;
			}
			break;
		}
		parser_buffer_add(p);
	} while (parser_next(p));
	return PARSER_ERROR_STREAM(p, "in number");

l_complete:
	// p.flags |= PF_CARRY;
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
		case PC_EOF:
		case '\t':
		case '\n':
		case ' ':
		case ',': case ';':
		case '=': case ':': case '$':
		case '}': case ']': case ')':
		case '{': case '[': case '(':
		case '/':
			// p.flags |= PF_CARRY;
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
				if (!parser_next(p)) {
					goto l_stream_error;
				}
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

l_stream_error:
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
				if (!parser_next(p)) {
					goto l_stream_error;
				}
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
			// p.flags |= PF_CARRY;
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
			parser_buffer_add(p);
		} else {
			// p.flags |= PF_CARRY;
			p.buffer_type = PB_MARKER_APPROXIMATION;
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
			// p.flags |= PF_CARRY;
			p.buffer_type = PB_SOURCE;
			return true;
		} else {
			return PARSER_ERROR_EXPECTED(p, "(\"?\" [0-9]*) in source");
		}
	} while (parser_next(p));
	return false;
}

static void parser_apply(ObjectParser& p, ApplyBufferAs const apply_as = ApplyBufferAs::value) {
	// TOGO_LOGF("apply %u\n", unsigned_cast(p.buffer_type));
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

		case PB_IDENTIFIER:
		case PB_STRING:
			object::set_string(obj, parser_buffer_ref(p));
			break;
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
}

extern Stage const
* sequence_root[],
* sequence_base[],
** jump_base_after_marker_uncertainty,
** jump_base_after_value,

* sequence_tag[],
** jump_tag_after_name,

// * sub_tentative_child
* sub_assign,
* sub_unit,
* sub_sub_source
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
		parser_pop(p);
		RESP(eof);
	} else {
		parser_push(p, array::push_back_inplace(object::children(*p.branch->obj)), sequence_base);
		RESP_SEQ(jump, sequence_base);
	}
});

/*STAGE(stage_tentative_child, BF_NONE,
[](ObjectParser& p) -> Response {
	switch (p.c) {
	case ',': case ';':
		PARSER_ERROR_EXPECTED(p, "sub-object");
		RESP(error);
	}
	parser_push(p, array::push_back_inplace(object::children(*p.branch->obj)), sequence_base);
	RESP_SEQ(jump, sequence_base);
},
nullptr
);*/

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
			RESP_SEQ(jump, jump_base_after_marker_uncertainty);
		}
		break;

	default:
		if (parser_is_identifier_lead(p)) {
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
	case '}': case ')': case ']':
	case ',': case ';': case '\n':
		PARSER_ERROR_EXPECTED(p, "right-hand part following assignment");
		RESP(error);
	}
	RESP(pass);
});

STAGE(stage_marker_uncertainty, BF_NONE,
[](ObjectParser& p) -> Response {
	if (p.c == '?') {
		RESP_IF(!parser_read_uncertainty_marker(p), error)
		else parser_apply(p);
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
	} else {
		parser_apply(p);
		RESP_SEQ_IF(p.buffer_type != PB_MARKER_GUESS, jump, jump_base_after_value)
		else RESP(next);
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
		else parser_apply(p);
	}
	RESP(pass);
},
nullptr
);

STAGE(stage_value, BF_NONE,
[](ObjectParser& p) -> Response {
	bool is_numeric = false;
	if (p.c == '\"') {
		parser_read_string_quote(p);
	} else if (p.c == '`') {
		parser_read_string_block(p);
	} else if (parser_is_identifier_lead(p)) {
		parser_read_identifier(p);
	} else if (parser_is_number_lead(p)) {
		parser_read_number(p);
		is_numeric = true;
	} else {
		RESP(pass);
	}
	RESP_IF(p.flags & PF_ERROR, error)
	else {
		parser_apply(p);
		RESP_SEQ_IF(is_numeric, jump_sub, &sub_unit)
		else RESP(next);
	}
},
nullptr
);

STAGE(stage_unit, BF_NONE,
[](ObjectParser& p) -> Response {
	if (parser_is_identifier_lead(p)) {
		RESP_IF(!parser_read_identifier(p), error)
		else parser_apply(p, ApplyBufferAs::unit);
	}
	// resume sequence_base
	RESP(next);
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
	RESP_IF_ELSE(p.c == '$', exit_sub, pass)
},
[](ObjectParser& p) -> Response {
	RESP_IF(!parser_read_source(p), error)
	else {
		parser_apply(p, ApplyBufferAs::sub_source);
		// resume sequence_base
		RESP(next);
	}
});

STAGE(stage_tags, BF_S_TAGS,
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
	else {
		RESP_IF(!parser_next(p), error)
		else {
			parser_push(p, array::push_back_inplace(object::tags(*p.branch->obj)), sequence_tag);
			RESP(jump);
		}
	}
});

STAGE(stage_children, BF_S_CHILDREN,
[](ObjectParser& p) -> Response {
	RESP_IF_ELSE(p.c == '{', exit, pass)
},
[](ObjectParser& p) -> Response {
	RESP_IF(p.c == '}', complete)
	else if (p.c == PC_EOF) {
		PARSER_ERROR(p, "expected sub-object or block close, got EOF");
		RESP(error);
	} else if (p.c == ')' || p.c == ']') {
		PARSER_ERRORF(p, "unbalanced block close: '%c'", static_cast<char>(p.c));
		RESP(error);
	} else {
		parser_push(p, array::push_back_inplace(object::children(*p.branch->obj)), sequence_base);
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
	RESP_IF(p.c == ')', complete)
	else if (p.c == PC_EOF) {
		PARSER_ERROR(p, "expected sub-object or block close, got EOF");
		RESP(error);
	} else if (p.c == '}' || p.c == ']') {
		PARSER_ERRORF(p, "unbalanced block close: '%c'", static_cast<char>(p.c));
		RESP(error);
	} else {
		parser_push(p, array::push_back_inplace(object::children(*p.branch->obj)), sequence_base);
		RESP(jump);
	}
});

STAGE(stage_quantity, BF_S_QUANTITY,
[](ObjectParser& p) -> Response {
	RESP_IF_ELSE(p.c == '[', exit, pass);
},
[](ObjectParser& p) -> Response {
	RESP_IF(p.c == ']', complete)
	else if (p.c == PC_EOF) {
		PARSER_ERROR(p, "expected sub-object or block close, got EOF");
		RESP(error);
	} else if (p.c == '}' || p.c == ')') {
		PARSER_ERRORF(p, "unbalanced block close: '%c'", static_cast<char>(p.c));
		RESP(error);
	} else {
		parser_push(p, object::make_quantity(*p.branch->obj), sequence_base);
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
	switch (p.c) {
	case ':':
	case '}': case ')': case ']':
		p.flags |= PF_CARRY;

	case PC_EOF:
	case ',': case ';': case '\n':
		break;

	default:
		PARSER_ERRORF(
			p, "expected part or terminator following %s, got '%c' (%x)",
			(*p.branch->sequence_pos)->name,
			static_cast<char>(p.c), p.c
		);
		RESP(error);
	}
	RESP(complete);
},
nullptr
);

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
* sequence_base[]{
	&stage_lead, // -> stage_assign
	&stage_marker_uncertainty,
	&stage_marker_guess,
	&stage_marker_approximation,
	&stage_value, // -> stage_unit
	&stage_source, // -> stage_sub_source
	&stage_tags, // pre
	&stage_children,
	&stage_tags, // post
	&stage_quantity,
	&stage_complete,
	&stage_sequence_end,
},
** jump_base_after_marker_uncertainty = find_stage(sequence_base, &stage_marker_uncertainty) + 1,
** jump_base_after_value = find_stage(sequence_base, &stage_value) + 1,

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

// * sub_tentative_child = &stage_tentative_child,
* sub_assign = &stage_assign,
* sub_unit = &stage_unit,
* sub_sub_source = &stage_sub_source
;

static void parser_init(ObjectParser& p, Object& root) {
	p.info.line = 0;
	p.info.column = 0;
	p.info.message[0] = '\0';

	p.line = 1;
	p.column = 0;
	p.c = PC_EOF;
	p.flags = PF_NONE;
	p.buffer_type  = PB_NONE;
	p.branch = nullptr;
	array::clear(p.stack);
	array::clear(p.buffer);
	parser_push(p, root, sequence_root);
}

static bool parser_read(ObjectParser& p) {
	// NB: last stage in a sequence will always error
	Stage const** sequence_pos = p.branch->sequence_pos;
	StageFunction* stage_part;
	Response response = Response::error;

l_step_and_exit:
 	stage_part = (*p.branch->sequence_pos)->exit;
	if (!parser_next(p)) {
		return PARSER_ERROR_STREAM(p, "l_step_and_exit");
	}

l_do_stage_part:
	if (!parser_skip_junk(p, (*p.branch->sequence_pos)->flags & BF_M_SCOPE)) {
		return false;
	}

	response = stage_part(p);
	/*char c = static_cast<char>(p.c);
	TOGO_LOGF("FROM(%-26s) [%.*s %x] <%.*s>\n",
		(*sequence_pos)->name.data,
		p.c == PC_EOF ? 3 : 1,
		p.c == PC_EOF ? "EOF" : &c,
		unsigned_cast(p.c),
		static_cast<unsigned>(array::size(p.buffer)),
		array::begin(p.buffer)
	);*/

	// NB: sequence_pos purposefully retained in *_sub
	// Response::next from sub-sequence resumes original sequence
	switch (response) {
	case Response::next:
		p.branch->sequence_pos = sequence_pos;

	case Response::pass:
		++sequence_pos;
		stage_part = (*sequence_pos)->enter;
		goto l_do_stage_part;

	case Response::jump:
		sequence_pos = p.branch->sequence_pos;

	case Response::jump_sub:
		stage_part = (*p.branch->sequence_pos)->enter;
		goto l_do_stage_part;

	case Response::exit:
		TOGO_DEBUG_ASSERTE(stage_part == (*sequence_pos)->enter);
		p.branch->sequence_pos = sequence_pos;
		goto l_step_and_exit;

	case Response::exit_sub:
		TOGO_DEBUG_ASSERTE(stage_part == (*p.branch->sequence_pos)->enter);
		goto l_step_and_exit;

	case Response::complete:
		parser_pop(p);
		sequence_pos = p.branch->sequence_pos;
		if (p.c == PC_EOF) {
			stage_part = (*p.branch->sequence_pos)->exit;
			goto l_do_stage_part;
		} else {
			goto l_step_and_exit;
		}

	case Response::error:
		if (~p.flags & PF_ERROR) {
			PARSER_ERROR(p, "unknown error");
		}
		return false;

	case Response::eof:
		return true;
	}
}

} // anonymous namespace

} // namespace object
} // namespace quanta
