/*
 * Copyright (c) 2011-2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Nordic macro utilities
 *
 * Additional utility macros
 */

#ifndef UTIL_MACROS_H_
#define UTIL_MACROS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <util/util_internal.h>
#include <zephyr/sys/util_macro.h>

/**
 * @addtogroup sys-util
 * @{
 */

/**
 * @brief Check multiple macro definitions to see if all are set in
 *        compiler-visible expressions
 *
 * This utilizes the same methodology as IS_ENABLED but on an variable number
 * of argument. It will resolve as true (1) if all of the arguments in the input
 * are defined to 1.
 *
 * Example usage:
 *
 *     if (IS_ENABLED_ALL(CONFIG_FOO, CONFIG_BAR)) {
 *             do_something_if_all_of_these
 *     }
 *
 * This is cleaner since the compiler can generate errors and warnings
 * for @p do_something_if_all_of_these even when @p CONFIG_FOO and/or
 * @p CONFIG_BAR is undefined.
 *
 * @param ... Arguments to check if all are defined to 1
 * @return 1 if all params in @p ... are defined to 1, otherwise 0
 */
#define IS_ENABLED_ALL(...) Z_IS_ENABLED_ALL(__VA_ARGS__)

/**
 * @brief Check multiple macro definitions to see if any is set in
 *        compiler-visible expressions
 *
 * This utilizes the same methodology as IS_ENABLED but on an variable number
 * of argument. It will resolve as true (1) if any of the arguments in the input
 * are defined to 1.
 *
 * Example usage:
 *
 *     if (IS_ENABLED_ANY(CONFIG_FOO, CONFIG_BAR)) {
 *             do_something_if_any_of_these
 *     }
 *
 * This is cleaner since the compiler can generate errors and warnings
 * for @p do_something_if_any_of_these even when @p CONFIG_FOO and/or
 * @p CONFIG_BAR is undefined.
 *
 * @param ... Arguments to check if at least one is defined to 1
 * @return 1 if at least one param in @p ... is defined to 1, otherwise 0
 */
#define IS_ENABLED_ANY(...) Z_IS_ENABLED_ANY(__VA_ARGS__)

/**
 * @brief Insert code if all @p _flags are defined and equals 1.
 *
 * Like IF_ENABLED(), this emits code if @p _flags are all defined  and
 * set to 1; it expands to nothing otherwise.
 *
 * Example:
 *
 *     IF_ENABLED_ALL((CONFIG_FLAG_A, CONFIG_FLAG_B), uint32_t foo;)
 *
 * If @p CONFIG_FLAG_A and @p CONFIG_FLAG_B are defined to 1, this expands to:
 *
 *     uint32_t foo;
 *
 * and to nothing otherwise.
 *
 * @param _flags evaluated flags; must be in parentheses
 * @param ... Emitted code if all @p _flags are defined to 1
 */
#define IF_ENABLED_ALL(_flags, ...) \
	Z_IF_ENABLED_ALL((__VA_ARGS__), __DEBRACKET _flags)

/**
 * @brief Insert code if any @p _flags are defined to 1.
 *
 * Like IF_ENABLED(), this emits code if any @p _flags are defined to 1;
 * it expands to nothing otherwise.
 *
 * Example:
 *
 *     IF_ENABLED_ANY((CONFIG_FLAG_A, CONFIG_FLAG_B), uint32_t foo;)
 *
 * If @p CONFIG_FLAG_A or @p CONFIG_FLAG_B defined to 1, this expands to:
 *
 *     uint32_t foo;
 *
 * and to nothing otherwise.
 *
 * @param _flags evaluated flags; must be in parentheses
 * @param ... Emitted code if any @p _flags expands to 1
 */
#define IF_ENABLED_ANY(_flags, ...) \
	Z_IF_ENABLED_ANY((__VA_ARGS__), __DEBRACKET _flags)

/**
 * @brief Concatenates all arguments with "&&" between them into a single token
 *
 * This macro does primitive concatenation and doesn't try to process the
 * parameters in preprocessor.
 *
 * @param ... Arguments to concatenate with @p && between them
 * @return A concatenation of all the arguments with @p && between them or a
 *         single argument
 */
#define UTIL_CONCAT_AND(...) Z_UTIL_CONCAT_AND(__VA_ARGS__)

/**
 * @brief Concatenates all arguments with "||" between them into a single token
 *
 * This macro does primitive concatenation and doesn't try to process the
 * parameters in preprocessor.
 *
 * @param ... Arguments to concatenate with @p || between them
 * @return A concatenation of all the arguments with @p || between them or a
 *         single argument
 */
#define UTIL_CONCAT_OR(...) Z_UTIL_CONCAT_OR(__VA_ARGS__)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* UTIL_MACROS_H_ */
