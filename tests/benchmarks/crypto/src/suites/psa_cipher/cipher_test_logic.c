/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include "cipher_test_logic.h"

#define BLOCK_SIZE 16
#define SINGLE_ENCRYPTED_SIZE (TEXT_SIZE + 2 * BLOCK_SIZE)
/* The no-padding CBC and ECB tests take whole blocks only; catch a bad shared
 * TEXT_SIZE at build time rather than on the board.
 *
 * It also keeps update_in_halves()' halves at a whole number of blocks, which
 * matters for CBC-PKCS7 decryption: Cracen miscounts when an update exactly
 * completes the block it had buffered, asking for a block count of (size_t)-16
 * and failing with PSA_ERROR_NOT_PERMITTED. A half is never that small.
 */
BUILD_ASSERT(TEXT_SIZE % BLOCK_SIZE == 0,
	     "TEXT_SIZE must be a multiple of the AES block size");

static const uint8_t text[TEXT_SIZE] = "Crypto benchmarks cipher test data.";
static psa_key_id_t key_id;
static bool key_created;
static uint8_t single_encrypted[SINGLE_ENCRYPTED_SIZE];
static uint8_t single_decrypted[SINGLE_ENCRYPTED_SIZE];
static size_t single_encrypted_length;
static size_t single_decrypted_length;
static uint8_t iv[BLOCK_SIZE];
static uint8_t multipart_encrypted[SINGLE_ENCRYPTED_SIZE];
static uint8_t multipart_decrypted[SINGLE_ENCRYPTED_SIZE];
static size_t multipart_encrypted_length;
static size_t multipart_decrypted_length;
/* For cipher_check(), which the runner calls without a context. The operations
 * read their own context instead.
 */
static const struct cipher_test_data *active_test;

static size_t cipher_iv_size(const struct cipher_test_data *test)
{
	return test->algorithm == PSA_ALG_STREAM_CIPHER ? 12 : BLOCK_SIZE;
}

/* Two updates of half the input each. Halves come from the length passed in,
 * not TEXT_HALF_SIZE: decryption consumes a ciphertext, not the message.
 */
static psa_status_t update_in_halves(psa_cipher_operation_t *operation, const uint8_t *input,
				     size_t input_length, uint8_t *output, size_t output_size,
				     size_t *output_length)
{
	const size_t halves[] = {input_length / 2, input_length - input_length / 2};
	size_t offset = 0;
	size_t total = 0;

	for (size_t i = 0; i < ARRAY_SIZE(halves); i++) {
		size_t written;
		psa_status_t status = psa_cipher_update(operation, &input[offset], halves[i],
						       &output[total], output_size - total,
						       &written);

		if (status != PSA_SUCCESS) {
			return status;
		}

		offset += halves[i];
		total += written;
	}

	*output_length = total;

	return PSA_SUCCESS;
}

static psa_status_t generate_key(const void *context)
{
	const struct cipher_test_data *test = context;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;
	psa_key_lifetime_t lifetime = test->persistent_id == 0 ? PSA_KEY_LIFETIME_VOLATILE
							       : PSA_KEY_LIFETIME_PERSISTENT;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&attributes, lifetime);
	psa_set_key_algorithm(&attributes, test->algorithm);
	psa_set_key_type(&attributes, test->algorithm == PSA_ALG_STREAM_CIPHER
		? PSA_KEY_TYPE_CHACHA20 : PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, test->key_bits);
	if (lifetime == PSA_KEY_LIFETIME_PERSISTENT) {
		psa_set_key_id(&attributes, test->persistent_id);
	}
	status = psa_generate_key(&attributes, &key_id);
	psa_reset_key_attributes(&attributes);
	key_created = status == PSA_SUCCESS;
	active_test = test;
	return status;
}

static psa_status_t single_encrypt(const void *context)
{
	const struct cipher_test_data *test = context;

	return psa_cipher_encrypt(key_id, test->algorithm, text, sizeof(text),
				  single_encrypted, sizeof(single_encrypted),
				  &single_encrypted_length);
}

static psa_status_t single_decrypt(const void *context)
{
	const struct cipher_test_data *test = context;

	return psa_cipher_decrypt(key_id, test->algorithm, single_encrypted,
				  single_encrypted_length, single_decrypted,
				  sizeof(single_decrypted), &single_decrypted_length);
}

static psa_status_t multipart_encrypt(const void *context)
{
	const struct cipher_test_data *test = context;
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;
	psa_status_t status;
	size_t output_length;
	size_t total = 0;

	multipart_encrypted_length = 0;

	status = psa_cipher_encrypt_setup(&operation, key_id, test->algorithm);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_cipher_generate_iv(&operation, iv, cipher_iv_size(test), &output_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = update_in_halves(&operation, text, sizeof(text), multipart_encrypted,
				  sizeof(multipart_encrypted), &total);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_cipher_finish(&operation, &multipart_encrypted[total],
				   sizeof(multipart_encrypted) - total, &output_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	multipart_encrypted_length = total + output_length;

exit:
	psa_cipher_abort(&operation);

	return status;
}

static psa_status_t multipart_decrypt(const void *context)
{
	const struct cipher_test_data *test = context;
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;
	psa_status_t status;
	size_t output_length;
	size_t total = 0;

	multipart_decrypted_length = 0;

	status = psa_cipher_decrypt_setup(&operation, key_id, test->algorithm);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_cipher_set_iv(&operation, iv, cipher_iv_size(test));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = update_in_halves(&operation, multipart_encrypted, multipart_encrypted_length,
				  multipart_decrypted, sizeof(multipart_decrypted), &total);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_cipher_finish(&operation, &multipart_decrypted[total],
				   sizeof(multipart_decrypted) - total, &output_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	multipart_decrypted_length = total + output_length;

exit:
	psa_cipher_abort(&operation);

	return status;
}

int cipher_check(void)
{
	if (active_test == NULL) {
		return APP_ERROR;
	}

	if (active_test->has_singlepart && (single_decrypted_length != sizeof(text) ||
					    memcmp(single_decrypted, text, sizeof(text)) != 0)) {
		return APP_ERROR;
	}

	if (active_test->has_multipart && (multipart_decrypted_length != sizeof(text) ||
					   memcmp(multipart_decrypted, text, sizeof(text)) != 0)) {
		return APP_ERROR;
	}

	/* CBC picks a fresh IV per operation, so the two ciphertexts must differ. */
	if (active_test->algorithm == PSA_ALG_CBC_NO_PADDING &&
	    memcmp(multipart_encrypted, &single_encrypted[BLOCK_SIZE], sizeof(text)) == 0) {
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

void cipher_cleanup(void)
{
	if (!key_created) {
		return;
	}

	(void)psa_destroy_key(key_id);
	key_created = false;
}

const struct op cipher_keysetup_ops[] = {{"generate", generate_key, NULL}};
const struct op cipher_singlepart_ops[] = {
	{"encrypt", single_encrypt, NULL},
	{"decrypt", single_decrypt, NULL},
};
const struct op cipher_multipart_ops[] = {
	{"encrypt", multipart_encrypt, NULL},
	{"decrypt", multipart_decrypt, NULL},
};

