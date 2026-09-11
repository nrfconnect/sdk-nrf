/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include "aead_test_logic.h"

#define NONCE_SIZE 13
#define TAG_SIZE 16
#define CIPHERTEXT_SIZE (TEXT_SIZE + TAG_SIZE)

static const uint8_t text[TEXT_SIZE] = "Crypto benchmarks AEAD test data.";
static const uint8_t additional_data[] = "associated data";
static uint8_t nonce[NONCE_SIZE];
static uint8_t ciphertext[CIPHERTEXT_SIZE];
static uint8_t plaintext[TEXT_SIZE];
static uint8_t multipart_ciphertext[TEXT_SIZE];
static uint8_t multipart_plaintext[TEXT_SIZE];
static uint8_t multipart_tag[TAG_SIZE];
static psa_key_id_t key_id;
static bool key_created;
static size_t ciphertext_length;
static size_t plaintext_length;
static size_t multipart_ciphertext_length;

static psa_status_t aead_setup(const struct aead_test_data *test,
			       psa_aead_operation_t *operation)
{
	psa_status_t status;

	status = psa_aead_encrypt_setup(operation, key_id, test->algorithm);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = psa_aead_set_lengths(operation, sizeof(additional_data), sizeof(text));
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = psa_aead_set_nonce(operation, nonce, test->nonce_size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	return psa_aead_update_ad(operation, additional_data, sizeof(additional_data));
}

static psa_status_t generate_key(const void *context)
{
	const struct aead_test_data *test = context;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attributes, test->algorithm);
	psa_set_key_type(&attributes, test->key_type);
	psa_set_key_bits(&attributes, test->key_bits);

	status = psa_generate_key(&attributes, &key_id);
	psa_reset_key_attributes(&attributes);

	key_created = status == PSA_SUCCESS;

	return status;
}

static psa_status_t encrypt(const void *context)
{
	const struct aead_test_data *test = context;
	psa_status_t status;

	status = psa_generate_random(nonce, test->nonce_size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	return psa_aead_encrypt(key_id, test->algorithm, nonce, test->nonce_size, additional_data,
				sizeof(additional_data), text, sizeof(text), ciphertext,
				sizeof(ciphertext), &ciphertext_length);
}

static psa_status_t decrypt(const void *context)
{
	const struct aead_test_data *test = context;

	return psa_aead_decrypt(key_id, test->algorithm, nonce, test->nonce_size, additional_data,
				sizeof(additional_data), ciphertext, ciphertext_length, plaintext,
				sizeof(plaintext), &plaintext_length);
}

static psa_status_t multipart_encrypt(const void *context)
{
	const struct aead_test_data *test = context;
	psa_aead_operation_t operation = PSA_AEAD_OPERATION_INIT;
	psa_status_t status;
	size_t output_length;
	size_t total_length = 0;
	size_t tag_length;

	multipart_ciphertext_length = 0;

	status = psa_generate_random(nonce, test->nonce_size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = aead_setup(test, &operation);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_aead_update(&operation, text, TEXT_HALF_SIZE, multipart_ciphertext,
				 sizeof(multipart_ciphertext), &output_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	total_length = output_length;

	status = psa_aead_update(&operation, &text[TEXT_HALF_SIZE], TEXT_HALF_SIZE,
				 &multipart_ciphertext[total_length],
				 sizeof(multipart_ciphertext) - total_length, &output_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	total_length += output_length;

	status = psa_aead_finish(&operation, &multipart_ciphertext[total_length],
				 sizeof(multipart_ciphertext) - total_length, &output_length,
				 multipart_tag, sizeof(multipart_tag), &tag_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	if (tag_length != sizeof(multipart_tag)) {
		status = PSA_ERROR_CORRUPTION_DETECTED;
		goto exit;
	}

	multipart_ciphertext_length = total_length + output_length;

exit:
	psa_aead_abort(&operation);

	return status;
}

static psa_status_t multipart_decrypt(const void *context)
{
	const struct aead_test_data *test = context;
	psa_aead_operation_t operation = PSA_AEAD_OPERATION_INIT;
	psa_status_t status;
	size_t output_length;
	size_t total_length = 0;

	status = psa_aead_decrypt_setup(&operation, key_id, test->algorithm);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_aead_set_lengths(&operation, sizeof(additional_data), sizeof(text));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_aead_set_nonce(&operation, nonce, test->nonce_size);
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	status = psa_aead_update_ad(&operation, additional_data, sizeof(additional_data));
	if (status != PSA_SUCCESS) {
		goto exit;
	}

	/* The ciphertext is the input, so the second update takes the remainder
	 * rather than another exact half.
	 */
	size_t first_half = multipart_ciphertext_length / 2;

	status = psa_aead_update(&operation, multipart_ciphertext, first_half,
				 multipart_plaintext, sizeof(multipart_plaintext), &output_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	total_length = output_length;

	status = psa_aead_update(&operation, &multipart_ciphertext[first_half],
				 multipart_ciphertext_length - first_half,
				 &multipart_plaintext[total_length],
				 sizeof(multipart_plaintext) - total_length, &output_length);
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	total_length += output_length;

	status = psa_aead_verify(&operation, &multipart_plaintext[total_length],
				 sizeof(multipart_plaintext) - total_length, &output_length,
				 multipart_tag, sizeof(multipart_tag));
	if (status != PSA_SUCCESS) {
		goto exit;
	}
	total_length += output_length;

	if (total_length != sizeof(text) || memcmp(multipart_plaintext, text, sizeof(text)) != 0) {
		status = PSA_ERROR_CORRUPTION_DETECTED;
	}

exit:
	psa_aead_abort(&operation);

	return status;
}

int aead_check(void)
{
	if (plaintext_length != sizeof(text) || memcmp(plaintext, text, sizeof(text)) != 0) {
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

void aead_cleanup(void)
{
	if (!key_created) {
		return;
	}

	(void)psa_destroy_key(key_id);
	key_created = false;
}

const struct op aead_keysetup_ops[] = {{"generate", generate_key, NULL}};
const struct op aead_operations[] = {{"encrypt", encrypt, NULL}, {"decrypt", decrypt, NULL}};
const struct op aead_multipart_operations[] = {
	{"encrypt", multipart_encrypt, NULL},
	{"decrypt", multipart_decrypt, NULL},
};
