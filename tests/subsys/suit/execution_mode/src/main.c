/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <stdint.h>
#include <suit_execution_mode.h>

ZTEST_SUITE(suit_execution_mode_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(suit_execution_mode_tests, test_suit_execution_mode_get_before_set)
{
	suit_execution_mode_t mode = suit_execution_mode_get();

	zassert_equal(mode, EXECUTION_MODE_STARTUP,
		      "suit_execution_mode_get returned unexpected value: %i", mode);
}

ZTEST(suit_execution_mode_tests, test_suit_execution_mode_set_NOK)
{
	suit_plat_err_t err = suit_execution_mode_set(EXECUTION_MODE_STARTUP - 1);

	zassert_equal(err, SUIT_PLAT_ERR_INVAL,
		      "suit_execution_mode_set should have failed - invalid input");

	err = suit_execution_mode_set(EXECUTION_MODE_FAIL_INSTALL_NORDIC_TOP + 1);
	zassert_equal(err, SUIT_PLAT_ERR_INVAL,
		      "suit_execution_mode_set should have failed - invalid input");
}

ZTEST(suit_execution_mode_tests, test_suit_execution_mode_set_OK)
{
	suit_plat_err_t err = suit_execution_mode_set(EXECUTION_MODE_INVOKE);

	zassert_equal(err, SUIT_PLAT_SUCCESS, "suit_execution_mode_set failed: %i", err);

	suit_execution_mode_t mode = suit_execution_mode_get();

	zassert_equal(mode, EXECUTION_MODE_INVOKE,
		      "suit_execution_mode_get returned unexpected value: %i", mode);
}
