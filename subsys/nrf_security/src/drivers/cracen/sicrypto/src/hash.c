/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "../include/sicrypto/sicrypto.h"
#include "../include/sicrypto/hash.h"
#include <cracen/statuscodes.h>
#include "partialrun.h"
#include "final.h"

static int hash_status(struct sitask *t)
{
	int r;

	r = sx_hash_status(&t->u.h);
	if (r == SX_ERR_HW_PROCESSING) {
		return r;
	}

	if (t->partialrun == PARTIALRUN_INPROGRESS) {
		t->partialrun = PARTIALRUN_FINISHED;
		if (r == SX_OK) {
			t->statuscode = SX_ERR_READY;
			return SX_ERR_READY;
		}
	}
	if (r || (t->partialrun == PARTIALRUN_NONE)) {
		si_task_mark_final(t, r);
	}
	return r;
}

static int hash_wait(struct sitask *t)
{
	int r;

	r = sx_hash_wait(&t->u.h);
	if (r != SX_OK) {
		return si_task_mark_final(t, r);
	}
	if (t->partialrun == PARTIALRUN_INPROGRESS) {
		t->partialrun = PARTIALRUN_FINISHED;
		return SX_ERR_READY;
	}
	return si_task_mark_final(t, r);
}

static void hash_consume(struct sitask *t, const char *msg, size_t sz)
{
	int r;

	if (t->partialrun != PARTIALRUN_NONE) {
		t->partialrun = PARTIALRUN_NONE;
		r = sx_hash_resume_state(&t->u.h);
		if (r) {
			si_task_mark_final(t, r);
			return;
		}
	}
	r = sx_hash_feed(&t->u.h, msg, sz);
	if (r) {
		si_task_mark_final(t, r);
	}
}

static void hash_partial_run(struct sitask *t)
{
	int r;

	t->partialrun = PARTIALRUN_INPROGRESS;
	r = sx_hash_save_state(&t->u.h);
	if (r) {
		si_task_mark_final(t, r);
	}
}

static void hash_produce(struct sitask *t, char *out, size_t sz)
{
	int r;
	size_t digestsz = sx_hash_get_digestsz(&t->u.h);

	if (sz < digestsz) {
		sx_hash_free(&t->u.h);
		si_task_mark_final(t, SX_ERR_OUTPUT_BUFFER_TOO_SMALL);
		return;
	}

	r = sx_hash_digest(&t->u.h, out);
	if (r) {
		si_task_mark_final(t, r);
	}
}

static const struct siactions hash_actions = {.status = &hash_status,
					      .wait = &hash_wait,
					      .consume = &hash_consume,
					      .partial_run = &hash_partial_run,
					      .produce = &hash_produce};

void si_hash_create(struct sitask *t, const struct sxhashalg *hashalg)
{
	int r;

	t->statuscode = SX_ERR_READY;
	t->partialrun = PARTIALRUN_NONE;
	t->actions = hash_actions;

	r = sx_hash_create(&t->u.h, hashalg, sizeof(t->u));
	if (r != SX_OK) {
		si_task_mark_final(t, r);
	}
}
