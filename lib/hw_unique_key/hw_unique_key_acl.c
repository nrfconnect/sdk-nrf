/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <nrf_cc3xx_platform_kmu.h>
#include <hw_unique_key.h>
#include "hw_unique_key_internal.h"
#include <nrfx_nvmc.h>
#include <pm_config.h>
#include <fprotect.h>
#include <hal/nrf_acl.h>
#include <zephyr/kernel.h>

/* The magic data identify the flash page which contains the key */
static const uint32_t huk_magic[4] = {
	0xAD6DB556, 0xB56A5BAB, 0xDA5AAB55, 0x5AAD55AB
};

static uint32_t huk_addr = PM_HW_UNIQUE_KEY_PARTITION_ADDRESS;

void hw_unique_key_write(enum hw_unique_key_slot kmu_slot, const uint8_t *key)
{
	if (kmu_slot != HUK_KEYSLOT_KDR) {
		printk("KMU slot must be HUK_KEYSLOT_KDR on nRF52840\r\n");
		k_panic();
	}

	nrfx_nvmc_words_write(huk_addr, huk_magic, sizeof(huk_magic)/4);
	while (!nrfx_nvmc_write_done_check())
		;
	nrfx_nvmc_words_write(huk_addr + sizeof(huk_magic), key, HUK_SIZE_WORDS);
	while (!nrfx_nvmc_write_done_check())
		;
}

bool hw_unique_key_is_written(enum hw_unique_key_slot kmu_slot)
{
	if (kmu_slot != HUK_KEYSLOT_KDR) {
		printk("KMU slot must be HUK_KEYSLOT_KDR on nRF52840\r\n");
		k_panic();
	}

	uint32_t protection = fprotect_is_protected(huk_addr);

	if (protection == 3) {
		return true;
	}

	const uint32_t *huk_ptr = (const uint32_t *)huk_addr;

	for (int i = 0; i < (sizeof(huk_magic) / 4) + HUK_SIZE_WORDS; i++) {
		if (huk_ptr[i] != 0xFFFFFFFF) {
			return true;
		}
	}

	return false;
}

void hw_unique_key_load_kdr(void)
{
	int err = -1;

	/* Compare the huk_magic data with the content of the page */
	if (memcmp((uint8_t *)huk_addr, huk_magic, sizeof(huk_magic) != 0)) {
		printk("Could not load the HUK, magic data is not present\r\n");
		k_panic();
	}

	err = nrf_cc3xx_platform_kdr_load_key((uint8_t *)huk_addr +
					      sizeof(huk_magic));
	if (err != 0) {
		printk("The HUK loading failed with error code: %d\r\n", err);
		k_panic();
	}

	/* Lock the flash page which holds the key */
	err = fprotect_area_no_access(huk_addr, CONFIG_FPROTECT_BLOCK_SIZE);
	if (err != 0) {
		printk("Fprotect failed with error code: %d\r\n", err);
		k_panic();
	}
}
