/*
 *  Copyright (c) 2024 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <silexpk/core.h>
#include <silexpk/montgomery.h>
#include <sicrypto/sicrypto.h>
#include <sicrypto/ed448.h>
#include "waitqueue.h"
#include "sigdefs.h"
#include "final.h"
#include "util.h"

/* U-coordinate of the base point, from RFC 7748. */
const struct sx_x448_op x448_base = {.bytes = {5}};

/* U-coordinate of the base point, from RFC 7748. */
const struct sx_x25519_op x25519_base = {.bytes = {9}};

static int x448_genpubkey_finish(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* Get result of the point multiplication. This is the encoding of the
	 * [s]B point, which is the public key.
	 */
	sx_async_x448_ptmult_end(t->pk, (struct sx_x448_op *)t->params.x448_pubkey.pubkey);

	return t->statuscode;
}

static void x448_genpubkey(struct sitask *t)
{
	si_wq_run_after(t, &t->params.x448_pubkey.wq, x448_genpubkey_finish);

	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;

	struct sx_pk_acq_req pkreq;

	/* Perform point multiplication A = [s]B, to obtain the public key A. */
	pkreq = sx_async_x448_ptmult_go(t->params.x448_pubkey.privkey, &x448_base);
	t->pk = pkreq.req;
}

static int x25519_genpubkey_finish(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		sx_pk_release_req(t->pk);
		return t->statuscode;
	}

	/* Get result of the point multiplication. This is the encoding of the
	 * [s]B point, which is the public key.
	 */
	sx_async_x25519_ptmult_end(t->pk, (struct sx_x25519_op *)t->params.x25519_pubkey.pubkey);

	return t->statuscode;
}

static void x25519_genpubkey(struct sitask *t)
{
	si_wq_run_after(t, &t->params.x25519_pubkey.wq, x25519_genpubkey_finish);

	t->actions.status = si_silexpk_status;
	t->actions.wait = si_silexpk_wait;

	struct sx_pk_acq_req pkreq;

	/* Perform point multiplication A = [s]B, to obtain the public key A. */
	pkreq = sx_async_x25519_ptmult_go(t->params.x25519_pubkey.privkey, &x25519_base);

	t->pk = pkreq.req;
}

static void si_sig_create_x448_pubkey(struct sitask *t, const struct si_sig_privkey *privkey,
				      struct si_sig_pubkey *pubkey)
{
	if (t->workmemsz < SX_X448_OP_SZ) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;
	t->actions.run = x448_genpubkey;
	t->params.x448_pubkey.privkey = privkey->key.x448;
	t->params.x448_pubkey.pubkey = pubkey->key.x448;
}

static void si_sig_create_x25519_pubkey(struct sitask *t, const struct si_sig_privkey *privkey,
					struct si_sig_pubkey *pubkey)
{
	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;
	t->actions.run = x25519_genpubkey;
	t->params.ed25519_pubkey.privkey = privkey->key.ed25519;
	t->params.ed25519_pubkey.pubkey = pubkey->key.ed25519;
}

static unsigned short int get_x25519_key_opsz(const struct si_sig_privkey *privkey)
{
	(void)privkey;
	return SX_X25519_OP_SZ;
}

static unsigned short int get_x448_key_opsz(const struct si_sig_privkey *privkey)
{
	(void)privkey;
	return SX_X448_OP_SZ;
}

const struct si_sig_def *const si_sig_def_x448 = &(const struct si_sig_def){
	.pubkey = si_sig_create_x448_pubkey,
	.getkeyopsz = get_x448_key_opsz,
	.sigcomponents = 2,
};

const struct si_sig_def *const si_sig_def_x25519 = &(const struct si_sig_def){
	.pubkey = si_sig_create_x25519_pubkey,
	.getkeyopsz = get_x25519_key_opsz,
	.sigcomponents = 2,
};
