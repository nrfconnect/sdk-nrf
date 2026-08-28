/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OFF_RAW_TX_API_H__
#define __OFF_RAW_TX_API_H__

/**
 * @brief Header containing offloaded raw TX mode specific API declarations
 */

#include <zephyr/kernel.h>
#include <common/fw_if/nrf71_wifi_ctrl.h>
#include <common/rf_params.h>
#include <common/vtf.h>

/*  Minimum frame size for raw packet transmission */
#define NRF_WIFI_OFF_RAW_TX_FRAME_SIZE_MIN 26
/*  Maximum frame size for raw packet transmission */
#define NRF_WIFI_OFF_RAW_TX_FRAME_SIZE_MAX 600
/* Maximum length of country code*/
#define NRF_WIFI_COUNTRY_CODE_LEN 2

struct nrf_wifi_off_raw_tx_drv_ctx {
	void *drv_priv;
	void *rpu_ctx;
	uint8_t mac_addr[6];
	unsigned int phy_rf_params_addr[NUM_RF_PARAM_ADDRS];
	unsigned int vtf_buffer_start_address;
};


struct nrf_wifi_off_raw_tx_drv_priv {
	struct nrf_wifi_fmac_priv *fmac_priv;
	struct nrf_wifi_off_raw_tx_drv_ctx drv_ctx;
	struct k_spinlock lock;
};

extern struct nrf_wifi_off_raw_tx_drv_priv off_raw_tx_drv_priv;
#endif /* __OFF_RAW_TX_API_H__ */
