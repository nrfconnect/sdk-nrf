/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_PING_PDU_H
#define DECT_PHY_PING_PDU_H

#include <zephyr/kernel.h>
#include "dect_common.h"
#include "dect_common_pdu.h"

/**************************************************************************************************/

typedef enum {
	DECT_MAC_MESSAGE_TYPE_PING_REQUEST = 100, /* Note: the limit of 7bit for the value */
	DECT_MAC_MESSAGE_TYPE_PING_RESPONSE,
	DECT_MAC_MESSAGE_TYPE_PING_RESULTS_REQ,
	DECT_MAC_MESSAGE_TYPE_PING_RESULTS_RESP,
	DECT_MAC_MESSAGE_TYPE_PING_HARQ_FEEDBACK,
} dect_phy_ping_pdu_message_type_t;

/**************************************************************************************************/

#define DECT_PHY_PING_PDU_COMMON_PART_LEN                                                          \
	(sizeof(dect_phy_ping_pdu_message_type_t) + sizeof(uint8_t) + sizeof(uint16_t))
#define DECT_PHY_PING_TX_DATA_PDU_LEN_WITHOUT_PAYLOAD                                              \
	(DECT_PHY_PING_PDU_COMMON_PART_LEN + (2 * sizeof(uint16_t)))
#define DECT_PHY_PING_TX_DATA_PDU_PAYLOAD_MAX_LEN                                                  \
	(DECT_DATA_MAX_LEN - DECT_PHY_PING_TX_DATA_PDU_LEN_WITHOUT_PAYLOAD)
#define DECT_PHY_PING_RESULTS_REQ_LEN	   (DECT_PHY_PING_PDU_COMMON_PART_LEN + sizeof(uint32_t))
#define DECT_PHY_PING_RESULTS_DATA_MAX_LEN 480

typedef struct {
	char results_str[DECT_PHY_PING_RESULTS_DATA_MAX_LEN];
} dect_phy_ping_pdu_results_resp_data;

typedef struct {
	uint32_t unused;
} dect_phy_ping_pdu_results_req;

typedef struct {
	uint16_t seq_nbr;
	uint16_t payload_length;
	uint8_t pdu_payload[DECT_PHY_PING_TX_DATA_PDU_PAYLOAD_MAX_LEN];
} dect_phy_ping_pdu_tx_data;

typedef union {
	dect_phy_ping_pdu_tx_data tx_data;
	dect_phy_ping_pdu_results_req results_req;
	dect_phy_ping_pdu_results_resp_data results;
} dect_phy_ping_pdu_message_t;

typedef struct {
	dect_phy_ping_pdu_message_type_t message_type; /* Note: only 7 bit */
	int8_t pwr_ctrl_expected_rssi_level_dbm;
	uint16_t transmitter_id;
} dect_phy_ping_pdu_header_t;

typedef struct {
	dect_phy_ping_pdu_header_t header;
	dect_phy_ping_pdu_message_t message;
} dect_phy_ping_pdu_t;

#define DECT_PHY_PING_PDU_HEADER_LEN (sizeof(dect_phy_ping_pdu_header_t))

/**************************************************************************************************/

uint8_t *dect_phy_ping_pdu_encode(uint8_t *p_target, const dect_phy_ping_pdu_t *p_input);
int dect_phy_ping_pdu_decode(dect_phy_ping_pdu_t *p_target, const uint8_t *p_data);
void dect_phy_pdu_utils_ping_print(dect_phy_ping_pdu_t *ping_pdu);

#endif /* DECT_PHY_PING_PDU_H */
