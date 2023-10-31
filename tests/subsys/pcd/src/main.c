/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <string.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <dfu/pcd.h>
#include <hal/nrf_reset.h>
#include <hal/nrf_spu.h>

#define BUF_LEN ((32*1024) + 10) /* Some pages, and then some */

/* Only 'a' */
static const uint8_t data[BUF_LEN] = {[0 ... BUF_LEN - 1] = 'a'};
ZTEST(pcd_test, test_pcd_network_core_app_version_on_stack)
{
	int err;
	enum pcd_status status;
	uint32_t version;

	err = pcd_network_core_app_version((uint8_t *)&version, sizeof(version));
	zassert_equal(err, 0, "Expected success");
	zassert_equal(version, 3, "Wrong version number");

	status = pcd_cmd_status_get();
	zassert_equal(status, PCD_STATUS_DONE, "Expected success");
}

ZTEST(pcd_test, test_pcd_network_core_app_version_in_flash)
{
	int err;
	enum pcd_status status;

	err = pcd_network_core_app_version((uint8_t *)&data, sizeof(data));
	zassert_not_equal(err, 0, "Unexpected success");

	status = pcd_cmd_status_get();
	zassert_equal(status, PCD_STATUS_FAILED, "Unexpected success");
}

ZTEST(pcd_test, test_pcd_network_core_update)
{
	enum pcd_status status;
	int err;

	/* Since image does not have fw_info the update will fail */
	err = pcd_network_core_update(&data, sizeof(data));
	zassert_not_equal(err, 0, "Unexpected success");

	status = pcd_cmd_status_get();
	zassert_equal(status, PCD_STATUS_FAILED, "Unexpected success");
}

static void pcd_test_turn_off_network_core(void *f)
{
	nrf_reset_network_force_off(NRF_RESET, false);
}

ZTEST_SUITE(pcd_test, NULL, NULL, pcd_test_turn_off_network_core, NULL, NULL);
