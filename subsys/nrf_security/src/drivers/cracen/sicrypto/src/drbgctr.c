/* DRBG AES CTR without derivation function, based on NIST SP 800-90A Rev. 1.
 *
 * Copyright (c) 2020-2021 Silex Insight
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Notes
 * - As the implementation does not use the optional derivation function, it
 *   cannot take a nonce. Personalization string size must be smaller than or
 *   equal to the seed length (seedlen = key size + 16 bytes).
 * - The counter field length is assumed equal to blocklen.
 * - The "additional input" defined in the NIST specification is not supported.
 *
 * Workmem layout for a DRBG AES CTR task:
 *      1. key (size: keysz).
 *      2. v (size: BLOCKLEN). Works as the counter of block cipher CTR mode.
 *      3. xor buffer (size: XORBUFSZ). Used for:
 *          - work area for seeding and reseeding
 *          - zeros during production of random bytes and subsequent update.
 *
 * During instantiation, part 3 of the workmem holds the xor of the entropy
 * input with the personalization string.
 * During reseeding, part 3 of the workmem holds the entropy input.
 * In both cases (instantiation and reseeding), only the first seedlen bytes of
 * part 3 are used and these bytes are the plaintext to encrypt with AES CTR.
 * During production of pseudorandom bytes, part 3 of the workmem must contain
 * all zeros, which are used as the plaintext to encrypt with AES CTR. During
 * this phase, the size of this area (XORBUFSZ) determines the number of
 * internal iterations: the output byte string is produced in several
 * iterations, with each iteration producing a segment of XORBUF_SZ bytes (the
 * last segment can be smaller).
 * During the update that follows the production of random bytes, only the first
 * seedlen bytes of part 3 of the workmem are used (as plaintext to encrypt with
 * AES CTR). These bytes must be all zeros: in fact they represent the
 * "additional input", which is not supported.
 */

#include <string.h>
#include <sxsymcrypt/keyref.h>
#include <sxsymcrypt/blkcipher.h>
#include <sxsymcrypt/aes.h>
#include "../include/sicrypto/sicrypto.h"
#include "../include/sicrypto/drbgctr.h"
#include <cracen/mem_helpers.h>
#include <cracen/statuscodes.h>
#include "waitqueue.h"
#include "final.h"
#include "util.h"

#define BLOCKLEN	      16 /* 128 bits for all AES ciphers as per NIST spec */
#define XORBUF_SZ	      128
#define RESEED_START_INTERVAL ((uint64_t)1 << 48) /* 2^48 as per NIST spec */
#define MAX_PRODUCTION_SZ     (1u << 16)	  /* min(B, 2^16) bytes as per NIST spec */

struct workmemptrs {
	char *key;
	struct sxkeyref kref;
	char *v;
	char *xorbuf;
};

static struct workmemptrs drbgctr_ptrs(struct sitask *t)
{
	struct workmemptrs ptrs = {.key = t->workmem,
				   .v = t->workmem + t->params.drbgctr.keysz,
				   .xorbuf = t->workmem + t->params.drbgctr.keysz + BLOCKLEN};
	ptrs.kref = sx_keyref_load_material(t->params.drbgctr.keysz, ptrs.key);

	return ptrs;
}

static char *drbgctr_xorbuf(struct sitask *t)
{
	return t->workmem + t->params.drbgctr.keysz + BLOCKLEN;
}

static int drbgctr_status(struct sitask *t);
static int drbgctr_wait(struct sitask *t);
static void drbgctr_seed_run(struct sitask *t);
static void drbgctr_seed(struct sitask *t, const char *data, size_t sz);

static int start_drbgctr_update(struct sitask *t)
{
	struct workmemptrs ptrs = drbgctr_ptrs(t);
	int r;
	size_t seedlen = t->params.drbgctr.keysz + BLOCKLEN;

	/* Adjust counter V with the number of processed blocks */
	si_be_add((unsigned char *)ptrs.v, BLOCKLEN, t->params.drbgctr.processed);
	/* After internal update, V must be incremented before using AES
	 * CTR. See CTR_DRBG Generate, step in the spec.
	 */
	t->params.drbgctr.processed = 1;

	r = sx_blkcipher_create_aesctr_enc(&t->u.b, &ptrs.kref, ptrs.v,
					   BA411_AES_COUNTERMEASURES_DISABLE);
	if (r) {
		return si_task_mark_final(t, r);
	}
	r = sx_blkcipher_crypt(&t->u.b, ptrs.xorbuf, seedlen, t->workmem);
	if (r) {
		return si_task_mark_final(t, r);
	}
	t->actions.status = drbgctr_status;
	t->actions.wait = drbgctr_wait;
	r = sx_blkcipher_run(&t->u.b);
	if (r) {
		return si_task_mark_final(t, r);
	}

	t->statuscode = SX_ERR_HW_PROCESSING;
	return SX_ERR_HW_PROCESSING;
}

static int drbgctr_becomes_ready(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	t->statuscode = SX_ERR_READY;

	return SX_ERR_READY;
}

static int start_drbgctr_internal_update(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	/* CTR_DRBG Generate Process, step 6 from spec:
	 *    (Key, V) = CTR_DRBG_Update (additional_input, Key, V))
	 * As we don't have additional_input, use zeros instead.
	 */
	si_wq_run_after(t, &t->params.drbgctr.wq, drbgctr_becomes_ready);

	return start_drbgctr_update(t);
}

static int drbgctr_continue_produce(struct sitask *t, struct siwq *wq)
{
	struct workmemptrs ptrs = drbgctr_ptrs(t);
	int r;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	/* Adjust counter V with the number of processed blocks */
	si_be_add((unsigned char *)ptrs.v, BLOCKLEN, t->params.drbgctr.processed);

	r = sx_blkcipher_create_aesctr_enc(&t->u.b, &ptrs.kref, ptrs.v,
					   BA411_AES_COUNTERMEASURES_DISABLE);
	if (r) {
		return si_task_mark_final(t, r);
	}
	/* CTR_DRBG Generate Process, step 2 : no additional data */
	/* CTR_DRBG Generate Process, step 3, 4 and 5 from spec */
	size_t psz = (t->params.drbgctr.outsz > XORBUF_SZ) ? XORBUF_SZ : t->params.drbgctr.outsz;

	r = sx_blkcipher_crypt(&t->u.b, ptrs.xorbuf, psz, t->output);
	if (r) {
		return si_task_mark_final(t, r);
	}
	t->params.drbgctr.outsz -= psz;
	t->output += psz;
	t->params.drbgctr.processed = (psz + BLOCKLEN - 1) / BLOCKLEN;

	t->actions.status = drbgctr_status;
	t->actions.wait = drbgctr_wait;
	if (t->params.drbgctr.outsz) {
		si_wq_run_after(t, &t->params.drbgctr.wq, drbgctr_continue_produce);
	} else {
		si_wq_run_after(t, &t->params.drbgctr.wq, start_drbgctr_internal_update);
	}

	/* Total size will be added to V after wait/status finishes.
	 * That's done in start_drbgctr_internal_update()
	 * or in drbgctr_continue_produce().
	 */

	r = sx_blkcipher_run(&t->u.b);
	if (r) {
		return si_task_mark_final(t, r);
	}
	t->statuscode = SX_ERR_HW_PROCESSING;

	return SX_ERR_HW_PROCESSING;
}

static void drbgctr_produce_random(struct sitask *t, char *out, size_t sz)
{
	t->output = out;
	t->params.drbgctr.outsz = sz;
	t->actions.status = drbgctr_status;
	t->actions.wait = drbgctr_wait;

	t->params.drbgctr.reseed_interval--;
	if (!t->params.drbgctr.reseed_interval) {
		si_task_mark_final(t, SX_ERR_RESEED_NEEDED);
		t->actions.consume = &drbgctr_seed;
		return;
	}
	if (sz > MAX_PRODUCTION_SZ) {
		si_task_mark_final(t, SX_ERR_INVALID_REQ_SZ);
		return;
	}

	/* override statuscode value to be able to run
	 * drbgctr_continue_produce()
	 */
	t->statuscode = SX_OK;

	drbgctr_continue_produce(t, NULL);
}

static void drbgctr_seed_extra(struct sitask *t, const char *data, size_t sz)
{
	size_t seedlen = t->params.drbgctr.keysz + BLOCKLEN;

	if (sz > seedlen) {
		si_task_mark_final(t, SX_ERR_TOO_BIG);
		return;
	}

	si_xorbytes(drbgctr_xorbuf(t), data, sz);

	/* uninstall consume action: only one call allowed to provide pers
	 * string
	 */
	t->actions.consume = NULL;
}

static void drbgctr_seed(struct sitask *t, const char *data, size_t sz)
{
	size_t seedlen = t->params.drbgctr.keysz + BLOCKLEN;

	if (sz < seedlen) {
		si_task_mark_final(t, SX_ERR_INPUT_BUFFER_TOO_SMALL);
		return;
	}
	if (sz > seedlen) {
		si_task_mark_final(t, SX_ERR_TOO_BIG);
		return;
	}

	/* if reseeding at the end of seed life */
	if (t->statuscode == SX_ERR_RESEED_NEEDED) {
		t->statuscode = SX_ERR_READY;
	}

	t->actions.run = &drbgctr_seed_run;
	t->actions.consume = &drbgctr_seed_extra;
	t->actions.produce = NULL;

	si_xorbytes(drbgctr_xorbuf(t), data, sz);
	t->params.drbgctr.reseed_interval = RESEED_START_INTERVAL;
}

static int finish_run(struct sitask *t, int r)
{
	si_task_mark_final(t, r);
	if (r) {
		return r;
	}
	t->actions.consume = &drbgctr_seed;
	t->actions.produce = &drbgctr_produce_random;

	return r;
}

static int drbgctr_status(struct sitask *t)
{
	int r;

	r = sx_blkcipher_status(&t->u.b);
	if (r == SX_ERR_HW_PROCESSING) {
		return r;
	}

	return finish_run(t, r);
}

static int drbgctr_wait(struct sitask *t)
{
	int r;

	r = sx_blkcipher_wait(&t->u.b);

	return finish_run(t, r);
}

static int clear_xorbuf(struct sitask *t, struct siwq *wq)
{
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	safe_memzero(drbgctr_xorbuf(t), XORBUF_SZ);
	t->statuscode = SX_ERR_READY;

	return SX_ERR_READY;
}

static void drbgctr_seed_run(struct sitask *t)
{
	si_wq_run_after(t, &t->params.drbgctr.wq, clear_xorbuf);

	/* CTR_DRBG Instantiate and reseed without derivation function. */
	/* Step 6. (Key, V) = CTR_DRBG_Update (seed_material, Key, V). */
	t->statuscode = start_drbgctr_update(t);
}

void si_prng_create_drbg_aes_ctr(struct sitask *t, size_t keysz, const char *entropy)
{
	t->params.drbgctr.processed = 0;
	t->params.drbgctr.keysz = keysz;
	t->params.drbgctr.outsz = 0;

	if (t->workmemsz < keysz + 144) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;

	/* CTR_DRBG Instantiate without derivation function */
	/* Step 4. key = 0   keylen bits of zeros. */
	/* Step 5. V = 0    blocklen bits of zeros. */
	safe_memset(t->workmem, t->workmemsz, 0, t->params.drbgctr.keysz + BLOCKLEN + XORBUF_SZ);

	/* As we use AES CTR hardware, we need to increment V => V=1
	 * See CTR_DRBG_Update, step 2.1 in the spec.
	 */
	t->workmem[t->params.drbgctr.keysz + BLOCKLEN - 1] = 1;

	drbgctr_seed(t, entropy, t->params.drbgctr.keysz + BLOCKLEN);
}
