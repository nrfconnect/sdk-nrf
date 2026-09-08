/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <fw_info.h>
#if defined(CONFIG_NRFX_RRAMC)
#include <haly/nrfy_rramc.h>
#define PROTECTION_BLOCK_SIZE 0x800

static void rram_words_write(uint32_t address, const void *src, uint32_t num_words)
{
	nrfy_rramc_words_write(NRF_RRAMC, address, src, num_words);
}
#else
#include <nrfx_nvmc.h>
#define PROTECTION_BLOCK_SIZE 0x8000
#endif
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/devicetree.h>
#include <bl_storage.h>

#define S0_SLOT_ADDRESS PARTITION_ADDRESS(s0_partition)
#define S0_SLOT_SIZE PARTITION_SIZE(s0_partition)
#define S1_SLOT_ADDRESS PARTITION_ADDRESS(s1_partition)

/* These symbols are defined in linker scripts. */
extern const uint32_t _ext_apis_size[];
extern const uint32_t _ext_apis_req_size[];

extern const struct fw_info m_firmware_info;

#define VAL_INFO_MAX_SIZE 0x1000
uint32_t val_info_buf[VAL_INFO_MAX_SIZE];

ZTEST(test_bl_validation_neg, test_validation_neg1)
{
	uint32_t copy_len = ROUND_UP((uint32_t)_flash_used, 4);

#if defined(CONFIG_NRFX_RRAMC)
	/* b0 write-protects the active slot (s0); place the displaced image in s1. */
	uint32_t new_addr = ROUND_UP(S1_SLOT_ADDRESS + CONFIG_SB_IMAGE_BOOT_OFFSET,
				     PROTECTION_BLOCK_SIZE);
#else
	/* Round up to at least the next protection region. */
	uint32_t new_addr = ROUND_UP(S0_SLOT_ADDRESS + (S0_SLOT_SIZE / 2),
				     PROTECTION_BLOCK_SIZE);
#endif

	const struct fw_info s1_info = {
		.magic = {FIRMWARE_INFO_MAGIC},
		.total_size = S0_SLOT_ADDRESS - S1_SLOT_ADDRESS,
		.size = ((uint32_t)_flash_used),
		.version = CONFIG_FW_INFO_FIRMWARE_VERSION + 1,
		.address = new_addr,
		.boot_address = new_addr,
		.valid = CONFIG_FW_INFO_VALID_VAL,
		.reserved = {0, 0, 0, 0},
		.ext_api_num = 0,
		.ext_api_request_num = 0,
	};

	const struct fw_info *s1_info_copied = fw_info_find(S1_SLOT_ADDRESS);

	if (s1_info_copied) {
		/* Second boot */
		zassert_not_equal(CONFIG_FW_INFO_VALID_VAL,
			s1_info_copied->valid, "Failed to invalidate S1.\r\n");
		zassert_equal((uint32_t)s1_info_copied, S1_SLOT_ADDRESS,
			"S1 info found at wrong address.\r\n");
#if defined(CONFIG_NRFX_RRAMC)
		rram_words_write(S1_SLOT_ADDRESS, (const uint32_t *)&(uint32_t){0},
			ROUND_UP(sizeof(struct fw_info), 4) / 4);
#else
		int ret = nrfx_nvmc_page_erase(S1_SLOT_ADDRESS);

		zassert_equal(0, ret, "Erase failed.\r\n");
#endif
	} else {
		/* First boot */

		/* Copy app */
#if defined(CONFIG_NRFX_RRAMC)
		rram_words_write(new_addr, (const uint32_t *)S0_SLOT_ADDRESS,
			copy_len / 4);
#else
		for (uint32_t erase_addr = new_addr;
			erase_addr < (new_addr + copy_len);
			erase_addr += DT_PROP(DT_CHOSEN(zephyr_flash),
							erase_block_size)) {
			int ret = nrfx_nvmc_page_erase(new_addr);

			zassert_equal(0, ret, "Erase failed.\r\n");
		}
		nrfx_nvmc_words_write(new_addr, (const uint32_t *)S0_SLOT_ADDRESS,
			copy_len / 4);
#endif

		/* Write to S1 */
#if defined(CONFIG_NRFX_RRAMC)
		rram_words_write(S1_SLOT_ADDRESS, (const uint32_t *)&s1_info,
			ROUND_UP(sizeof(s1_info), 4) / 4);
#else
		nrfx_nvmc_words_write(S1_SLOT_ADDRESS, &s1_info,
			ROUND_UP(sizeof(s1_info), 4) / 4);
#endif

		zassert_mem_equal(&s1_info, (void *)S1_SLOT_ADDRESS,
			sizeof(s1_info), "Failed to copy S1 info.\r\n");

		s1_info_copied = fw_info_find(S1_SLOT_ADDRESS);
		zassert_equal((uint32_t)s1_info_copied, S1_SLOT_ADDRESS,
			"S1 info wrongly copied.\r\n");

		/* Modify copied app's validation info */
		memcpy(val_info_buf, (const uint32_t *)(S0_SLOT_ADDRESS + copy_len),
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
#if defined(CONFIG_NRFX_RRAMC)
		rram_words_write(s1_info.address + ROUND_UP(s1_info.size, 4), val_info,
			VAL_INFO_MAX_SIZE);
#else
		nrfx_nvmc_words_write(s1_info.address + ROUND_UP(s1_info.size, 4), val_info,
			VAL_INFO_MAX_SIZE);
#endif

		/* Reboot */
		printk("Rebooting. Should fail to validate slot 1.");
		sys_reboot(0);
		zassert_true(false, "should not come here.");
	}
}


ZTEST(test_bl_validation_neg, test_validation_neg2)
{
	/* Skipped on platforms in bootloader.bl_validation.negative.no_otp_keys. */
#if defined(CONFIG_SOC_SERIES_NRF52) || defined(CONFIG_SOC_SERIES_NRF54L)
	ztest_test_skip();
#else
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
