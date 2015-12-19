#line 2 "quanta/core/object/io_text.cpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.
*/

#include <quanta/core/config.hpp>
#include <quanta/core/chrono/time.hpp>
#include <quanta/core/object/object.hpp>

#include <togo/core/error/assert.hpp>
#include <togo/core/log/log.hpp>
#include <togo/core/memory/memory.hpp>
#include <togo/core/memory/temp_allocator.hpp>
#include <togo/core/collection/array.hpp>
#include <togo/core/string/string.hpp>
#include <togo/core/io/types.hpp>
#include <togo/core/io/io.hpp>
#include <togo/core/io/memory_stream.hpp>
#include <togo/core/io/file_stream.hpp>

#include <quanta/core/object/io/common.ipp>
#include <quanta/core/object/io/parser.ipp>
#include <quanta/core/object/io/writer.ipp>

#include <cstdio>

namespace quanta {

/// Read text-format object from stream.
///
/// If single_value is true, reads directly into root. If there are multiple
/// values, an error will occur.
/// Returns false if a parser error occurred. pinfo will have the position and
/// error message of the parser.
bool object::read_text(Object& root, IReader& stream, ObjectParserInfo& pinfo, bool single_value IGEN_DEFAULT(false)) {
	TempAllocator<4096> allocator{};
	ObjectParser p{stream, pinfo, allocator};
	array::reserve(p.stack, 32);
	array::reserve(p.buffer, 4096 - (1 * sizeof(void*)) - (32 * sizeof(ObjectParser::Branch)));

	object::clear(root);
	parser_init(p, root, single_value);
	return parser_read(p);
}

/// Read text-format object from stream (sans parser info).
bool object::read_text(Object& root, IReader& stream, bool single_value IGEN_DEFAULT(false)) {
	ObjectParserInfo pinfo{};
	if (!object::read_text(root, stream, pinfo, single_value)) {
		TOGO_LOG_ERRORF(
			"failed to read object: [%2u,%2u]: %s\n",
			pinfo.line, pinfo.column, pinfo.message
		);
		return false;
	}
	return true;
}

/// Read text-format object from string.
bool object::read_text_string(Object& root, StringRef text, bool single_value IGEN_DEFAULT(false)) {
	ObjectParserInfo pinfo{};
	MemoryReader stream{text};
	if (!object::read_text(root, stream, pinfo, single_value)) {
		TOGO_LOG_ERRORF(
			"failed to read object: [%2u,%2u]: %s\n",
			pinfo.line, pinfo.column, pinfo.message
		);
		return false;
	}
	return true;
}

/// Read text-format object from file.
bool object::read_text_file(Object& root, StringRef const& path, bool single_value IGEN_DEFAULT(false)) {
	FileReader stream{};
	if (!stream.open(path)) {
		TOGO_LOG_ERRORF(
			"failed to read object from '%.*s': failed to open file\n",
			path.size, path.data
		);
		return false;
	}

	ObjectParserInfo pinfo{};
	bool const success = object::read_text(root, stream, pinfo, single_value);
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
/// If single_value is false, only the children of obj are written (without an
/// enclosing block).
/// Returns true if the write succeeded.
bool object::write_text(Object const& obj, IWriter& stream, bool single_value IGEN_DEFAULT(false)) {
	if (single_value) {
		if (!write_object(stream, obj, 0)) {
			return false;
		}
	} else if (object::has_children(obj)) {
		auto& last_child = array::back(object::children(obj));
		for (auto& child : object::children(obj)) {
			if (!(
				write_object(stream, child, 0) &&
				(&child == &last_child || io::write_value(stream, '\n'))
			)) {
				return false;
			}
		}
	}
	return io::status(stream).ok();
}

/// Write text-format object to file.
bool object::write_text_file(Object const& obj, StringRef const& path, bool single_value IGEN_DEFAULT(false)) {
	FileWriter stream{};
	if (!stream.open(path, false)) {
		TOGO_LOG_ERRORF(
			"failed to write object to '%.*s': failed to open file\n",
			path.size, path.data
		);
		return false;
	}
	bool const success = object::write_text(obj, stream, single_value);
	stream.close();
	return success;
}

} // namespace quanta
