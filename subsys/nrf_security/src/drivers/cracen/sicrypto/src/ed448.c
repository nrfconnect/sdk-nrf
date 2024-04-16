/** Ed448 signature scheme and public key computation. Based on RFC 8032.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * The comments in this file use the notations and conventions from RFC 8032.
 *
 * Workmem layout for an Ed448 signature verification task:
 *      1. This area (size: 114 bytes) is first used to hold the dom4() string.
 *         Then it is used to store the output of the shake256 operation (a
 *         digest of 114 bytes).
 *
 * Workmem layout for an Ed448 signature generation task:
 *      The workmem is made of 5 areas of 57 bytes each (total size 285 bytes).
 *      In the following we refer to these areas using the numbers 1 to 5.
 *      - The first hash operation computes the private key's digest, which is
 *        stored in areas 1 and 2.
 *      - The second hash operation computes r. Prior to this, the dom4()
 *        string, which is an input to the hash operation, is written to area 4.
 *        The hash output r is stored in areas 4 and 5.
 *      - The first point multiplication computes R, which is written directly
 *        to the output buffer. Then the secret scalar s is computed in place in
 *        area 1. Area 2 is cleared.
 *      - The second point multiplication computes the public key A, which is
 *        stored in area 2.
 *      - The third hash operation computes k. Prior to this, the dom4() string,
 *        which is an input to the hash operation, is written to area 3. The
 *        hash output k is stored in areas 2 and 3.
 *      - The final operation computes S by doing (r + k * s) mod L. The
 *        computed S is written directly to the output buffer.
 *
 * Workmem layout for an Ed448 public key generation task:
 *      1. digest (size: 114 bytes). The digest of the private key is written in
 *         this area. Then the secret scalar s is computed in place in the first
 *         57 bytes of this area.
 *
 * Implementation notes:
 * - Ed448ph is not supported.
 * - The context input is fixed to the empty string.
 */

#include <string.h>
#include <sxsymcrypt/sha3.h>
#include <silexpk/core.h>
#include <silexpk/ed448.h>
#include <sicrypto/sicrypto.h>
#include <cracen/statuscodes.h>
#include <cracen/mem_helpers.h>
#include <sicrypto/ed448.h>
#include <sicrypto/hash.h>
#include "waitqueue.h"
#include "sigdefs.h"
#include "final.h"
#include "util.h"

/** Size in bytes of dom4(), context excluded. */
#define DOM4_PREFIX_SZ 10

static int finish_ed448_ver(struct sitask *t, struct siwq *wq)
{
	(void)wq;
	sx_pk_release_req(t->pk);
	return t->statuscode;
}

static int continue_ed448_ver(struct sitask *t, struct siwq *wq)
{
	struct sx_pk_acq_req pkreq;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	si_wq_run_after(t, &t->params.ed448_verif.wq, finish_ed448_ver);

	pkreq = sx_async_ed448_verify_go(
		(struct sx_ed448_dgst *)t->workmem, t->params.ed448_verif.pubkey,
		(const struct sx_ed448_v *)(t->params.ed448_verif.signature->r + SX_ED448_PT_SZ),
		(const struct sx_ed448_pt *)t->params.ed448_verif.signature->r);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}

	t->pk = pkreq.req;
	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;
	t->statuscode = SX_ERR_HW_PROCESSING;

	return SX_ERR_HW_PROCESSING;
}

static void run_ed448_ver(struct sitask *t)
{
	si_wq_run_after(t, &t->params.ed448_verif.wq, continue_ed448_ver);

	/* Override statuscode to be able to execute si_task_produce(). */
	t->statuscode = SX_ERR_READY;
	si_task_produce(t, t->workmem, SX_ED448_DGST_SZ);
}

static void write_dom4(char *dst)
{
	unsigned int i;
	static const char dom4_string[8] = "SigEd448";

	/* Write domain string to destination address. */
	for (i = 0; i < sizeof(dom4_string); i++) {
		*dst++ = dom4_string[i];
	}

	/* Write phflag: 0 because we do Ed448 (PureEdDSA) and not Ed448ph. */
	*dst++ = 0;

	/* Write context size: 0 because an empty context is used. */
	*dst++ = 0;
}

static void si_sig_create_ed448_verify(struct sitask *t, const struct si_sig_pubkey *pubkey,
				       const struct si_sig_signature *signature)
{
	if (t->workmemsz < SX_ED448_DGST_SZ) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	write_dom4(t->workmem);

	si_hash_create(t, &sxhashalg_shake256_114);
	si_task_consume(t, t->workmem, DOM4_PREFIX_SZ);
	si_task_consume(t, signature->r, SX_ED448_PT_SZ);
	si_task_consume(t, pubkey->key.ed448->encoded, SX_ED448_PT_SZ);
	t->actions.run = &run_ed448_ver;
	t->params.ed448_verif.pubkey = pubkey->key.ed448;
	t->params.ed448_verif.signature = signature;
}

static int ed448_compute_pubkey(struct sitask *t)
{
	struct sx_pk_acq_req pkreq;

	/* The secret scalar s is computed in place from the first half of the
	 * private key digest.
	 */
	t->workmem[0] &= ~0x03;		     /* clear bits 0 and 1 */
	t->workmem[SX_ED448_SZ - 1] = 0;     /* clear last octet */
	t->workmem[SX_ED448_SZ - 2] |= 0x80; /* set bit 447 */

	/* Clear second half of private key digest: sx_async_ed448_ptmult_go()
	 * works on an input of SX_ED448_DGST_SZ bytes.
	 */
	safe_memset(t->workmem + SX_ED448_SZ, t->workmemsz - SX_ED448_SZ, 0, SX_ED448_SZ);

	/* Perform point multiplication A = [s]B, to obtain the public key A. */
	pkreq = sx_async_ed448_ptmult_go((const struct sx_ed448_dgst *)t->workmem);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}

	t->pk = pkreq.req;

	return (t->statuscode = SX_ERR_HW_PROCESSING);
}

static int ed448_sign_finish(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* Get the second part of the signature, the encoded S. */
	sx_async_ed448_sign_end(
		t->pk, (struct sx_ed448_v *)(t->params.ed448_sign.signature->r + SX_ED448_PT_SZ));

	return t->statuscode;
}

static int ed448_sign_continue(struct sitask *t, struct siwq *wq)
{
	struct sx_pk_acq_req pkreq;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	si_wq_run_after(t, &t->params.ed448_sign.wq, ed448_sign_finish);

	/* Compute (r + k * s) mod L. This gives the second part of the
	 * signature, which is the encoded S.
	 */
	pkreq = sx_pk_async_ed448_sign_go(
		(const struct sx_ed448_dgst *)(t->workmem + SX_ED448_SZ),
		(const struct sx_ed448_dgst *)(t->workmem + 3 * SX_ED448_SZ),
		(const struct sx_ed448_v *)t->workmem);

	t->pk = pkreq.req;
	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;
	t->statuscode = SX_ERR_HW_PROCESSING;

	return SX_ERR_HW_PROCESSING;
}

static int ed448_sign_k_hash(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* Get result of the point multiplication. This is the encoding of the
	 * [s]B point, which is the public key A.
	 */
	sx_async_ed448_ptmult_end(t->pk, (struct sx_ed448_pt *)(t->workmem + SX_ED448_SZ));

	si_wq_run_after(t, &t->params.ed448_sign.wq, ed448_sign_continue);

	write_dom4(t->workmem + 2 * SX_ED448_SZ);

	/* Obtain k by hashing (dom4, R || A || message). */
	si_hash_create(t, &sxhashalg_shake256_114);
	si_task_consume(t, t->workmem + 2 * SX_ED448_SZ, DOM4_PREFIX_SZ);
	si_task_consume(t, t->params.ed448_sign.signature->r, SX_ED448_SZ);
	si_task_consume(t, t->workmem + SX_ED448_SZ, SX_ED448_SZ);
	si_task_consume(t, t->params.ed448_sign.msg, t->params.ed448_sign.msgsz);
	si_task_produce(t, t->workmem + SX_ED448_SZ, SX_ED448_DGST_SZ);

	return SX_ERR_HW_PROCESSING;
}

static int ed448_sign_pubkey_ptmult(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* Get result of the point multiplication. This is the encoded point R,
	 * which is the first part of the signature.
	 */
	sx_async_ed448_ptmult_end(t->pk, (struct sx_ed448_pt *)t->params.ed448_sign.signature->r);

	si_wq_run_after(t, &t->params.ed448_sign.wq, ed448_sign_k_hash);

	return ed448_compute_pubkey(t);
}

static int ed448_sign_r_ptmult(struct sitask *t, struct siwq *wq)
{
	struct sx_pk_acq_req pkreq;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	si_wq_run_after(t, &t->params.ed448_sign.wq, ed448_sign_pubkey_ptmult);

	/* Perform point multiplication R = [r]B. */
	pkreq = sx_async_ed448_ptmult_go(
		(const struct sx_ed448_dgst *)(t->workmem + 3 * SX_ED448_SZ));
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}

	t->pk = pkreq.req;
	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;

	return (t->statuscode = SX_ERR_HW_PROCESSING);
}

static int ed448_sign_hash_message(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	si_wq_run_after(t, &t->params.ed448_sign.wq, ed448_sign_r_ptmult);

	write_dom4(t->workmem + 3 * SX_ED448_SZ);

	/* Obtain r by hashing (dom4 || prefix || message), where prefix is the
	 * second half of the private key digest.
	 */
	si_hash_create(t, &sxhashalg_shake256_114);
	si_task_consume(t, t->workmem + 3 * SX_ED448_SZ, DOM4_PREFIX_SZ);
	si_task_consume(t, t->workmem + SX_ED448_SZ, SX_ED448_SZ);
	si_task_consume(t, t->params.ed448_sign.msg, t->params.ed448_sign.msgsz);
	si_task_produce(t, t->workmem + 3 * SX_ED448_SZ, SX_ED448_DGST_SZ);

	return SX_ERR_HW_PROCESSING;
}

static void ed448_sign_hash_privkey(struct sitask *t)
{
	si_wq_run_after(t, &t->params.ed448_sign.wq, ed448_sign_hash_message);

	/* Hash the private key. */
	si_hash_create(t, &sxhashalg_shake256_114);
	si_task_consume(t, t->params.ed448_sign.privkey->bytes, SX_ED448_SZ);
	si_task_produce(t, t->workmem, SX_ED448_DGST_SZ);
}

static void ed448_sign_consume(struct sitask *t, const char *data, size_t sz)
{
	t->params.ed448_sign.msg = data;
	t->params.ed448_sign.msgsz = sz;
	t->actions.consume = NULL;
}

static void si_sig_create_ed448_sign(struct sitask *t, const struct si_sig_privkey *privkey,
				     struct si_sig_signature *signature)
{
	if (t->workmemsz < 285) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	t->params.ed448_sign.privkey = privkey->key.ed448;
	t->params.ed448_sign.signature = signature;
	t->params.ed448_sign.msg = NULL;
	t->params.ed448_sign.msgsz = 0;

	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;

	t->actions.consume = ed448_sign_consume;
	t->actions.run = ed448_sign_hash_privkey;
}

static int ed448_genpubkey_finish(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* Get result of the point multiplication. This is the encoding of the
	 * [s]B point, which is the public key.
	 */
	sx_async_ed448_ptmult_end(t->pk, t->params.ed448_pubkey.pubkey);

	return t->statuscode;
}

static int ed448_genpubkey_pointmul(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	si_wq_run_after(t, &t->params.ed448_pubkey.wq, ed448_genpubkey_finish);

	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;

	return ed448_compute_pubkey(t);
}

static void ed448_genpubkey_hash_privkey(struct sitask *t)
{
	si_wq_run_after(t, &t->params.ed448_pubkey.wq, ed448_genpubkey_pointmul);

	si_hash_create(t, &sxhashalg_shake256_114);
	si_task_consume(t, t->params.ed448_pubkey.privkey->bytes, SX_ED448_SZ);
	si_task_produce(t, t->workmem, SX_ED448_DGST_SZ);
}

static void si_sig_create_ed448_pubkey(struct sitask *t, const struct si_sig_privkey *privkey,
				       struct si_sig_pubkey *pubkey)
{
	if (t->workmemsz < SX_ED448_DGST_SZ) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;
	t->actions.run = ed448_genpubkey_hash_privkey;
	t->params.ed448_pubkey.privkey = privkey->key.ed448;
	t->params.ed448_pubkey.pubkey = pubkey->key.ed448;
}

static unsigned short int get_ed448_key_opsz(const struct si_sig_privkey *privkey)
{
	(void)privkey;
	return SX_ED448_SZ;
}

const struct si_sig_def *const si_sig_def_ed448 = &(const struct si_sig_def){
	.sign = si_sig_create_ed448_sign,
	.verify = si_sig_create_ed448_verify,
	.pubkey = si_sig_create_ed448_pubkey,
	.getkeyopsz = get_ed448_key_opsz,
	.sigcomponents = 2,
};
