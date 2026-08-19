/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing API definitions for the
 * HAL Layer of the Wi-Fi driver in the system mode of operation.
 */

#include <common/mem_mgmt.h>
#include <common/lock_mgmt.h>
#include "common/hal_structs_common.h"
#include "common/hal_api_common.h"
#include "system/hal_api.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

struct nrf_wifi_hal_dev_ctx *nrf_wifi_sys_hal_dev_add(struct nrf_wifi_hal_priv *hpriv,
						      void *mac_dev_ctx)
{
	struct nrf_wifi_hal_dev_ctx *hal_dev_ctx = NULL;

	hal_dev_ctx = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*hal_dev_ctx));

	if (!hal_dev_ctx) {
		LOG_ERR("%s: Unable to allocate hal_dev_ctx", __func__);
		goto err;
	}

	hal_dev_ctx->hpriv = hpriv;
	hal_dev_ctx->mac_dev_ctx = mac_dev_ctx;
	hal_dev_ctx->idx = hpriv->num_devs++;

	hal_dev_ctx->lock_hal = nrf_wifi_lock_alloc();

	if (!hal_dev_ctx->lock_hal) {
		LOG_ERR("%s: Unable to allocate HAL lock", __func__);
		goto hal_dev_free;
	}

	nrf_wifi_lock_init(hal_dev_ctx->lock_hal);

	hal_dev_ctx->lock_rx = nrf_wifi_lock_alloc();

	if (!hal_dev_ctx->lock_rx) {
		LOG_ERR("%s: Unable to allocate RX lock", __func__);
		goto lock_hal_free;
	}

	nrf_wifi_lock_init(hal_dev_ctx->lock_rx);

	hal_dev_ctx->bal_dev_ctx = nrf_wifi_bal_dev_add(hpriv->bpriv, hal_dev_ctx);

	if (!hal_dev_ctx->bal_dev_ctx) {
		LOG_ERR("%s: nrf_wifi_bal_dev_add failed", __func__);
		goto lock_rx_free;
	}

	return hal_dev_ctx;

lock_rx_free:
	nrf_wifi_lock_free(hal_dev_ctx->lock_rx);
lock_hal_free:
	nrf_wifi_lock_free(hal_dev_ctx->lock_hal);
hal_dev_free:
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, hal_dev_ctx);
err:
	return NULL;
}

void nrf_wifi_sys_hal_lock_rx(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	unsigned long flags = 0;

	nrf_wifi_lock_irq_take(hal_dev_ctx->lock_rx, &flags);
}

void nrf_wifi_sys_hal_unlock_rx(struct nrf_wifi_hal_dev_ctx *hal_dev_ctx)
{
	unsigned long flags = 0;

	nrf_wifi_lock_irq_rel(hal_dev_ctx->lock_rx, &flags);
}
