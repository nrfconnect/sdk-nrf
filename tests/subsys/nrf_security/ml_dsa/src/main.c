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

/** Positive known-answer test against a NIST ACVP sigVer vector. */
ZTEST(ml_dsa_verify, test_kat_valid_signature)
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
