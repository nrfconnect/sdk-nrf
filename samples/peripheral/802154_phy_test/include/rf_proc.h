/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: RF interfaces */

#ifndef RF_PROC_H__
#define RF_PROC_H__

#include <stdint.h>

#include "ptt_rf_api.h"

/**< Size of PHY header */
#define RF_PHR_SIZE (1u)

/**< Size of frame check sequence */
#define RF_FCS_SIZE (2u)

/**< Maximum size of PSDU */
#define RF_PSDU_MAX_SIZE (127u)

/**< Start of PSDU payload */
#define RF_PSDU_START (1u)

/** @brief NRF radio driver initialization
 *
 *  @param none
 *
 *  @return none
 */
void rf_init(void);

/** @brief NRF radio driver deinitialization
 *
 *  @param none
 *
 *  @return none
 */
void rf_uninit(void);

/** @brief Thread function for Radio handling
 *
 *  @param none
 *
 *  @return none
 *
 */
void rf_thread(void);

/**< element of received packets array */
struct rf_rx_pkt_s {
	uint8_t *rf_buf; /**< pointer to buffer inside radio driver with a received packet */
	uint8_t *data; /**< pointer to payload */
	ptt_pkt_len_t length; /**< size of payload of received packet */
	int8_t rssi; /**< RSSI */
	uint8_t lqi; /**< LQI */
};

#endif /* RF_PROC_H__ */
