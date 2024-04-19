/*
 *  Copyright (c) 2020 Silex Insight
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef WAITQUEUE_HEADER_FILE
#define WAITQUEUE_HEADER_FILE

#include <nrf.h>
#include <zephyr/sys/__assert.h>

typedef int (*si_wq_func)(struct sitask *t, struct siwq *w);

static inline void validate_function_pointer(si_wq_func func)
{
#if defined(NRF54L15_XXAA) || defined(NRF54L15_ENGA_XXAA)
	/* Check that the function pointer is pointing to an address that
	 * is below 2MB.
	 *
	 * This may assert on crashes that corrupt the work queue, and may
	 * prevent certain attacks that rely on corrupting function
	 * pointers.
	 */
	__ASSERT_NO_MSG((uint32_t)func < 0x200000);
#endif
}


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
		validate_function_pointer(w->run);

		t->wq = w->next;
		w->run(t, w);
		w = t->wq;
	}
}

#endif
