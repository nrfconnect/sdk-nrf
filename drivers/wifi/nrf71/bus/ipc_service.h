/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IPC_SERVICE_H
#define IPC_SERVICE_H

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>

#define GET_IPC_INSTANCE(dev) (dev)
typedef struct device ipc_device_wrapper_t;

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @struct wifi_ipc_buf_desc_t
 * @brief Descriptor for TX (Host->UMAC) and RX (UMAC->Host) message.
 *        Host sends (addr, size) for TX; UMAC sends (addr, size) for RX events.
 *        Host sends same descriptor back on RX free to notify UMAC to release ring slot.
 */
typedef struct wifi_ipc_buf_desc {
	uint32_t addr;
	uint32_t size;
} wifi_ipc_buf_desc_t;

/**
 * @enum wifi_ipc_status_t
 * @brief Status codes for the Wi-Fi IPC service.
 */
typedef enum {
	WIFI_IPC_STATUS_OK = 0,
	WIFI_IPC_STATUS_INIT_ERR,
	WIFI_IPC_STATUS_FREEQ_UNINIT_ERR,
	WIFI_IPC_STATUS_FREEQ_EMPTY,
	WIFI_IPC_STATUS_FREEQ_INVALID,
	WIFI_IPC_STATUS_FREEQ_FULL,
	WIFI_IPC_STATUS_BUSYQ_NOTREADY,
	WIFI_IPC_STATUS_BUSYQ_FULL,
	WIFI_IPC_STATUS_BUSYQ_CRITICAL_ERR,
} wifi_ipc_status_t;

typedef struct {
	const ipc_device_wrapper_t *ipc_inst;
	struct ipc_ept ipc_ep;
	struct ipc_ept_cfg ipc_ep_cfg;
	void (*recv_cb)(void *data, size_t len, void *priv);
	void *priv;
	volatile bool ipc_ready;
} wifi_ipc_busyq_t;

typedef struct {
	wifi_ipc_busyq_t busy_q;
	bool send_ack;
} wifi_ipc_t;

wifi_ipc_status_t wifi_ipc_bind_ipc_service(wifi_ipc_t *p_context,
						const ipc_device_wrapper_t *ipc_inst,
						void (*rx_cb)(void *data, size_t len, void *priv),
						void *priv);

wifi_ipc_status_t wifi_ipc_host_tx_init(wifi_ipc_t *p_context, uint32_t addr_freeq);

wifi_ipc_status_t wifi_ipc_host_rx_init(wifi_ipc_t *p_context, uint32_t addr_freeq);

wifi_ipc_status_t wifi_ipc_host_tx_send(wifi_ipc_t *p_context, const void *p_msg, size_t len);
#endif /* IPC_SERVICE_H */
