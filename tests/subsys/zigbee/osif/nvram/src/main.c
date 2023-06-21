/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <pm_config.h>
#include <zboss_api.h>
#include <zb_errors.h>
#include <zb_osif.h>

#define PAGE_SIZE 0x400         /* Size for testing purpose */

#define ZBOSS_NVRAM_PAGE_SIZE (PM_ZBOSS_NVRAM_SIZE / CONFIG_ZIGBEE_NVRAM_PAGE_COUNT)

#define PHYSICAL_PAGE_SIZE 0x1000 /* For nvram in nrf5 products */

BUILD_ASSERT((ZBOSS_NVRAM_PAGE_SIZE % PHYSICAL_PAGE_SIZE) == 0,
	     "The size must be a multiply of physical page size.");

static uint8_t zb_nvram_buf[PAGE_SIZE];

/* Stub for ZBOSS callout */
void zb_nvram_erase_finished_stub(zb_uint8_t page)
{
}

/* Stub for ZBOSS signal handler */
void zboss_signal_handler(zb_bufid_t bufid)
{
}

static void test_case_setup(void *f)
{
	ARG_UNUSED(f);

	/* Erase NVRAM to have repeatability of test runs. */
	for (int page = 0; page < CONFIG_ZIGBEE_NVRAM_PAGE_COUNT; page++) {
		int ret = zb_osif_nvram_erase_async(page);

		zassert_true(ret == RET_OK, "Erasing failed");
	}
}

ZTEST_SUITE(osif_test, NULL, NULL, test_case_setup, NULL, NULL);

ZTEST(osif_test, test_zb_nvram_memory_size)
{
	zassert_true(zb_get_nvram_page_length() == ZBOSS_NVRAM_PAGE_SIZE,
		     "Page size fail");

	zassert_true(zb_get_nvram_page_count() == CONFIG_ZIGBEE_NVRAM_PAGE_COUNT,
		     "Page count fail");
}

ZTEST(osif_test, test_zb_nvram_erase)
{
	for (int page = 0; page < CONFIG_ZIGBEE_NVRAM_PAGE_COUNT; page++) {
		int ret = zb_osif_nvram_erase_async(page);

		zassert_true(ret == RET_OK, "Erasing failed");
	}

	/* Validate if flash memory is cleared */
	for (uint8_t page = 0; page < CONFIG_ZIGBEE_NVRAM_PAGE_COUNT; page++) {

		/* Validate all physical page offsets */
		for (uint32_t offset = 0; offset < ZBOSS_NVRAM_PAGE_SIZE;
		     offset += PHYSICAL_PAGE_SIZE) {

			zb_osif_nvram_read(page, offset, zb_nvram_buf,
					   PAGE_SIZE);
			for (int i = 0; i < PAGE_SIZE; i++) {
				zassert_true(zb_nvram_buf[i] == 0xFF,
					     "Erasing failed");
			}
		}
	}
}

ZTEST(osif_test, test_zb_nvram_write)
{
	const uint8_t MEM_PATTERN = 0xAA;

	memset(zb_nvram_buf, MEM_PATTERN, sizeof(zb_nvram_buf));

	for (uint8_t page = 0; page < CONFIG_ZIGBEE_NVRAM_PAGE_COUNT; page++) {

		/* Write to all physical page offsets */
		for (uint32_t offset = 0; offset < ZBOSS_NVRAM_PAGE_SIZE;
		     offset += PHYSICAL_PAGE_SIZE) {

			int ret = zb_osif_nvram_write(page, offset,
						      zb_nvram_buf,
						      PAGE_SIZE);

			zassert_true(ret == RET_OK, "writing failed");

			/* Validate write operation */
			zb_osif_nvram_read(page,
					   offset,
					   zb_nvram_buf,
					   PAGE_SIZE);
			for (int i = 0; i < PAGE_SIZE; i++) {
				zassert_true(zb_nvram_buf[i] == MEM_PATTERN,
					     "writing failed");
			}
		}
	}
}
