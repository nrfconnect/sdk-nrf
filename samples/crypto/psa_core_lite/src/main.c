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

/* Global variables/defines for the PSA Crypto Lite example */

/* Below text is used for hashing and signature validation (note null termination) */
const size_t aligned_buffer_size = 64;

static uint8_t m_plain_text[] = {
	"Example string to demonstrate PSA Crypto Lite."
};

const size_t m_plaintext_len = sizeof(m_plain_text)/sizeof(m_plain_text[0]);

const mbedtls_svc_key_id_t m_pub_key_id = {0};

const size_t m_signature_len = 64;

mbedtls_svc_key_id_t m_enc_key_id = {0};

#if defined(PSA_WANT_ALG_PURE_EDDSA)
#define VALIDATE_ALG PSA_ALG_PURE_EDDSA
#elif defined(PSA_WANT_ALG_ECDSA)
#define VALIDATE_ALG PSA_WANT_ALG_ECDSA
#else
#error "This sample expects signature validation of ECDSA or EdDSA. None set"
#endif

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

int generate_random(void)
{
	psa_status_t status;
	uint8_t rng_buffer[aligned_buffer_size];

	status = psa_generate_random(rng_buffer, aligned_buffer_size);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_generate_random failed! (Error: %d)", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int hash_message(void)
{
	psa_status_t status;
#if defined(CONFIG_PSA_CORE_LITE_USE_VERIFY_HASH)
	const psa_algorithm_t hash_alg = PSA_ALG_SHA_256;
#else
	const psa_algorithm_t hash_alg = PSA_ALG_SHA_512;
#endif

	const size_t hash_size = PSA_HASH_LENGTH(hash_alg);
	uint8_t hash[hash_size];
	size_t hash_length;

	LOG_INF("Calculating hash...");

	status = psa_hash_compute(hash_alg,
			          m_plain_text, m_plaintext_len,
				  hash, hash_size, &hash_length);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_hash_compute failed! (Error: %d)", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int verify_message(void)
{
	psa_status_t status;

	LOG_INF("Verifying EDDSA signature...");

	/* Dummy signature atm*/
	uint8_t m_signature[m_signature_len];

#if defined(PSA_CORE_LITE_USE_VERIFY_HASH)
	const size_t hash_size = PSA_HASH_LENGTH(PSA_ALG_SHA_256);
	uint8_t hash[hash_size];
	/* Verify the signature of a hash */
	status = psa_verify_hash(m_pub_key_id,
					VALIDATE_ALG,
				    hash,
				    hash_size,
				    m_signature,
				    m_signature_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_verify_message failed! (Error: %d)", status);
		return APP_ERROR;
	}
#else
	/* Verify the signature of the message */
	status = psa_verify_message(m_pub_key_id,
					VALIDATE_ALG,
				    m_plain_text,
				    m_plaintext_len,
				    m_signature,
				    m_signature_len);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_verify_message failed! (Error: %d)", status);
		return APP_ERROR;
	}
#endif

	LOG_INF("Signature verification was successful!");

	return APP_SUCCESS;
}

#if defined(PSA_WANT_KEY_TYPE_AES) && defined(PSA_WANT_ALG_CTR)

int encrypt_message()
{
	LOG_INF("Encrypting message...");

	int status;
	psa_cipher_operation_t oper;

	/* Dummy values for IV */
	const size_t iv_length = 12;
	const uint8_t iv[12] = {0};

	uint8_t ciphertext_buffer[aligned_buffer_size];
	uint8_t plaintext_buffer[aligned_buffer_size];
	uint8_t * output_ptr = &ciphertext_buffer[0];
	size_t output_size;
	size_t full_output_len;

	/* Encrypt the plaintext */
	status = psa_cipher_encrypt_setup(&oper, m_enc_key_id, PSA_ALG_CTR);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_cipher_encrypt_setup failed! (Error: %d)", status);
		return APP_ERROR;
	}

	status = psa_cipher_set_iv(&oper, iv, iv_length);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_cipher_set_iv failed! (Error: %d)", status);
		return APP_ERROR;
	}

	status = psa_cipher_update(&oper, m_plain_text, m_plaintext_len,
				   output_ptr, aligned_buffer_size, &output_size);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_cipher_update failed! (Error: %d)", status);
		return APP_ERROR;
	}

	output_ptr += output_size;
	full_output_len = output_size;

	status = psa_cipher_finish(&oper, output_ptr,
				   aligned_buffer_size - output_size, &output_size);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_cipher_finish failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("Decrypting message...");

	full_output_len += output_size;

	/* Decrypt the created ciphertext */
	output_ptr = &plaintext_buffer[0];

	status = psa_cipher_decrypt_setup(&oper, m_enc_key_id, PSA_ALG_CTR);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_cipher_decrypt_setup failed! (Error: %d)", status);
		return APP_ERROR;
	}

	status = psa_cipher_set_iv(&oper, iv, iv_length);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_cipher_set_iv failed! (Error: %d)", status);
		return APP_ERROR;
	}

	status = psa_cipher_update(&oper, ciphertext_buffer, m_plaintext_len,
				   output_ptr, aligned_buffer_size, &output_size);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_cipher_update failed! (Error: %d)", status);
		return APP_ERROR;
	}

	output_ptr += output_size;
	full_output_len = output_size;

	status = psa_cipher_finish(&oper, output_ptr,
				   aligned_buffer_size - output_size, &output_size);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_cipher_finish failed! (Error: %d)", status);
		return APP_ERROR;
	}

	full_output_len += output_size;

	if (memcmp(m_plain_text, plaintext_buffer, m_plaintext_len) == 0) {
		LOG_ERR("Encryption and decryption was successful!");
		return APP_SUCCESS;
	}

	LOG_ERR("Plaintext did not match decrypted ciphertext");
	return APP_ERROR;
}

#endif /* PSA_WANT_KEY_TYPE_AES && PSA_WANT_ALG_CTR */

int main(void)
{
	int status;

	LOG_INF("Starting PSA Crypto Lite example...");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

#if defined(PSA_WANT_GENERATE_RANDOM)
	status = generate_random();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}
#endif

#if defined(PSA_WANT_ALG_SHA_512) || defined(PSA_WANT_ALG_SHA_256)
	status = hash_message();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}
#endif

#if defined(PSA_WANT_ALG_PURE_EDDSA)
	status = verify_message();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}
#endif

#if defined(PSA_WANT_KEY_TYPE_AES) && defined(PSA_WANT_ALG_CTR)
	status = encrypt_message();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}
#endif

	LOG_INF(APP_SUCCESS_MESSAGE);

	return APP_SUCCESS;
}
