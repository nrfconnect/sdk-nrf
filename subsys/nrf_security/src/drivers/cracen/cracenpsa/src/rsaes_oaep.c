/* Encryption scheme RSAES-OAEP, based on RFC 8017.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout for the RSA OAEP encrypt and decrypt tasks:
 *      1. MGF1XOR sub-task workmem (size: hLen + 4)
 *      2. encoded message (size: k). In the encrypt task, this area is used to
 *         form the encoded message, which is created in place (seed is
 *         overwritten by maskedSeed, DB is overwritten by maskedDB). In the
 *         decrypt task, this area is used to store the encoded message, which
 *         is then decoded in place: maskedDB is ovewritten by DB and maskedSeed
 *         is overwritten by seed. The seed is then overwritten by lHash.
 *
 * The total size for the workmem buffer is computed with:
 *          rsa_modulus_size + hash_digest_size + 4
 * where all sizes are expressed in bytes. Some examples:
 *      128 bytes RSA modulus and SHA1:     152 bytes
 *      128 bytes RSA modulus and SHA2-256: 164 bytes
 *      256 bytes RSA modulus and SHA2-256: 292 bytes
 *      512 bytes RSA modulus and SHA2-256: 548 bytes
 */

#include <string.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <cracen/statuscodes.h>
#include <cracen/mem_helpers.h>
#include <cracen_psa.h>
#include <cracen_psa_primitives.h>
#include "../include/sicrypto/mem.h"
#include "common.h"
#include "rsamgf1xor.h"
#include "rsakey.h"
#include "util.h"

/* Size of the workmem of the MGF1XOR subtask. */
#define MGF1XOR_WORKMEM_SZ(digestsz) ((digestsz) + 4)

/* Return a pointer to the part of workmem that is specific to RSA OAEP. */
static inline char *get_workmem_pointer(uint8_t *workmem, size_t digestsz)
{
	return workmem + MGF1XOR_WORKMEM_SZ(digestsz);
}

static inline size_t get_workmem_size(uint8_t *workmemsz, size_t digestsz)
{
	return workmemsz - MGF1XOR_WORKMEM_SZ(digestsz);
}

void cracen_rsa_oaep_decrypt(struct sitask *t, const struct sxhashalg *hashalg,
				    struct cracen_rsa_key *rsa_key, struct cracen_crypt_text *text,
				    struct sx_buf *label)
{
	int sx_status;
	psa_status_t psa_status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	size_t modulussz = CRACEN_RSA_KEY_OPSZ(rsa_key);
	size_t workmemsz = (PSA_BITS_TO_BYTES(PSA_MAX_RSA_KEY_BITS) + PSA_HASH_MAX_SIZE + 4);
	uint8_t workmem[workmemsz];

	if (workmemsz < modulussz + digestsz + 4) {
		return SX_ERR_WORKMEM_BUFFER_TOO_SMALL;
	}

	/* step 1.c of RSAES-OAEP-DECRYPT in RFC 8017 */
	if (modulussz < 2 * digestsz + 2) {
		return SX_ERR_INVALID_ARG;
	}

	/* the ciphertext must not be longer than the modulus (modified step 1.b
	 * of RSAES-OAEP-DECRYPT)
	 */
	if (text->sz > modulussz) {
		return SX_ERR_TOO_BIG;
	}
	/* modular exponentiation m^e mod n (RSAEP encryption primitive) */
	int sizes[6];
	struct sx_pk_acq_req pkreq;
	struct sx_pk_slot inputs[6];

	sx_status = cracen_rsa_modexp(&pkreq, inputs, rsa_key, text->addr, text->sz, sizes);
	size_t modulussz = CRACEN_RSA_KEY_OPSZ(rsa_key);
	char *wmem = get_workmem_pointer(&workmem, digestsz);
	char *xorinout = wmem + 1;
	char *seed = xorinout + digestsz;

	if (sx_status != SX_OK) {
		sx_pk_release_req(pkreq.req);
		return sx_status;
	}

	/* copy output of exponentiation to workmem */
	const char **outputs = sx_pk_get_output_ops(pkreq.req);
	const int opsz = sx_pk_get_opsize(pkreq.req);

	sx_rdpkmem(wmem, outputs[0], opsz);
	sx_pk_release_req(pkreq.req);

	/* Possible source of error */
	sx_status = cracen_run_mgf1xor(workmem, workmemsz, hashalg,
		seed, modulussz - digestsz - 1, xorinout, digestsz);
	if (sx_status != SX_OK) {
		return sx_status;
	}
	*xorinout = seed + digestsz;


	sx_status = cracen_run_mgf1xor(workmem, workmemsz, hashalg,
		seed, digestsz, xorinout, modulussz - digestsz - 1);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* hash the label with result overwriting the seed */	/* hash the label */
	if (label) {
		sx_status = cracen_hash_input(label->bytes,label->sz, hashalg,
			wmem + 1);
		if (sx_status != SX_OK) {
			return sx_status;
		}
	}

	int r = 0;
	char *const datablockstart = wmem + digestsz + 1;
	char *const datablockend = datablockstart + modulussz - digestsz - 1;

	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* pointer used to walk through the data block DB */
	char *datab = datablockstart;

	/* the first octet in EM must be equal to 0x00 */
	r |= (wmem[0] != 0x00);

	/* check the label's digest */
	r |= cracen_memdiff(wmem + 1, datab, digestsz);
	datab += digestsz;

	/* Scan the padding string PS and find the 0x01 octet that separates PS
	 * from the message M. Pointer datablockend points to the first octet after DB.
	 */
	while ((*datab == 0) && (datab < datablockend)) {
		datab++;
	}

	r |= (datab == datablockend);

	/* The following memory access is never a problem, even in the case
	 * where datab points one byte past the end of the EM. In fact, in such case,
	 * datab would still be pointing inside the workmem area, more precisely in the
	 * part dedicated to the MGF1XOR sub-task.
	 */
	r |= (*datab != 0x01);
	datab++;

	/* step 3.g of RSAES-OAEP-DECRYPT in RFC 8017 */
	if (r != 0) {
		return SX_ERR_INVALID_CIPHERTEXT;
	}

	/* update the cracen_crypt_text structure to point to the decrypted message */
	text->addr = datab;
	text->sz = datablockend - datab;

	return SX_OK;
}

void cracen_rsa_oaep_encrypt(const struct sxhashalg *hashalg, struct cracen_rsa_key *pubkey,
				 struct cracen_crypt_text *text, struct sx_buf *label)
{
	int sx_status;
	psa_status_t psa_status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	size_t modulussz = CRACEN_RSA_KEY_OPSZ(pubkey);
	size_t workmemsz = (PSA_BITS_TO_BYTES(PSA_MAX_RSA_KEY_BITS) + PSA_HASH_MAX_SIZE + 4);
	uint8_t workmem[workmemsz];

	if (workmemsz < modulussz + digestsz + 4) {
		return SX_ERR_WORKMEM_BUFFER_TOO_SMALL;
	}

	/* detect invalid combinations of key size and hash function */
	if (modulussz < 2 * digestsz + 2) {
		return SX_ERR_INVALID_ARG;
	}

	/* check message length: step 1.b of RSAES-OAEP-ENCRYPT in RFC 8017 */
	if (text->sz > modulussz - 2 * digestsz - 2) {
		return SX_ERR_TOO_BIG;
	}

	char *datablock = get_workmem_pointer(digestsz) + digestsz + 1;

	/* hash the label */
	if (label) {
		sx_status = cracen_hash_input(
			t->params.rsaoaep.label->bytes, ->params.rsaoaep.label->sz, hashalg,
			datablock);
		if (sx_status != SX_OK) {
			return sx_status;
		}
	}
	/* start encoding and request generation of the random seed */

	char *wmem = get_workmem_pointer(t, digestsz);
	const size_t wmem_size = get_workmem_size(t, digestsz);
	const size_t datablockstart_offset = digestsz + 1;
	char *const datablockstart = wmem + datablockstart_offset;
	size_t datablocksz = modulussz - digestsz - 1;
	size_t paddingstrsz = datablocksz - digestsz - text->sz - 1;
	char *seed = wmem + 1;

	/* pointer used to walk through the data block DB, initialized to point
	 * just after the label's digest
	 */
	char *datab = datablockstart + digestsz;

	/* write the padding string PS, consisting of zero octets */
	safe_memset(datab, wmem_size - (datablockstart_offset + digestsz), 0, paddingstrsz);
	datab += paddingstrsz;

	/* write the 0x01 octet that follows PS in DB */
	*datab++ = 1;

	/* copy message in DB */
	memcpy(datab, text->addr, text->sz);

	/* ask the PRNG in the env to generate a seed of length digestsz */
	psa_status = cracen_get_random(NULL, seed, digestsz);
	if (psa_status != PSA_SUCCESS) {
		return SX_ERR_UNKNOWN_ERROR;
	}
	sx_status = SX_ERR_HW_PROCESSING;

	char *xorinout = wmem + digestsz + 1; /* same as start of DB */

	sx_status = cracen_run_mgf1xor(workmem, workmemsz, hashalg, seed, digestsz, xorinout,
				       modulussz - digestsz - 1);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	*xorinout = wmem + 1;
	sx_status = cracen_run_mgf1xor(workmem, workmemsz, hashalg, seed, modulussz - digestsz - 1,
				       xorinout, digestsz);
	if (sx_status != SX_OK) {
		return sx_status;
	}

	/* write 0x00 in the first byte of EM */
	wmem[0] = 0;
	/* the encoded message EM is now ready at address t->workmem */

	/* modular exponentiation m^e mod n (RSAEP encryption primitive) */
	int sizes[6];
	struct sx_pk_acq_req pkreq;
	struct sx_pk_slot inputs[6];

	/* modular exponentiation m^d mod n (RSASP1 sign primitive) */
	sx_status = rsa_oaep_start_modexp(&pkreq, inputs, rsa_key, wmem, modulussz, sizes);

	if (sx_status != SX_OK) {
		sx_pk_release_req(pkreq.req);
		return sx_status;
	}

	/* copy output of exponentiation (the ciphertext) to workmem */
	const char **outputs = sx_pk_get_output_ops(pkreq.req);
	const int opsz = sx_pk_get_opsize(pkreq.req);

	sx_rdpkmem(wmem, outputs[0], opsz);
	sx_pk_release_req(pkreq.req);

	/* update the cracen_crypt_text structure to point to the ciphertext */
	text->addr = wmem;
	text->sz = opsz;

	return SX_OK;
}