/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>

#include "asymmetric_encryption_test_logic.h"

#define RSA_CIPHERTEXT_SIZE 256

/* RSA-2048 OAEP capacity: the modulus less two SHA-256 digests and two bytes.
 * PKCS#1 v1.5 carries more, so OAEP is what bounds the shared TEXT_SIZE.
 */
#define RSA_OAEP_MAX_TEXT_SIZE (RSA_CIPHERTEXT_SIZE - 2 * 32 - 2)

BUILD_ASSERT(TEXT_SIZE <= RSA_OAEP_MAX_TEXT_SIZE,
	     "TEXT_SIZE is more than RSA-2048 OAEP with SHA-256 can encrypt");

static const uint8_t plaintext[TEXT_SIZE] = "Crypto benchmarks RSA encryption test data.";
static uint8_t ciphertext[RSA_CIPHERTEXT_SIZE];
static uint8_t decrypted[TEXT_SIZE];
static size_t ciphertext_length;
static size_t decrypted_length;
static psa_key_id_t key_id;
static bool key_created;

static psa_status_t generate_key(const void *context)
{
	const struct asymmetric_encryption_test_data *test = context;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attributes, test->algorithm);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_RSA_KEY_PAIR);
	psa_set_key_bits(&attributes, 2048);
	status = psa_generate_key(&attributes, &key_id);
	psa_reset_key_attributes(&attributes);

	key_created = status == PSA_SUCCESS;

	return status;
}

static psa_status_t encrypt(const void *context)
{
	const struct asymmetric_encryption_test_data *test = context;

	return psa_asymmetric_encrypt(key_id, test->algorithm, plaintext, sizeof(plaintext),
				     NULL, 0, ciphertext, sizeof(ciphertext), &ciphertext_length);
}

static psa_status_t decrypt(const void *context)
{
	const struct asymmetric_encryption_test_data *test = context;

	return psa_asymmetric_decrypt(key_id, test->algorithm, ciphertext, ciphertext_length,
				     NULL, 0, decrypted, sizeof(decrypted), &decrypted_length);
}

int asymmetric_encryption_check(void)
{
	return decrypted_length == sizeof(plaintext) &&
		memcmp(decrypted, plaintext, sizeof(plaintext)) == 0 ? APP_SUCCESS : APP_ERROR;
}

void asymmetric_encryption_cleanup(void)
{
	if (key_created) {
		(void)psa_destroy_key(key_id);
		key_created = false;
	}
}

const struct op asymmetric_encryption_keysetup_ops[] = {{"generate", generate_key, NULL}};
const struct op asymmetric_encryption_operations[] = {{"encrypt", encrypt, NULL},
	{"decrypt", decrypt, NULL}};