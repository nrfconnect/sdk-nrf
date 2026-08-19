/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file ipc_bus.h
 *
 * @brief IPC bus layer for the nRF71 Wi-Fi driver.
 */

#ifndef __IPC_BUS_H__
#define __IPC_BUS_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <common/status.h>
#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/kernel.h>

#define GET_IPC_INSTANCE(dev) (dev)
typedef struct device ipc_device_wrapper_t;

/**
 * Ring buffer descriptor (used by RPU->Host RX path for free without IPC).
 */
typedef struct wifi_ipc_ring_info {
	uint32_t tail_addr;
	uint32_t base;
	uint32_t size;
	bool padded;
} wifi_ipc_ring_info_t;

/**
 * @brief Descriptor for TX (Host->UMAC) and RX (UMAC->Host) messages.
 */
typedef struct wifi_ipc_buf_desc {
	uint32_t addr;
	uint32_t size;
	uint32_t *ack_addr;
	wifi_ipc_ring_info_t ring;
} wifi_ipc_buf_desc_t;

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
	bool ipc_bound;
} wifi_ipc_busyq_t;

typedef struct {
	wifi_ipc_busyq_t busy_q;
	bool send_ack;
} wifi_ipc_t;

wifi_ipc_status_t wifi_ipc_bind_ipc_service(wifi_ipc_t *p_context,
					    const ipc_device_wrapper_t *ipc_inst,
					    void (*rx_cb)(void *data, size_t len, void *priv),
					    void *priv);

wifi_ipc_status_t wifi_ipc_bind_ipc_service_tx_rx(wifi_ipc_t *tx_context,
						  wifi_ipc_t *rx_context,
						  const ipc_device_wrapper_t *ipc_inst,
						  void (*rx_cb)(void *data, size_t len, void *priv),
						  void *priv);

wifi_ipc_status_t wifi_ipc_host_tx_init(wifi_ipc_t *p_context, uint32_t addr_freeq);

wifi_ipc_status_t wifi_ipc_host_rx_init(wifi_ipc_t *p_context, uint32_t addr_freeq);

wifi_ipc_status_t wifi_ipc_host_tx_send(wifi_ipc_t *p_context,
					const void *p_msg,
					size_t len,
					uint32_t *ack_addr);

void wifi_ipc_host_rx_free_event(const wifi_ipc_buf_desc_t *event_info);

typedef enum {
	IPC_INSTANCE_CMD_CTRL = 0,
	IPC_INSTANCE_CMD_TX,
	IPC_INSTANCE_EVT,
	IPC_INSTANCE_RX
} ipc_instances_nrf71_t;

typedef enum {
	IPC_EPT_UMAC = 0,
	IPC_EPT_LMAC
} ipc_epts_nrf71_t;

typedef struct ipc_ctx {
	ipc_instances_nrf71_t inst;
	ipc_epts_nrf71_t ept;
} ipc_ctx_t;

int ipc_init(void);
int ipc_deinit(void);
int ipc_send(ipc_ctx_t ctx, const void *data, int len);
int ipc_recv(ipc_ctx_t ctx, void *data, int len);
int ipc_register_rx_cb(int (*rx_handler)(void *priv), void *data);
void ipc_unregister_rx_cb(void);

/**
 * @brief Structure to hold context information for the IPC bus.
 */
struct nrf_wifi_bus_ipc_priv {
	enum nrf_wifi_status (*intr_callbk_fn)(void *hal_ctx);
};

/**
 * @brief Structure to hold the device context for the IPC bus.
 */
struct nrf_wifi_bus_ipc_dev_ctx {
	struct nrf_wifi_bus_ipc_priv *ipc_priv;
	void *bal_dev_ctx;
	unsigned long host_addr_base;
};

/**
 * @brief IPC message types for host-to-RPU command send.
 */
enum nrf_wifi_ipc_msg_type {
	NRF_WIFI_IPC_MSG_CMD_CTRL = 0,
	NRF_WIFI_IPC_MSG_CMD_DATA_RX = 2,
	NRF_WIFI_IPC_MSG_CMD_DATA_TX = 4,
};

#endif /* __IPC_BUS_H__ */
