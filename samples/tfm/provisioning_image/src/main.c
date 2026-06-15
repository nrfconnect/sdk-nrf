/*
 * Copyright (c) 2022 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <hw_unique_key.h>
#include <bl_storage.h>

#include <config_implementation_id.h>

#if defined(CONFIG_NRFX_RRAMC)
#include <nrfx_rramc.h>
#endif

#if !defined(CONFIG_HAS_HW_NRF_CRACEN)
#include <identity_key.h>
#endif

LOG_MODULE_REGISTER(provisioning_image, LOG_LEVEL_DBG);

#define IMPLEMENTATION_ID_NUM_WORDS (sizeof(implementation_id_in_flash) / sizeof(uint32_t))

int provision_implementation_id(void)
{
#if defined(CONFIG_NRFX_RRAMC)
	/* BL_STORAGE is at the start of UICR OTP, so each field's OTP index
	 * is its word offset from BL_STORAGE.
	 */
	const uint32_t base_index = ((uint32_t)&BL_STORAGE->implementation_id -
				     (uint32_t)BL_STORAGE) / sizeof(uint32_t);

	for (uint32_t i = 0; i < IMPLEMENTATION_ID_NUM_WORDS; i++) {
		uint32_t word;

		memcpy(&word, &implementation_id_in_flash[i * sizeof(word)],
		       sizeof(word));
		if (!nrfx_rramc_otp_word_write(base_index + i, word)) {
			LOG_ERR("nrfx_rramc_otp_word_write failed at OTP index %u",
				base_index + i);
			return -1;
		}
	}
#else
	nrfx_nvmc_words_write((uint32_t)&BL_STORAGE->implementation_id,
			      &implementation_id_in_flash, IMPLEMENTATION_ID_NUM_WORDS);
#endif
	return 0;
}

int main(void)
{
	enum bl_storage_lcs lcs;
	int err;

	err = read_life_cycle_state(&lcs);
	if (err != 0) {
		LOG_INF("Failure: Cannot read PSA lifecycle state. Exiting!");
		return 0;
	}

	if (lcs != BL_STORAGE_LCS_ASSEMBLY) {
		LOG_INF("Failure: Lifecycle state is not ASSEMBLY as expected. Exiting!");
		return 0;
	}

	LOG_INF("Successfully verified PSA lifecycle state ASSEMBLY!");

	err = update_life_cycle_state(BL_STORAGE_LCS_PROVISIONING);
	if (err != 0) {
		LOG_INF("Failure: Cannot set PSA lifecycle state PROVISIONING. Exiting!");
		return 0;
	}

	err = read_life_cycle_state(&lcs);
	if (err != 0) {
		LOG_INF("Failure: Cannot read PSA lifecycle state. Exiting!");
		return 0;
	}

	if (lcs != BL_STORAGE_LCS_PROVISIONING) {
		LOG_INF("Lifecycle state was %d", lcs);
		LOG_INF("Failure: Lifecycle state is not PROVISION as expected. Exiting!");
		return 0;
	}

	LOG_INF("Successfully switched to PSA lifecycle state PROVISIONING!");

	LOG_INF("Generating random HUK keys (including MKEK)");
	err = hw_unique_key_write_random();
	if (err != HW_UNIQUE_KEY_SUCCESS) {
		LOG_INF("Failure: Writing random HUK keys failed. Exiting!");
		return 0;
	}

#if !defined(CONFIG_HAS_HW_NRF_CRACEN)
	if (identity_key_is_written()) {
		LOG_INF("Failure: Identity key slot already written. Exiting!");
		return 0;
	}

	LOG_INF("Writing the identity key to KMU");

#ifdef CONFIG_IDENTITY_KEY_DUMMY
	err = identity_key_write_dummy();
#else
	err = identity_key_write_random();
#endif

	if (err != IDENTITY_KEY_SUCCESS) {
		LOG_INF("Failure: Identity key write failed! Exiting!");
		return 0;
	}

	if (!identity_key_is_written()) {
		LOG_INF("Failure: Identity key is not written! Exiting!");
		return 0;
	}
#endif /* !CONFIG_HAS_HW_NRF_CRACEN */

	err = provision_implementation_id();
	if (err != 0) {
		LOG_INF("Failure: Writing implementation ID failed. Exiting!");
		return 0;
	}

	LOG_INF("Success!");

	return 0;
}
