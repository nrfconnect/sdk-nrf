/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing API definitions for the
 * HAL Layer of the Wi-Fi driver in the offloaded raw TX
 * mode of operation
 */

#include <common/mem_mgmt.h>
#include <common/work_mgmt.h>
#include <queue.h>
#include <common/hal_structs_common.h>
#include <common/hal_api_common.h>
#include <offload_raw_tx/hal_api.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

static void event_tasklet_fn(unsigned long data)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_hal_dev_ctx *hal_dev_ctx = NULL;
	unsigned long flags = 0;

	hal_dev_ctx = (struct nrf_wifi_hal_dev_ctx *)data;

	nrf_wifi_osal_spinlock_irq_take(hal_dev_ctx->lock_rx,
					&flags);

	if (hal_dev_ctx->hal_status != NRF_WIFI_HAL_STATUS_ENABLED) {
		/* Ignore the interrupt if the HAL is not enabled */
		status = NRF_WIFI_STATUS_SUCCESS;
		goto out;
	}

	status = hal_rpu_eventq_process(hal_dev_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Event queue processing failed",
				      __func__);
	}

out:
	nrf_wifi_osal_spinlock_irq_rel(hal_dev_ctx->lock_rx,
				       &flags);
}


struct nrf_wifi_hal_dev_ctx *nrf_wifi_off_raw_tx_hal_dev_add(struct nrf_wifi_hal_priv *hpriv,
							     void *mac_dev_ctx)
{
	struct nrf_wifi_hal_dev_ctx *hal_dev_ctx = NULL;

	hal_dev_ctx = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*hal_dev_ctx));

	if (!hal_dev_ctx) {
		LOG_ERR("%s: Unable to allocate hal_dev_ctx",
				      __func__);
		goto err;
	}

	hal_dev_ctx->hpriv = hpriv;
	hal_dev_ctx->mac_dev_ctx = mac_dev_ctx;
	hal_dev_ctx->idx = hpriv->num_devs++;

	hal_dev_ctx->cmd_q = nrf_wifi_utils_ctrl_q_alloc();

	if (!hal_dev_ctx->cmd_q) {
		LOG_ERR("%s: Unable to allocate command queue",
				      __func__);
		goto hal_dev_free;
	}

	hal_dev_ctx->event_q = nrf_wifi_utils_ctrl_q_alloc();

	if (!hal_dev_ctx->event_q) {
		LOG_ERR("%s: Unable to allocate event queue",
				      __func__);
		goto cmd_q_free;
	}

	hal_dev_ctx->lock_hal = nrf_wifi_osal_spinlock_alloc();

	if (!hal_dev_ctx->lock_hal) {
		LOG_ERR("%s: Unable to allocate HAL lock", __func__);
		hal_dev_ctx = NULL;
		goto event_q_free;
	}

	nrf_wifi_osal_spinlock_init(hal_dev_ctx->lock_hal);

	hal_dev_ctx->lock_rx = nrf_wifi_osal_spinlock_alloc();

	if (!hal_dev_ctx->lock_rx) {
		LOG_ERR("%s: Unable to allocate HAL lock",
				      __func__);
		goto lock_hal_free;
	}

	nrf_wifi_osal_spinlock_init(hal_dev_ctx->lock_rx);

	hal_dev_ctx->event_tasklet = nrf_wifi_work_alloc(ZEP_WORK_TYPE_BH);

	if (!hal_dev_ctx->event_tasklet) {
		LOG_ERR("%s: Unable to allocate event_tasklet",
				      __func__);
		goto lock_rx_free;
	}

	nrf_wifi_work_init(hal_dev_ctx->event_tasklet,
				   event_tasklet_fn,
				   (unsigned long)hal_dev_ctx);

	hal_dev_ctx->bal_dev_ctx = nrf_wifi_bal_dev_add(hpriv->bpriv,
							hal_dev_ctx);

	if (!hal_dev_ctx->bal_dev_ctx) {
		LOG_ERR("%s: nrf_wifi_bal_dev_add failed",
				      __func__);
		goto lock_recovery_free;
	}

	return hal_dev_ctx;

lock_recovery_free:
	nrf_wifi_osal_spinlock_free(hal_dev_ctx->lock_recovery);
lock_rx_free:
	nrf_wifi_osal_spinlock_free(hal_dev_ctx->lock_rx);
lock_hal_free:
	nrf_wifi_osal_spinlock_free(hal_dev_ctx->lock_hal);
event_q_free:
	nrf_wifi_utils_ctrl_q_free(hal_dev_ctx->event_q);
cmd_q_free:
	nrf_wifi_utils_ctrl_q_free(hal_dev_ctx->cmd_q);
hal_dev_free:
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, hal_dev_ctx);
	hal_dev_ctx = NULL;
err:
	return NULL;
}
