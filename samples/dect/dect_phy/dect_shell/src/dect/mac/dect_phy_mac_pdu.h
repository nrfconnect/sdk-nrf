/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_PHY_MAC_PDU_H
#define DECT_PHY_MAC_PDU_H

#include <zephyr/kernel.h>
#include "dect_common.h"

/******************************************************************************/

/* MAC spec: ETSI TS 103 636-4 v1.5.1 */

/* MAC spec: Table 6.3.2-1: MAC Security */
typedef enum {
	DECT_PHY_MAC_SECURITY_NOT_USED = 0,	/* 0b00 */
	DECT_PHY_MAC_SECURITY_USED = 1,		/* 0b01 */
	DECT_PHY_MAC_SECURITY_USED_WITH_IE = 2, /* 0b10 */
	DECT_PHY_MAC_SECURITY_RESERVED = 3,	/* 0b11 */
} dect_phy_mac_security_t;

/* MAC spec: Table 6.3.2-2: MAC header Type field */
typedef enum {
	DECT_PHY_MAC_HEADER_TYPE_PDU = 0,	/* 0b0000 */
	DECT_PHY_MAC_HEADER_TYPE_BEACON = 1,	/* 0b0001 */
	DECT_PHY_MAC_HEADER_TYPE_UNICAST = 2,	/* 0b0010 */
	DECT_PHY_MAC_HEADER_TYPE_BROADCAST = 3, /* 0b0011 */
	DECT_PHY_MAC_HEADER_ESCAPE = 15,	/* 0b1111 */
} dect_phy_mac_header_type_t;

/* MAC spec: 6.3.2, MAC Header type */
typedef struct {
	uint8_t version;
	uint8_t security;
	dect_phy_mac_header_type_t type;
} dect_phy_mac_type_header_t;

/* MAC spec: 6.3.3, MAC Common header */
typedef struct {
	dect_phy_mac_header_type_t type;
	uint16_t reset;
	uint16_t seq_nbr;

	uint32_t transmitter_id;
	uint32_t receiver_id;
	uint32_t nw_id;
} dect_phy_mac_common_header_t;

/* Defines for header sizes (type+common headers) */
#define DECT_PHY_MAC_PDU_HEADER_SIZE	   3
#define DECT_PHY_MAC_BEACON_HEADER_SIZE	   8
#define DECT_PHY_MAC_UNICAST_HEADER_SIZE   11
#define DECT_PHY_MAC_BROADCAST_HEADER_SIZE 7

/******************************************************************************/

/* MAC spec: Table 6.3.4-1: MAC extension field encoding */
typedef enum {
	DECT_PHY_MAC_EXT_NO_LENGTH = 0, /* 0b00 */
	DECT_PHY_MAC_EXT_8BIT_LEN = 1,	/* 0b01 */
	DECT_PHY_MAC_EXT_16BIT_LEN = 2, /* 0b10 */
	DECT_PHY_MAC_EXT_SHORT_IE = 3,	/* 0b11 */
} dect_phy_mac_ext_field_t;

typedef enum {
	/* MAC spec, Table 6.3.4-2: IE type field encoding
	 * for MAC Extension field encoding 00, 01, 10
	 */
	DECT_PHY_MAC_IE_TYPE_PADDING = 0,		       /* 0b000000 */
	DECT_PHY_MAC_IE_TYPE_HIGHER_LAYER_SIGNALING_FLOW1 = 1, /* 0b000001 */
	DECT_PHY_MAC_IE_TYPE_HIGHER_LAYER_SIGNALING_FLOW2 = 2, /* 0b000010 */
	DECT_PHY_MAC_IE_TYPE_USER_PLANE_DATA_FLOW1 = 3,	       /* 0b000011 */
	DECT_PHY_MAC_IE_TYPE_USER_PLANE_DATA_FLOW2 = 4,	       /* 0b000100 */
	DECT_PHY_MAC_IE_TYPE_USER_PLANE_DATA_FLOW3 = 5,	       /* 0b000101 */
	DECT_PHY_MAC_IE_TYPE_USER_PLANE_DATA_FLOW4 = 6,	       /* 0b000110 */
	/* Reserved */
	DECT_PHY_MAC_IE_TYPE_NETWORK_BEACON = 8,	     /* 0b001000 */
	DECT_PHY_MAC_IE_TYPE_CLUSTER_BEACON = 9,	     /* 0b001001 */
	DECT_PHY_MAC_IE_TYPE_ASSOCIATION_REQ = 10,	     /* 0b001010 */
	DECT_PHY_MAC_IE_TYPE_ASSOCIATION_RESP = 11,	     /* 0b001011 */
	DECT_PHY_MAC_IE_TYPE_ASSOCIATION_REL = 12,	     /* 0b001100 */
	DECT_PHY_MAC_IE_TYPE_RECONFIG_REQ = 13,		     /* 0b001101 */
	DECT_PHY_MAC_IE_TYPE_RECONFIG_RESP = 14,	     /* 0b001110 */
	DECT_PHY_MAC_IE_TYPE_ADDITIONAL_MAC_MESSAGES = 15,   /* 0b001111 */
	DECT_PHY_MAC_IE_TYPE_SECURITY_INFO_IE = 16,	     /* 0b010000 */
	DECT_PHY_MAC_IE_TYPE_ROUTE_INFO_IE = 17,	     /* 0b010001 */
	DECT_PHY_MAC_IE_TYPE_RESOURCE_ALLOCATION_IE = 18,    /* 0b010010 */
	DECT_PHY_MAC_IE_TYPE_RANDOM_ACCESS_RESOURCE_IE = 19, /* 0b010011 */
	DECT_PHY_MAC_IE_TYPE_RD_CAPABILITY_IE = 20,	     /* 0b010100 */
	DECT_PHY_MAC_IE_TYPE_NEIGHBOURING_IE = 21,	     /* 0b010101 */
	DECT_PHY_MAC_IE_TYPE_BROADCAST_IND_IE = 22,	     /* 0b010110 */
	DECT_PHY_MAC_IE_TYPE_GROUP_ASSIGNMENT_IE = 23,	     /* 0b010111 */
	DECT_PHY_MAC_IE_TYPE_LOAD_INFO_IE = 24,		     /* 0b011000 */
	DECT_PHY_MAC_IE_TYPE_MEASUREMENT_REPORT = 25,	     /* 0b011001 */
	/* Reserved */
	DECT_PHY_MAC_IE_TYPE_ESCAPE = 62,    /* 0b111110 */
	DECT_PHY_MAC_IE_TYPE_EXTENSION = 63, /* 0b111111 */

	/* MAC spec, Table 6.3.4-3: IE type field encoding
	 * for MAC extension field encoding 11 and payload length 0 byte
	 */
	DECT_PHY_MAC_IE_TYPE_0BYTE_PADDING = 0,	      /* 0b00000 */
	DECT_PHY_MAC_IE_TYPE_0BYTE_CONFIG_REQ_IE = 1, /* 0b00001 */
	DECT_PHY_MAC_IE_TYPE_0BYTE_KEEP_ALIVE_IE = 2, /* 0b00010 */
	/* Reserved */
	DECT_PHY_MAC_IE_TYPE_0BYTE_SECURITY_INFO_IE = 16, /* 0b10000 */
	DECT_PHY_MAC_IE_TYPE_0BYTE_ESCAPE = 30,		  /* 0b11110 */

	/* MAC spec, Table 6.3.4-4: IE type field encoding
	 * for MAC extension field encoding 11 and payload length 1 byte
	 */
	DECT_PHY_MAC_IE_TYPE_1BYTE_PADDING = 0,		    /* 0b00000 */
	DECT_PHY_MAC_IE_TYPE_1BYTE_RADIO_DEV_STATUS_IE = 1, /* 0b00001 */
	/* Reserved */
	DECT_PHY_MAC_IE_TYPE_1BYTE_ESCAPE = 30, /* 0b11110 */
} dect_phy_mac_ie_type_t;

/* DLC spec: Table 5.3.1-1: DLC IE Type coding */
typedef enum {
	DECT_PHY_MAC_DLC_IE_TYPE_SERV_0_WITH_ROUTING = 0,    /* 0b0000 */
	DECT_PHY_MAC_DLC_IE_TYPE_SERV_0_WITHOUT_ROUTING = 1, /* 0b0001 */
	DECT_PHY_MAC_DLC_IE_TYPE_SERV_1_WITH_ROUTING = 2,    /* 0b0010 */
	DECT_PHY_MAC_DLC_IE_TYPE_SERV_1_WITHOUT_ROUTING = 3, /* 0b0011 */
	DECT_PHY_MAC_DLC_IE_TYPE_TIMERS_CONF = 4,	     /* 0b0100 */
	DECT_PHY_MAC_DLC_IE_TYPE_ESCAPE = 14,		     /* 0b1110 */
} dect_phy_dlc_ie_type_t;
#define DECT_PHY_MAC_DLC_IE_TYPE_SERV_0_WITHOUT_ROUTING_LEN (1)

/* MAC spec: ch. 6.3.4: MAC multiplexing header */
typedef struct {
	dect_phy_mac_ext_field_t mac_ext;
	dect_phy_mac_ie_type_t ie_type;
	uint8_t ie_ext; /* Used only when mac_ext = b10 (option 'f') */
	uint16_t payload_length;

	const uint8_t *payload_ptr;
} dect_phy_mac_mux_header_t;

/******************************************************************************/

/* MAC spec: ch 6.4.2: MAC messages (set according to mux_header.ie_type )
 * Only supported MAC messages and IEs.
 */
typedef enum {
	DECT_PHY_MAC_MESSAGE_TYPE_DATA_SDU,
	DECT_PHY_MAC_MESSAGE_TYPE_CLUSTER_BEACON,
	DECT_PHY_MAC_MESSAGE_TYPE_ASSOCIATION_REQ,
	DECT_PHY_MAC_MESSAGE_TYPE_ASSOCIATION_RESP,
	DECT_PHY_MAC_MESSAGE_TYPE_ASSOCIATION_REL,

	DECT_PHY_MAC_MESSAGE_RANDOM_ACCESS_RESOURCE_IE,
	DECT_PHY_MAC_MESSAGE_ESCAPE,

	DECT_PHY_MAC_MESSAGE_PADDING,

	DECT_PHY_MAC_MESSAGE_TYPE_NONE,
} dect_phy_mac_message_type_t;

/* MAC spec: Table 6.4.2.2-1: Network Beacon period */
typedef enum {
	DECT_PHY_MAC_NW_BEACON_PERIOD_50MS = 0,
	DECT_PHY_MAC_NW_BEACON_PERIOD_100MS = 1,
	DECT_PHY_MAC_NW_BEACON_PERIOD_500MS = 2,
	DECT_PHY_MAC_NW_BEACON_PERIOD_1000MS = 3,
	DECT_PHY_MAC_NW_BEACON_PERIOD_1500MS = 4,
	DECT_PHY_MAC_NW_BEACON_PERIOD_2000MS = 5,
	DECT_PHY_MAC_NW_BEACON_PERIOD_4000MS = 6,
	/* rest are reserved */
} dect_phy_mac_nw_beacon_period_t;

/* MAC spec: Table 6.4.2.2-1: Cluster Beacon period */
typedef enum {
	DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_10MS = 0,
	DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_50MS = 1,
	DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_100MS = 2,
	DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_500MS = 3,
	DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_1000MS = 4,
	DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_1500MS = 5,
	DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_2000MS = 6,
	DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_4000MS = 7,
	DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_8000MS = 8,
	DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_16000MS = 9,
	DECT_PHY_MAC_CLUSTER_BEACON_PERIOD_32000MS = 10,
	/* rest are reserved */
} dect_phy_mac_cluster_beacon_period_t;

#define DECT_PHY_MAC_CLUSTER_BEACON_MIN_LEN (4)
typedef struct {
	uint8_t system_frame_number;
	uint8_t tx_pwr_bit;
	uint8_t pwr_const_bit;
	uint8_t frame_offset_bit;
	uint8_t next_channel_bit;
	uint8_t time_to_next_next;

	dect_phy_mac_nw_beacon_period_t nw_beacon_period;
	dect_phy_mac_cluster_beacon_period_t cluster_beacon_period;
	uint8_t count_to_trigger : 4, /* coded value */
		relative_quality : 2, /* coded value */
		min_quality : 2	     /* coded value */
		;

	uint8_t max_phy_tx_power; /* MAC spec: Table 6.2.1-3b */
	uint16_t frame_offset;
	uint16_t next_cluster_channel;
	uint32_t time_to_next;
} dect_phy_mac_cluster_beacon_t;

/**************************************************************************************************/

/* MAC spec: Table 6.4.2.4-2: Association Setup Cause IE */

typedef enum {
	DECT_PHY_MAC_ASSOCIATION_SETUP_CAUSE_INIT = 0,		    /* 000 */
	DECT_PHY_MAC_ASSOCIATION_SETUP_CAUSE_NEW_SET_OF_FLOWS = 1,  /* 001 */
	DECT_PHY_MAC_ASSOCIATION_SETUP_CAUSE_MOBILITY = 2,	    /* 010 */
	DECT_PHY_MAC_ASSOCIATION_SETUP_CAUSE_AFTER_ERROR = 3,	    /* 011 */
	DECT_PHY_MAC_ASSOCIATION_SETUP_CAUSE_CHANGE_OP_CHANNEL = 4, /* 100 */
	DECT_PHY_MAC_ASSOCIATION_SETUP_CAUSE_CHANGE_OP_MODE = 5,    /* 101 */
	DECT_PHY_MAC_ASSOCIATIONP_CAUSE_PAGING_RESP = 6,	    /* 110 */
								    /* the rest are reserved */
} dect_phy_mac_association_setup_cause_ie_t;

#define DECT_PHY_MAC_ASSOCIATION_REQ_MIN_LEN (5)
typedef struct {
	uint8_t pwr_const_bit;
	uint8_t ft_mode_bit;
	uint8_t current_cluster_channel_bit;

	dect_phy_mac_association_setup_cause_ie_t setup_cause;
	uint8_t flow_count;

	uint8_t harq_tx_process_count;
	uint8_t max_harq_tx_retransmission_delay;
	uint8_t harq_rx_process_count;
	uint8_t max_harq_rx_retransmission_delay;

	uint8_t flow_id; /* Only one flow supported */

	dect_phy_mac_nw_beacon_period_t nw_beacon_period;
	dect_phy_mac_cluster_beacon_period_t cluster_beacon_period;

	uint16_t next_cluster_channel;
	uint32_t time_to_next;
	uint16_t current_cluster_channel;
} dect_phy_mac_association_req_t;

/* MAC spec: Table 6.4.2.5-2: Reject Cause and Reject Timer */
typedef enum {
	DECT_PHY_MAC_ASSOCIATION_REJ_CAUSE_NO_RADIO_CAPACITY = 0,
	DECT_PHY_MAC_ASSOCIATION_REJ_CAUSE_NO_HW_CAPACITY = 1,
	DECT_PHY_MAC_ASSOCIATION_REJ_CAUSE_CONFLICTED_SHORT_ID = 2,
	DECT_PHY_MAC_ASSOCIATION_REJ_CAUSE_SECURITY_NEEDED = 3,
	DECT_PHY_MAC_ASSOCIATION_REJ_CAUSE_OTHER = 4,
	/* the rest are reserved */
} dect_phy_mac_association_rej_cause_t;

/* MAC spec: Table 6.4.2.6-1: Association Release Cause */
typedef enum {
	DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_CONN_TERMINATION = 0,
	DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_MOBILITY,
	DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_LONG_INACTIVITY,
	DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_INCOMPATIBLE_CONFIG,
	DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_NO_HW_RESOURCES,
	DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_NO_RADIO_RESOURCES,
	DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_BAD_RADIO_QUALITY,
	DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_SECURITY_ERROR,
	DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_PT_SIDE_SHORT_RD_ID_CONFLICT,
	DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_FT_SIDE_SHORT_RD_ID_CONFLICT,
	DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_NOT_ASOCIATED_PT,
	DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_NOT_ASOCIATED_FT,
	DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_NOT_IN_FT_MODE,
	DECT_PHY_MAC_ASSOCIATION_REL_CAUSE_OTHER_ERROR,
	/* the rest are reserved */
} dect_phy_mac_association_rel_cause_t;

typedef enum {
	DECT_PHY_MAC_ASSOCIATION_REJ_TIME_0s = 0,
	DECT_PHY_MAC_ASSOCIATION_REJ_TIME_5s = 1,
	DECT_PHY_MAC_ASSOCIATION_REJ_TIME_10s = 2,
	DECT_PHY_MAC_ASSOCIATION_REJ_TIME_30s = 3,
	DECT_PHY_MAC_ASSOCIATION_REJ_TIME_60s = 4,
	DECT_PHY_MAC_ASSOCIATION_REJ_TIME_120s = 5,
	DECT_PHY_MAC_ASSOCIATION_REJ_TIME_180s = 6,
	DECT_PHY_MAC_ASSOCIATION_REJ_TIME_300s = 7,
	DECT_PHY_MAC_ASSOCIATION_REJ_TIME_600s = 8,
	/* the rest are reserved */
} dect_phy_mac_association_reject_time_t;

#define DECT_PHY_MAC_ASSOCIATION_RESP_MIN_LEN (2)

typedef struct {
	uint8_t ack_bit;
	uint8_t harq_conf_bit;
	uint8_t group_bit;

	uint8_t flow_count; /* 111, All flows accepted as configured in association request */
	dect_phy_mac_association_rej_cause_t reject_cause;
	dect_phy_mac_association_reject_time_t reject_time;

	uint8_t harq_rx_process_count;
	uint8_t max_harq_rx_retransmission_delay;
	uint8_t harq_tx_process_count;
	uint8_t max_harq_tx_retransmission_delay;

	uint8_t flow_id[1];

	uint8_t group_id;
	uint8_t resource_tag;
} dect_phy_mac_association_resp_t;

#define DECT_PHY_MAC_ASSOCIATION_REL_LEN (1)
typedef struct {
	dect_phy_mac_association_rel_cause_t rel_cause;
} dect_phy_mac_association_rel_t;

/**************************************************************************************************/
typedef struct {
	uint8_t dlc_ie_type; /* DLC spec: Table 5.3.1-1 */
	uint16_t data_length;
	uint8_t data[DECT_DATA_MAX_LEN];
} dect_phy_mac_data_sdu_t;

typedef enum {
	DECT_PHY_MAC_RA_REPEAT_TYPE_SINGLE = 0,
	DECT_PHY_MAC_RA_REPEAT_TYPE_FRAMES = 1,
	DECT_PHY_MAC_RA_REPEAT_TYPE_SUBSLOTS = 2,
	DECT_PHY_MAC_RA_REPEAT_TYPE_RESERVED
} dect_phy_mac_rach_repeat_type_t;

#define DECT_PHY_MAC_RANDOM_ACCESS_RES_IE_MIN_LEN 5

/* MAC spec: 6.4.3.4, Random Access Resource IE */
typedef struct {
	dect_phy_mac_rach_repeat_type_t repeat;
	bool sfn_included;
	bool channel_included;
	bool channel2_included;

	uint8_t start_subslot;

	enum dect_phy_packet_length_type length_type;
	uint8_t length;

	enum dect_phy_packet_length_type max_rach_length_type;
	uint8_t max_rach_length;
	uint8_t cw_min_sig;

	uint8_t dect_delay;
	uint8_t response_win;
	uint8_t cw_max_sig;

	uint8_t repetition;
	uint8_t validity;
	uint8_t system_frame_number;
	uint16_t channel1;
	uint16_t channel2;
} dect_phy_mac_random_access_resource_ie_t;

typedef struct {
	uint16_t data_length;
	uint8_t data[DECT_DATA_MAX_LEN];
} dect_phy_mac_common_sdu_t;

typedef union {
	/* Only supported MAC messages and IEs */
	dect_phy_mac_data_sdu_t data_sdu;
	dect_phy_mac_cluster_beacon_t cluster_beacon;
	dect_phy_mac_random_access_resource_ie_t rach_ie;
	dect_phy_mac_association_req_t association_req;
	dect_phy_mac_association_resp_t association_resp;
	dect_phy_mac_association_rel_t association_rel;

	/* A container for others */
	dect_phy_mac_common_sdu_t common_msg;
} dect_phy_mac_message_t;

typedef struct {
	sys_dnode_t dnode;

	dect_phy_mac_mux_header_t mux_header;

	dect_phy_mac_message_type_t message_type;
	dect_phy_mac_message_t message;
} dect_phy_mac_sdu_t;

/**************************************************************************************************/

const char *dect_phy_mac_pdu_header_type_to_string(int type, char *out_str_buff);
const char *dect_phy_mac_pdu_security_to_string(int type, char *out_str_buff);

bool dect_phy_mac_pdu_header_type_value_valid(uint8_t type_value);
bool dect_phy_mac_pdu_type_header_decode(uint8_t *header_ptr, uint32_t data_len,
					 dect_phy_mac_type_header_t *type_header);
uint8_t *dect_phy_mac_pdu_type_header_encode(dect_phy_mac_type_header_t *type_header_in,
					     uint8_t *target_ptr);

bool dect_phy_mac_pdu_ie_type_value_0byte_len_valid(uint8_t type_value);
bool dect_phy_mac_pdu_ie_type_value_1byte_len_valid(uint8_t type_value);
const char *dect_phy_mac_pdu_ie_type_to_string(dect_phy_mac_ext_field_t mac_ext, int len, int type,
					       char *out_str_buff);
const char *dect_phy_mac_dlc_pdu_ie_type_string_get(int type);

bool dect_phy_mac_pdu_common_header_decode(const uint8_t *header_ptr, uint32_t data_len,
					   dect_phy_mac_type_header_t *type_header_in,
					   dect_phy_mac_common_header_t *common_header_out);
uint8_t *dect_phy_mac_pdu_common_header_encode(dect_phy_mac_common_header_t *common_header_in,
					       uint8_t *target_ptr);

uint8_t dect_phy_mac_pdy_type_n_common_header_len_get(dect_phy_mac_header_type_t type);

bool dect_phy_mac_pdu_mux_header_decode(uint8_t *header_ptr, uint32_t data_len,
					dect_phy_mac_mux_header_t *mux_header_out);
uint8_t *dect_phy_mac_mux_header_header_encode(dect_phy_mac_mux_header_t *mux_header_in,
					       uint8_t *target_ptr);
uint8_t dect_phy_mac_pdu_mux_header_length_get(dect_phy_mac_mux_header_t *header_ptr);

bool dect_phy_mac_pdu_sdu_cluster_beacon_decode(const uint8_t *payload_ptr, uint32_t payload_len,
						dect_phy_mac_cluster_beacon_t *cluster_beacon_out);
uint8_t *
dect_phy_mac_pdu_sdu_cluster_beacon_encode(const dect_phy_mac_cluster_beacon_t *cluster_beacon_in,
					   uint8_t *target_ptr);

bool dect_phy_mac_pdu_sdu_association_req_decode(const uint8_t *payload_ptr, uint32_t payload_len,
						 dect_phy_mac_association_req_t *req_out);
uint8_t *dect_phy_mac_pdu_sdu_association_req_encode(const dect_phy_mac_association_req_t *req_in,
						     uint8_t *target_ptr);
uint8_t *dect_phy_mac_pdu_sdu_association_rel_encode(const dect_phy_mac_association_rel_t *req_in,
						     uint8_t *target_ptr);
bool dect_phy_mac_pdu_sdu_association_rel_decode(const uint8_t *payload_ptr, uint32_t payload_len,
						 dect_phy_mac_association_rel_t *req_out);

bool dect_phy_mac_pdu_sdu_association_resp_decode(const uint8_t *payload_ptr, uint32_t payload_len,
						  dect_phy_mac_association_resp_t *resp_out);
uint8_t *
dect_phy_mac_pdu_sdu_association_resp_encode(const dect_phy_mac_association_resp_t *resp_in,
					     uint8_t *target_ptr);

bool dect_phy_mac_pdu_sdus_decode(uint8_t *payload_ptr, uint32_t payload_len,
				  sys_dlist_t *sdu_list);
uint8_t *dect_phy_mac_pdu_sdus_encode(uint8_t *target_ptr, sys_dlist_t *sdu_input_list);

int dect_phy_mac_pdu_nw_beacon_period_in_ms(dect_phy_mac_nw_beacon_period_t period);
int dect_phy_mac_pdu_cluster_beacon_period_in_ms(dect_phy_mac_cluster_beacon_period_t period);
const char *
dect_phy_mac_pdu_cluster_beacon_repeat_string_get(dect_phy_mac_rach_repeat_type_t repeat);
const char *dect_phy_mac_pdu_association_req_setup_cause_string_get(
	dect_phy_mac_association_setup_cause_ie_t setup_cause);
const char *
dect_phy_mac_pdu_association_rel_cause_string_get(dect_phy_mac_association_rel_cause_t rel_cause);

int dect_phy_mac_pdu_sdu_list_add_padding(
	uint8_t **pdu_ptr, sys_dlist_t *sdu_list, uint8_t padding_len);

#endif /* DECT_PHY_MAC_PDU_H */
