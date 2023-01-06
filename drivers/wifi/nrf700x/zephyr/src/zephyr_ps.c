/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing power save specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <math.h>

#include "fmac_api.h"
#include "zephyr_fmac_main.h"
#include "zephyr_ps.h"

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_LOG_LEVEL);

int wifi_nrf_set_power_save(const struct device *dev,
			    struct wifi_ps_params *params)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	int ret = -1;

	if (!dev || !params) {
		goto out;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		goto out;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL\n", __func__);
		goto out;
	}

	status = wifi_nrf_fmac_set_power_save(rpu_ctx_zep->rpu_ctx,
					      vif_ctx_zep->vif_idx,
					      params->enabled);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_ failed\n", __func__);
		goto out;
	}

	ret = 0;
out:
	return ret;
}


int wifi_nrf_set_power_save_mode(const struct device *dev,
				 struct wifi_ps_mode_params *ps_mode_params)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	int ret = -1;

	if (!dev || !ps_mode_params) {
		goto out;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		goto out;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL\n", __func__);
		goto out;
	}

	if (ps_mode_params->mode == WIFI_PS_MODE_WMM) {
		/*WMM mode */
		status = wifi_nrf_fmac_set_uapsd_queue(rpu_ctx_zep->rpu_ctx,
						       vif_ctx_zep->vif_idx,
						       UAPSD_Q_MAX);
	} else {
		/* Legacy mode*/
		status = wifi_nrf_fmac_set_uapsd_queue(rpu_ctx_zep->rpu_ctx,
						       vif_ctx_zep->vif_idx,
						       UAPSD_Q_MIN);
	}

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_ failed\n", __func__);
		goto out;
	}

	ret = 0;
out:
	return ret;
}
