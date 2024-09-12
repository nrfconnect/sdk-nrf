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
#include <nrfx_nvmc.h>
#include <zephyr/sys/util.h>
#include <fprotect.h>
#include <zephyr/linker/linker-defs.h>

#define BUF_SIZE 256

#ifdef CONFIG_SECURE_BOOT
#include <pm_config.h>
#define TEST_FPROTECT_BOOTLOADER_PROTECTED (PM_B0_ADDRESS + 0x800)
#endif

#define TEST_FPROTECT_WRITE_ADDR ROUND_UP( \
		(uint32_t)__rom_region_start + (uint32_t)_flash_used, \
		CONFIG_FPROTECT_BLOCK_SIZE)

#define TEST_FPROTECT_READ_ADDR \
		(ROUND_UP((uint32_t)__rom_region_start + (uint32_t)_flash_used, \
		CONFIG_FPROTECT_BLOCK_SIZE) \
	+ CONFIG_FPROTECT_BLOCK_SIZE)

static volatile uint32_t expected_fatal;
static uint32_t actual_fatal;
static uint8_t read_buf[BUF_SIZE];

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	printk("Caught system error -- reason %d\n", reason);
	actual_fatal++;
}

static void flash_write_protected_fails(uint32_t addr, bool backup)
{
	uint8_t buf[BUF_SIZE];

#ifdef CONFIG_HAS_HW_NRF_ACL
	zassert_equal(1, fprotect_is_protected(ROUND_DOWN(addr, CONFIG_FPROTECT_BLOCK_SIZE)), NULL);
#endif

	(void)memset(buf, 0xa5, sizeof(buf));
	if (backup) {
		memcpy(read_buf, (void *)addr, sizeof(read_buf));
	}

	printk("NOTE: A BUS FAULT immediately after this message"
		" means the test passed!\n");
	zassert_equal(expected_fatal, actual_fatal, "An unexpected fatal error has occurred.\n");
	expected_fatal++;
	nrfx_nvmc_bytes_write(addr, buf, sizeof(buf));
	zassert_unreachable("Should have BUS FAULTed before coming here.");
}

static void flash_write_protected_unmodified(uint32_t addr)
{
	uint8_t buf[BUF_SIZE];

	memcpy(buf, (void *)addr, sizeof(buf));
	zassert_mem_equal(buf, read_buf, sizeof(buf), "write protected flash has been modified.\n");
}

ZTEST(test_fprotect_negative, test_flash_write_protected_fails)
{
	uint8_t buf[BUF_SIZE];

	(void)memset(buf, 0x5a, sizeof(buf));
	nrfx_nvmc_bytes_write(TEST_FPROTECT_WRITE_ADDR, buf, sizeof(buf));

#ifdef CONFIG_HAS_HW_NRF_ACL
	zassert_equal(0, fprotect_is_protected(TEST_FPROTECT_WRITE_ADDR), NULL);
#endif
	fprotect_area(TEST_FPROTECT_WRITE_ADDR, CONFIG_FPROTECT_BLOCK_SIZE);

	flash_write_protected_fails(TEST_FPROTECT_WRITE_ADDR, true);
}

ZTEST(test_fprotect_negative, test_flash_write_protected_unmodified)
{
	flash_write_protected_unmodified(TEST_FPROTECT_WRITE_ADDR);
}

ZTEST(test_fprotect_negative, test_bootloader_protection)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_SECURE_BOOT);
#ifdef CONFIG_SECURE_BOOT
	flash_write_protected_fails(TEST_FPROTECT_BOOTLOADER_PROTECTED, true);
#endif
}

ZTEST(test_fprotect_negative, test_flash_read_protected_fails_r)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_HAS_HW_NRF_ACL);
#ifdef CONFIG_HAS_HW_NRF_ACL
	const volatile uint32_t *test_read = (void *)TEST_FPROTECT_READ_ADDR;
	uint32_t expected_content = *test_read;

	zassert_equal(0, fprotect_is_protected(TEST_FPROTECT_READ_ADDR), NULL);

	fprotect_area_no_access(TEST_FPROTECT_READ_ADDR, CONFIG_FPROTECT_BLOCK_SIZE);

	zassert_equal(3, fprotect_is_protected(TEST_FPROTECT_READ_ADDR), NULL);

	printk("NOTE: A BUS FAULT immediately after this message"
		" means the test passed!\n");
	zassert_equal(expected_fatal, actual_fatal, "An unexpected fatal error has occurred.\n");
	expected_fatal++;

	/* The following line should busfault. */
	zassert_equal(expected_content, *test_read, "Unexpected flash contents\n");

	zassert_unreachable("Should have BUS FAULTed before coming here.");
#endif
}

void check_fatal(void *f)
{
	zassert_equal(expected_fatal, actual_fatal,
			"The wrong number of fatal error has occurred (e:%d != a:%d).\n",
			expected_fatal, actual_fatal);
}

ZTEST_SUITE(test_fprotect_negative, NULL, NULL, NULL, check_fatal, NULL);
