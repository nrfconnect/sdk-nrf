/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#if CONFIG_NRF_RPC_IPC_SERVICE
#include <nrf_rpc/nrf_rpc_ipc.h>
#elif CONFIG_NRF_RPC_UART_TRANSPORT
#include <nrf_rpc/nrf_rpc_uart.h>
#endif
#include <nrf_rpc_cbor.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#if CONFIG_NRF_RPC_IPC_SERVICE
NRF_RPC_IPC_TRANSPORT(ot_group_tr, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "ot_rpc_ept");
#elif CONFIG_NRF_RPC_UART_TRANSPORT && !CONFIG_BT_RPC
NRF_RPC_UART_TRANSPORT(rpc_tr, DEVICE_DT_GET(DT_NODELABEL(uart1)));
#elif CONFIG_NRF_RPC_UART_TRANSPORT && CONFIG_BT_RPC
extern const struct nrf_rpc_tr rpc_tr;
#endif
NRF_RPC_GROUP_DEFINE(ot_group, "ot", &rpc_tr, NULL, NULL, NULL);
LOG_MODULE_REGISTER(ot_rpc, LOG_LEVEL_DBG);

#ifdef CONFIG_OPENTHREAD_RPC_INITIALIZE_NRF_RPC
static void err_handler(const struct nrf_rpc_err_report *report)
{
	LOG_ERR("nRF RPC error %d ocurred. See nRF RPC logs for more details", report->code);
	k_oops();
}

static int serialization_init(void)
{
	int err;

	err = nrf_rpc_init(err_handler);
	if (err) {
		return -EINVAL;
	}

	return 0;
}

SYS_INIT(serialization_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
#endif /* CONFIG_OPENTHREAD_RPC_INITIALIZE_NRF_RPC */
