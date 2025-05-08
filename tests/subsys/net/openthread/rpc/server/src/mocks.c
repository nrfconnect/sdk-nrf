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

#include <zephyr/net/openthread.h>

static struct openthread_context ot_context;

struct otInstance *openthread_get_default_instance(void)
{
	return ot_context.instance;
}

void ot_rpc_mutex_lock(void)
{
	(void)k_mutex_lock(&ot_context.api_lock, K_FOREVER);
}

void ot_rpc_mutex_unlock(void)
{
	(void)k_mutex_unlock(&ot_context.api_lock);
}
