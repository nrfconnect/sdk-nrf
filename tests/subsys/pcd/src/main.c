/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <string.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <devicetree.h>
#include <drivers/flash.h>
#include <dfu/pcd.h>

#define BUF_LEN ((32*1024) + 10) /* Some pages, and then some */

/* Only 'a' */
static const uint8_t data[BUF_LEN] = {[0 ... BUF_LEN - 1] = 'a'};

static void test_pcd_network_core_update(void)
{
	enum pcd_status status;
	int err;

	err = pcd_network_core_update(&data, sizeof(data));
	zassert_equal(err, 0, "Unexpected failure");

	status = pcd_fw_copy_status_get();
	zassert_equal(status, PCD_STATUS_COPY_DONE, "Unexpected failure");

	/* Invalidate the CMD */
	pcd_fw_copy_invalidate();
	status = pcd_fw_copy_status_get();
	zassert_equal(status, PCD_STATUS_COPY_FAILED, "Unexpected success");
}


void test_main(void)
{
	ztest_test_suite(pcd_test,
			 ztest_unit_test(test_pcd_network_core_update)
			 );

	ztest_run_test_suite(pcd_test);
}
