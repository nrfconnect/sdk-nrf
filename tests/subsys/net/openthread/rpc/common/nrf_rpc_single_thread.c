/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Replacement implementation of selected nRF RPC OS functions, which enables single-threaded
 * processing of a received nRF RPC command.
 *
 * Typically, an nRF RPC command that initiates a conversation is dispatched by the nRF RPC core
 * using a dedicated thread pool. In unit tests, however, it is preferable to dispatch the command
 * synchronously so that no operation timeouts are needed to detect a test case failure.
 */

#include <nrf_rpc_os.h>

#include <zephyr/ztest.h>

static nrf_rpc_os_work_t receive_callback;

int __real_nrf_rpc_os_init(nrf_rpc_os_work_t callback);

int __wrap_nrf_rpc_os_init(nrf_rpc_os_work_t callback)
{
	receive_callback = callback;

	return __real_nrf_rpc_os_init(callback);
}

void __wrap_nrf_rpc_os_thread_pool_send(const uint8_t *data, size_t len)
{
	zassert_not_null(receive_callback);

	receive_callback(data, len);
}
