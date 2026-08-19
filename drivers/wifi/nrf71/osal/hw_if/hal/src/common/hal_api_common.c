/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing API definitions for the
 * HAL Layer of the Wi-Fi driver.
 */

#include <common/mem_mgmt.h>
#include <common/lock_mgmt.h>

#include "common/hal_api_common.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

enum nrf_wifi_status nrf_wifi_hal_ctrl_cmd_send(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx,
						void *cmd,
						unsigned int cmd_size)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

#ifdef NRF_WIFI_CMD_EVENT_LOG
	LOG_INF("%s: caller %p",
		__func__,
		__builtin_return_address(0));
#else
	LOG_DBG("%s: caller %p",
		__func__,
		__builtin_return_address(0));
#endif
	nrf_wifi_lock_take(hal_dev_ctx->lock_hal);
	status = nrf_wifi_ipc_send_msg(hal_dev_ctx->ipc_dev_ctx,
				       NRF_WIFI_HAL_MSG_TYPE_CMD_CTRL,
				       cmd,
				       cmd_size);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Sending command to RPU failed", __func__);
		goto out;
	}
out:
	nrf_wifi_lock_rel(hal_dev_ctx->lock_hal);

	return status;
}

void nrf_wifi_hal_dev_rem(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	nrf_wifi_lock_free(hal_dev_ctx->lock_hal);
	nrf_wifi_lock_free(hal_dev_ctx->lock_rx);

	nrf_wifi_ipc_dev_rem(hal_dev_ctx->ipc_dev_ctx);

	hal_dev_ctx->hpriv->num_devs--;

	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, hal_dev_ctx);
}

enum nrf_wifi_status nrf_wifi_hal_dev_init(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	enum nrf_wifi_status status;

	status = nrf_wifi_ipc_dev_init(hal_dev_ctx->ipc_dev_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_ipc_dev_init failed", __func__);
		return status;
	}

	nrf_wifi_hal_enable(hal_dev_ctx);

	return NRF_WIFI_STATUS_SUCCESS;
}

enum nrf_wifi_status nrf_wifi_hal_ipc_msg_handler(void *priv)
{
	struct nrf_wifi_hal_dev_ctx *hal_dev_ctx = (struct nrf_wifi_hal_dev_ctx *)priv;
	void *event_data = hal_dev_ctx->ipc_msg;
	unsigned int event_len = sizeof(event_data);

	LOG_DBG("%s: IPC message received\n", __func__);

	return hal_dev_ctx->hpriv->intr_callbk_fn(hal_dev_ctx->mac_dev_ctx,
						  event_data,
						  event_len);
}

void nrf_wifi_hal_dev_deinit(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	nrf_wifi_hal_disable(hal_dev_ctx);
	nrf_wifi_ipc_dev_deinit(hal_dev_ctx->ipc_dev_ctx);
}

struct nrf_wifi_hal_priv *nrf_wifi_hal_init(
	enum nrf_wifi_status (*intr_callbk_fn)(void *dev_ctx,
					       void *event_data,
					       unsigned int len))
{
	struct nrf_wifi_hal_priv *hpriv;

	hpriv = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*hpriv));
	if (!hpriv) {
		LOG_ERR("%s: Unable to allocate memory for hpriv", __func__);
		return NULL;
	}

	hpriv->intr_callbk_fn = intr_callbk_fn;

	hpriv->ipc_priv = nrf_wifi_ipc_init(&nrf_wifi_hal_ipc_msg_handler);
	if (!hpriv->ipc_priv) {
		LOG_ERR("%s: Failed", __func__);
		nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, hpriv);
		return NULL;
	}

	return hpriv;
}

void nrf_wifi_hal_deinit(struct nrf_wifi_hal_priv *hpriv)
{
	nrf_wifi_ipc_deinit(hpriv->ipc_priv);

	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, hpriv);
}

void nrf_wifi_hal_enable(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	nrf_wifi_lock_irq_take(hal_dev_ctx->lock_rx, NULL);
	hal_dev_ctx->hal_status = NRF_WIFI_HAL_STATUS_ENABLED;
	nrf_wifi_lock_irq_rel(hal_dev_ctx->lock_rx, NULL);
}

void nrf_wifi_hal_disable(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	nrf_wifi_lock_irq_take(hal_dev_ctx->lock_rx, NULL);
	hal_dev_ctx->hal_status = NRF_WIFI_HAL_STATUS_DISABLED;
	nrf_wifi_lock_irq_rel(hal_dev_ctx->lock_rx, NULL);
}

enum NRF_WIFI_HAL_STATUS nrf_wifi_hal_status_unlocked(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	return hal_dev_ctx->hal_status;
}
