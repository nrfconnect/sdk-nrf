/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <ztest.h>
#include <bl_validation.h>
#include <fw_info.h>
#include <pm_config.h>
#include <sys/util.h>
#include <nrfx_nvmc.h>
#include <linker/linker-defs.h>


void test_key_looping(void)
{
	/* If this boots, the test passed. The public key is at the end of the
	 * list, so the bootloader looped through all to validate this app.
	 */
}

/* 1. Validate current app in place. Expect success.
 * 2. Validate current app without first 0x200 bytes. Expect failure.
 * 3. Validate current app copied to somewhere else. Expect success.
 * 4. Validate copy against wrong address. Expect failure.
 * 5. Validate mangled copy. Expect failure
 */
void test_validation(void)
{
	zassert_true(bl_validate_firmware(PM_ADDRESS, PM_ADDRESS),
		"Fail 1. Failed to validate current app.\r\n");
	zassert_false(bl_validate_firmware(PM_ADDRESS, PM_ADDRESS + 0x200),
		"Fail 2. Incorrectly validated app with wrong offset.\r\n");

	/* 0x1000 to account for validation info. */
	u32_t copy_len = (u32_t)_flash_used + 1000;
	u32_t new_addr = ROUND_UP(PM_ADDRESS + copy_len, 0x1000);

	for (u32_t erase_addr = new_addr; erase_addr < (new_addr + copy_len);
		erase_addr += 0x1000) {
		u32_t ret = nrfx_nvmc_page_erase(new_addr);

		zassert_equal(NRFX_SUCCESS, ret, "Erase failed.\r\n");
	}
	nrfx_nvmc_words_write(new_addr, (const u32_t *)PM_ADDRESS,
		(copy_len + 3) / 4);

	zassert_true(bl_validate_firmware(PM_ADDRESS, new_addr),
		"Fail 3. Failed to validate displaced app.\r\n");

	zassert_false(bl_validate_firmware(PM_ADDRESS + 0x300, new_addr),
		"Fail 4. Incorrectly validated copy against wrong addr.\r\n");

	u32_t mangle_addr = new_addr + 0x300;

	zassert_not_equal(0, *(u32_t *)mangle_addr,
		"Unable to mangle, word is 0. \r\n");

	nrfx_nvmc_word_write(mangle_addr, 0);

	zassert_equal(0, *(u32_t *)mangle_addr,
		"Unable to mangle, word was not written to 0. \r\n");

	zassert_false(bl_validate_firmware(PM_ADDRESS, new_addr),
		"Fail 5. Incorrectly validated mangled app.\r\n");
}

void test_main(void)
{
	ztest_test_suite(test_bl_validation,
			 ztest_unit_test(test_key_looping),
			 ztest_unit_test(test_validation)
	);
	ztest_run_test_suite(test_bl_validation);
}
