/*
 * Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "multithreading_lock.h"

static K_MUTEX_DEFINE(mpsl_lock);

int multithreading_lock_acquire(k_timeout_t timeout)
{
	return k_mutex_lock(&mpsl_lock, timeout);
}

void multithreading_lock_release(void)
{
	k_mutex_unlock(&mpsl_lock);
}
