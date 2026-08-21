/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <fprotect.h>
#include <string.h>

#define STORAGE_PARTITION storage_partition
#define STORAGE_DEV       PARTITION_DEVICE(STORAGE_PARTITION)
#define STORAGE_ADDR      PARTITION_OFFSET(STORAGE_PARTITION)

#define BUF_SIZE 256

BUILD_ASSERT(STORAGE_ADDR % CONFIG_FPROTECT_BLOCK_SIZE == 0,
	     "storage area must be FPROTECT-block aligned");

static volatile uint32_t expected_fatal;
static uint32_t actual_fatal;
static uint8_t read_buf[BUF_SIZE];

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	printk("Caught system error -- reason %d\n", reason);
	actual_fatal++;
}

static void protected_write_fails(uint32_t addr, bool backup)
{
	uint8_t buf[BUF_SIZE];
	const struct device *dev = STORAGE_DEV;

	zassert_true(device_is_ready(dev), "flash device not ready");
	memset(buf, 0xa5, sizeof(buf));

	if (backup) {
		zassert_equal(0, flash_read(dev, addr, read_buf, sizeof(read_buf)),
			      "flash_read failed");
	}

	printk("NOTE: A BUS FAULT immediately after this message"
	       " means the test passed!\n");
	zassert_equal(expected_fatal, actual_fatal,
		      "An unexpected fatal error has occurred.\n");
	expected_fatal++;

	zassert_equal(0, flash_write(dev, addr, buf, sizeof(buf)),
		      "flash_write should not return");
	zassert_unreachable("Should have BUS FAULTed before coming here.");
}

static void protected_area_unmodified(uint32_t addr)
{
	uint8_t buf[BUF_SIZE];

	zassert_equal(0, flash_read(STORAGE_DEV, addr, buf, sizeof(buf)),
		      "flash_read failed");
	zassert_mem_equal(buf, read_buf, sizeof(buf),
			  "write protected flash has been modified.\n");
}

ZTEST(fprotect_negative_rram, test_misaligned_address_rejected)
{
	zassert_equal(-EINVAL,
		      fprotect_area(STORAGE_ADDR + 1, CONFIG_FPROTECT_BLOCK_SIZE));
}

ZTEST(fprotect_negative_rram, test_flash_write_protected_fails)
{
	uint8_t buf[BUF_SIZE] = {0x5a};

	zassert_true(device_is_ready(STORAGE_DEV), "flash device not ready");
	zassert_equal(0, flash_write(STORAGE_DEV, STORAGE_ADDR, buf, sizeof(buf)),
		      "legal flash_write failed");
	zassert_equal(0,
		      fprotect_area(STORAGE_ADDR, CONFIG_FPROTECT_BLOCK_SIZE),
		      "fprotect_area failed");

	protected_write_fails(STORAGE_ADDR, true);
}

ZTEST(fprotect_negative_rram, test_flash_write_protected_unmodified)
{
	protected_area_unmodified(STORAGE_ADDR);
}

static void check_fatal(void *unused)
{
	ARG_UNUSED(unused);

	zassert_equal(expected_fatal, actual_fatal,
		      "The wrong number of fatal error has occurred (e:%d != a:%d).\n",
		      expected_fatal, actual_fatal);
}

ZTEST_SUITE(fprotect_negative_rram, NULL, NULL, NULL, check_fatal, NULL);
