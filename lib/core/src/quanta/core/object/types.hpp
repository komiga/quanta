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
#include <togo/core/utility/tags.hpp>
#include <togo/core/utility/traits.hpp>
#include <togo/core/utility/utility.hpp>
#include <togo/core/collection/types.hpp>
#include <togo/core/string/types.hpp>
#include <togo/core/hash/hash.hpp>

#include <quanta/core/string/types.hpp>

namespace quanta {
namespace object {

/**
	@addtogroup lib_core_object
	@{
*/

/// Object name hash.
using ObjectNameHash = hash32;

/// Object numeric unit hash.
using ObjectNumericUnitHash = hash32;

namespace hash_literals {

/// Object name hash literal.
inline constexpr ObjectNameHash
operator"" _object_name(
	char const* const data,
	std::size_t const size
) {
	return hash::calc_generic_ce<ObjectNameHash>(data, size);
}

} // namespace hash_literals

using namespace hash_literals;

/// Object names.
enum : ObjectNameHash {
	OBJECT_NAME_NULL = ""_object_name,
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
	/// String.
	string		= 1 << 4,
	/// String.
	expression	= 1 << 5,
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

/// Object.
struct Object {
	union Value {
		bool boolean;
		struct Numeric {
			union {
				s64 integer;
				f64 decimal;
			};
			HashedUnmanagedString<ObjectNumericUnitHash> unit;
		} numeric;
		UnmanagedString string;
	};

	enum : unsigned {
		M_TYPE = (unsigned_cast(ObjectValueType::expression) << 1) - 1,
		S_VALUE_UNCERTAIN = 6,
		M_VALUE_UNCERTAIN = 1 << S_VALUE_UNCERTAIN,
		S_VALUE_GUESS = 7,
		M_VALUE_GUESS = 1 << S_VALUE_GUESS,
		// bits: NXX, where: N = negative, X = value (0-3)
		S_VALUE_APPROXIMATE = 8,
		M_VALUE_APPROXIMATE = ((1 << 2) | 3) << S_VALUE_APPROXIMATE,
		S_SOURCE_UNCERTAIN = 11,
		M_SOURCE_UNCERTAIN = 1 << S_SOURCE_UNCERTAIN,
		S_SUB_SOURCE_UNCERTAIN = 12,
		M_SUB_SOURCE_UNCERTAIN = 1 << S_SUB_SOURCE_UNCERTAIN,
		S_OP = 13,
		M_OP = 3 << S_OP,

		M_BOTH_SOURCE_UNCERTAIN
			= M_SOURCE_UNCERTAIN
			| M_SUB_SOURCE_UNCERTAIN
		,
		M_VALUE_UNCERTAIN_AND_GUESS
			= M_VALUE_UNCERTAIN
			| M_VALUE_GUESS
		,
		M_VALUE_MARKERS
			= M_VALUE_UNCERTAIN
			| M_VALUE_GUESS
			| M_VALUE_APPROXIMATE
		,
	};
	static_assert(
		0 == (
			  M_TYPE
			& M_VALUE_UNCERTAIN
			& M_VALUE_GUESS
			& M_VALUE_APPROXIMATE
			& M_SOURCE_UNCERTAIN
			& M_SUB_SOURCE_UNCERTAIN
			& M_OP
		) &&
		((1 << (S_OP + 2)) - 1) == (
			  M_TYPE
			| M_VALUE_UNCERTAIN
			| M_VALUE_GUESS
			| M_VALUE_APPROXIMATE
			| M_SOURCE_UNCERTAIN
			| M_SUB_SOURCE_UNCERTAIN
			| M_OP
		)
		, "bitjumbo is janked up"
	);

	u64 properties;
	u16 source;
	u16 sub_source;
	HashedUnmanagedString<ObjectNameHash> name;
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
using object::ObjectNumericUnitHash;
using namespace object::hash_literals;
using object::OBJECT_NAME_NULL;
using object::ObjectValueType;
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
