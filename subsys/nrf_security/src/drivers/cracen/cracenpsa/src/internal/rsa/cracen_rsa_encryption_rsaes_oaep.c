/* Encryption scheme RSAES-OAEP, based on RFC 8017.
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout for the RSA OAEP encrypt and decrypt functions:
 *      1. MGF1XOR sub-function workmem (size: hLen + 4)
 *      2. encoded message (size: k). In the encrypt function, this area is used to
 *         form the encoded message, which is created in place (seed is
 *         overwritten by maskedSeed, DB is overwritten by maskedDB). In the
 *         decrypt function, this area is used to store the encoded message, which
 *         is then decoded in place: maskedDB is overwritten by DB and maskedSeed
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


#include <internal/rsa/cracen_rsa_common.h>
#include <internal/rsa/cracen_rsa_mgf1xor.h>
#include <internal/rsa/cracen_rsa_key.h>

#include <string.h>
#include <silexpk/sxbuf/sxbufop.h>
#include <sxsymcrypt/hash.h>
#include <cracen/statuscodes.h>
#include <cracen/common.h>
#include <cracen/mem_helpers.h>
#include <cracen_psa.h>
#include <cracen_psa_ctr_drbg.h>
#include <cracen_psa_primitives.h>

#define WORKMEM_SIZE (PSA_BITS_TO_BYTES(PSA_MAX_RSA_KEY_BITS) + PSA_HASH_MAX_SIZE + 4)
#define NUMBER_OF_SLOTS 6

struct rsa_oaep_workmem {
	uint8_t *wmem;
	uint8_t *seed;
	uint8_t *salt;
	uint8_t *datablock;
	uint8_t *datablockstart;
	uint8_t *datablockend;
	uint8_t workmem[WORKMEM_SIZE];
};

static void rsa_oaep_decrypt_init(struct rsa_oaep_workmem *workmem, size_t digestsz,
				  size_t modulussz)
{
	workmem->wmem = cracen_get_rsa_workmem_pointer(workmem->workmem, digestsz);
	workmem->seed = workmem->wmem + 1;
	workmem->datablockstart = workmem->wmem + digestsz + 1;
	workmem->datablockend = workmem->datablockstart + modulussz - digestsz - 1;
}

int cracen_rsa_oaep_decrypt(const struct sxhashalg *hashalg, struct cracen_rsa_key *rsa_key,
			    struct cracen_crypt_text *text, struct sx_const_buf *label,
			    uint8_t *output, size_t *output_length)
{
	int sx_status;
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	size_t modulussz = CRACEN_RSA_KEY_OPSZ(rsa_key);
	struct rsa_oaep_workmem workmem;

	rsa_oaep_decrypt_init(&workmem, digestsz, modulussz);

	if (WORKMEM_SIZE < modulussz + digestsz + 4) {
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
	int input_sizes[NUMBER_OF_SLOTS];
	sx_pk_req req;
	struct sx_pk_slot inputs[NUMBER_OF_SLOTS];

	sx_pk_acquire_hw(&req);
	sx_status = cracen_rsa_modexp(&req, inputs, rsa_key, text->addr, modulussz, input_sizes);
	if (sx_status != SX_OK) {
		sx_pk_release_req(&req);
		return sx_status;
	}

	/* copy output of exponentiation to workmem */
	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(&req);
	const int opsz = sx_pk_get_opsize(&req);

	sx_rdpkmem(workmem.wmem, outputs[0], opsz);
	sx_pk_release_req(&req);
	uint8_t *xorinout = workmem.wmem + 1 + digestsz;

	sx_status = cracen_run_mgf1xor(workmem.workmem, sizeof(workmem.workmem), hashalg, xorinout,
				       modulussz - digestsz - 1, workmem.seed, digestsz);
	if (sx_status != SX_OK) {
		safe_memzero(workmem.workmem, sizeof(workmem.workmem));
		return sx_status;
	}
	sx_status = cracen_run_mgf1xor(workmem.workmem, sizeof(workmem.workmem), hashalg,
				       workmem.seed, digestsz, xorinout, modulussz - digestsz - 1);
	if (sx_status != SX_OK) {
		safe_memzero(workmem.workmem, sizeof(workmem.workmem));
		return sx_status;
	}

	/* hash the label, even when label is empty, with result overwriting the seed */
	if (label) {
		sx_status = cracen_hash_input(label->bytes, label->sz, hashalg, workmem.wmem + 1);
	} else {
		sx_status = cracen_hash_input(0, 0, hashalg, workmem.wmem + 1);
	}
	if (sx_status != SX_OK) {
		safe_memzero(workmem.workmem, sizeof(workmem.workmem));
		return sx_status;
	}
	int r = 0;

	/* pointer used to walk through the data block DB */
	uint8_t *datab = workmem.datablockstart;

	/* the first octet in EM must be equal to 0x00 */
	r |= (workmem.wmem[0] != 0x00);

	/* check the label's digest */
	r |= constant_memcmp(workmem.wmem + 1, datab, digestsz);
	datab += digestsz;

	/* Scan the padding string PS and find the 0x01 octet that separates PS
	 * from the message M. Pointer datablockend points to the first octet after DB.
	 */
	while ((*datab == 0) && (datab < workmem.datablockend)) {
		datab++;
	}

	r |= (datab == workmem.datablockend);

	/* The following memory access is never a problem, even in the case
	 * where datab points one byte past the end of the EM. In fact, in such case,
	 * datab would still be pointing inside the workmem area, more precisely in the
	 * part dedicated to the MGF1XOR sub-function.
	 */
	r |= (*datab != 0x01);
	datab++;

	/* step 3.g of RSAES-OAEP-DECRYPT in RFC 8017 */
	if (r != 0) {
		return SX_ERR_INVALID_CIPHERTEXT;
	}

	memcpy(output, datab, workmem.datablockend - datab);
	*output_length = workmem.datablockend - datab;
	safe_memzero(workmem.workmem, sizeof(workmem.workmem));
	return SX_OK;
}

static void rsa_oaep_encrypt_init(struct rsa_oaep_workmem *workmem, size_t digestsz)
{
	workmem->wmem = cracen_get_rsa_workmem_pointer(workmem->workmem, digestsz);
	workmem->datablock = workmem->wmem + digestsz + 1;
	workmem->seed = workmem->wmem + 1;
}

int cracen_rsa_oaep_encrypt(const struct sxhashalg *hashalg, struct cracen_rsa_key *rsa_key,
			    struct cracen_crypt_text *text, struct sx_const_buf *label,
			    uint8_t *output, size_t *output_length)
{
	int sx_status;
	psa_status_t psa_status = PSA_ERROR_CORRUPTION_DETECTED;
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	size_t modulussz = CRACEN_RSA_KEY_OPSZ(rsa_key);
	struct rsa_oaep_workmem workmem;

	rsa_oaep_encrypt_init(&workmem, digestsz);
	if (WORKMEM_SIZE < modulussz + digestsz + 4) {
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
	/* hash the label, even when the label is empty */
	if (label) {
		sx_status = cracen_hash_input(label->bytes, label->sz, hashalg, workmem.datablock);
	} else {
		sx_status = cracen_hash_input(0, 0, hashalg, workmem.datablock);
	}
	if (sx_status != SX_OK) {
		safe_memzero(workmem.workmem, sizeof(workmem.workmem));
		return sx_status;
	}
	/* start encoding and request generation of the random seed */

	/* pointer used to walk through the data block DB */
	uint8_t *datab = workmem.datablock + digestsz;
	size_t datablocksz = modulussz - digestsz - 1;
	size_t paddingstrsz = datablocksz - (digestsz + text->sz + 1);

	/* write the padding string PS, consisting of zero octets */
	safe_memzero(datab, paddingstrsz);

	datab += paddingstrsz;

	/* write the 0x01 octet that follows PS in DB */
	*datab++ = 1;

	/* copy message in DB */
	memcpy(datab, text->addr, text->sz);

	/* ask the PRNG in the env to generate a seed of length digestsz */
	psa_status = cracen_get_random(NULL, workmem.seed, digestsz);
	if (psa_status != PSA_SUCCESS) {
		safe_memzero(workmem.workmem, sizeof(workmem.workmem));
		return SX_ERR_UNKNOWN_ERROR;
	}

	sx_status =
		cracen_run_mgf1xor(workmem.workmem, sizeof(workmem.workmem), hashalg, workmem.seed,
				   digestsz, workmem.datablock, modulussz - digestsz - 1);
	if (sx_status != SX_OK) {
		safe_memzero(workmem.workmem, sizeof(workmem.workmem));
		return sx_status;
	}

	sx_status = cracen_run_mgf1xor(workmem.workmem, sizeof(workmem.workmem), hashalg,
				       workmem.datablock, modulussz - digestsz - 1,
				       workmem.wmem + 1, digestsz);
	if (sx_status != SX_OK) {
		safe_memzero(workmem.workmem, sizeof(workmem.workmem));
		return sx_status;
	}

	/* write 0x00 in the first byte of EM */
	workmem.wmem[0] = 0;
	/* the encoded message EM is now ready at address workmem.wmem */

	/* modular exponentiation m^e mod n (RSAEP encryption primitive) */
	int input_sizes[NUMBER_OF_SLOTS];
	sx_pk_req req;
	struct sx_pk_slot inputs[NUMBER_OF_SLOTS];

	sx_pk_acquire_hw(&req);
	/* modular exponentiation m^d mod n (RSASP1 sign primitive) */
	sx_status =
		cracen_rsa_modexp(&req, inputs, rsa_key, workmem.wmem, modulussz, input_sizes);
	if (sx_status != SX_OK) {
		safe_memzero(workmem.workmem, sizeof(workmem.workmem));
		sx_pk_release_req(&req);
		return sx_status;
	}

	/* copy output of exponentiation (the ciphertext) to workmem */
	const uint8_t **outputs = (const uint8_t **)sx_pk_get_output_ops(&req);
	const int opsz = sx_pk_get_opsize(&req);

	sx_rdpkmem(workmem.wmem, outputs[0], opsz);
	sx_pk_release_req(&req);

	memcpy(output, workmem.wmem, opsz);
	*output_length = opsz;
	safe_memzero(workmem.workmem, sizeof(workmem.workmem));
	return SX_OK;
}
