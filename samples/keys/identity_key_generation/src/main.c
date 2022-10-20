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

LOG_MODULE_REGISTER(identity_key_generation, LOG_LEVEL_DBG);

void main(void)
{
	lcs_t lcs = UNKNOWN;
	int err = 0;

	err = read_life_cycle_state(&lcs);
	if(err != 0){
		LOG_INF("Failure: Cannot read PSA lifecycle state. Exiting!");
		return;
	}

	if(lcs != ASSEMBLY){
		LOG_INF("Failure: Lifecycle state is not ASSEMBLY as expected. Exiting!");
		return;
	}

	LOG_INF("Successfully verified PSA lifecycle state ASSEMBLY!");

	err = update_life_cycle_state(PROVISION);
	if(err != 0){
		LOG_INF("Failure: Cannot set PSA lifecycle state PROVISIONING. Exiting!");
		return;
	}

	err = read_life_cycle_state(&lcs);
	if(err != 0){
		LOG_INF("Failure: Cannot read PSA lifecycle state. Exiting!");
		return;
	}

	if(lcs != PROVISION){
		LOG_INF("Failure: Lifecycle state is not PROVISION as expected. Exiting!");
		return;
	}

	LOG_INF("Successfully switched to PSA lifecycle state PROVISIONING!");

	LOG_INF("Generating random HUK keys (including MKEK)");
	hw_unique_key_write_random();

	if (identity_key_is_written()) {
		LOG_INF("Failure: Identity key slot already written. Exiting!");
		return;
	}

	LOG_INF("Writing the identity key to KMU");

#ifdef CONFIG_IDENTITY_KEY_DUMMY
	identity_key_write_dummy();
#else
	identity_key_write_random();
#endif

	if (!identity_key_is_written()) {
		LOG_INF("Failure: Identity key is not written! Exiting!");
		return;
	}

	LOG_INF("Success!");
}
