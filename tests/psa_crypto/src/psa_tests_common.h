/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include "common.h"
#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/entropy.h>

#if defined(CONFIG_ZTEST)
#include <zephyr/ztest.h>
#endif

#define APP_SUCCESS		(0)
#define APP_ERROR		(-1)
#define APP_SUCCESS_MESSAGE "Example finished successfully!"
#define APP_ERROR_MESSAGE "Example exited with error!"

#ifndef TEST_MEMCMP
#define TEST_MEMCMP(x, y, size) ((memcmp(x, y, size) == 0) ? 0 : 1)
#endif

/**@brief Macro asserting equality.
 */
#ifndef TEST_VECTOR_ASSERT_EQUAL
#define TEST_VECTOR_ASSERT_EQUAL(expected, actual)                             \
	zassert_equal((expected), (actual),                                    \
			  "\tAssert values: 0x%04X != -0x%04X", (expected),        \
			  (-actual))
#endif

/**@brief Macro asserting inequality.
 */
#ifndef TEST_VECTOR_ASSERT_NOT_EQUAL
#define TEST_VECTOR_ASSERT_NOT_EQUAL(expected, actual)                         \
	zassert_not_equal((expected), (actual),                                \
			  "\tAssert values: 0x%04X == -0x%04X", (expected),    \
			  (-actual))
#endif

int crypto_finish(void);
int crypto_init(void);
