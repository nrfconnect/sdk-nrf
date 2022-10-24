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

LOG_MODULE_REGISTER(identity_key_generation, LOG_LEVEL_DBG);

void main(void)
{
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
