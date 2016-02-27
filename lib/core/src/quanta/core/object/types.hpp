#line 2 "quanta/core/object/types.hpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.

@file
@brief Object types.
@ingroup lib_core_types
@ingroup lib_core_object
*/

#pragma once

#include <quanta/core/config.hpp>
#include <quanta/core/types.hpp>
#include <quanta/core/string/types.hpp>
#include <quanta/core/chrono/types.hpp>

#include <togo/core/utility/traits.hpp>
#include <togo/core/collection/types.hpp>
#include <togo/core/string/types.hpp>
#include <togo/core/hash/hash.hpp>
#include <togo/core/lua/types.hpp>

namespace quanta {
namespace object {

/**
	@addtogroup lib_core_object
	@{
*/

/// Object name hasher.
using ObjectNameHasher = hash::Default32;

/// Object name hash.
using ObjectNameHash = ObjectNameHasher::Value;

/// Object value hasher (numeric unit, identifier).
using ObjectValueHasher = hash::Default32;

/// Object value hash.
using ObjectValueHash = ObjectValueHasher::Value;

namespace hash_literals {

/// Object name hash literal.
inline constexpr ObjectNameHash
operator"" _object_name(
	char const* const data,
	std::size_t const size
) {
	return hash::calc_ce<ObjectNameHasher>(data, size);
}

/// Object value hash literal.
inline constexpr ObjectValueHash
operator"" _object_value(
	char const* const data,
	std::size_t const size
) {
	return hash::calc_ce<ObjectValueHasher>(data, size);
}

} // namespace hash_literals

using namespace hash_literals;

/// Object name hashes.
enum : ObjectNameHash {
	OBJECT_NAME_NULL = ""_object_name,
};

/// Object value hashes.
enum : ObjectValueHash {
	OBJECT_VALUE_NULL = ""_object_value,
};

/// Object value type.
enum class ObjectValueType : unsigned {
	/// Empty value.
	null		= 1 << 0,
	/// Boolean.
	boolean		= 1 << 1,
	/// 64-bit signed integer.
	integer		= 1 << 2,
	/// 64-bit floating-point number.
	decimal		= 1 << 3,
	/// Time.
	time		= 1 << 4,
	/// Currency/money.
	currency	= 1 << 5,
	/// String.
	string		= 1 << 6,
	/// Identifier.
	identifier	= 1 << 7,
	/// Expression.
	expression	= 1 << 8,
};

/// Object operator.
enum class ObjectOperator : unsigned {
	/// No operator (technically equivalent to add).
	none = 0,
	/// Add.
	add = none,
	/// Subtract.
	sub,
	/// Multiply.
	mul,
	/// Divide.
	div,
};

/// Object time type.
enum class ObjectTimeType : unsigned {
	/// Date and clock.
	date_and_clock,
	/// Date.
	date,
	/// Clock.
	clock,
};

/// Object.
struct Object {
	TOGO_LUA_MARK_USERDATA(quanta::object::Object);

	struct Currency {
		s64 value;
		s32 exponent;
	};
	struct Numeric {
		union {
			s64 integer;
			f64 decimal;
			Currency c;
		};
		HashedUnmanagedString<ObjectValueHasher> unit;
	};

	union Value {
		bool boolean;
		Numeric numeric;
		Time time;
		UnmanagedString string;
		HashedUnmanagedString<ObjectValueHasher> identifier;
	};

	u32 properties;
	u32 source_line;
	u16 source;
	u16 sub_source;
	HashedUnmanagedString<ObjectNameHasher> name;
	Value value;
	Array<Object> tags;
	Array<Object> children;
	Object* quantity;

	Object& operator=(Object&&) = delete;

	~Object();
	Object();
	Object(Object const& other);
	Object(Object&& other);

private:
	friend struct togo::collection_npod_impl<Object, true>;
	Object& operator=(Object const&);
};

/// Object parser information.
struct ObjectParserInfo {
	/// Line in stream.
	unsigned line;
	/// Column on line.
	unsigned column;
	/// Error message.
	char message[512];
};

/** @} */ // end of doc-group lib_core_object

} // namespace object

using object::ObjectNameHash;
using object::ObjectValueHash;
using namespace object::hash_literals;
using object::OBJECT_NAME_NULL;
using object::OBJECT_VALUE_NULL;
using object::ObjectValueType;
using object::ObjectTimeType;
using object::ObjectOperator;
using object::Object;
using object::ObjectParserInfo;

} // namespace quanta

/** @cond INTERNAL */
namespace togo {
	template<> struct enable_enum_bitwise_ops<quanta::ObjectValueType> : true_type {};
	template<> struct allow_collection_value_type<quanta::Object> : true_type {};
	template<> struct enable_collection_construction_and_destruction<quanta::Object> : true_type {};
} // namespace togo
/** @endcond */ // INTERNAL
