/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Mock implementation of OpenThread RPC Lock APIs used by OT RPC client.
 * This is needed to verify that user callbacks are called with the mutex locked.
 */

#include "mocks.h"

#include <ot_rpc_lock.h>

static bool is_locked;

void ot_rpc_mutex_lock(void)
{
	is_locked = true;
}

void ot_rpc_mutex_unlock(void)
{
	is_locked = false;
}

bool ot_rpc_is_mutex_locked(void)
{
	return is_locked;
}
