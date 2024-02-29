/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>

#define TEST_AREA_DEVICE	DEVICE_DT_GET(DT_NODELABEL(rpc_flash_controller))
#define TEST_AREA_PAGE_SIZE	DT_PROP(DT_NODELABEL(flash_rpc), erase_block_size)
#define TEST_AREA_START_ADDR	(DT_REG_ADDR(DT_NODELABEL(flash_rpc)) + \
				(DT_REG_SIZE(DT_NODELABEL(flash_rpc)) / \
				TEST_AREA_PAGE_SIZE / 2 * TEST_AREA_PAGE_SIZE))
#define TEST_AREA_ERASE_SIZE	(TEST_AREA_PAGE_SIZE * 3)
#define TEST_AREA_WRITE_SIZE	(TEST_AREA_PAGE_SIZE * 2)
#define TEST_AREA_BUFFER_SIZE	(TEST_AREA_WRITE_SIZE + 5)
#define CANARY 0x42

/*
 *      Test is performed in a way to ensure read/write
 *      occurs in a boundary between pages.
 *      Three pages are erased each time.
 *      Write address is shifted by one four times.
 *      Read operations are performed on whole written area
 *      and loaded into buffer with progressively increased offset.
 *      Each time CANARIES are placed in the buffer to verify if
 *      leading and trailing portions of the buffer hasn't been corrupted.
 *
 *         _______________ _______________ _______________
 *        |    PAGE0      |    PAGE1      |    PAGE2      |
 *         --------------- --------------- ---------------
 *ERASE   ^______________________________________________^
 *WRITE   ^______________________________^
 *READS
 *ERASE   ^______________________________________________^
 *WRITE    ^______________________________^
 *READS
 *ERASE   ^______________________________________________^
 *WRITE     ^______________________________^
 *READS
 *ERASE   ^______________________________________________^
 *WRITE      ^______________________________^
 *READS
 *
 *       ______________________________________
 *      |C|              BUF              |CCCC|
 *       --------------------------------------
 *READ    ^______________________________^
 *       ______________________________________
 *      |CC|             BUF               |CCC|
 *       --------------------------------------
 *READ     ^______________________________^
 *       ______________________________________
 *      |CCC|            BUF                |CC|
 *       --------------------------------------
 *READ      ^______________________________^
 *       ______________________________________
 *      |CCCC|           BUF                 |C|
 *       --------------------------------------
 *READ       ^______________________________^
 *
 */

static const struct device *const flash_dev = TEST_AREA_DEVICE;
static uint8_t __aligned(4) expected[TEST_AREA_BUFFER_SIZE];
static uint8_t buf[TEST_AREA_BUFFER_SIZE];

ZTEST(flash_driver, test_read_unaligned_address)
{
	zassert_true(device_is_ready(flash_dev));

	for (int i = 0; i < TEST_AREA_BUFFER_SIZE; i++) {
		if (i == CANARY) {
			expected[i] = ~CANARY;
		} else {
			expected[i] = i;
		}
	}
	int rc = flash_erase(flash_dev, TEST_AREA_START_ADDR, TEST_AREA_ERASE_SIZE);

	zassert_equal(rc, 0, "Flash memory erase failed");

	for (off_t addr_o = 0; addr_o < 4; addr_o++) {
		rc = flash_write(flash_dev, TEST_AREA_START_ADDR + addr_o, expected,
				TEST_AREA_WRITE_SIZE);

		if (addr_o % 4) {
			zassert_not_equal(rc, 0, "Unexpected unaligned flash write success");

		} else {
			zassert_equal(rc, 0, "Cannot write to flash");

			for (off_t buf_o = 1; buf_o < 5; buf_o++) {
				memset(buf, CANARY, TEST_AREA_BUFFER_SIZE);
				rc = flash_read(flash_dev,
						TEST_AREA_START_ADDR + addr_o, buf + buf_o,
						TEST_AREA_WRITE_SIZE);
				zassert_equal(rc, 0, "Cannot read flash");
				zassert_equal(memcmp(buf + buf_o, expected + addr_o,
					      TEST_AREA_WRITE_SIZE), 0,
					      "Flash read failed addr_o=%d, buf_o=%d", addr_o,
					      buf_o);
				zassert_equal(buf[buf_o - 1], CANARY,
					"Buffer underflow addr_o=%d, buf_o=%d", addr_o, buf_o);
				zassert_equal(buf[buf_o + TEST_AREA_WRITE_SIZE], CANARY,
					"Buffer overflow addr_o=%d, buf_o=%d", addr_o, buf_o);
			}

			rc = flash_erase(flash_dev, TEST_AREA_START_ADDR, TEST_AREA_ERASE_SIZE);
			zassert_equal(rc, 0, "Flash memory erase failed");

		}
	}
}

ZTEST_SUITE(flash_driver, NULL, NULL, NULL, NULL, NULL);
