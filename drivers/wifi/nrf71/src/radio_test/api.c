/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing API functions for the Wi-Fi driver for radio test mode.
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <radio_test/api.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

enum nrf_wifi_status nrf_wifi_rt_fmac_dev_rem(struct nrf_wifi_rt_drv_priv *drv_priv)
{
	struct nrf_wifi_rt_drv_ctx *drv_ctx = NULL;

	drv_ctx = &drv_priv->drv_ctx;

	nrf_wifi_rt_fmac_dev_deinit(drv_ctx->rpu_ctx);

	nrf_wifi_fmac_dev_rem(drv_ctx->rpu_ctx);

	for (int i = 0; i < NUM_RF_PARAM_ADDRS; i++) {
		nrf_wifi_osal_mem_free((void *)drv_ctx->phy_rf_params_addr[i]);
		drv_ctx->phy_rf_params_addr[i] = 0;
	}

	/* vtf_buffer_start_address points at the static vtf_snapshots region,
	 * not heap memory, so it must not be freed.
	 */
	drv_ctx->vtf_buffer_start_address = 0;

	drv_ctx->rpu_ctx = NULL;
	LOG_DBG("%s: Device removed", __func__);

	return NRF_WIFI_STATUS_SUCCESS;
}
