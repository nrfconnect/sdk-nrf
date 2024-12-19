/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <string.h>
#include <sxsymcrypt/sha2.h>
#include <silexpk/ed25519.h>
#include <cracen/ec_helpers.h>
#include <sxsymcrypt/hash.h>
#include <cracen/mem_helpers.h>

int ed25519_sign(const uint8_t *ed25519, char *signature, const uint8_t *message)
{
	char workmem[160];
	struct sxhash hashopctx;
	char pointr_buffer[64];
	char *pointr = pointr_buffer;
	int r;

	/*Hash the private key, the digest is stored in the first 64 bytes of workmem*/
	r = sx_hash_create(&hashopctx, &sxhashalg_sha2_512, sizeof(hashopctx));
	if (r != 0) {
		return r;
	}
	r = sx_hash_feed(&hashopctx, ed25519, 32);
	if (r != 0) {
		return r;
	}
	r = sx_hash_digest(&hashopctx, workmem);
	if (r != 0) {
		return r;
	}
	/*Wait to make sure hash operation is finished before doing calculations on it*/
	r = sx_hash_wait(&hashopctx);
	if (r != 0) {
		return r;
	}
	/* Obtain r by hashing (prefix || message), where prefix is the second
	 * half of the private key digest.
	 */

	r = sx_hash_create(&hashopctx, &sxhashalg_sha2_512, sizeof(hashopctx));
	if (r != 0) {
		return r;
	}
	r = sx_hash_feed(&hashopctx, workmem+32, 32);
	if (r != 0) {
		return r;
	}
	r = sx_hash_feed(&hashopctx, message, 100);
	if (r != 0) {
		return r;
	}
	r = sx_hash_digest(&hashopctx, workmem+96);
	if (r != 0) {
		return r;
	}

	/* Perform point multiplication R = [r]B. This is the encoded point R,
	 * which is the first part of the signature.
	 */
	r = sx_ed25519_ptmult((const struct sx_ed25519_dgst *)(workmem + 96),
						(struct sx_ed25519_pt *)pointr);
	if (r != 0) {
		return r;

	}
	/* The secret scalar s is computed in place from the first half of the
	 * private key digest.
	 */
	decode_scalar_25519(workmem);

	/* Clear second half of private key digest: sx_async_ed25519_ptmult_go()
	 * works on an input of SX_ED25519_DGST_SZ bytes.
	 */
	safe_memset(workmem + 32, 128, 0, 32);

	/* Perform point multiplication A = [s]B, to obtain the public key A.
	 * Which is stored in workmem[32:63]
	 */
	r = sx_ed25519_ptmult((const struct sx_ed25519_dgst *)(workmem),
		(struct sx_ed25519_pt *)(struct sx_ed25519_pt *)((char *)workmem + 32));
	if (r != 0) {
		return r;
	}

	/* Obtain k by hashing (R || A || message). */
	r = sx_hash_create(&hashopctx, &sxhashalg_sha2_512, sizeof(hashopctx));
	if (r != 0) {
		return r;
	}
	r = sx_hash_feed(&hashopctx, pointr, 32);
	if (r != 0) {
		return r;
	}
	r = sx_hash_feed(&hashopctx, workmem+32, 32);
	if (r != 0) {
		return r;
	}
	r = sx_hash_feed(&hashopctx, message, 100);
	if (r != 0) {
		return r;
	}
	r = sx_hash_digest(&hashopctx, workmem+32);
	if (r != 0) {
		return r;
	}

	/* Compute (r + k * s) mod L. This gives the second part of the
	 * signature, which is the encoded S which is stored in pointr.
	 */

	r = sx_ed25519_sign(
		(const struct sx_ed25519_dgst *)(workmem + 32),
		(const struct sx_ed25519_dgst *)(workmem + 3 * 32),
		(const struct sx_ed25519_v *)workmem,
		(struct sx_ed25519_v *)(pointr + SX_ED25519_PT_SZ));
	if (r != 0) {
		return r;
	}

	memcpy(signature, pointr, 64);

	return r;
}

int ed25519_verify(const uint8_t *pubkey, const char *message,
				size_t message_length, const char *signature)
{

	char workmem[64];
	struct sxhash hashopctx;
	int r;


	r = sx_hash_create(&hashopctx, &sxhashalg_sha2_512, sizeof(hashopctx));
	if (r != 0) {
		return r;
	}
	r = sx_hash_feed(&hashopctx, signature, 32);
	if (r != 0) {
		return r;
	}
	r = sx_hash_feed(&hashopctx, pubkey, 32);
	if (r != 0) {
		return r;
	}
	r = sx_hash_feed(&hashopctx, message, message_length);
	if (r != 0) {
		return r;
	}
	r = sx_hash_digest(&hashopctx, workmem);
	if (r != 0) {
		return r;
	}

	r = sx_ed25519_verify(
		(struct sx_ed25519_dgst *)workmem,
		(struct sx_ed25519_pt *)pubkey,
		(const struct sx_ed25519_v *)(signature + 32),
		(const struct sx_ed25519_pt *)signature);

	return r;
}

int create_ed25519_pubkey(const uint8_t *ed25519,
				       uint8_t *pubkey)
{
	char workmem[64];
	struct sxhash hashopctx;
	int r;

	r = sx_hash_create(&hashopctx, &sxhashalg_sha2_512, sizeof(hashopctx));
	if (r != 0) {
		return r;
	}
	r = sx_hash_feed(&hashopctx, ed25519, 32);
	if (r != 0) {
		return r;
	}
	r = sx_hash_digest(&hashopctx, workmem);
	if (r != 0) {
		return r;
		}
	/*Wait to make sure hash operation is finished before doing calculations on it
	 * removing this breaks pubkey generation
	 */
	r = sx_hash_wait(&hashopctx);
	if (r != 0) {
		return r;
	}
	/* The secret scalar s is computed in place from the first half of the
	 * private key digest.
	 */
	decode_scalar_25519(workmem);

	/* Clear second half of private key digest: sx_async_ed25519_ptmult_go()
	 * works on an input of SX_ED25519_DGST_SZ bytes.
	 */
	safe_memset(workmem+32, 32, 0, 32);
	/* Perform point multiplication A = [s]B, to obtain the public key A. */
	r = sx_ed25519_ptmult((const struct sx_ed25519_dgst *)(workmem),
		(struct sx_ed25519_pt *)(struct sx_ed25519_pt *)((char *)workmem + 32));
	if (r != 0) {
		return r;
	}
	memcpy(pubkey, workmem+32, 32);

	return r;
}
