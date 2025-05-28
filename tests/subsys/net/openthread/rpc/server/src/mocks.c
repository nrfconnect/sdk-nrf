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
#include <zephyr/ztest.h>

static bool locked;
static int dummy;

struct otInstance *openthread_get_default_instance(void)
{
	return (struct otInstance *)&dummy;
}

void ot_rpc_mutex_lock(void)
{
	/*
	 * The unit tests are executed in a single thread but implement these mocks to
	 * validate that each command decoder unlocks the stack after processing command,
	 * and each callback encoder locks the stack after invoking the callback.
	 */
	zassert_false(locked);
	locked = true;
}

void ot_rpc_mutex_unlock(void)
{
	zassert_true(locked);
	locked = false;
}
