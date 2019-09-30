/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/* This test checks if fprotect have been set up correctly and is able to
 * protect the flash from being modified. We should be able to read the flash
 * but not modify it. This test should fail with an BUS fault when trying to
 * write to an address below the location of b0(Secure bootloader) as it should
 * have been set up to be protected by SPU/ACL etc. depending on the chip used
 */

#include <ztest.h>
#include <device.h>
#include <flash.h>

u8_t write_data[] = "Hello world";
u32_t valid_write_addr = 0x1C000;

void test_flash_write(void)
{
	int retval = 0;
	struct device *flash_dev = device_get_binding(DT_FLASH_DEV_NAME);
	(void) flash_write_protection_set(flash_dev, false);
	retval = flash_write(flash_dev, valid_write_addr, write_data, ARRAY_SIZE(write_data));
	zassert_true(retval == 0, "flash_write failed");
}

void test_flash_read(void)
{
	int retval = 0;
	u8_t read_data[strlen(write_data)+1];
	struct device *flash_dev = device_get_binding(DT_FLASH_DEV_NAME);
	retval = flash_read(flash_dev, valid_write_addr, read_data, ARRAY_SIZE(read_data));
	zassert_true(retval == 0, "flash_read failed");
	for (size_t i = 0; i < ARRAY_SIZE(read_data); i++) {
		zassert_equal(read_data[i], write_data[i],
					  "Expected:%c got %c at positition %ld",
					  write_data[i],
					  read_data[i],
					  i);
	}
	retval = flash_read(flash_dev, 0x6032, read_data, ARRAY_SIZE(read_data));
	for (size_t i = 0; i < ARRAY_SIZE(read_data); i++) {
		zassert_not_equal(read_data[i], write_data[i],
					  "Expected:%c should not be equal to %c at positition %ld",
					  write_data[i],
					  read_data[i],
					  i);
	}
}

void test_flash_read_protected(void)
{
	int retval;
	u8_t rd[256];
	struct device *flash_dev = device_get_binding(DT_FLASH_DEV_NAME);
	retval = flash_read(flash_dev, 0, rd, sizeof(rd));
	zassert_true(retval == 0, "flash read to protected area failed");
}

void test_flash_write_protected(void)
{
	u8_t wd[256];
	u32_t invalid_write_addr = 0x7000;
	(void)memset(wd, 0xa5, sizeof(wd));
	struct device *flash_dev = device_get_binding(DT_FLASH_DEV_NAME);
	(void) flash_write_protection_set(flash_dev, false);
	printk("NOTE: A BUS FAULT (BFAR addr 0x%x) immediately after this message"
		" means the test passed!\n", invalid_write_addr);
	int err = flash_write(flash_dev, invalid_write_addr, wd, sizeof(wd));
	zassert_equal(0, err, "flash_write failed with err code %d\r\n", err);
	zassert_unreachable("Should have BUS FAULTed before coming here.");
}

void test_main(void)
{
	ztest_test_suite(test_ib_fprotect_readback,
			ztest_unit_test(test_flash_write),
			ztest_unit_test(test_flash_read)
			);
	ztest_test_suite(test_ib_fprotect,
			ztest_unit_test(test_flash_read_protected),
			ztest_unit_test(test_flash_write_protected)
			);
	ztest_run_test_suite(test_ib_fprotect_readback);
	ztest_run_test_suite(test_ib_fprotect);
}
