/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <psa/crypto.h>
#include <string.h>
#include <util/util_macro.h>
#include <nrf_security_mem_helpers.h>

#include "vectors_rsa.h"

extern void test_hash(void);

/* RSA verification */
static void init_rsa_key(psa_key_attributes_t *attributes, psa_key_type_t key_type,
			 psa_algorithm_t alg, psa_key_usage_t usage, size_t key_size_bits)
{
	psa_set_key_usage_flags(attributes, usage);
	psa_set_key_lifetime(attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(attributes, alg);
	psa_set_key_type(attributes, key_type);
	psa_set_key_bits(attributes, key_size_bits);
}

static void test_rsa_verify_message(psa_key_attributes_t *attributes,
				    const uint8_t *pub_key, size_t pub_key_size,
				    const uint8_t *message, size_t message_size,
				    psa_algorithm_t alg, const char *key_label,
				    const uint8_t *signature, size_t signature_size)
{
	psa_status_t err;
	mbedtls_svc_key_id_t key_id = MBEDTLS_SVC_KEY_ID_INIT;

	err = psa_import_key(attributes, pub_key, pub_key_size, &key_id);
	zassert_equal(err, PSA_SUCCESS,
		      "Failed to import key for '%s'. Err: %d", key_label, err);

	err = psa_verify_message(key_id, alg, message, message_size, signature, signature_size);
	zassert_equal(err, PSA_SUCCESS,
		      "Failed to verify for '%s'. Err: %d",
		      key_label, err);

	err = psa_destroy_key(key_id);
	zassert_equal(err, PSA_SUCCESS,
		      "Failed to destroy key for '%s'. Err: %d",
		      key_label, err);
}

static void test_rsa_verify_hash(psa_key_attributes_t *attributes,
				 const uint8_t *pub_key, size_t pub_key_size,
				 const uint8_t *digest, size_t digest_size,
				 psa_algorithm_t alg, const char *key_label,
				 const uint8_t *signature, size_t signature_size)
{
	psa_status_t err;
	mbedtls_svc_key_id_t key_id = MBEDTLS_SVC_KEY_ID_INIT;

	err = psa_import_key(attributes, pub_key, pub_key_size, &key_id);
	zassert_equal(err, PSA_SUCCESS,
		      "Failed to import key for '%s'. Err: %d", key_label, err);

	err = psa_verify_hash(key_id, alg, digest, digest_size, signature, signature_size);
	zassert_equal(err, PSA_SUCCESS,
		      "Failed to verify for '%s'. Err: %d",
		      key_label, err);

	err = psa_destroy_key(key_id);
	zassert_equal(err, PSA_SUCCESS,
		      "Failed to destroy key for '%s' key. Err: %d",
		      key_label, err);
}

/* RSA PSS tests */

/**
 * @brief Function to test RSA PSS verify hash with 2048 bit key
 */
static void test_rsa_pss_2048_verify_message(void)
{
	psa_key_attributes_t attributes = {0};
	psa_algorithm_t alg = PSA_ALG_RSA_PSS(PSA_ALG_SHA_256);
	static const char key_label[] = "RSA PSS 2048 (verify message)";

	init_rsa_key(&attributes, PSA_KEY_TYPE_RSA_PUBLIC_KEY, alg, PSA_KEY_USAGE_VERIFY_MESSAGE,
		     RSA_2048_KEY_BIT_SIZE);

	test_rsa_verify_message(&attributes,
				rsa_pss_2048_pubkey, sizeof(rsa_pss_2048_pubkey),
				rsa_pss_2048_msg, sizeof(rsa_pss_2048_msg),
				alg, key_label,
				rsa_pss_2048_signature, sizeof(rsa_pss_2048_signature));
}

/**
 * @brief Function to test RSA PSS verify hash with 2048 bit key
 */
static void test_rsa_pss_2048_verify_hash(void)
{
	psa_key_attributes_t attributes = {0};
	psa_algorithm_t alg = PSA_ALG_RSA_PSS(PSA_ALG_SHA_256);
	static const char key_label[] = "RSA PSS 2048 (verify hash)";

	init_rsa_key(&attributes, PSA_KEY_TYPE_RSA_PUBLIC_KEY, alg, PSA_KEY_USAGE_VERIFY_HASH,
		     RSA_2048_KEY_BIT_SIZE);

	test_rsa_verify_hash(&attributes,
			     rsa_pss_2048_pubkey, sizeof(rsa_pss_2048_pubkey),
			     rsa_pss_2048_hash, sizeof(rsa_pss_2048_hash),
			     alg, key_label,
			     rsa_pss_2048_signature, sizeof(rsa_pss_2048_signature));
}

/* RSA PKCS#1 v1.5 tests */

/**
 * @brief Function to test RSA PKCS#1 v1.5 verify hash with 2048 bit key
 */
static void test_rsa_pkcs1v15_2048_verify_message(void)
{
	psa_key_attributes_t attributes = {0};
	psa_algorithm_t alg = PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256);
	static const uint8_t key_label[] = "RSA PKCS#1 v1.5 2048 (verify message)";

	init_rsa_key(&attributes, PSA_KEY_TYPE_RSA_PUBLIC_KEY, alg, PSA_KEY_USAGE_VERIFY_MESSAGE,
		     RSA_2048_KEY_BIT_SIZE);

	test_rsa_verify_message(&attributes,
				rsa_pkcs1v15_2048_pubkey, sizeof(rsa_pkcs1v15_2048_pubkey),
				rsa_pkcs1v15_2048_msg, sizeof(rsa_pkcs1v15_2048_msg),
				alg, key_label,
				rsa_pkcs1v15_2048_signature, sizeof(rsa_pkcs1v15_2048_signature));
}

/**
 * @brief Function to test RSA PKCS#1 v1.5 verify hash with 2048 bit key
 */
static void test_rsa_pkcs1v15_2048_verify_hash(void)
{
	psa_key_attributes_t attributes = {0};
	psa_algorithm_t alg = PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_256);
	static uint8_t key_label[] = "RSA PKCS#1 v1.5 2048 (verify hash)";

	init_rsa_key(&attributes, PSA_KEY_TYPE_RSA_PUBLIC_KEY, alg, PSA_KEY_USAGE_VERIFY_HASH,
		     RSA_2048_KEY_BIT_SIZE);

	test_rsa_verify_hash(&attributes,
			     rsa_pkcs1v15_2048_pubkey, sizeof(rsa_pkcs1v15_2048_pubkey),
			     rsa_pkcs1v15_2048_hash, sizeof(rsa_pkcs1v15_2048_hash),
			     alg, key_label,
			     rsa_pkcs1v15_2048_signature, sizeof(rsa_pkcs1v15_2048_signature));
}

/**
 * @brief Function to test RSA PSS verify message with 3072 bit key
 */
static void test_rsa_pss_3072_verify_message(void)
{
	psa_key_attributes_t attributes = {0};
	psa_algorithm_t alg = PSA_ALG_RSA_PSS(PSA_ALG_SHA_384);
	static const char key_label[] = "RSA PSS 3072 (verify message)";

	init_rsa_key(&attributes, PSA_KEY_TYPE_RSA_PUBLIC_KEY, alg, PSA_KEY_USAGE_VERIFY_MESSAGE,
		     RSA_3072_KEY_BIT_SIZE);

	test_rsa_verify_message(&attributes,
				rsa_pss_3072_pubkey, sizeof(rsa_pss_3072_pubkey),
				rsa_pss_3072_msg, sizeof(rsa_pss_3072_msg),
				alg, key_label,
				rsa_pss_3072_signature, sizeof(rsa_pss_3072_signature));
}

/**
 * @brief Function to test RSA PSS verify hash with 3072 bit key
 */
static void test_rsa_pss_3072_verify_hash(void)
{
	psa_key_attributes_t attributes = {0};
	psa_algorithm_t alg = PSA_ALG_RSA_PSS(PSA_ALG_SHA_384);
	static const char key_label[] = "RSA PSS 3072 (verify hash)";

	init_rsa_key(&attributes, PSA_KEY_TYPE_RSA_PUBLIC_KEY, alg, PSA_KEY_USAGE_VERIFY_HASH,
		     RSA_3072_KEY_BIT_SIZE);

	test_rsa_verify_hash(&attributes,
			     rsa_pss_3072_pubkey, sizeof(rsa_pss_3072_pubkey),
			     rsa_pss_3072_hash, sizeof(rsa_pss_3072_hash),
			     alg, key_label,
			     rsa_pss_3072_signature, sizeof(rsa_pss_3072_signature));
}

static void test_rsa_pkcs1v15_3072_verify_message(void)
{
	psa_key_attributes_t attributes = {0};
	psa_algorithm_t alg = PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_384);
	static uint8_t key_label[] = "RSA PKCS#1 v1.5 3072 (verify message)";

	init_rsa_key(&attributes, PSA_KEY_TYPE_RSA_PUBLIC_KEY, alg, PSA_KEY_USAGE_VERIFY_MESSAGE,
		     RSA_3072_KEY_BIT_SIZE);

	test_rsa_verify_message(&attributes,
				rsa_pkcs1v15_3072_pubkey, sizeof(rsa_pkcs1v15_3072_pubkey),
				rsa_pkcs1v15_3072_msg, sizeof(rsa_pkcs1v15_3072_msg),
				alg, key_label,
				rsa_pkcs1v15_3072_signature, sizeof(rsa_pkcs1v15_3072_signature));
}

static void test_rsa_pkcs1v15_3072_verify_hash(void)
{
	psa_key_attributes_t attributes = {0};
	psa_algorithm_t alg = PSA_ALG_RSA_PKCS1V15_SIGN(PSA_ALG_SHA_384);
	static uint8_t key_label[] = "RSA PKCS#1 v1.5 3072 (verify hash)";

	init_rsa_key(&attributes, PSA_KEY_TYPE_RSA_PUBLIC_KEY, alg, PSA_KEY_USAGE_VERIFY_HASH,
		     RSA_3072_KEY_BIT_SIZE);

	test_rsa_verify_hash(&attributes,
			     rsa_pkcs1v15_3072_pubkey, sizeof(rsa_pkcs1v15_3072_pubkey),
			     rsa_pkcs1v15_3072_hash, sizeof(rsa_pkcs1v15_3072_hash),
			     alg, key_label,
			     rsa_pkcs1v15_3072_signature, sizeof(rsa_pkcs1v15_3072_signature));
}

void test_verify_rsa(void)
{
	bool ran_verify = false;
	/* RSA PSS 2048 with SHA-256 */
	if (IS_ENABLED_ALL(PSA_WANT_ALG_RSA_PSS, PSA_WANT_RSA_KEY_SIZE_2048,
			   PSA_WANT_ALG_SHA_256)) {
		test_rsa_pss_2048_verify_message();
		test_rsa_pss_2048_verify_hash();
		ran_verify = true;
	}

	/* RSA PKCS#1 v1.5 2048 with SHA-256 */
	if (IS_ENABLED_ALL(PSA_WANT_ALG_RSA_PKCS1V15_SIGN, PSA_WANT_RSA_KEY_SIZE_2048,
			   PSA_WANT_ALG_SHA_256)) {
		test_rsa_pkcs1v15_2048_verify_message();
		test_rsa_pkcs1v15_2048_verify_hash();
		ran_verify = true;
	}

	/* RSA PSS 3072 with SHA-384 */
	if (IS_ENABLED_ALL(PSA_WANT_ALG_RSA_PSS, PSA_WANT_RSA_KEY_SIZE_3072,
			   PSA_WANT_ALG_SHA_384)) {
		test_rsa_pss_3072_verify_message();
		test_rsa_pss_3072_verify_hash();
		ran_verify = true;
	}

	/* RSA PKCS#1 v1.5 3072 with SHA-384 */
	if (IS_ENABLED_ALL(PSA_WANT_ALG_RSA_PKCS1V15_SIGN, PSA_WANT_RSA_KEY_SIZE_3072,
			   PSA_WANT_ALG_SHA_384)) {
		test_rsa_pkcs1v15_3072_verify_message();
		test_rsa_pkcs1v15_3072_verify_hash();
		ran_verify = true;
	}

	zassert_true(ran_verify, "Did not run RSA signature verify, check configs!");
}

static void test_decrypt_rsa_oaep(psa_key_attributes_t *attributes,
				 const uint8_t *priv_key, size_t priv_key_size,
				 const uint8_t *cipher, size_t cipher_size,
				 const uint8_t *expected_plain, size_t expected_plain_size,
				 psa_algorithm_t alg, const char *key_label)
{
	psa_status_t err;
	mbedtls_svc_key_id_t key_id = MBEDTLS_SVC_KEY_ID_INIT;
	size_t output_size;
	uint8_t plain[RSA_OAEP_MAX_OUTPUT_SIZE] = {0};

	zassert_true(expected_plain_size <= RSA_OAEP_MAX_OUTPUT_SIZE,
		     "Too large input for RSA decrypt for '%s' %d > %d", key_label,
		     expected_plain_size, RSA_OAEP_MAX_OUTPUT_SIZE);

	err = psa_import_key(attributes, priv_key, priv_key_size, &key_id);
	zassert_equal(err, PSA_SUCCESS,
		      "Failed to import key for '%s'. Err: %d", key_label, err);

	err = psa_asymmetric_decrypt(key_id, alg, cipher, cipher_size, "", 0,
				     plain, sizeof(plain), &output_size);
	zassert_equal(err, PSA_SUCCESS,
		      "Failed to decrypt for '%s'. Err: %d",
		      key_label, err);

	zassert_equal(expected_plain_size, output_size,
		      "Output size for decrypt mismatched. Expected: %d, got: %d",
		      expected_plain_size, output_size);

	if (constant_memcmp(plain, expected_plain, expected_plain_size) != 0) {
		zassert_false("Decrypted RSA OAEP did not match expected plaintext");
	}

	err = psa_destroy_key(key_id);
	zassert_equal(err, PSA_SUCCESS,
		      "Failed to destroy key for '%s' key. Err: %d",
		      key_label, err);
}

void test_decrypt_rsa_oaep_2048(void)
{
	psa_key_attributes_t attributes = {0};
	psa_algorithm_t alg = PSA_ALG_RSA_OAEP(PSA_ALG_SHA_256);
	static uint8_t key_label[] = "RSA OAEP decrypt 2048";

	init_rsa_key(&attributes, PSA_KEY_TYPE_RSA_KEY_PAIR, alg, PSA_KEY_USAGE_DECRYPT,
		     RSA_2048_KEY_BIT_SIZE);

	test_decrypt_rsa_oaep(
		&attributes, rsa_oaep_2048_private_key, sizeof(rsa_oaep_2048_private_key),
		rsa_oaep_2048_ciphertext, sizeof(rsa_oaep_2048_ciphertext),
		rsa_oaep_2048_plaintext, sizeof(rsa_oaep_2048_plaintext), alg, key_label);
}

void test_decrypt_rsa_oaep_3072(void)
{
	psa_key_attributes_t attributes = {0};
	psa_algorithm_t alg = PSA_ALG_RSA_OAEP(PSA_ALG_SHA_256);
	static uint8_t key_label[] = "RSA OAEP decrypt 3072";

	init_rsa_key(&attributes, PSA_KEY_TYPE_RSA_KEY_PAIR, alg, PSA_KEY_USAGE_DECRYPT,
		     RSA_3072_KEY_BIT_SIZE);

	test_decrypt_rsa_oaep(
		&attributes, rsa_oaep_3072_priv_key, sizeof(rsa_oaep_3072_priv_key),
		rsa_oaep_3072_ciphertext, sizeof(rsa_oaep_3072_ciphertext),
		rsa_oaep_3072_plaintext, sizeof(rsa_oaep_3072_plaintext), alg, key_label);
}

void test_decrypt_rsa(void)
{
	bool ran_decrypt = false;

	if (IS_ENABLED_ALL(PSA_WANT_ALG_RSA_OAEP, PSA_WANT_RSA_KEY_SIZE_2048,
			   PSA_WANT_ALG_SHA_256)) {
		test_decrypt_rsa_oaep_2048();
		ran_decrypt = true;
	}

	if (IS_ENABLED_ALL(PSA_WANT_ALG_RSA_OAEP, PSA_WANT_RSA_KEY_SIZE_3072,
			   PSA_WANT_ALG_SHA_256)) {
		test_decrypt_rsa_oaep_3072();
		ran_decrypt = true;
	}

	zassert_true(ran_decrypt, "Did not run RSA OAEP decrypt, check configs!");
}

void test_main(void)
{
	bool ran_tests = false;

	/* Test hashing */
	if (IS_ENABLED(PSA_CORE_LITE_HAS_HASH)) {
		test_hash();
		ran_tests = true;
	}


	/* Verify is required to be run in these tests */
	if (IS_ENABLED_ANY(PSA_WANT_ALG_RSA_PSS, PSA_WANT_ALG_RSA_PKCS1V15_SIGN)) {
		test_verify_rsa();
		ran_tests = true;
	}

	/* Test that RSA decrypt works for transport key wrapping */
	if (IS_ENABLED(PSA_WANT_ALG_RSA_OAEP)) {
		test_decrypt_rsa();
		ran_tests = true;
	}

	zassert_true(ran_tests, "psa_core_lite unit tests did not run (check config)");
}
