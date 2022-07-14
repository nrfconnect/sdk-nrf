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
#include <zephyr.h>
#include "fmac_api.h"
#include "zephyr_fmac_main.h"
#include "zephyr_disp_scan.h"

int wifi_nrf_disp_scan_zep(const struct device *dev,
			   scan_result_cb_t cb)
{
	enum wifi_nrf_status status = WIFI_NRF_STATUS_FAIL;
	struct wifi_nrf_ctx_zep *rpu_ctx_zep = NULL;
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct img_umac_scan_info scan_info;
	int ret = -1;

	vif_ctx_zep = dev->data;
	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		goto out;
	}

	vif_ctx_zep->disp_scan_cb = cb;

	memset(&scan_info, 0, sizeof(scan_info));

	scan_info.scan_mode = AUTO_SCAN;
	scan_info.scan_reason = SCAN_DISPLAY;

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

void wifi_nrf_event_proc_disp_scan_res_zep(void *vif_ctx,
					   struct img_umac_event_new_scan_display_results *scan_res,
					   unsigned int event_len,
					   bool more_res)
{
	struct wifi_nrf_vif_ctx_zep *vif_ctx_zep = NULL;
	struct umac_display_results *r = NULL;
	struct wifi_scan_result res;
	unsigned int i = 0;

	vif_ctx_zep = vif_ctx;

	for (i = 0; i < scan_res->event_bss_count; i++) {
		memset(&res, 0x0, sizeof(res));

		r = &scan_res->display_results[i];

		res.ssid_length = MIN(sizeof(res.ssid), r->ssid.img_ssid_len);

		res.channel = r->nwk_channel;
		res.security = WIFI_SECURITY_TYPE_NONE;

		/* TODO : show other security modes as PSK */
		if (r->security_type != IMG_OPEN) {
			res.security = WIFI_SECURITY_TYPE_PSK;
		}

		memcpy(res.ssid,
		       r->ssid.img_ssid,
		       res.ssid_length);

		if (r->signal.signal_type == IMG_SIGNAL_TYPE_MBM) {
			int val = (r->signal.signal.mbm_signal);

			res.rssi = (val / 100);
		} else if (r->signal.signal_type == IMG_SIGNAL_TYPE_UNSPEC) {
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
