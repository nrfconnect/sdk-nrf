/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Replacement implementation of Zephyr OpenThread L2 functions used by the OpenThread RPC server.
 * This enables building the OpenThread RPC server without Zephyr OpenThread L2.
 */

#include <zephyr/net/openthread.h>

struct openthread_context *openthread_get_default_context(void)
{
	static struct openthread_context ot_context;

	return &ot_context;
}

struct otInstance *openthread_get_default_instance(void)
{
	struct openthread_context *ot_context = openthread_get_default_context();

	return ot_context ? ot_context->instance : NULL;
}

void openthread_api_mutex_lock(struct openthread_context *ot_context)
{
	(void)k_mutex_lock(&ot_context->api_lock, K_FOREVER);
}

int openthread_api_mutex_try_lock(struct openthread_context *ot_context)
{
	return k_mutex_lock(&ot_context->api_lock, K_NO_WAIT);
}

void openthread_api_mutex_unlock(struct openthread_context *ot_context)
{
	(void)k_mutex_unlock(&ot_context->api_lock);
}
