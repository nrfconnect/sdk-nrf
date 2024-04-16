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
#include "../include/sicrypto/sicrypto.h"
#include "../include/sicrypto/hash.h"
#include "../include/sicrypto/rsaes_oaep.h"
#include <cracen/statuscodes.h>
#include <cracen/mem_helpers.h>
#include <cracen_psa.h>
#include "../include/sicrypto/mem.h"
#include "rsamgf1xor.h"
#include "waitqueue.h"
#include "rsakey.h"
#include "final.h"
#include "util.h"

/* Size of the workmem of the MGF1XOR subtask. */
#define MGF1XOR_WORKMEM_SZ(digestsz) ((digestsz) + 4)

/* Return a pointer to the part of workmem that is specific to RSA OAEP. */
static inline char *get_workmem_pointer(struct sitask *t, size_t digestsz)
{
	return t->workmem + MGF1XOR_WORKMEM_SZ(digestsz);
}

static inline size_t get_workmem_size(struct sitask *t, size_t digestsz)
{
	return t->workmemsz - MGF1XOR_WORKMEM_SZ(digestsz);
}

/* Modular exponentiation: base^key mod n */
static int rsaes_oaep_start_modexp(struct sitask *t, const char *base, size_t basesz)
{
	struct si_rsa_key *key = t->params.rsaoaep.key;
	struct sx_pk_acq_req pkreq;
	struct sx_pk_slot inputs[6];
	int sizes[6];

	pkreq = sx_pk_acquire_req(key->cmd);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}

	/* set up values in the sizes[] array */
	si_ffkey_write_sz(key, sizes);
	SI_FFKEY_REFER_INPUT(key, sizes) = basesz;

	pkreq.status = sx_pk_list_gfp_inslots(pkreq.req, sizes, inputs);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}

	/* copy the key elements to device memory */
	si_ffkey_write(key, inputs);

	/* copy the base operand to device memory */
	sx_wrpkmem(SI_FFKEY_REFER_INPUT(key, inputs).addr, base, basesz);

	t->statuscode = SX_ERR_HW_PROCESSING;
	t->pk = pkreq.req;
	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;

	/* start the modular exponentiation */
	sx_pk_run(t->pk);

	return SX_ERR_HW_PROCESSING;
}

static int rsaoaep_encrypt_finish(struct sitask *t, struct siwq *wq)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	char *wmem = get_workmem_pointer(t, digestsz);
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* copy output of exponentiation (the ciphertext) to workmem */
	const char **outputs = sx_pk_get_output_ops(t->pk);
	const int opsz = sx_pk_get_opsize(t->pk);

	sx_rdpkmem(wmem, outputs[0], opsz);
	sx_pk_release_req(t->pk);

	/* update the si_ase_text structure to point to the ciphertext */
	t->params.rsaoaep.text->addr = wmem;
	t->params.rsaoaep.text->sz = opsz;

	return SX_OK;
}

static int rsaoaep_encrypt_mod_exp(struct sitask *t, struct siwq *wq)
{
	size_t modulussz = SI_RSA_KEY_OPSZ(t->params.rsaoaep.key);
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	char *wmem = get_workmem_pointer(t, digestsz);
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	/* write 0x00 in the first byte of EM */
	wmem[0] = 0;
	/* the encoded message EM is now ready at address t->workmem */

	si_wq_run_after(t, &t->params.rsaoaep.wq, rsaoaep_encrypt_finish);

	/* modular exponentiation m^e mod n (RSAEP encryption primitive) */
	return rsaes_oaep_start_modexp(t, wmem, modulussz);
}

static int rsaoaep_encrypt_second_mgf(struct sitask *t, struct siwq *wq)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	size_t modulussz = SI_RSA_KEY_OPSZ(t->params.rsaoaep.key);
	char *wmem = get_workmem_pointer(t, digestsz);
	char *seed = wmem + digestsz + 1;
	char *xorinout = wmem + 1;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	si_wq_run_after(t, &t->params.rsaoaep.wq, rsaoaep_encrypt_mod_exp);

	si_create_mgf1xor(t, t->hashalg);
	si_task_consume(t, seed, modulussz - digestsz - 1);
	si_task_produce(t, xorinout, digestsz);

	return SX_ERR_HW_PROCESSING;
}

static int rsaoaep_encrypt_first_mgf(struct sitask *t, struct siwq *wq)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	size_t modulussz = SI_RSA_KEY_OPSZ(t->params.rsaoaep.key);
	char *wmem = get_workmem_pointer(t, digestsz);
	char *seed = wmem + 1;
	char *xorinout = wmem + digestsz + 1; /* same as start of DB */
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	si_wq_run_after(t, &t->params.rsaoaep.wq, rsaoaep_encrypt_second_mgf);

	si_create_mgf1xor(t, t->hashalg);
	si_task_consume(t, seed, digestsz);
	si_task_produce(t, xorinout, modulussz - digestsz - 1);

	return SX_ERR_HW_PROCESSING;
}

/* start encoding and request generation of the random seed */
static int rsaoaep_encrypt_get_seed(struct sitask *t, struct siwq *wq)
{
	size_t modulussz = SI_RSA_KEY_OPSZ(t->params.rsaoaep.key);
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	char *wmem = get_workmem_pointer(t, digestsz);
	const size_t wmem_size = get_workmem_size(t, digestsz);
	const size_t datablockstart_offset = digestsz + 1;
	char *const datablockstart = wmem + datablockstart_offset;
	size_t datablocksz = modulussz - digestsz - 1;
	struct si_ase_text *text = t->params.rsaoaep.text;
	size_t paddingstrsz = datablocksz - digestsz - text->sz - 1;
	char *seed = wmem + 1;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

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
	status = cracen_get_random(NULL, seed, digestsz);
	if (status != PSA_SUCCESS) {
		return si_task_mark_final(t, SX_ERR_UNKNOWN_ERROR);
	}

	/* set t->statuscode as if task t was waiting for the HW */
	t->statuscode = SX_ERR_HW_PROCESSING;

	si_wq_run_after(t, &t->params.rsaoaep.wq, rsaoaep_encrypt_first_mgf);
	t->actions.status = si_status_on_prng;
	t->actions.wait = si_wait_on_prng;

	return SX_ERR_HW_PROCESSING;
}

static void rsaoaep_encrypt_run(struct sitask *t)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	char *datablock = get_workmem_pointer(t, digestsz) + digestsz + 1;

	si_wq_run_after(t, &t->params.rsaoaep.wq, rsaoaep_encrypt_get_seed);

	/* hash the label */
	si_hash_create(t, t->hashalg);
	if (t->params.rsaoaep.label) {
		si_task_consume(t, t->params.rsaoaep.label->bytes, t->params.rsaoaep.label->sz);
	}
	si_task_produce(t, datablock, digestsz);
}

void si_ase_create_rsa_oaep_encrypt(struct sitask *t, const struct sxhashalg *hashalg,
				    struct si_rsa_key *pubkey, struct si_ase_text *text,
				    struct sx_buf *label)
{
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);
	size_t modulussz = SI_RSA_KEY_OPSZ(pubkey);

	if (t->workmemsz < modulussz + digestsz + 4) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	/* detect invalid combinations of key size and hash function */
	if (modulussz < 2 * digestsz + 2) {
		si_task_mark_final(t, SX_ERR_INVALID_ARG);
		return;
	}

	/* check message length: step 1.b of RSAES-OAEP-ENCRYPT in RFC 8017 */
	if (text->sz > modulussz - 2 * digestsz - 2) {
		si_task_mark_final(t, SX_ERR_TOO_BIG);
		return;
	}

	t->hashalg = hashalg;
	t->params.rsaoaep.key = pubkey;
	t->params.rsaoaep.text = text;
	t->params.rsaoaep.label = label;

	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;

	t->actions.run = rsaoaep_encrypt_run;
}

static int rsaoaep_decrypt_finish(struct sitask *t, struct siwq *wq)
{
	int r = 0;
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	size_t modulussz = SI_RSA_KEY_OPSZ(t->params.rsaoaep.key);
	char *wmem = get_workmem_pointer(t, digestsz);
	char *const datablockstart = wmem + digestsz + 1;
	char *const datablockend = datablockstart + modulussz - digestsz - 1;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	/* pointer used to walk through the data block DB */
	char *datab = datablockstart;

	/* the first octet in EM must be equal to 0x00 */
	r |= (wmem[0] != 0x00);

	/* check the label's digest */
	r |= si_memdiff(wmem + 1, datab, digestsz);
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
		return si_task_mark_final(t, SX_ERR_INVALID_CIPHERTEXT);
	}

	/* update the si_ase_text structure to point to the decrypted message */
	t->params.rsaoaep.text->addr = datab;
	t->params.rsaoaep.text->sz = datablockend - datab;

	return t->statuscode;
}

static int rsaoaep_decrypt_hashlabel(struct sitask *t, struct siwq *wq)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	char *wmem = get_workmem_pointer(t, digestsz);
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	si_wq_run_after(t, &t->params.rsaoaep.wq, rsaoaep_decrypt_finish);

	/* hash the label with result overwriting the seed */
	si_hash_create(t, t->hashalg);
	if (t->params.rsaoaep.label) {
		si_task_consume(t, t->params.rsaoaep.label->bytes, t->params.rsaoaep.label->sz);
	}
	si_task_produce(t, wmem + 1, sx_hash_get_alg_digestsz(t->hashalg));

	return SX_ERR_HW_PROCESSING;
}

static int rsaoaep_decrypt_second_mgf(struct sitask *t, struct siwq *wq)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	size_t modulussz = SI_RSA_KEY_OPSZ(t->params.rsaoaep.key);
	char *seed = get_workmem_pointer(t, digestsz) + 1;
	char *xorinout = seed + digestsz;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	si_wq_run_after(t, &t->params.rsaoaep.wq, rsaoaep_decrypt_hashlabel);

	si_create_mgf1xor(t, t->hashalg);
	si_task_consume(t, seed, digestsz);
	si_task_produce(t, xorinout, modulussz - digestsz - 1);

	return SX_ERR_HW_PROCESSING;
}

/* get the result of the exponentiation and start with the first MGF */
static int rsaoaep_decrypt_continue(struct sitask *t, struct siwq *wq)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	size_t modulussz = SI_RSA_KEY_OPSZ(t->params.rsaoaep.key);
	char *wmem = get_workmem_pointer(t, digestsz);
	char *xorinout = wmem + 1;
	char *seed = xorinout + digestsz;
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* copy output of exponentiation to workmem */
	const char **outputs = sx_pk_get_output_ops(t->pk);
	const int opsz = sx_pk_get_opsize(t->pk);

	sx_rdpkmem(wmem, outputs[0], opsz);
	sx_pk_release_req(t->pk);

	si_wq_run_after(t, &t->params.rsaoaep.wq, rsaoaep_decrypt_second_mgf);

	si_create_mgf1xor(t, t->hashalg);
	si_task_consume(t, seed, modulussz - digestsz - 1);
	si_task_produce(t, xorinout, digestsz);

	return SX_ERR_HW_PROCESSING;
}

static void rsaoaep_decrypt_run(struct sitask *t)
{
	si_wq_run_after(t, &t->params.rsaoaep.wq, rsaoaep_decrypt_continue);

	/* modular exponentiation c^d mod n (RSADP decryption primitive) */
	rsaes_oaep_start_modexp(t, t->params.rsaoaep.text->addr, t->params.rsaoaep.text->sz);
}

void si_ase_create_rsa_oaep_decrypt(struct sitask *t, const struct sxhashalg *hashalg,
				    struct si_rsa_key *privkey, struct si_ase_text *text,
				    struct sx_buf *label)
{
	size_t modulussz = SI_RSA_KEY_OPSZ(privkey);
	size_t digestsz = sx_hash_get_alg_digestsz(hashalg);

	if (t->workmemsz < modulussz + digestsz + 4) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	/* step 1.c of RSAES-OAEP-DECRYPT in RFC 8017 */
	if (modulussz < 2 * digestsz + 2) {
		si_task_mark_final(t, SX_ERR_INVALID_ARG);
		return;
	}

	/* the ciphertext must not be longer than the modulus (modified step 1.b
	 * of RSAES-OAEP-DECRYPT)
	 */
	if (text->sz > modulussz) {
		si_task_mark_final(t, SX_ERR_TOO_BIG);
		return;
	}

	t->hashalg = hashalg;
	t->params.rsaoaep.key = privkey;
	t->params.rsaoaep.text = text;
	t->params.rsaoaep.label = label;

	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;

	t->actions.run = rsaoaep_decrypt_run;
}
