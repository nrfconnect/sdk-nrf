/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing radio test API declarations
 */

#ifndef __RADIO_TEST_API_H__
#define __RADIO_TEST_API_H__

#include <zephyr/kernel.h>
#include <nrf71_wifi_ctrl.h>
#include <common/rf_params.h>
#include <radio_test/fmac_api.h>

struct nrf_wifi_rt_drv_ctx {
	void *drv_priv;
	void *rpu_ctx;
	struct rpu_conf_params conf_params;
	bool rf_test_run;
	unsigned char rf_test;
	struct k_mutex rpu_lock;
	unsigned int phy_rf_params_addr[NUM_RF_PARAM_ADDRS];
	unsigned int vtf_buffer_start_address;
};

struct nrf_wifi_rt_drv_priv {
	struct nrf_wifi_fmac_priv *fmac_priv;
	struct nrf_wifi_rt_drv_ctx drv_ctx;
};


enum nrf_wifi_status nrf_wifi_rt_fmac_dev_rem(struct nrf_wifi_rt_drv_priv *drv_priv);

#endif /* __RADIO_TEST_API_H__ */
