/* HMAC implementation, based on FIPS 198-1.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout for the HMAC operation:
 *      1. Area used to store values obtained from processing the key.
 *         Size: hash algorithm block size.
 *      2. Area used to store the intermediate hash value.
 *         Size: hash algorithm digest size.
 */

#include <string.h>
#include <sxsymcrypt/hash.h>
#include <cracen/statuscodes.h>
#include <cracen/mem_helpers.h>
#include "cracen_psa_primitives.h"
#include "cracen_psa.h"
#include "common.h"

/* xor bytes in 'buf' with a constant 'v', in place */
static void xorbuf(uint8_t *buf, uint8_t v, size_t sz)
{
	for (size_t i = 0; i < sz; i++) {
		*buf = *buf ^ v;
		buf++;
	}
}

int hmac_produce(struct sxhash *hashctx, const struct sxhashalg *hashalg, uint8_t *digest,
		 size_t output_buffer_size, uint8_t *workmem)
{
	int status;
	size_t blocksz;
	size_t digestsz;

	digestsz = sx_hash_get_alg_digestsz(hashalg);
	if (output_buffer_size < digestsz) {
		sx_hash_free(hashctx);
		return SX_ERR_OUTPUT_BUFFER_TOO_SMALL;
	}

	blocksz = sx_hash_get_alg_blocksz(hashalg);

	/* run hash (1st hash of HMAC algorithm), result inside workmem */
	status = sx_hash_digest(hashctx, workmem + blocksz);
	if (status != SX_OK) {
		return status;
	}

	status = sx_hash_wait(hashctx);
	if (status != SX_OK) {
		return status;
	}

	/* compute K0 xor opad (0x36 ^ 0x5C = 0x6A) */
	xorbuf(workmem, 0x6A, blocksz);

	return cracen_hash_input_with_context(hashctx, workmem, blocksz + digestsz, hashalg,
					      digest);
}

static int internal_start_hmac_computation(struct sxhash *hashopctx,
					   const struct sxhashalg *hashalg, uint8_t *workmem)
{
	int status;
	size_t blocksz;

	/* Create hash context for computing the 1st hash in the HMAC algorithm */
	status = sx_hash_create(hashopctx, hashalg, sizeof(*hashopctx));
	if (status != SX_OK) {
		return status;
	}

	blocksz = sx_hash_get_alg_blocksz(hashalg);

	/* The "prepared" key (called K0 in FIPS 198-1) is available and
	 * workmem points to it. Here we compute K0 xor ipad.
	 */
	xorbuf(workmem, 0x36, blocksz);

	/* start feeding the hash operation */
	status = sx_hash_feed(hashopctx, workmem, blocksz);

	return status;
}

int mac_create_hmac(const struct sxhashalg *hashalg, struct sxhash *hashopctx, const uint8_t *key,
		    size_t keysz, uint8_t *workmem, size_t workmemsz)
{
	int status;
	size_t digestsz;
	size_t blocksz;

	digestsz = sx_hash_get_alg_digestsz(hashalg);
	blocksz = sx_hash_get_alg_blocksz(hashalg);

	/* a key longer than hash block size needs an additional preparation
	 * step
	 */
	if (keysz > blocksz) {
		/* hash the key */
		status = cracen_hash_input_with_context(hashopctx, key, keysz, hashalg, workmem);
		if (status != SX_OK) {
			return status;
		}

		/* zero padding */
		safe_memset(workmem + digestsz, workmemsz - digestsz, 0, blocksz - digestsz);
		status = internal_start_hmac_computation(hashopctx, hashalg, workmem);
	} else {
		memcpy(workmem, key, keysz);
		/* zero padding */
		safe_memset(workmem + keysz, workmemsz - keysz, 0, blocksz - keysz);

		status = internal_start_hmac_computation(hashopctx, hashalg, workmem);
	}
	return status;
}
