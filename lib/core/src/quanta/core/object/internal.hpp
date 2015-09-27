#line 2 "quanta/core/object/internal.hpp"
/**
@copyright MIT license; see @ref index or the accompanying LICENSE file.

@file
@brief Object internals.
@ingroup lib_core_types
@ingroup lib_core_object
*/

#pragma once

#include <quanta/core/config.hpp>
#include <quanta/core/types.hpp>
#include <togo/core/utility/utility.hpp>

#include <quanta/core/object/types.hpp>

namespace quanta {
namespace object {

namespace {

enum : unsigned {
	M_TYPE = (unsigned_cast(ObjectValueType::expression) << 1) - 1,

	P_BASE = 7,

	S_VALUE_UNCERTAIN = P_BASE + 0,
	M_VALUE_UNCERTAIN = 1 << S_VALUE_UNCERTAIN,
	S_VALUE_GUESS = P_BASE + 1,
	M_VALUE_GUESS = 1 << S_VALUE_GUESS,
	// bits: NXX, where: N = negative, X = value (0-3)
	S_VALUE_APPROXIMATE = P_BASE + 2,
	M_VALUE_APPROXIMATE = ((1 << 2) | 3) << S_VALUE_APPROXIMATE,
	S_SOURCE_UNCERTAIN = P_BASE + 5,
	M_SOURCE_UNCERTAIN = 1 << S_SOURCE_UNCERTAIN,
	S_SUB_SOURCE_UNCERTAIN = P_BASE + 6,
	M_SUB_SOURCE_UNCERTAIN = 1 << S_SUB_SOURCE_UNCERTAIN,
	S_OP = P_BASE + 7,
	M_OP = 3 << S_OP,
	S_TM_TYPE = P_BASE + 9,
	M_TM_TYPE = 3 << S_TM_TYPE,
	S_TM_UNZONED = P_BASE + 11,
	M_TM_UNZONED = 1 << S_TM_UNZONED,
	S_TM_CONTEXTUAL_MONTH = P_BASE + 12,
	M_TM_CONTEXTUAL_MONTH = 1 << S_TM_CONTEXTUAL_MONTH,
	S_TM_CONTEXTUAL_YEAR = P_BASE + 13,
	M_TM_CONTEXTUAL_YEAR = 1 << S_TM_CONTEXTUAL_YEAR,

	P_END = P_BASE + 14,

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

	M_TM_PROPERTIES
		= M_TM_TYPE
		| M_TM_UNZONED
		| M_TM_CONTEXTUAL_MONTH
		| M_TM_CONTEXTUAL_YEAR
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
		& M_TM_TYPE
		& M_TM_UNZONED
		& M_TM_CONTEXTUAL_MONTH
		& M_TM_CONTEXTUAL_YEAR
	) &&
	((1 << P_END) - 1) == (
		  M_TYPE
		| M_VALUE_UNCERTAIN
		| M_VALUE_GUESS
		| M_VALUE_APPROXIMATE
		| M_SOURCE_UNCERTAIN
		| M_SUB_SOURCE_UNCERTAIN
		| M_OP
		| M_TM_TYPE
		| M_TM_UNZONED
		| M_TM_CONTEXTUAL_MONTH
		| M_TM_CONTEXTUAL_YEAR
	)
	, "bitjumbo is janked up"
);

static constexpr ObjectValueType const type_mask_numeric
	= ObjectValueType::integer
	| ObjectValueType::decimal
;

} // anonymous namespace

namespace internal {

inline unsigned get_property(Object const& obj, unsigned mask, unsigned shift) {
	return (obj.properties & mask) >> shift;
}

inline void set_property(Object& obj, unsigned mask, unsigned shift, unsigned value) {
	obj.properties = (obj.properties & ~mask) | (value << shift);
}

inline void clear_property(Object& obj, unsigned mask) {
	obj.properties &= ~mask;
}

} // namespace internal

} // namespace object
} // namespace quanta
