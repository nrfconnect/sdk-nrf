/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IPC_SERVICE_H
#define IPC_SERVICE_H

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include "spsc_qm.h"

#define GET_IPC_INSTANCE(dev) (dev)
typedef struct device ipc_device_wrapper_t;

#include <stdint.h>
#include <stdbool.h>

/*
 * Must be large enough to contain the internal struct (spsc_pbuf struct) and
 * at least two bytes of data (one is reserved for written message length)
 */
#define _MIN_SPSC_SIZE            (sizeof(spsc_queue_t) + sizeof(uint32_t))
/*
 * TODO: Unsure why some additional bytes are needed for overhead.
 */
#define WIFI_IPC_GET_SPSC_SIZE(x) (_MIN_SPSC_SIZE + 12 + (x))

/* 4 x cmd location 32-bit pointers of 400 bytes each */
#define WIFI_IPC_CMD_SIZE      400
#define WIFI_IPC_CMD_NUM       4
#define WIFI_IPC_CMD_SPSC_SIZE WIFI_IPC_GET_SPSC_SIZE(WIFI_IPC_CMD_NUM * sizeof(uint32_t))

/* 7 x event location 32-bit pointers of 1000 bytes each */
#define WIFI_IPC_EVENT_SIZE      1000
#define WIFI_IPC_EVENT_NUM       7
#define WIFI_IPC_EVENT_SPSC_SIZE WIFI_IPC_GET_SPSC_SIZE(WIFI_IPC_EVENT_NUM * sizeof(uint32_t))

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
	void (*recv_cb)(void *data, void *priv);
	void *priv;
	volatile bool ipc_ready;
} wifi_ipc_busyq_t;

typedef struct {
	spsc_queue_t *free_q;

	wifi_ipc_busyq_t busy_q;
	wifi_ipc_busyq_t *linked_ipc;
} wifi_ipc_t;

wifi_ipc_status_t wifi_ipc_bind_ipc_service(wifi_ipc_t *p_context,
						const ipc_device_wrapper_t *ipc_inst,
						void (*rx_cb)(void *data, void *priv), void *priv);

wifi_ipc_status_t wifi_ipc_bind_ipc_service_tx_rx(wifi_ipc_t *p_tx, wifi_ipc_t *p_rx,
						  const ipc_device_wrapper_t *ipc_inst,
						  void (*rx_cb)(void *data, void *priv),
						  void *priv);

wifi_ipc_status_t wifi_ipc_freeq_get(wifi_ipc_t *p_context, uint32_t *data);

wifi_ipc_status_t wifi_ipc_freeq_send(wifi_ipc_t *p_context, uint32_t data);

wifi_ipc_status_t wifi_ipc_busyq_send(wifi_ipc_t *p_context, uint32_t *data);

wifi_ipc_status_t wifi_ipc_host_cmd_init(wifi_ipc_t *p_context, uint32_t addr_freeq);

wifi_ipc_status_t wifi_ipc_host_event_init(wifi_ipc_t *p_context, uint32_t addr_freeq);

wifi_ipc_status_t wifi_ipc_host_cmd_get(wifi_ipc_t *p_context, uint32_t *p_data);

wifi_ipc_status_t wifi_ipc_host_cmd_send(wifi_ipc_t *p_context, uint32_t *p_data);

wifi_ipc_status_t wifi_ipc_host_cmd_send_memcpy(wifi_ipc_t *p_context, const void *p_msg,
						size_t len);

wifi_ipc_status_t wifi_ipc_host_tx_send(wifi_ipc_t *p_context, const void *p_msg);

#endif /* IPC_SERVICE_H */
