/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief Header containing command specific declarations for the
 * offloaded raw TX mode in the FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_CMD_OFF_RAW_TX_H__
#define __FMAC_CMD_OFF_RAW_TX_H__

#include "common/fmac_cmd_common.h"

enum nrf_wifi_status umac_cmd_off_raw_tx_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					      struct nrf_wifi_phy_rf_params *rf_params,
					      bool rf_params_valid,
#ifdef NRF_WIFI_LOW_POWER
					      int sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
					      unsigned int phy_calib,
					      unsigned char op_band,
					      bool beamforming,
					      struct nrf_wifi_tx_pwr_ctrl_params
					      *tx_pwr_ctrl_params,
					      struct nrf_wifi_board_params *board_params,
					      unsigned char *country_code);


enum nrf_wifi_status umac_cmd_off_raw_tx_prog_stats_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

enum nrf_wifi_status umac_cmd_off_raw_tx_conf(
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
	struct nrf_wifi_offload_ctrl_params *offloaded_tx_params,
	struct nrf_wifi_offload_tx_ctrl *offload_tx_ctr);

enum nrf_wifi_status umac_cmd_off_raw_tx_ctrl(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					      unsigned char ctrl_type);

#endif /* __FMAC_CMD_OFF_RAW_TX_H__ */
