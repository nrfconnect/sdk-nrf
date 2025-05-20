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

#define BUF_LEN ((32*1024) + 10) /* Some pages, and then some */

/* Only 'a' */
static const uint8_t data[BUF_LEN] = {[0 ... BUF_LEN - 1] = 'a'};

ZTEST(pcd_test, test_pcd_network_core_update)
{
	enum pcd_status status;
	int err;

	/* Since image does not have fw_info the update will fail */
	err = pcd_network_core_update(&data, sizeof(data));
	zassert_not_equal(err, 0, "Unexpected success");

	status = pcd_fw_copy_status_get();
	zassert_equal(status, PCD_STATUS_FAILED, "Unexpected success");

	err = pcd_network_core_app_version((uint8_t *)&data, sizeof(data));
	zassert_not_equal(err, 0, "Unexpected success");

	status = pcd_fw_copy_status_get();
	zassert_equal(status, PCD_STATUS_FAILED, "Unexpected success");
}

ZTEST_SUITE(pcd_test, NULL, NULL, NULL, NULL, NULL);
