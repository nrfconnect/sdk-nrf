/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
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

#include "util.h"
#include "fmac_api.h"
#include "fmac_tx.h"
#include "zephyr_fmac_main.h"
#include "zephyr_wifi_mgmt_scan.h"

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_LOG_LEVEL);

extern struct wifi_nrf_drv_priv_zep rpu_drv_priv_zep;

int wifi_nrf_disp_scan_zep(const struct device *dev, struct wifi_scan_params *params,
			   scan_result_cb_t cb)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_umac_scan_info scan_info;
	uint8_t band_flags = 0xFF;
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

	if (params) {
		band_flags &= (~(1 << WIFI_FREQ_BAND_2_4_GHZ));

#ifndef CONFIG_BOARD_NRF7001
		band_flags &= (~(1 << WIFI_FREQ_BAND_5_GHZ));
#endif /* CONFIG_BOARD_NF7001 */

		if (params->bands & band_flags) {
			LOG_ERR("%s: Unsupported band(s) (0x%X)\n", __func__, params->bands);
			ret = -EBUSY;
			goto out;
		}
	}

	vif_ctx_zep->disp_scan_cb = cb;

	memset(&scan_info, 0, sizeof(scan_info));

	scan_info.scan_reason = SCAN_DISPLAY;

	if (params) {
		if (params->scan_type == WIFI_SCAN_TYPE_ACTIVE) {
			/* Wildcard SSID to trigger active scan */
			scan_info.scan_params.num_scan_ssids = 1;
		}

		scan_info.scan_params.bands = params->bands;
		scan_info.scan_params.dwell_time_active = params->dwell_time_active;
	} else {
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

static inline enum wifi_mfp_options drv_to_wifi_mgmt_mfp(unsigned char mfp_flag)
{
	if (!mfp_flag)
		return WIFI_MFP_DISABLE;
	if (mfp_flag & NRF_WIFI_MFP_REQUIRED)
		return WIFI_MFP_REQUIRED;
	if (mfp_flag & NRF_WIFI_MFP_CAPABLE)
		return WIFI_MFP_OPTIONAL;

	return WIFI_MFP_UNKNOWN;
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

		res.mfp = drv_to_wifi_mgmt_mfp(r->mfp_flag);

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

	bcn_prb_resp_info.rssi = MBM_TO_DBM(val);
	bcn_prb_resp_info.frequency = frequency;
	bcn_prb_resp_info.frame_length = frame_length;

	wifi_mgmt_raise_raw_scan_result_event(vif_ctx_zep->zep_net_if_ctx,
					      &bcn_prb_resp_info);

}
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */
