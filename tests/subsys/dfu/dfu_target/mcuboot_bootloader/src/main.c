/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/ztest.h>
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
static int schedule_retval;

bool dfu_target_mcuboot_identify(const void *const buf)
{
	return identify_retval;
}

int dfu_target_mcuboot_init(size_t file_size, int img_num, dfu_target_callback_t cb)
{
	int ret = init_retval;

	init_retval = -1;
	write_retval = 0;
	offset_get_retval = 0;
	done_retval = 0;
	return ret;
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
	int ret = done_retval;

	write_retval = -1;
	offset_get_retval = -1;
	done_retval = -1;
	return ret;
}

int dfu_target_mcuboot_schedule_update(int img_num)
{
	return schedule_retval;
}

int dfu_target_mcuboot_reset(void)
{
	return 0;
}

void test_setup(void)
{
	init_retval = 0;
	done_retval = 0;
	schedule_retval = 0;
}

static void init(void)
{
	int err;
	int ret;
	uint8_t buf[64] = {0};

	dfu_target_reset();
	test_setup();

	/* Return 'true' when dfu_target_mcuboot_identify() is called */
	identify_retval = true;

	ret = dfu_target_img_type(buf, sizeof(buf));

	zassert_true(ret > 0, "Valid type not recognized");

	err = dfu_target_init(ret, 0, FILE_SIZE, NULL);
	zassert_equal(err, 0, NULL);
}

static void done(void)
{
	(void)dfu_target_done(true);
	(void)dfu_target_schedule_update(0);
}

ZTEST(dfu_target, test_init)
{
	int ret;
	int err;
	uint8_t buf[64] = {0};

	init();

	identify_retval = true;
	ret = dfu_target_img_type(buf, sizeof(buf));
	zassert_true(ret > 0, "Valid type not recognized");

	err = dfu_target_init(ret, 0, FILE_SIZE, NULL);
	zassert_equal(err, 0, NULL);

	err = dfu_target_init(ret, 0, FILE_SIZE, NULL);
	zassert_equal(err, 0, "Re-initialization should pass");

	err = dfu_target_done(true);
	zassert_equal(err, 0, "Should not get error code");

	/* Now clean up and try invalid types */
	done();

	err = dfu_target_init(DFU_TARGET_IMAGE_TYPE_ANY, 0, FILE_SIZE, NULL);
	zassert_true(err < 0, "Did not fail when invalid type is used");

	done();

	err = dfu_target_init(-1, 0, FILE_SIZE, NULL);
	zassert_true(err < 0, "Did not fail when invalid type is used");

	init_retval = -42;

	err = dfu_target_init(ret, 1, FILE_SIZE, NULL);
	zassert_equal(err, -42, "Did not return error code from target");

	done();
}

ZTEST(dfu_target, test_done)
{
	int err;

	/* Un-initialize */
	dfu_target_done(true);
	dfu_target_schedule_update(0);

	err = dfu_target_done(true);
	zassert_true(err < 0, "Should get error code when not initialized");

	init();
	err = dfu_target_done(true);
	zassert_equal(err, 0, "Should not get error code when initialized");
	done();

	init();
	done_retval = -42;
	err = dfu_target_done(true);
	zassert_equal(err, -42, "Did not get error from dfu target");
	done();

}

ZTEST(dfu_target, test_schedule)
{
	int err;

	init();
	dfu_target_done(true);
	err = dfu_target_schedule_update(0);
	zassert_equal(err, 0, "Should not get error code when initialized");
	done();

	init();
	schedule_retval = -91;
	err = dfu_target_schedule_update(0);
	zassert_equal(err, -91, "Did not get error from dfu target");
	done();

}

ZTEST(dfu_target, test_offset_get)
{
	int err;
	size_t offset = 0;

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
	err = dfu_target_offset_get(&offset);
	zassert_true(err < 0, "Expected negative error code");
}

ZTEST(dfu_target, test_write)
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

ZTEST_SUITE(dfu_target, NULL, NULL, NULL, NULL, NULL);
