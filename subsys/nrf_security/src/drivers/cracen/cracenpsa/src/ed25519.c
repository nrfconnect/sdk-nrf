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

/* This is the ASCII string with the
 * PHflag 1 and context size 0 appended as defined in:
 * https://datatracker.ietf.org/doc/html/rfc8032.html#section-2
 * used for domain seperation between Ed25519 and Ed25519ph
 */
const char dom2[34] = {
	0x53, 0x69, 0x67, 0x45, 0x64, 0x32, 0x35,
	0x35, 0x31, 0x39, 0x20, 0x6e, 0x6f, 0x20,
	0x45, 0x64, 0x32, 0x35, 0x35, 0x31, 0x39,
	0x20, 0x63, 0x6f, 0x6c, 0x6c, 0x69, 0x73,
	0x69, 0x6f, 0x6e, 0x73, 0x01, 0x00
};

int ed25519_hash_message(const char *message,  size_t message_length, char *workmem)
{
	int status;
	struct sxhash hashopctx;

	status = sx_hash_create(&hashopctx, &sxhashalg_sha2_512, sizeof(hashopctx));

	if (status != 0) {
		return status;
	}
	status = sx_hash_feed(&hashopctx, message, message_length);
	if (status != 0) {
		return status;
	}
	status = sx_hash_digest(&hashopctx, workmem);
	if (status != 0) {
		return status;
	}
	status = sx_hash_wait(&hashopctx);

	return status;

}

int ed25519_hash_privkey(struct sxhash hashopctx, char *workmem, const uint8_t *priv_key)
{

	int status;
	/*Hash the private key, the digest is stored in the first 64 bytes of workmem*/
	status = sx_hash_create(&hashopctx, &sxhashalg_sha2_512, sizeof(hashopctx));
	if (status != 0) {
		return status;
	}
	status = sx_hash_feed(&hashopctx, priv_key, SX_ED25519_SZ);
	if (status != 0) {
		return status;
	}
	status = sx_hash_digest(&hashopctx, workmem);
	if (status != 0) {
		return status;
	}
	/*Wait to make sure hash operation is finished before doing calculations on it*/
	status = sx_hash_wait(&hashopctx);

	return status;
}


int ed25519_calculate_r(struct sxhash hashopctx, char *workmem, const uint8_t *message,
			size_t message_length, int prehash)
{
	int status;

	status = sx_hash_create(&hashopctx, &sxhashalg_sha2_512, sizeof(hashopctx));
	if (status != 0) {
		return status;
	}
	if (prehash) {
		status = sx_hash_feed(&hashopctx, dom2, 34);
		if (status != 0) {
			return status;
		}
	}
	status = sx_hash_feed(&hashopctx, workmem+SX_ED25519_SZ, SX_ED25519_SZ);
	if (status != 0) {
		return status;
	}
	status = sx_hash_feed(&hashopctx, message, message_length);
	if (status != 0) {
		return status;
	}
	status = sx_hash_digest(&hashopctx, workmem+96);
	if (status != 0) {
		return status;
	}
	status = sx_hash_wait(&hashopctx);
	if (status != 0) {
		return status;
	}

	return status;
}

int ed25519_calculate_k(struct sxhash hashopctx, char *workmem, char *pointr,
			const char *message, size_t message_length, int prehash)
{
	/* Obtain k by hashing (R || A || message). where A is the public key */
	int status;

	status = sx_hash_create(&hashopctx, &sxhashalg_sha2_512, sizeof(hashopctx));

	if (status != 0) {
		return status;
	}
	if (prehash) {
		status = sx_hash_feed(&hashopctx, dom2, sizeof(dom2));
		if (status != 0) {
			return status;
		}
	}
	status = sx_hash_feed(&hashopctx, pointr, SX_ED25519_SZ);
	if (status != 0) {
		return status;
	}
	status = sx_hash_feed(&hashopctx, workmem+SX_ED25519_SZ, SX_ED25519_SZ);
	if (status != 0) {
		return status;
	}
	status = sx_hash_feed(&hashopctx, message, message_length);
	if (status != 0) {
		return status;
	}
	status = sx_hash_digest(&hashopctx, workmem+SX_ED25519_SZ);
	if (status != 0) {
		return status;
	}
	status = sx_hash_wait(&hashopctx);
	if (status != 0) {
		return status;
	}
	return status;
}

int ed25519_sign_internal(const uint8_t *priv_key, char *signature, const uint8_t *message,
			size_t message_length, int prehash)
{
	char workmem[160];
	struct sxhash hashopctx;
	char pointr_buffer[SX_ED25519_DGST_SZ];
	char *pointr = pointr_buffer;
	int status;

	/*Hash the private key, the digest is stored in the first 64 bytes of workmem*/
	status = ed25519_hash_privkey(hashopctx, workmem, priv_key);
	if (status != 0) {
		return status;
	}

	/* Obtain r by hashing (prefix || message), where prefix is the second
	 * half of the private key digest.
	 */
	status = ed25519_calculate_r(hashopctx, workmem, message, message_length, prehash);
	/* Perform point multiplication R = [r]B. This is the encoded point R,
	 * which is the first part of the signature.
	 */
	status = sx_ed25519_ptmult((const struct sx_ed25519_dgst *)(workmem + 96),
				(struct sx_ed25519_pt *)pointr);
	if (status != 0) {
		return status;

	}
	/* The secret scalar s is computed in place from the first half of the
	 * private key digest.
	 */
	decode_scalar_25519(workmem);

	/* Clear second half of private key digest: sx_async_ed25519_ptmult_go()
	 * works on an input of SX_ED25519_DGST_SZ bytes.
	 */
	safe_memset(workmem + SX_ED25519_SZ, sizeof(workmem) - SX_ED25519_SZ, 0, SX_ED25519_SZ);
	/* Perform point multiplication A = [s]B,
	 * to obtain the public key A. which is stored in workmem[32:63]
	 */
	status = sx_ed25519_ptmult((const struct sx_ed25519_dgst *)(workmem),
	(struct sx_ed25519_pt *)(struct sx_ed25519_pt *)((char *)workmem + SX_ED25519_PT_SZ));

	if (status != 0) {
		return status;
	}

	status = ed25519_calculate_k(hashopctx, workmem, pointr, message, message_length, prehash);
	if (status != 0) {
		return status;
	}

	/* Compute (r + k * s) mod L. This gives the second part of the
	 * signature, which is the encoded S which is stored in pointr.
	 */

	status = sx_ed25519_sign(
		(const struct sx_ed25519_dgst *)(workmem + SX_ED25519_SZ),
		(const struct sx_ed25519_dgst *)(workmem + 3 * SX_ED25519_SZ),
		(const struct sx_ed25519_v *)workmem,
		(struct sx_ed25519_v *)(pointr + SX_ED25519_PT_SZ));
	if (status != 0) {
		return status;
	}

	memcpy(signature, pointr, SX_ED25519_DGST_SZ);

	return status;

}


int ed25519_sign(const uint8_t *priv_key, char *signature,
		const uint8_t *message, size_t message_length)
{
	return ed25519_sign_internal(priv_key, signature, message, message_length, 0);
}

int ed25519ph_sign(const uint8_t *priv_key, char *signature,
		const uint8_t *message, size_t message_length, int ismessage)
{
	char hashedmessage[SX_ED25519_DGST_SZ];
	int status;

	if (ismessage) {
		status = ed25519_hash_message(message, message_length, hashedmessage);
		if (status != 0) {
			return status;
		}
		return ed25519_sign_internal(priv_key, signature,
					hashedmessage, SX_ED25519_DGST_SZ, 1);
	} else {
		return ed25519_sign_internal(priv_key, signature, message, message_length, 1);
	}
}

int ed25519_verify_internal(const uint8_t *pubkey, const char *message,
				size_t message_length, const char *signature, int prehash)
{
	char workmem[SX_ED25519_DGST_SZ];
	struct sxhash hashopctx;
	int status;

	status = sx_hash_create(&hashopctx, &sxhashalg_sha2_512, sizeof(hashopctx));

	if (status != 0) {
		return status;
	}
	if (prehash) {
		status = sx_hash_feed(&hashopctx, dom2, sizeof(dom2));
		if (status != 0) {
			return status;
		}
	}
	status = sx_hash_feed(&hashopctx, signature, SX_ED25519_SZ);

	if (status != 0) {
		return status;
	}
	status = sx_hash_feed(&hashopctx, pubkey, SX_ED25519_SZ);

	if (status != 0) {
		return status;
	}
	status = sx_hash_feed(&hashopctx, message, message_length);

	if (status != 0) {
		return status;
	}
	status = sx_hash_digest(&hashopctx, workmem);
	if (status != 0) {
		return status;
	}
	status = sx_hash_wait(&hashopctx);
	if (status != 0) {
		return status;
	}

	status = sx_ed25519_verify(
		(struct sx_ed25519_dgst *)workmem,
		(struct sx_ed25519_pt *)pubkey,
		(const struct sx_ed25519_v *)(signature + SX_ED25519_SZ),
		(const struct sx_ed25519_pt *)signature);

	return status;
}

int ed25519_verify(const uint8_t *pubkey, const char *message,
				size_t message_length, const char *signature)
{
	return ed25519_verify_internal(pubkey, message,
				message_length, signature, 0);
}

int ed25519ph_verify(const uint8_t *pubkey, const char *message,
				size_t message_length, const char *signature, int ismessage)
{
	int status;
	char hashedmessage[SX_ED25519_DGST_SZ];

	if (ismessage) {
		status = ed25519_hash_message(message, message_length, hashedmessage);
		if (status != 0) {
			return status;
		}
		return ed25519_verify_internal(pubkey, hashedmessage,
						SX_ED25519_DGST_SZ, signature, 1);
	}
	return ed25519_verify_internal(pubkey, message, message_length, signature, 1);
}

int ed25519_create_pubkey(const uint8_t *priv_key,
				       uint8_t *pubkey)
{
	char workmem[SX_ED25519_DGST_SZ];
	struct sxhash hashopctx;
	int status;

	/*Hash the privat key*/
	status = sx_hash_create(&hashopctx, &sxhashalg_sha2_512, sizeof(hashopctx));
	if (status != 0) {
		return status;
	}

	status = sx_hash_feed(&hashopctx, priv_key, SX_ED25519_SZ);
	if (status != 0) {
		return status;
	}

	status = sx_hash_digest(&hashopctx, workmem);
	if (status != 0) {
		return status;
	}

	/*Wait to make sure hash operation is finished before doing
	 * calculations on it. Removing this breaks pubkey generation
	 */
	status = sx_hash_wait(&hashopctx);
	if (status != 0) {
		return status;
	}

	/* The secret scalar s is computed in place from the first half of the
	 * private key digest.
	 */
	decode_scalar_25519(workmem);

	/* Clear second half of private key digest: sx_async_ed25519_ptmult_go()
	 * works on an input of SX_ED25519_DGST_SZ bytes.
	 */
	safe_memset(workmem+SX_ED25519_SZ, SX_ED25519_SZ, 0, SX_ED25519_SZ);

	/* Perform point multiplication A = [s]B, to obtain the public key A. */
	status = sx_ed25519_ptmult((const struct sx_ed25519_dgst *)(workmem),
	(struct sx_ed25519_pt *)(struct sx_ed25519_pt *)((char *)workmem + SX_ED25519_PT_SZ));

	if (status != 0) {
		return status;
	}

	memcpy(pubkey, workmem+SX_ED25519_SZ, SX_ED25519_SZ);

	return status;

}
