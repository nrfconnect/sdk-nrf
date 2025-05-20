/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef WAITQUEUE_HEADER_FILE
#define WAITQUEUE_HEADER_FILE

typedef int (*si_wq_func)(struct sitask *t, struct siwq *w);

/** Queue a new siwq to run immediately after the sitask 'target' finishes.
 *
 * Of course, the new siwq 'w' should be unused before calling this
 * function. The function pointer 'func' will be stored in 'w'.
 */
static inline void si_wq_run_after(struct sitask *target, struct siwq *w, si_wq_func func)
{
	w->run = func;
	w->next = target->wq;
	target->wq = w;
}

/** Run all the siwq waiting on task t, one after the other, stopping if the
 * task's statuscode becomes SX_ERR_HW_PROCESSING (the statuscode becomes
 * SX_ERR_HW_PROCESSING if a siwq function triggers the HW). Note: before being
 * run, a siwq is removed from the queue of waiting siwq.
 */
static inline void si_wq_wake_next(struct sitask *t)
{
	struct siwq *w = t->wq;

	while ((w != NULL) && (t->statuscode != SX_ERR_HW_PROCESSING)) {
		t->wq = w->next;
		w->run(t, w);
		w = t->wq;
	}
}

#endif
