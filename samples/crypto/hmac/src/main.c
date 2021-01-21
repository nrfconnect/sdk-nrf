/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <logging/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <crypto_common.h>

LOG_MODULE_REGISTER(hmac, LOG_LEVEL_DBG);

/* ====================================================================== */
/*				Global variables/defines for the HMAC example			  */

#define NRF_CRYPTO_EXAMPLE_HMAC_TEXT_SIZE (100)
#define NRF_CRYPTO_EXAMPLE_HMAC_KEY_SIZE (32)

/* Below text is used as plaintext for signing/verifiction */
static uint8_t m_plain_text[NRF_CRYPTO_EXAMPLE_HMAC_TEXT_SIZE] = {
	"Example string to demonstrate basic usage of HMAC signing/verification."
};

static uint8_t hmac[NRF_CRYPTO_EXAMPLE_HMAC_KEY_SIZE];

static psa_key_handle_t key_handle;
/* ====================================================================== */

int crypto_finish(void)
{
	psa_status_t status;

	/* Destroy the key handle */
	status = psa_destroy_key(key_handle);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_destroy_key", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int generate_key(void)
{
	psa_status_t status;

	PRINT_MESSAGE("Generating random HMAC key...");

	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes,
				PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_SIGN_HASH);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_HMAC);
	psa_set_key_bits(&key_attributes, 256);

	/* Generate a random key. The key is not exposed to the application,
	 * we can use it to encrypt/decrypt using the key handle
	 */
	status = psa_generate_key(&key_attributes, &key_handle);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_generate_key", status);
		return APP_ERROR;
	}

	/* After the key handle is acquired the attributes are not needed */
	psa_reset_key_attributes(&key_attributes);

	PRINT_MESSAGE("HMAC key generated successfully!");

	return APP_SUCCESS;
}

int hmac_sign(void)
{
	uint32_t olen;
	psa_status_t status;
	psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;

	PRINT_MESSAGE("Signing using HMAC ...");

	/* Initialize the HMAC signing operation */
	status = psa_mac_sign_setup(&operation, key_handle, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_mac_sign_setup", status);
		return APP_ERROR;
	}

	/* Perform the HMAC signing */
	status = psa_mac_update(&operation, m_plain_text, sizeof(m_plain_text));
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_mac_update", status);
		return APP_ERROR;
	}

	/* Finalize the HMAC signing */
	status = psa_mac_sign_finish(&operation, hmac, sizeof(hmac), &olen);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_mac_sign_finish", status);
		return APP_ERROR;
	}

	PRINT_MESSAGE("Signing successful!");
	PRINT_HEX("Plaintext", m_plain_text, sizeof(m_plain_text));
	PRINT_HEX("HMAC", hmac, sizeof(hmac));

	return APP_SUCCESS;
}

int hmac_verify(void)
{
	psa_status_t status;
	psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;

	PRINT_MESSAGE("Verifying the HMAC signature...");

	/* Initialize the HMAC verification operation */
	status = psa_mac_verify_setup(&operation, key_handle, PSA_ALG_HMAC(PSA_ALG_SHA_256));
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_mac_verify_setup", status);
		return APP_ERROR;
	}

	/* Perform the HMAC verification */
	status = psa_mac_update(&operation, m_plain_text, sizeof(m_plain_text));
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_mac_update", status);
		return APP_ERROR;
	}

	/* Finalize the HMAC verification */
	status = psa_mac_verify_finish(&operation, hmac, sizeof(hmac));
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_mac_verify_finish", status);
		return APP_ERROR;
	}

	PRINT_MESSAGE("HMAC verified successfully!");

	return APP_SUCCESS;
}

int main(void)
{
	int status;

	PRINT_MESSAGE("Starting HMAC example...");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		PRINT_MESSAGE(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = generate_key();
	if (status != APP_SUCCESS) {
		PRINT_MESSAGE(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = hmac_sign();
	if (status != APP_SUCCESS) {
		PRINT_MESSAGE(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = hmac_verify();
	if (status != APP_SUCCESS) {
		PRINT_MESSAGE(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = crypto_finish();
	if (status != APP_SUCCESS) {
		PRINT_MESSAGE(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	PRINT_MESSAGE(APP_SUCCESS_MESSAGE);

	return APP_SUCCESS;
}
