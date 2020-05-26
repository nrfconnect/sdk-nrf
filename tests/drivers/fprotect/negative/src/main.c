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
#include <drivers/flash.h>

#define SOC_NV_FLASH_CONTROLLER_NODE DT_NODELABEL(flash_controller)
#define FLASH_DEV_NAME DT_LABEL(SOC_NV_FLASH_CONTROLLER_NODE)

static void test_flash_write_protected(void)
{
	u8_t wd[256];
	u32_t invalid_write_addr = 0x7000;
	int err;

	(void)memset(wd, 0xa5, sizeof(wd));
	struct device *flash_dev = device_get_binding(FLASH_DEV_NAME);
	(void) flash_write_protection_set(flash_dev, false);
	printk("NOTE: A BUS FAULT (BFAR addr 0x%x) immediately after this message"
		" means the test passed!\n", invalid_write_addr);
	err = flash_write(flash_dev, invalid_write_addr, wd, sizeof(wd));
	zassert_equal(0, err, "flash_write failed with err code %d\r\n", err);
	zassert_unreachable("Should have BUS FAULTed before coming here.");
}

void test_main(void)
{
	ztest_test_suite(test_fprotect_negative,
			ztest_unit_test(test_flash_write_protected)
			);
	ztest_run_test_suite(test_fprotect_negative);
}
