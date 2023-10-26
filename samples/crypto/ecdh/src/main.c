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

LOG_MODULE_REGISTER(ecdh, LOG_LEVEL_DBG);

/* ====================================================================== */
/*				Global variables/defines for the ECDH example			  */

#define NRF_CRYPTO_EXAMPLE_ECDH_KEY_BITS (256)
#define NRF_CRYPTO_EXAMPLE_ECDH_PUBLIC_KEY_SIZE (65)

/* Buffers to hold Bob's and Alice's public keys */
static uint8_t m_pub_key_bob[NRF_CRYPTO_EXAMPLE_ECDH_PUBLIC_KEY_SIZE];
static uint8_t m_pub_key_alice[NRF_CRYPTO_EXAMPLE_ECDH_PUBLIC_KEY_SIZE];

/* Buffers to hold Bob's and Alice's secret values */
static uint8_t m_secret_alice[32];
static uint8_t m_secret_bob[32];

psa_key_id_t key_id_alice;
psa_key_id_t key_id_bob;

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
	status = psa_destroy_key(key_id_alice);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_destroy_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* Destroy the key handle */
	status = psa_destroy_key(key_id_bob);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_destroy_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int create_ecdh_keypair(psa_key_id_t *key_id)
{
	psa_status_t status;
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	/* Crypto settings for ECDH using the SHA256 hashing algorithm,
	 * the secp256r1 curve
	 */
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDH);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attributes, 256);

	/* Generate a key pair */
	status = psa_generate_key(&key_attributes, key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_generate_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	psa_reset_key_attributes(&key_attributes);

	LOG_INF("ECDH keypair created successfully!");

	return APP_SUCCESS;
}

int export_ecdh_public_key(psa_key_id_t *key_id, uint8_t *buff, size_t buff_size)
{
	size_t olen;
	psa_status_t status;

	/* Export the public key */
	status = psa_export_public_key(*key_id, buff, buff_size, &olen);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_export_public_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("ECDH public key exported successfully!");

	return APP_SUCCESS;
}

int calculate_ecdh_secret(psa_key_id_t *key_id,
			  uint8_t *pub_key,
			  size_t pub_key_len,
			  uint8_t *secret,
			  size_t secret_len)
{
	uint32_t output_len;
	psa_status_t status;

	/* Perform the ECDH key exchange to calculate the secret */
	status = psa_raw_key_agreement(
		PSA_ALG_ECDH, *key_id, pub_key, pub_key_len, secret, secret_len, &output_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_raw_key_agreement failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("ECDH secret calculated successfully!");

	return APP_SUCCESS;
}

int compare_secrets(void)
{
	int status;

	LOG_INF("Comparing the secret values of Alice and Bob");

	status = memcmp(m_secret_bob, m_secret_alice, sizeof(m_secret_alice));
	if (status != 0) {
		LOG_INF("Error: Secret values don't match!");
		return APP_ERROR;
	}

	LOG_INF("The secret values of Alice and Bob match!");

	return APP_SUCCESS;
}

int main(void)
{
	int status;

	LOG_INF("Starting ECDH example...");
	/* Init crypto */
	status = crypto_init();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	/* Create the ECDH key pairs for Alice and Bob  */
	LOG_INF("Creating ECDH key pair for Alice");
	status = create_ecdh_keypair(&key_id_alice);
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	LOG_INF("Creating ECDH key pair for Bob");
	status = create_ecdh_keypair(&key_id_bob);
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	/* Export the ECDH public keys */
	LOG_INF("Export Alice's public key");
	status =
		export_ecdh_public_key(&key_id_alice, m_pub_key_alice, sizeof(m_pub_key_alice));
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	LOG_INF("Export Bob's public key");
	status = export_ecdh_public_key(&key_id_bob, m_pub_key_bob, sizeof(m_pub_key_bob));
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	/* Calculate the secret value for each participant */
	LOG_INF("Calculating the secret value for Alice");
	status = calculate_ecdh_secret(&key_id_alice,
				       m_pub_key_bob,
				       sizeof(m_pub_key_bob),
				       m_secret_alice,
				       sizeof(m_secret_alice));
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	LOG_INF("Calculating the secret value for Bob");
	status = calculate_ecdh_secret(&key_id_bob,
				       m_pub_key_alice,
				       sizeof(m_pub_key_alice),
				       m_secret_bob,
				       sizeof(m_secret_bob));
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	/* Verify that the calculated secrets match */
	status = compare_secrets();
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
