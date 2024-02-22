/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file mpsl_work.h
 *
 * @defgroup mpsl_work Multiprotocol Service Layer workqueue.
 *
 * @brief Internal MPSL workqueue.
 * @{
 */

#ifndef MPSL_WORK__
#define MPSL_WORK__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>

extern struct k_work_q mpsl_work_q;

/** @brief Submit a work item to the MPSL workqueue.
 *
 * This routine submits work item @a work to be processed by the MPSL
 * workqueue. If the work item is already pending in the MPSL workqueue or
 * any other workqueue as a result of an earlier submission, this routine
 * has no effect on the work item. If the work item has already been
 * processed, or is currently being processed, its work is considered
 * complete and the work item can be resubmitted.
 *
 * @note
 * Work items submitted to the MPSL workqueue should avoid using handlers
 * that block or yield since this may prevent the MPSL workqueue from
 * processing other work items in a timely manner.
 *
 * @note Can be called by ISRs.
 *
 * @param work Address of work item.
 */
static inline void mpsl_work_submit(struct k_work *work)
{
	if (k_work_submit_to_queue(&mpsl_work_q, work) < 0) {
		__ASSERT(false, "k_work_submit_to_queue() failed.");
	}
}

/** @brief Submit an idle work item to the MPSL workqueue after a delay.
 *
 * @note Can be called by ISRs.
 *
 * @note
 * Work items submitted to the MPSL workqueue should avoid using handlers
 * that block or yield since this may prevent the MPSL workqueue from
 * processing other work items in a timely manner.
 *
 * @note This is a no-op if the work item is already scheduled or submitted,
 * even if @p delay is @c K_NO_WAIT.
 *
 * @param dwork Address of delayable work item.
 *
 * @param delay the time to wait before submitting the work item.  If @c
 * K_NO_WAIT and the work is not pending this is equivalent to
 * mpsl_work_submit().
 */
static inline void mpsl_work_schedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
	if (k_work_schedule_for_queue(&mpsl_work_q, dwork, delay) < 0) {
		__ASSERT(false, "k_work_schedule_for_queue() failed.");
	}
}

#ifdef __cplusplus
}
#endif

#endif /* MPSL_WORK__ */

/**@} */
