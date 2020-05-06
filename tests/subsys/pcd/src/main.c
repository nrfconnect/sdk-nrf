/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <ztest.h>
#include <string.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <devicetree.h>
#include <drivers/flash.h>
#include <dfu/pcd.h>

#define FLASH_NAME DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL

#define BUF_LEN ((32*1024) + 10) /* Some pages, and then some */

/* Only 'a' */
static const u8_t data[BUF_LEN] = {[0 ... BUF_LEN - 1] = 'a'};
static u8_t read_buf[sizeof(data)];

static void test_pcd_get_cmd(void)
{
	u32_t buf;

	buf = 0;
	struct pcd_cmd *cmd = pcd_get_cmd(&buf);

	zassert_equal(cmd, NULL, "should be NULL");

	buf = PCD_CMD_MAGIC_COPY;
	cmd = pcd_get_cmd(&buf);
	zassert_not_equal(cmd, NULL, "should not be NULL");
}

static void test_pcd_invalidate(void)
{
	bool ret;
	struct pcd_cmd cmd;

	/* Normal use case */
	ret = pcd_invalidate(&cmd);
	zassert_true(ret == 0, "Unexpected failure");
	zassert_equal(PCD_CMD_MAGIC_FAIL, cmd.magic, "Incorrect magic");

	/* NULL argument */
	ret = pcd_invalidate(NULL);
	zassert_true(ret != 0, "should return false since NULL param");
}

static void test_pcd_transfer(void)
{
	int rc;

	struct pcd_cmd cmd = {.src = (const void *)data,
				 .len = sizeof(data),
				 .offset = 0x10000};

	struct device *fdev = device_get_binding(FLASH_NAME);

	zassert_true(fdev != NULL, "fdev is NULL");
	rc = pcd_transfer(&cmd, fdev);
	zassert_equal(rc, 0, "Unexpected failure");

	rc = flash_read(fdev, 0x10000, read_buf, sizeof(data));
	zassert_equal(rc, 0, "Unexpected failure");

	zassert_true(memcmp((const void *)data, (const void *)read_buf,
			    sizeof(data)) == 0, "neq");

	zassert_equal(cmd.magic, PCD_CMD_MAGIC_DONE,
		      "Magic does not indicate success");
}

void test_main(void)
{
	ztest_test_suite(pcd_test,
			 ztest_unit_test(test_pcd_get_cmd),
			 ztest_unit_test(test_pcd_invalidate),
			 ztest_unit_test(test_pcd_transfer)
			 );

	ztest_run_test_suite(pcd_test);
}
