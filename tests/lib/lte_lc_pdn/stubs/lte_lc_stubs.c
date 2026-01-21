/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include "common/work_q.h"

static struct k_work_q stub_work_q;

void work_q_start(void)
{
	/* No-op for unit tests */
}

struct k_work_q *work_q_get(void)
{
	return &stub_work_q;
}
