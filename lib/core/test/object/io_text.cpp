
#include <togo/core/error/assert.hpp>
#include <togo/core/utility/utility.hpp>
#include <togo/core/log/log.hpp>
#include <togo/core/collection/array.hpp>
#include <togo/core/string/string.hpp>
#include <togo/core/io/memory_stream.hpp>

#include <quanta/core/object/object.hpp>

#include <togo/support/test.hpp>

#include <cmath>

using namespace quanta;

#define TSN(d) {true, d, {}},
#define TSE(d, e) {true, d, e},
#define TF(d) {false, d, {}},

struct Test {
	bool success;
	StringRef data;
	StringRef expected_output;
} const tests[]{
	TSN("")
	TSN(" ")
	TSN("\n")
	TSN(" \t\n")

	TSN("//")
	TSN("/**/")
	TSN("/*\nblah\n*/")
	TF("/")
	TF("/*")

	TSE("x", "x")
	TSE("null", "null")
	TSE("false", "false")
	TSE("true", "true")

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

	TSE(" 1",  "1")
	TSE("+1",  "1")
	TSE("-1", "-1")

	// . is actually a valid identifier lead here, so this is not true
	// TSE(" .1", "0.1")
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

	TSE("a", "a")
	TSE("a\nb", "a\nb")
	TSE("\"\"", "\"\"")
	TSE("\"a\"", "a")
	TSE("``````", "\"\"")
	TSE("```a```", "a")

	TSE("\"\\t\"", "\"\t\"")
	TSE("\"\\n\"", "```\n```")

	TF(":")
	TF("::")
	TF(":,")
	TF(":;")
	TF(":\n")
	TF(":\"a\"")
	TF(":```a```")
	TF(":1")
	TF(":x=")
	TF("x=:")
	TF(":x:")
	TSE(":x", ":x")
	TSE("x:y", "x:y")
	TSE(":x:y", ":x:y")

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

	TF("=")
	TF("a=")
	TF("=,")
	TF("=;")
	TF("=\n")
	TSE("a=1", "a = 1")
	TSE("a=b", "a = b")
	TSE("a = 1", "a = 1")
	TSE("a = b", "a = b")

	TSE("a=\"\"", "a = \"\"")
	TSE("a=\"ab\"", "a = ab")
	TSE("a=\"a b\"", "a = \"a b\"")
	TF("a=\"\n")

	TSE("a=```ab```", "a = ab")
	TSE("a=```a b```", "a = \"a b\"")
	TSE("a=```a\nb```", "a = ```a\nb```")

	TF("{")
	TF("(")
	TF("[")
	TSE("{}", "null")
	TSE("{{}}", "{\n\tnull\n}")
	TSE("x{}", "x")
	TSE("x[]", "x")
	TSE("x[1]", "x[1]")
	TSE("x[y]", "x[y]")
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
		// Remove trailing newline
		if (output.size > 0) {
			--output.size;
		}
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

signed main() {
	memory_init();

	for (auto& test : tests) {
		check(test);
	}
	return 0;
}
