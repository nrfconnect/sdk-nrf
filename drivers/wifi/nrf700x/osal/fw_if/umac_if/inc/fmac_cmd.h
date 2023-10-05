/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing command specific declarations for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_CMD_H__
#define __FMAC_CMD_H__

#define NRF_WIFI_FMAC_STATS_RECV_TIMEOUT 50 /* ms */
#define NRF_WIFI_FMAC_PS_CONF_EVNT_RECV_TIMEOUT 50 /* ms */
#ifdef CONFIG_NRF700X_RADIO_TEST
#define NRF_WIFI_FMAC_RF_TEST_EVNT_TIMEOUT 50 /* 5s */
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

struct host_rpu_msg *umac_cmd_alloc(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				    int type,
				    int size);

enum nrf_wifi_status umac_cmd_cfg(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				  void *params,
				  int len);

enum nrf_wifi_status umac_cmd_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
#ifndef CONFIG_NRF700X_RADIO_TEST
				   unsigned char *rf_params,
				   bool rf_params_valid,
				   struct nrf_wifi_data_config_params *config,
#endif /* !CONFIG_NRF700X_RADIO_TEST */
#ifdef CONFIG_NRF_WIFI_LOW_POWER
				   int sleep_type,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
				   unsigned int phy_calib,
				   enum op_band op_band,
				   bool beamforming,
				   struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl_params);

enum nrf_wifi_status umac_cmd_deinit(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

enum nrf_wifi_status umac_cmd_btcoex(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
	void *cmd, unsigned int cmd_len);

enum nrf_wifi_status umac_cmd_he_ltf_gi(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					unsigned char he_ltf,
					unsigned char he_gi,
					unsigned char enabled);

#ifdef CONFIG_NRF700X_RADIO_TEST
enum nrf_wifi_status umac_cmd_prog_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					struct nrf_wifi_radio_test_init_info *init_params);

enum nrf_wifi_status umac_cmd_prog_tx(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				      struct rpu_conf_params *params);

enum nrf_wifi_status umac_cmd_prog_rx(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				      struct rpu_conf_rx_radio_test_params *rx_params);

enum nrf_wifi_status umac_cmd_prog_rf_test(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					   void *rf_test_params,
					   unsigned int rf_test_params_sz);
#endif /* CONFIG_NRF700X_RADIO_TEST */

enum nrf_wifi_status umac_cmd_prog_stats_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
#ifdef CONFIG_NRF700X_RADIO_TEST
					     int op_mode,
#endif /* CONFIG_NRF700X_RADIO_TEST */
					     int stat_type);

#endif /* __FMAC_CMD_H__ */
