/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "signature_test_logic.h"

#include "ml_dsa_65_vectors.h"

#define MAX_HASH_SIZE 64
/* Longest signature produced here: RSA-4096, 512 bytes. ML-DSA verifies out of
 * the vector's own array, so it does not size this.
 */
#define MAX_SIGNATURE_SIZE 512

static const uint8_t text[TEXT_SIZE] = "Crypto benchmarks signature test data.";
static uint8_t hash[MAX_HASH_SIZE];
static uint8_t signature[MAX_SIGNATURE_SIZE];
static size_t hash_length;
static size_t signature_length;
static psa_key_id_t key_id;
static bool key_created;

static psa_status_t generate_key(const void *context)
{
	const struct signature_test_data *test = context;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;
	psa_key_usage_t usage = test->hash_algorithm == 0
					? PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_VERIFY_MESSAGE
					: PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH;

	psa_set_key_usage_flags(&attributes, usage);
	psa_set_key_algorithm(&attributes, test->algorithm);
	psa_set_key_type(&attributes, test->key_type);
	psa_set_key_bits(&attributes, test->key_bits);

	status = psa_generate_key(&attributes, &key_id);
	psa_reset_key_attributes(&attributes);

	key_created = status == PSA_SUCCESS;

	return status;
}

static psa_status_t sign(const void *context)
{
	const struct signature_test_data *test = context;
	psa_status_t status;

	if (test->hash_algorithm == 0) {
		return psa_sign_message(key_id, test->algorithm, text, sizeof(text), signature,
					sizeof(signature), &signature_length);
	}

	status = psa_hash_compute(test->hash_algorithm, text, sizeof(text), hash, sizeof(hash),
				  &hash_length);
	if (status != PSA_SUCCESS) {
		return status;
	}

	return psa_sign_hash(key_id, test->algorithm, hash, hash_length, signature,
			     sizeof(signature), &signature_length);
}

static psa_status_t verify(const void *context)
{
	const struct signature_test_data *test = context;
	psa_status_t status;

	if (test->hash_algorithm == 0) {
		return psa_verify_message(key_id, test->algorithm, text, sizeof(text), signature,
					  signature_length);
	}

	status = psa_hash_compute(test->hash_algorithm, text, sizeof(text), hash, sizeof(hash),
				  &hash_length);

	if (status != PSA_SUCCESS) {
		return status;
	}
	return psa_verify_hash(key_id, test->algorithm, hash, hash_length, signature,
			       signature_length);
}

/*
 * ML-DSA is verify-only on Cracen: no signing, no key pair generate, import or
 * export, and no other driver can take over because Cracen claims the algorithm
 * outright. So a known-answer vector's public half is the only way to get a key
 * in, and its signature the only one to verify.
 */
static psa_status_t ml_dsa_import_key(const void *context)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	ARG_UNUSED(context);

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_MESSAGE);
	psa_set_key_algorithm(&attributes, PSA_ALG_ML_DSA);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_ML_DSA_PUBLIC_KEY);

	status = psa_import_key(&attributes, ml_dsa_65_pub_key, sizeof(ml_dsa_65_pub_key),
				&key_id);
	psa_reset_key_attributes(&attributes);

	key_created = status == PSA_SUCCESS;

	return status;
}

static psa_status_t ml_dsa_verify(const void *context)
{
	ARG_UNUSED(context);

	return psa_verify_message(key_id, PSA_ALG_ML_DSA, ml_dsa_65_message,
				  sizeof(ml_dsa_65_message), ml_dsa_65_signature,
				  sizeof(ml_dsa_65_signature));
}

void signature_cleanup(void)
{
	if (!key_created) {
		return;
	}

	(void)psa_destroy_key(key_id);
	key_created = false;
}

const struct op signature_keysetup_ops[] = {{"generate", generate_key, NULL}};
const struct op signature_operations[] = {{"sign", sign, NULL}, {"verify", verify, NULL}};
const struct op ml_dsa_keysetup_ops[] = {{"import", ml_dsa_import_key, NULL}};
const struct op ml_dsa_operations[] = {{"verify", ml_dsa_verify, NULL}};
