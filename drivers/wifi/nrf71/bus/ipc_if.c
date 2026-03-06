/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief IPC bus layer of the nRF71 Wi-Fi driver.
 *
 * Design: single IPC endpoint (ipc0) for both TX and RX.
 * - Host sends (addr, size, ack_addr) for TX.
 * - RPU writes completion to ack_addr directly, no IPC ACK.
 * - UMAC sends (event addr, size, ring) for RX; Host processes then frees
 *   via tail write.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

#include "ipc_if.h"
#include "bal_structs.h"
#include "qspi.h"
#include "common/hal_structs_common.h"
#include "osal_api.h"

#define NUM_INSTANCES 1
#define NUM_ENDPOINTS 1

struct device *ipc_instances[NUM_INSTANCES];
struct ipc_ept ept[NUM_ENDPOINTS];
struct ipc_ept_cfg ept_cfg[NUM_ENDPOINTS];

/* Single endpoint: TX (Host->UMAC) and RX (UMAC->Host) on same channel */
static wifi_ipc_t wifi_host_tx;
static wifi_ipc_t wifi_host_rx;

static int (*callback_func)(void *data);

#define IPC_TX_ACK_SLOTS 64

static K_MUTEX_DEFINE(host_tx_ack_lock);
static uint32_t host_tx_ack_slots[IPC_TX_ACK_SLOTS];
static const void *host_tx_pending_bufs[IPC_TX_ACK_SLOTS];

static void host_tx_reclaim_completed(void)
{
	int i;

	k_mutex_lock(&host_tx_ack_lock, K_FOREVER);

	for (i = 0; i < IPC_TX_ACK_SLOTS; i++) {
		uint32_t completed_addr;

		completed_addr = host_tx_ack_slots[i];

		if ((completed_addr == 0U) || (host_tx_pending_bufs[i] == NULL)) {
			continue;
		}

		if (completed_addr !=
		    (uint32_t)(uintptr_t)host_tx_pending_bufs[i]) {
			LOG_WRN("Unexpected TX completion addr 0x%x in slot %d",
				completed_addr, i);
			continue;
		}

		nrf_wifi_osal_mem_free((void *)host_tx_pending_bufs[i]);
		host_tx_pending_bufs[i] = NULL;
		host_tx_ack_slots[i] = 0U;
	}

	k_mutex_unlock(&host_tx_ack_lock);
}

static uint32_t *host_tx_ack_slot_alloc(const void *data)
{
	int i;

	k_mutex_lock(&host_tx_ack_lock, K_FOREVER);

	for (i = 0; i < IPC_TX_ACK_SLOTS; i++) {
		if (host_tx_pending_bufs[i] != NULL) {
			continue;
		}

		host_tx_ack_slots[i] = 0U;
		host_tx_pending_bufs[i] = data;
		k_mutex_unlock(&host_tx_ack_lock);

		return &host_tx_ack_slots[i];
	}

	k_mutex_unlock(&host_tx_ack_lock);

	return NULL;
}

static void host_tx_ack_slot_free(uint32_t *ack_addr)
{
	int i;

	if (ack_addr == NULL) {
		return;
	}

	k_mutex_lock(&host_tx_ack_lock, K_FOREVER);

	for (i = 0; i < IPC_TX_ACK_SLOTS; i++) {
		if (&host_tx_ack_slots[i] != ack_addr) {
			continue;
		}

		host_tx_ack_slots[i] = 0U;
		host_tx_pending_bufs[i] = NULL;
		break;
	}

	k_mutex_unlock(&host_tx_ack_lock);
}

/* RX path: receive event (addr, size, ring); after handler, free by writing tail to GDRAM */
static void host_rx_recv(void *data, size_t len, void *priv)
{
	struct nrf_wifi_bus_qspi_dev_ctx *dev_ctx = (struct nrf_wifi_bus_qspi_dev_ctx *)priv;
	struct nrf_wifi_bal_dev_ctx *bal_dev_ctx = dev_ctx->bal_dev_ctx;
	struct nrf_wifi_hal_dev_ctx *hal_dev_ctx = bal_dev_ctx->hal_dev_ctx;
	wifi_ipc_buf_desc_t msg_info = *(wifi_ipc_buf_desc_t *)data;

	LOG_DBG("Host RX IPC received");
	hal_dev_ctx->ipc_msg = (void *)msg_info.addr;
	callback_func(priv);
	wifi_ipc_host_rx_free_event(&msg_info);
	LOG_DBG("Host RX IPC callback completed");
}

void wifi_ipc_host_rx_free_event(const wifi_ipc_buf_desc_t *event_info)
{
	volatile uint32_t *p_tail = (volatile uint32_t *)event_info->ring.tail_addr;
	uint32_t new_tail;

	if (event_info->ring.padded) {
		/* Ring was padded; tail advances past the event to ring base. */
		new_tail = event_info->size;
	} else {
		new_tail = (*p_tail + event_info->size) % event_info->ring.size;
	}
	*p_tail = new_tail;
}

int ipc_init(void)
{
	int i;

	wifi_ipc_host_tx_init(&wifi_host_tx, 0);
	wifi_ipc_host_rx_init(&wifi_host_rx, 0);
	wifi_host_rx.send_ack = false;

	for (i = 0; i < IPC_TX_ACK_SLOTS; i++) {
		host_tx_ack_slots[i] = 0U;
		host_tx_pending_bufs[i] = NULL;
	}

	LOG_DBG("IPC host single endpoint (ipc0) TX+RX initialized");
	return 0;
}

int ipc_deinit(void)
{
	return 0;
}

int ipc_recv(ipc_ctx_t ctx, void *data, int len)
{
	return 0;
}

int ipc_send(ipc_ctx_t ctx, const void *data, int len)
{
	int ret = 0;
	wifi_ipc_status_t status;

	LOG_DBG("IPC send: inst %d, len %d", ctx.inst, len);
	switch (ctx.inst) {
	case IPC_INSTANCE_CMD_CTRL:
	case IPC_INSTANCE_CMD_TX: {
		uint32_t *ack_addr;

		host_tx_reclaim_completed();
		ack_addr = host_tx_ack_slot_alloc(data);
		if (ack_addr == NULL) {
			LOG_ERR("No free TX ack slots");
			return -1;
		}

		/* TX path: send (addr, size, ack_addr) on IPC0 */
		do {
			status = wifi_ipc_host_tx_send(&wifi_host_tx,
						       data,
						       (size_t)len,
						       ack_addr);
		} while (status == WIFI_IPC_STATUS_BUSYQ_NOTREADY);

		if (status != WIFI_IPC_STATUS_OK) {
			host_tx_ack_slot_free(ack_addr);

			if (status == WIFI_IPC_STATUS_BUSYQ_CRITICAL_ERR) {
				LOG_ERR("Critical error during IPC host TX transfer");
				return -1;
			}
		}

		ret = (status == WIFI_IPC_STATUS_OK) ? 0 : -1;
		break;
	}
	case IPC_INSTANCE_RX:
		break;
	case IPC_INSTANCE_EVT:
		break;
	default:
		break;
	}

	LOG_DBG("IPC send completed: %d", ret);
	return ret;
}

int ipc_register_rx_cb(int (*rx_handler)(void *priv), void *data)
{
	int ret;

	callback_func = rx_handler;

	ret = wifi_ipc_bind_ipc_service_tx_rx(&wifi_host_tx, &wifi_host_rx,
					     DEVICE_DT_GET(DT_NODELABEL(ipc0)),
					     host_rx_recv, data);
	if (ret != WIFI_IPC_STATUS_OK) {
		LOG_ERR("Failed to bind IPC host TX+RX (ipc0): %d", ret);
		return -1;
	}

	return 0;
}
