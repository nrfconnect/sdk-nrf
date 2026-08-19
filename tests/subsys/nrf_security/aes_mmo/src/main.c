/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * AES-MMO known-answer tests against the PSA API.
 *
 * PSA_ALG_AES_MMO_ZIGBEE (Zigbee specification, clause B.6) is a Merkle-Damgard
 * hash over a Matyas-Meyer-Oseas compression function built from AES-128. CRACEN
 * has no hardware mode for it, so the driver runs one hardware AES-ECB operation
 * per message block and does the chaining in software. The chaining and the
 * padding are therefore what these tests aim at: the multipart tests split the
 * same message at every offset that can leave the block buffer in a different
 * state, and the vectors cover each branch of the padding rule.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <psa/crypto.h>
#include <string.h>

#include "aes_mmo_vectors.h"

/* The longest message in the corpus, allocated once instead of on the stack. */
static uint8_t message[AES_MMO_MAX_MESSAGE_LENGTH];

static void *setup_crypto(void)
{
	zassert_equal(psa_crypto_init(), PSA_SUCCESS, "PSA Crypto initialization failed");
	return NULL;
}

ZTEST_SUITE(aes_mmo_psa, NULL, setup_crypto, NULL, NULL, NULL);

/* The one vector with an external source: the Zigbee install code example. */
ZTEST(aes_mmo_psa, test_install_code)
{
	uint8_t digest[AES_MMO_DIGEST_SIZE];
	size_t digest_length = 0;
	psa_status_t status;

	status = psa_hash_compute(PSA_ALG_AES_MMO_ZIGBEE, aes_mmo_install_code,
				  sizeof(aes_mmo_install_code), digest, sizeof(digest),
				  &digest_length);

	zassert_equal(status, PSA_SUCCESS, "psa_hash_compute failed: %d", (int)status);
	zassert_equal(digest_length, AES_MMO_DIGEST_SIZE, "unexpected digest length: %u",
		      (unsigned int)digest_length);
	zassert_mem_equal(digest, aes_mmo_install_code_digest, AES_MMO_DIGEST_SIZE,
			  "install code digest mismatch");
}

ZTEST(aes_mmo_psa, test_single_part)
{
	for (size_t i = 0; i < ARRAY_SIZE(aes_mmo_vectors); i++) {
		const struct aes_mmo_vector *v = &aes_mmo_vectors[i];
		uint8_t digest[AES_MMO_DIGEST_SIZE];
		size_t digest_length = 0;
		psa_status_t status;

		aes_mmo_fill_pattern(message, v->len);

		status = psa_hash_compute(PSA_ALG_AES_MMO_ZIGBEE, message, v->len, digest,
					  sizeof(digest), &digest_length);

		zassert_equal(status, PSA_SUCCESS, "len %u: psa_hash_compute failed: %d",
			      (unsigned int)v->len, (int)status);
		zassert_equal(digest_length, AES_MMO_DIGEST_SIZE, "len %u: digest length %u",
			      (unsigned int)v->len, (unsigned int)digest_length);
		zassert_mem_equal(digest, v->digest, AES_MMO_DIGEST_SIZE, "len %u: digest mismatch",
				  (unsigned int)v->len);
	}
}

/* Feeding the message in fragments must not change the digest, whichever way the
 * fragments straddle the 16-byte block boundary.
 */
ZTEST(aes_mmo_psa, test_multipart)
{
	static const size_t chunk_sizes[] = {1, 2, 7, 15, 16, 17, 31, 32};

	for (size_t i = 0; i < ARRAY_SIZE(aes_mmo_vectors); i++) {
		const struct aes_mmo_vector *v = &aes_mmo_vectors[i];

		aes_mmo_fill_pattern(message, v->len);

		for (size_t c = 0; c < ARRAY_SIZE(chunk_sizes); c++) {
			psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;
			uint8_t digest[AES_MMO_DIGEST_SIZE];
			size_t digest_length = 0;
			size_t offset = 0;
			psa_status_t status;

			status = psa_hash_setup(&operation, PSA_ALG_AES_MMO_ZIGBEE);
			zassert_equal(status, PSA_SUCCESS, "psa_hash_setup failed: %d",
				      (int)status);

			while (offset < v->len) {
				size_t chunk = MIN(chunk_sizes[c], v->len - offset);

				status = psa_hash_update(&operation, message + offset, chunk);
				zassert_equal(status, PSA_SUCCESS,
					      "len %u chunk %u: update failed: %d",
					      (unsigned int)v->len, (unsigned int)chunk_sizes[c],
					      (int)status);
				offset += chunk;
			}

			status = psa_hash_finish(&operation, digest, sizeof(digest),
						 &digest_length);
			zassert_equal(status, PSA_SUCCESS, "len %u chunk %u: finish failed: %d",
				      (unsigned int)v->len, (unsigned int)chunk_sizes[c],
				      (int)status);
			zassert_mem_equal(digest, v->digest, AES_MMO_DIGEST_SIZE,
					  "len %u chunk %u: digest mismatch", (unsigned int)v->len,
					  (unsigned int)chunk_sizes[c]);
		}
	}
}

/* A cloned operation must continue from the same state as the original. */
ZTEST(aes_mmo_psa, test_clone)
{
	psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;
	psa_hash_operation_t clone = PSA_HASH_OPERATION_INIT;
	uint8_t digest[AES_MMO_DIGEST_SIZE];
	uint8_t clone_digest[AES_MMO_DIGEST_SIZE];
	size_t digest_length = 0;
	psa_status_t status;

	/* Split so that the clone starts with a partially filled block. */
	const size_t split = 20;
	const size_t len = 33;

	aes_mmo_fill_pattern(message, len);

	zassert_equal(psa_hash_setup(&operation, PSA_ALG_AES_MMO_ZIGBEE), PSA_SUCCESS);
	zassert_equal(psa_hash_update(&operation, message, split), PSA_SUCCESS);

	status = psa_hash_clone(&operation, &clone);
	zassert_equal(status, PSA_SUCCESS, "psa_hash_clone failed: %d", (int)status);

	zassert_equal(psa_hash_update(&operation, message + split, len - split), PSA_SUCCESS);
	zassert_equal(psa_hash_finish(&operation, digest, sizeof(digest), &digest_length),
		      PSA_SUCCESS);

	zassert_equal(psa_hash_update(&clone, message + split, len - split), PSA_SUCCESS);
	zassert_equal(psa_hash_finish(&clone, clone_digest, sizeof(clone_digest), &digest_length),
		      PSA_SUCCESS);

	zassert_mem_equal(digest, clone_digest, AES_MMO_DIGEST_SIZE, "clone digest mismatch");
}

ZTEST(aes_mmo_psa, test_compare)
{
	uint8_t corrupted[AES_MMO_DIGEST_SIZE];
	psa_status_t status;

	status = psa_hash_compare(PSA_ALG_AES_MMO_ZIGBEE, aes_mmo_install_code,
				  sizeof(aes_mmo_install_code), aes_mmo_install_code_digest,
				  AES_MMO_DIGEST_SIZE);
	zassert_equal(status, PSA_SUCCESS, "psa_hash_compare failed: %d", (int)status);

	memcpy(corrupted, aes_mmo_install_code_digest, sizeof(corrupted));
	corrupted[AES_MMO_DIGEST_SIZE - 1] ^= 0x01;

	status = psa_hash_compare(PSA_ALG_AES_MMO_ZIGBEE, aes_mmo_install_code,
				  sizeof(aes_mmo_install_code), corrupted, sizeof(corrupted));
	zassert_equal(status, PSA_ERROR_INVALID_SIGNATURE,
		      "corrupted digest was accepted: %d", (int)status);
}

ZTEST(aes_mmo_psa, test_output_buffer_too_small)
{
	uint8_t digest[AES_MMO_DIGEST_SIZE - 1];
	size_t digest_length = 0;
	psa_status_t status;

	status = psa_hash_compute(PSA_ALG_AES_MMO_ZIGBEE, aes_mmo_install_code,
				  sizeof(aes_mmo_install_code), digest, sizeof(digest),
				  &digest_length);

	zassert_equal(status, PSA_ERROR_BUFFER_TOO_SMALL, "small buffer accepted: %d",
		      (int)status);
}

/* Zigbee clause B.6 encodes the message length in 16 bits, so longer messages are
 * outside the domain of the algorithm and must be rejected rather than wrapped.
 */
ZTEST(aes_mmo_psa, test_message_too_long)
{
	psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;
	psa_status_t status;

	memset(message, 0, sizeof(message));

	zassert_equal(psa_hash_setup(&operation, PSA_ALG_AES_MMO_ZIGBEE), PSA_SUCCESS);
	zassert_equal(psa_hash_update(&operation, message, AES_MMO_MAX_MESSAGE_LENGTH),
		      PSA_SUCCESS);

	status = psa_hash_update(&operation, message, 1);
	zassert_equal(status, PSA_ERROR_INVALID_ARGUMENT, "over-long message accepted: %d",
		      (int)status);

	psa_hash_abort(&operation);
}
