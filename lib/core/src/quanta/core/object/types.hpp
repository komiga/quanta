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
#include <togo/core/math/types.hpp>
#include <togo/core/collection/types.hpp>
#include <togo/core/string/types.hpp>
#include <togo/core/hash/hash.hpp>

namespace quanta {
namespace object {

/**
	@addtogroup lib_core_object
	@{
*/

/// Object name hash.
using ObjectNameHash = hash32;

/// Object name hash literal.
inline constexpr ObjectNameHash
operator"" _object_name(
	char const* const data,
	std::size_t const size
) {
	return hash::calc_generic_ce<ObjectNameHash>(data, size);
}

/// Object names.
enum : ObjectNameHash {
	OBJECT_NAME_NULL = ""_object_name,
};

/// Object value type.
enum class ObjectValueType : u32 {
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
};

/// Object value marker.
enum class ObjectValueMarker : unsigned {
	/// No marker.
	none,
	/// Standalone uncertainty marker.
	uncertain,
	/// Approximate, maybe less than value.
	approximate_less,
	/// Approximate, maybe more than value.
	approximate_more,
};

/// Object.
struct Object {
	using UnitString = HashedString<hash32>;

	union Value {
		bool boolean;
		struct Numeric {
			union {
				s64 integer;
				f64 decimal;
			};
			UnitString unit;
		} numeric;
		String string;
	};

	enum : unsigned {
		M_TYPE = (unsigned_cast(ObjectValueType::string) << 1) - 1,
		S_MARKER_UNCERTAIN = 5,
		M_MARKER_UNCERTAIN = 1 << S_MARKER_UNCERTAIN,
		S_MARKER_GUESS = 6,
		M_MARKER_GUESS = 1 << S_MARKER_GUESS,
		S_MARKER_TYPE = 7,
		M_MARKER_TYPE = ((1 << (unsigned_cast(ObjectValueMarker::approximate_more))) - 1) << S_MARKER_TYPE,
		// 0-3 scale, technically bounded to 0-2 because non-zero M_MARKER_TYPE already counts for one
		S_MARKER_COUNT = 10,
		M_MARKER_COUNT = 3 << S_MARKER_COUNT,
		S_SOURCE_UNCERTAIN = 12,
		M_SOURCE_UNCERTAIN = 1 << S_SOURCE_UNCERTAIN,
		S_SUB_SOURCE_UNCERTAIN = 13,
		M_SUB_SOURCE_UNCERTAIN = 1 << S_SUB_SOURCE_UNCERTAIN,
	};
	static_assert(
		0 == (
			  M_TYPE
			& M_MARKER_UNCERTAIN
			& M_MARKER_GUESS
			& M_MARKER_TYPE
			& M_MARKER_COUNT
			& M_SOURCE_UNCERTAIN
			& M_SUB_SOURCE_UNCERTAIN
		) &&
		((1 << (S_SUB_SOURCE_UNCERTAIN + 1)) - 1) == (
			  M_TYPE
			| M_MARKER_UNCERTAIN
			| M_MARKER_GUESS
			| M_MARKER_TYPE
			| M_MARKER_COUNT
			| M_SOURCE_UNCERTAIN
			| M_SUB_SOURCE_UNCERTAIN
		)
		, "bitjumbo is janked up"
	);

	u64 properties;
	u16 source;
	u16 sub_source;
	HashedString<ObjectNameHash> name;
	Value value;
	Array<Object> tags;
	Array<Object> children;
	Object* quantity;

	Object& operator=(Object const&) = delete;
	Object& operator=(Object&&) = delete;

	~Object();
	Object();
	Object(Object const& other);
	Object(Object&& other);
};

/** @} */ // end of doc-group lib_core_object

} // namespace object

using object::ObjectNameHash;
using object::_object_name;
using object::ObjectValueType;
using object::Object;

/** @cond INTERNAL */
template<>
struct enable_enum_bitwise_ops<ObjectValueType> : true_type {};
/** @endcond */ // INTERNAL

} // namespace quanta
