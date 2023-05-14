/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing RX data path specific declarations for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_RX_H__
#define __FMAC_RX_H__

#include "host_rpu_data_if.h"
#include "fmac_structs.h"

#define RX_BUF_HEADROOM 4

enum nrf_wifi_fmac_rx_cmd_type {
	NRF_WIFI_FMAC_RX_CMD_TYPE_INIT,
	NRF_WIFI_FMAC_RX_CMD_TYPE_DEINIT,
	NRF_WIFI_FMAC_RX_CMD_TYPE_MAX,
};


struct nrf_wifi_fmac_rx_pool_map_info {
	unsigned int pool_id;
	unsigned int buf_id;
};


enum nrf_wifi_status nrf_wifi_fmac_rx_cmd_send(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					       enum nrf_wifi_fmac_rx_cmd_type cmd_type,
					       unsigned int desc_id);

enum nrf_wifi_status nrf_wifi_fmac_rx_event_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
						    struct nrf_wifi_rx_buff *config);

void nrf_wifi_fmac_rx_tasklet(void *data);

#endif /* __FMAC_RX_H__ */
