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

#include <ztest.h>
#include <device.h>
#include <drivers/flash.h>
#include <pm_config.h>

#define SOC_NV_FLASH_CONTROLLER_NODE DT_NODELABEL(flash_controller)
#define FLASH_DEV_NAME DT_LABEL(SOC_NV_FLASH_CONTROLLER_NODE)

#define BUF_SIZE 256
#define INVALID_WRITE_ADDR (PM_B0_END_ADDRESS - BUF_SIZE)

#ifdef PM_HW_UNIQUE_KEY_PARTITION_ADDRESS
#define INVALID_READ_ADDR PM_HW_UNIQUE_KEY_PARTITION_ADDRESS
#endif

static uint32_t expected_fatal;
static uint32_t actual_fatal;
static uint8_t read_buf[BUF_SIZE];

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *pEsf)
{
	printk("Caught system error -- reason %d\n", reason);
	actual_fatal++;
}

static void flash_write_protected_fails(uint32_t addr, bool backup)
{
	uint8_t buf[BUF_SIZE];
	int err;

	(void)memset(buf, 0xa5, sizeof(buf));
	const struct device *flash_dev = device_get_binding(FLASH_DEV_NAME);
	if (backup) {
		err = flash_read(flash_dev, addr, read_buf, sizeof(read_buf));
		zassert_equal(0, err, "flash_read() failed: %d.\n", err);
	}
	printk("NOTE: A BUS FAULT immediately after this message"
		" means the test passed!\n");
	zassert_equal(expected_fatal, actual_fatal, "An unexpected fatal error has occurred.\n");
	expected_fatal++;
	err = flash_write(flash_dev, addr, buf, sizeof(buf));
	zassert_unreachable("Should have BUS FAULTed before coming here.");
}


static void test_flash_write_protected_fails(void)
{
	flash_write_protected_fails(INVALID_WRITE_ADDR, true);
}

static void flash_write_protected_unmodified(uint32_t addr)
{
	uint8_t buf[256];
	int err;

	const struct device *flash_dev = device_get_binding(FLASH_DEV_NAME);

	err = flash_read(flash_dev, addr, buf, sizeof(buf));
	zassert_equal(0, err, "flash_read() failed: %d.\n", err);
	zassert_mem_equal(buf, read_buf, sizeof(buf), "write protected flash has been modified.\n");
}

static void test_flash_write_protected_unmodified(void)
{
	flash_write_protected_unmodified(INVALID_WRITE_ADDR);
}


static void test_flash_read_protected_fails_r(void)
{
#ifdef PM_HW_UNIQUE_KEY_PARTITION_ADDRESS
	uint8_t buf[256];
	int err;

	const struct device *flash_dev = device_get_binding(FLASH_DEV_NAME);

	printk("NOTE: A BUS FAULT immediately after this message"
		" means the test passed!\n");
	zassert_equal(expected_fatal, actual_fatal, "An unexpected fatal error has occurred.\n");
	expected_fatal++;
	err = flash_read(flash_dev, INVALID_READ_ADDR, buf, sizeof(buf));
	zassert_unreachable("Should have BUS FAULTed before coming here.");
#endif
}

static void test_fatal(void)
{
	zassert_equal(expected_fatal, actual_fatal,
			"An unexpected fatal error has occurred (%d != %d).\n",
			expected_fatal, actual_fatal);
}

void test_main(void)
{
	ztest_test_suite(test_fprotect_negative,
			ztest_unit_test(test_flash_write_protected_fails),
			ztest_unit_test(test_flash_write_protected_unmodified),
			ztest_unit_test(test_flash_read_protected_fails_r),
			ztest_unit_test(test_fatal)
			);
	ztest_run_test_suite(test_fprotect_negative);
}
