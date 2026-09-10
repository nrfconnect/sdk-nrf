/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <fw_info.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/devicetree.h>
#include <bl_storage.h>

#if defined(CONFIG_SOC_SERIES_NRF54L)
#include <hal/nrf_rramc.h>
#include <nrfx_rramc.h>

#define ERASE_BLOCK_SIZE DT_PROP(DT_CHOSEN(zephyr_flash), erase_block_size)
#define REGION_ALIGN     0x400

static void rramc_prepare(void)
{
	static bool prepared;

	if (!prepared) {
		nrfx_rramc_config_t config = NRFX_RRAMC_DEFAULT_CONFIG(0);

		config.mode_write = true;
		nrfx_rramc_init(&config, NULL);
		nrfx_rramc_write_enable_set(true, 0);
		prepared = true;
	}
}

static int flash_page_erase(uint32_t addr)
{
	rramc_prepare();

	for (uint32_t offset = 0; offset < ERASE_BLOCK_SIZE; offset += sizeof(uint32_t)) {
		nrf_rramc_word_write(addr + offset, 0xFFFFFFFF);
	}

	return 0;
}

static void flash_words_write(uint32_t addr, const void *src, uint32_t num_words)
{
	rramc_prepare();
	nrf_rramc_buffer_write(addr, (void *)src, num_words * sizeof(uint32_t));
}
#else
#include <nrfx_nvmc.h>

#define ERASE_BLOCK_SIZE DT_PROP(DT_CHOSEN(zephyr_flash), erase_block_size)
#define REGION_ALIGN     0x8000

static int flash_page_erase(uint32_t addr)
{
	return nrfx_nvmc_page_erase(addr);
}

static void flash_words_write(uint32_t addr, const void *src, uint32_t num_words)
{
	nrfx_nvmc_words_write(addr, src, num_words);
}
#endif

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

	/* Round up to the next memory protection region. */
	uint32_t new_addr = ROUND_UP(S0_SLOT_ADDRESS + (S0_SLOT_SIZE / 2), REGION_ALIGN);

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
		int ret = flash_page_erase(S1_SLOT_ADDRESS);

		zassert_equal(0, ret, "Erase failed.\r\n");
	} else {
		/* First boot */

		/* Copy app */
		for (uint32_t erase_addr = new_addr;
			erase_addr < (new_addr + copy_len);
			erase_addr += ERASE_BLOCK_SIZE) {
			int ret = flash_page_erase(erase_addr);

			zassert_equal(0, ret, "Erase failed.\r\n");
		}
		flash_words_write(new_addr, (const uint32_t *)S0_SLOT_ADDRESS,
			copy_len / 4);

		/* Write to S1 */
		flash_words_write(S1_SLOT_ADDRESS, &s1_info,
			ROUND_UP(sizeof(s1_info), 4) / 4);

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
		flash_words_write(s1_info.address + ROUND_UP(s1_info.size, 4), val_info,
			VAL_INFO_MAX_SIZE);

		/* Reboot */
		printk("Rebooting. Should fail to validate slot 1.");
		sys_reboot(0);
		zassert_true(false, "should not come here.");
	}
}


ZTEST(test_bl_validation_neg, test_validation_neg2)
{
	/* testcase.yaml nrf52 variant of test does not catch below regex */
#ifndef CONFIG_SOC_SERIES_NRF52
#if defined(CONFIG_SOC_SERIES_NRF54L)
	/* Ed25519 validation uses KMU keys, not bl_storage public keys. */
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
#else
	ztest_test_skip();
#endif
}


static void *test_suite_setup(void)
{
#if defined(CONFIG_SOC_SERIES_NRF54L)
	rramc_prepare();
#endif

	return NULL;
}

ZTEST_SUITE(test_bl_validation_neg, NULL, test_suite_setup, NULL, NULL, NULL);
