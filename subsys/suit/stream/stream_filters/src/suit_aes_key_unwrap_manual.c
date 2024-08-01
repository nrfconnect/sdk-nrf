/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include "suit_aes_key_unwrap_manual.h"

#define KW_BLOCK_SIZE_BITS  64
#define KW_BLOCK_SIZE_BYTES 8

#define MAX_KEY_SIZE_BITS  256
#define MAX_KEY_SIZE_BYTES 32
/** The maximum key size in bits is 256, so the maximum n used in the algorithm
 *  is 4 (256/64).
 */
#define MAX_N		   4

/*
 * Use pointer to volatile function, as stated in Percival's blog article at:
 *
 * http://www.daemonology.net/blog/2014-09-04-how-to-zero-a-buffer.html
 *
 * Although some compilers may still optimize out the memset, it is safer to use
 * some sort of trick than simply call memset.
 */
static void *(*const volatile memset_func)(void *, int, size_t) = memset;

static void zeroize(void *buf, size_t len)
{
	if ((buf == NULL) || (len == 0)) {
		return;
	}

	memset_func(buf, 0, len);
}

/* Initial vector used by the AES KW algorithm, see RFC3394 chapter 2.2.3.1*/
const static uint8_t KW_IV[KW_BLOCK_SIZE_BYTES] = {0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6};

static psa_status_t import_cek(const uint8_t *cek, size_t cek_length_bits,
			       psa_key_type_t cek_key_type, psa_algorithm_t cek_key_alg,
			       psa_key_id_t *cek_key_id)
{
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, cek_key_alg);
	psa_set_key_type(&key_attributes, cek_key_type);
	psa_set_key_bits(&key_attributes, cek_length_bits);

	return psa_import_key(&key_attributes, cek, cek_length_bits / 8, cek_key_id);
}

psa_status_t suit_aes_key_unwrap_manual(psa_key_id_t kek_key_id, const uint8_t *wrapped_cek,
					size_t cek_bits, psa_key_type_t cek_key_type,
					psa_algorithm_t cek_key_alg,
					psa_key_id_t *unwrapped_cek_key_id)
{
	psa_status_t status = PSA_SUCCESS;
	uint8_t unwrapped_cek[MAX_KEY_SIZE_BYTES] = {0};
	uint8_t cmp_ret = 0;

	/* TODO: verify wrapped_cek is of correct length */

	/* The variable names are matching the names from RFC3394,
	 * chapter 2.2.2 (second algorithm variant)
	 */
	size_t n = cek_bits / KW_BLOCK_SIZE_BITS;
	uint8_t t = 0;			      /* Maximum value for t is 24 -> can be uint8_t */
	uint8_t A[KW_BLOCK_SIZE_BYTES] = {0}; /* A is a 64 bit value */
	uint8_t B[2 * KW_BLOCK_SIZE_BYTES] = {0};
	/* Need n + 1 indices of .
	 * An additional 1 is added to allow indexing R from 1 as in the specification.
	 */
	uint8_t R[(MAX_N + 1) + 1][KW_BLOCK_SIZE_BYTES] = {{0}};

	uint8_t temp[2 * KW_BLOCK_SIZE_BYTES] = {0};
	size_t temp_output_length = 0;

	/* Initialize the variables */
	memcpy(A, wrapped_cek, KW_BLOCK_SIZE_BYTES);

	for (size_t i = 1; i <= n; i++) {
		wrapped_cek += KW_BLOCK_SIZE_BYTES;
		memcpy(R[i], wrapped_cek, KW_BLOCK_SIZE_BYTES);
	}

	/* Compute intermediate values */
	for (int j = 5; j >= 0; j--) {
		for (size_t i = n; i > 0; i--) {
			t = n * j + i;
			/* B = AES-1(K, (A ^ t) | R[i])
			 * Only the LSB will be affected by the xor, as t is always smaller than 256
			 */
			A[7] = A[7] ^ t;
			memcpy(temp, A, KW_BLOCK_SIZE_BYTES);
			memcpy(temp + KW_BLOCK_SIZE_BYTES, R[i], KW_BLOCK_SIZE_BYTES);
			status =
				psa_cipher_decrypt(kek_key_id, PSA_ALG_ECB_NO_PADDING, temp,
						   sizeof(temp), B, sizeof(B), &temp_output_length);

			if (status != PSA_SUCCESS) {
				goto cleanup;
			}

			if (temp_output_length != sizeof(B)) {
				status = PSA_ERROR_GENERIC_ERROR;
				goto cleanup;
			}

			/* A = MSB(64, B) */
			memcpy(A, B, KW_BLOCK_SIZE_BYTES);
			/* R[i] = LSB(64, B) */
			memcpy(R[i], B + KW_BLOCK_SIZE_BYTES, KW_BLOCK_SIZE_BYTES);
		}
	}

	for (size_t i = 0; i < KW_BLOCK_SIZE_BYTES; i++) {
		cmp_ret |= A[i] ^ KW_IV[i];
	}

	if (cmp_ret != 0) {
		status = PSA_ERROR_INVALID_SIGNATURE;
		goto cleanup;
	}

	for (size_t i = 0; i < n; i++) {
		memcpy(unwrapped_cek + KW_BLOCK_SIZE_BYTES * i, R[i + 1], KW_BLOCK_SIZE_BYTES);
	}

	status = import_cek(unwrapped_cek, cek_bits, cek_key_type, cek_key_alg,
			    unwrapped_cek_key_id);

cleanup:
	zeroize(unwrapped_cek, sizeof(unwrapped_cek));
	zeroize(A, sizeof(A));
	zeroize(B, sizeof(B));
	zeroize(R, sizeof(R));
	zeroize(temp, sizeof(temp));

	return status;
}
