/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing display scan specific declarations for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#ifndef __ZEPHYR_DISP_SCAN_H__
#define __ZEPHYR_DISP_SCAN_H__
#include <math.h>

#include <zephyr/device.h>
#include <zephyr/net/wifi_mgmt.h>

#include "osal_api.h"

struct twt_interval_float {
	unsigned short mantissa;
	unsigned char exponent;
};

int nrf_wifi_disp_scan_zep(const struct device *dev, scan_result_cb_t cb);

enum nrf_wifi_status nrf_wifi_disp_scan_res_get_zep(struct nrf_wifi_vif_ctx_zep *vif_ctx_zep);

void nrf_wifi_event_proc_disp_scan_res_zep(void *vif_ctx,
				struct nrf_wifi_umac_event_new_scan_display_results *scan_res,
				unsigned int event_len,
				bool is_last);

int nrf_wifi_set_power_save(const struct device *dev,
			    struct wifi_ps_params *params);

int nrf_wifi_set_twt(const struct device *dev,
		     struct wifi_twt_params *twt_params);

void nrf_wifi_event_proc_twt_setup_zep(void *vif_ctx,
		struct nrf_wifi_umac_cmd_config_twt *twt_setup_info,
		unsigned int event_len);

void nrf_wifi_event_proc_twt_teardown_zep(void *vif_ctx,
		struct nrf_wifi_umac_cmd_teardown_twt *twt_teardown_info,
		unsigned int event_len);

void nrf_wifi_event_proc_twt_sleep_zep(void *vif_ctx,
		struct nrf_wifi_umac_event_twt_sleep *twt_sleep_info,
		unsigned int event_len);

int nrf_wifi_get_power_save_config(const struct device *dev,
		struct wifi_ps_config *ps_config);

void nrf_wifi_event_proc_get_power_save_info(void *vif_ctx,
		struct nrf_wifi_umac_event_power_save_info *ps_info,
		unsigned int event_len);

#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS
void nrf_wifi_rx_bcn_prb_resp_frm(void *vif_ctx,
				  void *frm,
				  unsigned short frequency,
				  signed short signal);
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */
#endif /*  __ZEPHYR_DISP_SCAN_H__ */
