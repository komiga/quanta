
#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>
#include <togo/core/log/log.hpp>
#include <togo/core/collection/array.hpp>
#include <togo/core/string/string.hpp>
#include <togo/core/io/io.hpp>
#include <togo/core/io/memory_stream.hpp>

#include <quanta/core/object/object.hpp>

#include <togo/support/test.hpp>

using namespace quanta;

#define M_TSN(d)	{true, false, d, {}},
#define M_TSE(d, e)	{true, false, d, e},
#define M_TSS(d)	{true, false, d, d},
#define M_TF(d)		{false, false, d, {}},

#define S_TSN(d)	{true, true, d, {}},
#define S_TSE(d, e)	{true, true, d, e},
#define S_TSS(d)	{true, true, d, d},
#define S_TF(d)		{false, true, d, {}},

#define TSN(d)		M_TSN(d) S_TSN(d)
#define TSE(d, e)	M_TSE(d, e) S_TSE(d, e)
#define TSS(d)		M_TSS(d) S_TSS(d)
#define TF(d)		M_TF(d) S_TF(d)

struct Test {
	bool success;
	bool single_value;
	StringRef data;
	StringRef expected_output;
} const tests[]{
// whitespace
	TSN("")
	TSN(" ")
	TSN("\n")
	TSN(" \t\n")

// comments
	TF("\\")
	TF("\\*")
	TSN("\\\\")
	TSN("\\**\\")
	TSN("\\*\nblah\n*\\")
	M_TSE("x\\\\\ny", "x\ny")
	S_TF("x\\\\\ny")

	// nested
	TF("\\* \\* *\\")
	TSN("\\* \\**\\ *\\")

// constants
	TSE("null", "null")
	TSE("false", "false")
	TSE("true", "true")

// identifiers
	TSE("a", "a")
	TSE("á", "á")
	TSE(".á", ".á")
	TSE("_", "_")

	M_TSE("a\nb", "a\nb")

	S_TSE("\n\\**\\a\n", "a")
	S_TSE("a\n", "a")
	S_TF("a\nb")

// markers
	TF("G~?")
	TF("?G~")

	TSE("~", "~")
	TSE("~x", "~x")
	TSE("~~x", "~~x")
	TSE("~~~x", "~~~x")

	TSE("^", "^")
	TSE("^x", "^x")
	TSE("^^x", "^^x")
	TSE("^^^x", "^^^x")

	TSE("G~~x", "G~~x")
	TSE(" G~x",  "G~x")
	TSE("G~^x", "G~^x")

	TSE("?",  "?")
	TSE("?~x", "?~x")
	TSE(" ?x",  "?x")
	TSE("?^x", "?^x")

// source
	TSE("x$0", "x")
	TSE("x$0$0", "x")
	TSE("x$0$1", "x")
	TSE("x$0$?", "x")

	TSE("x$1", "x$1")
	TSE("x$1$0", "x$1")
	TSE("x$1$?", "x$1$?")
	TSE("x$1$2", "x$1$2")

	TSE("x$?1", "x$?1")
	TSE("x$?1$0", "x$?1")
	TSE("x$?1$?", "x$?1$?")
	TSE("x$?1$2", "x$?1$2")

	TSE("x$?$?", "x$?$?")

// integer & decimal
	TSE(" 1",  "1")
	TSE("+1",  "1")
	TSE("-1", "-1")

	TSE("+.1",  "0.1")
	TSE("-.1", "-0.1")

	TSE(" 1.1",  "1.1")
	TSE("+1.1",  "1.1")
	TSE("-1.1", "-1.1")

	TSE(" 1.1e1",  "11")
	TSE("+1.1e1",  "11")
	TSE("-1.1e1", "-11")

	TSE(" 1.1e+1",  "11")
	TSE("+1.1e+1",  "11")
	TSE("-1.1e+1", "-11")

	TSE(" 1.1e-1",  "0.11")
	TSE("+1.1e-1",  "0.11")
	TSE("-1.1e-1", "-0.11")

// units
	TSS("1a")
	TSS("1µg")
	TSS("1°C")
	TSS("1e")
	TSE("1e{}", "1e")
	TSS("1exact")
	TSS("1.1e")

// currency
	TF("¤")
	TF("¤1")
	TF("¤-1")
	TF("¤+1")
	TF("¤1.")
	TF("¤.1x")

	TSS("¤1x")
	TSS("¤-1x")
	TSE("¤+1x", "¤1x")
	TSS("¤1.0x")
	TSS("¤1.0023x")
	TSS("¤-0.9x")
	TSS("¤-0.00009x")
	TSS("¤0.00009x")
	TSS("¤4.23usd")
	TSS("¤480yen")
	TSS("¤-124.12eur")

// times
	TF("2-")
	TF("201-")
	TF("2015-")
	TF("2015-01")
	TF("2015-01-")
	TF("2015-01-02-")

	TF("2015-Z")
	TF("2015-0Z")
	TF("2015-01Z")
	TF("2015-01-Z")
	TF("2015-01-0Z")

	TF("2015+")
	TF("2015-01+")
	TF("2015-01-02+")

	TSS("2015-01-02")
	TSE("2015-01-02T", "2015-01-02")
	TSS("01-02")
	TSE("01-02T", "01-02")
	TSS("02T")
	TSS("x = 02T")
	TSE("x = 02T{}", "x = 02T")
	TSE("{x = 02T}", "{\n\tx = 02T\n}")

	TSS("2015-01-02Z")
	TSS("01-02Z")
	TSS("02Z")

	TSS("2015-01-02T-03")
	TSS("2015-01-02T+03")
	TSS("01-02T-03")
	TSS("01-02T+03")
	TSS("02T-03")
	TSS("02T+03")

	TSS("2015-01-02T-03:04")
	TSS("2015-01-02T+03:04")
	TSS("01-02T-03:04")
	TSS("01-02T+03:04")
	TSS("02T-03:04")
	TSS("02T+03:04")

	TF("01:0")
	TF("01:02:")
	TF("01:02:0")
	TF("01:02:03:")
	TF("01:02:03:00")

	TF("01:0Z")
	TF("01:02:0Z")

	TF("01:02-")
	TF("01:02+")

	TF("01:02:03-")
	TF("01:02:03+")

	TSE("01:02", "01:02:00")
	TSS("01:02:03")

	TSE("01:02Z", "01:02:00Z")
	TSS("01:02:03Z")

	TSE("01:02+04", "01:02:00+04")
	TSE("01:02-04", "01:02:00-04")
	TSE("01:02+04:05", "01:02:00+04:05")
	TSE("01:02-04:05", "01:02:00-04:05")

	TSS("01:02:03+04")
	TSS("01:02:03-04")
	TSS("01:02:03+04:05")
	TSS("01:02:03-04:05")

	TF("01T02")
	TF("01T02Z")

	TSS("2015-01-02T03:04:05")
	TSE("2015-01-02T03:04", "2015-01-02T03:04:00")

	TSS("01-02T03:04:05")
	TSE("01-02T03:04", "01-02T03:04:00")

	TSS("02T03:04:05")
	TSE("02T03:04", "02T03:04:00")

// strings
	TSE("\"\"", "\"\"")
	TSS("\"a\"")
	TSE("``````", "\"\"")
	TSE("```a```", "\"a\"")
	TSE("```a b```", "\"a b\"")
	TSE("```a\"b```", "```a\"b```")
	TSE("```\na\n```", "```\na\n```")

	TSE("\"\\t\"", "\"\t\"")
	TSE("\"\\n\"", "```\n```")

	TSS("G\"\"")
	TSS("type\"\"")
	TSE("type``````", "type\"\"")
	TSS("type\"value\"")
	TSS("type```value\nvalue```")

// names
	TF("=")
	TF("a=")
	TF("=,")
	TF("=;")
	TF("=\n")
	TF("1=b")
	TSE(".1=b", ".1 = b")
	TSE("a=1", "a = 1")
	TSE("a=b", "a = b")
	TSE("a = 1", "a = 1")
	TSE("a = b", "a = b")
	TSE("a = +1", "a = 1")
	TSS("a = -1")

	TSE("a=\"\"", "a = \"\"")
	TSE("a=\"ab\"", "a = \"ab\"")
	TSE("a=\"a b\"", "a = \"a b\"")
	TF("a=\"\n")

	TSE("a=```ab```", "a = \"ab\"")
	TSE("a=```a b```", "a = \"a b\"")
	TSE("a=```a\nb```", "a = ```a\nb```")

// tags
	TF(":")
	TF("::")
	TF(":,")
	TF(":;")
	TF(":\n")
	TF(":\"a\"")
	TF(":```a```")
	TF(":1")
	TF(":x:")
	TF(":x=")
	TF("x=:")

	TSS(":x")
	TSS(":x=1")
	TSS(":x=1(a, b)")

	TSS(":=1")
	TSS(":=1(a, b)")
	TSE("x:y", "x:y")
	TSE(":x:y", ":x:y")

	TSS(":()")
	TSS(":G~")
	TSS(":?")
	TSS(":false")
	TSS(":true")
	TSS(":null")

	TSS(":=(x + y)(a, b)")
	TSE(":x=()()", ":x=()")

	TSS(":?x")
	TSS(":G~x")

	TSS(":~x")
	TSS(":~~x")
	TSS(":~~~x")

	TSS(":^x")
	TSS(":^^x")
	TSS(":^^^x")

	TSS(":?~x")
	TSS(":?~~x")
	TSS(":?~~~x")

	TSS(":?^x")
	TSS(":?^^x")
	TSS(":?^^^x")

	TSS(":G~~x")
	TSS(":G~~~x")
	TSS(":G~~~~x")

	TSS(":G~^x")
	TSS(":G~^^x")
	TSS(":G~^^^x")

	TF(":x(")
	TSE(":x()", ":x")
	TSE(":x( )", ":x")
	TSE(":x(\t)", ":x")
	TSE(":x(\n)", ":x")
	TSE(":x(1)", ":x(1)")
	TSE(":x(y)", ":x(y)")
	TSE(":x(y,z)", ":x(y, z)")
	TSE(":x(y;z)", ":x(y, z)")
	TSE(":x(y\nz)", ":x(y, z)")

// scopes
	TF("{")
	TF("(")
	TF("[")
	TF("x[+")
	TF("x[,")
	TF("x[;")
	TF("x[\n")
	TSE("{}", "null")
	TSE("{{}}", "{\n\tnull\n}")
	TSE("x{}", "x")
	TSE("x[]", "x")
	TSE("x[1]", "x[1]")
	TSE("x[y]", "x[y]")
	TSE("x[y:a(1){2}, b]", "x[y:a(1){\n\t2\n}, b]")

// expressions
	TF("x + ")
	TF("x - ,")
	TF("x * ;")
	TF("{x / }")
	TSS("x + y")
	TSS("x = y + z")
	TSS("a[x - y]")
	TSS("a[x - y, 1 * 2, z]")
	TSS(":a(x * y)")
	TSS("x + y - z")
	TSE("{x / y}", "{\n\tx / y\n}")
	M_TSS("x\nz + w")
	S_TF("x\nz + w")
	M_TSE("x + y, z / w", "x + y\nz / w")
	S_TF("x + y, z / w")
	TSS("x[y] + z")

// scoped expressions
	TF("(")
	TF("(x + )")
	TF("(x + y + )")
	TF("(x,)")
	TF("(x;)")
	TF("(x + y,)")
	TF("(x + y;)")
	TSS("()")
	TSE("(\n)", "()")
	TSE("(\\*blargh*\\)", "()")
	TSE("(x\n)", "(x)")
	TSE("(x + y)", "x + y")
	TSE("(x + y){a, b}", "(x + y){\n\ta\n\tb\n}")
	TSE("(\\**\\x\\**\\ + y)", "x + y")
	TSE("(x + y):a", "(x + y):a")
	TSE(":a()()", "():a")
	TSE(":a:b()(x)", "(x):a:b")
	TSE(":a()(x + y):b", "(x + y):a:b")
};

void check(Test const& test) {
	TOGO_LOGF(
		"reading   (%3u): <%.*s>\n",
		test.data.size, test.data.size, test.data.data
	);
	Object root;
	MemoryReader in_stream{test.data};
	ObjectParserInfo pinfo;
	if (object::read_text(root, in_stream, pinfo, test.single_value)) {
		MemoryStream out_stream{memory::default_allocator(), test.expected_output.size + 1};
		TOGO_ASSERTE(object::write_text(root, out_stream, test.single_value));
		StringRef output{
			reinterpret_cast<char*>(array::begin(out_stream.data())),
			static_cast<unsigned>(out_stream.size())
		};
		TOGO_LOGF(
			"rewritten (%3u): <%.*s>\n",
			output.size, output.size, output.data
		);
		TOGO_ASSERT(test.success, "read succeeded when it should have failed");
		TOGO_ASSERT(
			string::compare_equal(
				test.single_value && test.expected_output.empty()
				? StringRef{"null"}
				: test.expected_output,
				output
			),
			"output does not match"
		);
	} else {
		TOGO_LOGF(
			"failed to read%s: [%2u,%2u]: %s\n",
			test.success ? " (UNEXPECTED)" : "",
			pinfo.line,
			pinfo.column,
			pinfo.message
		);
		TOGO_ASSERT(!test.success, "read failed when it should have succeeded");
	}
	TOGO_LOG("\n");
}

bool rewrite_file(MemoryStream& out_stream, StringRef path) {
	Object root;
	if (!object::read_text_file(root, path)) {
		TOGO_LOG("\\\\ failed to read\n");
		return false;
	}
	if (!object::write_text(root, out_stream)) {
		TOGO_LOG("\\\\ failed to rewrite\n");
		return false;
	}
	io::seek_to(out_stream, 0);
	if (!object::read_text(root, out_stream)) {
		TOGO_LOG("\\\\ failed to reread\n");
		return false;
	}
	return true;
}

signed main(signed argc, char* argv[]) {
	memory_init();

	if (argc > 1) {
		bool print = true;
		signed i = 1;
		if (string::compare_equal({argv[i], cstr_tag{}}, "--noprint")) {
			print = false;
			++i;
		}

		MemoryStream out_stream{memory::default_allocator(), 4096 * 16};
		for (; i < argc; ++i) {
			StringRef path{argv[i], cstr_tag{}};
			TOGO_LOGF("\n\\\\ file: %.*s\n", path.size, path.data);
			if (rewrite_file(out_stream, path) && print) {
				TOGO_LOGF("%.*s",
					static_cast<unsigned>(out_stream.size()),
					reinterpret_cast<char*>(array::begin(out_stream.data()))
				);
			}
			out_stream.clear();
		}
	} else {
		for (auto& test : tests) {
			check(test);
		}
	}
	return 0;
}
