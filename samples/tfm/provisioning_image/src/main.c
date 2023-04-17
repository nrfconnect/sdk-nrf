/*
 * Copyright (c) 2022 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include <hw_unique_key.h>
#include <identity_key.h>
#include <psa/crypto.h>
#include <bl_storage.h>

#include <config_implementation_id.h>

LOG_MODULE_REGISTER(provisioning_image, LOG_LEVEL_DBG);

void provision_implementation_id(void)
{
	uint32_t num_words_to_write = 32 / 4;

	nrfx_nvmc_words_write((uint32_t)&BL_STORAGE->implementation_id, &implementation_id_in_flash,
		num_words_to_write);
}

int main(void)
{
	enum lcs lcs;
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
	hw_unique_key_write_random();

	if (identity_key_is_written()) {
		LOG_INF("Failure: Identity key slot already written. Exiting!");
		return 0;
	}

	LOG_INF("Writing the identity key to KMU");

#ifdef CONFIG_IDENTITY_KEY_DUMMY
	identity_key_write_dummy();
#else
	identity_key_write_random();
#endif

	if (!identity_key_is_written()) {
		LOG_INF("Failure: Identity key is not written! Exiting!");
		return 0;
	}

	provision_implementation_id();

	LOG_INF("Success!");

	return 0;
}
