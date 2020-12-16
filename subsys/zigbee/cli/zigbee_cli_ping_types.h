/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZIGBEE_CLI_PING_H__
#define ZIGBEE_CLI_PING_H__

#include <shell/shell.h>

#include <zboss_api.h>
#include <zboss_api_addons.h>
#include "zigbee_cli_cmd_zcl.h"

#define PING_CUSTOM_CLUSTER           0xBEEF
#define PING_MAX_LENGTH               79
#define PING_ECHO_REQUEST             0x00
#define PING_ECHO_REPLY               0x01
#define PING_ECHO_NO_ACK_REQUEST      0x02
#define PING_NO_ECHO_REQUEST          0x03
#define PING_ECHO_REQUEST_BYTE        0xAB
#define PING_ECHO_REPLY_BYTE          0xCD
#define PING_ECHO_REQUEST_TIMEOUT_S   10

/**@brief  Ping event type. Informs what kind of acknowledgment was received.
 */
enum ping_time_evt {
	/* APS acknowledgment of PING request received. */
	PING_EVT_ACK_RECEIVED,
	/* PING reply received. */
	PING_EVT_ECHO_RECEIVED,
	/* PING reply or APS acknowledgment was not received
	 * within timeout time.
	 */
	PING_EVT_FRAME_TIMEOUT,
	/* PING request was successfully scheduled for sending by the stack. */
	PING_EVT_FRAME_SCHEDULED,
	/* PING request was successfully sent. This event occurrs only
	 * if both APS ACK was not requested.
	 */
	PING_EVT_FRAME_SENT,
	/* PING received was received. This event is passed only
	 * to the callback registered via @p zb_ping_set_ping_indication_cb.
	 */
	PING_EVT_REQUEST_RECEIVED,
	/* An error occurred during sending PING request. */
	PING_EVT_ERROR,
};

/**@brief  Ping event callback definition.
 *
 * @param[in] evt           Type of received  ping acknowledgment
 * @param[in] delay_ms      Time, in miliseconds, between ping request
 *                          and the event.
 * @param[in] entry         Pointer to context manager entry in which the ping
 *                          request data is stored.
 */
typedef void (*ping_time_cb_t)(enum ping_time_evt evt, zb_uint32_t delay_ms,
			     struct ctx_entry *entry);

/* Structure used to store PING request data in the context manager entry. */
struct ping_req_data {
	zb_uint8_t ping_seq;
	zb_uint8_t request_ack;
	zb_uint8_t request_echo;
	zb_uint16_t count;
	zb_uint16_t timeout_ms;
	ping_time_cb_t cb;
	volatile int64_t sent_time;
	struct zcl_packet_info packet_info;
};

/* Structure used to store PING reply data in the context manager entry. */
struct ping_reply_data {
	zb_uint8_t ping_seq;
	zb_uint8_t count;
	zb_uint8_t send_ack;
	struct zcl_packet_info packet_info;
};

/**@brief Set ping request indication callback.
 *
 * @note The @p cb argument delay_ms will reflect current time
 *       in miliseconds.
 */
void zb_ping_set_ping_indication_cb(ping_time_cb_t cb);

#endif /* ZIGBEE_CLI_PING_H__ */
