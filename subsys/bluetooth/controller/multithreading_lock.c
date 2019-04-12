/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "multithreading_lock.h"

static K_SEM_DEFINE(ble_controller_lock, 1, 1);

int multithreading_lock_acquire(int32_t timeout)
{
	return k_sem_take(&ble_controller_lock,
			  ((timeout == K_FOREVER || timeout == K_NO_WAIT) ?
			   timeout : K_MSEC(timeout)));
}

void multithreading_lock_release(void)
{
	k_sem_give(&ble_controller_lock);
}
