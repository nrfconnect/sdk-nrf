/* MGF1 mask computation and bitwise xor, based on RFC 8017.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * Workmem layout for the MGF1XOR task:
 *      1. MGF1 counter (size: MGF1_COUNTER_SZ)
 *      2. MGF1 output segment (size: digestsz).
 */

#include "../include/sicrypto/sicrypto.h"
#include "../include/sicrypto/hash.h"
#include <cracen/statuscodes.h>
#include "rsamgf1xor.h"
#include "waitqueue.h"
#include "final.h"
#include "util.h"

/* number of bytes used for the MGF1 internal counter */
#define MGF1_COUNTER_SZ 4

static int mgf1xor_hash(struct sitask *t);

/* this function: (1) computes the xor of the mask segment that was just
 * produced with the bytes pointed by xorinout, (2) increments the MGF1 counter
 */
static int mgf1xor_xorincr(struct sitask *t, struct siwq *wq)
{
	size_t toxor;
	size_t digestsz = sx_hash_get_alg_digestsz(t->hashalg);
	char *mgfctr = t->workmem;
	char *mask_segment = t->workmem + MGF1_COUNTER_SZ;
	(void)wq;

	if (t->statuscode != SX_OK) {
		return t->statuscode;
	}

	toxor = (t->params.mgf1xor.masksz > digestsz) ? digestsz : t->params.mgf1xor.masksz;

	si_xorbytes(t->params.mgf1xor.xorinout, mask_segment, toxor);

	t->params.mgf1xor.xorinout += toxor;

	/* masksz holds the number of remaining mask bytes to produce */
	t->params.mgf1xor.masksz -= toxor;

	if (t->params.mgf1xor.masksz > 0) {
		si_be_add((unsigned char *)mgfctr, MGF1_COUNTER_SZ, 1);
		return mgf1xor_hash(t);
	} else {
		return SX_OK;
	}
}

static int mgf1xor_hash(struct sitask *t)
{
	char *mgfctr = t->workmem;
	char *mask_segment = t->workmem + MGF1_COUNTER_SZ;

	si_wq_run_after(t, &t->params.mgf1xor.wq, mgf1xor_xorincr);

	si_hash_create(t, t->hashalg);
	si_task_consume(t, t->params.mgf1xor.seed, t->params.mgf1xor.seedsz);
	si_task_consume(t, mgfctr, MGF1_COUNTER_SZ);
	si_task_produce(t, mask_segment, sx_hash_get_alg_digestsz(t->hashalg));

	return SX_ERR_HW_PROCESSING;
}

static void mgf1xor_produce(struct sitask *t, char *out, size_t sz)
{
	char *mgfctr = t->workmem;

	/* initialize the MGF1 counter to zero */
	mgfctr[0] = 0;
	mgfctr[1] = 0;
	mgfctr[2] = 0;
	mgfctr[3] = 0;

	t->params.mgf1xor.masksz = sz;
	t->params.mgf1xor.xorinout = out;

	mgf1xor_hash(t);
}

/* receive the MGF1 seed input */
static void mgf1xor_consume(struct sitask *t, const char *data, size_t sz)
{
	t->params.mgf1xor.seed = data;
	t->params.mgf1xor.seedsz = sz;
}

void si_create_mgf1xor(struct sitask *t, const struct sxhashalg *hashalg)
{
	t->hashalg = hashalg;
	t->params.mgf1xor.seed = NULL;
	t->params.mgf1xor.seedsz = 0;

	if (t->workmemsz < sx_hash_get_alg_digestsz(hashalg) + 4) {
		si_task_mark_final(t, SX_ERR_WORKMEM_BUFFER_TOO_SMALL);
		return;
	}

	t->actions = (struct siactions){0};
	t->statuscode = SX_ERR_READY;

	t->actions.consume = mgf1xor_consume;
	t->actions.produce = mgf1xor_produce;
}
