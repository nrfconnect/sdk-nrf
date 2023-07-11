/*
 * Copyright (c) 2022 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include <psa/crypto.h>
#include <nrf_cc3xx_platform.h>
#include <nrf_cc3xx_platform_ctr_drbg.h>
#include <nrf_cc3xx_platform_defines.h>
#include <nrf_cc3xx_platform_identity_key.h>
#include <identity_key.h>

#define IDENTITY_KEY_PUBLIC_KEY_SIZE (65)
uint8_t m_pub_key[IDENTITY_KEY_PUBLIC_KEY_SIZE];

LOG_MODULE_REGISTER(identity_key_usage, LOG_LEVEL_DBG);

int main(void)
{
	int err;
	psa_status_t status;
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id_out = 0;
	uint8_t key[IDENTITY_KEY_SIZE_BYTES];
	size_t olen;

	err = nrf_cc3xx_platform_init();
	if (err != NRF_CC3XX_PLATFORM_SUCCESS) {
		LOG_INF("nrf_cc3xx_platform_init returned error: %d", err);
		return 0;
	}

	LOG_INF("Initializing PSA crypto.");

	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_crypto_init returned error: %d", status);
		return 0;
	}

	/* Configure the key attributes for Curve type secp256r1*/
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attributes, IDENTITY_KEY_SIZE_BYTES * 8);

	if (!identity_key_is_written()) {
		LOG_INF("No identity key found in KMU (required). Exiting!");
		return 0;
	}

	LOG_INF("Reading the identity key.");
	err = identity_key_read(key);
	if (err != IDENTITY_KEY_SUCCESS) {
		LOG_INF("Identity key read failed! (Error: %d). Exiting!", err);
		return 0;
	}

	LOG_INF("Importing the identity key into PSA crypto.");
	status = psa_import_key(&key_attributes, key, IDENTITY_KEY_SIZE_BYTES, &key_id_out);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_import_key failed! (Error: %d). Exiting!", status);
		return 0;
	}

	/* Clear out the local copy of the key to prevent leakage */
	nrf_cc3xx_platform_identity_key_free(key);

	LOG_INF("Exporting the public key corresponding to the identity key.");
	status = psa_export_public_key(key_id_out, m_pub_key, sizeof(m_pub_key), &olen);

	if (status != PSA_SUCCESS) {
		LOG_INF("psa_export_public_key failed! (Error: %d). Exiting!", status);
		return 0;
	}

	if (olen != IDENTITY_KEY_PUBLIC_KEY_SIZE) {
		LOG_INF("Output length is invalid! (Expected %d, got %d). Exiting!",
			IDENTITY_KEY_PUBLIC_KEY_SIZE, olen);
		return 0;
	}

	LOG_HEXDUMP_INF(m_pub_key, IDENTITY_KEY_PUBLIC_KEY_SIZE, "Public key:");

	LOG_INF("Success!");

	return 0;
}
