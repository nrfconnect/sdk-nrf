/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/types.h>
#include <zephyr/drivers/flash.h>
#include <stdbool.h>
#include <zephyr/ztest.h>
#include <dfu/dfu_target_stream.h>

#define FLASH_BASE (64*1024)
#define FLASH_SIZE DT_REG_SIZE(SOC_NV_FLASH_NODE)
#define FLASH_AVAILABLE (FLASH_SIZE-FLASH_BASE)

#define TEST_ID_1 "test_1"
#define TEST_ID_2 "test_2"

#define BUF_LEN 14000 /* Note, not page aligned */

static const struct device *fdev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
static uint8_t sbuf[128];
static uint8_t read_buf[BUF_LEN];
static uint8_t write_buf[BUF_LEN] = {[0 ... BUF_LEN - 1] = 0xaa};

#ifdef CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS
static int page_size;
#endif

#define DFU_TARGET_STREAM_INIT(id_, fdev_, buf_, len_, offset_, size_, cb_)  \
	dfu_target_stream_init(&(struct dfu_target_stream_init) { .id = id_, \
		.fdev = fdev_, .buf = buf_, .len = len_, .offset = offset_,  \
		.size = size_, .cb = cb_})

ZTEST(dfu_target_stream_test, test_dfu_target_stream)
{
	int err;
	size_t offset;

	/* Null checks */
	err = DFU_TARGET_STREAM_INIT(NULL, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_true(err < 0, "Unexpected success: %d", err);

	err = DFU_TARGET_STREAM_INIT(TEST_ID_1, NULL, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_true(err < 0, "Unexpected success: %d", err);

	err = DFU_TARGET_STREAM_INIT(TEST_ID_1, fdev, NULL, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_true(err < 0, "Unexpected success: %d", err);

	/* Expected successful call */
	err = DFU_TARGET_STREAM_INIT(TEST_ID_1, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	/* Call _init again without calling "complete". This should result
	 * in an error since only one id is supported simultaneously
	 */
	err = DFU_TARGET_STREAM_INIT(TEST_ID_2, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_true(err < 0, "Unexpected success: %d", err);

	/* Perform write, and verify offset */
	err = dfu_target_stream_write(write_buf, sizeof(write_buf));
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	err = dfu_target_stream_offset_get(&offset);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	/* Since 'write_buf' is not page aligned, the 'offset' should not
	 * correspond to the sice written.
	 */
	zassert_not_equal(offset, sizeof(write_buf), "Invalid offset");

	/* Complete transfer */
	err = dfu_target_stream_done(true);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	/* Call _init again after calling "_done". This should NOT result
	 * in an error since the id should be reset, and the setting
	 * should be deleted.
	 */
	err = DFU_TARGET_STREAM_INIT(TEST_ID_2, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	/* Read out the data to ensure that it was written correctly */
	err = flash_read(fdev, FLASH_BASE, read_buf, BUF_LEN);
	zassert_equal(err, 0, "Unexpected failure: %d", err);
	zassert_mem_equal(read_buf, write_buf, BUF_LEN, "Incorrect value");
}

#ifdef CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS
ZTEST(dfu_target_stream_test, test_dfu_target_stream_save_progress)
{
	int err;
	size_t first_offset;
	size_t second_offset;
	const struct stream_flash_ctx *ctx = NULL;
	off_t erased_page_offset;

	/* Reset state to avoid failure when initializing */
	err = dfu_target_stream_done(true);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	err = DFU_TARGET_STREAM_INIT(TEST_ID_1, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	/* Perform write, and verify offset */
	err = dfu_target_stream_write(write_buf, sizeof(write_buf)/2);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	err = dfu_target_stream_offset_get(&first_offset);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	/* Complete transfer with failure */
	err = dfu_target_stream_done(false);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	/* Re-initialize dfu target, don't perform any write operation, and
	 * verify that the offsets are the same.
	 */
	err = DFU_TARGET_STREAM_INIT(TEST_ID_1, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	err = dfu_target_stream_offset_get(&second_offset);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	zassert_equal(first_offset, second_offset, "Offsets do not match");

	/* Complete transfer with success */
	err = dfu_target_stream_done(true);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	/* Re-initialize dfu target, don't perform any write operation, and
	 * verify that the offset is now 0, since we had a succesfull 'done'.
	 */
	err = DFU_TARGET_STREAM_INIT(TEST_ID_1, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	err = dfu_target_stream_offset_get(&second_offset);
	zassert_equal(err, 0, "Unexpected failure: %d", err);
	zassert_equal(0, second_offset, "Offsets do not match");

	/* Next we ensure that changing the target results in the offset
	 * being reset
	 */

	/* Complete transfer with success */
	err = dfu_target_stream_done(true);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	/* Re-initialize dfu target, for 'TEST_ID_1' */
	err = DFU_TARGET_STREAM_INIT(TEST_ID_1, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	/* Verify that offset is 0 */
	err = dfu_target_stream_offset_get(&first_offset);
	zassert_equal(err, 0, "Unexpected failure: %d", err);
	zassert_equal(0, first_offset, "Offsets do not match");

	/* Perform write, and verify that offset has changed*/
	err = dfu_target_stream_write(write_buf, sizeof(write_buf));
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	/* Verify that the offset is not 0 anymore */
	err = dfu_target_stream_offset_get(&second_offset);
	zassert_equal(err, 0, "Unexpected failure: %d", err);
	zassert_not_equal(second_offset, first_offset, "Offset not updated");

	/* Complete transfer with failure, this retains the non-0 offset */
	err = dfu_target_stream_done(false);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	/* Re-initialize dfu target, for 'TEST_ID_2', verify that the offset
	 * loaded is 0 even though it was just retained an non-0 for TEST_ID_1
	 */
	err = DFU_TARGET_STREAM_INIT(TEST_ID_2, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	err = dfu_target_stream_offset_get(&first_offset);
	zassert_equal(err, 0, "Unexpected failure: %d", err);
	zassert_equal(0, first_offset, "Offsets has not been reset");

	/* Next, check that writing zero bytes does not change the last page
	 * erased.
	 */
	err = dfu_target_stream_write(write_buf, 0);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	/* Store the last erased page start offset from before load. */
	ctx = dfu_target_stream_get_stream();
	zassert_not_null(ctx, "Expected non-null ctx.");
	erased_page_offset = ctx->last_erased_page_start_offset;

	/* Re-initialize to reload the progress */
	err = dfu_target_stream_done(false);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	err = DFU_TARGET_STREAM_INIT(TEST_ID_2, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	/* Check that last erased page offset was set correctly when loading */
	ctx = dfu_target_stream_get_stream();
	zassert_not_null(ctx, "Expected non-null ctx.");
	zassert_equal(erased_page_offset, ctx->last_erased_page_start_offset,
		      "Expected last erased page offset to be unchanged.");

	/* Next, check that writes that end up right after a page boundary
	 * result in a correct start offset for the last page erased.
	 */

	/* Write exactly one page of data */
	err = dfu_target_stream_write(write_buf, page_size);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	/* Store the last erased page start offset from before load */
	ctx = dfu_target_stream_get_stream();
	zassert_not_null(ctx, "Expected non-null ctx.");
	erased_page_offset = ctx->last_erased_page_start_offset;

	/* Verify that at least one page was erased. */
	zassert_true(erased_page_offset >= 0, "Expected pages to be erased.");

	/* Re-initialize to reload the progress */
	err = dfu_target_stream_done(false);
	zassert_equal(err, 0, "Unexpected failure: %d", err);
	err = DFU_TARGET_STREAM_INIT(TEST_ID_2, fdev, sbuf, sizeof(sbuf),
				     FLASH_BASE, 0, NULL);
	zassert_equal(err, 0, "Unexpected failure: %d", err);

	/* Check that last erased page offset was set correctly when loading */
	ctx = dfu_target_stream_get_stream();
	zassert_not_null(ctx, "Expected non-null ctx.");
	zassert_equal(erased_page_offset, ctx->last_erased_page_start_offset,
		      "Expected last erased page offset to be unchanged.");
}

static size_t get_flash_page_size(const struct device *dev)
{
	struct flash_driver_api *api = (struct flash_driver_api *) dev->api;
	const struct flash_pages_layout *layout;
	size_t layout_size;

	api->page_layout(dev, &layout, &layout_size);
	return layout->pages_size;
}

static void check_flash_base_at_page_start(const struct device *dev)
{
	uint32_t err;
	struct flash_pages_info page;

	err = flash_get_page_info_by_offs(dev, FLASH_BASE, &page);
	__ASSERT(err == 0, "Unexpected failure: %d", err);
	__ASSERT(page.start_offset == FLASH_BASE,
		 "Expected FLASH_BASE to be at a page boundary.");
}

#else

ZTEST(dfu_target_stream_test, test_dfu_target_stream_save_progress)
{
	ztest_test_skip();
}

#endif

static void *setup(void)
{
	__ASSERT_NO_MSG(device_is_ready(fdev));

#ifdef CONFIG_DFU_TARGET_STREAM_SAVE_PROGRESS
	/* Check that the FLASH_BASE macro is actually at the start of a page,
	 * since this is important for the validity of the progress test.
	 */
	check_flash_base_at_page_start(fdev);

	page_size = get_flash_page_size(fdev);
	__ASSERT(page_size <= BUF_LEN,
		 "BUF_LEN must be at least one page long");
#endif

	return NULL;
}

ZTEST_SUITE(dfu_target_stream_test, NULL, setup, NULL, NULL, NULL);
