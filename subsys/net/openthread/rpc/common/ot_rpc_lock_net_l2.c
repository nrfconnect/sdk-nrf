/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_rpc_lock.h"

#include <openthread.h>

void ot_rpc_mutex_lock(void)
{
	openthread_mutex_lock();
}

void ot_rpc_mutex_unlock(void)
{
	openthread_mutex_unlock();
}
