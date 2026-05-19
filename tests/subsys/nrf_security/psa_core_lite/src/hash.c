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

#define SHA256_HASH_SIZE	(32)
#define SHA384_HASH_SIZE	(48)
#define SHA512_HASH_SIZE	(64)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif


/* Test string without null-termination (6 octets)*/
static uint8_t ecdsa_secp256r1_msg[6] = "sample";

/* SHA-256 hash of the string "sample" (6 octets)*/
static uint8_t ecdsa_secp256r1_hash[SHA256_HASH_SIZE] = {
	0xaf, 0x2b, 0xdb, 0xe1, 0xaa, 0x9b, 0x6e, 0xc1, 0xe2, 0xad, 0xe1, 0xd6,
	0x94, 0xf4, 0x1f, 0xc7, 0x1a, 0x83, 0x1d, 0x02, 0x68, 0xe9, 0x89, 0x15,
	0x62, 0x11, 0x3d, 0x8a, 0x62, 0xad, 0xd1, 0xbf
};

/* Test string without null-termination (6 octets)*/
static uint8_t ecdsa_secp384r1_msg[6] = "sample";

/* SHA-384 hash of the string "sample" (6 octets)*/
static uint8_t ecdsa_secp384r1_hash[SHA384_HASH_SIZE] = {
	0x9a, 0x90, 0x83, 0x50, 0x5b, 0xc9, 0x22, 0x76, 0xae, 0xc4, 0xbe, 0x31,
	0x26, 0x96, 0xef, 0x7b, 0xf3, 0xbf, 0x60, 0x3f, 0x4b, 0xbd, 0x38, 0x11,
	0x96, 0xa0, 0x29, 0xf3, 0x40, 0x58, 0x53, 0x12, 0x31, 0x3b, 0xca, 0x4a,
	0x9b, 0x5b, 0x89, 0x0e, 0xfe, 0xe4, 0x2c, 0x77, 0xb1, 0xee, 0x25, 0xfe
};

/* Ed25518ph test message (used only by hash test) */
static uint8_t ed25519ph_msg[3] = {
	0x61, 0x62, 0x63
};

/* SHA-512 Hash of message '0x61, 0x62, 0x63' (hex input, 3 octets)*/
static uint8_t ed25519ph_hash[SHA512_HASH_SIZE] = {
	0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba, 0xcc, 0x41, 0x73, 0x49,
	0xae, 0x20, 0x41, 0x31, 0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2,
	0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a, 0x21, 0x92, 0x99, 0x2a,
	0x27, 0x4f, 0xc1, 0xa8, 0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
	0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e, 0x2a, 0x9a, 0xc9, 0x4f,
	0xa5, 0x4c, 0xa4, 0x9f
};

static void test_hash_compute(psa_algorithm_t alg, const uint8_t *input, size_t input_length,
			      uint8_t *hash, size_t hash_size)
{
	psa_status_t err;
	size_t hash_length;
	uint8_t calculated_hash[SHA512_HASH_SIZE];

	err = psa_hash_compute(alg, input, input_length, calculated_hash, hash_size, &hash_length);
	zassert_equal(err, PSA_SUCCESS, "Failed to hash Alg: %d, Err: %d", alg, err);
	zassert_equal(hash_size, hash_length, "Hash size mismatch. Got %d, expected %d",
		      hash_length, hash_size);

	if (constant_memcmp(calculated_hash, hash, hash_size) != 0) {
		zassert_false(true, "Hash calculate mismatched results");
	}
}

static void test_hash_incremental(psa_algorithm_t alg, const uint8_t *input, size_t input_length,
				  uint8_t *hash, size_t hash_size)
{
	psa_status_t err;
	size_t hash_length;
	uint8_t calculated_hash[SHA512_HASH_SIZE];
	psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;

	err = psa_hash_setup(&operation, alg);
	zassert_equal(err, PSA_SUCCESS, "Failed to setup hash Alg: %d, Err: %d", alg, err);

	err = psa_hash_update(&operation, input, input_length);
	zassert_equal(err, PSA_SUCCESS, "Failed to update hash Alg: %d, Err: %d", alg, err);

	err = psa_hash_finish(&operation, calculated_hash, hash_size, &hash_length);
	zassert_equal(err, PSA_SUCCESS, "Failed to finish hash Alg: %d, Err: %d", alg, err);

	err = psa_hash_abort(&operation);
	zassert_equal(err, PSA_SUCCESS, "Failed to abort hash Alg: %d, Err: %d", alg, err);

	zassert_equal(hash_size, hash_length, "Hash size mismatch. Got %d, expected %d",
		      hash_length, hash_size);

	if (constant_memcmp(calculated_hash, hash, hash_size) != 0) {
		zassert_false(true, "Hash incremental mismathed results");
	}
}


void test_hash(void)
{
	bool ran_hash = false;

	if (IS_ENABLED(PSA_WANT_ALG_SHA_256)) {
		psa_algorithm_t alg = PSA_ALG_SHA_256;

		test_hash_compute(alg, ecdsa_secp256r1_msg, ARRAY_SIZE(ecdsa_secp256r1_msg),
				  ecdsa_secp256r1_hash, SHA256_HASH_SIZE);


		test_hash_incremental(alg, ecdsa_secp256r1_msg, ARRAY_SIZE(ecdsa_secp256r1_msg),
				  ecdsa_secp256r1_hash, SHA256_HASH_SIZE);

		ran_hash = true;
	}

	if (IS_ENABLED(PSA_WANT_ALG_SHA_384)) {
		psa_algorithm_t alg = PSA_ALG_SHA_384;

		test_hash_compute(alg, ecdsa_secp384r1_msg, ARRAY_SIZE(ecdsa_secp384r1_msg),
				  ecdsa_secp384r1_hash, SHA384_HASH_SIZE);


		test_hash_incremental(alg, ecdsa_secp384r1_msg, ARRAY_SIZE(ecdsa_secp384r1_msg),
				  ecdsa_secp384r1_hash, SHA384_HASH_SIZE);

		ran_hash = true;
	}

	if (IS_ENABLED(PSA_WANT_ALG_SHA_512)) {
		psa_algorithm_t alg = PSA_ALG_SHA_512;

		test_hash_compute(alg, ed25519ph_msg, ARRAY_SIZE(ed25519ph_msg),
				  ed25519ph_hash, SHA512_HASH_SIZE);

		test_hash_incremental(alg, ed25519ph_msg, ARRAY_SIZE(ed25519ph_msg),
				  ed25519ph_hash, SHA512_HASH_SIZE);

		ran_hash = true;
	}

	zassert_true(ran_hash, "Did not run any hash calculation, check configs!");
}
