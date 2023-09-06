/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing WiFi management operation implementations
 * for the Zephyr OS.
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "util.h"
#include "fmac_api.h"
#include "fmac_tx.h"
#include "fmac_util.h"
#include "zephyr_fmac_main.h"
#include "zephyr_wifi_mgmt.h"

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_LOG_LEVEL);

extern struct wifi_nrf_drv_priv_zep rpu_drv_priv_zep;

int wifi_nrf_set_power_save(const struct device *dev,
			    struct wifi_ps_params *params)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	int ret = -1;
	unsigned int uapsd_queue = UAPSD_Q_MIN; /* Legacy mode */

	if (!dev || !params) {
		LOG_ERR("%s: dev or params is NULL\n", __func__);
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

	switch (params->type) {
	case WIFI_PS_PARAM_LISTEN_INTERVAL:
		if ((params->listen_interval <
		     NRF_WIFI_LISTEN_INTERVAL_MIN) ||
		    (params->listen_interval >
		     WIFI_LISTEN_INTERVAL_MAX)) {
			params->fail_reason =
				WIFI_PS_PARAM_LISTEN_INTERVAL_RANGE_INVALID;
			return -EINVAL;
		}
		status = wifi_nrf_fmac_set_listen_interval(
						rpu_ctx_zep->rpu_ctx,
						vif_ctx_zep->vif_idx,
						params->listen_interval);
	break;
	case  WIFI_PS_PARAM_TIMEOUT:
		if (vif_ctx_zep->if_type != NRF_WIFI_IFTYPE_STATION) {
			LOG_ERR("%s: Operation supported only in STA mode\n",
				__func__);
			params->fail_reason =
				WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
			goto out;
		}

		status = wifi_nrf_fmac_set_power_save_timeout(
							rpu_ctx_zep->rpu_ctx,
							vif_ctx_zep->vif_idx,
							params->timeout_ms);
	break;
	case WIFI_PS_PARAM_MODE:
		if (params->mode == WIFI_PS_MODE_WMM) {
			uapsd_queue = UAPSD_Q_MAX; /* WMM mode */
		}

		status = wifi_nrf_fmac_set_uapsd_queue(rpu_ctx_zep->rpu_ctx,
						       vif_ctx_zep->vif_idx,
						       uapsd_queue);
	break;
	case  WIFI_PS_PARAM_STATE:
		status = wifi_nrf_fmac_set_power_save(rpu_ctx_zep->rpu_ctx,
						      vif_ctx_zep->vif_idx,
						      params->enabled);
	break;
	case WIFI_PS_PARAM_WAKEUP_MODE:
		status = wifi_nrf_fmac_set_ps_wakeup_mode(
							rpu_ctx_zep->rpu_ctx,
							vif_ctx_zep->vif_idx,
							params->wakeup_mode);
	break;
	default:
		params->fail_reason =
			WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
		return -ENOTSUP;
	}

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: Confiuring PS param %d failed\n",
			__func__, params->type);
		params->fail_reason =
			WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
		goto out;
	}


	ret = 0;
out:
	return ret;
}

int wifi_nrf_get_power_save_config(const struct device *dev,
				   struct wifi_ps_config *ps_config)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	int ret = -1;
	int count = 0;

	if (!dev || !ps_config) {
		goto out;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		goto out;
	}

	if (vif_ctx_zep->if_type != NRF_WIFI_IFTYPE_STATION) {
		LOG_ERR("%s: Operation supported only in STA mode\n",
			__func__);
		goto out;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL\n", __func__);
		goto out;
	}

	vif_ctx_zep->ps_info = ps_config;

	vif_ctx_zep->ps_config_info_evnt = false;

	status = wifi_nrf_fmac_get_power_save_info(rpu_ctx_zep->rpu_ctx,
						   vif_ctx_zep->vif_idx);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_get_power_save_info failed\n",
			__func__);
		goto out;
	}

	do {
		wifi_nrf_osal_sleep_ms(fmac_dev_ctx->fpriv->opriv,
					1);
		 count++;
	} while ((vif_ctx_zep->ps_config_info_evnt == false) &&
		 (count < WIFI_NRF_FMAC_PS_CONF_EVNT_RECV_TIMEOUT));

	if (count == WIFI_NRF_FMAC_PS_CONF_EVNT_RECV_TIMEOUT) {
		wifi_nrf_osal_log_err(fmac_dev_ctx->fpriv->opriv,
				      "%s: Timed out\n",
				      __func__);
		goto out;
	}

	ret = 0;
out:
	return ret;
}

/* TWT interval conversion helpers: User <-> Protocol */
static struct twt_interval_float wifi_nrf_twt_us_to_float(uint32_t twt_interval)
{
	double mantissa = 0.0;
	int exponent = 0;
	struct twt_interval_float twt_interval_float;

	double twt_interval_ms = twt_interval / 1000.0;

	mantissa = frexp(twt_interval_ms, &exponent);
	/* Ceiling and conversion to milli seconds */
	twt_interval_float.mantissa = ceil(mantissa * 1000);
	twt_interval_float.exponent = exponent;

	return twt_interval_float;
}

static uint64_t wifi_nrf_twt_float_to_us(struct twt_interval_float twt_interval_float)
{
	/* Conversion to micro-seconds */
	return floor(ldexp(twt_interval_float.mantissa, twt_interval_float.exponent) / (1000)) *
			     1000;
}

static unsigned char twt_wifi_mgmt_to_rpu_neg_type(enum wifi_twt_negotiation_type neg_type)
{
	unsigned char rpu_neg_type = 0;

	switch (neg_type) {
	case WIFI_TWT_INDIVIDUAL:
		rpu_neg_type = NRF_WIFI_TWT_NEGOTIATION_TYPE_INDIVIDUAL;
		break;
	case WIFI_TWT_BROADCAST:
		rpu_neg_type = NRF_WIFI_TWT_NEGOTIATION_TYPE_BROADCAST;
		break;
	default:
		LOG_ERR("%s: Invalid negotiation type: %d\n",
			__func__, neg_type);
		break;
	}

	return rpu_neg_type;
}

static enum wifi_twt_negotiation_type twt_rpu_to_wifi_mgmt_neg_type(unsigned char neg_type)
{
	enum wifi_twt_negotiation_type wifi_neg_type = WIFI_TWT_INDIVIDUAL;

	switch (neg_type) {
	case NRF_WIFI_TWT_NEGOTIATION_TYPE_INDIVIDUAL:
		wifi_neg_type = WIFI_TWT_INDIVIDUAL;
		break;
	case NRF_WIFI_TWT_NEGOTIATION_TYPE_BROADCAST:
		wifi_neg_type = WIFI_TWT_BROADCAST;
		break;
	default:
		LOG_ERR("%s: Invalid negotiation type: %d\n",
			__func__, neg_type);
		break;
	}

	return wifi_neg_type;
}

/* Though setup_cmd enums have 1-1 mapping but due to data type different need these */
static enum wifi_twt_setup_cmd twt_rpu_to_wifi_mgmt_setup_cmd(signed int setup_cmd)
{
	enum wifi_twt_setup_cmd wifi_setup_cmd = WIFI_TWT_SETUP_CMD_REQUEST;

	switch (setup_cmd) {
	case NRF_WIFI_REQUEST_TWT:
		wifi_setup_cmd = WIFI_TWT_SETUP_CMD_REQUEST;
		break;
	case NRF_WIFI_SUGGEST_TWT:
		wifi_setup_cmd = WIFI_TWT_SETUP_CMD_SUGGEST;
		break;
	case NRF_WIFI_DEMAND_TWT:
		wifi_setup_cmd = WIFI_TWT_SETUP_CMD_DEMAND;
		break;
	case NRF_WIFI_GROUPING_TWT:
		wifi_setup_cmd = WIFI_TWT_SETUP_CMD_GROUPING;
		break;
	case NRF_WIFI_ACCEPT_TWT:
		wifi_setup_cmd = WIFI_TWT_SETUP_CMD_ACCEPT;
		break;
	case NRF_WIFI_ALTERNATE_TWT:
		wifi_setup_cmd = WIFI_TWT_SETUP_CMD_ALTERNATE;
		break;
	case NRF_WIFI_DICTATE_TWT:
		wifi_setup_cmd = WIFI_TWT_SETUP_CMD_DICTATE;
		break;
	case NRF_WIFI_REJECT_TWT:
		wifi_setup_cmd = WIFI_TWT_SETUP_CMD_REJECT;
		break;
	default:
		LOG_ERR("%s: Invalid setup command: %d\n",
			__func__, setup_cmd);
		break;
	}

	return wifi_setup_cmd;
}

static signed int twt_wifi_mgmt_to_rpu_setup_cmd(enum wifi_twt_setup_cmd setup_cmd)
{
	signed int rpu_setup_cmd = NRF_WIFI_REQUEST_TWT;

	switch (setup_cmd) {
	case WIFI_TWT_SETUP_CMD_REQUEST:
		rpu_setup_cmd = NRF_WIFI_REQUEST_TWT;
		break;
	case WIFI_TWT_SETUP_CMD_SUGGEST:
		rpu_setup_cmd = NRF_WIFI_SUGGEST_TWT;
		break;
	case WIFI_TWT_SETUP_CMD_DEMAND:
		rpu_setup_cmd = NRF_WIFI_DEMAND_TWT;
		break;
	case WIFI_TWT_SETUP_CMD_GROUPING:
		rpu_setup_cmd = NRF_WIFI_GROUPING_TWT;
		break;
	case WIFI_TWT_SETUP_CMD_ACCEPT:
		rpu_setup_cmd = NRF_WIFI_ACCEPT_TWT;
		break;
	case WIFI_TWT_SETUP_CMD_ALTERNATE:
		rpu_setup_cmd = NRF_WIFI_ALTERNATE_TWT;
		break;
	case WIFI_TWT_SETUP_CMD_DICTATE:
		rpu_setup_cmd = NRF_WIFI_DICTATE_TWT;
		break;
	case WIFI_TWT_SETUP_CMD_REJECT:
		rpu_setup_cmd = NRF_WIFI_REJECT_TWT;
		break;
	default:
		LOG_ERR("%s: Invalid setup command: %d\n",
			__func__, setup_cmd);
		break;
	}

	return rpu_setup_cmd;
}

void wifi_nrf_event_proc_get_power_save_info(void *vif_ctx,
					     struct nrf_wifi_umac_event_power_save_info *ps_info,
					     unsigned int event_len)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;

	if (!vif_ctx || !ps_info) {
		return;
	}

	vif_ctx_zep = vif_ctx;

	vif_ctx_zep->ps_info->ps_params.mode = ps_info->ps_mode;
	vif_ctx_zep->ps_info->ps_params.enabled = ps_info->enabled;
	vif_ctx_zep->ps_info->num_twt_flows = ps_info->num_twt_flows;
	vif_ctx_zep->ps_info->ps_params.timeout_ms = ps_info->ps_timeout;
	vif_ctx_zep->ps_info->ps_params.listen_interval = ps_info->listen_interval;
	vif_ctx_zep->ps_info->ps_params.wakeup_mode = ps_info->extended_ps;

	for (int i = 0; i < ps_info->num_twt_flows; i++) {
		struct twt_interval_float twt_interval_float;
		struct wifi_twt_flow_info *twt_zep = &vif_ctx_zep->ps_info->twt_flows[i];
		struct nrf_wifi_umac_config_twt_info *twt_rpu = &ps_info->twt_flow_info[i];

		memset(twt_zep, 0, sizeof(struct wifi_twt_flow_info));

		twt_zep->flow_id = twt_rpu->twt_flow_id;
		twt_zep->implicit = twt_rpu->is_implicit ? 1 : 0;
		twt_zep->trigger = twt_rpu->ap_trigger_frame ? 1 : 0;
		twt_zep->announce = twt_rpu->twt_flow_type == NRF_WIFI_TWT_FLOW_TYPE_ANNOUNCED;
		twt_zep->negotiation_type = twt_rpu_to_wifi_mgmt_neg_type(twt_rpu->neg_type);
		twt_zep->dialog_token = twt_rpu->dialog_token;
		twt_interval_float.mantissa = twt_rpu->twt_target_wake_interval_mantissa;
		twt_interval_float.exponent = twt_rpu->twt_target_wake_interval_exponent;
		twt_zep->twt_interval = wifi_nrf_twt_float_to_us(twt_interval_float);
		twt_zep->twt_wake_interval = twt_rpu->nominal_min_twt_wake_duration;
	}

	vif_ctx_zep->ps_config_info_evnt = true;
}

static void wifi_nrf_twt_update_internal_state(struct wifi_nrf_vif_ctx_zep *vif_ctx_zep,
	bool setup, unsigned char flow_id)
{
	if (setup) {
		vif_ctx_zep->twt_flows_map |= BIT(flow_id);
		vif_ctx_zep->twt_flow_in_progress_map &= ~BIT(flow_id);
	} else {
		vif_ctx_zep->twt_flows_map &= ~BIT(flow_id);
	}
}

int wifi_nrf_twt_teardown_flows(struct wifi_nrf_vif_ctx_zep *vif_ctx_zep,
		unsigned char start_flow_id, unsigned char end_flow_id)
{
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_umac_config_twt_info twt_info = {0};
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	int ret = 0;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		ret = -1;
		goto out;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL\n", __func__);
		ret = -1;
		goto out;
	}

	for (int flow_id = start_flow_id; flow_id < end_flow_id; flow_id++) {
		if (!(vif_ctx_zep->twt_flows_map & BIT(flow_id))) {
			continue;
		}
		twt_info.twt_flow_id = flow_id;
		status = wifi_nrf_fmac_twt_teardown(rpu_ctx_zep->rpu_ctx,
						vif_ctx_zep->vif_idx,
						&twt_info);
		if (status != WIFI_NRF_STATUS_SUCCESS) {
			LOG_ERR("%s: TWT teardown for flow id %d failed\n",
				__func__, flow_id);
			ret = -1;
			continue;
		}
		/* UMAC doesn't send TWT teardown event for host initiated teardown */
		wifi_nrf_twt_update_internal_state(vif_ctx_zep, false, flow_id);
	}

out:
	return ret;
}

int wifi_nrf_set_twt(const struct device *dev,
		     struct wifi_twt_params *twt_params)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_umac_config_twt_info twt_info = {0};
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

	if (!(twt_params->operation == WIFI_TWT_TEARDOWN && twt_params->teardown.teardown_all) &&
		twt_params->flow_id >= WIFI_MAX_TWT_FLOWS) {
		LOG_ERR("%s: Invalid flow id: %d\n",
			__func__, twt_params->flow_id);
		twt_params->fail_reason = WIFI_TWT_FAIL_INVALID_FLOW_ID;
		goto out;
	}

	switch (twt_params->operation) {
	case WIFI_TWT_SETUP:
		if (vif_ctx_zep->twt_flow_in_progress_map & BIT(twt_params->flow_id)) {
			twt_params->fail_reason = WIFI_TWT_FAIL_OPERATION_IN_PROGRESS;
			goto out;
		}

		if (twt_params->setup_cmd == WIFI_TWT_SETUP_CMD_REQUEST) {
			if (vif_ctx_zep->twt_flows_map & BIT(twt_params->flow_id)) {
				twt_params->fail_reason = WIFI_TWT_FAIL_FLOW_ALREADY_EXISTS;
				goto out;
			}
		}

		struct twt_interval_float twt_interval_float =
			wifi_nrf_twt_us_to_float(twt_params->setup.twt_interval);

		twt_info.twt_flow_id = twt_params->flow_id;
		twt_info.neg_type = twt_wifi_mgmt_to_rpu_neg_type(twt_params->negotiation_type);
		twt_info.setup_cmd = twt_wifi_mgmt_to_rpu_setup_cmd(twt_params->setup_cmd);
		twt_info.ap_trigger_frame = twt_params->setup.trigger;
		twt_info.is_implicit = twt_params->setup.implicit;
		if (twt_params->setup.announce) {
			twt_info.twt_flow_type = NRF_WIFI_TWT_FLOW_TYPE_ANNOUNCED;
		} else {
			twt_info.twt_flow_type = NRF_WIFI_TWT_FLOW_TYPE_UNANNOUNCED;
		}

		twt_info.nominal_min_twt_wake_duration =
				twt_params->setup.twt_wake_interval;
		twt_info.twt_target_wake_interval_mantissa = twt_interval_float.mantissa;
		twt_info.twt_target_wake_interval_exponent = twt_interval_float.exponent;

		twt_info.dialog_token = twt_params->dialog_token;

		status = wifi_nrf_fmac_twt_setup(rpu_ctx_zep->rpu_ctx,
					   vif_ctx_zep->vif_idx,
					   &twt_info);

		break;
	case WIFI_TWT_TEARDOWN:
		unsigned char start_flow_id = 0;
		unsigned char end_flow_id = WIFI_MAX_TWT_FLOWS;

		if (!twt_params->teardown.teardown_all) {
			if (!(vif_ctx_zep->twt_flows_map & BIT(twt_params->flow_id))) {
				twt_params->fail_reason = WIFI_TWT_FAIL_INVALID_FLOW_ID;
				goto out;
			}
			start_flow_id = twt_params->flow_id;
			end_flow_id = twt_params->flow_id + 1;
			twt_info.twt_flow_id = twt_params->flow_id;
		}

		status = wifi_nrf_twt_teardown_flows(vif_ctx_zep,
						     start_flow_id,
						     end_flow_id);
		if (status != WIFI_NRF_STATUS_SUCCESS) {
			LOG_ERR("%s: TWT teardown failed: start_flow_id: %d, end_flow_id: %d\n",
				__func__, start_flow_id, end_flow_id);
			goto out;
		}
		break;

	default:
		LOG_ERR("Unknown TWT operation\n");
		status = WIFI_NRF_STATUS_FAIL;
		break;
	}

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_set_twt failed\n", __func__);
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
	struct twt_interval_float twt_interval_float;

	if (!vif_ctx || !twt_setup_info) {
		return;
	}

	vif_ctx_zep = vif_ctx;

	twt_params.operation = WIFI_TWT_SETUP;
	twt_params.flow_id = twt_setup_info->info.twt_flow_id;
	twt_params.negotiation_type = twt_rpu_to_wifi_mgmt_neg_type(twt_setup_info->info.neg_type);
	twt_params.setup_cmd = twt_rpu_to_wifi_mgmt_setup_cmd(twt_setup_info->info.setup_cmd);
	twt_params.setup.trigger = twt_setup_info->info.ap_trigger_frame ? 1 : 0;
	twt_params.setup.implicit = twt_setup_info->info.is_implicit ? 1 : 0;
	twt_params.setup.announce =
		twt_setup_info->info.twt_flow_type == NRF_WIFI_TWT_FLOW_TYPE_ANNOUNCED;
	twt_params.setup.twt_wake_interval =
			twt_setup_info->info.nominal_min_twt_wake_duration;
	twt_interval_float.mantissa = twt_setup_info->info.twt_target_wake_interval_mantissa;
	twt_interval_float.exponent = twt_setup_info->info.twt_target_wake_interval_exponent;
	twt_params.setup.twt_interval = wifi_nrf_twt_float_to_us(twt_interval_float);
	twt_params.dialog_token = twt_setup_info->info.dialog_token;
	twt_params.resp_status = twt_setup_info->info.twt_resp_status;

	if ((twt_setup_info->info.twt_resp_status == 0) ||
	    (twt_setup_info->info.neg_type == NRF_WIFI_ACCEPT_TWT)) {
		wifi_nrf_twt_update_internal_state(vif_ctx_zep, true, twt_params.flow_id);
	}

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
	/* TODO: ADD reason code in the twt_params structure */
	wifi_nrf_twt_update_internal_state(vif_ctx_zep, false, twt_params.flow_id);

	wifi_mgmt_raise_twt_event(vif_ctx_zep->zep_net_if_ctx, &twt_params);
}

void wifi_nrf_event_proc_twt_sleep_zep(void *vif_ctx,
					struct nrf_wifi_umac_event_twt_sleep *sleep_evnt,
					unsigned int event_len)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct wifi_nrf_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct wifi_nrf_fmac_priv_def *def_priv = NULL;
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
	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	def_priv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	if (!sleep_evnt) {
		LOG_ERR("%s: sleep_evnt is NULL\n", __func__);
		return;
	}

	switch (sleep_evnt->info.type) {
	case TWT_BLOCK_TX:
		wifi_nrf_osal_spinlock_take(fmac_dev_ctx->fpriv->opriv,
					    def_dev_ctx->tx_config.tx_lock);

		def_dev_ctx->twt_sleep_status = WIFI_NRF_FMAC_TWT_STATE_SLEEP;

		wifi_mgmt_raise_twt_sleep_state(vif_ctx_zep->zep_net_if_ctx,
						WIFI_TWT_STATE_SLEEP);
		wifi_nrf_osal_spinlock_rel(fmac_dev_ctx->fpriv->opriv,
					    def_dev_ctx->tx_config.tx_lock);
	break;
	case TWT_UNBLOCK_TX:
		wifi_nrf_osal_spinlock_take(fmac_dev_ctx->fpriv->opriv,
					    def_dev_ctx->tx_config.tx_lock);
		def_dev_ctx->twt_sleep_status = WIFI_NRF_FMAC_TWT_STATE_AWAKE;
		wifi_mgmt_raise_twt_sleep_state(vif_ctx_zep->zep_net_if_ctx,
						WIFI_TWT_STATE_AWAKE);
#ifdef CONFIG_NRF700X_DATA_TX
		for (ac = WIFI_NRF_FMAC_AC_BE;
		     ac <= WIFI_NRF_FMAC_AC_MAX; ++ac) {
			desc = tx_desc_get(fmac_dev_ctx, ac);
			if (desc < def_priv->num_tx_tokens) {
				tx_pending_process(fmac_dev_ctx, desc, ac);
			}
		}
#endif
		wifi_nrf_osal_spinlock_rel(fmac_dev_ctx->fpriv->opriv,
				def_dev_ctx->tx_config.tx_lock);
	break;
	default:
	break;
	}
}
