/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <psa/crypto.h>
#include <string.h>

#include "ml_kem_vectors.h"

/* Encoded sizes of the ML-KEM parameter sets (FIPS 203, Table 3). */
#define ML_KEM_768_KEY_BITS 768
#define ML_KEM_768_PK_SIZE  1184
#define ML_KEM_768_CT_SIZE  1088

/* Buffers are sized for the largest supported parameter set (ML-KEM-1024). */
#define ML_KEM_MAX_PK_SIZE 1568
#define ML_KEM_MAX_CT_SIZE 1568

/* ML-KEM key pairs are stored as the 64-byte (d || z) seed. */
#define ML_KEM_SEED_SIZE 64

/* ML-KEM shared secret size. */
#define ML_KEM_SS_SIZE 32

struct ml_kem_param_set {
	size_t key_bits;
	size_t pk_size;
	size_t ct_size;
	const struct ml_kem_kat *kats;
	size_t kat_count;
};

static const struct ml_kem_param_set ml_kem_768 = {
	.key_bits = ML_KEM_768_KEY_BITS,
	.pk_size = ML_KEM_768_PK_SIZE,
	.ct_size = ML_KEM_768_CT_SIZE,
	.kats = ml_kem_768_kats,
	.kat_count = ARRAY_SIZE(ml_kem_768_kats),
};

/* Arbitrary (d || z) seed used by the tests that import a key pair. */
static const uint8_t kem_seed[ML_KEM_SEED_SIZE] = {
	0x6D, 0xBB, 0xC4, 0x37, 0x51, 0x36, 0xDF, 0x3B, 0x07, 0xF7, 0xC7, 0x0E, 0x63, 0x9E, 0x22,
	0x3E, 0x17, 0x7E, 0x7F, 0xD5, 0x3B, 0x16, 0x1B, 0x3F, 0x4D, 0x57, 0x79, 0x17, 0x94, 0xF1,
	0x26, 0x24, 0xF6, 0x96, 0x48, 0x40, 0x48, 0xEC, 0x21, 0xF9, 0x6C, 0xF5, 0x0A, 0x56, 0xD0,
	0x75, 0x9C, 0x44, 0x8F, 0x37, 0x79, 0x75, 0x2F, 0x03, 0x83, 0xD3, 0x74, 0x49, 0x69, 0x06,
	0x94, 0xCF, 0x7A, 0x68};

/**
 * @brief Common test suite setup function
 */
static void *setup_crypto(void)
{
	psa_status_t status = psa_crypto_init();

	zassert_equal(status, PSA_SUCCESS, "PSA Crypto initialization failed");
	return NULL;
}

/** Attributes of an ML-KEM key pair usable for both encapsulation and decapsulation. */
static void ml_kem_key_pair_attributes(psa_key_attributes_t *attr,
				       const struct ml_kem_param_set *params)
{
	psa_set_key_usage_flags(attr, PSA_KEY_USAGE_ENCRYPT |
				      PSA_KEY_USAGE_DECRYPT |
				      PSA_KEY_USAGE_EXPORT);

	psa_set_key_algorithm(attr, PSA_ALG_ML_KEM);
	psa_set_key_type(attr, PSA_KEY_TYPE_ML_KEM_KEY_PAIR);
	psa_set_key_bits(attr, params->key_bits);
}

static void shared_secret_attributes(psa_key_attributes_t *attr)
{
	psa_set_key_usage_flags(attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(attr, PSA_ALG_CCM);
	psa_set_key_type(attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(attr, PSA_BYTES_TO_BITS(ML_KEM_SS_SIZE));
}

static psa_key_id_t ml_kem_import_key_pair(const struct ml_kem_param_set *params,
					   const uint8_t *seed, size_t seed_len)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key = PSA_KEY_ID_NULL;
	psa_status_t status;

	ml_kem_key_pair_attributes(&attr, params);

	status = psa_import_key(&attr, seed, seed_len, &key);
	psa_reset_key_attributes(&attr);
	zassert_equal(status, PSA_SUCCESS, "ML-KEM key pair import failed, got %d", status);

	return key;
}

static psa_key_id_t ml_kem_generate_key_pair(const struct ml_kem_param_set *params)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key = PSA_KEY_ID_NULL;
	psa_status_t status;

	ml_kem_key_pair_attributes(&attr, params);

	status = psa_generate_key(&attr, &key);
	psa_reset_key_attributes(&attr);
	zassert_equal(status, PSA_SUCCESS, "ML-KEM key pair generation failed, got %d", status);

	return key;
}

static psa_key_id_t ml_kem_derive_key_pair(const struct ml_kem_param_set *params)
{
	psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key = PSA_KEY_ID_NULL;
	psa_key_id_t base_key = PSA_KEY_ID_NULL;
	psa_status_t status;

	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_HKDF(PSA_ALG_SHA_256));
	psa_set_key_type(&attr, PSA_KEY_TYPE_DERIVE);
	psa_set_key_bits(&attr, PSA_BYTES_TO_BITS(32));
	status = psa_import_key(&attr, kem_seed, 32, &base_key);
	psa_reset_key_attributes(&attr);
	zassert_equal(status, PSA_SUCCESS, "derivation base key import failed, got %d", status);

	status = psa_key_derivation_setup(&op, PSA_ALG_HKDF(PSA_ALG_SHA_256));
	zassert_equal(status, PSA_SUCCESS, "key derivation setup failed, got %d", status);
	status = psa_key_derivation_input_key(&op, PSA_KEY_DERIVATION_INPUT_SECRET, base_key);
	zassert_equal(status, PSA_SUCCESS, "key derivation secret input failed, got %d", status);
	status = psa_key_derivation_input_bytes(&op, PSA_KEY_DERIVATION_INPUT_INFO, NULL, 0);
	zassert_equal(status, PSA_SUCCESS, "key derivation info input failed, got %d", status);

	ml_kem_key_pair_attributes(&attr, params);
	status = psa_key_derivation_output_key(&attr, &op, &key);
	psa_reset_key_attributes(&attr);
	zassert_equal(status, PSA_SUCCESS, "ML-KEM key pair derivation failed, got %d", status);

	psa_key_derivation_abort(&op);
	psa_destroy_key(base_key);

	return key;
}

static psa_key_id_t ml_kem_get_public_key_from_key_pair(psa_key_id_t key,
							const struct ml_kem_param_set *params)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	uint8_t pub[ML_KEM_MAX_PK_SIZE];
	psa_key_id_t pub_key = PSA_KEY_ID_NULL;
	psa_status_t status;
	size_t pub_len;

	status = psa_export_public_key(key, pub, sizeof(pub), &pub_len);
	zassert_equal(status, PSA_SUCCESS, "ML-KEM public key export failed, got %d", status);
	zassert_equal(pub_len, params->pk_size, "unexpected public key size %zu", pub_len);

	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT);
	psa_set_key_algorithm(&attr, PSA_ALG_ML_KEM);
	psa_set_key_type(&attr, PSA_KEY_TYPE_ML_KEM_PUBLIC_KEY);
	psa_set_key_bits(&attr, params->key_bits);

	status = psa_import_key(&attr, pub, pub_len, &pub_key);
	psa_reset_key_attributes(&attr);
	zassert_equal(status, PSA_SUCCESS, "ML-KEM public key import failed, got %d", status);

	return pub_key;
}

/**
 * @brief Encapsulate using key pair @p key and return
 *        the ciphertext and the exported shared secret.
 */
static void ml_kem_encapsulate(psa_key_id_t key, const struct ml_kem_param_set *params,
			       uint8_t *ct, size_t ct_size, size_t *ct_len, uint8_t *ss,
			       size_t ss_size, size_t *ss_len)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t ss_key = PSA_KEY_ID_NULL;
	psa_status_t status;

	shared_secret_attributes(&attr);

	status = psa_encapsulate(key, PSA_ALG_ML_KEM, &attr, &ss_key, ct, ct_size, ct_len);
	psa_reset_key_attributes(&attr);
	zassert_equal(status, PSA_SUCCESS, "ML-KEM encapsulation failed, got %d", status);
	zassert_equal(*ct_len, params->ct_size, "unexpected ciphertext size %zu", *ct_len);

	status = psa_export_key(ss_key, ss, ss_size, ss_len);
	zassert_equal(status, PSA_SUCCESS, "shared secret export failed, got %d", status);
	zassert_equal(*ss_len, ML_KEM_SS_SIZE, "unexpected shared secret size %zu", *ss_len);

	psa_destroy_key(ss_key);
}

/**
 * @brief Decapsulate ciphertext @p ct using key pair @p key and return the exported shared secret.
 */
static void ml_kem_decapsulate(psa_key_id_t key, const uint8_t *ct, size_t ct_len, uint8_t *ss,
			       size_t ss_size, size_t *ss_len)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t ss_key = PSA_KEY_ID_NULL;
	psa_status_t status;

	shared_secret_attributes(&attr);

	status = psa_decapsulate(key, PSA_ALG_ML_KEM, ct, ct_len, &attr, &ss_key);
	psa_reset_key_attributes(&attr);
	zassert_equal(status, PSA_SUCCESS, "ML-KEM decapsulation failed, got %d", status);

	status = psa_export_key(ss_key, ss, ss_size, ss_len);
	zassert_equal(status, PSA_SUCCESS, "shared secret export failed, got %d", status);
	zassert_equal(*ss_len, ML_KEM_SS_SIZE, "unexpected shared secret size %zu", *ss_len);

	psa_destroy_key(ss_key);
}

/**
 * @brief Encapsulate to the public key of @p key and decapsulate with @p key.
 *
 * Both sides must end up with the same shared secret.
 */
static void test_ml_kem_round_trip_public_key(psa_key_id_t key,
					      const struct ml_kem_param_set *params)
{
	psa_key_id_t pub_key;

	uint8_t ciphertext[ML_KEM_MAX_CT_SIZE];
	size_t ciphertext_len;
	uint8_t ss_enc[ML_KEM_SS_SIZE];
	size_t ss_enc_len;
	uint8_t ss_dec[ML_KEM_SS_SIZE];
	size_t ss_dec_len;

	pub_key = ml_kem_get_public_key_from_key_pair(key, params);

	ml_kem_encapsulate(pub_key, params,
			   ciphertext, sizeof(ciphertext), &ciphertext_len,
			   ss_enc, sizeof(ss_enc), &ss_enc_len);

	ml_kem_decapsulate(key, ciphertext, ciphertext_len, ss_dec, sizeof(ss_dec), &ss_dec_len);

	zassert_equal(ss_enc_len, ss_dec_len, "shared secret lengths differ");
	zassert_mem_equal(ss_enc, ss_dec, ss_enc_len, "shared secrets do not match");

	psa_destroy_key(pub_key);
}

ZTEST_SUITE(psa_ml_kem_tests, NULL, setup_crypto, NULL, NULL, NULL);

/**
 * @brief Checks ML-KEM operation result if the key (seed) was imported.
 */
ZTEST(psa_ml_kem_tests, test_ml_kem_768_imported_key)
{
	const struct ml_kem_param_set *params = &ml_kem_768;
	psa_key_id_t key = ml_kem_import_key_pair(params, kem_seed, sizeof(kem_seed));
	uint8_t seed[ML_KEM_SEED_SIZE];
	psa_status_t status;
	size_t seed_len;

	/* An imported key pair must export back to the seed it was created from. */
	status = psa_export_key(key, seed, sizeof(seed), &seed_len);
	zassert_equal(status, PSA_SUCCESS, "ML-KEM key pair export failed, got %d", status);
	zassert_equal(seed_len, ML_KEM_SEED_SIZE, "wrong size of exported seed");
	zassert_mem_equal(seed, kem_seed, sizeof(kem_seed), "exported seed does not match");

	test_ml_kem_round_trip_public_key(key, params);

	psa_destroy_key(key);
}

/**
 * @brief Checks ML-KEM operation result if the key (seed) was generated.
 */
ZTEST(psa_ml_kem_tests, test_ml_kem_768_generated_key)
{
	const struct ml_kem_param_set *params = &ml_kem_768;
	psa_key_id_t key = ml_kem_generate_key_pair(params);

	test_ml_kem_round_trip_public_key(key, params);

	psa_destroy_key(key);
}

/**
 * @brief Checks ML-KEM operation result if the key (seed) was derived.
 */
ZTEST(psa_ml_kem_tests, test_ml_kem_768_derived_key)
{
	const struct ml_kem_param_set *params = &ml_kem_768;
	psa_key_id_t key = ml_kem_derive_key_pair(params);

	test_ml_kem_round_trip_public_key(key, params);

	psa_destroy_key(key);
}

/**
 * @brief Check if ML-KEM round works with key pair.
 *
 * Encapsulation must be able to give the same result either with key pair
 * or with a public key.
 */
ZTEST(psa_ml_kem_tests, test_ml_kem_768_encapsulate_to_key_pair)
{
	const struct ml_kem_param_set *params = &ml_kem_768;
	psa_key_id_t key = ml_kem_import_key_pair(params, kem_seed, sizeof(kem_seed));

	uint8_t ciphertext[ML_KEM_MAX_CT_SIZE];
	size_t ciphertext_len;
	uint8_t ss_enc[ML_KEM_SS_SIZE];
	size_t ss_enc_len;
	uint8_t ss_dec[ML_KEM_SS_SIZE];
	size_t ss_dec_len;

	ml_kem_encapsulate(key, params,
			   ciphertext, sizeof(ciphertext), &ciphertext_len,
			   ss_enc, sizeof(ss_enc), &ss_enc_len);

	ml_kem_decapsulate(key, ciphertext, ciphertext_len, ss_dec, sizeof(ss_dec), &ss_dec_len);

	zassert_equal(ss_enc_len, ss_dec_len, "shared secret lengths differ");
	zassert_mem_equal(ss_enc, ss_dec, ss_enc_len, "shared secrets do not match");

	psa_destroy_key(key);
}

/**
 * @brief Implicit rejection of a modified ciphertext.
 *
 * ML-KEM decapsulation of an invalid ciphertext succeeds, but yields an
 * unrelated shared secret instead of reporting an error.
 */
ZTEST(psa_ml_kem_tests, test_ml_kem_768_implicit_rejection)
{
	const struct ml_kem_param_set *params = &ml_kem_768;
	psa_key_id_t key = ml_kem_import_key_pair(params, kem_seed, sizeof(kem_seed));

	uint8_t ciphertext[ML_KEM_MAX_CT_SIZE];
	size_t ciphertext_len;
	uint8_t ss_enc[ML_KEM_SS_SIZE];
	size_t ss_enc_len;
	uint8_t ss_dec[ML_KEM_SS_SIZE];
	size_t ss_dec_len;

	ml_kem_encapsulate(key, params,
			   ciphertext, sizeof(ciphertext), &ciphertext_len,
			   ss_enc, sizeof(ss_enc), &ss_enc_len);

	ciphertext[0] ^= 0x01;
	ml_kem_decapsulate(key, ciphertext, ciphertext_len, ss_dec, sizeof(ss_dec), &ss_dec_len);

	zassert_true(memcmp(ss_enc, ss_dec, ML_KEM_SS_SIZE) != 0,
		     "modified ciphertext allowed to get "
		     "the same shared secret after decapsulation");

	psa_destroy_key(key);
}

/**
 * @brief Known-answer test of key generation.
 *
 * Key generation is deterministic in the (d || z) seed, so the public key derived from
 * a published seed must match the published encapsulation key.
 */
ZTEST(psa_ml_kem_tests, test_ml_kem_768_kat_public_key)
{
	const struct ml_kem_param_set *params = &ml_kem_768;

	uint8_t pub[ML_KEM_MAX_PK_SIZE];
	size_t pub_len;

	for (size_t i = 0; i < params->kat_count; i++) {
		const struct ml_kem_kat *kat = &params->kats[i];
		psa_key_id_t key;
		psa_status_t status;

		key = ml_kem_import_key_pair(params, kat->seed, kat->seed_len);

		status = psa_export_public_key(key, pub, sizeof(pub), &pub_len);
		zassert_equal(status, PSA_SUCCESS,
			      "public key export failed for tcId %d (%s), got %d",
			      kat->tc_id, kat->comment, status);
		zassert_equal(pub_len, kat->ek_len,
			      "unexpected public key size %zu for tcId %d (%s)",
			      pub_len, kat->tc_id, kat->comment);
		zassert_mem_equal(pub, kat->ek, kat->ek_len,
				  "public key mismatch for tcId %d (%s)",
				  kat->tc_id, kat->comment);

		psa_destroy_key(key);
	}
}

/**
 * @brief Known-answer test of decapsulation.
 *
 * Decapsulation involves no randomness, so a published ciphertext must decapsulate to
 * the published shared secret.
 */
ZTEST(psa_ml_kem_tests, test_ml_kem_768_kat_decapsulate)
{
	const struct ml_kem_param_set *params = &ml_kem_768;

	uint8_t ss[ML_KEM_SS_SIZE];
	size_t ss_len;

	for (size_t i = 0; i < params->kat_count; i++) {
		const struct ml_kem_kat *kat = &params->kats[i];
		psa_key_id_t key;

		key = ml_kem_import_key_pair(params, kat->seed, kat->seed_len);

		ml_kem_decapsulate(key, kat->ct, kat->ct_len, ss, sizeof(ss), &ss_len);

		zassert_equal(ss_len, kat->ss_len,
			      "unexpected shared secret size %zu for tcId %d (%s)",
			      ss_len, kat->tc_id, kat->comment);
		zassert_mem_equal(ss, kat->ss, kat->ss_len,
				  "shared secret mismatch for tcId %d (%s)",
				  kat->tc_id, kat->comment);

		psa_destroy_key(key);
	}
}
