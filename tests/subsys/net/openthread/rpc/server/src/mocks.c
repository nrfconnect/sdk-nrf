/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Mock implementation of Zephyr OpenThread L2 and OpenThread RPC Lock APIs used by OT RPC server.
 * This enables building the OpenThread RPC server without Zephyr OpenThread L2.
 */

#include <ot_rpc_lock.h>

#include <openthread.h>

static int dummy;

struct otInstance *openthread_get_default_instance(void)
{
	return (struct otInstance *)&dummy;
}

void ot_rpc_mutex_lock(void)
{
}

void ot_rpc_mutex_unlock(void)
{
}
