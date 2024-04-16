/* Signature scheme RSASSA-PSS, based on RFC 8017.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
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
#include "../include/sicrypto/sicrypto.h"
#include "../include/sicrypto/hash.h"
#include "../include/sicrypto/rsapss.h"
#include <cracen/statuscodes.h>
#include <cracen/mem_helpers.h>
#include <cracen_psa.h>
#include "../include/sicrypto/mem.h"
#include "rsamgf1xor.h"
#include "waitqueue.h"
#include "final.h"
#include "util.h"
#include "rsakey.h"
#include "sigdefs.h"

#define ZERO_PADDING_BYTES 8

/* Size of the workmem of the MGF1XOR subtask. */
#define MGF1XOR_WORKMEM_SZ(digestsz) ((digestsz) + 4)

static int rsapss_sign_finish(struct sitask *t, struct siwq *wq);

/* Return a pointer to the part of workmem that is specific to RSA PSS. */
static inline char *get_workmem_pointer(struct sitask *t, size_t digestsz)
{
	return t->workmem + MGF1XOR_WORKMEM_SZ(digestsz);
}

static inline size_t get_workmem_size(struct sitask *t, size_t digestsz)
{
	return t->workmemsz - MGF1XOR_WORKMEM_SZ(digestsz);
}

/* return 0 if and only if all elements in array 'a' are equal to 'val' */
static int diff_array_val(const char *a, char val, size_t sz)
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
static unsigned char create_msb_mask(struct sitask *t)
{
	unsigned int mask;
	unsigned int shift;

	shift = si_ffkey_bitsz(t->params.rsapss.key);
	shift = (shift + 7) % 8;
	mask = (0xFF << shift) & 0xFF;

	return (unsigned char)mask;
}

static int rsapss_verify_final_check(struct sitask *t, struct siwq *wq)
{
	int r;
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	struct siparams_rsapss *par = &t->params.rsapss;
	size_t masksz = par->emsz - digestsz - 1;
	char *wmem = get_workmem_pointer(t, digestsz);
	char *H = wmem + SI_RSA_KEY_OPSZ(par->key) - par->emsz + masksz;
	char *H1 = wmem + SI_RSA_KEY_OPSZ(par->key);
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	/* step 14 of EMSA-PSS-VERIFY in RFC 8017 */
	r = si_memdiff(H, H1, digestsz);
	if (r == 0) {
		return si_task_mark_final(t, SX_OK);
	} else {
		return si_task_mark_final(t, SX_ERR_INVALID_SIGNATURE);
	}
}

static int rsapss_verify_finish(struct sitask *t, struct siwq *wq)
{
	int r;
	size_t zero_len;
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	struct siparams_rsapss *par = &t->params.rsapss;
	size_t modulussz = SI_RSA_KEY_OPSZ(par->key);
	size_t masksz = par->emsz - digestsz - 1;
	char *wmem = get_workmem_pointer(t, digestsz);
	unsigned char bitmask;
	char *mHash, *zeros, *db;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	mHash = wmem + modulussz;
	db = wmem + modulussz - par->emsz;

	/* Step 9 of EMSA-PSS-VERIFY in RFC 8017 (clear the leftmost bits in DB
	 * according to bitmask). This works also when emsz equals modulussz - 1.
	 */
	bitmask = create_msb_mask(t);
	wmem[0] &= ~bitmask;

	/* number of bytes in DB that must be equal to zero */
	zero_len = masksz - par->saltsz - 1;

	/* step 10 of EMSA-PSS-VERIFY in RFC 8017 */
	r = diff_array_val(db, 0, zero_len);
	r |= (db[zero_len] != 1);
	if (r) {
		return si_task_mark_final(t, SX_ERR_INVALID_SIGNATURE);
	}

	/* zero padding for next hash: reuse mgf1xor workmem */
	zeros = t->workmem;
	safe_memset(zeros, t->workmemsz, 0, ZERO_PADDING_BYTES);

	/* hash task to produce H' (step 13 of EMSA-PSS-VERIFY in RFC 8017) */
	si_hash_create(t, t->hashalg);
	si_task_consume(t, zeros, ZERO_PADDING_BYTES);
	si_task_consume(t, mHash, digestsz);
	si_task_consume(t, db + masksz - par->saltsz, par->saltsz);
	si_task_produce(t, mHash, digestsz); /* H' overwrites mHash */

	si_wq_run_after(t, &t->params.rsapss.wq, rsapss_verify_final_check);

	return SX_ERR_HW_PROCESSING;
}

static int rsapss_mgf1_start(struct sitask *t)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	size_t modulussz = SI_RSA_KEY_OPSZ(t->params.rsapss.key);
	size_t masksz = t->params.rsapss.emsz - digestsz - 1;
	char *wmem = get_workmem_pointer(t, digestsz);
	char *seed = wmem + modulussz - digestsz - 1;
	char *xorinout = wmem + modulussz - t->params.rsapss.emsz;

	si_create_mgf1xor(t, t->hashalg);
	/* the seed for MGF1 is what RFC 8017 calls H */
	si_task_consume(t, seed, digestsz);
	si_task_produce(t, xorinout, masksz);

	return SX_ERR_HW_PROCESSING;
}

static int rsapss_sign_mgf1_start(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	si_wq_run_after(t, &t->params.rsapss.wq, rsapss_sign_finish);

	return rsapss_mgf1_start(t);
}

/** Get modexp result, perform some checks, start mask computation. */
static int rsapss_verify_continue(struct sitask *t, struct siwq *wq)
{
	struct siparams_rsapss *par = &t->params.rsapss;
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	char *wmem = get_workmem_pointer(t, digestsz);
	unsigned char bitmask;
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* the output of the exponentiation is the encoded message EM. */
	const char **outputs = sx_pk_get_output_ops(t->pk);
	const int opsz = sx_pk_get_opsize(t->pk);

	sx_rdpkmem(wmem, outputs[0], opsz);
	sx_pk_release_req(t->pk);

	/* invalid signature if rightmost byte of EM is not 0xBC (step 4 of
	 * EMSA-PSS-VERIFY in RFC 8017)
	 */
	if (((unsigned char)wmem[SI_RSA_KEY_OPSZ(par->key) - 1]) != 0xBC) {
		return si_task_mark_final(t, SX_ERR_INVALID_SIGNATURE);
	}

	/* Step 6 of EMSA-PSS-VERIFY in RFC 8017: test the leftmost bits in EM
	 * according to bitmask. This works also when emsz equals modulussz - 1.
	 */
	bitmask = create_msb_mask(t);
	if (((unsigned char)wmem[0]) & bitmask) {
		return si_task_mark_final(t, SX_ERR_INVALID_SIGNATURE);
	}

	si_wq_run_after(t, &t->params.rsapss.wq, rsapss_verify_finish);

	return rsapss_mgf1_start(t);
}

static const struct siactions rsapss_actions_silexpk = {.status = &si_silexpk_status,
							.wait = &si_silexpk_wait};

/** Modular exponentiation (base^key mod n).
 *
 * This function is used by both the sign and the verify tasks. Note: if the
 * base is greater than the modulus, SilexPK will return the SX_ERR_OUT_OF_RANGE
 * status code.
 */
static int rsapss_start_modexp(struct sitask *t, const char *base, size_t basesz)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_slot inputs[6];
	const struct si_rsa_key *key = t->params.rsapss.key;

	t->actions = rsapss_actions_silexpk;

	pkreq = sx_pk_acquire_req(key->cmd);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}
	int sizes[6];

	si_ffkey_write_sz(key, sizes);
	SI_FFKEY_REFER_INPUT(key, sizes) = basesz;
	pkreq.status = sx_pk_list_gfp_inslots(pkreq.req, sizes, inputs);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}

	/* copy modulus and exponent to device memory */
	si_ffkey_write(key, inputs);
	sx_wrpkmem(SI_FFKEY_REFER_INPUT(key, inputs).addr, base, basesz);

	t->statuscode = SX_ERR_HW_PROCESSING;
	t->pk = pkreq.req;
	sx_pk_run(t->pk);

	return SX_ERR_HW_PROCESSING;
}

/** Start computation of modular exponentiation (RSAVP1 verify primitive). */
static void rsapss_verify_start_primitive(struct sitask *t)
{
	struct siparams_rsapss *par = &t->params.rsapss;

	si_wq_run_after(t, &t->params.rsapss.wq, rsapss_verify_continue);

	/* Modular exponentiation s^e mod n. */
	rsapss_start_modexp(t, par->sig->r, par->sig->sz);
}

static void rsapss_hash_message(struct sitask *t)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	char *wmem = get_workmem_pointer(t, digestsz);
	char *mHash = wmem + SI_RSA_KEY_OPSZ(t->params.rsapss.key);

	/* Override the statuscode value to be able to run si_task_produce(). */
	t->statuscode = SX_ERR_READY;

	/* Hash input message: mHash = hash(M). Digest written inside workmem.
	 */
	si_task_produce(t, mHash, digestsz);
}

static int rsapss_verify_init(struct sitask *t, const struct si_sig_pubkey *pubkey,
			      const struct si_sig_signature *signature)
{
	struct siparams_rsapss *par = &t->params.rsapss;
	size_t digestsz = sx_hash_get_alg_digestsz(pubkey->hashalg);
	const struct si_rsa_key *rsakey = &pubkey->key.rsa;
	size_t modulussz = SI_RSA_KEY_OPSZ(rsakey);

	if (t->workmemsz < modulussz + 2 * digestsz + 4) {
		return si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
	}

	/* assumption: the most significant byte of the modulus is not zero */
	if (((si_ffkey_bitsz(rsakey) + 7) >> 3) != modulussz) {
		return si_task_mark_final(t, SX_ERR_INVALID_ARG);
	}

	/* emsz: size in bytes of the encoded message (called emLen in RFC 8017)
	 */
	if (si_ffkey_bitsz(rsakey) % 8 == 1) {
		par->emsz = modulussz - 1;
	} else {
		par->emsz = modulussz;
	}

	/* step 3 of EMSA-PSS-VERIFY in RFC 8017 */
	if (par->emsz < digestsz + pubkey->saltsz + 2) {
		return si_task_mark_final(t, SX_ERR_INVALID_SIGNATURE);
	}

	par->key = rsakey;
	par->sig = (struct si_sig_signature *)signature;
	par->saltsz = pubkey->saltsz;
	t->hashalg = pubkey->hashalg;

	return SX_OK;
}

static void rsapss_verify_consume_digest(struct sitask *t, const char *data, size_t sz)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	char *wmem = get_workmem_pointer(t, digestsz);
	char *mHash = wmem + SI_RSA_KEY_OPSZ(t->params.rsapss.key);

	if (sz < digestsz) {
		si_task_mark_final(t, SX_ERR_INPUT_BUFFER_TOO_SMALL);
		return;
	}

	if (sz > digestsz) {
		si_task_mark_final(t, SX_ERR_TOO_BIG);
		return;
	}

	/* Copy the message digest to workmem. */
	memcpy(mHash, data, sz);

	t->actions.consume = NULL;
	t->actions.run = rsapss_verify_start_primitive;
}

static void si_sig_create_rsa_pss_verify_digest(struct sitask *t,
						const struct si_sig_pubkey *pubkey,
						const struct si_sig_signature *signature)
{
	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;
	t->actions.consume = rsapss_verify_consume_digest;

	rsapss_verify_init(t, pubkey, signature);
}

static int rsapss_verify_start(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	rsapss_verify_start_primitive(t);

	return t->statuscode;
}

static void si_sig_create_rsa_pss_verify(struct sitask *t, const struct si_sig_pubkey *pubkey,
					 const struct si_sig_signature *signature)
{
	int r;

	r = rsapss_verify_init(t, pubkey, signature);
	if (r != SX_OK) {
		return;
	}

	si_wq_run_after(t, &t->params.rsapss.wq, rsapss_verify_start);

	si_hash_create(t, t->hashalg);
	t->actions.run = rsapss_hash_message;
}

/** Get result of modular exponentiation, placing it in user's output buffer. */
static int rsapss_sign_exp_finish(struct sitask *t, struct siwq *wq)
{
	struct siparams_rsapss *par = &t->params.rsapss;
	(void)wq;

	/* the output of the exponentiation is the signature.*/
	const char **outputs = sx_pk_get_output_ops(t->pk);
	const int opsz = sx_pk_get_opsize(t->pk);

	sx_rdpkmem(par->sig->r, outputs[0], opsz);
	par->sig->sz = opsz;
	/* releases the HW resource so it has to be
	 * called even if an error occurred
	 */
	sx_pk_release_req(t->pk);

	return t->statuscode;
}

static int rsapss_sign_finish(struct sitask *t, struct siwq *wq)
{
	size_t keysz = SI_RSA_KEY_OPSZ(t->params.rsapss.key);
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	char *wmem = get_workmem_pointer(t, digestsz);
	unsigned char bitmask;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	/* step 11 of EMSA-PSS-ENCODE in RFC 8017 (clear the leftmost bits in
	 * maskedDB according to bitmask). This works also when emsz equals
	 * modulussz - 1.
	 */
	bitmask = create_msb_mask(t);
	wmem[0] &= ~bitmask;

	/* set to 0xBC the last byte of EM (step 12 of EMSA-PSS-ENCODE) */
	((unsigned char *)(wmem))[keysz - 1] = 0xBC;

	si_wq_run_after(t, &t->params.rsapss.wq, rsapss_sign_exp_finish);

	/* modular exponentiation m^d mod n (RSASP1 sign primitive) */
	return rsapss_start_modexp(t, wmem, keysz);
}

/** Prepare DB and launch H = hash(M'), for the signature generation task. */
static int rsapss_sign_continue(struct sitask *t, struct siwq *wq)
{
	struct siparams_rsapss *par = &t->params.rsapss;
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	size_t masksz = par->emsz - digestsz - 1;
	char *wmem = get_workmem_pointer(t, digestsz);
	const size_t wmem_sz = get_workmem_size(t, digestsz);
	const size_t padding_offset = SI_RSA_KEY_OPSZ(par->key) - ZERO_PADDING_BYTES;
	char *padding = wmem + padding_offset;
	char *salt = wmem + SI_RSA_KEY_OPSZ(par->key) - par->emsz + masksz - par->saltsz;
	char *H = wmem + SI_RSA_KEY_OPSZ(par->key) - digestsz - 1;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	/* prepare padding string PS (step 7 of EMSA-PSS-ENCODE) */
	const size_t padding_ps_offset = SI_RSA_KEY_OPSZ(par->key) - par->emsz;

	safe_memset(wmem + padding_ps_offset, wmem_sz - padding_ps_offset, 0,
		    masksz - par->saltsz - 1);

	/* step 8 of EMSA-PSS-ENCODE (padding string and salt are already there)
	 */
	*(salt - 1) = 1;

	/* prepare padding */
	safe_memset(padding, wmem_sz - padding_offset, 0, ZERO_PADDING_BYTES);

	/* hash task */
	si_hash_create(t, t->hashalg);
	si_task_consume(t, padding, ZERO_PADDING_BYTES + digestsz);
	si_task_consume(t, salt, par->saltsz);
	si_task_produce(t, H, digestsz);

	si_wq_run_after(t, &t->params.rsapss.wq, rsapss_sign_mgf1_start);

	return SX_ERR_HW_PROCESSING;
}

static void rsapss_sign_get_salt(struct sitask *t)
{
	struct siparams_rsapss *par = &t->params.rsapss;
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	size_t masksz = par->emsz - digestsz - 1;
	char *wmem = get_workmem_pointer(t, digestsz);
	char *salt = wmem + SI_RSA_KEY_OPSZ(par->key) - par->emsz + masksz - par->saltsz;
	psa_status_t status = PSA_ERROR_CORRUPTION_DETECTED;

	/* ask the PRNG task (part of the environment) to generate the salt */
	status = cracen_get_random(NULL, salt, par->saltsz);
	if (status != PSA_SUCCESS) {
		si_task_mark_final(t, SX_ERR_UNKNOWN_ERROR);
		return;
	}

	/* set t->statuscode as if task t was waiting for the HW */
	t->statuscode = SX_ERR_HW_PROCESSING;

	si_wq_run_after(t, &t->params.rsapss.wq, rsapss_sign_continue);
	t->actions.status = &si_status_on_prng;
	t->actions.wait = &si_wait_on_prng;
}

static int rsapss_sign_init(struct sitask *t, const struct si_sig_privkey *privkey,
			    struct si_sig_signature *signature)
{
	struct siparams_rsapss *par = &t->params.rsapss;
	size_t digestsz = sx_hash_get_alg_digestsz(privkey->hashalg);
	const struct si_rsa_key *rsakey = &privkey->key.rsa;
	size_t modulussz = SI_RSA_KEY_OPSZ(rsakey);

	if (t->workmemsz < modulussz + 2 * digestsz + 4) {
		return si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
	}

	/* assumption: the most significant byte of the modulus is not zero */
	if (((si_ffkey_bitsz(rsakey) + 7) >> 3) != modulussz) {
		return si_task_mark_final(t, SX_ERR_INVALID_ARG);
	}

	/* emsz: size in bytes of the encoded message (called emLen in RFC 8017)
	 */
	if (si_ffkey_bitsz(rsakey) % 8 == 1) {
		par->emsz = modulussz - 1;
	} else {
		par->emsz = modulussz;
	}

	/* step 3 of EMSA-PSS-ENCODE in RFC 8017 */
	if (par->emsz < digestsz + privkey->saltsz + 2) {
		return si_task_mark_final(t, SX_ERR_INPUT_BUFFER_TOO_SMALL);
	}

	/* guarantee there's enough room in signature to store it */
	if (modulussz > signature->sz) {
		return si_task_mark_final(t, SX_ERR_INPUT_BUFFER_TOO_SMALL);
	}

	par->key = rsakey;
	par->sig = signature;
	par->saltsz = privkey->saltsz;
	t->hashalg = privkey->hashalg;

	return SX_OK;
}

static void rsapss_sign_consume_digest(struct sitask *t, const char *data, size_t sz)
{
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	char *wmem = get_workmem_pointer(t, digestsz);
	char *mHash = wmem + SI_RSA_KEY_OPSZ(t->params.rsapss.key);

	if (sz < digestsz) {
		si_task_mark_final(t, SX_ERR_INPUT_BUFFER_TOO_SMALL);
		return;
	}

	if (sz > digestsz) {
		si_task_mark_final(t, SX_ERR_TOO_BIG);
		return;
	}

	/* Copy the message digest to workmem. */
	memcpy(mHash, data, sz);

	t->actions.consume = NULL;
	t->actions.run = rsapss_sign_get_salt;
}

static void si_sig_create_rsa_pss_sign_digest(struct sitask *t,
					      const struct si_sig_privkey *privkey,
					      struct si_sig_signature *signature)
{
	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;
	t->actions.consume = rsapss_sign_consume_digest;

	rsapss_sign_init(t, privkey, signature);
}

static int rsapss_sign_start(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	rsapss_sign_get_salt(t);

	return t->statuscode;
}

static void si_sig_create_rsa_pss_sign(struct sitask *t, const struct si_sig_privkey *privkey,
				       struct si_sig_signature *signature)
{
	int r;

	r = rsapss_sign_init(t, privkey, signature);
	if (r != SX_OK) {
		return;
	}

	si_wq_run_after(t, &t->params.rsapss.wq, rsapss_sign_start);

	si_hash_create(t, t->hashalg);
	t->actions.run = rsapss_hash_message;
}

static unsigned short int get_key_opsz(const struct si_sig_privkey *privkey)
{
	const struct si_rsa_key *key = &(privkey->key.rsa);

	return SI_RSA_KEY_OPSZ(key);
}

const struct si_sig_def *const si_sig_def_rsa_pss = &(const struct si_sig_def){
	.sign = si_sig_create_rsa_pss_sign,
	.sign_digest = si_sig_create_rsa_pss_sign_digest,
	.verify = si_sig_create_rsa_pss_verify,
	.verify_digest = si_sig_create_rsa_pss_verify_digest,
	.getkeyopsz = get_key_opsz,
	.sigcomponents = 1,
};
