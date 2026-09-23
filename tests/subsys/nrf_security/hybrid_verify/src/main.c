/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <psa/crypto.h>
#include <cracen_psa_hybrid_verify.h>
#include <cracen/statuscodes.h>
#include <internal/ecc/cracen_ecdsa.h>
#include <silexpk/ec_curves.h>
#include <sxsymcrypt/hashdefs.h>
#include <string.h>

#include "ecdsa_vectors.h"
#include "ml_dsa_vectors.h"

#define BENCHMARK_ITERATIONS 10

/* An ML-DSA known-answer test paired with an ECDSA signature over the same
 * message. The curve and the hash algorithm are only needed to verify the
 * ECDSA component on its own in the benchmark.
 */
struct hybrid_profile {
	const char *name;
	const uint8_t *message;
	size_t message_length;
	struct cracen_hybrid_verify_ml_dsa ml_dsa;
	struct cracen_hybrid_verify_ecdsa ecdsa;
	const struct sx_pk_ecurve *curve;
	const struct sxhashalg *hashalg;
	psa_key_id_t ml_dsa_key_id;
};

static struct hybrid_profile profiles[] = {
	{
		.name = "ML-DSA-44 + P-256/SHA-256",
		.message = ml_dsa44_kat_msg,
		.message_length = sizeof(ml_dsa44_kat_msg),
		.ml_dsa = {
			.alg = PSA_ALG_ML_DSA,
			.key_bits = 128,
			.public_key = ml_dsa44_kat_pk,
			.public_key_length = sizeof(ml_dsa44_kat_pk),
			.signature = ml_dsa44_kat_sig,
			.signature_length = sizeof(ml_dsa44_kat_sig),
		},
		.ecdsa = {
			.alg = PSA_ALG_ECDSA(PSA_ALG_SHA_256),
			.family = PSA_ECC_FAMILY_SECP_R1,
			.key_bits = 256,
			.public_key = ecdsa_p256_sha256_pub,
			.public_key_length = sizeof(ecdsa_p256_sha256_pub),
			.signature = ecdsa_p256_sha256_sig,
			.signature_length = sizeof(ecdsa_p256_sha256_sig),
		},
		.curve = &sx_curve_nistp256,
		.hashalg = &sxhashalg_sha2_256,
	},
	{
		.name = "ML-DSA-65 + P-384/SHA-384",
		.message = ml_dsa_kat_msg,
		.message_length = sizeof(ml_dsa_kat_msg),
		.ml_dsa = {
			.alg = PSA_ALG_ML_DSA,
			.key_bits = 192,
			.public_key = ml_dsa_kat_pk,
			.public_key_length = sizeof(ml_dsa_kat_pk),
			.signature = ml_dsa_kat_sig,
			.signature_length = sizeof(ml_dsa_kat_sig),
		},
		.ecdsa = {
			.alg = PSA_ALG_ECDSA(PSA_ALG_SHA_384),
			.family = PSA_ECC_FAMILY_SECP_R1,
			.key_bits = 384,
			.public_key = ecdsa_p384_sha384_pub,
			.public_key_length = sizeof(ecdsa_p384_sha384_pub),
			.signature = ecdsa_p384_sha384_sig,
			.signature_length = sizeof(ecdsa_p384_sha384_sig),
		},
		.curve = &sx_curve_nistp384,
		.hashalg = &sxhashalg_sha2_384,
	},
	{
		/* Digest shorter than the curve operand, so it is padded. */
		.name = "ML-DSA-87 + P-384/SHA-256",
		.message = ml_dsa87_kat_msg,
		.message_length = sizeof(ml_dsa87_kat_msg),
		.ml_dsa = {
			.alg = PSA_ALG_ML_DSA,
			.key_bits = 256,
			.public_key = ml_dsa87_kat_pk,
			.public_key_length = sizeof(ml_dsa87_kat_pk),
			.signature = ml_dsa87_kat_sig,
			.signature_length = sizeof(ml_dsa87_kat_sig),
		},
		.ecdsa = {
			.alg = PSA_ALG_ECDSA(PSA_ALG_SHA_256),
			.family = PSA_ECC_FAMILY_SECP_R1,
			.key_bits = 384,
			.public_key = ecdsa_p384_sha256_pub,
			.public_key_length = sizeof(ecdsa_p384_sha256_pub),
			.signature = ecdsa_p384_sha256_sig,
			.signature_length = sizeof(ecdsa_p384_sha256_sig),
		},
		.curve = &sx_curve_nistp384,
		.hashalg = &sxhashalg_sha2_256,
	},
	{
		/* Digest longer than the curve operand, so it is truncated. */
		.name = "ML-DSA-65 + P-256/SHA-512",
		.message = ml_dsa_kat_msg,
		.message_length = sizeof(ml_dsa_kat_msg),
		.ml_dsa = {
			.alg = PSA_ALG_ML_DSA,
			.key_bits = 192,
			.public_key = ml_dsa_kat_pk,
			.public_key_length = sizeof(ml_dsa_kat_pk),
			.signature = ml_dsa_kat_sig,
			.signature_length = sizeof(ml_dsa_kat_sig),
		},
		.ecdsa = {
			.alg = PSA_ALG_ECDSA(PSA_ALG_SHA_512),
			.family = PSA_ECC_FAMILY_SECP_R1,
			.key_bits = 256,
			.public_key = ecdsa_p256_sha512_pub,
			.public_key_length = sizeof(ecdsa_p256_sha512_pub),
			.signature = ecdsa_p256_sha512_sig,
			.signature_length = sizeof(ecdsa_p256_sha512_sig),
		},
		.curve = &sx_curve_nistp256,
		.hashalg = &sxhashalg_sha2_512,
	},
};

/* Corrupted copies of the vectors under test, kept out of the test stacks. */
static uint8_t ml_dsa_signature_copy[PSA_VENDOR_ML_DSA_MAX_SIGNATURE_BYTES];
static uint8_t ecdsa_signature_copy[PSA_ECDSA_SIGNATURE_SIZE(521)];

static psa_status_t verify_profile(const struct hybrid_profile *profile)
{
	return cracen_hybrid_verify(profile->message, profile->message_length, &profile->ml_dsa,
				    &profile->ecdsa);
}

static void *setup_crypto(void)
{
	psa_status_t status = psa_crypto_init();

	zassert_equal(status, PSA_SUCCESS, "PSA Crypto initialization failed: %d", status);

	for (size_t i = 0; i < ARRAY_SIZE(profiles); i++) {
		psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

		psa_set_key_type(&attributes, PSA_KEY_TYPE_ML_DSA_PUBLIC_KEY);
		psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_MESSAGE);
		psa_set_key_algorithm(&attributes, profiles[i].ml_dsa.alg);

		status = psa_import_key(&attributes, profiles[i].ml_dsa.public_key,
					profiles[i].ml_dsa.public_key_length,
					&profiles[i].ml_dsa_key_id);
		psa_reset_key_attributes(&attributes);
		zassert_equal(status, PSA_SUCCESS, "%s: public-key import failed: %d",
			      profiles[i].name, status);
	}

	return NULL;
}

static void teardown_crypto(void *fixture)
{
	ARG_UNUSED(fixture);

	for (size_t i = 0; i < ARRAY_SIZE(profiles); i++) {
		if (profiles[i].ml_dsa_key_id != PSA_KEY_ID_NULL) {
			(void)psa_destroy_key(profiles[i].ml_dsa_key_id);
			profiles[i].ml_dsa_key_id = PSA_KEY_ID_NULL;
		}
	}
}

ZTEST_SUITE(hybrid_verify, NULL, setup_crypto, NULL, NULL, teardown_crypto);

ZTEST(hybrid_verify, test_valid_signatures)
{
	for (size_t i = 0; i < ARRAY_SIZE(profiles); i++) {
		psa_status_t status = verify_profile(&profiles[i]);

		zassert_equal(status, PSA_SUCCESS, "%s: verification failed: %d",
			      profiles[i].name, status);
	}
}

ZTEST(hybrid_verify, test_invalid_ml_dsa_signature)
{
	for (size_t i = 0; i < ARRAY_SIZE(profiles); i++) {
		struct hybrid_profile profile = profiles[i];

		memcpy(ml_dsa_signature_copy, profile.ml_dsa.signature,
		       profile.ml_dsa.signature_length);
		ml_dsa_signature_copy[0] ^= 1;
		profile.ml_dsa.signature = ml_dsa_signature_copy;

		zassert_equal(verify_profile(&profile), PSA_ERROR_INVALID_SIGNATURE,
			      "%s: invalid ML-DSA signature was accepted", profile.name);
	}
}

ZTEST(hybrid_verify, test_invalid_ecdsa_signature)
{
	for (size_t i = 0; i < ARRAY_SIZE(profiles); i++) {
		struct hybrid_profile profile = profiles[i];

		memcpy(ecdsa_signature_copy, profile.ecdsa.signature,
		       profile.ecdsa.signature_length);
		ecdsa_signature_copy[0] ^= 1;
		profile.ecdsa.signature = ecdsa_signature_copy;

		zassert_equal(verify_profile(&profile), PSA_ERROR_INVALID_SIGNATURE,
			      "%s: invalid ECDSA signature was accepted", profile.name);
	}
}

ZTEST(hybrid_verify, test_both_signatures_invalid)
{
	struct hybrid_profile profile = profiles[1];

	memcpy(ml_dsa_signature_copy, profile.ml_dsa.signature, profile.ml_dsa.signature_length);
	memcpy(ecdsa_signature_copy, profile.ecdsa.signature, profile.ecdsa.signature_length);
	ml_dsa_signature_copy[0] ^= 1;
	ecdsa_signature_copy[0] ^= 1;
	profile.ml_dsa.signature = ml_dsa_signature_copy;
	profile.ecdsa.signature = ecdsa_signature_copy;

	zassert_equal(verify_profile(&profile), PSA_ERROR_INVALID_SIGNATURE,
		      "invalid signatures were accepted");
}

/* The components must cover the same message, so an ECDSA signature over
 * another message must not be accepted just because the ML-DSA component is
 * valid.
 */
ZTEST(hybrid_verify, test_ecdsa_signature_over_other_message)
{
	struct hybrid_profile profile = profiles[2];

	profile.ecdsa = profiles[1].ecdsa;

	zassert_equal(verify_profile(&profile), PSA_ERROR_INVALID_SIGNATURE,
		      "ECDSA signature over another message was accepted");
}

ZTEST(hybrid_verify, test_bad_ecdsa_inputs)
{
	struct hybrid_profile profile = profiles[1];

	profile.ecdsa.signature_length--;
	zassert_equal(verify_profile(&profile), PSA_ERROR_INVALID_ARGUMENT,
		      "bad ECDSA signature length was accepted");

	/* Public key without the uncompressed-format prefix. */
	profile = profiles[1];
	profile.ecdsa.public_key++;
	profile.ecdsa.public_key_length--;
	zassert_equal(verify_profile(&profile), PSA_ERROR_INVALID_ARGUMENT,
		      "ECDSA public key without 0x04 prefix was accepted");

	/* Curve size that does not match the public key. */
	profile = profiles[1];
	profile.ecdsa.key_bits = 256;
	zassert_equal(verify_profile(&profile), PSA_ERROR_INVALID_ARGUMENT,
		      "mismatched ECDSA curve size was accepted");

	/* Algorithm that is not ECDSA. */
	profile = profiles[1];
	profile.ecdsa.alg = PSA_ALG_SHA_384;
	zassert_equal(verify_profile(&profile), PSA_ERROR_INVALID_ARGUMENT,
		      "non-ECDSA algorithm was accepted");
}

ZTEST(hybrid_verify, test_bad_ml_dsa_inputs)
{
	struct hybrid_profile profile = profiles[1];

	profile.ml_dsa.signature_length--;
	zassert_equal(verify_profile(&profile), PSA_ERROR_INVALID_ARGUMENT,
		      "bad ML-DSA signature length was accepted");

	/* Parameter set that does not match the key and signature lengths. */
	profile = profiles[1];
	profile.ml_dsa.key_bits = 256;
	zassert_equal(verify_profile(&profile), PSA_ERROR_INVALID_ARGUMENT,
		      "mismatched ML-DSA parameter set was accepted");

	/* Parameter set that does not exist. */
	profile = profiles[1];
	profile.ml_dsa.key_bits = 512;
	zassert_equal(verify_profile(&profile), PSA_ERROR_NOT_SUPPORTED,
		      "unknown ML-DSA parameter set was accepted");
}

ZTEST(hybrid_verify, test_missing_arguments)
{
	struct hybrid_profile profile = profiles[1];

	zassert_equal(cracen_hybrid_verify(NULL, profile.message_length, &profile.ml_dsa,
					   &profile.ecdsa),
		      PSA_ERROR_INVALID_ARGUMENT, "missing message was accepted");

	zassert_equal(cracen_hybrid_verify(profile.message, profile.message_length, NULL,
					   &profile.ecdsa),
		      PSA_ERROR_INVALID_ARGUMENT, "missing ML-DSA component was accepted");

	zassert_equal(cracen_hybrid_verify(profile.message, profile.message_length,
					   &profile.ml_dsa, NULL),
		      PSA_ERROR_INVALID_ARGUMENT, "missing ECDSA component was accepted");
}

/* A failing ECDSA verification must leave the PKE available for the next one. */
ZTEST(hybrid_verify, test_pke_released_after_failure)
{
	struct hybrid_profile invalid = profiles[1];

	memcpy(ecdsa_signature_copy, invalid.ecdsa.signature, invalid.ecdsa.signature_length);
	ecdsa_signature_copy[0] ^= 1;
	invalid.ecdsa.signature = ecdsa_signature_copy;

	for (size_t i = 0; i < 2; i++) {
		zassert_equal(verify_profile(&invalid), PSA_ERROR_INVALID_SIGNATURE,
			      "invalid iteration %zu returned wrong result", i);
		zassert_equal(verify_profile(&profiles[1]), PSA_SUCCESS,
			      "valid iteration %zu failed after PKE error", i);
	}
}

static void benchmark_profile(const struct hybrid_profile *profile)
{
	uint64_t ecdsa_cycles = 0;
	uint64_t ml_dsa_cycles = 0;
	uint64_t sequential_cycles = 0;
	uint64_t hybrid_cycles = 0;
	uint64_t sequential_average;
	uint64_t hybrid_average;
	uint64_t reduction_hundredths = 0;
	size_t unused_stack;
	psa_status_t psa_status;
	int sx_status;

	for (size_t i = 0; i < BENCHMARK_ITERATIONS; i++) {
		uint64_t start = k_cycle_get_64();

		sx_status = cracen_ecdsa_verify_message(profile->ecdsa.public_key + 1,
						       profile->hashalg, profile->message,
						       profile->message_length, profile->curve,
						       profile->ecdsa.signature);
		ecdsa_cycles += k_cycle_get_64() - start;
		zassert_equal(sx_status, SX_OK, "ECDSA component failed: %d", sx_status);

		start = k_cycle_get_64();
		psa_status = psa_verify_message(profile->ml_dsa_key_id, profile->ml_dsa.alg,
						profile->message, profile->message_length,
						profile->ml_dsa.signature,
						profile->ml_dsa.signature_length);
		ml_dsa_cycles += k_cycle_get_64() - start;
		zassert_equal(psa_status, PSA_SUCCESS, "ML-DSA component failed: %d", psa_status);

		start = k_cycle_get_64();
		sx_status = cracen_ecdsa_verify_message(profile->ecdsa.public_key + 1,
						       profile->hashalg, profile->message,
						       profile->message_length, profile->curve,
						       profile->ecdsa.signature);
		psa_status = psa_verify_message(profile->ml_dsa_key_id, profile->ml_dsa.alg,
						profile->message, profile->message_length,
						profile->ml_dsa.signature,
						profile->ml_dsa.signature_length);
		sequential_cycles += k_cycle_get_64() - start;
		zassert_equal(sx_status, SX_OK, "sequential ECDSA failed: %d", sx_status);
		zassert_equal(psa_status, PSA_SUCCESS, "sequential ML-DSA failed: %d", psa_status);

		start = k_cycle_get_64();
		zassert_equal(verify_profile(profile), PSA_SUCCESS,
			      "hybrid verification failed");
		hybrid_cycles += k_cycle_get_64() - start;
	}

	sequential_average = sequential_cycles / BENCHMARK_ITERATIONS;
	hybrid_average = hybrid_cycles / BENCHMARK_ITERATIONS;
	if (sequential_average > hybrid_average) {
		reduction_hundredths =
			(sequential_average - hybrid_average) * 10000 / sequential_average;
	}

	printk("hybrid_verify %s: ecdsa_avg_us=%llu ml_dsa_avg_us=%llu\n", profile->name,
	       (unsigned long long)k_cyc_to_us_floor64(ecdsa_cycles / BENCHMARK_ITERATIONS),
	       (unsigned long long)k_cyc_to_us_floor64(ml_dsa_cycles / BENCHMARK_ITERATIONS));
	printk("hybrid_verify %s: sequential_avg_us=%llu hybrid_avg_us=%llu reduction=%llu.%02llu%%"
	       " iterations=%d\n", profile->name,
	       (unsigned long long)k_cyc_to_us_floor64(sequential_average),
	       (unsigned long long)k_cyc_to_us_floor64(hybrid_average),
	       (unsigned long long)(reduction_hundredths / 100),
	       (unsigned long long)(reduction_hundredths % 100), BENCHMARK_ITERATIONS);

	if (k_thread_stack_space_get(k_current_get(), &unused_stack) == 0) {
		printk("hybrid_verify %s: unused_test_stack_bytes=%zu\n", profile->name,
		       unused_stack);
	}
}

ZTEST(hybrid_verify, test_benchmark)
{
	for (size_t i = 0; i < ARRAY_SIZE(profiles); i++) {
		benchmark_profile(&profiles[i]);
	}
}
