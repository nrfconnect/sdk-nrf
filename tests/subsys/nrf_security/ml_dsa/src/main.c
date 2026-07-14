/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <psa/crypto.h>
#include <string.h>

#include "ml_dsa_vectors.h"

/* Encoded public-key size of ML-DSA-65 (FIPS 204, Table 2), used by the
 * negative tests that do not require a valid signature.
 */
#define ML_DSA_65_PK_SIZE  1952
#define ML_DSA_65_SIG_SIZE 3309

/* Largest encoded public-key and signature sizes across the parameter sets
 * (ML-DSA-87, FIPS 204, Table 2), used to size the round-trip test buffers.
 */
#define ML_DSA_MAX_PK_SIZE  2592
#define ML_DSA_MAX_SIG_SIZE 4627

static void *setup_crypto(void)
{
	psa_status_t status = psa_crypto_init();

	zassert_equal(status, PSA_SUCCESS, "PSA Crypto initialization failed");
	return NULL;
}

static psa_key_id_t import_public_key(const uint8_t *pk, size_t pk_len, psa_algorithm_t alg)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_status_t status;

	psa_set_key_type(&attributes, PSA_KEY_TYPE_ML_DSA_PUBLIC_KEY);
	psa_set_key_usage_flags(&attributes,
				PSA_KEY_USAGE_VERIFY_MESSAGE | PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_algorithm(&attributes, alg);

	status = psa_import_key(&attributes, pk, pk_len, &key_id);
	psa_reset_key_attributes(&attributes);

	if (status != PSA_SUCCESS) {
		return PSA_KEY_ID_NULL;
	}

	return key_id;
}

ZTEST_SUITE(ml_dsa_verify, NULL, setup_crypto, NULL, NULL, NULL);

ZTEST(ml_dsa_verify, test_reject_invalid_signature)
{
	const uint8_t pk[ML_DSA_65_PK_SIZE] = {};
	const uint8_t sig[ML_DSA_65_SIG_SIZE] = {};
	psa_key_id_t key_id;
	psa_status_t status;

	key_id = import_public_key(pk, sizeof(pk), PSA_ALG_ML_DSA);
	zassert_not_equal(key_id, PSA_KEY_ID_NULL, "ML-DSA-65 public key import failed");

	status = psa_verify_message(key_id, PSA_ALG_ML_DSA, pk, sizeof(pk), sig, sizeof(sig));
	zassert_equal(status, PSA_ERROR_INVALID_SIGNATURE, "expected invalid signature, got %d",
		      status);

	psa_destroy_key(key_id);
}

ZTEST(ml_dsa_verify, test_reject_wrong_signature_length)
{
	const uint8_t pk[ML_DSA_65_PK_SIZE] = {};
	const uint8_t sig[ML_DSA_65_SIG_SIZE - 1] = {};
	psa_key_id_t key_id;
	psa_status_t status;

	key_id = import_public_key(pk, sizeof(pk), PSA_ALG_ML_DSA);
	zassert_not_equal(key_id, PSA_KEY_ID_NULL, "ML-DSA-65 public key import failed");

	status = psa_verify_message(key_id, PSA_ALG_ML_DSA, pk, sizeof(pk), sig, sizeof(sig));
	zassert_not_equal(status, PSA_SUCCESS, "wrong-length signature was accepted");

	psa_destroy_key(key_id);
}

ZTEST(ml_dsa_verify, test_reject_bad_key_size)
{
	const uint8_t pk[100] = {};
	psa_key_id_t key_id;

	key_id = import_public_key(pk, sizeof(pk), PSA_ALG_ML_DSA);
	zassert_equal(key_id, PSA_KEY_ID_NULL, "import of an undersized public key succeeded");
}

/** Positive known-answer test against a NIST ACVP sigVer vector for ML-DSA-44. */
ZTEST(ml_dsa_verify, test_kat_valid_signature_44)
{
	psa_key_id_t key_id;
	psa_status_t status;
	uint8_t tampered[sizeof(ml_dsa44_kat_sig)];

	key_id = import_public_key(ml_dsa44_kat_pk, sizeof(ml_dsa44_kat_pk), PSA_ALG_ML_DSA);
	zassert_not_equal(key_id, PSA_KEY_ID_NULL, "ML-DSA-44 KAT public key import failed");

	status = psa_verify_message(key_id, PSA_ALG_ML_DSA, ml_dsa44_kat_msg,
				    sizeof(ml_dsa44_kat_msg), ml_dsa44_kat_sig,
				    sizeof(ml_dsa44_kat_sig));
	zassert_equal(status, PSA_SUCCESS, "valid ML-DSA-44 signature was rejected, got %d",
		      status);

	/* Flipping a single bit must invalidate the signature. */
	memcpy(tampered, ml_dsa44_kat_sig, sizeof(tampered));
	tampered[0] ^= 0x01;
	status = psa_verify_message(key_id, PSA_ALG_ML_DSA, ml_dsa44_kat_msg,
				    sizeof(ml_dsa44_kat_msg), tampered, sizeof(tampered));
	zassert_equal(status, PSA_ERROR_INVALID_SIGNATURE,
		      "tampered ML-DSA-44 signature was accepted");

	psa_destroy_key(key_id);
}

/** Positive known-answer test against a NIST ACVP sigVer vector for ML-DSA-65. */
ZTEST(ml_dsa_verify, test_kat_valid_signature_65)
{
	psa_key_id_t key_id;
	psa_status_t status;
	uint8_t tampered[sizeof(ml_dsa_kat_sig)];

	key_id = import_public_key(ml_dsa_kat_pk, sizeof(ml_dsa_kat_pk), PSA_ALG_ML_DSA);
	zassert_not_equal(key_id, PSA_KEY_ID_NULL, "KAT public key import failed");

	status = psa_verify_message(key_id, PSA_ALG_ML_DSA, ml_dsa_kat_msg, sizeof(ml_dsa_kat_msg),
				    ml_dsa_kat_sig, sizeof(ml_dsa_kat_sig));
	zassert_equal(status, PSA_SUCCESS, "valid ML-DSA signature was rejected, got %d", status);

	/* Flipping a single bit must invalidate the signature. */
	memcpy(tampered, ml_dsa_kat_sig, sizeof(tampered));
	tampered[0] ^= 0x01;
	status = psa_verify_message(key_id, PSA_ALG_ML_DSA, ml_dsa_kat_msg, sizeof(ml_dsa_kat_msg),
				    tampered, sizeof(tampered));
	zassert_equal(status, PSA_ERROR_INVALID_SIGNATURE, "tampered signature was accepted");

	psa_destroy_key(key_id);
}

/** Positive known-answer test against a NIST ACVP sigVer vector for ML-DSA-87. */
ZTEST(ml_dsa_verify, test_kat_valid_signature_87)
{
	psa_key_id_t key_id;
	psa_status_t status;
	uint8_t tampered[sizeof(ml_dsa87_kat_sig)];

	key_id = import_public_key(ml_dsa87_kat_pk, sizeof(ml_dsa87_kat_pk), PSA_ALG_ML_DSA);
	zassert_not_equal(key_id, PSA_KEY_ID_NULL, "ML-DSA-87 KAT public key import failed");

	status = psa_verify_message(key_id, PSA_ALG_ML_DSA, ml_dsa87_kat_msg,
				    sizeof(ml_dsa87_kat_msg), ml_dsa87_kat_sig,
				    sizeof(ml_dsa87_kat_sig));
	zassert_equal(status, PSA_SUCCESS, "valid ML-DSA-87 signature was rejected, got %d",
		      status);

	/* Flipping a single bit must invalidate the signature. */
	memcpy(tampered, ml_dsa87_kat_sig, sizeof(tampered));
	tampered[0] ^= 0x01;
	status = psa_verify_message(key_id, PSA_ALG_ML_DSA,
				    ml_dsa87_kat_msg, sizeof(ml_dsa87_kat_msg),
				    tampered, sizeof(tampered));
	zassert_equal(status, PSA_ERROR_INVALID_SIGNATURE,
		      "tampered ML-DSA-87 signature was accepted");

	psa_destroy_key(key_id);
}

/* ============================ ML-DSA signing tests ============================ */

static const uint8_t ml_dsa_sign_msg[] = "CRACEN ML-DSA signing round-trip test message";
static const uint8_t ml_dsa_sign_ctx[] = {0x63, 0x6f, 0x6e, 0x74, 0x65, 0x78, 0x74};

static psa_key_id_t import_key_pair(size_t bits, psa_algorithm_t alg)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	uint8_t seed[32];
	psa_status_t status;

	/* Any 32-byte seed yields a valid key pair; use a deterministic pattern. */
	for (size_t i = 0; i < sizeof(seed); i++) {
		seed[i] = (uint8_t)(i + bits);
	}

	psa_set_key_type(&attributes, PSA_KEY_TYPE_ML_DSA_KEY_PAIR);
	psa_set_key_bits(&attributes, bits);
	psa_set_key_usage_flags(&attributes,
				PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_SIGN_HASH);
	psa_set_key_algorithm(&attributes, alg);

	status = psa_import_key(&attributes, seed, sizeof(seed), &key_id);
	psa_reset_key_attributes(&attributes);

	if (status != PSA_SUCCESS) {
		return PSA_KEY_ID_NULL;
	}

	return key_id;
}

/* Export the public key of a key pair and re-import it as a stand-alone public key. */
static psa_key_id_t derive_public_key(psa_key_id_t key_pair, psa_algorithm_t alg)
{
	uint8_t pk[ML_DSA_MAX_PK_SIZE];
	size_t pk_len;
	psa_status_t status;

	status = psa_export_public_key(key_pair, pk, sizeof(pk), &pk_len);
	if (status != PSA_SUCCESS) {
		return PSA_KEY_ID_NULL;
	}

	return import_public_key(pk, pk_len, alg);
}

ZTEST_SUITE(ml_dsa_sign, NULL, setup_crypto, NULL, NULL, NULL);

static void sign_verify_roundtrip(size_t bits, size_t expected_sig_size)
{
	psa_key_id_t key_pair;
	psa_key_id_t public_key;
	psa_status_t status;
	uint8_t signature[ML_DSA_MAX_SIG_SIZE];
	size_t signature_length;

	key_pair = import_key_pair(bits, PSA_ALG_ML_DSA);
	zassert_not_equal(key_pair, PSA_KEY_ID_NULL, "ML-DSA key-pair import failed");

	public_key = derive_public_key(key_pair, PSA_ALG_ML_DSA);
	zassert_not_equal(public_key, PSA_KEY_ID_NULL, "public-key derivation failed");

	status = psa_sign_message(key_pair, PSA_ALG_ML_DSA, ml_dsa_sign_msg,
				  sizeof(ml_dsa_sign_msg), signature, sizeof(signature),
				  &signature_length);
	zassert_equal(status, PSA_SUCCESS, "signing failed, got %d", status);
	zassert_equal(signature_length, expected_sig_size, "unexpected signature length %u",
		      (unsigned int)signature_length);

	status = psa_verify_message(public_key, PSA_ALG_ML_DSA, ml_dsa_sign_msg,
				    sizeof(ml_dsa_sign_msg), signature, signature_length);
	zassert_equal(status, PSA_SUCCESS, "verification of a fresh signature failed, got %d",
		      status);

	/* Flipping a single bit must invalidate the signature. */
	signature[0] ^= 0x01;
	status = psa_verify_message(public_key, PSA_ALG_ML_DSA, ml_dsa_sign_msg,
				    sizeof(ml_dsa_sign_msg), signature, signature_length);
	zassert_equal(status, PSA_ERROR_INVALID_SIGNATURE, "tampered signature was accepted");

	psa_destroy_key(key_pair);
	psa_destroy_key(public_key);
}

ZTEST(ml_dsa_sign, test_sign_verify_roundtrip_44)
{
	sign_verify_roundtrip(128, 2420);
}

ZTEST(ml_dsa_sign, test_sign_verify_roundtrip_65)
{
	sign_verify_roundtrip(192, 3309);
}

ZTEST(ml_dsa_sign, test_sign_verify_roundtrip_87)
{
	sign_verify_roundtrip(256, 4627);
}

/* A non-empty context must be bound into the signature and required for verification. */
ZTEST(ml_dsa_sign, test_sign_verify_with_context)
{
	psa_key_id_t key_pair;
	psa_key_id_t public_key;
	psa_status_t status;
	uint8_t signature[ML_DSA_MAX_SIG_SIZE];
	size_t signature_length;

	key_pair = import_key_pair(192, PSA_ALG_ML_DSA);
	zassert_not_equal(key_pair, PSA_KEY_ID_NULL, "ML-DSA key-pair import failed");

	public_key = derive_public_key(key_pair, PSA_ALG_ML_DSA);
	zassert_not_equal(public_key, PSA_KEY_ID_NULL, "public-key derivation failed");

	status = psa_sign_message_with_context(key_pair, PSA_ALG_ML_DSA, ml_dsa_sign_msg,
					       sizeof(ml_dsa_sign_msg), ml_dsa_sign_ctx,
					       sizeof(ml_dsa_sign_ctx), signature,
					       sizeof(signature), &signature_length);
	zassert_equal(status, PSA_SUCCESS, "signing with context failed, got %d", status);

	status = psa_verify_message_with_context(public_key, PSA_ALG_ML_DSA, ml_dsa_sign_msg,
						 sizeof(ml_dsa_sign_msg), ml_dsa_sign_ctx,
						 sizeof(ml_dsa_sign_ctx), signature,
						 signature_length);
	zassert_equal(status, PSA_SUCCESS, "verification with matching context failed, got %d",
		      status);

	/* Verifying with an empty context must fail (context is bound into the signature). */
	status = psa_verify_message(public_key, PSA_ALG_ML_DSA, ml_dsa_sign_msg,
				    sizeof(ml_dsa_sign_msg), signature, signature_length);
	zassert_equal(status, PSA_ERROR_INVALID_SIGNATURE,
		      "signature verified without its context");

	psa_destroy_key(key_pair);
	psa_destroy_key(public_key);
}

/* Deterministic ML-DSA must produce identical signatures for identical inputs. */
ZTEST(ml_dsa_sign, test_deterministic_signature)
{
	psa_key_id_t key_pair;
	psa_key_id_t public_key;
	psa_status_t status;
	uint8_t signature1[ML_DSA_MAX_SIG_SIZE];
	uint8_t signature2[ML_DSA_MAX_SIG_SIZE];
	size_t signature1_length;
	size_t signature2_length;

	key_pair = import_key_pair(192, PSA_ALG_DETERMINISTIC_ML_DSA);
	zassert_not_equal(key_pair, PSA_KEY_ID_NULL, "ML-DSA key-pair import failed");

	public_key = derive_public_key(key_pair, PSA_ALG_DETERMINISTIC_ML_DSA);
	zassert_not_equal(public_key, PSA_KEY_ID_NULL, "public-key derivation failed");

	status = psa_sign_message(key_pair, PSA_ALG_DETERMINISTIC_ML_DSA, ml_dsa_sign_msg,
				  sizeof(ml_dsa_sign_msg), signature1, sizeof(signature1),
				  &signature1_length);
	zassert_equal(status, PSA_SUCCESS, "first deterministic signing failed, got %d", status);

	status = psa_sign_message(key_pair, PSA_ALG_DETERMINISTIC_ML_DSA, ml_dsa_sign_msg,
				  sizeof(ml_dsa_sign_msg), signature2, sizeof(signature2),
				  &signature2_length);
	zassert_equal(status, PSA_SUCCESS, "second deterministic signing failed, got %d", status);

	zassert_equal(signature1_length, signature2_length, "signature lengths differ");
	zassert_mem_equal(signature1, signature2, signature1_length,
			  "deterministic signatures differ");

	status = psa_verify_message(public_key, PSA_ALG_DETERMINISTIC_ML_DSA, ml_dsa_sign_msg,
				    sizeof(ml_dsa_sign_msg), signature1, signature1_length);
	zassert_equal(status, PSA_SUCCESS, "deterministic signature failed to verify, got %d",
		      status);

	psa_destroy_key(key_pair);
	psa_destroy_key(public_key);
}
