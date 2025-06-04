/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/*
#include <stdio.h>
#include <stdlib.h> */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>

#define APP_SUCCESS 0
#define APP_ERROR   -1
#define APP_SUCCESS_MESSAGE "Example finished successfully!"
#define APP_ERROR_MESSAGE   "Example exited with error!"

#define PRINT_HEX(p_label, p_text, len) \
	({ \
		LOG_INF("---- %s (len: %u): ----", p_label, len); \
		LOG_HEXDUMP_INF(p_text, len, "Content:"); \
		LOG_INF("---- %s end ----", p_label); \
	})

LOG_MODULE_REGISTER(cmac, LOG_LEVEL_DBG);

/* ========================================================================= */

#define NRF_CRYPTO_EXAMPLE_CMAC_TEXT_SIZE (100)
#define NRF_CRYPTO_EXAMPLE_CMAC_KEY_BITS  (128) // AES-128
#define NRF_CRYPTO_EXAMPLE_CMAC_KEY_SIZE  (NRF_CRYPTO_EXAMPLE_CMAC_KEY_BITS / 8)

static uint8_t m_plain_text[NRF_CRYPTO_EXAMPLE_CMAC_TEXT_SIZE] = {
	"Example string to demonstrate basic usage of CMAC signing/verification."
};

static uint8_t cmac[PSA_MAC_LENGTH(PSA_KEY_TYPE_AES, NRF_CRYPTO_EXAMPLE_CMAC_KEY_BITS, PSA_ALG_CMAC)];

static psa_key_id_t key_id;

/* ========================================================================= */

int crypto_init(void)
{
	psa_status_t status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_crypto_init failed! (Error: %d)", status);
		return APP_ERROR;
	}
	return APP_SUCCESS;
}

int crypto_finish(void)
{
	psa_status_t status = psa_destroy_key(key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_destroy_key failed! (Error: %d)", status);
		return APP_ERROR;
	}
	return APP_SUCCESS;
}

int generate_key(void)
{
	psa_status_t status;
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	LOG_INF("Generating random CMAC key...");

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_CMAC);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attributes, NRF_CRYPTO_EXAMPLE_CMAC_KEY_BITS);

	status = psa_generate_key(&key_attributes, &key_id);
	psa_reset_key_attributes(&key_attributes);

	if (status != PSA_SUCCESS) {
		LOG_INF("psa_generate_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("CMAC key generated successfully!");
	return APP_SUCCESS;
}

int cmac_sign(void)
{
	uint32_t olen;
	psa_status_t status;
	psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;

	LOG_INF("Signing using CMAC...");

	status = psa_mac_sign_setup(&operation, key_id, PSA_ALG_CMAC);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_mac_sign_setup failed! (Error: %d)", status);
		return APP_ERROR;
	}

	status = psa_mac_update(&operation, m_plain_text, sizeof(m_plain_text));
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_mac_update failed! (Error: %d)", status);
		return APP_ERROR;
	}

	status = psa_mac_sign_finish(&operation, cmac, sizeof(cmac), &olen);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_mac_sign_finish failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("Signing successful!");
	PRINT_HEX("Plaintext", m_plain_text, sizeof(m_plain_text));
	PRINT_HEX("CMAC", cmac, olen);

	return APP_SUCCESS;
}

int cmac_verify(void)
{
	psa_status_t status;
	psa_mac_operation_t operation = PSA_MAC_OPERATION_INIT;

	LOG_INF("Verifying the CMAC...");

	status = psa_mac_verify_setup(&operation, key_id, PSA_ALG_CMAC);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_mac_verify_setup failed! (Error: %d)", status);
		return APP_ERROR;
	}

	status = psa_mac_update(&operation, m_plain_text, sizeof(m_plain_text));
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_mac_update failed! (Error: %d)", status);
		return APP_ERROR;
	}

	status = psa_mac_verify_finish(&operation, cmac,
		PSA_MAC_LENGTH(PSA_KEY_TYPE_AES, NRF_CRYPTO_EXAMPLE_CMAC_KEY_BITS, PSA_ALG_CMAC));
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_mac_verify_finish failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("CMAC verified successfully!");
	return APP_SUCCESS;
}

int main(void)
{
	int status;

	LOG_INF("Starting CMAC example...");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = generate_key();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = cmac_sign();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = cmac_verify();
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
