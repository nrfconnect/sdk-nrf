/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef K_WORK_FAKE_H__
#define K_WORK_FAKE_H__

#include <zephyr/fff.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>

DECLARE_FAKE_VOID_FUNC(k_work_queue_init, struct k_work_q *);
DECLARE_FAKE_VOID_FUNC(k_work_queue_start, struct k_work_q *, k_thread_stack_t *, size_t, int,
		       const struct k_work_queue_config *);
DECLARE_FAKE_VOID_FUNC(k_work_init, struct k_work *, k_work_handler_t);

DECLARE_FAKE_VALUE_FUNC(int, k_work_submit_to_queue, struct k_work_q *, struct k_work *);
DECLARE_FAKE_VALUE_FUNC(int, k_work_queue_drain, struct k_work_q *, bool);

#define DO_FOREACH_K_WORK_FAKE(FUNC)                                                               \
	do {                                                                                       \
		FUNC(k_work_queue_init)                                                            \
		FUNC(k_work_queue_start)                                                           \
		FUNC(k_work_init)                                                                  \
		FUNC(k_work_submit_to_queue)                                                       \
		FUNC(k_work_queue_drain)                                                           \
	} while (0)

void k_work_init_valid_fake(struct k_work *work, k_work_handler_t handler);

int k_work_submit_to_queue_valid_fake(struct k_work_q *work_q, struct k_work *work);

#endif /* K_WORK_FAKE_H__ */
