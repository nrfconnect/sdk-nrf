/* Signature scheme RSASSA-PSS, based on RFC 8017.
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout for the RSA PSS sign and verify function:
 *      1. MGF1XOR sub-function workmem (size: hLen + 4). In the verify function, once
 *         the MGF1XOR sub-function is not needed anymore, the first
 *         ZERO_PADDING_BYTES octets of this area are used to store the zeros of
 *         M' (see step 12 of EMSA-PSS-VERIFY in RFC 8017).
 *      2. EM: encoded message octet string (size: same as the size of the RSA
 *         modulus). In the verify function, the maskedDB portion of EM is decoded
 *         in place, hence it is overwritten with DB. In the sign function, DB is
 *         prepared inside EM and then overwritten with maskedDB. Also in the
 *         sign function, the last 8 bytes of the EM area are initially used to
 *         store the 8 zero octets that are part of M'
 *      3. mHash: message digest (size: hLen bytes). In the verify function, once
 *         mHash is not needed anymore, this area is used to store H'
 *
 * The required workmem size is computed with:
 *      rsa_modulus_size + 2*hash_digest_size + 4
 * where all sizes are expressed in bytes. Some examples:
 *      workmem size for 1024 bit RSA modulus and SHA-1:     172 bytes
 *      workmem size for 1024 bit RSA modulus and SHA2-256:  196 bytes
 *      workmem size for 2048 bit RSA modulus and SHA2-256:  324 bytes
 *      workmem size for 4096 bit RSA modulus and SHA2-256:  580 bytes
 *
 * Assumptions
 * - All the byte strings (signature, key and message) given to the RSA PSS
 *   functions are big endian (as in RFC 8017).
 *
 * Notes
 * - We do not check that the length of the message M is <= max input size for
 *   the hash function (see e.g. step 1 of the EMSA-PSS-ENCODE operation in
 *   RFC 8017): in fact the sx_hash_feed() function
 *   will impose a stricter limitation (see documentation of sx_hash_feed()).
 * - We do not perform the check in step 1 of MGF1 in RFC 8017: for practical
 *   RSA modulus lengths that error condition cannot occur.
 * - We do not perform the check in step 1 of RSASSA-PSS-VERIFY in RFC 8017: we
 *   want to allow signature representations with smaller size than the modulus.
 */

#include <internal/rsa/cracen_rsa_mgf1xor.h>
#include <internal/rsa/cracen_rsa_key.h>
#include <internal/rsa/cracen_rsa_signature_pss.h>
#include <internal/rsa/cracen_rsa_common.h>

#include <string.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <silexpk/sxops/rsa.h>
#include <silexpk/iomem.h>
#include <sxsymcrypt/hash.h>
#include <psa/crypto.h>
#include <cracen/statuscodes.h>
#include <cracen/mem_helpers.h>
#include <cracen/common.h>
#include <cracen_psa.h>
#include <cracen_psa_ctr_drbg.h>
#include <cracen_psa_primitives.h>

#define WORKMEM_SIZE	(PSA_BITS_TO_BYTES(PSA_MAX_RSA_KEY_BITS) + 2 * PSA_HASH_MAX_SIZE + 4)
#define NUMBER_OF_SLOTS 6

enum {
	PSS_TRAILER_BYTE = 0xBC,
	PSS_MGF1_OVERHEAD = 4,
	ZERO_PADDING_BYTES = 8
};

struct rsa_pss_workmem {
	uint8_t workmem[WORKMEM_SIZE];
	uint8_t *wmem;
	uint8_t *mHash;
	uint8_t *seed;
	uint8_t *salt;
	uint8_t *db;
	uint8_t *H;
	uint8_t *H1;
	uint8_t *padding;
};

/* Produce the bit mask that is needed for steps 6 and 9 of EMSA-PSS-VERIFY and
 * for step 11 of EMSA-PSS-ENCODE. In the returned bit mask, all the bits with
 * higher or equal significance than the most significant non-zero bit in the
 * most significant byte of the modulus are set to 1.
 */
static uint8_t create_msb_mask(struct cracen_rsa_key *key)
{
	unsigned int mask;
	unsigned int shift;

	shift = cracen_ffkey_bitsz(key);
	shift = (shift + 7) % 8;
	mask = (0xFF << shift) & 0xFF;

	return mask;
}

int cracen_rsa_pss_sign_message(struct cracen_rsa_key *rsa_key, struct cracen_signature *signature,
				const struct sxhashalg *hashalg, const uint8_t *message,
				size_t message_length, size_t saltsz)
{
	int status;
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	uint8_t digest[digestsz];

	status = cracen_hash_input(message, message_length, hashalg, digest);
	if (status != SX_OK) {
		return status;
	}

	return cracen_rsa_pss_sign_digest(rsa_key, signature, hashalg, digest, digestsz, saltsz);
}

static void rsa_pss_sign_init(struct rsa_pss_workmem *workmem, size_t emsz, size_t masksz,
			      size_t saltsz, size_t digestsz, size_t modulussz,
			      size_t padding_offset)
{
	workmem->wmem = cracen_get_rsa_workmem_pointer(workmem->workmem, digestsz);
	workmem->mHash = workmem->wmem + modulussz;
	workmem->salt = workmem->mHash - emsz + masksz - saltsz;
	workmem->seed = workmem->wmem + modulussz - digestsz - 1;
	workmem->db = workmem->mHash - emsz;
	workmem->H = workmem->mHash - digestsz - 1;
	workmem->padding = workmem->wmem + padding_offset;
}

int cracen_rsa_pss_sign_digest(struct cracen_rsa_key *rsa_key, struct cracen_signature *signature,
			       const struct sxhashalg *hashalg, const uint8_t *digest,
			       size_t digest_length, size_t saltsz)
{
	int sx_status;
	psa_status_t psa_status;
	size_t emsz; /* size in bytes of the encoded message (called emLen in RFC 8017) */
	size_t modulussz = CRACEN_RSA_KEY_OPSZ(rsa_key);

	if (cracen_ffkey_bitsz(rsa_key) % 8 == 1) {
		emsz = modulussz - 1;
	} else {
		emsz = modulussz;
	}

	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	size_t masksz = emsz - digestsz - 1;
	const size_t padding_offset = modulussz - ZERO_PADDING_BYTES;
	const size_t wmem_sz = cracen_get_rsa_workmem_size(WORKMEM_SIZE, digestsz);

	/* Workmem for RSA signature function is rsa_modulus_size + 2*hash_digest_size + 4 */
	struct rsa_pss_workmem workmem;

	rsa_pss_sign_init(&workmem, emsz, masksz, saltsz, digestsz, modulussz, padding_offset);

	if (WORKMEM_SIZE < modulussz + 2 * digestsz + 4) {
		return SX_ERR_WORKMEM_BUFFER_TOO_SMALL;
	}

	/* assumption: the most significant byte of the modulus is not zero */
	if (((cracen_ffkey_bitsz(rsa_key) + 7) >> 3) != modulussz) {
		return SX_ERR_INVALID_ARG;
	}

	/* step 3 of EMSA-PSS-ENCODE in RFC 8017 */
	if (emsz < digestsz + saltsz + 2) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	/* guarantee there's enough room in signature to store it */
	if (modulussz > signature->sz) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	if (digest_length < digestsz) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	/* Copy the message digest to workmem. */
	memcpy(workmem.mHash, digest, digestsz);

	psa_status = cracen_get_random(NULL, workmem.salt, saltsz);
	if (psa_status != PSA_SUCCESS) {
		return SX_ERR_UNKNOWN_ERROR;
	}

	/* prepare padding string PS (step 7 of EMSA-PSS-ENCODE) */
	const size_t padding_ps_offset = CRACEN_RSA_KEY_OPSZ(rsa_key) - emsz;

	safe_memset(workmem.wmem + padding_ps_offset, wmem_sz - padding_ps_offset, 0,
		    masksz - saltsz - 1);

	/* step 8 of EMSA-PSS-ENCODE (padding string and salt are already there)
	 */
	*(workmem.salt - 1) = 1;

	/* prepare padding */
	safe_memset(workmem.padding, wmem_sz - padding_offset, 0, ZERO_PADDING_BYTES);

	uint8_t const *hash_array[] = {workmem.padding, workmem.salt};
	size_t hash_array_lengths[] = {ZERO_PADDING_BYTES + digestsz, saltsz};
	size_t input_count = 2;

	sx_status = cracen_hash_all_inputs(hash_array, hash_array_lengths, input_count, hashalg,
					   workmem.H);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	uint8_t *xorinout = workmem.mHash - emsz;

	sx_status = cracen_run_mgf1xor(workmem.workmem, WORKMEM_SIZE, hashalg, workmem.seed,
				       digestsz, xorinout, masksz);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	uint8_t bitmask;

	/* step 11 of EMSA-PSS-ENCODE in RFC 8017 (clear the leftmost bits in
	 * maskedDB according to bitmask). This works also when emsz equals
	 * modulussz - 1.
	 */
	bitmask = create_msb_mask(rsa_key);
	workmem.wmem[0] &= ~bitmask;

	/* set to 0xBC the last byte of EM (step 12 of EMSA-PSS-ENCODE) */
	(workmem.wmem)[modulussz - 1] = PSS_TRAILER_BYTE;

	int input_sizes[NUMBER_OF_SLOTS];
	sx_pk_req req;
	struct sx_pk_slot inputs[NUMBER_OF_SLOTS];

	sx_pk_acquire_hw(&req);
	/* modular exponentiation m^d mod n (RSASP1 sign primitive) */
	sx_status =
		cracen_rsa_modexp(&req, inputs, rsa_key, workmem.wmem, modulussz, input_sizes);
	if (sx_status != SX_OK) {
		sx_pk_release_req(&req);
		return sx_status;
	}
	/* Get result of modular exponentiation, placing it in user's output buffer. the output of
	 * the exponentiation is the signature
	 */
	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(&req);
	const int opsz = sx_pk_get_opsize(&req);

	sx_rdpkmem(signature->r, outputs[0], opsz);
	signature->sz = opsz;
	/* releases the HW resource so it has to be
	 * called even if an error occurred
	 */
	sx_pk_release_req(&req);
	safe_memzero(workmem.workmem, WORKMEM_SIZE);

	return sx_status;
}

static void rsa_pss_verify_init(struct rsa_pss_workmem *workmem, size_t emsz, size_t masksz,
				size_t saltsz, size_t digestsz, size_t modulussz)
{
	workmem->wmem = cracen_get_rsa_workmem_pointer(workmem->workmem, digestsz);
	workmem->mHash = workmem->wmem + modulussz;
	workmem->salt = workmem->mHash - emsz + masksz - saltsz;
	workmem->seed = workmem->mHash - digestsz - 1;
	workmem->db = workmem->mHash - emsz;
	workmem->H = workmem->mHash - emsz + masksz;
}

int cracen_rsa_pss_verify_message(struct cracen_rsa_key *rsa_key,
				  struct cracen_const_signature *signature,
				  const struct sxhashalg *hashalg, const uint8_t *message,
				  size_t message_length, size_t saltsz)
{
	int status;
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	uint8_t digest[digestsz];

	status = cracen_hash_input(message, message_length, hashalg, digest);
	if (status != SX_OK) {
		return status;
	}

	return cracen_rsa_pss_verify_digest(rsa_key, signature, hashalg, digest, digestsz, saltsz);
}

int cracen_rsa_pss_verify_digest(struct cracen_rsa_key *rsa_key,
				 struct cracen_const_signature *signature,
				 const struct sxhashalg *hashalg, const uint8_t *digest,
				 size_t digest_length, size_t saltsz)
{
	int r;
	int sx_status;
	size_t modulussz = CRACEN_RSA_KEY_OPSZ(rsa_key);
	size_t emsz; /* emsz: size in bytes of the encoded message (called emLen in RFC 8017) */

	if (cracen_ffkey_bitsz(rsa_key) % 8 == 1) {
		emsz = modulussz - 1;
	} else {
		emsz = modulussz;
	}
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	size_t masksz = emsz - digestsz - 1;
	/* Workmem for RSA signature function is rsa_modulus_size + 2*hash_digest_size + 4 */
	struct rsa_pss_workmem workmem;

	rsa_pss_verify_init(&workmem, emsz, masksz, saltsz, digestsz, modulussz);

	if (WORKMEM_SIZE < modulussz + 2 * digestsz + 4) {
		return SX_ERR_WORKMEM_BUFFER_TOO_SMALL;
	}

	/* assumption: the most significant byte of the modulus is not zero */
	if (((cracen_ffkey_bitsz(rsa_key) + 7) / 8) != modulussz) {
		return SX_ERR_INVALID_ARG;
	}

	/* step 3 of EMSA-PSS-ENCODE in RFC 8017 */
	if (emsz < digestsz + saltsz + 2) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	/* guarantee there's enough room in signature to store it */
	if (modulussz > signature->sz) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	if (digest_length < digestsz) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	/* Copy the message digest to workmem. */
	memcpy(workmem.mHash, digest, digestsz);

	int input_sizes[NUMBER_OF_SLOTS];
	sx_pk_req req;
	struct sx_pk_slot inputs[NUMBER_OF_SLOTS];

	sx_pk_acquire_hw(&req);
	/* modular exponentiation m^d mod n (RSASP1 sign primitive) */
	sx_status = cracen_rsa_modexp(&req, inputs, rsa_key, signature->r, signature->sz,
				      input_sizes);
	if (sx_status != SX_OK) {
		sx_pk_release_req(&req);
		return sx_status;
	}

	/* the output of the exponentiation is the encoded message EM. */
	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(&req);
	const int opsz = sx_pk_get_opsize(&req);

	sx_rdpkmem(workmem.wmem, outputs[0], opsz);
	sx_pk_release_req(&req);

	/* invalid signature if rightmost byte of EM is not 0xBC (step 4 of
	 * EMSA-PSS-VERIFY in RFC 8017)
	 */
	if ((workmem.wmem[CRACEN_RSA_KEY_OPSZ(rsa_key) - 1]) != PSS_TRAILER_BYTE) {
		return SX_ERR_INVALID_SIGNATURE;
	}

	/* Step 6 of EMSA-PSS-VERIFY in RFC 8017: test the leftmost bits in EM
	 * according to bitmask. This works also when emsz equals modulussz - 1.
	 */
	uint8_t bitmask;

	bitmask = create_msb_mask(rsa_key);
	if ((workmem.wmem[0]) & bitmask) {
		return SX_ERR_INVALID_SIGNATURE;
	}

	sx_status = cracen_run_mgf1xor(workmem.workmem, WORKMEM_SIZE, hashalg, workmem.seed,
				       digestsz, workmem.db, masksz);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* Step 9 of EMSA-PSS-VERIFY in RFC 8017 (clear the leftmost bits in DB
	 * according to bitmask). This works also when emsz equals modulussz - 1.
	 */
	bitmask = create_msb_mask(rsa_key);
	workmem.wmem[0] &= ~bitmask;
	/* zero padding for next hash: reuse mgf1xor workmem */
	size_t zero_len;
	uint8_t *zeros;

	zeros = workmem.workmem;
	/* number of bytes in DB that must be equal to zero */
	zero_len = masksz - saltsz - 1;

	/* step 10 of EMSA-PSS-VERIFY in RFC 8017 */
	r = constant_memdiff_array_value(workmem.db, 0, zero_len);
	r |= (workmem.db[zero_len] != 1);
	if (r) {
		return SX_ERR_INVALID_SIGNATURE;
	}

	/* hash function to produce H' (step 13 of EMSA-PSS-VERIFY in RFC 8017) */
	uint8_t const *hash_array[] = {zeros, workmem.mHash, workmem.db + masksz - saltsz};
	size_t hash_array_lengths[] = {ZERO_PADDING_BYTES, digestsz, saltsz};
	size_t input_count = 3;

	safe_memset(zeros, WORKMEM_SIZE, 0, ZERO_PADDING_BYTES);
	sx_status = cracen_hash_all_inputs(hash_array, hash_array_lengths, input_count, hashalg,
					   workmem.mHash); /* H' overwrites mHash */
	if (sx_status != SX_OK) {
		return sx_status;
	}
	/* step 14 of EMSA-PSS-VERIFY in RFC 8017 */
	sx_status = constant_memcmp(workmem.H, workmem.mHash, digestsz);
	if (sx_status != SX_OK) {
		return SX_ERR_INVALID_SIGNATURE;
	} else {
		return SX_OK;
	}
}
