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

#include <zephyr/zephyr.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/ethernet.h>

#include "fmac_api.h"
#include "host_rpu_umac_if.h"
#ifdef CONFIG_WPA_SUPP
#include "drivers/driver_zephyr.h"
#endif /* CONFIG_WPA_SUPP */

struct wifi_nrf_vif_ctx_zep {
	const struct device *zep_dev_ctx;
	struct net_if *zep_net_if_ctx;
	void *supp_drv_if_ctx;
	void *rpu_ctx_zep;
	unsigned char vif_idx;

	scan_result_cb_t disp_scan_cb;
	bool scan_in_progress;
	int scan_type;

	unsigned int assoc_freq;
	enum wifi_nrf_fmac_if_state if_state;
#ifdef CONFIG_WPA_SUPP
	struct zep_wpa_supp_dev_callbk_fns supp_callbk_fns;
#endif /* CONFIG_WPA_SUPP */
};

struct wifi_nrf_vif_ctx_map {
	const char *ifname;
	struct wifi_nrf_vif_ctx_zep *vif_ctx;
};

struct wifi_nrf_ctx_zep {
	void *drv_priv_zep;
	void *rpu_ctx;
	struct wifi_nrf_vif_ctx_zep vif_ctx_zep[MAX_NUM_VIFS];
	struct net_eth_addr mac_addr;
};

struct wifi_nrf_drv_priv_zep {
	struct wifi_nrf_fmac_priv *fmac_priv;
	/* TODO: Replace with a linked list to handle unlimited RPUs */
	struct wifi_nrf_ctx_zep rpu_ctx_zep;
};


struct wifi_nrf_dev_offload_ops {
	int (*disp_scan)(const struct device *dev,
			 scan_result_cb_t cb);
};

#endif /* __ZEPHYR_FMAC_MAIN_H__ */
