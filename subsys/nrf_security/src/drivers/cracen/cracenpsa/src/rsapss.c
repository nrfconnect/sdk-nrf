/* Signature scheme RSASSA-PSS, based on RFC 8017.
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout for the RSA PSS sign and verify task:
 *      1. MGF1XOR sub-task workmem (size: hLen + 4). In the verify task, once
 *         the MGF1XOR sub-task is not needed anymore, the first
 *         ZERO_PADDING_BYTES octets of this area are used to store the zeros of
 *         M' (see step 12 of EMSA-PSS-VERIFY in RFC 8017).
 *      2. EM: encoded message octet string (size: same as the size of the RSA
 *         modulus). In the verify task, the maskedDB portion of EM is decoded
 *         in place, hence it is overwritten with DB. In the sign task, DB is
 *         prepared inside EM and then overwritten with maskedDB. Also in the
 *         sign task, the last 8 bytes of the EM area are initially used to
 *         store the 8 zero octets that are part of M'
 *      3. mHash: message digest (size: hLen bytes). In the verify task, once
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
 *   tasks are big endian (as in RFC 8017).
 *
 * Notes
 * - We do not check that the length of the message M is <= max input size for
 *   the hash function (see e.g. step 1 of the EMSA-PSS-ENCODE operation in
 *   RFC 8017): in fact the sx_hash_feed() function called by si_task_consume()
 *   will impose a stricter limitation (see documentation of sx_hash_feed()).
 * - We do not perform the check in step 1 of MGF1 in RFC 8017: for practical
 *   RSA modulus lengths that error condition cannot occur.
 * - We do not perform the check in step 1 of RSASSA-PSS-VERIFY in RFC 8017: we
 *   want to allow signature representations with smaller size than the modulus.
 */

#include <string.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <silexpk/sxops/rsa.h>
#include <silexpk/iomem.h>
#include <psa/crypto.h>
#include <cracen/statuscodes.h>
#include <cracen/mem_helpers.h>
#include <cracen_psa.h>
#include "../include/sicrypto/mem.h"
#include "rsamgf1xor.h"
#include "rsa_key.h"
#include "cracen_psa_primitives.h"
#include "cracen_psa_rsa_signature_pss.h"
#include "rsamgf1xor.h"
#include "common.h"

#define ZERO_PADDING_BYTES 8

/* Size of the workmem of the MGF1XOR subtask. */
#define MGF1XOR_WORKMEM_SZ(digestsz) ((digestsz) + 4)

/* Return a pointer to the part of workmem that is specific to RSA PSS. */
static inline uint8_t *get_workmem_pointer(uint8_t *workmem, size_t digestsz)
{
	return workmem + MGF1XOR_WORKMEM_SZ(digestsz);
}

static inline size_t get_workmem_size(size_t workmemsz, size_t digestsz)
{
	return workmemsz - MGF1XOR_WORKMEM_SZ(digestsz);
}

/* return 0 if and only if all elements in array 'a' are equal to 'val' */
static int diff_array_val(const uint8_t *a, uint8_t val, size_t sz)
{
	size_t i;
	int r = 0;

	for (i = 0; i < sz; i++) {
		r |= a[i] ^ val;
	}

	return r;
}

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

/** Modular exponentiation (base^key mod n).
 *
 * This function is used by both the sign and the verify tasks. Note: if the
 * base is greater than the modulus, SilexPK will return the SX_ERR_OUT_OF_RANGE
 * status code.
 */

static int rsa_pss_start_modexp(struct sx_pk_acq_req *pkreq, struct sx_pk_slot *inputs,
				struct cracen_rsa_key *rsa_key, uint8_t *base, size_t basez,
				int *sizes)
{
	*pkreq = sx_pk_acquire_req(rsa_key->cmd);
	if (pkreq->status != SX_OK) {
		return pkreq->status;
	}

	cracen_ffkey_write_sz(rsa_key, sizes);
	CRACEN_FFKEY_REFER_INPUT(rsa_key, sizes) = basez;
	pkreq->status = sx_pk_list_gfp_inslots(pkreq->req, sizes, inputs);
	if (pkreq->status) {
		return pkreq->status;
	}

	/* copy modulus and exponent to device memory */
	cracen_ffkey_write(rsa_key, inputs);
	sx_wrpkmem(CRACEN_FFKEY_REFER_INPUT(rsa_key, inputs).addr, base, basez);

	sx_pk_run(pkreq->req);
	return sx_pk_wait(pkreq->req);
}

int cracen_rsa_pss_sign_message(struct cracen_rsa_key *rsa_key,
	struct cracen_signature *signature, const struct sxhashalg *hashalg,
	const uint8_t *message,
	size_t message_length, size_t saltsz)
{
	int status;
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	uint8_t digest[digestsz];

	status = cracen_hash_input(message, message_length, hashalg, digest);
	if (status != SX_OK) {
		return status;
	}

	return cracen_rsa_pss_sign_digest(rsa_key, signature, hashalg, message, message_length, saltsz);
}

int cracen_rsa_pss_sign_digest(struct cracen_rsa_key *rsa_key,
			       struct cracen_signature *signature, const struct sxhashalg *hashalg,
			       const uint8_t *digest,
			       size_t digest_length, size_t saltsz)
{
	int sx_status;
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	size_t modulussz = CRACEN_RSA_KEY_OPSZ(rsa_key);
	size_t workmemsz = PSA_BITS_TO_BYTES(PSA_MAX_RSA_KEY_BITS) + 2 * PSA_HASH_MAX_SIZE + 4;
	/* Workmem for RSA signature task is rsa_modulus_size + 2*hash_digest_size + 4 */
	uint8_t workmem[workmemsz];

	/* emsz: size in bytes of the encoded message (called emLen in RFC 8017)
	 */
	size_t emsz;

	if (workmemsz < modulussz + 2 * digestsz + 4) {
		return SX_ERR_WORKMEM_BUFFER_TOO_SMALL;
	}

	/* assumption: the most significant byte of the modulus is not zero */
	if (((cracen_ffkey_bitsz(rsa_key) + 7) >> 3) != modulussz) {
		return SX_ERR_INVALID_ARG;
	}

	if (cracen_ffkey_bitsz(rsa_key) % 8 == 1) {
		emsz = modulussz - 1;
	} else {
		emsz = modulussz;
	}

	/* step 3 of EMSA-PSS-ENCODE in RFC 8017 */
	if (emsz < digestsz + saltsz + 2) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	/* guarantee there's enough room in signature to store it */
	if (modulussz > signature->sz) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	uint8_t *wmem = get_workmem_pointer(workmem, digestsz);
	const size_t wmem_sz = get_workmem_size(workmemsz, digestsz);
	uint8_t *mHash = wmem + CRACEN_RSA_KEY_OPSZ(rsa_key);

	if (digest_length < digestsz) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	if (digest_length > digestsz) {
		return SX_ERR_TOO_BIG;
	}

	/* Copy the message digest to workmem. */
	memcpy(mHash, digest, digest_length);

	size_t masksz = emsz - digestsz - 1;

	uint8_t *salt = wmem + CRACEN_RSA_KEY_OPSZ(rsa_key) - emsz + masksz - saltsz;

	psa_status_t psa_status = PSA_ERROR_CORRUPTION_DETECTED;

	/* ask the PRNG task (part of the environment) to generate the salt */
	psa_status = cracen_get_random(NULL, salt, saltsz);
	if (psa_status != PSA_SUCCESS) {
		return SX_ERR_UNKNOWN_ERROR;
	}

	const size_t padding_offset = CRACEN_RSA_KEY_OPSZ(rsa_key) - ZERO_PADDING_BYTES;
	uint8_t *padding = wmem + padding_offset;
	uint8_t *H = wmem + CRACEN_RSA_KEY_OPSZ(rsa_key) - digestsz - 1;

	/* prepare padding string PS (step 7 of EMSA-PSS-ENCODE) */
	const size_t padding_ps_offset = CRACEN_RSA_KEY_OPSZ(rsa_key) - emsz;

	safe_memset(wmem + padding_ps_offset, wmem_sz - padding_ps_offset, 0,
		    masksz - saltsz - 1);

	/* step 8 of EMSA-PSS-ENCODE (padding string and salt are already there)
	 */
	*(salt - 1) = 1;

	/* prepare padding */
	safe_memset(padding, wmem_sz - padding_offset, 0, ZERO_PADDING_BYTES);


	uint8_t const *hash_array[] = {padding, salt};
	size_t hash_array_lengths[] = {ZERO_PADDING_BYTES + digestsz, saltsz};
	size_t input_count = 2;

	sx_status = cracen_hash_all_inputs(hash_array, hash_array_lengths,
				     input_count, hashalg,
				     H);

	if (sx_status != SX_OK) {
		return sx_status;
	}

	uint8_t *seed = wmem + modulussz - digestsz - 1;
	uint8_t *xorinout = wmem + modulussz - emsz;

	sx_status = cracen_run_mgf1xor(workmem, workmemsz, hashalg,
		seed, digestsz, xorinout, masksz);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	size_t keysz = CRACEN_RSA_KEY_OPSZ(rsa_key);
	uint8_t bitmask;

	/* step 11 of EMSA-PSS-ENCODE in RFC 8017 (clear the leftmost bits in
	 * maskedDB according to bitmask). This works also when emsz equals
	 * modulussz - 1.
	 */
	bitmask = create_msb_mask(rsa_key);
	wmem[0] &= ~bitmask;

	/* set to 0xBC the last byte of EM (step 12 of EMSA-PSS-ENCODE) */
	((uint8_t *)(wmem))[keysz - 1] = 0xBC;

	int sizes[6];
	struct sx_pk_acq_req pkreq;
	struct sx_pk_slot inputs[6];

	/* modular exponentiation m^d mod n (RSASP1 sign primitive) */
	sx_status = rsa_pss_start_modexp(&pkreq, inputs, rsa_key, wmem, keysz, sizes);
	if (sx_status != SX_OK) {
		return sx_status;
	}
	/** Get result of modular exponentiation, placing it in user's output buffer. */

	/* the output of the exponentiation is the signature.*/
	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(pkreq.req);
	const int opsz = sx_pk_get_opsize(pkreq.req);

	sx_rdpkmem(signature->r, outputs[0], opsz);
	signature->sz = opsz;
	/* releases the HW resource so it has to be
	 * called even if an error occurred
	 */
	sx_pk_release_req(pkreq.req);
	safe_memzero(workmem, workmemsz);

	return sx_status;
}

int cracen_rsa_pss_verify_message(struct cracen_rsa_key *rsa_key,
	struct cracen_signature *signature, const struct sxhashalg *hashalg,
	const uint8_t *message,
	size_t message_length, size_t saltsz)
{
	int status;
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	uint8_t digest[digestsz];

	status = cracen_hash_input(message, message_length, hashalg, digest);
	if (status != SX_OK) {
		return status;
	}

	return cracen_rsa_pss_sign_digest(rsa_key, signature, hashalg, message, message_length, saltsz);
}


int cracen_rsa_pss_verify_digest(struct cracen_rsa_key *rsa_key,
	struct cracen_signature *signature, const struct sxhashalg *hashalg,
	const uint8_t *digest,
	size_t digest_length, size_t saltsz)
{
	int r;
	int sx_status;
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	size_t modulussz = CRACEN_RSA_KEY_OPSZ(rsa_key);
	size_t workmemsz = PSA_BITS_TO_BYTES(PSA_MAX_RSA_KEY_BITS) + 2 * PSA_HASH_MAX_SIZE + 4;
	/* Workmem for RSA signature task is rsa_modulus_size + 2*hash_digest_size + 4 */
	uint8_t workmem[workmemsz];

	/* emsz: size in bytes of the encoded message (called emLen in RFC 8017)
	 */
	size_t emsz;

	if (workmemsz < modulussz + 2 * digestsz + 4) {
		return SX_ERR_WORKMEM_BUFFER_TOO_SMALL;
	}

	/* assumption: the most significant byte of the modulus is not zero */
	if (((cracen_ffkey_bitsz(rsa_key) + 7) >> 3) != modulussz) {
		return SX_ERR_INVALID_ARG;
	}

	if (cracen_ffkey_bitsz(rsa_key) % 8 == 1) {
		emsz = modulussz - 1;
	} else {
		emsz = modulussz;
	}

	/* step 3 of EMSA-PSS-ENCODE in RFC 8017 */
	if (emsz < digestsz + saltsz + 2) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	/* guarantee there's enough room in signature to store it */
	if (modulussz > signature->sz) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	uint8_t *wmem = get_workmem_pointer(workmem, digestsz);
	uint8_t *mHash = wmem + CRACEN_RSA_KEY_OPSZ(rsa_key);

	if (digest_length < digestsz) {
		return SX_ERR_INPUT_BUFFER_TOO_SMALL;
	}

	if (digest_length > digestsz) {
		return SX_ERR_TOO_BIG;
	}

	/* Copy the message digest to workmem. */
	memcpy(mHash, digest, digest_length);

	int sizes[6];
	struct sx_pk_acq_req pkreq;
	struct sx_pk_slot inputs[6];

	/* modular exponentiation m^d mod n (RSASP1 sign primitive) */
	sx_status = rsa_pss_start_modexp(&pkreq, inputs, rsa_key, signature->r, signature->sz, sizes);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	uint8_t bitmask;

	/* the output of the exponentiation is the encoded message EM. */
	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(pkreq.req);
	const int opsz = sx_pk_get_opsize(pkreq.req);

	sx_rdpkmem(wmem, outputs[0], opsz);
	sx_pk_release_req(pkreq.req);

	/* invalid signature if rightmost byte of EM is not 0xBC (step 4 of
	 * EMSA-PSS-VERIFY in RFC 8017)
	 */
	if ((wmem[CRACEN_RSA_KEY_OPSZ(rsa_key) - 1]) != 0xBC) {
		return SX_ERR_INVALID_SIGNATURE;
	}

	/* Step 6 of EMSA-PSS-VERIFY in RFC 8017: test the leftmost bits in EM
	 * according to bitmask. This works also when emsz equals modulussz - 1.
	 */
	bitmask = create_msb_mask(rsa_key);
	if ((wmem[0]) & bitmask) {
		return SX_ERR_INVALID_SIGNATURE;
	}

	uint8_t *seed = wmem + modulussz - digestsz - 1;
	uint8_t *xorinout = wmem + modulussz - emsz;
	size_t masksz = emsz - digestsz - 1;

	sx_status = cracen_run_mgf1xor(workmem, workmemsz, hashalg,
		seed, digestsz, xorinout, masksz);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	size_t zero_len;
	uint8_t *zeros;
	uint8_t *db;

	mHash = wmem + modulussz;
	db = wmem + modulussz - emsz;

	/* Step 9 of EMSA-PSS-VERIFY in RFC 8017 (clear the leftmost bits in DB
	 * according to bitmask). This works also when emsz equals modulussz - 1.
	 */
	bitmask = create_msb_mask(rsa_key);
	wmem[0] &= ~bitmask;

	/* number of bytes in DB that must be equal to zero */
	zero_len = masksz - saltsz - 1;

	/* step 10 of EMSA-PSS-VERIFY in RFC 8017 */
	r = diff_array_val(db, 0, zero_len);
	r |= (db[zero_len] != 1);
	if (r) {
		return SX_ERR_INVALID_SIGNATURE;
	}

	/* zero padding for next hash: reuse mgf1xor workmem */
	zeros = workmem;
	safe_memset(zeros, workmemsz, 0, ZERO_PADDING_BYTES);

	/* hash task to produce H' (step 13 of EMSA-PSS-VERIFY in RFC 8017) */
	uint8_t const *hash_array[] = {zeros, mHash, db + masksz - saltsz};
	size_t hash_array_lengths[] = {ZERO_PADDING_BYTES, digestsz, saltsz};
	size_t input_count = 3;

	sx_status = cracen_hash_all_inputs(hash_array, hash_array_lengths,
				     input_count, hashalg,
				     mHash); /* H' overwrites mHash */
	if (sx_status != SX_OK) {
		return sx_status;
	}
	uint8_t *H = wmem + CRACEN_RSA_KEY_OPSZ(rsa_key) - emsz + masksz;
	uint8_t *H1 = wmem + CRACEN_RSA_KEY_OPSZ(rsa_key);

	/* step 14 of EMSA-PSS-VERIFY in RFC 8017 */
	sx_status = cracen_memdiff(H, H1, digestsz);
	if (sx_status == 0) {
		return SX_OK;
	} else {
		return SX_ERR_INVALID_SIGNATURE;
	}

	return SX_OK;
}