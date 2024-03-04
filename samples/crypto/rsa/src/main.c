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

#include "key.h"

#ifdef CONFIG_BUILD_WITH_TFM
#include <tfm_ns_interface.h>
#endif

#define APP_SUCCESS	    (0)
#define APP_ERROR	    (-1)
#define APP_SUCCESS_MESSAGE "Example finished successfully!"
#define APP_ERROR_MESSAGE   "Example exited with error!"

#define PRINT_HEX(p_label, p_text, len)                                                            \
	({                                                                                         \
		LOG_INF("---- %s (len: %u): ----", p_label, len);                                  \
		LOG_HEXDUMP_INF(p_text, len, "Content:");                                          \
		LOG_INF("---- %s end  ----", p_label);                                             \
	})

LOG_MODULE_REGISTER(rsa, LOG_LEVEL_DBG);

/* ====================================================================== */
/*				Global variables/defines for the RSA example			  */

#ifndef CONFIG_PSA_WANT_RSA_KEY_SIZE_4096
#error "This sample needs a key size of 4096"
#endif

#define NRF_CRYPTO_EXAMPLE_RSA_TEXT_SIZE       (100)
#define NRF_CRYPTO_EXAMPLE_RSA_PUBLIC_KEY_SIZE (PSA_KEY_EXPORT_RSA_PUBLIC_KEY_MAX_SIZE(4096))
#define NRF_CRYPTO_EXAMPLE_RSA_SIGNATURE_SIZE  (PSA_BITS_TO_BYTES(4096))

/* Below text is used as plaintext for signing using RSA . */
static char m_plain_text[NRF_CRYPTO_EXAMPLE_RSA_TEXT_SIZE] = {
	"Example string to demonstrate basic usage of RSA."};

static char m_signature[NRF_CRYPTO_EXAMPLE_RSA_SIGNATURE_SIZE];
static char m_hash[32];

static psa_key_id_t keypair_handle;
static psa_key_id_t pub_key_handle;
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

	/* Destroy the key handle */
	status = psa_destroy_key(keypair_handle);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_destroy_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	status = psa_destroy_key(pub_key_handle);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_destroy_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int import_rsa_keypair(void)
{
	psa_status_t status;

	LOG_INF("Importing RSA keypair...");

	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	/* Configure the key attributes */
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_SIGN_HASH);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_RSA_KEY_PAIR);
	psa_set_key_bits(&key_attributes, 4096);

	/* Generate a random keypair. The keypair is not exposed to the application,
	 * we can use it to signing/verification the key handle.
	 */
	status = psa_import_key(&key_attributes, private_key_der, sizeof(private_key_der),
				&keypair_handle);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_import_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* After the key handle is acquired the attributes are not needed */
	psa_reset_key_attributes(&key_attributes);

	LOG_INF("RSA private key imported successfully!");

	return APP_SUCCESS;
}

int import_rsa_pub_key(void)
{
	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	/* Configure the key attributes */
	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256));
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_RSA_PUBLIC_KEY);
	psa_set_key_bits(&key_attributes, 4096);

	status = psa_import_key(&key_attributes, public_key_der, sizeof(public_key_der),
				&pub_key_handle);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_import_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* After the key handle is acquired the attributes are not needed */
	psa_reset_key_attributes(&key_attributes);

	LOG_INF("RSA public key imported successfully!");

	return APP_SUCCESS;
}

int sign_message_rsa(void)
{
	uint32_t olen;
	psa_status_t status;

	LOG_INF("Signing a message using RSA...");

	/* Compute the SHA256 hash */
	status = psa_hash_compute(PSA_ALG_SHA_256, m_plain_text, sizeof(m_plain_text), m_hash,
				  sizeof(m_hash), &olen);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_hash_compute failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* Sign the hash using RSA */
	status = psa_sign_hash(keypair_handle, PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256), m_hash,
			       sizeof(m_hash), m_signature, sizeof(m_signature), &olen);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_sign_hash failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("Signing was successful!");
	PRINT_HEX("Plaintext", m_plain_text, sizeof(m_plain_text));
	PRINT_HEX("SHA256 hash", m_hash, sizeof(m_hash));
	PRINT_HEX("Signature", m_signature, sizeof(m_signature));

	return APP_SUCCESS;
}

int verify_message_rsa(void)
{
	psa_status_t status;

	LOG_INF("Verifying RSA signature...");

	/* Verify the hash */
	status = psa_verify_hash(pub_key_handle, PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256), m_hash,
				 sizeof(m_hash), m_signature, sizeof(m_signature));
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_verify_hash failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("Signature verification was successful!");

	return APP_SUCCESS;
}

int main(void)
{
	int status;

	LOG_INF("Starting the RSA example...");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = import_rsa_keypair();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = import_rsa_pub_key();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = sign_message_rsa();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = verify_message_rsa();
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
