/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LOG_RPC_INTERNAL_H_
#define LOG_RPC_INTERNAL_H_

#include <nrf_rpc.h>
#include <zephyr/device.h>

#ifdef CONFIG_NRF_RPC_IPC_SERVICE
#include <nrf_rpc/nrf_rpc_ipc.h>
#elif defined(CONFIG_NRF_RPC_UART_TRANSPORT)
#include <nrf_rpc/nrf_rpc_uart.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_NRF_RPC_IPC_SERVICE
NRF_RPC_IPC_TRANSPORT(log_rpc_tr, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "log_rpc_ept");
#elif defined(CONFIG_NRF_RPC_UART_TRANSPORT)
#define log_rpc_tr NRF_RPC_UART_TRANSPORT(DT_CHOSEN(nordic_rpc_uart))
#endif
NRF_RPC_GROUP_DEFINE(log_rpc_group, "log", &log_rpc_tr, NULL, NULL, NULL);

enum log_rpc_evt_forwarder {
	LOG_RPC_EVT_MSG = 0,
	LOG_RPC_EVT_HISTORY_THRESHOLD_REACHED = 1,
};

enum log_rpc_cmd_forwarder {
	LOG_RPC_CMD_PUT_HISTORY_CHUNK = 0,
};

enum log_rpc_cmd_backend {
	LOG_RPC_CMD_SET_STREAM_LEVEL = 0,
	LOG_RPC_CMD_SET_HISTORY_LEVEL,
	LOG_RPC_CMD_GET_HISTORY_USAGE_THRESHOLD,
	LOG_RPC_CMD_SET_HISTORY_USAGE_THRESHOLD,
	LOG_RPC_CMD_GET_HISTORY_USAGE_SIZE,
	LOG_RPC_CMD_GET_HISTORY_MAX_SIZE,
	LOG_RPC_CMD_FETCH_HISTORY,
	LOG_RPC_CMD_STOP_FETCH_HISTORY,
	LOG_RPC_CMD_GET_CRASH_DUMP,
	LOG_RPC_CMD_INVALIDATE_CRASH_DUMP,
	LOG_RPC_CMD_ECHO,
	LOG_RPC_CMD_SET_TIME,
	LOG_RPC_CMD_GET_CRASH_INFO,
};

#ifdef __cplusplus
}
#endif

#endif /* LOG_RPC_INTERNAL_H_ */
