/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <fw_info.h>
#include <pm_config.h>
#include <nrfx_nvmc.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>
#include <bl_storage.h>
#include <fw_info.h>



/* These symbols are defined in linker scripts. */
extern const uint32_t _ext_apis_size[];
extern const uint32_t _ext_apis_req_size[];

extern const struct fw_info m_firmware_info;

#define VAL_INFO_MAX_SIZE 0x1000
uint32_t val_info_buf[VAL_INFO_MAX_SIZE];

ZTEST(test_bl_validation_neg, test_validation_neg1)
{
	uint32_t copy_len = ROUND_UP((uint32_t)_flash_used, 4);

	/* Round up to at least the next SPU region. */
	uint32_t new_addr = ROUND_UP(PM_ADDRESS + (PM_SIZE / 2), 0x8000);

	const struct fw_info s1_info = {
		.magic = {FIRMWARE_INFO_MAGIC},
		.total_size = PM_S0_ADDRESS - PM_S1_ADDRESS,
		.size = ((uint32_t)_flash_used),
		.version = CONFIG_FW_INFO_FIRMWARE_VERSION + 1,
		.address = new_addr,
		.boot_address = new_addr,
		.valid = CONFIG_FW_INFO_VALID_VAL,
		.reserved = {0, 0, 0, 0},
		.ext_api_num = 0,
		.ext_api_request_num = 0,
	};

	const struct fw_info *s1_info_copied = fw_info_find(PM_S1_ADDRESS);

	if (s1_info_copied) {
		/* Second boot */
		zassert_not_equal(CONFIG_FW_INFO_VALID_VAL,
			s1_info_copied->valid, "Failed to invalidate S1.\r\n");
		zassert_equal((uint32_t)s1_info_copied, PM_S1_ADDRESS,
			"S1 info found at wrong address.\r\n");
		uint32_t ret = nrfx_nvmc_page_erase(PM_S1_ADDRESS);

		zassert_equal(NRFX_SUCCESS, ret, "Erase failed.\r\n");
	} else {
		/* First boot */

		/* Copy app */
		for (uint32_t erase_addr = new_addr;
			erase_addr < (new_addr + copy_len);
			erase_addr += DT_PROP(DT_CHOSEN(zephyr_flash),
							erase_block_size)) {
			uint32_t ret = nrfx_nvmc_page_erase(new_addr);

			zassert_equal(NRFX_SUCCESS, ret, "Erase failed.\r\n");
		}
		nrfx_nvmc_words_write(new_addr, (const uint32_t *)PM_ADDRESS,
			copy_len / 4);

		/* Write to S1 */
		nrfx_nvmc_words_write(PM_S1_ADDRESS, &s1_info,
			ROUND_UP(sizeof(s1_info), 4) / 4);

		zassert_mem_equal(&s1_info, (void *)PM_S1_ADDRESS,
			sizeof(s1_info), "Failed to copy S1 info.\r\n");

		s1_info_copied = fw_info_find(PM_S1_ADDRESS);
		zassert_equal((uint32_t)s1_info_copied, PM_S1_ADDRESS,
			"S1 info wrongly copied.\r\n");

		/* Modify copied app's validation info */
		memcpy(val_info_buf, (const uint32_t *)(PM_ADDRESS + copy_len),
			VAL_INFO_MAX_SIZE);

		struct __packed {
			uint32_t magic[MAGIC_LEN_WORDS];
			uint32_t address;
		} *val_info = (void *)(val_info_buf);

		const uint32_t validation_info_magic[] = {VALIDATION_INFO_MAGIC};

		zassert_mem_equal(validation_info_magic, val_info->magic,
			MAGIC_LEN_WORDS*4,
			"Could not find validation info.\r\n");

		val_info->address = s1_info.address;
		nrfx_nvmc_words_write(s1_info.address + ROUND_UP(s1_info.size, 4), val_info,
			VAL_INFO_MAX_SIZE);

		/* Reboot */
		printk("Rebooting. Should fail to validate slot 1.");
		sys_reboot(0);
		zassert_true(false, "should not come here.");
	}
}


ZTEST(test_bl_validation_neg, test_validation_neg2)
{
#if PM_PROVISION_ADDRESS > 0xF00000
	uint32_t num_public_keys = num_public_keys_read();
	bool any_valid = false;

	__aligned(4) uint8_t key_data[SB_PUBLIC_KEY_HASH_LEN];

	for (uint32_t key_data_idx = 0; key_data_idx < num_public_keys;
			key_data_idx++) {
		int retval = public_key_data_read(key_data_idx, key_data);
		if (retval != -EINVAL) {
			zassert_equal(SB_PUBLIC_KEY_HASH_LEN, retval,
				"Unexpected public key error, %d.", retval);
			invalidate_public_key(key_data_idx);
			any_valid = true;
		}
	}
	zassert_true(any_valid,
		"All public keys invalidated, should not have booted!");
	printk("Rebooting. Should fail to validate because of invalid public "
		"keys.");
	sys_reboot(0);
	zassert_true(false, "should not come here.");
#endif
}


ZTEST_SUITE(test_bl_validation_neg, NULL, NULL, NULL, NULL, NULL);
