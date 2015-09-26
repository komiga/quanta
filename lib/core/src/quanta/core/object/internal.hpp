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

} // anonymous namespace
} // namespace object
} // namespace quanta
