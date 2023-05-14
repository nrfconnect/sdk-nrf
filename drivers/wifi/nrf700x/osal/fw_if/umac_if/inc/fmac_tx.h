/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing TX data path specific declarations for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_TX_H__
#define __FMAC_TX_H__

#include "host_rpu_data_if.h"
#include "fmac_structs.h"

#define TX_DESC_BUCKET_BOUND 32
#define DOT11_WMM_PARAMS_LEN 2

/* 4 bits represent 4 access categories.
 * (VOVIBEBO) respectively
 */
#define SPARE_DESC_Q_MAP_SIZE 4

/**
 * enum nrf_wifi_fmac_tx_status - The status of a TX operation performed by the
 *						RPU driver.
 * @NRF_WIFI_FMAC_TX_STATUS_SUCCESS: The TX operation was successful (sent packet to RPU).
 * @NRF_WIFI_FMAC_TX_STATUS_QUEUED: The TX operation was successful (packet queued in driver).
 * @NRF_WIFI_FMAC_TX_STATUS_FAIL: The TX operation failed.
 *
 * This enum lists the possible outcomes of a TX operation performed by the
 * RPU driver.
 */
enum nrf_wifi_fmac_tx_status {
	NRF_WIFI_FMAC_TX_STATUS_SUCCESS,
	NRF_WIFI_FMAC_TX_STATUS_QUEUED = 1,
	NRF_WIFI_FMAC_TX_STATUS_FAIL = -1,
};

struct tx_pkt_info {
	void *pkt;
	unsigned int peer_id;
};

struct tx_cmd_prep_info {
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx;
	struct nrf_wifi_tx_buff *config;
};

enum nrf_wifi_status tx_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

void tx_deinit(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

enum nrf_wifi_status nrf_wifi_fmac_tx_done_event_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
							 struct nrf_wifi_tx_buff_done *config);

unsigned int tx_desc_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
			 int queue);

enum nrf_wifi_status tx_pending_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					unsigned int desc,
					unsigned int ac);

enum nrf_wifi_status tx_cmd_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
				 void *txq,
				 int desc,
				 int peer_id);

unsigned int tx_buff_req_free(struct nrf_wifi_fmac_dev_ctx *fmac_ctx,
			      unsigned int desc,
			      unsigned char *ac);

#endif /* __FMAC_TX_H__ */
