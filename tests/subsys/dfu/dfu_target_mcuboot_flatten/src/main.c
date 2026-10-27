/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <dfu/dfu_target_mcuboot.h>
#include <dfu/dfu_target_stream.h>
#include <flatten_fake.h>

#define SLOT1_NODE DT_NODELABEL(slot1_partition)

#define EXPECTED_SEC_BASE  DT_REG_ADDR(SLOT1_NODE)
#define EXPECTED_SEC_SIZE  DT_REG_SIZE(SLOT1_NODE)
#define EXPECTED_LAST_BYTE (EXPECTED_SEC_BASE + EXPECTED_SEC_SIZE - 1)

#define TEST_FILE_SIZE 256

static uint8_t stream_buf[4096] __aligned(4);

static void flatten_fake_reset(void)
{
	RESET_FAKE(stream_flash_flatten_page);
}

static void mcuboot_stream_setup(void)
{
	int err;

	flatten_fake_reset();

	err = dfu_target_mcuboot_set_buf(stream_buf, sizeof(stream_buf));
	zassert_ok(err, NULL);

	err = dfu_target_mcuboot_init(TEST_FILE_SIZE, 0, NULL);
	zassert_ok(err, NULL);

	err = dfu_target_mcuboot_write((const uint8_t[]){ 0x3d, 0xb8, 0xf3, 0x96 }, 4);
	zassert_ok(err, NULL);
}

ZTEST(dfu_target_mcuboot_flatten, test_done_success_flattens_last_slot_byte)
{
	struct stream_flash_ctx *stream;
	int err;

	mcuboot_stream_setup();
	stream = dfu_target_stream_get_stream();

	err = dfu_target_mcuboot_done(true);
	zassert_ok(err, NULL);

	zassert_equal(stream_flash_flatten_page_fake.call_count, 1);
	zassert_equal_ptr(stream_flash_flatten_page_fake.arg0_val, stream);
	zassert_equal(stream_flash_flatten_page_fake.arg1_val, EXPECTED_LAST_BYTE);
	zassert_equal(stream_flash_flatten_page_fake.arg1_val,
		      stream->offset + stream->available - 1);
}

ZTEST(dfu_target_mcuboot_flatten, test_done_abort_does_not_flatten)
{
	int err;

	mcuboot_stream_setup();

	err = dfu_target_mcuboot_done(false);
	zassert_ok(err, NULL);

	zassert_equal(stream_flash_flatten_page_fake.call_count, 0);
}

ZTEST(dfu_target_mcuboot_flatten, test_reset_flattens_partition_base)
{
	struct stream_flash_ctx *stream;
	int err;

	mcuboot_stream_setup();
	stream = dfu_target_stream_get_stream();

	err = dfu_target_mcuboot_reset();
	zassert_ok(err, NULL);

	zassert_equal(stream_flash_flatten_page_fake.call_count, 1);
	zassert_equal_ptr(stream_flash_flatten_page_fake.arg0_val, stream);
	zassert_equal(stream_flash_flatten_page_fake.arg1_val, EXPECTED_SEC_BASE);
	zassert_equal(stream_flash_flatten_page_fake.arg1_val, stream->offset);
}

ZTEST(dfu_target_mcuboot_flatten, test_done_propagates_flatten_error)
{
	int err;

	mcuboot_stream_setup();
	stream_flash_flatten_page_fake.return_val = -EIO;

	err = dfu_target_mcuboot_done(true);
	zassert_equal(err, -EIO, NULL);
	zassert_equal(stream_flash_flatten_page_fake.call_count, 1);
}

ZTEST_SUITE(dfu_target_mcuboot_flatten, NULL, NULL, NULL, NULL, NULL);
