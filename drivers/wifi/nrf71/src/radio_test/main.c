/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing initialization and deinitialization of the Wi-Fi driver for nRF71.
 */

#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>

#include <util.h>
#include <common/rf_params.h>
#include <radio_test/core.h>

#define DT_DRV_COMPAT nordic_wlan
LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

struct nrf_wifi_rt_drv_priv rt_drv_priv;
extern const struct nrf_wifi_osal_ops nrf_wifi_os_zep_ops;


static enum nrf_wifi_status nrf_wifi_rt_drv_dev_add(struct nrf_wifi_rt_drv_priv *drv_priv)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_rt_drv_ctx *drv_ctx = NULL;
	void *rpu_ctx = NULL;
	unsigned char op_band = nrf_wifi_utils_get_op_band();
	struct nrf_wifi_tx_pwr_ctrl_params tx_pwr_ctrl_params;
	struct nrf_wifi_tx_pwr_ceil_params tx_pwr_ceil_params;
	unsigned int fw_ver = 0;

	drv_ctx = &drv_priv->drv_ctx;

	drv_ctx->drv_priv = drv_priv;

	rpu_ctx = nrf_wifi_rt_fmac_dev_add(drv_priv->fmac_priv, drv_ctx);

	if (!rpu_ctx) {
		LOG_ERR("%s: Device addition failed", __func__);
		drv_ctx = NULL;
		goto err;
	}

	drv_ctx->rpu_ctx = rpu_ctx;

	status = nrf_wifi_fmac_ver_get(rpu_ctx,
				       &fw_ver);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: FW version read failed", __func__);
		goto err;
	}

	LOG_DBG("Firmware (v%d.%d.%d.%d) booted successfully",
		NRF_WIFI_UMAC_VER(fw_ver),
		NRF_WIFI_UMAC_VER_MAJ(fw_ver),
		NRF_WIFI_UMAC_VER_MIN(fw_ver),
		NRF_WIFI_UMAC_VER_EXTRA(fw_ver));

	status = nrf_wifi_fmac_config_rf_params(rpu_ctx,
						drv_ctx->phy_rf_params_addr);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Failed to configure RF params", __func__);
		goto err;
	}

	/* TODO: Remove hardcodes once we hook in sensor readings */
	status = nrf_wifi_fmac_config_vtf_params(rpu_ctx, 243, 25, 0,
						 &drv_ctx->vtf_buffer_start_address);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Failed to configure VTF params", __func__);
		goto err;
	}

	memset(&tx_pwr_ctrl_params, 0, sizeof(tx_pwr_ctrl_params));
	memset(&tx_pwr_ceil_params, 0, sizeof(tx_pwr_ceil_params));

	configure_tx_pwr_settings(&tx_pwr_ctrl_params, &tx_pwr_ceil_params);

	status = nrf_wifi_rt_fmac_dev_init(rpu_ctx,
#ifdef CONFIG_NRF_WIFI_LOW_POWER
					   SLEEP_DISABLE,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
					   NRF_WIFI_DEF_PHY_CALIB,
					   op_band,
					   IS_ENABLED(CONFIG_NRF_WIFI_BEAMFORMING),
					   &tx_pwr_ctrl_params,
					   &tx_pwr_ceil_params,
					   STRINGIFY(CONFIG_NRF71_REG_DOMAIN),
					   drv_ctx->phy_rf_params_addr,
					   drv_ctx->vtf_buffer_start_address);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_sys_fmac_dev_init failed", __func__);
		goto err;
	}

	return status;
err:
	if (rpu_ctx) {
		nrf_wifi_fmac_dev_rem(rpu_ctx);
		drv_ctx->rpu_ctx = NULL;
	}
	return status;
}

static int nrf_wifi_rt_drv_main(const struct device *dev)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	/* The OSAL layer needs to be initialized before any other initialization
	 * so that other layers (like FW IF,HW IF etc) have access to OS ops
	 */
	nrf_wifi_osal_init(&nrf_wifi_os_zep_ops);

	rt_drv_priv.fmac_priv = nrf_wifi_rt_fmac_init();

	if (rt_drv_priv.fmac_priv == NULL) {
		LOG_ERR("%s: FMAC initialization failed",
			__func__);
		goto err;
	}

	status = nrf_wifi_rt_drv_dev_add(&rt_drv_priv);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Device addition failed", __func__);
		goto fmac_deinit;
	}

	k_mutex_init(&rt_drv_priv.drv_ctx.rpu_lock);

	return 0;

fmac_deinit:
	nrf_wifi_fmac_deinit(rt_drv_priv.fmac_priv);
	nrf_wifi_osal_deinit();
err:
	return -1;
}


DEVICE_DT_INST_DEFINE(0,
	      nrf_wifi_rt_drv_main, /* init_fn */
	      NULL, /* pm_action_cb */
	      NULL,
	      NULL, /* cfg */
	      POST_KERNEL,
	      CONFIG_WIFI_INIT_PRIORITY, /* prio */
	      NULL); /* api */
