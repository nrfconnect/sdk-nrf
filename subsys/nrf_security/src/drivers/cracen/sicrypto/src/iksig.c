/* Isolated key operations (signature generation, ...)
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout for the sign task:
 *      1. Hash digest of the message to be signed (size: digestsz).
 * Other IK tasks don't need workmem memory.
 */

#include <silexpk/core.h>
#include <silexpk/iomem.h>
#include <sxsymcrypt/hash.h>
#include <sxsymcrypt/sha2.h>
#include <silexpk/ik.h>
#include "../include/sicrypto/sicrypto.h"
#include "../include/sicrypto/hash.h"
#include <cracen/statuscodes.h>
#include "../include/sicrypto/ik.h"
#include "../include/sicrypto/ecdsa.h"
#include "waitqueue.h"
#include "final.h"
#include "util.h"
#include "sigdefs.h"

#ifndef MAX_ECDSA_ATTEMPTS
#define MAX_ECDSA_ATTEMPTS 255
#endif

/** Convert a digest into an operand for ECDSA
 *
 * The raw digest may need to be padded or truncated to fit the curve
 * operand used for ECDSA.
 *
 * Conversion could also imply byte order inversion, but that is not
 * implemented here. It's expected that SilexPK takes big endian
 * operands here.
 *
 * As the digest size is expressed in bytes, this procedure does not
 * work for curves which have sizes not multiples of 8 bits.
 */
static void digest2op(const char *digest, size_t sz, char *dst, size_t opsz)
{
	if (opsz > sz) {
		sx_clrpkmem(dst, opsz - sz);
		dst += opsz - sz;
	}
	sx_wrpkmem(dst, digest, opsz);
}

static void si_ecdsa_read_sig(struct si_sig_signature *sig, const char *r, const char *s,
			      size_t opsz)
{
	sx_rdpkmem(sig->r, r, opsz);
	sx_rdpkmem(sig->s, s, opsz);
}

static int restore_ik_statuscode(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	sx_pk_release_req(t->pk);
	if (t->params.ik.statuscode) {
		t->statuscode = t->params.ik.statuscode;
	}

	return t->statuscode;
}

static int exit_ikg(struct sitask *t)
{
	struct sx_pk_acq_req pkreq;

	sx_pk_release_req(t->pk);
	t->params.ik.statuscode = t->statuscode;
	pkreq = sx_pk_acquire_req(SX_PK_CMD_IK_EXIT);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}
	pkreq.status = sx_pk_list_ik_inslots(pkreq.req, 0, NULL);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}

	sx_pk_run(pkreq.req);
	t->statuscode = SX_ERR_HW_PROCESSING;
	si_wq_run_after(t, &t->params.ik.wq, restore_ik_statuscode);

	return SX_ERR_HW_PROCESSING;
}

static int start_ecdsa_ik_sign(struct sitask *t, struct siwq *wq);

static int finish_ecdsa_ik_sign(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if ((t->statuscode == SX_ERR_INVALID_SIGNATURE) && (t->params.ik.attempts--)) {
		sx_pk_release_req(t->pk);
		si_wq_run_after(t, &t->params.ik.wq, start_ecdsa_ik_sign);
		return (t->statuscode = SX_OK);
	}

	if (t->statuscode != SX_OK) {
		return exit_ikg(t);
	}

	const char **outputs = sx_pk_get_output_ops(t->pk);
	const int opsz = sx_pk_get_opsize(t->pk);

	si_ecdsa_read_sig(t->params.ik.signature, outputs[0], outputs[1], opsz);

	return exit_ikg(t);
}

static int start_ecdsa_ik_sign(struct sitask *t, struct siwq *wq)
{
	struct sx_pk_acq_req pkreq;
	struct sx_pk_inops_ik_ecdsa_sign inputs;
	int opsz;
	size_t digestsz = sx_hash_get_digestsz(&t->u.h);
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;

	t->params.ik.attempts = MAX_ECDSA_ATTEMPTS;
	pkreq = sx_pk_acquire_req(SX_PK_CMD_IK_ECDSA_SIGN);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}
	pkreq.status = sx_pk_list_ik_inslots(pkreq.req, t->params.ik.privkey->key.ref.index,
					     (struct sx_pk_slot *)&inputs);
	if (pkreq.status) {
		return si_task_mark_final(t, pkreq.status);
	}
	t->pk = pkreq.req;
	opsz = sx_pk_get_opsize(pkreq.req);
	digest2op(t->workmem, digestsz, inputs.h.addr, opsz);
	sx_pk_run(pkreq.req);
	t->statuscode = SX_ERR_HW_PROCESSING;
	si_wq_run_after(t, &t->params.ik.wq, finish_ecdsa_ik_sign);

	return SX_ERR_HW_PROCESSING;
}

static void run_ecdsa_ik_hash(struct sitask *t)
{
	/* Override SX_ERR_HW_PROCESSING state pre-set by si_task_run()
	 * to be able to start the hash
	 */
	t->statuscode = SX_ERR_READY;
	si_task_produce(t, t->workmem, sx_hash_get_digestsz(&t->u.h));

	si_wq_run_after(t, &t->params.ik.wq, start_ecdsa_ik_sign);
}

static void create_sign(struct sitask *t, const struct si_sig_privkey *privkey,
			struct si_sig_signature *signature)
{
	if (t->workmemsz < sx_hash_get_alg_digestsz(privkey->hashalg)) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	si_hash_create(t, privkey->hashalg);
	t->actions.run = run_ecdsa_ik_hash;
	t->params.ik.privkey = privkey;
	t->params.ik.signature = signature;
}

static int finish_ik_pubkey(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		return exit_ikg(t);
	}

	const char **outputs = sx_pk_get_output_ops(t->pk);
	const int opsz = sx_pk_get_opsize(t->pk);

	sx_rdpkmem(t->params.ik.pubkey->key.eckey.qx, outputs[0], opsz);
	sx_rdpkmem(t->params.ik.pubkey->key.eckey.qy, outputs[1], opsz);

	return exit_ikg(t);
}

static void run_ik_genpubkey(struct sitask *t)
{
	struct sx_pk_acq_req pkreq;

	pkreq = sx_pk_acquire_req(SX_PK_CMD_IK_PUBKEY_GEN);
	if (pkreq.status) {
		si_task_mark_final(t, pkreq.status);
		return;
	}
	pkreq.status = sx_pk_list_ik_inslots(pkreq.req, t->params.ik.privkey->key.ref.index, NULL);
	if (pkreq.status) {
		si_task_mark_final(t, pkreq.status);
		return;
	}
	t->pk = pkreq.req;
	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;
	t->statuscode = SX_ERR_HW_PROCESSING;
	si_wq_run_after(t, &t->params.ik.wq, finish_ik_pubkey);
	sx_pk_run(pkreq.req);
}

static void create_pubkey(struct sitask *t, const struct si_sig_privkey *privkey,
			  struct si_sig_pubkey *pubkey)
{
	t->statuscode = SX_ERR_READY;
	t->actions = (struct siactions){0};
	t->actions.run = run_ik_genpubkey;
	t->params.ik.privkey = privkey;
	t->params.ik.pubkey = pubkey;
	pubkey->hashalg = privkey->hashalg;
	pubkey->def = si_sig_def_ecdsa;
	pubkey->key.eckey.curve = privkey->key.ref.curve;
}

static const struct si_sig_def si_sig_def_ik = {
	.sign = create_sign,
	.pubkey = create_pubkey,
};

static const struct sxhashalg *select_optimal_hashalg(void)
{
	const struct sx_pk_capabilities *caps = sx_pk_fetch_capabilities();
	int sz = caps->ik_opsz;

	switch (sz) {
	case 48:
		return &sxhashalg_sha2_384;
	case 66:
		return &sxhashalg_sha2_512;
	default:
		return &sxhashalg_sha2_256;
	}
}

struct si_sig_privkey si_sig_fetch_ikprivkey(const struct sx_pk_ecurve *curve, int index)
{
	struct si_sig_privkey privkey = {.def = &si_sig_def_ik,
					 .key.ref = {
						 .curve = curve,
						 .index = index,
					 }};
	privkey.hashalg = select_optimal_hashalg();

	return privkey;
}
