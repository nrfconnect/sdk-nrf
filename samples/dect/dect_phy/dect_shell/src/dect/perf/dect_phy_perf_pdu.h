/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_PERF_PDU_H
#define DECT_PHY_PERF_PDU_H

#include <zephyr/kernel.h>
#include "dect_common.h"
#include "dect_common_pdu.h"

/**************************************************************************************************/

typedef enum {
	DECT_MAC_MESSAGE_TYPE_PERF_HARQ_FEEDBACK = 50,
	DECT_MAC_MESSAGE_TYPE_PERF_RESULTS_REQ,
	DECT_MAC_MESSAGE_TYPE_PERF_RESULTS_RESP,
	DECT_MAC_MESSAGE_TYPE_PERF_TX_DATA,
} dect_phy_perf_pdu_message_type_t;

/**************************************************************************************************/

#define DECT_PHY_PERF_PDU_COMMON_PART_LEN                                                         \
	(sizeof(dect_phy_perf_pdu_message_type_t) + sizeof(uint8_t) + sizeof(uint16_t))
#define DECT_PHY_PERF_TX_DATA_PDU_LEN_WITHOUT_PAYLOAD                                             \
	(DECT_PHY_PERF_PDU_COMMON_PART_LEN + (2 * sizeof(uint16_t)))
#define DECT_PHY_PERF_TX_DATA_PDU_PAYLOAD_MAX_LEN                                                 \
	(DECT_DATA_MAX_LEN - DECT_PHY_PERF_TX_DATA_PDU_LEN_WITHOUT_PAYLOAD)
#define DECT_PHY_PERF_RESULTS_REQ_LEN	    (DECT_PHY_PERF_PDU_COMMON_PART_LEN + sizeof(uint32_t))
#define DECT_PHY_PERF_RESULTS_DATA_MAX_LEN 491
typedef struct {
	char results_str[DECT_PHY_PERF_RESULTS_DATA_MAX_LEN];
} dect_phy_perf_pdu_results_resp_data;
typedef struct {
	uint32_t unused;
} dect_phy_perf_pdu_results_req;

typedef struct {
	uint16_t seq_nbr;
	uint16_t payload_length;
	uint8_t pdu_payload[DECT_PHY_PERF_TX_DATA_PDU_PAYLOAD_MAX_LEN];
} dect_phy_perf_pdu_tx_data;

typedef struct {
	uint8_t unused;
} dect_phy_perf_pdu_harq_feedback_t;

typedef union {
	dect_phy_perf_pdu_tx_data tx_data;
	dect_phy_perf_pdu_results_req results_req;
	dect_phy_perf_pdu_results_resp_data results;
	dect_phy_perf_pdu_harq_feedback_t harq_feedback;
} dect_phy_perf_pdu_message_t;

typedef struct {
	dect_phy_perf_pdu_message_type_t message_type; /* Note: only 7 bit */
	uint8_t padding;
	uint16_t transmitter_id;
} dect_phy_perf_pdu_header_t;

typedef struct {
	dect_phy_perf_pdu_header_t header;
	dect_phy_perf_pdu_message_t message;
} dect_phy_perf_pdu_t;

#define DECT_PHY_PERF_PDU_HEADER_LEN (sizeof(dect_phy_perf_pdu_header_t))

/**************************************************************************************************/

uint8_t *dect_phy_perf_pdu_encode(uint8_t *p_target, const dect_phy_perf_pdu_t *p_input);
int dect_phy_perf_pdu_decode(dect_phy_perf_pdu_t *p_target, const uint8_t *p_data);

#endif /* DECT_PHY_PERF_PDU_H */
