/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>

#include <cracen_psa_kmu.h>

#ifdef CONFIG_BUILD_WITH_TFM
#include <tfm_ns_interface.h>
#endif

#include "main.h"
#include "key_operations.h"

#define APP_SUCCESS	    (0)
#define APP_ERROR	    (-1)
#define APP_SUCCESS_MESSAGE "Example finished successfully!"
#define APP_ERROR_MESSAGE   "Example exited with error!"

LOG_MODULE_REGISTER(kmu_usage_nrf54l, LOG_LEVEL_DBG);

/* ====================================================================== */
/*			Global variables/defines for the KMU usage example			  */

#define SAMPLE_MAX_KEY_COUNT (4)

static sample_key_entry_t m_sample_keys[SAMPLE_MAX_KEY_COUNT] = {
	/* Pre-provisioned keys.
	 * These keys remain in the KMU between MCU resets.
	 */
	{
		/* The key requires 1 KMU slot */
		.key_id = PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED,
						       PSA_KEY_ID_USER_MIN),
		.supported_operations = {
			.gen_key_cb = NULL,
			.use_key_cb = key_operations_use_aes_key,
		}
	},
	{
		/* The key requires 2 KMU slots */
		.key_id = PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW,
						       PSA_KEY_ID_USER_MIN + 1),
		.supported_operations = {
			.gen_key_cb = NULL,
			.use_key_cb = key_operations_use_eddsa_key_pair,
		}
	},

	/* Generated keys.
	 * These keys will be erased from the KMU after MCU reset.
	 */
	{
		/* The key requires 1 KMU slot + 2 additional slots
		 *are required since the key is encrypted
		 */
		.key_id = PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED,
						       PSA_KEY_ID_USER_MIN + 3),
		.supported_operations = {
			.gen_key_cb = key_operations_generate_aes_key,
			.use_key_cb = key_operations_use_aes_key,
		}
	},
	{
		/* The key requires 2 KMU slots */
		.key_id = PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW,
						       PSA_KEY_ID_USER_MIN + 6),
		.supported_operations = {
			.gen_key_cb = key_operations_generate_ecdsa_key_pair,
			.use_key_cb = key_operations_use_ecdsa_key_pair,
		}
	},
};

/* ====================================================================== */

int crypto_init(void)
{
	psa_status_t status;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int crypto_finish(void)
{
	psa_status_t status;

	LOG_INF("Destroying non-provisioned keys...");

	/* Destroy the key handles for generated keys only */
	for (uint8_t key_entry = 0; key_entry < SAMPLE_MAX_KEY_COUNT; key_entry++) {
		if (m_sample_keys[key_entry].supported_operations.gen_key_cb != NULL) {
			status = psa_destroy_key(m_sample_keys[key_entry].key_id);
			if (status != PSA_SUCCESS) {
				LOG_INF("psa_destroy_key failed! (Error: %d, key_entry: %d)",
					status, key_entry);
				return APP_ERROR;
			}
		}
	}

	return APP_SUCCESS;
}

int use_kmu_keys(void)
{
	psa_status_t status;
	use_key_callback_t use_key_cb;
	use_key_callback_t gen_key_cb;

	LOG_INF("Using keys...");

	for (uint8_t key_entry = 0; key_entry < SAMPLE_MAX_KEY_COUNT; key_entry++) {
		use_key_cb = m_sample_keys[key_entry].supported_operations.use_key_cb;
		gen_key_cb = m_sample_keys[key_entry].supported_operations.gen_key_cb;

		if (use_key_cb != NULL) {
			LOG_INF("Using key [%d]\n", key_entry);
			status = use_key_cb(&m_sample_keys[key_entry].key_id);
			if (status != PSA_SUCCESS) {
				LOG_INF("Failed to use key (entry: %d, error: %d)", key_entry,
					status);

				if (gen_key_cb == NULL) {
					LOG_WRN("Please make sure the key is pre-provisioned!");
				} else {
					return APP_ERROR;
				}
			}
		} else {
			LOG_WRN("Can't check key usage: no callback (entry: %d)", key_entry);
		}
	}

	LOG_INF("Key usage finished");

	return APP_SUCCESS;
}

int prepare_kmu_keys(void)
{
	psa_status_t status;
	generate_key_callback_t gen_key_cb;

	LOG_INF("Preparing non-provisioned keys...");

	for (uint8_t key_entry = 0; key_entry < SAMPLE_MAX_KEY_COUNT; key_entry++) {
		gen_key_cb = m_sample_keys[key_entry].supported_operations.gen_key_cb;

		if (gen_key_cb != NULL) {
			LOG_INF("Generating key [%d]\n", key_entry);
			status = gen_key_cb(&m_sample_keys[key_entry].key_id);
			if (status == PSA_ERROR_ALREADY_EXISTS) {
				LOG_WRN("Failed to generate key [%d]: key already exists! "
					"Erase, pre-provision and flash the board again.",
					key_entry);
			}

			if (status != PSA_SUCCESS) {
				LOG_INF("Failed to generate key (entry: %d, error: %d)", key_entry,
					status);
				return APP_ERROR;
			}
		}
	}

	LOG_INF("All keys prepared");

	return APP_SUCCESS;
}

int main(void)
{
	int status;

	LOG_INF("Starting KMU usage example...");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = prepare_kmu_keys();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = use_kmu_keys();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = crypto_finish();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	LOG_INF(APP_SUCCESS_MESSAGE);

	return APP_SUCCESS;
}
