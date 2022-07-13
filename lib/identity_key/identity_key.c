/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <identity_key.h>
#include <nrf_cc3xx_platform_kmu.h>
#include <nrf_cc3xx_platform_ctr_drbg.h>
#include <nrf_cc3xx_platform_identity_key.h>
#include <hw_unique_key.h>
#include <zephyr/sys/util.h>
#include <psa/crypto.h>
#include <nrfx.h>
#include <nrfx_nvmc.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(identity_key);

static void generate_random_secp256r1_private_key(uint8_t *key_buff)
{
	psa_status_t status;
	psa_key_handle_t key_handle;
	size_t olen;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	__ASSERT(status == PSA_SUCCESS, "psa_crypto_init failed! Error: %d", status);

	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	/* Configure the key attributes for Curve type secp256r1
	 * This key needs to be exported from the volatile storage
	 */
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_EXPORT);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attributes, IDENTITY_KEY_SIZE_BYTES * 8);

	status = psa_generate_key(&key_attributes, &key_handle);
	__ASSERT(status == PSA_SUCCESS, "psa_generate_key failed! Error: %d", status);

	status = psa_export_key(key_handle, key_buff, IDENTITY_KEY_SIZE_BYTES, &olen);
	__ASSERT(status == PSA_SUCCESS, "psa_export_key failed! Error: %d", status);

	__ASSERT(olen == IDENTITY_KEY_SIZE_BYTES, "Exported size missmatch! Key size: %d", olen);

	status = psa_destroy_key(key_handle);
	__ASSERT(status == PSA_SUCCESS, "psa_destroy_key failed! Error: %d", status);
}

void identity_key_write_random(void)
{
	uint8_t key_buff[IDENTITY_KEY_SIZE_BYTES];

	/* This function will panic if the key generation fails */
	generate_random_secp256r1_private_key(key_buff);

	/* Write the key into KMU using the MKEK as encryption key */
	identity_key_write_key(key_buff);

	/* Clear the unencrypted key from RAM for security reasons */
	nrf_cc3xx_platform_identity_key_free(key_buff);
}

void identity_key_write_key(uint8_t key[IDENTITY_KEY_SIZE_BYTES])
{
	int err;

	__ASSERT(identity_key_mkek_is_written(), "Could not find the MKEK!");

	err = nrf_cc3xx_platform_identity_key_store(NRF_KMU_SLOT_KIDENT, key);
	__ASSERT(err == NRF_CC3XX_PLATFORM_SUCCESS, "Identity key write failed ! Error: %d", err);

}

/* This key is used by the TF-M regression tests and it is taken from the TF-M
 * repository. It should ONLY be used for testing and debugging purposes.
 *
 * TF-M repo path: platform/ext/common/template/tfm_initial_attestation_key.pem
 *
 * Intentionally non-static to ensure it is stored in RAM and not FLASH
 * (limitation in CryptoCell peripheral.)
 *
 * #######  DO NOT USE THIS KEY IN PRODUCTION #######
 */
uint8_t dummy_identity_secp256r1_private_key[IDENTITY_KEY_SIZE_BYTES] = {
	0xA9, 0xB4, 0x54, 0xB2, 0x6D, 0x6F, 0x90, 0xA4,
	0xEA, 0x31, 0x19, 0x35, 0x64, 0xCB, 0xA9, 0x1F,
	0xEC, 0x6F, 0x9A, 0x00, 0x2A, 0x7D, 0xC0, 0x50,
	0x4B, 0x92, 0xA1, 0x93, 0x71, 0x34, 0x58, 0x5F
};

void identity_key_write_dummy(void)
{
	LOG_INF("WARNING: Using a dummy identity key not meant for production!");
	identity_key_write_key(dummy_identity_secp256r1_private_key);
}

bool identity_key_mkek_is_written(void)
{
	return hw_unique_key_is_written(HUK_KEYSLOT_MKEK);
}

bool identity_key_is_written(void)
{
	int perm;

	for (int i = 0; i < 2; i++) {
		NRF_KMU->SELECTKEYSLOT = KMU_SELECT_SLOT(NRF_KMU_SLOT_KIDENT + i);

		perm = nrfx_nvmc_uicr_word_read(
			&NRF_UICR_S->KEYSLOT.CONFIG[NRF_KMU_SLOT_KIDENT + i].PERM);

		NRF_KMU->SELECTKEYSLOT = 0;

		/* If the slot permissions are wrong we can assume that the key is not written. */
		if (perm != NRF_CC3XX_PLATFORM_KMU_IDENTITY_KEY_PERMISSIONS) {
			return false;
		}
	}

	/* Both slots have the correct permissions set */
	return true;
}

int identity_key_read(uint8_t key[IDENTITY_KEY_SIZE_BYTES])
{
	int err;

	/* MKEK is required to retrieve key */
	if (!identity_key_mkek_is_written()) {
		return -ERR_IDENTITY_KEY_MKEK_MISSING;
	}

	if (!identity_key_is_written()) {
		return -ERR_IDENTITY_KEY_MISSING;
	}

	/* Retrieve the identity key */
	err = nrf_cc3xx_platform_identity_key_retrieve(NRF_KMU_SLOT_KIDENT, key);
	if (err != NRF_CC3XX_PLATFORM_SUCCESS) {
		LOG_INF("The identity key read from: %d", NRF_KMU_SLOT_KIDENT);
		LOG_INF("failed with error code: %d", err);
		return -ERR_IDENTITY_KEY_READ_FAILED;
	}

	return 0;
}
