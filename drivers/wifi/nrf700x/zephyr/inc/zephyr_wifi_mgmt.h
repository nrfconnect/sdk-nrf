/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing WiFi management operation implementations
 * for the Zephyr OS.
 */

#ifndef __ZEPHYR_WIFI_MGMT_H__
#define __ZEPHYR_WIFI_MGMT_H__
#include <math.h>

#include <zephyr/device.h>
#include <zephyr/net/wifi_mgmt.h>

#include "osal_api.h"

struct twt_interval_float {
	unsigned short mantissa;
	unsigned char exponent;
};

int wifi_nrf_set_power_save(const struct device *dev,
			    struct wifi_ps_params *params);

int wifi_nrf_set_twt(const struct device *dev,
	struct wifi_twt_params *twt_params);

void wifi_nrf_event_proc_twt_setup_zep(void *vif_ctx,
		struct nrf_wifi_umac_cmd_config_twt *twt_setup_info,
		unsigned int event_len);

void wifi_nrf_event_proc_twt_teardown_zep(void *vif_ctx,
		struct nrf_wifi_umac_cmd_teardown_twt *twt_teardown_info,
		unsigned int event_len);

void wifi_nrf_event_proc_twt_sleep_zep(void *vif_ctx,
		struct nrf_wifi_umac_event_twt_sleep *twt_sleep_info,
		unsigned int event_len);

int wifi_nrf_twt_teardown_flows(struct wifi_nrf_vif_ctx_zep *vif_ctx_zep,
		unsigned char start_flow_id, unsigned char end_flow_id);

int wifi_nrf_get_power_save_config(const struct device *dev,
		struct wifi_ps_config *ps_config);

void wifi_nrf_event_proc_get_power_save_info(void *vif_ctx,
		struct nrf_wifi_umac_event_power_save_info *ps_info,
		unsigned int event_len);

#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS
void wifi_nrf_rx_bcn_prb_resp_frm(void *vif_ctx,
				  void *frm,
				  unsigned short frequency,
				  signed short signal);
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */
#endif /*  __ZEPHYR_WIFI_MGMT_H__ */
