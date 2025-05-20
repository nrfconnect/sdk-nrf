/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdint.h>
#include <psa/crypto.h>

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

LOG_MODULE_REGISTER(pkbdf2, LOG_LEVEL_DBG);

/* ====================================================================== */
/*        Global variables/defines for the PBKDF2 example                 */

#define PBKDF2_SAMPLE_INPUT_KEY_SIZE (6)
#define PBKDF2_SAMPLE_SALT_SIZE (4)
#define PBKDF2_SAMPLE_OUTPUT_KEY_SIZE (64)

/* Test data from RFC 5869 Test Case 2
 * https://datatracker.ietf.org/doc/html/rfc7914.html#section-11
 */
static const uint8_t m_input_password[PBKDF2_SAMPLE_INPUT_KEY_SIZE] = {
	'p', 'a', 's', 's', 'w', 'd',
};

static const uint8_t m_salt[PBKDF2_SAMPLE_SALT_SIZE] = {
	's', 'a', 'l', 't'
};

/* (NIST) "Recommendation for Password-Based Key Derivation:
 * The iteration count shall be  selected as large as possible, as
 * long as the time required to generate the key using the entered
 * password is acceptable for the users. [...] A minimum iteration
 * count of 1,000 is recommended.
 */
static uint32_t m_iteration_count = 1;

/* Buffer to hold the output generated from PKBD2S */
static uint8_t m_output_key[PBKDF2_SAMPLE_OUTPUT_KEY_SIZE];

static const uint8_t m_expected_output_key[PBKDF2_SAMPLE_OUTPUT_KEY_SIZE] = {
	0x55, 0xac, 0x04, 0x6e, 0x56, 0xe3, 0x08, 0x9f,
	0xec, 0x16, 0x91, 0xc2, 0x25, 0x44, 0xb6, 0x05,
	0xf9, 0x41, 0x85, 0x21, 0x6d, 0xde, 0x04, 0x65,
	0xe6, 0x8b, 0x9d, 0x57, 0xc2, 0x0d, 0xac, 0xbc,
	0x49, 0xca, 0x9c, 0xcc, 0xf1, 0x79, 0xb6, 0x45,
	0x99, 0x16, 0x64, 0xb3, 0x9d, 0x77, 0xef, 0x31,
	0x7c, 0x71, 0xb8, 0x45, 0xb1, 0xe3, 0x0b, 0xd5,
	0x09, 0x11, 0x20, 0x41, 0xd3, 0xa1, 0x97, 0x83,
};

static psa_key_id_t m_input_key_id;
static psa_key_id_t m_output_key_id;
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
	status = psa_destroy_key(m_input_key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_destroy_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	status = psa_destroy_key(m_output_key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_destroy_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int import_input_password(void)
{
	psa_status_t status;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&attributes, PSA_ALG_PBKDF2_HMAC(PSA_ALG_SHA_256));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_PASSWORD);
	psa_set_key_bits(&attributes, PSA_BYTES_TO_BITS(PBKDF2_SAMPLE_INPUT_KEY_SIZE));

	status = psa_import_key(&attributes, m_input_password, PBKDF2_SAMPLE_INPUT_KEY_SIZE,
				&m_input_key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_import_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int derive_output_key(void)
{
	psa_status_t status;
	int cmp_status;
	psa_key_derivation_operation_t operation = PSA_KEY_DERIVATION_OPERATION_INIT;

	status = psa_key_derivation_setup(&operation, PSA_ALG_PBKDF2_HMAC(PSA_ALG_SHA_256));
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_setup failed! (Error: %d)", status);
		return APP_ERROR;
	}

	status = psa_key_derivation_input_integer(&operation, PSA_KEY_DERIVATION_INPUT_COST,
						  m_iteration_count);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_input_integer failed! (Error: %d)", status);
		return APP_ERROR;
	}

	status = psa_key_derivation_input_bytes(&operation, PSA_KEY_DERIVATION_INPUT_SALT, m_salt,
						PBKDF2_SAMPLE_SALT_SIZE);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_input_bytes failed! (Error: %d)", status);
		return APP_ERROR;
	}

	status = psa_key_derivation_input_key(&operation, PSA_KEY_DERIVATION_INPUT_PASSWORD,
					      m_input_key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_input_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* This outputs the derived key as bytes to the application.
	 * If the derived key is to be used for in cryptographic operations use
	 * psa_key_derivation_output_key instead.
	 */
	status = psa_key_derivation_output_bytes(&operation, m_output_key,
						 PBKDF2_SAMPLE_OUTPUT_KEY_SIZE);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_output_bytes failed! (Error: %d)", status);
		return APP_ERROR;
	}

	status = psa_key_derivation_abort(&operation);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_key_derivation_abort failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("Compare derived key with expected value...");
	cmp_status = memcmp(m_expected_output_key, m_output_key, sizeof(m_output_key));
	if (cmp_status != 0) {
		LOG_INF("Error, the derived key doesn't match the expected value!");
		return APP_ERROR;
	}

	LOG_INF("Key derivation successful!");
	PRINT_HEX("Password", m_input_password, sizeof(m_input_password));
	PRINT_HEX("Salt", m_salt, sizeof(m_salt));
	LOG_INF("Iteration count: %d", m_iteration_count);
	PRINT_HEX("Derived Key:", m_output_key, sizeof(m_output_key));

	return APP_SUCCESS;
}


int main(void)
{
	int status;

	LOG_INF("Starting PBKDF2 example...");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = import_input_password();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = derive_output_key();
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
