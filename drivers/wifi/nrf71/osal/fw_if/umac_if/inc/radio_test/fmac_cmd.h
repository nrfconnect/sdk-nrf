/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief Header containing command specific declarations for the
 * radio test mode in the FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_CMD_RT_H__
#define __FMAC_CMD_RT_H__

#include "common/fmac_cmd_common.h"

#define NRF_WIFI_FMAC_RF_TEST_EVNT_TIMEOUT 50 /* 5s */

enum nrf_wifi_status umac_cmd_rt_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				      unsigned int *rf_params_addr,
				      unsigned int vtf_buffer_start_address,
				      bool rf_params_valid,
#ifdef NRF_WIFI_LOW_POWER
				      int sleep_type,
#endif /* NRF_WIFI_LOW_POWER */
				      unsigned int phy_calib,
				      unsigned char op_band,
				      bool beamforming,
				      struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl_params,
				      struct nrf_wifi_board_params *board_params,
				      unsigned char *country_code);

enum nrf_wifi_status umac_cmd_rt_prog_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					   struct nrf_wifi_radio_test_init_info *init_params);

enum nrf_wifi_status umac_cmd_rt_prog_tx(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					 struct rpu_conf_params *params);

enum nrf_wifi_status umac_cmd_rt_prog_rx(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					 struct rpu_conf_rx_radio_test_params *rx_params);

enum nrf_wifi_status umac_cmd_rt_prog_rf_test(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					      void *rf_test_params,
					      unsigned int rf_test_params_sz);


enum nrf_wifi_status umac_cmd_rt_prog_stats_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
						int op_mode);
#endif /* __FMAC_CMD_RT_H__ */
