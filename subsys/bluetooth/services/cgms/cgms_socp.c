/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/types.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <bluetooth/services/cgms.h>

#include "cgms_internal.h"

LOG_MODULE_DECLARE(cgms, CONFIG_BT_CGMS_LOG_LEVEL);

#define CGMS_SOCP_LENGTH 20

/**@brief Specific Operation Control Point opcodes. */
enum socp_opcode {
	SOCP_OPCODE_RESERVED = 0x00,
	SOCP_WRITE_CGM_COMMUNICATION_INTERVAL = 0x01,
	SOCP_READ_CGM_COMMUNICATION_INTERVAL = 0x02,
	SOCP_READ_CGM_COMMUNICATION_INTERVAL_RESPONSE = 0x03,
	SOCP_WRITE_GLUCOSE_CALIBRATION_VALUE = 0x04,
	SOCP_READ_GLUCOSE_CALIBRATION_VALUE = 0x05,
	SOCP_READ_GLUCOSE_CALIBRATION_VALUE_RESPONSE = 0x06,
	SOCP_WRITE_PATIENT_HIGH_ALERT_LEVEL = 0x07,
	SOCP_READ_PATIENT_HIGH_ALERT_LEVEL = 0x08,
	SOCP_READ_PATIENT_HIGH_ALERT_LEVEL_RESPONSE = 0x09,
	SOCP_WRITE_PATIENT_LOW_ALERT_LEVEL = 0x0A,
	SOCP_READ_PATIENT_LOW_ALERT_LEVEL = 0x0B,
	SOCP_READ_PATIENT_LOW_ALERT_LEVEL_RESPONSE = 0x0C,
	SOCP_SET_HYPO_ALERT_LEVEL = 0x0D,
	SOCP_GET_HYPO_ALERT_LEVEL = 0x0E,
	SOCP_HYPO_ALERT_LEVEL_RESPONSE = 0x0F,
	SOCP_SET_HYPER_ALERT_LEVEL = 0x10,
	SOCP_GET_HYPER_ALERT_LEVEL = 0x11,
	SOCP_HYPER_ALERT_LEVEL_RESPONSE = 0x12,
	SOCP_SET_RATE_OF_DECREASE_ALERT_LEVEL = 0x13,
	SOCP_GET_RATE_OF_DECREASE_ALERT_LEVEL = 0x14,
	SOCP_RATE_OF_DECREASE_ALERT_LEVEL_RESPONSE = 0x15,
	SOCP_SET_RATE_OF_INCREASE_ALERT_LEVEL = 0x16,
	SOCP_GET_RATE_OF_INCREASE_ALERT_LEVEL = 0x17,
	SOCP_RATE_OF_INCREASE_ALERT_LEVEL_RESPONSE = 0x18,
	SOCP_RESET_DEVICE_SPECIFIC_ALERT = 0x19,
	SOCP_START_THE_SESSION = 0x1A,
	SOCP_STOP_THE_SESSION = 0x1B,
	SOCP_RESPONSE_CODE = 0x1C,
};

/**@brief Specific Operation Control Point response codes. */
enum socp_rsp_code {
	SOCP_RSP_RESERVED_FOR_FUTURE_USE = 0x00,
	SOCP_RSP_SUCCESS = 0x01,
	SOCP_RSP_OP_CODE_NOT_SUPPORTED = 0x02,
	SOCP_RSP_INVALID_OPERAND = 0x03,
	SOCP_RSP_PROCEDURE_NOT_COMPLETED = 0x04,
	SOCP_RSP_OUT_OF_RANGE = 0x05,
};

static int generic_handler(struct bt_conn *peer, struct cgms_server *instance,
		uint8_t req_opcode, uint8_t response_code)
{
	NET_BUF_SIMPLE_DEFINE(rsp, CGMS_SOCP_LENGTH);

	net_buf_simple_add_u8(&rsp, SOCP_RESPONSE_CODE);
	net_buf_simple_add_u8(&rsp, req_opcode);
	net_buf_simple_add_u8(&rsp, response_code);

	return cgms_socp_send_response(peer, &rsp);
}

static int write_cgms_communication_interval_handler(struct bt_conn *peer,
		struct cgms_server *instance, struct net_buf_simple *operand)
{
	uint8_t comm_interval_proposed;

	if (operand->len < 1) {
		return generic_handler(peer, instance, SOCP_WRITE_CGM_COMMUNICATION_INTERVAL,
					SOCP_RSP_INVALID_OPERAND);
	}

	comm_interval_proposed = net_buf_simple_pull_u8(operand);
	if (comm_interval_proposed > 0 &&
				comm_interval_proposed < instance->comm_interval_minimum) {
		return generic_handler(peer, instance, SOCP_WRITE_CGM_COMMUNICATION_INTERVAL,
					SOCP_RSP_OUT_OF_RANGE);
	}

	if (comm_interval_proposed == 0xFF) {
		instance->comm_interval = instance->comm_interval_minimum;
	} else {
		/* In case of comm_interval = 0, we just change the value to 0,
		 * and the peridoic notification task is responsible to examine
		 * and apply the value correctly.
		 */
		instance->comm_interval = comm_interval_proposed;
	}

	return generic_handler(peer, instance, SOCP_WRITE_CGM_COMMUNICATION_INTERVAL,
					SOCP_RSP_SUCCESS);
}

static int read_cgms_communication_interval_handler(struct bt_conn *peer,
		struct cgms_server *instance)
{
	NET_BUF_SIMPLE_DEFINE(rsp, CGMS_SOCP_LENGTH);

	net_buf_simple_add_u8(&rsp, SOCP_READ_CGM_COMMUNICATION_INTERVAL_RESPONSE);
	net_buf_simple_add_le16(&rsp, instance->comm_interval);

	return cgms_socp_send_response(peer, &rsp);
}

int cgms_socp_recv_request(struct bt_conn *peer, struct cgms_server *instance,
			const uint8_t *req_data, uint16_t req_len)
{
	int rc;
	enum socp_opcode opcode;
	struct net_buf_simple req;

	/* Early check of the data length. */
	if (!req_data || req_len == 0) {
		return -ENODATA;
	}

	net_buf_simple_init_with_data(&req, (void *)req_data, req_len);

	opcode = net_buf_simple_pull_u8(&req);
	if (opcode == SOCP_WRITE_CGM_COMMUNICATION_INTERVAL) {
		rc = write_cgms_communication_interval_handler(peer, instance, &req);
	} else if (opcode == SOCP_READ_CGM_COMMUNICATION_INTERVAL) {
		/* command does not require parameter */
		rc = read_cgms_communication_interval_handler(peer, instance);
	} else {
		rc = generic_handler(peer, instance,
			opcode, SOCP_RSP_OP_CODE_NOT_SUPPORTED);
	}

	return rc;
}
