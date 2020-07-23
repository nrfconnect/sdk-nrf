/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZIGBEE_CLI_ZCL_TYPES_H__
#define ZIGBEE_CLI_ZCL_TYPES_H__

#include <zboss_api.h>
#include <zboss_api_addons.h>
#include "zigbee_cli_cmd_zcl.h"


/* Payload size in bytes, payload read from string is twice the size. */
#define CMD_PAYLOAD_SIZE    25


/* Enum used to determine which command Write or Read Attribue to send. */
enum attr_req_type {
	ATTR_READ_REQUEST,
	ATTR_WRITE_REQUEST
};

/* Structure used to store Read/Write Attributes request data in the context manager entry. */
struct read_write_attr_req_data {
	zb_zcl_frame_direction_t direction;
	zb_uint8_t attr_type;
	zb_uint16_t attr_id;
	zb_uint8_t attr_value[32];
	enum attr_req_type req_type;
	struct zcl_packet_info packet_info;
};

/* Structure used to store Configure Reporting request data in the context manager entry. */
struct configure_reporting_req_data {
	zb_uint8_t attr_type;
	zb_uint16_t attr_id;
	zb_uint16_t interval_min;
	zb_uint16_t interval_max;
	struct zcl_packet_info packet_info;
};

/* Structure used to store generic ZCL command data in the context manager entry. */
struct generic_cmd_data {
	zb_zcl_disable_default_response_t def_resp;
	zb_uint8_t payload_length;
	zb_uint16_t cmd_id;
	zb_uint8_t payload[CMD_PAYLOAD_SIZE];
	struct zcl_packet_info packet_info;
};

#endif /* ZIGBEE_CLI_ZCL_TYPES_H__ */
