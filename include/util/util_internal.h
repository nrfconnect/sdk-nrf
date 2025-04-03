/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Misc utilities
 *
 * Repetitive or obscure helper macros needed by util_macro.h.
 */

#ifndef UTIL_INTERNAL_H_
#define UTIL_INTERNAL_H_

#include <zephyr/sys/util_internal.h>

/* Helper macro for IS_ENABLED_ALL */
#define Z_IS_ENABLED_ALL(...) \
	Z_IS_ENABLED_ALL_N(NUM_VA_ARGS(__VA_ARGS__), __VA_ARGS__)
#define Z_IS_ENABLED_ALL_N(N, ...) UTIL_CAT(Z_IS_ENABLED_ALL_, N)(__VA_ARGS__)
#define Z_IS_ENABLED_ALL_0(...)
#define Z_IS_ENABLED_ALL_1(a, ...)  COND_CODE_1(a, (1), (0))
#define Z_IS_ENABLED_ALL_2(a, ...)  COND_CODE_1(a, (Z_IS_ENABLED_ALL_1(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_3(a, ...)  COND_CODE_1(a, (Z_IS_ENABLED_ALL_2(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_4(a, ...)  COND_CODE_1(a, (Z_IS_ENABLED_ALL_3(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_5(a, ...)  COND_CODE_1(a, (Z_IS_ENABLED_ALL_4(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_6(a, ...)  COND_CODE_1(a, (Z_IS_ENABLED_ALL_5(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_7(a, ...)  COND_CODE_1(a, (Z_IS_ENABLED_ALL_6(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_8(a, ...)  COND_CODE_1(a, (Z_IS_ENABLED_ALL_7(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_9(a, ...)  COND_CODE_1(a, (Z_IS_ENABLED_ALL_8(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_10(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_9(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_11(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_10(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_12(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_11(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_13(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_12(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_14(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_13(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_15(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_14(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_16(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_15(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_17(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_16(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_18(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_17(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_19(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_18(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_20(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_19(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_21(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_20(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_22(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_21(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_23(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_22(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_24(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_23(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_25(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_24(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_26(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_25(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_27(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_26(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_28(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_27(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_29(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_28(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_30(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_29(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_31(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_30(__VA_ARGS__,)), (0))
#define Z_IS_ENABLED_ALL_32(a, ...) COND_CODE_1(a, (Z_IS_ENABLED_ALL_31(__VA_ARGS__,)), (0))

/* Helper macro for IS_ENABLED_ANY */
#define Z_IS_ENABLED_ANY(...) \
	Z_IS_ENABLED_ANY_N(NUM_VA_ARGS(__VA_ARGS__), __VA_ARGS__)
#define Z_IS_ENABLED_ANY_N(N, ...) UTIL_CAT(Z_IS_ENABLED_ANY_, N)(__VA_ARGS__)
#define Z_IS_ENABLED_ANY_0(...)
#define Z_IS_ENABLED_ANY_1(a, ...)  COND_CODE_1(a, (1), (0))
#define Z_IS_ENABLED_ANY_2(a, ...)  COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_1(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_3(a, ...)  COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_2(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_4(a, ...)  COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_3(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_5(a, ...)  COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_4(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_6(a, ...)  COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_5(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_7(a, ...)  COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_6(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_8(a, ...)  COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_7(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_9(a, ...)  COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_8(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_10(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_9(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_11(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_10(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_12(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_11(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_13(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_12(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_14(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_13(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_15(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_14(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_16(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_15(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_17(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_16(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_18(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_17(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_19(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_18(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_20(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_19(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_21(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_20(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_22(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_21(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_23(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_22(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_24(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_23(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_25(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_24(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_26(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_25(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_27(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_26(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_28(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_27(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_29(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_28(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_30(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_29(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_31(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_30(__VA_ARGS__,)))
#define Z_IS_ENABLED_ANY_32(a, ...) COND_CODE_1(a, (1), (Z_IS_ENABLED_ANY_31(__VA_ARGS__,)))

/* Helper macro for IF_ENABLED_ALL */
#define Z_IF_ENABLED_ALL(_code, ...) \
	Z_IF_ENABLED_ALL_N(NUM_VA_ARGS(__VA_ARGS__), _code, __VA_ARGS__)
#define Z_IF_ENABLED_ALL_N(N, _code, ...) \
	UTIL_CAT(Z_IF_ENABLED_ALL_, N)(_code, __VA_ARGS__)
#define IF_ENABLED_ALL_0(_code, ...)
#define Z_IF_ENABLED_ALL_1(_code, a, ...) \
	COND_CODE_1(a, _code, ())
#define Z_IF_ENABLED_ALL_2(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_1(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_3(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_2(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_4(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_3(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_5(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_4(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_6(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_5(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_7(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_6(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_8(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_7(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_9(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_8(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_10(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_9(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_11(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_10(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_12(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_11(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_13(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_12(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_14(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_13(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_15(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_14(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_16(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_15(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_17(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_16(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_18(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_17(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_19(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_18(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_20(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_19(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_21(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_20(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_22(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_21(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_23(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_22(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_24(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_23(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_25(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_24(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_26(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_25(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_27(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_26(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_28(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_27(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_29(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_28(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_30(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_29(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_31(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_30(_code, __VA_ARGS__,)), ())
#define Z_IF_ENABLED_ALL_32(_code, a, ...) \
	COND_CODE_1(a, (Z_IF_ENABLED_ALL_31(_code, __VA_ARGS__,)), ())

/* Helper macro for IF_ENABLED_ANY */
#define Z_IF_ENABLED_ANY(_code, ...) \
	Z_IF_ENABLED_ANY_N(NUM_VA_ARGS(__VA_ARGS__), _code, __VA_ARGS__)
#define Z_IF_ENABLED_ANY_N(N, _code, ...) \
	UTIL_CAT(Z_IF_ENABLED_ANY_, N)(_code, __VA_ARGS__)
#define Z_IF_ENABLED_ANY_0(_code, ...)
#define Z_IF_ENABLED_ANY_1(_code, a, ...) \
	COND_CODE_1(a, _code, ())
#define Z_IF_ENABLED_ANY_2(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_1(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_3(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_2(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_4(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_3(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_5(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_4(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_6(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_5(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_7(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_6(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_8(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_7(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_9(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_8(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_10(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_9(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_11(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_10(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_12(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_11(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_13(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_12(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_14(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_13(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_15(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_14(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_16(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_15(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_17(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_16(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_18(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_17(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_19(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_18(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_20(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_19(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_21(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_20(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_22(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_21(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_23(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_22(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_24(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_23(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_25(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_24(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_26(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_25(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_27(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_26(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_28(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_27(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_29(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_28(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_30(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_29(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_31(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_30(_code, __VA_ARGS__,)))
#define Z_IF_ENABLED_ANY_32(_code, a, ...) \
	COND_CODE_1(a, _code, (Z_IF_ENABLED_ANY_31(_code, __VA_ARGS__,)))

#define UTIL_AND_CAT(a, b) a && b
/* Used by UTIL_CONCAT_AND */
#define Z_UTIL_CONCAT_AND(...) \
	(Z_UTIL_CONCAT_AND_N(NUM_VA_ARGS(__VA_ARGS__), __VA_ARGS__))
#define Z_UTIL_CONCAT_AND_N(N, ...) UTIL_CAT(Z_UTIL_CONCAT_AND_, N)(__VA_ARGS__)
#define Z_UTIL_CONCAT_AND_0
#define Z_UTIL_CONCAT_AND_1(a, ...)  a
#define Z_UTIL_CONCAT_AND_2(a, ...)  UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_1(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_3(a, ...)  UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_2(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_4(a, ...)  UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_3(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_5(a, ...)  UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_4(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_6(a, ...)  UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_5(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_7(a, ...)  UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_6(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_8(a, ...)  UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_7(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_9(a, ...)  UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_8(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_10(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_9(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_11(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_10(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_12(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_11(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_13(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_12(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_14(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_13(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_15(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_14(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_16(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_15(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_17(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_16(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_18(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_17(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_19(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_18(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_20(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_19(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_21(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_20(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_22(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_21(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_23(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_22(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_24(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_23(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_25(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_24(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_26(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_25(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_27(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_26(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_28(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_27(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_29(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_28(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_30(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_29(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_31(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_30(__VA_ARGS__,))
#define Z_UTIL_CONCAT_AND_32(a, ...) UTIL_AND_CAT(a, Z_UTIL_CONCAT_AND_31(__VA_ARGS__,))

#define UTIL_OR_CAT(a, b) a || b
/* Used by UTIL_CONCAT_OR */
#define Z_UTIL_CONCAT_OR(...) \
	(Z_UTIL_CONCAT_OR_N(NUM_VA_ARGS(__VA_ARGS__), __VA_ARGS__))
#define Z_UTIL_CONCAT_OR_N(N, ...) UTIL_CAT(Z_UTIL_CONCAT_OR_, N)(__VA_ARGS__)
#define Z_UTIL_CONCAT_OR_0
#define Z_UTIL_CONCAT_OR_1(a, ...)  a
#define Z_UTIL_CONCAT_OR_2(a, ...)  UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_1(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_3(a, ...)  UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_2(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_4(a, ...)  UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_3(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_5(a, ...)  UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_4(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_6(a, ...)  UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_5(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_7(a, ...)  UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_6(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_8(a, ...)  UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_7(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_9(a, ...)  UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_8(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_10(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_9(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_11(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_10(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_12(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_11(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_13(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_12(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_14(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_13(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_15(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_14(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_16(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_15(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_17(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_16(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_18(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_17(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_19(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_18(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_20(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_19(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_21(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_20(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_22(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_21(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_23(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_22(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_24(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_23(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_25(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_24(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_26(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_25(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_27(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_26(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_28(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_27(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_29(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_28(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_30(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_29(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_31(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_30(__VA_ARGS__,))
#define Z_UTIL_CONCAT_OR_32(a, ...) UTIL_OR_CAT(a, Z_UTIL_CONCAT_OR_31(__VA_ARGS__,))

#endif /* UTIL_INTERNAL_H_ */
