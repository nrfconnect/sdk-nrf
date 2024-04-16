/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <assert.h>
#include "../include/sicrypto/sicrypto.h"
#include <cracen/statuscodes.h>
#include "waitqueue.h"

extern struct sx_pk_cnx silex_pk_engine;

void si_task_init(struct sitask *t, char *workmem, size_t workmemsz)
{
	assert(t);

	t->wq = NULL;
	t->workmem = workmem;
	t->workmemsz = workmemsz;
}

int si_task_status(struct sitask *t)
{
	assert(t);

	if (t->statuscode == SX_ERR_READY) {
		/* The task hasn't run on the hardware yet and it hasn't got its
		 * final statuscode yet. In this case, we should not run the
		 * next steps in the waitqueue. The waitqueue is run only when
		 * processing finishes.
		 */
		return t->statuscode;
	}
	if (t->statuscode == SX_ERR_HW_PROCESSING) {
		t->statuscode = t->actions.status(t);
	}
	si_wq_wake_next(t);

	return t->statuscode;
}

int si_task_wait(struct sitask *t)
{
	assert(t);

	if (t->statuscode == SX_ERR_READY) {
		/* The task hasn't run on the hardware yet and it hasn't got its
		 * final statuscode yet. In this case, there's no need to wait
		 * or run work from the waitqueue.
		 */
		return t->statuscode;
	}
	while (t->statuscode == SX_ERR_HW_PROCESSING) {
		if (t->actions.wait) {
			t->statuscode = t->actions.wait(t);
			assert(t->statuscode != SX_ERR_HW_PROCESSING);
		} else {
			t->statuscode = t->actions.status(t);
		}
		si_wq_wake_next(t);
	}
	si_wq_wake_next(t);

	return t->statuscode;
}

void si_task_consume(struct sitask *t, const char *data, size_t sz)
{
	assert(t);
	if ((t->statuscode != SX_ERR_READY) && (t->statuscode != SX_ERR_RESEED_NEEDED)) {
		return;
	}
	if (!t->actions.consume) {
		assert(0);
		return;
	}
	t->actions.consume(t, data, sz);
}

void si_task_partial_run(struct sitask *t)
{
	assert(t);
	if (t->statuscode != SX_ERR_READY) {
		return;
	}
	if (!t->actions.partial_run) {
		assert(0);
		return;
	}
	t->statuscode = SX_ERR_HW_PROCESSING;
	t->actions.partial_run(t);
}

void si_task_run(struct sitask *t)
{
	assert(t);
	if (t->statuscode != SX_ERR_READY) {
		return;
	}
	if (!t->actions.run) {
		assert(0);
		return;
	}
	t->statuscode = SX_ERR_HW_PROCESSING;
	t->actions.run(t);
}

void si_task_produce(struct sitask *t, char *out, size_t sz)
{
	assert(t);
	if (t->statuscode != SX_ERR_READY) {
		return;
	}
	if (!t->actions.produce) {
		assert(0);
		return;
	}
	t->statuscode = SX_ERR_HW_PROCESSING;
	t->actions.produce(t, out, sz);
}
