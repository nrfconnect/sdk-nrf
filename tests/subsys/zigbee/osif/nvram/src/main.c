/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <pm_config.h>
#include <zboss_api.h>
#include <zb_errors.h>
#include <zb_osif.h>

#define PAGE_SIZE 0x400         /* Size for testing purpose */
#define VIRTUAL_PAGE_COUNT 2    /* ZBOSS uses two virtual pages */

#define ZBOSS_NVRAM_PAGE_SIZE (PM_ZBOSS_NVRAM_SIZE / VIRTUAL_PAGE_COUNT)

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

static void test_zb_nvram_memory_size(void)
{
	zassert_true(zb_get_nvram_page_length() == ZBOSS_NVRAM_PAGE_SIZE,
		     "Page size fail");

	zassert_true(zb_get_nvram_page_count() == VIRTUAL_PAGE_COUNT,
		     "Page count fail");
}

static void test_zb_nvram_erase(void)
{
	for (int page = 0; page < VIRTUAL_PAGE_COUNT; page++) {
		int ret = zb_osif_nvram_erase_async(page);

		zassert_true(ret == RET_OK, "Erasing failed");
	}

	/* Validate if flash memory is cleared */
	for (uint8_t page = 0; page < VIRTUAL_PAGE_COUNT; page++) {

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

static void test_zb_nvram_write(void)
{
	const uint8_t MEM_PATTERN = 0xAA;

	memset(zb_nvram_buf, MEM_PATTERN, sizeof(zb_nvram_buf));

	for (uint8_t page = 0; page < VIRTUAL_PAGE_COUNT; page++) {

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


void test_main(void)
{
	ztest_test_suite(osif_test,
			 ztest_unit_test(test_zb_nvram_memory_size),
			 ztest_unit_test(test_zb_nvram_erase),
			 ztest_unit_test(test_zb_nvram_write)
			 );

	ztest_run_test_suite(osif_test);
}
