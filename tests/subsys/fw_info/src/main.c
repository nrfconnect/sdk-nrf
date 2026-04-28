/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

#include <fw_info.h>
#include <pm_config.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/devicetree.h>

#define TEST_PARTITION	s1_image
#define TEST_ADDRESS	PARTITION_ADDRESS(TEST_PARTITION)
#define TEST_SIZE	PARTITION_SIZE(TEST_PARTITION)
#define TEST_DEV	PARTITION_DEVICE(TEST_PARTITION)

/* Assumptions for test supported environment */
#define MAX_SUPPORTED_WBS 16
#define ERASE_PAGE_SIZE 0x1000

/* Address is used as offset with flash device writes and to access
 * structures via processor address space. Assumption was that they
 * are the same.
 */
BUILD_ASSERT(TEST_ADDRESS == PARTITION_OFFSET(TEST_PARTITION));

/* Assumption: The test is only run on the internal SoC NVM, so we can
 * figure out need to erase before write on SoC series only.
 */
#if defined(CONFIG_SOC_NRF54L15)
#define NO_ERASE_NEEDED
#elif defined(CONFIG_SOC_SERIES_NRF52) || defined(CONFIG_SOC_SERIES_NRF53) ||\
	defined(CONFIG_SOC_SERIES_NRF91)
/* Nothing */
#else
#error Test needs alignment to the platform
#endif

/* signature check of this will fail, causing .valid to be overwritten. */
const struct fw_info dummy_s1 = {
	.magic = {FIRMWARE_INFO_MAGIC},
	.address = TEST_ADDRESS,
	.size = TEST_SIZE,
	.version = 0xFF,
	.valid = CONFIG_FW_INFO_VALID_VAL,
};

static uint8_t fw_info_find_buf[0x1000 + sizeof(dummy_s1)];
static const struct device *flash_dev = TEST_DEV;


ZTEST(test_fw_info, test_fw_info_invalidated)
{
	const struct fw_info *target = (const struct fw_info *)(TEST_ADDRESS);
	uint8_t intermediate[(sizeof(dummy_s1) + MAX_SUPPORTED_WBS - 1) & ~(MAX_SUPPORTED_WBS - 1)];
	int ret;

	/* Write a dummy upgrade to S1 */
	if (!fw_info_check((uint32_t)target)) {
		uint32_t wbs = flash_get_write_block_size(flash_dev);

		TC_PRINT("#### Writing dummy fw_info of s1 ####\n");

#ifndef NO_ERASE_NEEDED
		ret = flash_erase(flash_dev, (off_t)target, ERASE_PAGE_SIZE);
		zassert_equal(0, ret, "flash_erase failed %d", ret);
#endif
		memset(intermediate, 0, sizeof(intermediate));
		memcpy(intermediate, &dummy_s1, sizeof(dummy_s1));
		ret = flash_write(flash_dev, (off_t)target, intermediate,
					(sizeof(dummy_s1) + wbs - 1) & ~(wbs - 1));
		zassert_equal(0, ret, "flash_write failed %d", ret);

		zassert_equal(CONFIG_FW_INFO_VALID_VAL, target->valid,
				"wrong valid value");

		TC_PRINT("#### REBOOTING ####\n\n");
		sys_reboot(0);
	}

	/* The s1 fw_info has been faked and boot should detect this, prior to
	 * booting the test, and invalidate the fw_info, which means t should
	 * be set to value different from CONFIG_FW_INFO_VALID_VAL.
	 */
	zassert_not_equal(CONFIG_FW_INFO_VALID_VAL, target->valid,
		"expected target to be invalidated 0x%x", target->valid);

	/* Reset flash so test will enter the fw_info setup path on next run */
#ifndef NO_ERASE_NEEDED
	memset(intermediate, 0, sizeof(intermediate));
	ret = flash_write(flash_dev, (off_t)target, intermediate, sizeof(intermediate));
	zassert_equal(0, ret, "flash_write_failed %d", ret);
#else
	ret = flash_erase(flash_dev, (off_t)target, ERASE_PAGE_SIZE);
	zassert_equal(0, ret, "flash_erase failed %d", ret);
#endif
}


ZTEST(test_fw_info, test_fw_info_find)
{
	for (uint32_t i = 0; i < FW_INFO_OFFSET_COUNT; i++) {
		const struct fw_info *fwinfo_res;

		memset(fw_info_find_buf, 0xFF, sizeof(fw_info_find_buf));
		memcpy(&fw_info_find_buf[fw_info_allowed_offsets[i]], &dummy_s1,
			sizeof(dummy_s1));
		fwinfo_res = fw_info_find((uint32_t)fw_info_find_buf);
		zassert_equal_ptr(&fw_info_find_buf[fw_info_allowed_offsets[i]],
			fwinfo_res,
			"fw_info_find returned 0x%x", (uint32_t)fwinfo_res);
	}
}

static void *test_fw_info_setup(void)
{
	uint32_t wbs = flash_get_write_block_size(flash_dev);

	/* Test may have not been tuned for a new platform with given write block size */
	zassert_true(wbs <= MAX_SUPPORTED_WBS, "Write block not yet supported within test");
#ifndef NO_ERASE_NEEDED
	/* Check if erase-block-size is supported */
	struct flash_pages_info page;

	zassert_ok(flash_get_page_info_by_offs(flash_dev, 0, &page));
	zassert_equal(page.size, ERASE_PAGE_SIZE);
#endif

	return NULL;
}

ZTEST_SUITE(test_fw_info, NULL, test_fw_info_setup, NULL, NULL, NULL);
