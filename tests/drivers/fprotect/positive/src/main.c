/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* This test checks if fprotect have been set up correctly and is able to
 * protect the flash from being modified. We should be able to read the flash
 * but not modify it. This test should fail with an BUS fault when trying to
 * write to an address below the location of b0(Secure bootloader) as it should
 * have been set up to be protected by SPU/ACL etc. depending on the chip used
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>

static const struct device *flash_dev = DEVICE_DT_GET(DT_NODELABEL(flash_controller));

static uint8_t write_data[] = "Hello World";
static uint32_t valid_write_addr = 0x1C000;
static uint32_t control_read_addr = 0x6032;
static uint8_t read_data_before[sizeof(write_data)];
static uint8_t read_data_after[sizeof(write_data)];

static void test_flash_write(void)
{
	int retval = flash_read(flash_dev, control_read_addr, read_data_before,
				ARRAY_SIZE(read_data_before));

	zassert_true(retval == 0, "flash_read failed");
	retval = flash_write(flash_dev, valid_write_addr, write_data,
			     ARRAY_SIZE(write_data));
	zassert_true(retval == 0, "flash_write failed");
}

static void test_flash_read(void)
{
	int retval = 0;

	retval = flash_read(flash_dev, valid_write_addr, read_data_after,
			    ARRAY_SIZE(read_data_after));
	zassert_true(retval == 0, "flash_read failed");
	for (size_t i = 0; i < ARRAY_SIZE(read_data_after); i++) {
		zassert_equal(read_data_after[i], write_data[i],
			      "Expected:'%c' got '%c' at positition %ld",
			      write_data[i], read_data_after[i], i);
	}
	retval = flash_read(flash_dev, control_read_addr, read_data_after,
			    ARRAY_SIZE(read_data_after));
	zassert_true(retval == 0, "flash_read failed");
	for (size_t i = 0; i < ARRAY_SIZE(read_data_after); i++) {
		zassert_equal(read_data_after[i], read_data_before[i],
			      "Expected:'%c' got '%c' at positition %ld",
			      read_data_before[i], read_data_after[i], i);
	}
}

static void test_flash_read_protected(void)
{
	int retval;
	uint8_t rd[256];

	retval = flash_read(flash_dev, 0, rd, sizeof(rd));
	zassert_true(retval == 0, "flash read to protected area failed");
}

void test_main(void)
{
	__ASSERT_NO_MSG(device_is_ready(flash_dev));

	ztest_test_suite(test_fprotect_positive,
			ztest_unit_test(test_flash_write),
			ztest_unit_test(test_flash_read),
			ztest_unit_test(test_flash_read_protected)
			);
	ztest_run_test_suite(test_fprotect_positive);
}
