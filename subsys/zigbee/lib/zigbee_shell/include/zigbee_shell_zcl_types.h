/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZIGBEE_SHELL_ZCL_TYPES_H__
#define ZIGBEE_SHELL_ZCL_TYPES_H__

#include <zboss_api.h>
#include "zigbee_shell_ping_types.h"

/* Payload size in bytes, payload read from string is twice the size. */
#define CMD_PAYLOAD_SIZE    25


/* Structure used to pass information required to send ZCL frame. */
struct zcl_packet_info {
	zb_bufid_t buffer;
	zb_uint8_t *ptr;
	zb_addr_u dst_addr;
	zb_uint8_t dst_addr_mode;
	zb_uint8_t dst_ep;
	zb_uint8_t ep;
	zb_uint16_t prof_id;
	zb_uint16_t cluster_id;
	zb_callback_t cb;
	zb_bool_t disable_aps_ack;
};

/* Enum used to determine which command Write or Read Attribute to send. */
enum attr_req_type {
	ATTR_READ_REQUEST,
	ATTR_WRITE_REQUEST
};

/* Structure used to store Read/Write Attributes request data in the context manager entry. */
struct read_write_attr_req {
	zb_zcl_frame_direction_t direction;
	zb_uint8_t attr_type;
	zb_uint16_t attr_id;
	zb_uint8_t attr_value[32];
	enum attr_req_type req_type;
};

/* Structure used to store Configure Reporting request data in the context manager entry. */
struct configure_reporting_req {
	zb_uint8_t attr_type;
	zb_uint16_t attr_id;
	zb_uint16_t interval_min;
	zb_uint16_t interval_max;
};

/* Structure used to store generic ZCL command data in the context manager entry. */
struct generic_cmd {
	zb_zcl_disable_default_response_t def_resp;
	zb_uint8_t payload_length;
	zb_uint16_t cmd_id;
	zb_uint8_t payload[CMD_PAYLOAD_SIZE];
};

/* Structure used to store Groups commands data in the context manager entry. */
struct groups_cmd {
	zb_uint8_t group_list_cnt;
	zb_uint16_t group_id;
	zb_callback_t send_fn;
	zb_uint16_t group_list[7];
};

/* Structure used to store ZCL data in the context manager entry. */
struct zcl_data {
	/* Common ZCL packet information. */
	struct zcl_packet_info pkt_info;
	/* Union of structures used to store a ZCL commands related data,
	 * shared between the different commands.
	 */
	union {
		struct ping_req ping_req;
		struct ping_reply ping_reply;
		struct read_write_attr_req read_write_attr_req;
		struct configure_reporting_req configure_reporting_req;
		struct generic_cmd generic_cmd;
		struct groups_cmd groups_cmd;
	};
};

#endif /* ZIGBEE_SHELL_ZCL_TYPES_H__ */
