/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @brief Header containing common (mode agnostic) command specific
 * declarations for the FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_CMD_COMMON_H__
#define __FMAC_CMD_COMMON_H__

#include "fmac_structs_common.h"

#define NRF_WIFI_FMAC_STATS_RECV_TIMEOUT 50 /* ms */
#define NRF_WIFI_FMAC_PS_CONF_EVNT_RECV_TIMEOUT 50 /* ms */
#define NRF_WIFI_FMAC_REG_SET_TIMEOUT_MS 2000 /* 2s */

struct host_rpu_msg *umac_cmd_alloc(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				    int type,
				    int size);

enum nrf_wifi_status umac_cmd_cfg(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				  void *params,
				  int len);

enum nrf_wifi_status umac_cmd_deinit(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

enum nrf_wifi_status umac_cmd_srcoex(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				     void *cmd, unsigned int cmd_len);

enum nrf_wifi_status umac_cmd_prog_stats_reset(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

enum nrf_wifi_status umac_cmd_set_ps_exit_strategy(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					enum ps_exit_strategy ps_exit_strategy);
#endif /* __FMAC_CMD_COMMON_H__ */
