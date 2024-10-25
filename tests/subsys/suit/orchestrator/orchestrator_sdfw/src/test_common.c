/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include <suit_execution_mode.h>

void check_startup_failure(void)
{
	suit_execution_mode_t mode_before = suit_execution_mode_get();
	bool has_failed = suit_execution_mode_failed();

	if (suit_execution_mode_booting()) {
		/* Update execution mode to leave transient states. */
		suit_execution_mode_startup_failed();

		/* If the device was booting - it should enter EXECUTION_MODE_FAIL_STARTUP state. */
		zassert_equal(false, suit_execution_mode_booting(),
			      "The device did not left boot mode");
		zassert_equal(false, suit_execution_mode_updating(),
			      "The device entered update mode");
		zassert_equal(true, suit_execution_mode_failed(),
			      "The device did not enter failed mode");
		zassert_equal(EXECUTION_MODE_FAIL_STARTUP, suit_execution_mode_get(),
			      "FAILED state not set after boot startup failed");
	} else if (suit_execution_mode_updating()) {
		/* Update execution mode to leave transient states. */
		suit_execution_mode_startup_failed();

		/* If the device was updating - it should enter EXECUTION_MODE_FAIL_STARTUP state.
		 */
		zassert_equal(false, suit_execution_mode_booting(), "The device entered boot mode");
		zassert_equal(false, suit_execution_mode_updating(),
			      "The device did not left update mode");
		zassert_equal(true, suit_execution_mode_failed(),
			      "The device did not enter failed mode");
		zassert_equal(EXECUTION_MODE_FAIL_STARTUP, suit_execution_mode_get(),
			      "FAILED state not set after update startup failed");
	} else {
		/* Update execution mode to leave transient states. */
		suit_execution_mode_startup_failed();

		/* If the device was in final state - it should stay in the same state state. */
		zassert_equal(false, suit_execution_mode_booting(), "The device entered boot mode");
		zassert_equal(false, suit_execution_mode_updating(),
			      "The device entered update mode");
		zassert_equal(has_failed, suit_execution_mode_failed(),
			      "The device changed failed mode");
		zassert_equal(mode_before, suit_execution_mode_get(),
			      "Unexpected execution mode change");
	}
}
