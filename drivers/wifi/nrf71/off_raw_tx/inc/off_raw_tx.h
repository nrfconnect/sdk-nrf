/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OFF_RAW_TX_H__
#define __OFF_RAW_TX_H__

/**
 * @brief File containing internal structures for the offloaded raw TX feature in the driver.
 */

#include "offload_raw_tx/fmac_structs.h"
#include "osal_api.h"

#define NUM_RF_PARAM_ADDRS_OFF_RAW_TX 22

struct nrf_wifi_ctx_zep {
	void *drv_priv_zep;
	void *rpu_ctx;
	uint8_t mac_addr[6];
	unsigned int phy_rf_params_addr[NUM_RF_PARAM_ADDRS_OFF_RAW_TX];
	unsigned int vtf_buffer_start_address;
};


struct nrf_wifi_off_raw_tx_drv_priv {
	struct nrf_wifi_fmac_priv *fmac_priv;
	/* TODO: Replace with a linked list to handle unlimited RPUs */
	struct nrf_wifi_ctx_zep rpu_ctx_zep;
	struct k_spinlock lock;
};

#endif /* __OFF_RAW_TX_H__ */
