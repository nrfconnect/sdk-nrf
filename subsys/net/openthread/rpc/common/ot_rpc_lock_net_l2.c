/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ot_rpc_lock.h"

#include <zephyr/net/openthread.h>

void ot_rpc_mutex_lock(void)
{
	openthread_api_mutex_lock(openthread_get_default_context());
}

void ot_rpc_mutex_unlock(void)
{
	openthread_api_mutex_unlock(openthread_get_default_context());
}
