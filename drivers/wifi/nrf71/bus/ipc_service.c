/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief IPC service layer of the nRF71 Wi-Fi driver.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

#include "ipc_service.h"

static void wifi_ipc_ep_bound(void *priv)
{
	wifi_ipc_t *context = (wifi_ipc_t *)priv;

	context->busy_q.ipc_ready = true;
}

/* For single-endpoint TX+RX bind: set both contexts ready when endpoint is bound */
static wifi_ipc_t *wifi_ipc_rx_ctx_shared;

static void wifi_ipc_ep_bound_tx_rx(void *priv)
{
	wifi_ipc_t *tx_ctx = (wifi_ipc_t *)priv;

	tx_ctx->busy_q.ipc_ready = true;
	if (wifi_ipc_rx_ctx_shared != NULL) {
		wifi_ipc_rx_ctx_shared->busy_q.ipc_ready = true;
	}
}

static void wifi_ipc_recv_callback(const void *data, size_t len, void *priv)
{
	wifi_ipc_t *context = (wifi_ipc_t *)priv;

	if (context->busy_q.recv_cb != NULL) {
		context->busy_q.recv_cb((void *)data, len, context->busy_q.priv);
	}

	if (context->send_ack) {
		while (!ipc_service_send(&context->busy_q.ipc_ep, data, len)) {
			/* Retry until success */
		}
	}
}

static void wifi_ipc_busyq_init(wifi_ipc_busyq_t *busyq, const ipc_device_wrapper_t *ipc_inst,
				void (*rx_cb)(void *data, size_t len, void *priv), void *priv)
{
	busyq->ipc_inst = ipc_inst;
	busyq->ipc_ep_cfg.cb.bound = wifi_ipc_ep_bound;
	busyq->recv_cb = rx_cb;
	busyq->ipc_ready = false;
	busyq->priv = priv;
	busyq->ipc_ep_cfg.cb.received = wifi_ipc_recv_callback;
}

static wifi_ipc_status_t wifi_ipc_busyq_register(wifi_ipc_t *context)
{
	int ret;
	const struct device *ipc_instance = GET_IPC_INSTANCE(context->busy_q.ipc_inst);

	ret = ipc_service_open_instance(ipc_instance);
	if (ret < 0) {
		return WIFI_IPC_STATUS_INIT_ERR;
	}

	context->busy_q.ipc_ep_cfg.name = "ep";
	context->busy_q.ipc_ep_cfg.priv = context;

	ret = ipc_service_register_endpoint(ipc_instance, &context->busy_q.ipc_ep,
						&context->busy_q.ipc_ep_cfg);
	if (ret < 0 && ret != -EALREADY) {
		return WIFI_IPC_STATUS_INIT_ERR;
	}

	LOG_INF("IPC busy queue registered");

	return WIFI_IPC_STATUS_OK;
}

wifi_ipc_status_t wifi_ipc_bind_ipc_service(wifi_ipc_t *context,
						const ipc_device_wrapper_t *ipc_inst,
						void (*rx_cb)(void *data, size_t len, void *priv),
						void *priv)
{
	wifi_ipc_busyq_init(&context->busy_q, ipc_inst, rx_cb, priv);

	return wifi_ipc_busyq_register(context);
}

wifi_ipc_status_t wifi_ipc_bind_ipc_service_tx_rx(wifi_ipc_t *tx_context,
						  wifi_ipc_t *rx_context,
						  const ipc_device_wrapper_t *ipc_inst,
						  void (*rx_cb)(void *data, size_t len, void *priv),
						  void *priv)
{
	wifi_ipc_status_t ret;

	wifi_ipc_rx_ctx_shared = rx_context;
	wifi_ipc_busyq_init(&tx_context->busy_q, ipc_inst, rx_cb, priv);
	tx_context->busy_q.ipc_ep_cfg.cb.bound = wifi_ipc_ep_bound_tx_rx;

	ret = wifi_ipc_busyq_register(tx_context);
	wifi_ipc_rx_ctx_shared = NULL;

	return ret;
}


static wifi_ipc_status_t wifi_ipc_busyq_send(wifi_ipc_t *context, const void *data, size_t len)
{
	wifi_ipc_busyq_t *busyq = &context->busy_q;

	if (!busyq->ipc_ready) {
		LOG_ERR("IPC service is not ready");
		return WIFI_IPC_STATUS_BUSYQ_NOTREADY;
	}

	LOG_DBG("IPC send: len %d", len);

	int ret = ipc_service_send(&busyq->ipc_ep, (const void *)data, len);

	if (ret == -ENOMEM) {
		LOG_ERR("IPC send: ENOMEM");
		return WIFI_IPC_STATUS_BUSYQ_FULL;
	} else if (ret < 0) {
		LOG_ERR("IPC send: Critical IPC failure: %d", ret);
		return WIFI_IPC_STATUS_BUSYQ_CRITICAL_ERR;
	}

	return WIFI_IPC_STATUS_OK;
}

wifi_ipc_status_t wifi_ipc_host_tx_init(wifi_ipc_t *context, uint32_t addr_freeq)
{
	context->send_ack = false;

	return WIFI_IPC_STATUS_OK;
}

wifi_ipc_status_t wifi_ipc_host_rx_init(wifi_ipc_t *context, uint32_t addr_freeq)
{
	context->send_ack = false;

	return WIFI_IPC_STATUS_OK;
}

wifi_ipc_status_t wifi_ipc_host_tx_send(wifi_ipc_t *context,
					const void *data,
					size_t len,
					uint32_t *ack_addr)
{
	wifi_ipc_buf_desc_t msg_info = { 0 };

	msg_info.addr = (uint32_t)data;
	msg_info.size = len;
	msg_info.ack_addr = ack_addr;

	return wifi_ipc_busyq_send(context, &msg_info, sizeof(msg_info));
}
