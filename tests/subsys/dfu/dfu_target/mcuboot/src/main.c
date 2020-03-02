/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <ztest.h>
#include <string.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <dfu/dfu_target.h>

#define FILE_SIZE 0x1000

static int offset_get_retval;
static size_t offset_get_out_param;
static int write_retval;
static int write_param_len;
static void const *write_param_buf;
static int done_retval;
static int init_retval;
static bool identify_retval;

bool dfu_target_mcuboot_identify(const void *const buf)
{
	return identify_retval;
}

int dfu_target_mcuboot_init(size_t file_size, dfu_target_callback_t cb)
{
	return init_retval;
}

int dfu_target_mcuboot_offset_get(size_t *offset)
{
	*offset = offset_get_out_param;
	return offset_get_retval;
}

int dfu_target_mcuboot_write(const void *const buf, size_t len)
{
	write_param_buf = buf;
	write_param_len = len;
	return write_retval;
}

int dfu_target_mcuboot_done(bool successful)
{
	return done_retval;
}

static void init(void)
{
	int err;
	int ret;

	/* Return 'true' when dfu_target_mcuboot_identify() is called */
	identify_retval = true;

	ret = dfu_target_img_type(0, 0);

	zassert_true(ret > 0, "Valid type not recognized");

	err = dfu_target_init(ret, FILE_SIZE, NULL);
	zassert_equal(err, 0, NULL);
}

static void done(void)
{
	(void)dfu_target_done(true);
}

static void test_init(void)
{
	int ret;
	int err;

	done();
	init_retval = 0;
	done_retval = 0;

	identify_retval = true;
	ret = dfu_target_img_type(0, 0);
	zassert_true(ret > 0, "Valid type not recognized");

	err = dfu_target_init(ret, FILE_SIZE, NULL);
	zassert_equal(err, 0, NULL);

	err = dfu_target_init(ret, FILE_SIZE, NULL);
	zassert_equal(err, 0, "Re-initialization should pass");

	err = dfu_target_done(true);
	zassert_equal(err, 0, "Should not get error code");

	/* Now clean up and try invalid types */
	done();

	err = dfu_target_init(0, FILE_SIZE, NULL);
	zassert_true(err < 0, "Did not fail when invalid type is used");

	done();

	err = dfu_target_init(2, FILE_SIZE, NULL);
	zassert_true(err < 0, "Did not fail when invalid type is used");

	init_retval = -42;

	err = dfu_target_init(ret, FILE_SIZE, NULL);
	zassert_equal(err, -42, "Did not return error code from target");

	done();
}

static void test_done(void)
{
	int err;

	/* Un-initialize */
	dfu_target_done(true);

	err = dfu_target_done(true);
	zassert_true(err < 0, "Should get error code when not initialized");

	init();
	err = dfu_target_done(true);
	zassert_equal(err, 0, "Should not get error code when initialized");
	done();

	/* Verify that passing 'false' does not de-initialize */
	init();
	err = dfu_target_done(false);
	zassert_equal(err, 0, NULL);
	err = dfu_target_done(true);
	zassert_equal(err, 0, NULL);

	init();
	done_retval = -42;
	err = dfu_target_done(true);
	zassert_equal(err, -42, "Did not get error from dfu target");
	done();

}

static void test_offset_get(void)
{
	int err;
	size_t offset;

	init();

	offset_get_retval = 0;
	offset_get_out_param = 42;
	err = dfu_target_offset_get(&offset);
	zassert_equal(err, 0, NULL);
	zassert_equal(offset, 42, NULL);

	offset_get_retval = -42;
	err = dfu_target_offset_get(&offset);
	zassert_equal(err, -42, NULL);

	/* De-initialize */
	done();
	offset_get_retval = 0;
	err = dfu_target_offset_get(&offset);
	zassert_true(err < 0, "Expected negative error code");
}

static void test_write(void)
{
	int err;
	int mybuf[100];

	init();
	err = dfu_target_write(mybuf, sizeof(mybuf));
	zassert_equal_ptr(write_param_buf, mybuf, NULL);
	zassert_equal(write_param_len, sizeof(mybuf), NULL);

	done(); /* De-initialize */
	err = dfu_target_write(mybuf, sizeof(mybuf));
	zassert_true(err < 0, "Did not get error when writing uninitialized");
}

void test_main(void)
{
	ztest_test_suite(dfu_target_test,
			 ztest_unit_test(test_write),
			 ztest_unit_test(test_offset_get),
			 ztest_unit_test(test_done),
			 ztest_unit_test(test_init)
			 );

	ztest_run_test_suite(dfu_target_test);
}
