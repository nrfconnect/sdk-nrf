/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <logging/log.h>
#include <stdio.h>

#include <psa/crypto.h>
#include <psa/crypto_extra.h>

#include "tfm_api.h"
#include "tfm_ns_interface.h"

LOG_MODULE_REGISTER(aes_ctr, LOG_LEVEL_DBG);

K_MUTEX_DEFINE(tfm_mutex);

#define PSA_ERROR_CHECK(func, status)\
	({\
		if (status != PSA_SUCCESS) {\
			LOG_INF("Function %s failed with error code: %d\r\n",\
				func, status);\
			return -1;\
		} \
	})

int32_t tfm_ns_interface_dispatch(veneer_fn fn, uint32_t arg0, uint32_t arg1,
				  uint32_t arg2, uint32_t arg3)
{
	int32_t result;

	/* TFM request protected by NS lock */
	if (k_mutex_lock(&tfm_mutex, K_FOREVER) != 0) {
		return (int32_t)TFM_ERROR_GENERIC;
	}

	result = fn(arg0, arg1, arg2, arg3);

	k_mutex_unlock(&tfm_mutex);

	return result;
}

enum tfm_status_e tfm_ns_interface_init(void)
{
	/* The static K_MUTEX_DEFINE handles mutex init, so just return. */

	return TFM_SUCCESS;
}

void print_message(char const *p_text)
{
	LOG_INF("%s", p_text);
}

void print_hex(char const *p_label, char const *p_text, size_t len)
{
	LOG_INF("---- %s (len: %u): ----", p_label, len);
	LOG_HEXDUMP_INF(p_text, len, "Content:");
	LOG_INF("---- %s end  ----", p_label);
}

/* ====================================================================== */
/* 			  Global variables/defines for the AES CTR example 			  */

#define NRF_CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE (100)
#define NRF_CRYPTO_EXAMPLE_AES_BLOCK_SIZE (16)

/* AES sample key, DO NOT USE IN PRODUCTION */
static uint8_t m_key[NRF_CRYPTO_EXAMPLE_AES_BLOCK_SIZE] = {
	'A', 'E', 'S', ' ', 'C', 'T', 'R', ' ',
	'K', 'E', 'Y', ' ', 'T', 'E', 'S', 'T'
};
static uint8_t m_iv[NRF_CRYPTO_EXAMPLE_AES_BLOCK_SIZE];

/* Below text is used as plaintext for encryption/decryption */
static uint8_t m_plain_text[NRF_CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE] = {
	"Example string to demonstrate basic usage of AES CTR mode."
};

static uint8_t m_encrypted_text[NRF_CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE];
static uint8_t m_decrypted_text[NRF_CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE];

/* ====================================================================== */

int decrypt_ctr_aes()
{
	uint32_t output_len;
	psa_status_t status;

	/* Crypto settings for CTR mode decryption using an AES-128 bit key */
	psa_algorithm_t alg = PSA_ALG_CTR;
	psa_key_type_t key_type = PSA_KEY_TYPE_AES;
	psa_key_usage_t key_usage = PSA_KEY_USAGE_DECRYPT;
	psa_key_lifetime_t key_lifetime = PSA_KEY_LIFETIME_VOLATILE;
	psa_key_handle_t key_handle;
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;

	print_message("Decrypting using AES CTR MODE...");

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	PSA_ERROR_CHECK("psa_crypto_init", status);

	/* Configure the key attributes */
	psa_set_key_usage_flags(&key_attributes, key_usage);
	psa_set_key_lifetime(&key_attributes, key_lifetime);
	psa_set_key_algorithm(&key_attributes, alg);
	psa_set_key_type(&key_attributes, key_type);
	psa_set_key_bits(&key_attributes, 128);

	/* Import the key to the keystore */
	status = psa_import_key(&key_attributes, m_key, sizeof(m_key),
				&key_handle);
	PSA_ERROR_CHECK("psa_import_key", status);

	/* After the key handle is acquired the attributes are not needed */
	psa_reset_key_attributes(&key_attributes);

	/* Setup the decryption operation */
	status = psa_cipher_decrypt_setup(&operation, key_handle, alg);
	PSA_ERROR_CHECK("psa_cipher_decrypt_setup", status);

	/* Set the IV to the one generated during encryption */
	status = psa_cipher_set_iv(&operation, m_iv, sizeof(m_iv));
	PSA_ERROR_CHECK("psa_cipher_set_iv", status);

	/* Perform the decryption */
	status = psa_cipher_update(&operation, m_encrypted_text,
				   sizeof(m_encrypted_text), m_decrypted_text,
				   sizeof(m_decrypted_text), &output_len);
	PSA_ERROR_CHECK("psa_cipher_update", status);

	/* Finalize the decryption */
	status = psa_cipher_finish(&operation, m_decrypted_text + output_len,
				   sizeof(m_decrypted_text) - output_len,
				   &output_len);
	PSA_ERROR_CHECK("psa_cipher_finish", status);

	print_message("Decryption succesfull!");
	print_hex("Decrypted text", m_decrypted_text, sizeof(m_decrypted_text));

	/* Clean up cipher operation context */
	status = psa_cipher_abort(&operation);
	PSA_ERROR_CHECK("psa_cipher_abort", status);

	/* Destroy the key */
	status = psa_destroy_key(key_handle);
	PSA_ERROR_CHECK("psa_destroy_key", status);

	return 1;
}

int encrypt_ctr_aes()
{
	uint32_t output_len;
	psa_status_t status;

	/* Crypto settings for CTR mode using an AES-128 bit key */
	psa_algorithm_t alg = PSA_ALG_CTR;
	psa_key_type_t key_type = PSA_KEY_TYPE_AES;
	psa_key_usage_t key_usage = PSA_KEY_USAGE_ENCRYPT;
	psa_key_lifetime_t key_lifetime = PSA_KEY_LIFETIME_VOLATILE;
	psa_key_handle_t key_handle;
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;

	print_message("Encrypting using AES CTR MODE...");

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	PSA_ERROR_CHECK("psa_crypto_init", status);

	/* Configure the key attributes */
	psa_set_key_usage_flags(&key_attributes, key_usage);
	psa_set_key_lifetime(&key_attributes, key_lifetime);
	psa_set_key_algorithm(&key_attributes, alg);
	psa_set_key_type(&key_attributes, key_type);
	psa_set_key_bits(&key_attributes, 128);

	/* Import the key to the keystore */
	status = psa_import_key(&key_attributes, m_key, sizeof(m_key),
				&key_handle);
	PSA_ERROR_CHECK("psa_import_key", status);

	/* After the key handle is acquired the attributes are not needed */
	psa_reset_key_attributes(&key_attributes);

	/* Setup the encryption operation */
	status = psa_cipher_encrypt_setup(&operation, key_handle, alg);
	PSA_ERROR_CHECK("psa_cipher_encrypt_setup", status);

	/* Generate a random IV */
	status = psa_cipher_generate_iv(&operation, m_iv, sizeof(m_iv),
					&output_len);
	PSA_ERROR_CHECK("psa_cipher_generate_iv", status);

	/* Perform the encryption */
	status = psa_cipher_update(&operation, m_plain_text,
				   sizeof(m_plain_text), m_encrypted_text,
				   sizeof(m_encrypted_text), &output_len);
	PSA_ERROR_CHECK("psa_cipher_update", status);

	/* Finalize encryption */
	status = psa_cipher_finish(&operation, m_encrypted_text + output_len,
				   sizeof(m_encrypted_text) - output_len,
				   &output_len);
	PSA_ERROR_CHECK("psa_cipher_finish", status);

	print_message("Encryption succesfull!\r\n");
	print_hex("Key", m_key, sizeof(m_key));
	print_hex("IV", m_iv, sizeof(m_iv));
	print_hex("Plaintext", m_plain_text, sizeof(m_plain_text));
	print_hex("Encrypted text", m_encrypted_text, sizeof(m_encrypted_text));

	/* Clean up cipher operation context */
	status = psa_cipher_abort(&operation);
	PSA_ERROR_CHECK("psa_cipher_abort", status);

	/* Destroy the key */
	status = psa_destroy_key(key_handle);
	PSA_ERROR_CHECK("psa_destroy_key", status);

	return 1;
}

void main(void)
{
	print_message("Starting AES CTR example...");

	if (encrypt_ctr_aes() > 0) {
		if (decrypt_ctr_aes() > 0) {
			print_message("Example finished succesfully!");
			return;
		}
	}

	print_message("Example exited with error!");
}

