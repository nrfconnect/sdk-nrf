/** Ed25519 signature generation/verification and public key
 * computation. Based on RFC 8032.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * The comments in this file use the notations and conventions from RFC 8032.
 *
 * Workmem layout for an Ed25519 signature verification task:
 *      1. digest (size: 64 bytes).
 *
 * Workmem layout for an Ed25519 signature generation task:
 *      The workmem is made of 5 areas of 32 bytes each (total size 160 bytes).
 *      In the following we refer to these areas using the numbers 1 to 5. The
 *      first hash operation computes the private key's digest, which is stored
 *      in areas 1 and 2. The second hash operation computes r, which is stored
 *      in areas 4 and 5. The first point multiplication computes R, which is
 *      written directly to the output buffer. Then the secret scalar s is
 *      computed in place in area 1. Area 2 is cleared. The second point
 *      multiplication computes the public key A, which is stored in area 2. The
 *      third hash operation computes k, which is stored in areas 2 and 3. The
 *      final operation (r + k * s) mod L computes S, which is written directly
 *      to the output buffer.
 *
 * Workmem layout for an Ed25519 public key generation task:
 *      1. digest (size: 64 bytes). The digest of the private key is written in
 *         this area. Then the secret scalar s is computed in place in the first
 *         32 bytes of this area.
 *
 * Implementation notes:
 * - Ed25519ph and Ed25519ctx are not supported.
 */

#include <string.h>
#include <sxsymcrypt/sha2.h>
#include <silexpk/core.h>
#include <silexpk/ed25519.h>
#include <sicrypto/sicrypto.h>
#include <cracen/statuscodes.h>
#include <cracen/mem_helpers.h>
#include <sicrypto/ed25519.h>
#include <sicrypto/hash.h>
#include "waitqueue.h"
#include "sigdefs.h"
#include "final.h"
#include "util.h"

static int finish_ed25519_ver(struct sitask *t, struct siwq *wq)
{
	(void)wq;
	sx_pk_release_req(t->pk);
	return t->statuscode;
}

static int continue_ed25519_ver(struct sitask *t, struct siwq *wq)
{
	struct sx_pk_acq_req pkreq;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	si_wq_run_after(t, &t->params.ed25519_verif.wq, finish_ed25519_ver);

	pkreq = sx_async_ed25519_verify_go(
		(struct sx_ed25519_dgst *)t->workmem, t->params.ed25519_verif.pubkey,
		(const struct sx_ed25519_v *)(t->params.ed25519_verif.signature->r +
					      SX_ED25519_PT_SZ),
		(const struct sx_ed25519_pt *)t->params.ed25519_verif.signature->r);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}

	t->pk = pkreq.req;
	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;
	t->statuscode = SX_ERR_HW_PROCESSING;

	return SX_ERR_HW_PROCESSING;
}

static void run_ed25519_ver(struct sitask *t)
{
	si_wq_run_after(t, &t->params.ed25519_verif.wq, continue_ed25519_ver);

	/* Override statuscode to be able to execute si_task_produce() */
	t->statuscode = SX_ERR_READY;
	si_task_produce(t, t->workmem, SX_ED25519_DGST_SZ);
}

static void si_sig_create_ed25519_verify(struct sitask *t, const struct si_sig_pubkey *pubkey,
					 const struct si_sig_signature *signature)
{
	if (t->workmemsz < SX_ED25519_DGST_SZ) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	si_hash_create(t, &sxhashalg_sha2_512);
	si_task_consume(t, signature->r, SX_ED25519_PT_SZ);
	si_task_consume(t, pubkey->key.ed25519->encoded, SX_ED25519_PT_SZ);
	t->actions.run = &run_ed25519_ver;
	t->params.ed25519_verif.pubkey = pubkey->key.ed25519;
	t->params.ed25519_verif.signature = signature;
}

static int ed25519_sign_finish(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* Get the second part of the signature, the encoded S. */
	sx_async_ed25519_sign_end(
		t->pk,
		(struct sx_ed25519_v *)(t->params.ed25519_sign.signature->r + SX_ED25519_PT_SZ));

	return t->statuscode;
}

static int ed25519_sign_continue(struct sitask *t, struct siwq *wq)
{
	struct sx_pk_acq_req pkreq;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	si_wq_run_after(t, &t->params.ed25519_sign.wq, ed25519_sign_finish);

	/* Compute (r + k * s) mod L. This gives the second part of the
	 * signature, which is the encoded S.
	 */
	pkreq = sx_pk_async_ed25519_sign_go(
		(const struct sx_ed25519_dgst *)(t->workmem + SX_ED25519_SZ),
		(const struct sx_ed25519_dgst *)(t->workmem + 3 * SX_ED25519_SZ),
		(const struct sx_ed25519_v *)t->workmem);

	t->pk = pkreq.req;
	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;
	t->statuscode = SX_ERR_HW_PROCESSING;

	return SX_ERR_HW_PROCESSING;
}

static int ed25519_sign_k_hash(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* Get result of the point multiplication. This is the encoding of the
	 * [s]B point, which is the public key A.
	 */
	sx_async_ed25519_ptmult_end(t->pk, (struct sx_ed25519_pt *)(t->workmem + SX_ED25519_SZ));

	si_wq_run_after(t, &t->params.ed25519_sign.wq, ed25519_sign_continue);

	/* Obtain k by hashing (R || A || message). */
	si_hash_create(t, &sxhashalg_sha2_512);
	si_task_consume(t, t->params.ed25519_sign.signature->r, SX_ED25519_SZ);
	si_task_consume(t, t->workmem + SX_ED25519_SZ, SX_ED25519_SZ);
	si_task_consume(t, t->params.ed25519_sign.msg, t->params.ed25519_sign.msgsz);
	si_task_produce(t, t->workmem + SX_ED25519_SZ, SX_ED25519_DGST_SZ);

	return SX_ERR_HW_PROCESSING;
}

static int ed25519_compute_pubkey(struct sitask *t)
{
	struct sx_pk_acq_req pkreq;

	/* The secret scalar s is computed in place from the first half of the
	 * private key digest.
	 */
	t->workmem[0] &= ~0x07;			/* clear bits 0, 1 and 2 */
	t->workmem[SX_ED25519_SZ - 1] &= ~0x80; /* clear bit 255 */
	t->workmem[SX_ED25519_SZ - 1] |= 0x40;	/* set bit 254 */

	/* Clear second half of private key digest: sx_async_ed25519_ptmult_go()
	 * works on an input of SX_ED25519_DGST_SZ bytes.
	 */
	safe_memset(t->workmem + SX_ED25519_SZ, t->workmemsz - SX_ED25519_SZ, 0, SX_ED25519_SZ);

	/* Perform point multiplication A = [s]B, to obtain the public key A. */
	pkreq = sx_async_ed25519_ptmult_go((const struct sx_ed25519_dgst *)t->workmem);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}

	t->pk = pkreq.req;

	return (t->statuscode = SX_ERR_HW_PROCESSING);
}

static int ed25519_sign_pubkey_ptmult(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* Get result of the point multiplication. This is the encoded point R,
	 * which is the first part of the signature.
	 */
	sx_async_ed25519_ptmult_end(t->pk,
				    (struct sx_ed25519_pt *)t->params.ed25519_sign.signature->r);

	si_wq_run_after(t, &t->params.ed25519_sign.wq, ed25519_sign_k_hash);

	return ed25519_compute_pubkey(t);
}

static int ed25519_sign_r_ptmult(struct sitask *t, struct siwq *wq)
{
	struct sx_pk_acq_req pkreq;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	si_wq_run_after(t, &t->params.ed25519_sign.wq, ed25519_sign_pubkey_ptmult);

	/* Perform point multiplication R = [r]B. */
	pkreq = sx_async_ed25519_ptmult_go(
		(const struct sx_ed25519_dgst *)(t->workmem + 3 * SX_ED25519_SZ));
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}

	t->pk = pkreq.req;
	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;
	t->statuscode = SX_ERR_HW_PROCESSING;

	return SX_ERR_HW_PROCESSING;
}

static int ed25519_sign_hash_message(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	si_wq_run_after(t, &t->params.ed25519_sign.wq, ed25519_sign_r_ptmult);

	/* Obtain r by hashing (prefix || message), where prefix is the second
	 * half of the private key digest.
	 */
	si_hash_create(t, &sxhashalg_sha2_512);
	si_task_consume(t, t->workmem + SX_ED25519_SZ, SX_ED25519_SZ);
	si_task_consume(t, t->params.ed25519_sign.msg, t->params.ed25519_sign.msgsz);
	si_task_produce(t, t->workmem + 3 * SX_ED25519_SZ, SX_ED25519_DGST_SZ);

	return SX_ERR_HW_PROCESSING;
}

static void ed25519_sign_hash_privkey(struct sitask *t)
{
	si_wq_run_after(t, &t->params.ed25519_sign.wq, ed25519_sign_hash_message);

	/* Hash the private key. */
	si_hash_create(t, &sxhashalg_sha2_512);
	si_task_consume(t, t->params.ed25519_sign.privkey->bytes, SX_ED25519_SZ);
	si_task_produce(t, t->workmem, SX_ED25519_DGST_SZ);
}

static void ed25519_sign_consume(struct sitask *t, const char *data, size_t sz)
{
	t->params.ed25519_sign.msg = data;
	t->params.ed25519_sign.msgsz = sz;
	t->actions.consume = NULL;
}

static void si_sig_create_ed25519_sign(struct sitask *t, const struct si_sig_privkey *privkey,
				       struct si_sig_signature *signature)
{
	if (t->workmemsz < 160) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	t->params.ed25519_sign.privkey = privkey->key.ed25519;
	t->params.ed25519_sign.signature = signature;
	t->params.ed25519_sign.msg = NULL;
	t->params.ed25519_sign.msgsz = 0;

	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;

	t->actions.consume = ed25519_sign_consume;
	t->actions.run = ed25519_sign_hash_privkey;
}

static int ed25519_genpubkey_finish(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* Get result of the point multiplication. This is the encoding of the
	 * [s]B point, which is the public key.
	 */
	sx_async_ed25519_ptmult_end(t->pk, t->params.ed25519_pubkey.pubkey);

	return t->statuscode;
}

static int ed25519_genpubkey_pointmul(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	si_wq_run_after(t, &t->params.ed25519_pubkey.wq, ed25519_genpubkey_finish);

	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;

	return ed25519_compute_pubkey(t);
}

static void ed25519_genpubkey_hash_privkey(struct sitask *t)
{
	si_wq_run_after(t, &t->params.ed25519_pubkey.wq, ed25519_genpubkey_pointmul);

	si_hash_create(t, &sxhashalg_sha2_512);
	si_task_consume(t, t->params.ed25519_pubkey.privkey->bytes, SX_ED25519_SZ);
	si_task_produce(t, t->workmem, SX_ED25519_DGST_SZ);
}

static void si_sig_create_ed25519_pubkey(struct sitask *t, const struct si_sig_privkey *privkey,
					 struct si_sig_pubkey *pubkey)
{
	if (t->workmemsz < SX_ED25519_DGST_SZ) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;
	t->actions.run = ed25519_genpubkey_hash_privkey;
	t->params.ed25519_pubkey.privkey = privkey->key.ed25519;
	t->params.ed25519_pubkey.pubkey = pubkey->key.ed25519;
}

static unsigned short int get_ed25519_key_opsz(const struct si_sig_privkey *privkey)
{
	(void)privkey;
	return SX_ED25519_SZ;
}

const struct si_sig_def *const si_sig_def_ed25519 = &(const struct si_sig_def){
	.sign = si_sig_create_ed25519_sign,
	.verify = si_sig_create_ed25519_verify,
	.pubkey = si_sig_create_ed25519_pubkey,
	.getkeyopsz = get_ed25519_key_opsz,
	.sigcomponents = 2,
};
