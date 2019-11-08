/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <ztest.h>
#include <dfu/dfu_target.h>

#include <dfu/flash_img.h>

static const u32_t mcuboot_header[] = {0x96f3b83d};

static void init(void)
{
	int ret = dfu_target_img_type(mcuboot_header, sizeof(mcuboot_header));

	zassert_true(ret > 0, "Valid type not recognized");

	ret = dfu_target_init(ret);
	zassert_equal(ret, 0, NULL);
}

static void done(void)
{
	(void)dfu_target_done();
}

static void test_init(void)
{
	int ret;
	int err;

	done();

	ret = dfu_target_img_type(mcuboot_header, sizeof(mcuboot_header));
	zassert_true(ret > 0, "Valid type not recognized");

	err = dfu_target_init(ret);
	zassert_equal(err, 0, NULL);

	err = dfu_target_init(ret);
	zassert_equal(err, -EBUSY, "Should return -EBUSY when initialized");

	err = dfu_target_done();
	zassert_equal(err, 0, "Should not get error code");

	/* Now clean up and try invalid types */
	done();

	err = dfu_target_init(0);
	zassert_true(err < 0, "Did not fail when invalid type is used");

	done();

	err = dfu_target_init(2);
	zassert_true(err < 0, "Did not fail when invalid type is used");

	done();
}

static void test_done(void)
{
	int err;

	err = dfu_target_done();
	zassert_true(err != 0, "Should get error code when not initialized");

	init();
	err = dfu_target_done();
	zassert_true(err == 0, "Should not get error code when initialized");

	done();
}

static void test_write(void)
{
	size_t offset;
	int err;
	int mybuf[100];

	done();

	err = dfu_target_write(mybuf, sizeof(mybuf));
	zassert_true(err < 0, "Did not get error when writing uninitialized");

	init();

	err = dfu_target_write(NULL, sizeof(mybuf));
	zassert_true(err != 0, "Did not get error when writing NULL");

	err = dfu_target_write(mybuf, 10);
	zassert_true(err == 0, "Got error when writing");

	err = dfu_target_offset_get(&offset);
	zassert_true(err == 0, "Got error when getting offset");

	zassert_equal(offset, 10, "Incorrect offset");

	err = dfu_target_write(mybuf, 10);
	zassert_true(err == 0, "Got error when writing");

	err = dfu_target_offset_get(&offset);
	zassert_true(err == 0, "Got error when getting offset");

	zassert_equal(offset, 20, "Incorrect offset");

	/* Re - initialize and verify that offset has been reset */
	done();
	init();

	err = dfu_target_write(mybuf, 10);
	zassert_true(err == 0, "Got error when writing");

	err = dfu_target_offset_get(&offset);
	zassert_true(err == 0, "Got error when getting offset");

	done();
}

void test_main(void)
{
	ztest_test_suite(lib_json_test,
			 ztest_unit_test(test_write),
			 ztest_unit_test(test_done),
			 ztest_unit_test(test_init)
			 );

	ztest_run_test_suite(lib_json_test);
}
