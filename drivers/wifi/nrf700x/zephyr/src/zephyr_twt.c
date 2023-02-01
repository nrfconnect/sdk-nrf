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
#include "fmac_tx.h"
#include "zephyr_fmac_main.h"
#include "zephyr_twt.h"

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_LOG_LEVEL);

extern struct wifi_nrf_drv_priv_zep rpu_drv_priv_zep;

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
		/* Interval values are passed as ms to UMAC */
		twt_info.nominal_min_twt_wake_duration =
				twt_params->setup.twt_wake_interval_ms;

		mantissa = frexp(twt_params->setup.twt_interval_ms, &exponent);
		/* Ceiling and conversion to milli seconds */
		twt_info.twt_target_wake_interval_mantissa = ceil(mantissa * 1000);
		twt_info.twt_target_wake_interval_exponent = exponent;
		twt_info.dialog_token = twt_params->dialog_token;

		status = wifi_nrf_fmac_twt_setup(rpu_ctx_zep->rpu_ctx,
					   vif_ctx_zep->vif_idx,
					   &twt_info);
		break;
	case WIFI_TWT_TEARDOWN:
		twt_info.twt_flow_id = twt_params->flow_id;

		if (twt_params->teardown.teardown_all) {
			if (vif_ctx_zep->neg_twt_flow_id == 0xFF) {
				LOG_ERR("Invalid negotiated TWT flow id\n");
				goto out;
			}
			/* Update the negotiated flow id
			 * from the negotiated value.
			 */
			twt_info.twt_flow_id = vif_ctx_zep->neg_twt_flow_id;
		} else {
			/* Update the flow id as given by the user. */
			twt_info.twt_flow_id = twt_params->flow_id;
		}

		status = wifi_nrf_fmac_twt_teardown(rpu_ctx_zep->rpu_ctx,
						    vif_ctx_zep->vif_idx,
						    &twt_info);

		if (status == WIFI_NRF_STATUS_SUCCESS) {
			vif_ctx_zep->neg_twt_flow_id = 0XFF;
		}

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
	/* Store the negotiated flow id and pass it to user. */
	vif_ctx_zep->neg_twt_flow_id = twt_setup_info->info.twt_flow_id;
	twt_params.negotiation_type = twt_setup_info->info.neg_type;
	twt_params.setup_cmd = twt_setup_info->info.setup_cmd;
	twt_params.setup.trigger = twt_setup_info->info.ap_trigger_frame;
	twt_params.setup.implicit = twt_setup_info->info.is_implicit;
	twt_params.setup.announce = twt_setup_info->info.twt_flow_type;
	/* Interval values are passed as us to UMAC */
	twt_params.setup.twt_wake_interval_ms =
			twt_setup_info->info.nominal_min_twt_wake_duration;

	exponent = twt_setup_info->info.twt_target_wake_interval_exponent;
	mantissa = (twt_setup_info->info.twt_target_wake_interval_mantissa / 1000);
	target_wake_time_ms = ldexp(mantissa, exponent);
	twt_params.setup.twt_interval_ms = (target_wake_time_ms);
	twt_params.dialog_token = twt_setup_info->info.dialog_token;

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
	vif_ctx_zep->neg_twt_flow_id = 0XFF;

	wifi_mgmt_raise_twt_event(vif_ctx_zep->zep_net_if_ctx, &twt_params);
}

void wifi_nrf_event_proc_twt_sleep_zep(void *vif_ctx,
					struct nrf_wifi_umac_event_twt_sleep *sleep_evnt,
					unsigned int event_len)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
#ifdef CONFIG_NRF700X_DATA_TX
	int desc = 0;
	int ac = 0;
#endif
	vif_ctx_zep = vif_ctx;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		return;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL\n", __func__);
		return;
	}

	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;

	if (!sleep_evnt) {
		LOG_ERR("%s: sleep_evnt is NULL\n", __func__);
		return;
	}

	switch (sleep_evnt->info.type) {
	case TWT_BLOCK_TX:
		wifi_nrf_osal_spinlock_take(fmac_dev_ctx->fpriv->opriv,
					    fmac_dev_ctx->tx_config.tx_lock);

		fmac_priv->twt_sleep_status = true;

		wifi_nrf_osal_spinlock_rel(fmac_dev_ctx->fpriv->opriv,
					   fmac_dev_ctx->tx_config.tx_lock);
	break;
	case TWT_UNBLOCK_TX:
		wifi_nrf_osal_spinlock_take(fmac_dev_ctx->fpriv->opriv,
					    fmac_dev_ctx->tx_config.tx_lock);

#ifdef CONFIG_NRF700X_DATA_TX
		for (ac = WIFI_NRF_FMAC_AC_BE;
		     ac <= WIFI_NRF_FMAC_AC_MAX; ++ac) {
			desc = tx_desc_get(fmac_dev_ctx, ac);
			if (desc < fmac_dev_ctx->fpriv->num_tx_tokens) {
				tx_pending_process(fmac_dev_ctx, desc, ac);
			}
		}
#endif
		fmac_priv->twt_sleep_status = false;

		wifi_nrf_osal_spinlock_rel(fmac_dev_ctx->fpriv->opriv,
					   fmac_dev_ctx->tx_config.tx_lock);
	break;
	default:
	break;
	}
}
