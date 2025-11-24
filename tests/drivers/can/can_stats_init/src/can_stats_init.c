/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>

#include <zephyr/ztest.h>

#define EXPECTED  0

static const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

/* Test that CAN statistics are cleared during driver initialization.
 *
 * Since CAN start/stop will clear CAN stats, it's crucial this test
 * is executed immediately after CAN driver initialization.
 */
ZTEST_USER(can_stats_init, test_can_stats_cleared_during_driver_init)
{
	uint32_t val;

	val = can_stats_get_bit_errors(can_dev);
	zassert_true(val == EXPECTED, "CAN bit errors are too high (%u)", val);
	val = can_stats_get_bit0_errors(can_dev);
	zassert_true(val == EXPECTED, "CAN bit0 errors are too high (%u)", val);
	val = can_stats_get_bit1_errors(can_dev);
	zassert_true(val == EXPECTED, "CAN bit1 errors are too high (%u)", val);
	val = can_stats_get_stuff_errors(can_dev);
	zassert_true(val == EXPECTED, "CAN stuff errors are too high (%u)", val);
	val = can_stats_get_crc_errors(can_dev);
	zassert_true(val == EXPECTED, "CAN crc errors are too high (%u)", val);
	val = can_stats_get_form_errors(can_dev);
	zassert_true(val == EXPECTED, "CAN form errors are too high (%u)", val);
	val = can_stats_get_ack_errors(can_dev);
	zassert_true(val == EXPECTED, "CAN ack errors are too high (%u)", val);
	val = can_stats_get_rx_overruns(can_dev);
	zassert_true(val == EXPECTED, "CAN rx overruns are too high (%u)", val);
}

void *suite_setup(void)
{
	zassert_true(device_is_ready(can_dev), "CAN device not ready");

	return NULL;
}

ZTEST_SUITE(can_stats_init, NULL, suite_setup, NULL, NULL, NULL);
