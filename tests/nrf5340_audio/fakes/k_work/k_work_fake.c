/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "k_work_fake.h"

DEFINE_FAKE_VOID_FUNC(k_work_queue_init, struct k_work_q *);
DEFINE_FAKE_VOID_FUNC(k_work_queue_start, struct k_work_q *, k_thread_stack_t *, size_t, int,
		      const struct k_work_queue_config *);
DEFINE_FAKE_VOID_FUNC(k_work_init, struct k_work *, k_work_handler_t);

DEFINE_FAKE_VALUE_FUNC(int, k_work_submit_to_queue, struct k_work_q *, struct k_work *);
DEFINE_FAKE_VALUE_FUNC(int, k_work_queue_drain, struct k_work_q *, bool);

k_work_handler_t fake_handler;

void k_work_init_valid_fake(struct k_work *work, k_work_handler_t handler)
{
	ARG_UNUSED(work);
	ARG_UNUSED(handler);

	fake_handler = handler;
}

int k_work_submit_to_queue_valid_fake(struct k_work_q *work_q, struct k_work *work)
{
	ARG_UNUSED(work_q);
	ARG_UNUSED(work);

	fake_handler(work);

	return 0;
}
