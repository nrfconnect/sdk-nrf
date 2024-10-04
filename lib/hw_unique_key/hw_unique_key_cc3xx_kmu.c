/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_cc3xx_platform_kmu.h>
#include <hw_unique_key.h>
#include "hw_unique_key_internal.h"

#include <nrfx.h>
#include <nrfx_nvmc.h>

#include <mdk/nrf_erratas.h>
#include <zephyr/logging/log.h>

#define KMU_KEYSLOT_SIZE_WORDS 4

LOG_MODULE_DECLARE(hw_unique_key);

/* Check whether a Hardware Unique Key has been written to the KMU. */
static bool key_written(enum hw_unique_key_slot kmu_slot)
{
	uint32_t idx = kmu_slot;

	NRF_KMU->SELECTKEYSLOT = KMU_SELECT_SLOT(kmu_slot);

	if (nrfx_nvmc_uicr_word_read(&NRF_UICR_S->KEYSLOT.CONFIG[idx].PERM) != 0xFFFFFFFF) {
		NRF_KMU->SELECTKEYSLOT = 0;
		return true;
	}
	if (nrfx_nvmc_uicr_word_read(&NRF_UICR_S->KEYSLOT.CONFIG[idx].DEST) != 0xFFFFFFFF) {
		NRF_KMU->SELECTKEYSLOT = 0;
		return true;
	}
	for (int i = 0; i < KMU_KEYSLOT_SIZE_WORDS; i++) {
		if (nrfx_nvmc_uicr_word_read(&NRF_UICR_S->KEYSLOT.KEY[idx].VALUE[i]) !=
		    0xFFFFFFFF) {
			NRF_KMU->SELECTKEYSLOT = 0;
			return true;
		}
	}
	NRF_KMU->SELECTKEYSLOT = 0;
	return false;
}

static int write_slot(enum hw_unique_key_slot kmu_slot, uint32_t target_addr, const uint8_t *key)
{
	return nrf_cc3xx_platform_kmu_write_key_slot(
			kmu_slot, target_addr,
			NRF_CC3XX_PLATFORM_KMU_DEFAULT_PERMISSIONS, key);
}

int hw_unique_key_write(enum hw_unique_key_slot key_slot, const uint8_t *key)
{
	int err = write_slot(key_slot, NRF_CC3XX_PLATFORM_KMU_AES_ADDR, key);

#ifdef HUK_HAS_CC312
	if (err == 0) {
		err = write_slot(key_slot + 1, NRF_CC3XX_PLATFORM_KMU_AES_ADDR_2,
				key + (HUK_SIZE_BYTES / 2));
	}
#endif

	if (err != 0) {
		LOG_ERR("The HUK writing to: %d failed with error code: %d", key_slot, err);
		return -HW_UNIQUE_KEY_ERR_WRITE_FAILED;
	}

	return HW_UNIQUE_KEY_SUCCESS;
}

bool hw_unique_key_is_written(enum hw_unique_key_slot key_slot)
{
#ifdef HUK_HAS_CC312
	return key_written(key_slot) || key_written(key_slot + 1);
#else
	return key_written(key_slot);
#endif
}
