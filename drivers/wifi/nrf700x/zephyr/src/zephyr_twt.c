/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing twt specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <math.h>

#include "fmac_api.h"
#include "zephyr_fmac_main.h"
#include "zephyr_twt.h"

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_LOG_LEVEL);

int wifi_nrf_set_twt(const struct device *dev,
		     struct wifi_twt_params *twt_params)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_umac_config_twt_info twt_info = {0};
	double mantissa = 0;
	int exponent;
	int ret = -1;

	if (!dev || !twt_params) {
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

	switch (twt_params->operation) {
	case WIFI_TWT_SETUP:
		twt_info.twt_flow_id = twt_params->flow_id;
		twt_info.neg_type = twt_params->negotiation_type;
		twt_info.setup_cmd = twt_params->setup_cmd;
		twt_info.ap_trigger_frame = twt_params->setup.trigger;
		twt_info.is_implicit = twt_params->setup.implicit;
		twt_info.twt_flow_type = twt_params->setup.announce;
		/* interval values are passed as us to UMAC */
		twt_info.nominal_min_twt_wake_duration =
			((twt_params->setup.twt_wake_interval_ms * 1000) / 256);

		mantissa = frexp(twt_params->setup.twt_interval_ms, &exponent);
		/* Ceiling and conversion to milli seconds */
		twt_info.twt_target_wake_interval_mantissa = ceil(mantissa * 1000);
		twt_info.twt_target_wake_interval_exponent = exponent;

		status = wifi_nrf_fmac_twt_setup(rpu_ctx_zep->rpu_ctx,
					   vif_ctx_zep->vif_idx,
					   &twt_info);
		break;
	case WIFI_TWT_TEARDOWN:
		twt_info.twt_flow_id = twt_params->flow_id;

		status = wifi_nrf_fmac_twt_teardown(rpu_ctx_zep->rpu_ctx,
						    vif_ctx_zep->vif_idx,
						    &twt_info);
		break;

	default:
		LOG_ERR("Unknown TWT operation\n");
		status = WIFI_NRF_STATUS_FAIL;
		break;
	}

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_scan failed\n", __func__);
		goto out;
	}

	ret = 0;
out:
	return ret;
}


void wifi_nrf_event_proc_twt_setup_zep(void *vif_ctx,
				       struct nrf_wifi_umac_cmd_config_twt *twt_setup_info,
				       unsigned int event_len)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_twt_params twt_params;
	double target_wake_time_ms = 0;
	double mantissa = 0;
	int exponent = 0;

	if (!vif_ctx || !twt_setup_info) {
		return;
	}

	vif_ctx_zep = vif_ctx;

	twt_params.operation = WIFI_TWT_SETUP;
	twt_params.flow_id = twt_setup_info->info.twt_flow_id;
	twt_params.negotiation_type = twt_setup_info->info.neg_type;
	twt_params.setup_cmd = twt_setup_info->info.setup_cmd;
	twt_params.setup.trigger = twt_setup_info->info.ap_trigger_frame;
	twt_params.setup.implicit = twt_setup_info->info.is_implicit;
	twt_params.setup.announce = twt_setup_info->info.twt_flow_type;
	/* interval values are passed as us to UMAC */
	twt_params.setup.twt_wake_interval_ms =
			((twt_setup_info->info.nominal_min_twt_wake_duration / 1000)  * 256);

	exponent = twt_setup_info->info.twt_target_wake_interval_exponent;
	mantissa = (twt_setup_info->info.twt_target_wake_interval_mantissa / 1000);
	target_wake_time_ms = ldexp(mantissa, exponent);
	twt_params.setup.twt_interval_ms = (target_wake_time_ms);

	wifi_mgmt_raise_twt_event(vif_ctx_zep->zep_net_if_ctx, &twt_params);
}


void wifi_nrf_event_proc_twt_teardown_zep(void *vif_ctx,
					  struct nrf_wifi_umac_cmd_teardown_twt *twt_teardown_info,
					  unsigned int event_len)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_twt_params twt_params = {0};

	if (!vif_ctx || !twt_teardown_info) {
		return;
	}

	vif_ctx_zep = vif_ctx;

	twt_params.operation = WIFI_TWT_TEARDOWN;
	twt_params.flow_id = twt_teardown_info->info.twt_flow_id;
	//TODO: ADD reason code in the twt_params structure
	//twt_params.reason_code = twt_teardown_info->info.reason_code;

	wifi_mgmt_raise_twt_event(vif_ctx_zep->zep_net_if_ctx, &twt_params);
}
