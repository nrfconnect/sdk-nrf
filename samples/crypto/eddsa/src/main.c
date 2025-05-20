/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
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

LOG_MODULE_REGISTER(eddsa, LOG_LEVEL_DBG);

/* Global variables/defines for the EDDSA example */

#define NRF_CRYPTO_EXAMPLE_EDDSA_TEXT_SIZE (100)

#define NRF_CRYPTO_EXAMPLE_EDDSA_PUBLIC_KEY_SIZE (32)
#define NRF_CRYPTO_EXAMPLE_EDDSA_SIGNATURE_SIZE (64)

/* Below text is used as plaintext for signing/verification */
static uint8_t m_plain_text[NRF_CRYPTO_EXAMPLE_EDDSA_TEXT_SIZE] = {
	"Example string to demonstrate basic usage of EDDSA."
};

static uint8_t m_pub_key[NRF_CRYPTO_EXAMPLE_EDDSA_PUBLIC_KEY_SIZE];
static size_t m_pub_key_len;

static uint8_t m_signature[NRF_CRYPTO_EXAMPLE_EDDSA_SIGNATURE_SIZE];
static size_t m_signature_len;

static psa_key_id_t m_key_pair_id;
static psa_key_id_t m_pub_key_id;

int crypto_init(void)
{
	psa_status_t status;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_crypto_init failed! (Error: %d)", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int crypto_finish(void)
{
	psa_status_t status;

	/* Destroy the key handle */
	status = psa_destroy_key(m_key_pair_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_destroy_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	status = psa_destroy_key(m_pub_key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_destroy_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int generate_eddsa_keypair(void)
{
	psa_status_t status;

	LOG_INF("Generating random EDDSA keypair...");

	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	/* Configure the key attributes */
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_PURE_EDDSA);
	psa_set_key_type(&key_attributes,
			 PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS));
	psa_set_key_bits(&key_attributes, 255);

	/* Generate a random keypair. The keypair is not exposed to the application,
	 * we can use it to sign messages.
	 */
	status = psa_generate_key(&key_attributes, &m_key_pair_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_generate_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* Export the public key */
	status = psa_export_public_key(m_key_pair_id,
				       m_pub_key, sizeof(m_pub_key),
				       &m_pub_key_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_export_public_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	PRINT_HEX("Public-key", m_pub_key, m_pub_key_len);

	/* Reset key attributes and free any allocated resources. */
	psa_reset_key_attributes(&key_attributes);

	return APP_SUCCESS;
}

int import_eddsa_pub_key(void)
{
	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	/* Configure the key attributes */
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_MESSAGE);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_PURE_EDDSA);
	psa_set_key_type(&key_attributes,
			 PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS));
	psa_set_key_bits(&key_attributes, 255);

	status = psa_import_key(&key_attributes, m_pub_key, m_pub_key_len, &m_pub_key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_import_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* Reset key attributes and free any allocated resources. */
	psa_reset_key_attributes(&key_attributes);

	return APP_SUCCESS;
}

int sign_message(void)
{
	psa_status_t status;

	LOG_INF("Signing a message using EDDSA...");

	/* Sign the message */
	status = psa_sign_message(m_key_pair_id,
				  PSA_ALG_PURE_EDDSA,
				  m_plain_text,
				  sizeof(m_plain_text),
				  m_signature,
				  sizeof(m_signature),
				  &m_signature_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_sign_message failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("Message signed successfully!");
	PRINT_HEX("Plaintext", m_plain_text, sizeof(m_plain_text));
	PRINT_HEX("Signature", m_signature, sizeof(m_signature));

	return APP_SUCCESS;
}

int verify_message(void)
{
	psa_status_t status;

	LOG_INF("Verifying EDDSA signature...");

	/* Verify the signature of the message */
	status = psa_verify_message(m_pub_key_id,
				    PSA_ALG_PURE_EDDSA,
				    m_plain_text,
				    sizeof(m_plain_text),
				    m_signature,
				    m_signature_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_verify_message failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("Signature verification was successful!");

	return APP_SUCCESS;
}

int main(void)
{
	int status;

	LOG_INF("Starting EDDSA example...");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = generate_eddsa_keypair();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = import_eddsa_pub_key();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = sign_message();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = verify_message();
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
