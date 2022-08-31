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

#define WIFI_NRF_FMAC_STATS_RECV_TIMEOUT 50 /* ms */

struct host_rpu_msg *umac_cmd_alloc(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				    int type,
				    int size);

enum wifi_nrf_status umac_cmd_cfg(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				  void *params,
				  int len);

enum wifi_nrf_status umac_cmd_init(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				   unsigned char *def_mac_addr,
				   unsigned char def_vif_idx,
				   unsigned char *rf_params,
				   bool rf_params_valid,
#ifdef CONFIG_NRF_WIFI_LOW_POWER
				   int sleep_type,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
				   struct img_data_config_params config,
				   unsigned int phy_calib);

enum wifi_nrf_status umac_cmd_deinit(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx);

enum wifi_nrf_status umac_cmd_btcoex(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
				     struct rpu_btcoex *params);

enum wifi_nrf_status umac_cmd_he_ltf_gi(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					unsigned char he_ltf,
					unsigned char he_gi,
					unsigned char enabled);


enum wifi_nrf_status umac_cmd_prog_stats_get(struct wifi_nrf_fmac_dev_ctx *fmac_dev_ctx,
					     int stat_type);

#endif /* __FMAC_CMD_H__ */
