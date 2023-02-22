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
	case NRF_WIFI_WPA2:
		return WIFI_SECURITY_TYPE_PSK;
	case NRF_WIFI_WPA2_256:
		return WIFI_SECURITY_TYPE_PSK_SHA256;
	case NRF_WIFI_WPA3:
		return WIFI_SECURITY_TYPE_SAE;
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
	}
}

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

	status = wifi_nrf_fmac_get_power_save_info(rpu_ctx_zep->rpu_ctx,
						   vif_ctx_zep->vif_idx);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_get_power_save_info failed\n",
			__func__);
		goto out;
	}

	vif_ctx_zep->ps_config_info_evnt = false;

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
	/* TODO: ADD reason code in the twt_params structure */
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

int wifi_nrf_set_power_save_timeout(const struct device *dev,
			 struct wifi_ps_timeout_params *ps_timeout_params)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	int ret = -1;

	if (!dev || !ps_timeout_params) {
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

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL\n", __func__);
		goto out;
	}

	status = wifi_nrf_fmac_set_power_save_timeout(rpu_ctx_zep->rpu_ctx,
					  vif_ctx_zep->vif_idx,
					  ps_timeout_params->timeout_ms);

	if (status != WIFI_NRF_STATUS_SUCCESS) {
		LOG_ERR("%s: wifi_nrf_fmac_ failed\n", __func__);
		goto out;
	}

	ret = 0;
out:
	return ret;
}
