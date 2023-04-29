/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief File containing display scan specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "fmac_api.h"
#include "fmac_tx.h"
#include "zephyr_fmac_main.h"
#include "zephyr_wifi_mgmt.h"

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_LOG_LEVEL);

extern struct wifi_nrf_drv_priv_zep rpu_drv_priv_zep;

void wifi_nrf_scan_timeout_work(struct k_work *work)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct wifi_scan_result res;
	scan_result_cb_t disp_scan_cb = NULL;

	vif_ctx_zep = CONTAINER_OF(work, struct wifi_nrf_vif_ctx_zep, scan_timeout_work);

	disp_scan_cb = (scan_result_cb_t)vif_ctx_zep->disp_scan_cb;

	if (!vif_ctx_zep->scan_in_progress) {
		LOG_INF("%s: Scan not in progress\n", __func__);
		return;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;

	if (disp_scan_cb) {
		memset(&res, 0x0, sizeof(res));

		disp_scan_cb(vif_ctx_zep->zep_net_if_ctx, -ETIMEDOUT, &res);
		vif_ctx_zep->disp_scan_cb = NULL;
	} else {
#ifdef CONFIG_WPA_SUPP
		/* WPA supplicant scan */
		union wpa_event_data event;
		struct scan_info *info = NULL;

		memset(&event, 0, sizeof(event));

		info = &event.scan_info;

		info->aborted = 0;
		info->external_scan = 0;
		info->nl_scan_event = 1;

		if (vif_ctx_zep->supp_drv_if_ctx &&
			vif_ctx_zep->supp_callbk_fns.scan_done) {
			vif_ctx_zep->supp_callbk_fns.scan_done(vif_ctx_zep->supp_drv_if_ctx,
				&event);
		}
#endif /* CONFIG_WPA_SUPP */
	}

	vif_ctx_zep->scan_in_progress = false;
}

int wifi_nrf_disp_scan_zep(const struct device *dev,
			   scan_result_cb_t cb)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_umac_scan_info scan_info;
	int ret = -1;

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		goto out;
	}

	if (vif_ctx_zep->if_op_state != WIFI_NRF_FMAC_IF_OP_STATE_UP) {
		LOG_ERR("%s: Interface not UP\n", __func__);
		goto out;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL\n", __func__);
		goto out;
	}

	if (vif_ctx_zep->scan_in_progress) {
		LOG_INF("%s: Scan already in progress\n", __func__);
		ret = -EBUSY;
		goto out;
	}

	vif_ctx_zep->disp_scan_cb = cb;

	memset(&scan_info, 0, sizeof(scan_info));

	scan_info.scan_mode = AUTO_SCAN;
	scan_info.scan_reason = SCAN_DISPLAY;

	if (!vif_ctx_zep->passive_scan) {
		/* Wildcard SSID to trigger active scan */
		scan_info.scan_params.num_scan_ssids = 1;
	}

	scan_info.scan_params.scan_ssids[0].nrf_wifi_ssid_len = 0;
	scan_info.scan_params.scan_ssids[0].nrf_wifi_ssid[0] = 0;

	status = wifi_nrf_fmac_scan(rpu_ctx_zep->rpu_ctx, vif_ctx_zep->vif_idx, &scan_info);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_scan failed\n", __func__);
		goto out;
	}

	vif_ctx_zep->scan_type = SCAN_DISPLAY;
	vif_ctx_zep->scan_in_progress = true;
	vif_ctx_zep->scan_res_cnt = 0;

	k_work_schedule(&vif_ctx_zep->scan_timeout_work, WIFI_NRF_SCAN_TIMEOUT);

	ret = 0;
out:
	return ret;
}

enum wifi_nrf_status wifi_nrf_disp_scan_res_get_zep(struct wifi_nrf_vif_ctx_zep *vif_ctx_zep)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	status = wifi_nrf_fmac_scan_res_get(rpu_ctx_zep->rpu_ctx,
					    vif_ctx_zep->vif_idx,
					    SCAN_DISPLAY);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_scan failed\n", __func__);
		goto out;
	}

	status = WIFI_NRF_STATUS_SUCCESS;
out:
	return status;
}

static inline enum wifi_security_type drv_to_wifi_mgmt(int drv_security_type)
{
	switch (drv_security_type) {
	case NRF_WIFI_OPEN:
		return WIFI_SECURITY_TYPE_NONE;
	case NRF_WIFI_WEP:
		return WIFI_SECURITY_TYPE_WEP;
	case NRF_WIFI_WPA:
		return WIFI_SECURITY_TYPE_WPA_PSK;
	case NRF_WIFI_WPA2:
		return WIFI_SECURITY_TYPE_PSK;
	case NRF_WIFI_WPA2_256:
		return WIFI_SECURITY_TYPE_PSK_SHA256;
	case NRF_WIFI_WPA3:
		return WIFI_SECURITY_TYPE_SAE;
	case NRF_WIFI_WAPI:
		return WIFI_SECURITY_TYPE_WAPI;
	case NRF_WIFI_EAP:
		return WIFI_SECURITY_TYPE_EAP;
	default:
		return WIFI_SECURITY_TYPE_UNKNOWN;
	}
}

void wifi_nrf_event_proc_disp_scan_res_zep(void *vif_ctx,
				struct nrf_wifi_umac_event_new_scan_display_results *scan_res,
				unsigned int event_len,
				bool more_res)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct umac_display_results *r = NULL;
	struct wifi_scan_result res;
	unsigned int i = 0;
	scan_result_cb_t cb = NULL;

	vif_ctx_zep = vif_ctx;

	cb = (scan_result_cb_t)vif_ctx_zep->disp_scan_cb;

	for (i = 0; i < scan_res->event_bss_count; i++) {
		/* Limit the scan results to the configured maximum */
		if ((CONFIG_NRF700X_SCAN_LIMIT >= 0) &&
		    (vif_ctx_zep->scan_res_cnt >= CONFIG_NRF700X_SCAN_LIMIT)) {
			break;
		}

		memset(&res, 0x0, sizeof(res));

		r = &scan_res->display_results[i];

		res.ssid_length = MIN(sizeof(res.ssid), r->ssid.nrf_wifi_ssid_len);

		res.band = r->nwk_band;

		res.channel = r->nwk_channel;

		res.security = drv_to_wifi_mgmt(r->security_type);

		memcpy(res.ssid,
		       r->ssid.nrf_wifi_ssid,
		       res.ssid_length);

		memcpy(res.mac,	r->mac_addr, NRF_WIFI_ETH_ADDR_LEN);
		res.mac_length = NRF_WIFI_ETH_ADDR_LEN;

		if (r->signal.signal_type == NRF_WIFI_SIGNAL_TYPE_MBM) {
			int val = (r->signal.signal.mbm_signal);

			res.rssi = (val / 100);
		} else if (r->signal.signal_type == NRF_WIFI_SIGNAL_TYPE_UNSPEC) {
			res.rssi = (r->signal.signal.unspec_signal);
		}

		vif_ctx_zep->disp_scan_cb(vif_ctx_zep->zep_net_if_ctx,
					  0,
					  &res);

		vif_ctx_zep->scan_res_cnt++;

		/* NET_MGMT dropping events if too many are queued */
		k_yield();
	}

	if (more_res == false) {
		vif_ctx_zep->disp_scan_cb(vif_ctx_zep->zep_net_if_ctx, 0, NULL);
		vif_ctx_zep->scan_in_progress = false;
		vif_ctx_zep->disp_scan_cb = NULL;
		k_work_cancel_delayable(&vif_ctx_zep->scan_timeout_work);
	}
}

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

	switch (twt_params->operation) {
	case WIFI_TWT_SETUP:
		if (vif_ctx_zep->twt_in_progress) {
			return WIFI_TWT_FAIL_OPERATION_IN_PROGRESS;
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

		if (status == WIFI_NRF_STATUS_SUCCESS) {
			vif_ctx_zep->twt_in_progress = true;
		}

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
			vif_ctx_zep->twt_in_progress = false;
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
	/* Store the negotiated flow id and pass it to user. */
	vif_ctx_zep->neg_twt_flow_id = twt_setup_info->info.twt_flow_id;
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
	    (twt_setup_info->info.neg_type != NRF_WIFI_ACCEPT_TWT)) {
		vif_ctx_zep->twt_in_progress = false;
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
	vif_ctx_zep->neg_twt_flow_id = 0XFF;
	vif_ctx_zep->twt_in_progress = false;

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

		fmac_dev_ctx->twt_sleep_status = WIFI_NRF_FMAC_TWT_STATE_SLEEP;

		wifi_nrf_osal_spinlock_rel(fmac_dev_ctx->fpriv->opriv,
					    fmac_dev_ctx->tx_config.tx_lock);
	break;
	case TWT_UNBLOCK_TX:
		wifi_nrf_osal_spinlock_take(fmac_dev_ctx->fpriv->opriv,
					    fmac_dev_ctx->tx_config.tx_lock);
		fmac_dev_ctx->twt_sleep_status = WIFI_NRF_FMAC_TWT_STATE_AWAKE;
#ifdef CONFIG_NRF700X_DATA_TX
		for (ac = WIFI_NRF_FMAC_AC_BE;
		     ac <= WIFI_NRF_FMAC_AC_MAX; ++ac) {
			desc = tx_desc_get(fmac_dev_ctx, ac);
			if (desc < fmac_dev_ctx->fpriv->num_tx_tokens) {
				tx_pending_process(fmac_dev_ctx, desc, ac);
			}
		}
#endif
		wifi_nrf_osal_spinlock_rel(fmac_dev_ctx->fpriv->opriv,
				fmac_dev_ctx->tx_config.tx_lock);
	break;
	default:
	break;
	}
}

#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS
void wifi_nrf_rx_bcn_prb_resp_frm(void *vif_ctx,
				  void *nwb,
				  unsigned short frequency,
				  signed short signal)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = vif_ctx;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct wifi_raw_scan_result bcn_prb_resp_info;
	int frame_length = 0;
	int val = signal;

	vif_ctx_zep = vif_ctx;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		return;
	}

	if (!vif_ctx_zep->scan_in_progress) {
		/*LOG_INF("%s: Scan not in progress : raw scan data not available\n", __func__);*/
		return;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL\n", __func__);
		return;
	}

	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;

	frame_length = wifi_nrf_osal_nbuf_data_size(fmac_dev_ctx->fpriv->opriv,
						    nwb);

	if (frame_length > CONFIG_WIFI_MGMT_RAW_SCAN_RESULT_LENGTH) {
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      &bcn_prb_resp_info.data,
				      wifi_nrf_osal_nbuf_data_get(
						fmac_dev_ctx->fpriv->opriv,
						nwb),
				      CONFIG_WIFI_MGMT_RAW_SCAN_RESULT_LENGTH);

	} else {
		wifi_nrf_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      &bcn_prb_resp_info.data,
				      wifi_nrf_osal_nbuf_data_get(
					      fmac_dev_ctx->fpriv->opriv,
					      nwb),
				      frame_length);
	}

#define MBM_TO_DBM 100
	bcn_prb_resp_info.rssi = (val / MBM_TO_DBM); /* mBm to dBm */
	bcn_prb_resp_info.frequency = frequency;
	bcn_prb_resp_info.frame_length = frame_length;

	wifi_mgmt_raise_raw_scan_result_event(vif_ctx_zep->zep_net_if_ctx,
					      &bcn_prb_resp_info);

}
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */
