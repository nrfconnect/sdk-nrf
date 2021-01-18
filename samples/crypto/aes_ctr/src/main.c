/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <logging/log.h>
#include <stdio.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <crypto_common.h>

LOG_MODULE_REGISTER(aes_ctr, LOG_LEVEL_DBG);

/* ====================================================================== */
/*			Global variables/defines for the AES CTR example			  */

#define NRF_CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE (100)
#define NRF_CRYPTO_EXAMPLE_AES_BLOCK_SIZE (16)

/* AES IV buffer */
static uint8_t m_iv[NRF_CRYPTO_EXAMPLE_AES_BLOCK_SIZE];

/* Below text is used as plaintext for encryption/decryption */
static uint8_t m_plain_text[NRF_CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE] = {
	"Example string to demonstrate basic usage of AES CTR mode."
};

static uint8_t m_encrypted_text[NRF_CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE];
static uint8_t m_decrypted_text[NRF_CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE];

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

	PRINT_MESSAGE("Generating random AES key...");

	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes,
				PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_CTR);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attributes, 128);

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

	PRINT_MESSAGE("AES key generated successfully!");

	return APP_SUCCESS;
}

int encrypt_ctr_aes(void)
{
	uint32_t olen;
	psa_status_t status;
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;

	PRINT_MESSAGE("Encrypting using AES CTR MODE...");

	/* Setup the encryption operation */
	status = psa_cipher_encrypt_setup(&operation, key_handle, PSA_ALG_CTR);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_cipher_encrypt_setup", status);
		return APP_ERROR;
	}

	/* Generate a random IV */
	status = psa_cipher_generate_iv(&operation, m_iv, sizeof(m_iv),
					&olen);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_cipher_generate_iv", status);
		return APP_ERROR;
	}

	/* Perform the encryption */
	status = psa_cipher_update(&operation,
							   m_plain_text,
							   sizeof(m_plain_text),
							   m_encrypted_text,
							   sizeof(m_encrypted_text),
							   &olen);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_cipher_update", status);
		return APP_ERROR;
	}

	/* Finalize encryption */
	status = psa_cipher_finish(&operation,
							   m_encrypted_text + olen,
							   sizeof(m_encrypted_text) - olen,
							   &olen);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_cipher_finish", status);
		return APP_ERROR;
	}

	/* Clean up cipher operation context */
	status = psa_cipher_abort(&operation);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_cipher_abort", status);
		return APP_ERROR;
	}

	PRINT_MESSAGE("Encryption successful!\r\n");
	PRINT_HEX("IV", m_iv, sizeof(m_iv));
	PRINT_HEX("Plaintext", m_plain_text, sizeof(m_plain_text));
	PRINT_HEX("Encrypted text", m_encrypted_text, sizeof(m_encrypted_text));

	return APP_SUCCESS;
}

int decrypt_ctr_aes(void)
{
	uint32_t olen;
	psa_status_t status;
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;

	PRINT_MESSAGE("Decrypting using AES CTR MODE...");

	/* Setup the decryption operation */
	status = psa_cipher_decrypt_setup(&operation, key_handle, PSA_ALG_CTR);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_cipher_decrypt_setup", status);
		return APP_ERROR;
	}

	/* Set the IV to the one generated during encryption */
	status = psa_cipher_set_iv(&operation, m_iv, sizeof(m_iv));
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_cipher_set_iv", status);
		return APP_ERROR;
	}

	/* Perform the decryption */
	status = psa_cipher_update(&operation,
							   m_encrypted_text,
							   sizeof(m_encrypted_text),
							   m_decrypted_text,
							   sizeof(m_decrypted_text), &olen);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_cipher_update", status);
		return APP_ERROR;
	}

	/* Finalize the decryption */
	status = psa_cipher_finish(&operation,
							   m_decrypted_text + olen,
							   sizeof(m_decrypted_text) - olen,
							   &olen);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_cipher_finish", status);
		return APP_ERROR;
	}

	PRINT_HEX("Decrypted text", m_decrypted_text, sizeof(m_decrypted_text));

	/* Check the validity of the decryption */
	if (memcmp(m_decrypted_text, m_plain_text, NRF_CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE) != 0) {
		PRINT_MESSAGE("Error: Decrypted text doesn't match the plaintext");
		return APP_ERROR;
	}

	/* Clean up cipher operation context */
	status = psa_cipher_abort(&operation);
	if (status != PSA_SUCCESS) {
		PRINT_ERROR("psa_cipher_abort", status);
		return APP_ERROR;
	}

	PRINT_MESSAGE("Decryption successful!");

	return APP_SUCCESS;
}

int main(void)
{
	int status;

	PRINT_MESSAGE("Starting AES CTR example...");

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

	status = encrypt_ctr_aes();
	if (status != APP_SUCCESS) {
		PRINT_MESSAGE(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = decrypt_ctr_aes();
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
