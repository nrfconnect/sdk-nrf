/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file fmac_tx.h
 *
 * @brief Header containing TX data path specific declarations for the
 * FMAC IF Layer of the Wi-Fi driver.
 */

#ifndef __FMAC_TX_H__
#define __FMAC_TX_H__

#include <nrf71_wifi_ctrl.h>
#include "system/fmac_structs.h"

/**
 * @defgroup fmac_tx FMAC TX
 * @{
 */

/**
 * @brief The maximum number of descriptors in a TX descriptor bucket.
 */
#define TX_DESC_BUCKET_BOUND 32

/**
 * @brief The length of the WMM parameters.
 */
#define DOT11_WMM_PARAMS_LEN 2

/**
 * @brief The size of the spare descriptor queue map.
 *
 * Four bits represent four access categories: VO, VI, BE, and BK.
 */
#define SPARE_DESC_Q_MAP_SIZE 4

/**
 * @brief The status of a TX operation performed by the RPU driver.
 */
enum nrf_wifi_fmac_tx_status {
	/** The TX operation was successful (sent packet to RPU). */
	NRF_WIFI_FMAC_TX_STATUS_SUCCESS,
	/** The TX operation was successful (packet queued in driver). */
	NRF_WIFI_FMAC_TX_STATUS_QUEUED = 1,
	/** The TX operation failed. */
	NRF_WIFI_FMAC_TX_STATUS_FAIL = -1
};

/**
 * @brief Structure containing information about a TX packet.
 */
struct tx_pkt_info {
	/** Pointer to the TX packet. */
	void *pkt;
	/** Peer ID. */
	unsigned int peer_id;
};

#ifdef NRF71_RAW_DATA_TX
/**
 * @brief Structure containing information for preparing a raw TX command.
 */
struct tx_cmd_prep_raw_info {
	/** Pointer to the FMAC device context. */
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx;
	/** Pointer to the raw TX configuration. */
	struct nrf_wifi_cmd_raw_tx *raw_config;
	/** Number of TX packets. */
	unsigned char num_tx_pkts;
};
#endif /* NRF71_RAW_DATA_TX */

/**
 * @brief Structure containing information for preparing a TX command.
 */
struct tx_cmd_prep_info {
	/** Pointer to the FMAC device context. */
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx;
	/** Pointer to the TX configuration. */
	struct nrf_wifi_tx_buff *config;
};

/**
 * @brief Initialize the TX module.
 *
 * @param fmac_dev_ctx Pointer to the FMAC device context.
 * @return The status of the initialization.
 */
enum nrf_wifi_status tx_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

/**
 * @brief Deinitialize the TX module.
 *
 * @param fmac_dev_ctx Pointer to the FMAC device context.
 */
void tx_deinit(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx);

/**
 * @brief Process the TX done event.
 *
 * @param fmac_dev_ctx Pointer to the FMAC device context.
 * @param config Pointer to the TX buffer done configuration.
 * @return The status of the event processing.
 */
enum nrf_wifi_status nrf_wifi_fmac_tx_done_event_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
		struct nrf_wifi_tx_buff_done *config);

#ifdef NRF71_RAW_DATA_TX
/**
 * @brief Process the raw TX done event.
 *
 * @param fmac_dev_ctx Pointer to the FMAC device context.
 * @param config Pointer to the raw TX done configuration.
 * @return The status of the event processing.
 */
enum nrf_wifi_status
nrf_wifi_fmac_rawtx_done_event_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
		struct nrf_wifi_event_raw_tx_done *config);
#endif /* CONFIG_NRF71_RAW_DATA_TX */

/**
 * @brief Get a TX descriptor from the specified queue.
 *
 * @param fmac_dev_ctx Pointer to the FMAC device context.
 * @param queue The queue index.
 * @return The TX descriptor.
 */
unsigned int tx_desc_get(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
		int queue);

/**
 * @brief Process the pending TX descriptors.
 *
 * @param fmac_dev_ctx Pointer to the FMAC device context.
 * @param desc The descriptor index.
 * @param ac The access category.
 * @return The status of the pending process.
 */
enum nrf_wifi_status tx_pending_process(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
		unsigned int desc,
		unsigned int ac);

/**
 * @brief Initialize a TX command.
 *
 * @param fmac_dev_ctx Pointer to the FMAC device context.
 * @param txq Pointer to the TX queue.
 * @param desc The descriptor index.
 * @param peer_id The peer ID.
 * @return The status of the command initialization.
 */
enum nrf_wifi_status tx_cmd_init(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
		void *txq,
		int desc,
		int peer_id);

/**
 * @brief Request free TX buffers.
 *
 * @param fmac_ctx Pointer to the FMAC context.
 * @param desc The descriptor index.
 * @param ac Pointer to the access category.
 * @return The number of free TX buffers.
 */
unsigned int tx_buff_req_free(struct nrf_wifi_fmac_dev_ctx *fmac_ctx,
		unsigned int desc,
		unsigned char *ac);

/** @} */

#endif /* __FMAC_TX_H__ */
