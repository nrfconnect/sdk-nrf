/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <ztest.h>
#include <nrfx.h>
#include <storage/flash_map.h>
#include <drivers/flash.h>
#include <nrfx_nvmc.h>
#include <devicetree.h>

#define EXPECTED_SIZE	256
#define FLASH_DEVICE DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL

static const struct device *flash_dev;
static struct flash_pages_info page_info;
static uint8_t __aligned(4) expected[EXPECTED_SIZE];

static void test_setup(void)
{
	int rc;

	flash_dev = device_get_binding(FLASH_DEVICE);
	const struct flash_parameters *fp =
			flash_get_parameters(flash_dev);

	/* For tests purposes use page */
	flash_get_page_info_by_offs(flash_dev, 0x3e000,
				    &page_info);

	/* Check if test region is not empty */
	uint8_t buf[EXPECTED_SIZE];

	rc = flash_read(flash_dev, page_info.start_offset,
			buf, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot read flash");

	/* Fill test buffer with random data */
	for (int i = 0; i < EXPECTED_SIZE; i++) {
		expected[i] = i;
	}

	/* Check if flash is cleared */
	bool is_buf_clear = true;

	for (off_t i = 0; i < EXPECTED_SIZE; i++) {
		if (buf[i] != fp->erase_value) {
			is_buf_clear = false;
			break;
		}
	}

	if (!is_buf_clear) {
		/* erase page */
		rc = flash_erase(flash_dev, page_info.start_offset,
				 page_info.size);
		zassert_equal(rc, 0, "Flash memory not properly erased");
	}
}

static void test_erase(void)
{
	int rc;
	uint8_t buf[EXPECTED_SIZE];
	const struct flash_parameters *fp =
			flash_get_parameters(flash_dev);

	rc = flash_write_protection_set(flash_dev, false);
	zassert_equal(rc, 0, NULL);

	/* Erase first, in order to not depend on earlier tests */
	rc = flash_erase(flash_dev, page_info.start_offset,
				page_info.size);
	zassert_equal(rc, 0, "Cannot erase flash");

	rc = flash_write(flash_dev,
			page_info.start_offset,
			expected, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot write to flash");

	rc = flash_erase(flash_dev, page_info.start_offset,
				page_info.size);
	zassert_equal(rc, 0, "Cannot erase flash");

	rc = flash_erase(flash_dev, page_info.start_offset,
				2 * page_info.size);
	zassert_equal(rc, 0, "Cannot erase flash");

	rc = flash_read(flash_dev, page_info.start_offset,
			buf, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot read flash");

	for (off_t i = 0; i < EXPECTED_SIZE; i++) {
		zassert_equal(buf[i], fp->erase_value,
				"Flash memory not properly erased");
	}
}

static void test_access(void)
{
	int rc;

	rc = flash_write_protection_set(flash_dev, true);
	zassert_equal(rc, 0, NULL);
}

static void test_write_unaligned(void)
{
	int rc;
	uint8_t data[4] = {0};

	rc = flash_write_protection_set(flash_dev, false);
	zassert_equal(rc, 0, NULL);

	rc = flash_write(flash_dev, page_info.start_offset + 1,
					data, 4);
	/* sdc flash driver supports unaligned writes */
	#if defined(CONFIG_SOC_FLASH_NRF_LL_SOFTDEVICE)
	zassert_equal(rc, 0, "Unexpected error code (%d)", rc);
	#else
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);
	#endif

	rc = flash_write(flash_dev, page_info.start_offset + 2,
					data, 4);
	#if defined(CONFIG_SOC_FLASH_NRF_LL_SOFTDEVICE)
	zassert_equal(rc, 0, "Unexpected error code (%d)", rc);
	#else
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);
	#endif

	rc = flash_write(flash_dev, page_info.start_offset + 3,
					data, 4);
	#if defined(CONFIG_SOC_FLASH_NRF_LL_SOFTDEVICE)
	zassert_equal(rc, 0, "Unexpected error code (%d)", rc);
	#else
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);
	#endif

	rc = flash_write(flash_dev, page_info.start_offset,
					data, 3);
	#if defined(CONFIG_SOC_FLASH_NRF_LL_SOFTDEVICE)
	zassert_equal(rc, 0, "Unexpected error code (%d)", rc);
	#else
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);
	#endif

	rc = flash_write(flash_dev, page_info.start_offset,
					data, 4);
	zassert_equal(rc, 0, "Unexpected error code (%d)", rc);
}

static void test_out_of_bounds(void)
{
	int rc;
	uint8_t data[8] = {0};
	size_t flash_size = 0;
	size_t page_size = 0;

	rc = flash_write_protection_set(flash_dev, false);
	flash_size = nrfx_nvmc_flash_size_get();
	page_size = nrfx_nvmc_flash_page_size_get();

	rc = flash_write(flash_dev, -1,
					data, 4);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_write(flash_dev, flash_size - 4,
					data, 4);
	zassert_equal(rc, 0, NULL);

	rc = flash_write(flash_dev, flash_size - 2 * page_size,
					data, 2 * page_size);
	zassert_equal(rc, 0, NULL);

	rc = flash_write(flash_dev, flash_size - 4,
					data, 8);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_write(flash_dev, flash_size,
					data, 4);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_write(flash_dev, page_info.start_offset,
					data, flash_size);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_write(flash_dev, page_info.start_offset,
					data, page_size);
	zassert_equal(rc, 0, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, flash_size, 4);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, flash_size - 4, 4);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, page_info.start_offset,
					2 * page_size);
	zassert_equal(rc, 0, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, flash_size - page_size,
					2 * page_size);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_erase(flash_dev, page_info.start_offset,
					3);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_read(flash_dev, -1,
					data, 4);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_read(flash_dev, flash_size,
					data, 4);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_read(flash_dev, flash_size - 4,
					data, 8);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);

	rc = flash_read(flash_dev, page_info.start_offset,
					data, flash_size);
	zassert_equal(rc, -EINVAL, "Unexpected error code (%d)", rc);
}

void test_main(void)
{
	ztest_test_suite(test_flash_driver,
			 ztest_unit_test(test_setup),
			 ztest_unit_test(test_erase),
			 ztest_unit_test(test_access),
			 ztest_unit_test(test_write_unaligned),
			 ztest_unit_test(test_out_of_bounds)
	);
	ztest_run_test_suite(test_flash_driver);
}
