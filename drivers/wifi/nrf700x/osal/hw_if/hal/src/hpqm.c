/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing HPQM interface specific definitions for the
 * HAL Layer of the Wi-Fi driver.
 */

#include "hal_reg.h"
#include "hal_mem.h"
#include "hal_common.h"

enum nrf_wifi_status hal_rpu_hpq_enqueue(struct nrf_wifi_hal_dev_ctx *hal_ctx,
					 struct host_rpu_hpq *hpq,
					 unsigned int val)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	status = hal_rpu_reg_write(hal_ctx,
				   hpq->enqueue_addr,
				   val);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_ctx->hpriv->opriv,
				      "%s: Writing to enqueue address failed\n",
				      __func__);
		goto out;
	}

out:
	return status;
}


enum nrf_wifi_status hal_rpu_hpq_dequeue(struct nrf_wifi_hal_dev_ctx *hal_ctx,
					 struct host_rpu_hpq *hpq,
					 unsigned int *val)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	status = hal_rpu_reg_read(hal_ctx,
				  val,
				  hpq->dequeue_addr);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		nrf_wifi_osal_log_err(hal_ctx->hpriv->opriv,
				      "%s: Dequeue failed, val (0x%X)\n",
				      __func__,
				      *val);
		goto out;
	}

	/* Pop the element only if it is valid */
	if (*val) {
		status = hal_rpu_reg_write(hal_ctx,
					   hpq->dequeue_addr,
					   *val);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			nrf_wifi_osal_log_err(hal_ctx->hpriv->opriv,
					      "%s: Writing to dequeue address failed, val (0x%X)\n",
					      __func__,
					      *val);
			goto out;
		}
	}
out:
	return status;
}
