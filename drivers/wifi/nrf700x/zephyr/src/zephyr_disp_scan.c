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
#include "zephyr_fmac_main.h"
#include "zephyr_disp_scan.h"

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_LOG_LEVEL);

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

	cb = (scan_result_cb_t) vif_ctx_zep->disp_scan_cb;

	for (i = 0; i < scan_res->event_bss_count; i++) {
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

		/* NET_MGMT dropping events if too many are queued */
		k_yield();
	}

	if (more_res == false) {
		vif_ctx_zep->disp_scan_cb(vif_ctx_zep->zep_net_if_ctx, 0, NULL);

		vif_ctx_zep->scan_in_progress = false;
	}
}
