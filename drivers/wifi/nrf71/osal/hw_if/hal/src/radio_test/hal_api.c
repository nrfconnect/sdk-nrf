/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief File containing API definitions for the
 * HAL Layer of the Wi-Fi driver in the radio test mode of operation.
 */

#include "queue.h"
#include "common/hal_structs_common.h"
#include "common/hal_common.h"
#include "common/hal_reg.h"
#include "common/hal_mem.h"
#include "common/hal_interrupt.h"
#include "radio_test/hal_api.h"

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
		nrf_wifi_osal_log_err("%s: Event queue processing failed",
				      __func__);
	}

out:
	nrf_wifi_osal_spinlock_irq_rel(hal_dev_ctx->lock_rx,
				       &flags);
}

struct nrf_wifi_hal_dev_ctx *nrf_wifi_rt_hal_dev_add(struct nrf_wifi_hal_priv *hpriv,
						     void *mac_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_hal_dev_ctx *hal_dev_ctx = NULL;

	hal_dev_ctx = nrf_wifi_osal_mem_zalloc(sizeof(*hal_dev_ctx));

	if (!hal_dev_ctx) {
		nrf_wifi_osal_log_err("%s: Unable to allocate hal_dev_ctx",
				      __func__);
		goto err;
	}

	hal_dev_ctx->hpriv = hpriv;
	hal_dev_ctx->mac_dev_ctx = mac_dev_ctx;
	hal_dev_ctx->idx = hpriv->num_devs++;

	hal_dev_ctx->num_cmds = RPU_CMD_START_MAGIC;

	hal_dev_ctx->cmd_q = nrf_wifi_utils_ctrl_q_alloc();

	if (!hal_dev_ctx->cmd_q) {
		nrf_wifi_osal_log_err("%s: Unable to allocate command queue",
				      __func__);
		goto hal_dev_free;
	}

	hal_dev_ctx->event_q = nrf_wifi_utils_ctrl_q_alloc();

	if (!hal_dev_ctx->event_q) {
		nrf_wifi_osal_log_err("%s: Unable to allocate event queue",
				      __func__);
		goto cmd_q_free;
	}

	hal_dev_ctx->lock_hal = nrf_wifi_osal_spinlock_alloc();

	if (!hal_dev_ctx->lock_hal) {
		nrf_wifi_osal_log_err("%s: Unable to allocate HAL lock", __func__);
		hal_dev_ctx = NULL;
		goto event_q_free;
	}

	nrf_wifi_osal_spinlock_init(hal_dev_ctx->lock_hal);

	hal_dev_ctx->lock_rx = nrf_wifi_osal_spinlock_alloc();

	if (!hal_dev_ctx->lock_rx) {
		nrf_wifi_osal_log_err("%s: Unable to allocate HAL lock",
				      __func__);
		goto lock_hal_free;
	}

	nrf_wifi_osal_spinlock_init(hal_dev_ctx->lock_rx);

	hal_dev_ctx->event_tasklet = nrf_wifi_osal_tasklet_alloc(NRF_WIFI_TASKLET_TYPE_BH);

	if (!hal_dev_ctx->event_tasklet) {
		nrf_wifi_osal_log_err("%s: Unable to allocate event_tasklet",
				      __func__);
		goto lock_rx_free;
	}

	nrf_wifi_osal_tasklet_init(hal_dev_ctx->event_tasklet,
				   event_tasklet_fn,
				   (unsigned long)hal_dev_ctx);

#ifdef NRF_WIFI_LOW_POWER
	status = hal_rpu_ps_init(hal_dev_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: hal_rpu_ps_init failed",
				      __func__);
		goto event_tasklet_free;
	}
#endif /* NRF_WIFI_LOW_POWER */

	hal_dev_ctx->bal_dev_ctx = nrf_wifi_bal_dev_add(hpriv->bpriv,
							hal_dev_ctx);

	if (!hal_dev_ctx->bal_dev_ctx) {
		nrf_wifi_osal_log_err("%s: nrf_wifi_bal_dev_add failed",
				      __func__);
		goto event_tasklet_free;
	}

	status = hal_rpu_irq_enable(hal_dev_ctx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err("%s: hal_rpu_irq_enable failed",
				      __func__);
		goto bal_dev_free;
	}

	return hal_dev_ctx;
bal_dev_free:
	nrf_wifi_bal_dev_rem(hal_dev_ctx->bal_dev_ctx);
event_tasklet_free:
	nrf_wifi_osal_tasklet_free(hal_dev_ctx->event_tasklet);
lock_rx_free:
	nrf_wifi_osal_spinlock_free(hal_dev_ctx->lock_rx);
lock_hal_free:
	nrf_wifi_osal_spinlock_free(hal_dev_ctx->lock_hal);
event_q_free:
	nrf_wifi_utils_ctrl_q_free(hal_dev_ctx->event_q);
cmd_q_free:
	nrf_wifi_utils_ctrl_q_free(hal_dev_ctx->cmd_q);
hal_dev_free:
	nrf_wifi_osal_mem_free(hal_dev_ctx);
	hal_dev_ctx = NULL;
err:
	return NULL;
}
