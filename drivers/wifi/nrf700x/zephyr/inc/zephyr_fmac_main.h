/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing FMAC interface specific declarations for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#ifndef __ZEPHYR_FMAC_MAIN_H__
#define __ZEPHYR_FMAC_MAIN_H__

#include <stdio.h>

#include <zephyr/kernel.h>
#ifndef CONFIG_NRF700X_RADIO_TEST
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/ethernet.h>
#ifdef CONFIG_NETWORKING
#include <zephyr_net_if.h>
#endif /* CONFIG_NETWORKING */
#ifdef CONFIG_WPA_SUPP
#include <drivers/driver_zephyr.h>
#endif /* CONFIG_WPA_SUPP */
#endif /* !CONFIG_NRF700X_RADIO_TEST */

#include <fmac_api.h>
#include <host_rpu_umac_if.h>

#ifndef CONFIG_NRF700X_RADIO_TEST
struct wifi_nrf_vif_ctx_zep {
	const struct device *zep_dev_ctx;
	struct net_if *zep_net_if_ctx;
	void *supp_drv_if_ctx;
	void *rpu_ctx_zep;
	unsigned char vif_idx;

	scan_result_cb_t disp_scan_cb;
	bool scan_in_progress;
	int scan_type;

	struct net_eth_addr mac_addr;

	unsigned int assoc_freq;
	enum wifi_nrf_fmac_if_op_state if_op_state;
	enum wifi_nrf_fmac_if_carr_state if_carr_state;
	struct wpa_signal_info *signal_info;
#ifdef CONFIG_WPA_SUPP
	struct zep_wpa_supp_dev_callbk_fns supp_callbk_fns;
#endif /* CONFIG_WPA_SUPP */
	/* Used to store the negotiated twt flow id
	 * for "twt_teardown_all" command.
	 */
	unsigned char neg_twt_flow_id;
#ifdef CONFIG_NET_STATISTICS_ETHERNET
	struct net_stats_eth eth_stats;
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
	int if_type;
	struct wifi_ps_config *ps_info;
	bool ps_config_info_evnt;
};

struct wifi_nrf_vif_ctx_map {
	const char *ifname;
	struct wifi_nrf_vif_ctx_zep *vif_ctx;
};
#endif /* !CONFIG_NRF700X_RADIO_TEST */

struct wifi_nrf_ctx_zep {
	void *drv_priv_zep;
	void *rpu_ctx;
#ifdef CONFIG_NRF700X_RADIO_TEST
	struct rpu_conf_params conf_params;
	bool rf_test_run;
	unsigned char rf_test;
#else /* CONFIG_NRF700X_RADIO_TEST */
	struct wifi_nrf_vif_ctx_zep vif_ctx_zep[MAX_NUM_VIFS];
#ifdef CONFIG_NRF700X_WIFI_UTIL
	struct rpu_conf_params conf_params;
#endif /* CONFIG_NRF700X_WIFI_UTIL */
#endif /* CONFIG_NRF700X_RADIO_TEST */
};

struct wifi_nrf_drv_priv_zep {
	struct wifi_nrf_fmac_priv *fmac_priv;
	/* TODO: Replace with a linked list to handle unlimited RPUs */
	struct wifi_nrf_ctx_zep rpu_ctx_zep;
};
#endif /* __ZEPHYR_FMAC_MAIN_H__ */
