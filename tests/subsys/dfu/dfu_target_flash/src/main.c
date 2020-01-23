/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <ztest.h>
#include <drivers/flash.h>

#include <dfu_target_flash.h>

#define BUF_LEN 0x2000 /* Must be equal or bigger to page size */
#define FLASH_SIZE DT_SOC_NV_FLASH_0_SIZE
#define FLASH_NAME DT_FLASH_DEV_NAME

/* so that we don't overwrite the application when running on hw */
#define FLASH_BASE (64*1024)

static struct device *flash_dev;
static const struct flash_driver_api *api;
static const struct flash_pages_layout *layout;
static size_t layout_size;
static int page_size;

static u8_t buf[BUF_LEN];

static u8_t write_buf[BUF_LEN * 4] = {[0 ... (BUF_LEN * 4) - 1] = 0xaa};

static u8_t read_buf[BUF_LEN * 4];

static u8_t written_pattern[BUF_LEN * 4] = {[0 ... (BUF_LEN * 4) - 1] = 0xaa};

static u8_t garbage_pattern[BUF_LEN] = {[0 ... BUF_LEN - 1] = 0x77};

#define VERIFY_BUF(start, size, buf) \
do { \
	rc = flash_read(flash_dev, FLASH_BASE + start, read_buf, size); \
	zassert_equal(rc, 0, "should succeed"); \
	zassert_mem_equal(read_buf, buf, size, "should equal %s", #buf);\
} while (0)

#define VERIFY_WRITTEN(start, size) VERIFY_BUF(start, size, written_pattern)
#define VERIFY_NOT_WRITTEN(start, size) VERIFY_BUF(start, size, garbage_pattern)

static void fill_flash_with_garbage(void)
{
	int rc;

	rc = flash_write_protection_set(flash_dev, false);
	zassert_equal(rc, 0, "should succeed");

	/* 4 pages is the max used in these tests */
	for (int i = 0; i < 4; i++) {
		rc = flash_erase(flash_dev,
				 FLASH_BASE + (i * layout->pages_size),
				 layout->pages_size);
		zassert_equal(rc, 0, "should succeed");

		rc = flash_write(flash_dev,
				 FLASH_BASE + (i * layout->pages_size),
				 garbage_pattern, layout->pages_size);
		zassert_equal(rc, 0, "should succeed");
	}

	rc = flash_write_protection_set(flash_dev, true);
	zassert_equal(rc, 0, "should succeed");
}


static void reset_target(void)
{
	/* Ensure that target is clean */
	(void) dfu_target_flash_done(true);

	fill_flash_with_garbage();
}

static void init_target(void)
{
	int rc;

	/* Set up flash device */
	rc = dfu_target_flash_cfg(FLASH_NAME, FLASH_BASE, 0, buf, page_size);
	zassert_equal(rc, 0, "expected success");

	reset_target();
}

static void test_dfu_target_flash__init(void)
{
	int rc;

	reset_target();

	/* Call 'init' before assigning flash_dev */
	rc = dfu_target_flash_init(0x1000);
	zassert_true(rc < 0, "expected init to fail as no flash_dev is set)");

	rc = dfu_target_flash_cfg(FLASH_NAME, FLASH_BASE + 0x1000,
				  FLASH_BASE + 0x2000, buf, BUF_LEN);
	zassert_equal(rc, 0, "expected success");

	/* Verify that the available space is correct by invoking _init()
	 * with different file sizes. There should be 0x1000 bytes available.
	 */
	rc = dfu_target_flash_init(0x1001);
	zassert_true(rc < 0, "init should fail as file is to big");

	rc = dfu_target_flash_init(0x1000);
	zassert_equal(rc, 0, "expected success from _init()");
}

static void test_dfu_target_flash__cfg(void)
{
	int rc;
	size_t expected_flash_size;

	reset_target();

	/* Non-existing flash-dev */
	rc = dfu_target_flash_cfg("lol", 0x1000, 0x2000, buf, BUF_LEN);
	zassert_true(rc < 0, "should fail for non-existing flash-dev");

	/* End address out of range */
	rc = dfu_target_flash_cfg(FLASH_NAME, 0x1000, FLASH_SIZE + 4, buf,
				  BUF_LEN);
	zassert_true(rc < 0, "should fail as end is after flash size");

	/* buffer is NULL */
	rc = dfu_target_flash_cfg(FLASH_NAME, 0x1000, FLASH_SIZE, NULL,
				  BUF_LEN);
	zassert_true(rc < 0, "should fail as buffer is NULL");

	/* Buffer size is smaller than page size */
	rc = dfu_target_flash_cfg(FLASH_NAME, 0x1000, FLASH_SIZE, NULL,
				  page_size-1);
	zassert_true(rc < 0, "should fail as buffer size is < page size");

	/* start > end and end is not 0*/
	rc = dfu_target_flash_cfg(FLASH_NAME, 0x1000, 0x200, NULL,
				  page_size-1);
	zassert_true(rc < 0, "should fail as start > end and end != 0");

	/* Entering '0' as flash size uses rest of flash.
	 * Verify this by calling _init()
	 */
	rc = dfu_target_flash_cfg(FLASH_NAME, 0x1000, 0, buf, BUF_LEN);
	zassert_equal(rc, 0, "should succeed");
	expected_flash_size = FLASH_SIZE - 0x1000;
	rc = dfu_target_flash_init(expected_flash_size);
	zassert_equal(rc, 0, "expected success from _init()");
	rc = dfu_target_flash_init(expected_flash_size + 1);
	zassert_true(rc < 0, "should fail as file size is too big");
}

static void test_dfu_target_flash__write(void)
{
	int rc;

	init_target();

	/* Don't fill up the buffer */
	rc = dfu_target_flash_write(write_buf, page_size - 1);
	zassert_equal(rc, 0, "expected success");

	/* Verify that no data has been written */
	VERIFY_NOT_WRITTEN(0, page_size);

	/* Now, write the missing byte, which should trigger a dump to flash */
	rc = dfu_target_flash_write(write_buf, 1);
	zassert_equal(rc, 0, "expected success");

	VERIFY_WRITTEN(0, page_size);
}

static void test_dfu_target_flash__write__cross_page_border(void)
{
	int rc;

	init_target();

	/* Test when write crosses border of page */
	rc = dfu_target_flash_write(write_buf, page_size + 128);
	zassert_equal(rc, 0, "expected success");

	/* First page should be written */
	VERIFY_WRITTEN(0, page_size);

	/* Fill rest of the page */
	rc = dfu_target_flash_write(write_buf, page_size - 128);
	zassert_equal(rc, 0, "expected success");

	/* First two pages should be written */
	VERIFY_WRITTEN(0, page_size * 2);
}

static void test_dfu_target_flash__write__multi_page(void)
{
	int rc;

	init_target();

	/* Test when write spans multiple pages crosses border of page */
	rc = dfu_target_flash_write(write_buf, (page_size * 3) + 128);
	zassert_equal(rc, 0, "expected success");

	/* First three pages should be written */
	VERIFY_WRITTEN(0, page_size * 3);

	/* Fill rest of the page */
	rc = dfu_target_flash_write(write_buf, page_size - 128);
	zassert_equal(rc, 0, "expected success");

	/* First four pages should be written */
	VERIFY_WRITTEN(0, page_size * 4);
}

static void test_dfu_target_flash__offset_retained(void)
{
	int rc;
	size_t offset;

	init_target();

	/* Verify that the offset is retained across failed downolads */
	rc = dfu_target_flash_write(write_buf, page_size + 128);
	zassert_equal(rc, 0, "expected success");

	/* First page should be written */
	VERIFY_WRITTEN(0, page_size);

	/* done with failure (success = false) */
	rc = dfu_target_flash_done(false);
	zassert_equal(rc, 0, "expected success");

	/* Fill rest of the page */
	(void)dfu_target_flash_offset_get(&offset);
	zassert_equal(offset, page_size + 128, "offset should be retained");

	/* Fill up a page MINUS 128 to verify that write_buf_pos is retained */
	rc = dfu_target_flash_write(write_buf, page_size - 128);
	zassert_equal(rc, 0, "expected success");

	/* Second page should be written */
	VERIFY_WRITTEN(page_size, page_size);
}

void test_main(void)
{
	flash_dev = device_get_binding(FLASH_NAME);
	api = flash_dev->driver_api;
	api->page_layout(flash_dev, &layout, &layout_size);

	page_size = layout->pages_size;
	__ASSERT_NO_MSG(page_size <= BUF_LEN);

	ztest_test_suite(lib_dfu_target_flash_test,
	     ztest_unit_test(test_dfu_target_flash__init), /* Must be first */
	     ztest_unit_test(test_dfu_target_flash__cfg),
	     ztest_unit_test(test_dfu_target_flash__write),
	     ztest_unit_test(test_dfu_target_flash__write__cross_page_border),
	     ztest_unit_test(test_dfu_target_flash__write__multi_page),
	     ztest_unit_test(test_dfu_target_flash__offset_retained)
	 );

	ztest_run_test_suite(lib_dfu_target_flash_test);
}
