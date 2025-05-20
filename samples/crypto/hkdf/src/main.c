/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>

#ifdef CONFIG_BUILD_WITH_TFM
#include <tfm_ns_interface.h>
#endif

#define APP_SUCCESS		(0)
#define APP_ERROR		(-1)
#define APP_SUCCESS_MESSAGE "Example finished successfully!"
#define APP_ERROR_MESSAGE "Example exited with error!"

#define PRINT_HEX(p_label, p_text, len)\
	({\
		LOG_INF("---- %s (len: %u): ----", p_label, len);\
		LOG_HEXDUMP_INF(p_text, len, "Content:");\
		LOG_INF("---- %s end  ----", p_label);\
	})

LOG_MODULE_REGISTER(hkdf, LOG_LEVEL_DBG);

/* ====================================================================== */
/*				Global variables/defines for the HKDF example			  */

#define NRF_CRYPTO_EXAMPLE_HKDF_INPUT_KEY_SIZE (22)
#define NRF_CRYPTO_EXAMPLE_HKDF_SALT_SIZE (13)
#define NRF_CRYPTO_EXAMPLE_HKDF_AINFO_SIZE (10)
#define NRF_CRYPTO_EXAMPLE_HKDF_OUTPUT_KEY_SIZE (42)

/* Test data from RFC 5869 Test Case 1 (https://tools.ietf.org/html/rfc5869) */
static uint8_t m_input_key[NRF_CRYPTO_EXAMPLE_HKDF_INPUT_KEY_SIZE] = {
	0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
	0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b
};

static uint8_t m_salt[NRF_CRYPTO_EXAMPLE_HKDF_SALT_SIZE] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
	0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c
};

/* Additional info material optionally used to increase entropy. */
static uint8_t m_ainfo[NRF_CRYPTO_EXAMPLE_HKDF_AINFO_SIZE] = {
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9
};

/* Buffer to hold the key generated from HKDF */
static uint8_t m_output_key[NRF_CRYPTO_EXAMPLE_HKDF_OUTPUT_KEY_SIZE];

/* Expected output key material (result of HKDF operation) */
static uint8_t m_expected_output_key[NRF_CRYPTO_EXAMPLE_HKDF_OUTPUT_KEY_SIZE] = {
	0x3c, 0xb2, 0x5f, 0x25, 0xfa, 0xac, 0xd5, 0x7a, 0x90, 0x43, 0x4f,
	0x64, 0xd0, 0x36, 0x2f, 0x2a, 0x2d, 0x2d, 0x0a, 0x90, 0xcf, 0x1a,
	0x5a, 0x4c, 0x5d, 0xb0, 0x2d, 0x56, 0xec, 0xc4, 0xc5, 0xbf, 0x34,
	0x00, 0x72, 0x08, 0xd5, 0xb8, 0x87, 0x18, 0x58, 0x65
};

psa_key_id_t input_key_id;
psa_key_id_t output_key_id;
/* ====================================================================== */

int crypto_init(void)
{
	psa_status_t status;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS)
		return APP_ERROR;

	return APP_SUCCESS;
}

int crypto_finish(void)
{
	psa_status_t status;

	/* Destroy the key handle */
	status = psa_destroy_key(input_key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_destroy_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	status = psa_destroy_key(output_key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_destroy_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int import_input_key(void)
{
	psa_status_t status;
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	/* Configure the input key attributes */
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_HKDF(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_DERIVE);
	psa_set_key_bits(&key_attributes,
			 NRF_CRYPTO_EXAMPLE_HKDF_INPUT_KEY_SIZE * 8);

	/* Import the master key into the keystore */
	status = psa_import_key(&key_attributes,
				m_input_key,
				sizeof(m_input_key),
				&input_key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_import_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int export_derived_key(void)
{
	size_t olen;
	psa_status_t status;
	int cmp_status;
	/* Export the generated key content to verify it's value */
	status = psa_export_key(output_key_id, m_output_key, sizeof(m_output_key), &olen);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_export_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("Compare derived key with expected value...");
	cmp_status = memcmp(m_expected_output_key, m_output_key, sizeof(m_output_key));
	if (cmp_status != 0) {
		LOG_INF("Error, the derived key doesn't match the expected value!");
		return APP_ERROR;
	}

	PRINT_HEX("Output key", m_output_key, sizeof(m_output_key));

	return APP_SUCCESS;
}

int derive_hkdf(void)
{
	psa_status_t status;
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_derivation_operation_t operation =
		PSA_KEY_DERIVATION_OPERATION_INIT;

	LOG_INF("Deriving a key using HKDF and SHA256...");

	/* Derived key settings
	 * WARNING: This key usage makes the key exportable which is not safe and
	 * is only done to demonstrate the validity of the results. Please do not use
	 * this in production environments.
	 */
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_RAW_DATA);
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_EXPORT); /* DONT USE IN PRODUCTION */
	psa_set_key_bits(&key_attributes, NRF_CRYPTO_EXAMPLE_HKDF_OUTPUT_KEY_SIZE * 8);

	/* Set the derivation algorithm */
	status = psa_key_derivation_setup(&operation,
					  PSA_ALG_HKDF(PSA_ALG_SHA_256));
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_setup failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* Set the salt for the operation */
	status = psa_key_derivation_input_bytes(&operation,
						PSA_KEY_DERIVATION_INPUT_SALT,
						m_salt,
						sizeof(m_salt));
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_input_bytes failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* Set the master key for the operation */
	status = psa_key_derivation_input_key(
		&operation, PSA_KEY_DERIVATION_INPUT_SECRET, input_key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_input_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* Set the additional info for the operation */
	status = psa_key_derivation_input_bytes(&operation,
						PSA_KEY_DERIVATION_INPUT_INFO,
						m_ainfo,
						sizeof(m_ainfo));
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_input_bytes failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* Store the derived key in the keystore slot pointed by out_key_id */
	status = psa_key_derivation_output_key(&key_attributes, &operation, &output_key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_output_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* Clean up the context */
	status = psa_key_derivation_abort(&operation);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_abort failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("Key derivation successful!");
	PRINT_HEX("Input key", m_input_key, sizeof(m_input_key));
	PRINT_HEX("Salt", m_salt, sizeof(m_salt));
	PRINT_HEX("Additional data", m_ainfo, sizeof(m_ainfo));

	return APP_SUCCESS;
}

int main(void)
{
	int status;

	LOG_INF("Starting HKDF example...");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = import_input_key();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = derive_hkdf();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = export_derived_key();
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
