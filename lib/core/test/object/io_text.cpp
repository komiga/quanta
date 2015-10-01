
#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>
#include <togo/core/log/log.hpp>
#include <togo/core/collection/array.hpp>
#include <togo/core/string/string.hpp>
#include <togo/core/io/memory_stream.hpp>
#include <togo/core/io/io.hpp>

#include <quanta/core/object/object.hpp>

#include <togo/support/test.hpp>

#include <cmath>

using namespace quanta;

#define TSN(d) {true, d, {}},
#define TSE(d, e) {true, d, e},
#define TSS(d) {true, d, d},
#define TF(d) {false, d, {}},

struct Test {
	bool success;
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
	TSE("x\\\\\ny", "x\ny")

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
	TSE("a\nb", "a\nb")

// markers
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
	TSE("1a", "1a")
	TSE("1µg", "1µg")

// times
	TF("2-")
	TF("201-")
	TF("2015-")
	TF("2015-01")
	TF("2015-01-")
	TF("2015-01-02-")

	TSS("2015-01-02")
	TSE("2015-01-02T", "2015-01-02")
	TSS("01-02")
	TSE("01-02T", "01-02")
	TSS("02T")

	TF("01:0")
	TF("01:02:")
	TF("01:02:0")
	TF("01:02:03:")
	TF("01:02:03:00")

	TSE("01:02", "01:02:00")
	TSS("01:02:03")

	TF("01T02")

	TSS("2015-01-02T03:04:05")
	TSE("2015-01-02T03:04", "2015-01-02T03:04:00")

	TSS("01-02T03:04:05")
	TSE("01-02T03:04", "01-02T03:04:00")

	TSS("02T03:04:05")
	TSE("02T03:04", "02T03:04:00")

// strings
	TSE("\"\"", "\"\"")
	TSE("\"a\"", "a")
	TSE("``````", "\"\"")
	TSE("```a```", "a")

	TSE("\"\\t\"", "\"\t\"")
	TSE("\"\\n\"", "```\n```")

// names
	TF("=")
	TF("a=")
	TF("=,")
	TF("=;")
	TF("=\n")
	TSE("a=1", "a = 1")
	TSE("a=b", "a = b")
	TSE("a = 1", "a = 1")
	TSE("a = b", "a = b")
	TSE("a = +1", "a = 1")
	TSS("a = -1")

	TSE("a=\"\"", "a = \"\"")
	TSE("a=\"ab\"", "a = ab")
	TSE("a=\"a b\"", "a = \"a b\"")
	TF("a=\"\n")

	TSE("a=```ab```", "a = ab")
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
	TSE(":x", ":x")
	TF(":x:")
	TF(":x=")
	TF("x=:")
	TSE("x:y", "x:y")
	TSE(":x:y", ":x:y")

	TSE(":?x", ":?x")
	TSE(":G~x", ":G~x")

	TSE(":~x", ":~x")
	TSE(":~~x", ":~~x")
	TSE(":~~~x", ":~~~x")

	TSE(":^x", ":^x")
	TSE(":^^x", ":^^x")
	TSE(":^^^x", ":^^^x")

	TSE(":?~x", ":?~x")
	TSE(":?~~x", ":?~~x")
	TSE(":?~~~x", ":?~~~x")

	TSE(":?^x", ":?^x")
	TSE(":?^^x", ":?^^x")
	TSE(":?^^^x", ":?^^^x")

	TSE(":G~~x", ":G~~x")
	TSE(":G~~~x", ":G~~~x")
	TSE(":G~~~~x", ":G~~~~x")

	TSE(":G~^x", ":G~^x")
	TSE(":G~^^x", ":G~^^x")
	TSE(":G~^^^x", ":G~^^^x")

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
	TSE("x + y", "x + y")
	TSE("a[x - y]", "a[x - y]")
	TSE("a[x - y, 1 * 2, z]", "a[x - y, 1 * 2, z]")
	TSE(":a(x * y)", ":a(x * y)")
	TSE("{x / y}", "{\n\tx / y\n}")
	TSE("x + y - z", "x + y - z")
	TSS("x\nz + w")
	TSE("x + y, z / w", "x + y\nz / w")
	TSS("x[y] + z")
};

void check(Test const& test) {
	TOGO_LOGF(
		"reading (%3u): <%.*s>\n",
		test.data.size, test.data.size, test.data.data
	);
	Object root;
	MemoryReader in_stream{test.data};
	ObjectParserInfo pinfo;
	if (object::read_text(root, in_stream, pinfo)) {
		MemoryStream out_stream{memory::default_allocator(), test.expected_output.size + 1};
		TOGO_ASSERTE(object::write_text(root, out_stream));
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
			string::compare_equal(test.expected_output, output),
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
