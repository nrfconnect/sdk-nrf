/*
 * Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "multithreading_lock.h"

static K_SEM_DEFINE(mpsl_lock, 1, 1);

int multithreading_lock_acquire(k_timeout_t timeout)
{
	return k_sem_take(&mpsl_lock, timeout);
}

void multithreading_lock_release(void)
{
	k_sem_give(&mpsl_lock);
}
