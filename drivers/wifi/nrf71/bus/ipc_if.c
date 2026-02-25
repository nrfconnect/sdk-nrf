/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief IPC bus layer of the nRF71 Wi-Fi driver.
 *
 * Design:
 * - IPC0: TX path. Host sends (addr, size); UMAC ACKs on same channel when ISR returns.
 * - IPC1: RX path. UMAC sends (event addr, size in GDRAM); Host processes then calls free API.
 * - No explicit free/busy queues; icmsg pbufs handle queuing.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

#include "ipc_if.h"
#include "bal_structs.h"
#include "qspi.h"
#include "common/hal_structs_common.h"
#include "osal_api.h"

#define NUM_INSTANCES 2
#define NUM_ENDPOINTS 2

struct device *ipc_instances[NUM_INSTANCES];
struct ipc_ept ept[NUM_ENDPOINTS];
struct ipc_ept_cfg ept_cfg[NUM_ENDPOINTS];

/* IPC0 = TX path (Host->UMAC), IPC1 = RX path (UMAC->Host) */
static wifi_ipc_t wifi_host_tx;
static wifi_ipc_t wifi_host_rx;

static int (*callback_func)(void *data);

/* TX path: on ACK from UMAC, free the TX buffer (was allocated with mem_zalloc) */
static void host_tx_ack_recv(void *data, size_t len, void *priv)
{
	wifi_ipc_buf_desc_t *desc = (wifi_ipc_buf_desc_t *)data;

	(void)len;
	(void)priv;

	LOG_DBG("Host TX ACK received");
	/* ACK payload is (addr, size); free the buffer we sent, not the IPC rx buffer */
	nrf_wifi_osal_mem_free((void *)(uintptr_t)desc->addr);
}

/* RX path: receive event (addr, size); after handler, notify UMAC to free ring slot */
static void host_rx_recv(void *data, size_t len, void *priv)
{
	struct nrf_wifi_bus_qspi_dev_ctx *dev_ctx = (struct nrf_wifi_bus_qspi_dev_ctx *)priv;
	struct nrf_wifi_bal_dev_ctx *bal_dev_ctx = dev_ctx->bal_dev_ctx;
	struct nrf_wifi_hal_dev_ctx *hal_dev_ctx = bal_dev_ctx->hal_dev_ctx;
	wifi_ipc_buf_desc_t msg_info = *(wifi_ipc_buf_desc_t *)data;

	LOG_DBG("Host RX IPC received");
	hal_dev_ctx->ipc_msg = (void *)msg_info.addr;
	callback_func(priv);
	LOG_DBG("Host RX IPC callback completed");
}

int ipc_init(void)
{
	wifi_ipc_host_tx_init(&wifi_host_tx, 0);
	wifi_ipc_host_rx_init(&wifi_host_rx, 0);
	wifi_host_rx.send_ack = true;
	LOG_DBG("IPC host TX (IPC0) / RX (IPC1) initialized");
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
	case IPC_INSTANCE_CMD_TX:
		/* TX path: send (addr, size) on IPC0 */
		do {
			status = wifi_ipc_host_tx_send(&wifi_host_tx, data, (size_t)len);
		} while (status == WIFI_IPC_STATUS_BUSYQ_NOTREADY);

		if (status == WIFI_IPC_STATUS_BUSYQ_CRITICAL_ERR) {
			LOG_ERR("Critical error during IPC host TX transfer");
			return -1;
		}
		ret = (status == WIFI_IPC_STATUS_OK) ? 0 : -1;
		break;
	case IPC_INSTANCE_RX:
		/* RX free is sent from host_rx_recv after processing */
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

	/* IPC0 = TX path: send (addr, size), receive ACK */
	ret = wifi_ipc_bind_ipc_service(&wifi_host_tx,
					DEVICE_DT_GET(DT_NODELABEL(ipc0)),
					host_tx_ack_recv, data);
	if (ret != WIFI_IPC_STATUS_OK) {
		LOG_ERR("Failed to bind IPC host TX (ipc0): %d", ret);
		return -1;
	}

	/* IPC1 = RX path: receive (addr, size), send free after processing */
	ret = wifi_ipc_bind_ipc_service(&wifi_host_rx,
					DEVICE_DT_GET(DT_NODELABEL(ipc1)),
					host_rx_recv, data);
	if (ret != WIFI_IPC_STATUS_OK) {
		LOG_ERR("Failed to bind IPC host RX (ipc1): %d", ret);
		return -1;
	}

	return 0;
}
