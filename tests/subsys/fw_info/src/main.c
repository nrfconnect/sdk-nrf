/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <ztest.h>

#include "fw_info.h"
#include "pm_config.h"
#include "flash.h"
#include "power/reboot.h"

#define S1_ADDRESS 0x20000
#define S1_SIZE 0x8000


/* signature check of this will fail, causing .valid to be overwritten. */
const struct fw_info dummy_s1 = {
	.magic = {FIRMWARE_INFO_MAGIC},
	.address = S1_ADDRESS,
	.size = S1_SIZE,
	.version = 0xFF,
	.valid = CONFIG_FW_INFO_VALID_VAL,
};

static u8_t fw_info_find_buf[0x1000 + sizeof(dummy_s1)];


void test_fw_info_invalidate(void)
{
	const struct fw_info *target = (const struct fw_info *)(S1_ADDRESS);
	const u32_t zero = 0;
	int ret;

	struct device *flash_dev = device_get_binding(PM_S0_DEV_NAME);
	(void) flash_write_protection_set(flash_dev, false);

	/* Write a dummy upgrade to S1 */
	if (!fw_info_check(S1_ADDRESS)) {
		ret = flash_erase(flash_dev, S1_ADDRESS, 0x1000);
		zassert_equal(0, ret, "flash_erase failed %d", ret);
		ret = flash_write(flash_dev, S1_ADDRESS, &dummy_s1,
					sizeof(dummy_s1));
		zassert_equal(0, ret, "flash_write failed %d", ret);
		zassert_equal(CONFIG_FW_INFO_VALID_VAL, target->valid,
				"wrong valid value");
		sys_reboot(0);
	}

	/* Dummy upgrade should have been invalidated during reboot. */
	zassert_equal(S1_ADDRESS, (u32_t)target,
		"wrong info address 0x%x", (u32_t)target);
	zassert_equal(CONFIG_FW_INFO_VALID_VAL & 0xFFFF0000, target->valid,
		"wrong valid value 0x%x", target->valid);

	/* Reset flash so test can be run again. */
	ret = flash_write(flash_dev, S1_ADDRESS, &zero,	sizeof(zero));
	zassert_equal(0, ret, "flash_write_failed %d", ret);
}


void test_fw_info_find(void)
{
	for (u32_t i = 0; i < FW_INFO_OFFSET_COUNT; i++) {
		const struct fw_info *fwinfo_res;

		memset(fw_info_find_buf, 0xFF, sizeof(fw_info_find_buf));
		memcpy(&fw_info_find_buf[fw_info_allowed_offsets[i]], &dummy_s1,
			sizeof(dummy_s1));
		fwinfo_res = fw_info_find((u32_t)fw_info_find_buf);
		zassert_equal_ptr(&fw_info_find_buf[fw_info_allowed_offsets[i]],
			fwinfo_res,
			"fw_info_find returned 0x%x", (u32_t)fwinfo_res);
	}
}


void test_main(void)
{
	ztest_test_suite(test_fw_info,
			 ztest_unit_test(test_fw_info_invalidate),
			 ztest_unit_test(test_fw_info_find)
	);
	ztest_run_test_suite(test_fw_info);
}
