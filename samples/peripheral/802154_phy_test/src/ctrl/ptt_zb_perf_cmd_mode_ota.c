/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: OTA commands processing of Zigbee RF Performance Test Plan CMD mode */

/* The module processes an event containing OTA command from UART module and
 * returns the event with result. The event locks the module as currently processed OTA command
 * and the module rejects any other commands while processing the one.
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "ctrl/ptt_trace.h"
#include "rf/ptt_rf.h"

#include "ptt_proto.h"

#include "ptt_ctrl_internal.h"
#include "ptt_events_internal.h"
#include "ptt_modes.h"
#include "ptt_zb_perf_cmd_mode.h"

#ifdef TESTS
#include "test_cmd_ota_conf.h"
#endif

/** currently processed OTA command */
static ptt_evt_id_t ota_cmd_evt;

/* current event handlers */
static enum ptt_ret cmd_ota_cmd_proc(void);
static enum ptt_ret cmd_set_channel(void);
static enum ptt_ret cmd_set_power(void);
static enum ptt_ret cmd_ping(void);
static void cmd_ping_sent(void);
static enum ptt_ret cmd_get_power(void);
static void cmd_get_power_sent(void);
static enum ptt_ret cmd_end_rx_test(void);
static void cmd_end_rx_test_sent(void);
static enum ptt_ret cmd_get_hardware_version(void);
static void cmd_get_hardware_sent(void);
static enum ptt_ret cmd_get_software_version(void);
static void cmd_get_software_sent(void);
static enum ptt_ret cmd_stream(void);
static enum ptt_ret cmd_start_rx_test(void);
static enum ptt_ret cmd_set_antenna(void);
static enum ptt_ret cmd_set_tx_antenna(void);
static enum ptt_ret cmd_set_rx_antenna(void);
static enum ptt_ret cmd_get_rx_antenna(void);
static enum ptt_ret cmd_get_tx_antenna(void);
static enum ptt_ret cmd_get_last_best_rx_antenna(void);
static void cmd_get_antenna_sent(void);

/* timer handlers */
static void cmd_ping_timeout(ptt_evt_id_t timer_evt_id);
static void cmd_get_power_timeout(ptt_evt_id_t timer_evt_id);
static void cmd_end_rx_test_timeout(ptt_evt_id_t timer_evt_id);
static void cmd_get_hardware_timeout(ptt_evt_id_t timer_evt_id);
static void cmd_get_software_timeout(ptt_evt_id_t timer_evt_id);
static void cmd_get_antenna_timeout(ptt_evt_id_t timer_evt_id);

/* new rf packets processing */
static void cmd_ping_ack(ptt_evt_id_t new_rf_pkt_evt);
static void cmd_get_power_response(ptt_evt_id_t new_rf_pkt_evt);
static void cmd_end_rx_test_report(ptt_evt_id_t new_rf_pkt_evt);
static void cmd_get_hardware_response(ptt_evt_id_t new_rf_pkt_evt);
static void cmd_get_software_response(ptt_evt_id_t new_rf_pkt_evt);
static void cmd_get_antenna_response(ptt_evt_id_t new_rf_pkt_evt);

/* notifications for UART command processing */
static void cmd_fill_ctx_end_and_reset(const uint8_t *data, ptt_pkt_len_t len);
static void cmd_send_finish_and_reset(void);
static void cmd_send_timeout_and_reset(void);

static enum ptt_ret cmd_make_and_send_rf_packet(enum ptt_cmd cmd);

/* OTA command lock routines */
static inline void cmd_ota_cmd_lock(ptt_evt_id_t new_ota_cmd);
static inline void cmd_change_ota_cmd_state(ptt_evt_state_t state);
static inline ptt_evt_id_t cmd_get_ota_cmd_and_reset(void);
static inline bool cmd_is_ota_cmd_locked(void);
static inline bool cmd_is_ota_cmd_locked_by(ptt_evt_id_t evt_id);
static void cmd_ota_cmd_unlock(void);

#ifdef TESTS
#include "test_cmd_ota_wrappers.c"
#endif

void cmd_ota_cmd_init(void)
{
	cmd_ota_cmd_unlock();
}

void cmd_ota_cmd_uninit(void)
{
	cmd_ota_cmd_unlock();
}

enum ptt_ret cmd_ota_cmd_process(ptt_evt_id_t new_ota_cmd)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;

	if (false == cmd_is_ota_cmd_locked()) {
		/* store event as currently processed OTA command */
		cmd_ota_cmd_lock(new_ota_cmd);
		ret = cmd_ota_cmd_proc();
		if (ret != PTT_RET_SUCCESS) {
			cmd_ota_cmd_unlock();
		}
	} else {
		PTT_TRACE("%s: state isn't idle cmd %d state %d\n", __func__,
			  ptt_event_get_cmd(ota_cmd_evt), ptt_event_get_state(ota_cmd_evt));
		ret = PTT_RET_BUSY;
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_ota_cmd_proc(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;
	enum ptt_cmd cmd = ptt_event_get_cmd(ota_cmd_evt);

	PTT_TRACE("%s: cmd %d\n", __func__, cmd);

	switch (ptt_event_get_cmd(ota_cmd_evt)) {
	case PTT_CMD_PING:
		ret = cmd_ping();
		break;

	case PTT_CMD_SET_CHANNEL:
		ret = cmd_set_channel();
		break;

	case PTT_CMD_SET_POWER:
		ret = cmd_set_power();
		break;

	case PTT_CMD_GET_POWER:
		ret = cmd_get_power();
		break;

	case PTT_CMD_STREAM:
		ret = cmd_stream();
		break;

	case PTT_CMD_START_RX_TEST:
		ret = cmd_start_rx_test();
		break;

	case PTT_CMD_END_RX_TEST:
		ret = cmd_end_rx_test();
		break;

	case PTT_CMD_GET_HARDWARE_VERSION:
		ret = cmd_get_hardware_version();
		break;

	case PTT_CMD_GET_SOFTWARE_VERSION:
		ret = cmd_get_software_version();
		break;

	case PTT_CMD_SET_ANTENNA:
		ret = cmd_set_antenna();
		break;

	case PTT_CMD_SET_TX_ANTENNA:
		ret = cmd_set_tx_antenna();
		break;

	case PTT_CMD_SET_RX_ANTENNA:
		ret = cmd_set_rx_antenna();
		break;

	case PTT_CMD_GET_RX_ANTENNA:
		ret = cmd_get_rx_antenna();
		break;

	case PTT_CMD_GET_TX_ANTENNA:
		ret = cmd_get_tx_antenna();
		break;

	case PTT_CMD_GET_LAST_BEST_RX_ANTENNA:
		ret = cmd_get_last_best_rx_antenna();
		break;

	default:
		PTT_TRACE("%s: unknown command cmd %d\n", __func__, cmd);
		ret = PTT_RET_INVALID_COMMAND;
		break;
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

void cmd_ota_rf_tx_finished(ptt_evt_id_t evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

	assert(true == cmd_is_ota_cmd_locked_by(evt_id));

	PTT_TRACE("%s: cmd %d state %d\n", __func__, ptt_event_get_cmd(ota_cmd_evt),
		  ptt_event_get_state(ota_cmd_evt));

	switch (ptt_event_get_state(ota_cmd_evt)) {
	case CMD_OTA_STATE_PING_SENDING:
		cmd_ping_sent();
		break;

	case CMD_OTA_STATE_SET_CHANNEL_SENDING:
	case CMD_OTA_STATE_SET_POWER_SENDING:
	case CMD_OTA_STATE_STREAM_SENDING:
	case CMD_OTA_STATE_START_RX_TEST_SENDING:
	case CMD_OTA_STATE_SET_ANTENNA_SENDING:
		cmd_send_finish_and_reset();
		break;

	case CMD_OTA_STATE_GET_POWER_SENDING:
		cmd_get_power_sent();
		break;

	case CMD_OTA_STATE_END_RX_TEST_SENDING:
		cmd_end_rx_test_sent();
		break;

	case CMD_OTA_STATE_GET_HARDWARE_SENDING:
		cmd_get_hardware_sent();
		break;

	case CMD_OTA_STATE_GET_SOFTWARE_SENDING:
		cmd_get_software_sent();
		break;

	case CMD_OTA_STATE_GET_ANTENNA_SENDING:
		cmd_get_antenna_sent();
		break;

	default:
		PTT_TRACE("%s: inappropriate state cmd %d state %d\n", __func__,
			  ptt_event_get_cmd(ota_cmd_evt), ptt_event_get_state(ota_cmd_evt));
		break;
	}

	PTT_TRACE_FUNC_EXIT();
}

void cmd_ota_rf_tx_failed(ptt_evt_id_t evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

	assert(true == cmd_is_ota_cmd_locked_by(evt_id));

	PTT_TRACE("%s: cmd %d state %d\n", __func__, ptt_event_get_cmd(ota_cmd_evt),
		  ptt_event_get_state(ota_cmd_evt));

	switch (ptt_event_get_state(ota_cmd_evt)) {
	case CMD_OTA_STATE_PING_SENDING:
	case CMD_OTA_STATE_SET_CHANNEL_SENDING:
	case CMD_OTA_STATE_SET_POWER_SENDING:
	case CMD_OTA_STATE_GET_POWER_SENDING:
	case CMD_OTA_STATE_STREAM_SENDING:
	case CMD_OTA_STATE_START_RX_TEST_SENDING:
	case CMD_OTA_STATE_END_RX_TEST_SENDING:
	case CMD_OTA_STATE_GET_HARDWARE_SENDING:
	case CMD_OTA_STATE_GET_SOFTWARE_SENDING:
	case CMD_OTA_STATE_SET_ANTENNA_SENDING:
	case CMD_OTA_STATE_GET_ANTENNA_SENDING:
		cmd_send_timeout_and_reset();
		break;

	default:
		PTT_TRACE("%s: inappropriate state cmd %d state %d\n", __func__,
			  ptt_event_get_cmd(ota_cmd_evt), ptt_event_get_state(ota_cmd_evt));
		break;
	}

	PTT_TRACE_FUNC_EXIT();
}

void cmd_ota_rf_rx_done(ptt_evt_id_t new_rf_pkt_evt)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(new_rf_pkt_evt);

	if (false == cmd_is_ota_cmd_locked()) {
		PTT_TRACE("%s unexpected packet for idle state ignored\n", __func__);
	} else {
		struct ptt_evt_data_s *evt_data = ptt_event_get_data(new_rf_pkt_evt);

		assert(evt_data != NULL);

		if (!ptt_proto_check_packet(evt_data->arr, evt_data->len)) {
			PTT_TRACE("%s not protocol packet received and ignored\n", __func__);
		} else {
			PTT_TRACE("%s: protocol packet received. Current cmd %d state %d\n",
				  __func__, ptt_event_get_cmd(ota_cmd_evt),
				  ptt_event_get_state(ota_cmd_evt));

			ptt_event_set_cmd(new_rf_pkt_evt, evt_data->arr[PTT_CMD_CODE_START]);

			switch (ptt_event_get_state(ota_cmd_evt)) {
			case CMD_OTA_STATE_PING_WAITING_FOR_ACK:
				cmd_ping_ack(new_rf_pkt_evt);
				break;

			case CMD_OTA_STATE_GET_POWER_WAITING_FOR_RSP:
				cmd_get_power_response(new_rf_pkt_evt);
				break;

			case CMD_OTA_STATE_END_RX_TEST_WAITING_FOR_REPORT:
				cmd_end_rx_test_report(new_rf_pkt_evt);
				break;

			case CMD_OTA_STATE_GET_HARDWARE_WAITING_FOR_RSP:
				cmd_get_hardware_response(new_rf_pkt_evt);
				break;

			case CMD_OTA_STATE_GET_SOFTWARE_WAITING_FOR_RSP:
				cmd_get_software_response(new_rf_pkt_evt);
				break;

			case CMD_OTA_STATE_GET_ANTENNA_WAITING_FOR_RSP:
				cmd_get_antenna_response(new_rf_pkt_evt);
				break;

			default:
				PTT_TRACE("%s: inappropriate state cmd %d state %d\n", __func__,
					  ptt_event_get_cmd(ota_cmd_evt),
					  ptt_event_get_state(ota_cmd_evt));
				break;
			}
		}
	}

	PTT_TRACE_FUNC_EXIT();
}

static enum ptt_ret cmd_ping(void)
{
	PTT_TRACE_FUNC_ENTER();

	cmd_change_ota_cmd_state(CMD_OTA_STATE_PING_SENDING);
	enum ptt_ret ret = cmd_make_and_send_rf_packet(PTT_CMD_PING);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static void cmd_ping_timeout(ptt_evt_id_t timer_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(timer_evt_id);

	assert(true == cmd_is_ota_cmd_locked_by(timer_evt_id));

	cmd_send_timeout_and_reset();

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_ping_sent(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = ptt_timer_add(ptt_ctrl_get_rsp_timeout(), cmd_ping_timeout, ota_cmd_evt);

	if (ret == PTT_RET_SUCCESS) {
		cmd_change_ota_cmd_state(CMD_OTA_STATE_PING_WAITING_FOR_ACK);
	} else {
		PTT_TRACE("%s: add_timer return error code: %d\n", __func__, ret);
		cmd_send_timeout_and_reset();
	}

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_ping_ack(ptt_evt_id_t new_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(new_evt_id);

	if (ptt_event_get_cmd(new_evt_id) == PTT_CMD_ACK) {
		ptt_timer_remove(ota_cmd_evt);

		uint8_t result = 1;

		cmd_fill_ctx_end_and_reset(&result, sizeof(result));
	} else {
		PTT_TRACE("%s: command is not ACK\n", __func__);
	}

	PTT_TRACE_FUNC_EXIT();
}

static enum ptt_ret cmd_set_channel(void)
{
	PTT_TRACE_FUNC_ENTER();

	cmd_change_ota_cmd_state(CMD_OTA_STATE_SET_CHANNEL_SENDING);
	enum ptt_ret ret = cmd_make_and_send_rf_packet(PTT_CMD_SET_CHANNEL);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_set_power(void)
{
	PTT_TRACE_FUNC_ENTER();

	cmd_change_ota_cmd_state(CMD_OTA_STATE_SET_POWER_SENDING);
	enum ptt_ret ret = cmd_make_and_send_rf_packet(PTT_CMD_SET_POWER);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_get_power(void)
{
	PTT_TRACE_FUNC_ENTER();

	cmd_change_ota_cmd_state(CMD_OTA_STATE_GET_POWER_SENDING);
	enum ptt_ret ret = cmd_make_and_send_rf_packet(PTT_CMD_GET_POWER);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static void cmd_get_power_timeout(ptt_evt_id_t timer_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(timer_evt_id);

	assert(true == cmd_is_ota_cmd_locked_by(timer_evt_id));

	cmd_send_timeout_and_reset();

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_get_power_sent(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret =
		ptt_timer_add(ptt_ctrl_get_rsp_timeout(), cmd_get_power_timeout, ota_cmd_evt);

	if (ret == PTT_RET_SUCCESS) {
		cmd_change_ota_cmd_state(CMD_OTA_STATE_GET_POWER_WAITING_FOR_RSP);
	} else {
		PTT_TRACE("%s: add_timer return error code: %d\n", __func__, ret);
		cmd_send_timeout_and_reset();
	}

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_get_power_response(ptt_evt_id_t new_rf_pkt_evt)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(new_rf_pkt_evt);

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(new_rf_pkt_evt);

	assert(evt_data != NULL);

	if ((ptt_event_get_cmd(new_rf_pkt_evt) == PTT_CMD_GET_POWER_RESPONSE) &&
	    ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_GET_POWER) == evt_data->len)) {
		ptt_timer_remove(ota_cmd_evt);

		cmd_fill_ctx_end_and_reset(&evt_data->arr[PTT_PAYLOAD_START],
					   PTT_PAYLOAD_LEN_GET_POWER);
	} else {
		PTT_TRACE("%s: invalid length %d\n", __func__, evt_data->len);
	}

	PTT_TRACE_FUNC_EXIT();
}

static enum ptt_ret cmd_set_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	cmd_change_ota_cmd_state(CMD_OTA_STATE_SET_ANTENNA_SENDING);
	enum ptt_ret ret = cmd_make_and_send_rf_packet(PTT_CMD_SET_ANTENNA);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_set_tx_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	cmd_change_ota_cmd_state(CMD_OTA_STATE_SET_ANTENNA_SENDING);
	enum ptt_ret ret = cmd_make_and_send_rf_packet(PTT_CMD_SET_TX_ANTENNA);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_set_rx_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	cmd_change_ota_cmd_state(CMD_OTA_STATE_SET_ANTENNA_SENDING);
	enum ptt_ret ret = cmd_make_and_send_rf_packet(PTT_CMD_SET_RX_ANTENNA);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_get_rx_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	cmd_change_ota_cmd_state(CMD_OTA_STATE_GET_ANTENNA_SENDING);
	enum ptt_ret ret = cmd_make_and_send_rf_packet(PTT_CMD_GET_RX_ANTENNA);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_get_tx_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	cmd_change_ota_cmd_state(CMD_OTA_STATE_GET_ANTENNA_SENDING);
	enum ptt_ret ret = cmd_make_and_send_rf_packet(PTT_CMD_GET_TX_ANTENNA);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_get_last_best_rx_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	cmd_change_ota_cmd_state(CMD_OTA_STATE_GET_ANTENNA_SENDING);
	enum ptt_ret ret = cmd_make_and_send_rf_packet(PTT_CMD_GET_LAST_BEST_RX_ANTENNA);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static void cmd_get_antenna_timeout(ptt_evt_id_t timer_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(timer_evt_id);

	assert(true == cmd_is_ota_cmd_locked_by(timer_evt_id));

	cmd_send_timeout_and_reset();

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_get_antenna_sent(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret =
		ptt_timer_add(ptt_ctrl_get_rsp_timeout(), cmd_get_antenna_timeout, ota_cmd_evt);

	if (ret == PTT_RET_SUCCESS) {
		cmd_change_ota_cmd_state(CMD_OTA_STATE_GET_ANTENNA_WAITING_FOR_RSP);
	} else {
		PTT_TRACE("%s: add_timer return error code: %d\n", __func__, ret);
		cmd_send_timeout_and_reset();
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
}

static void cmd_get_antenna_response(ptt_evt_id_t new_rf_pkt_evt)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(new_rf_pkt_evt);

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(new_rf_pkt_evt);

	assert(evt_data != NULL);

	if ((ptt_event_get_cmd(new_rf_pkt_evt) == PTT_CMD_GET_ANTENNA_RESPONSE) &&
	    ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_GET_ANTENNA) ==
	     evt_data->len)) {
		ptt_timer_remove(ota_cmd_evt);

		cmd_fill_ctx_end_and_reset(&evt_data->arr[PTT_PAYLOAD_START],
					   PTT_PAYLOAD_LEN_GET_ANTENNA);
	} else {
		PTT_TRACE("%s: invalid length %d\n", __func__, evt_data->len);
	}

	PTT_TRACE_FUNC_EXIT();
}

static enum ptt_ret cmd_end_rx_test(void)
{
	PTT_TRACE_FUNC_ENTER();

	cmd_change_ota_cmd_state(CMD_OTA_STATE_END_RX_TEST_SENDING);
	enum ptt_ret ret = cmd_make_and_send_rf_packet(PTT_CMD_END_RX_TEST);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static void cmd_end_rx_test_timeout(ptt_evt_id_t timer_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(timer_evt_id);

	assert(true == cmd_is_ota_cmd_locked_by(timer_evt_id));

	cmd_send_timeout_and_reset();

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_end_rx_test_sent(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret =
		ptt_timer_add(ptt_ctrl_get_rsp_timeout(), cmd_end_rx_test_timeout, ota_cmd_evt);

	if (ret == PTT_RET_SUCCESS) {
		cmd_change_ota_cmd_state(CMD_OTA_STATE_END_RX_TEST_WAITING_FOR_REPORT);
	} else {
		PTT_TRACE("%s: add_timer return error code: %d\n", __func__, ret);
		cmd_send_timeout_and_reset();
	}

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_end_rx_test_report(ptt_evt_id_t new_rf_pkt_evt)
{
	PTT_TRACE_FUNC_EXIT_WITH_VALUE(new_rf_pkt_evt);

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(new_rf_pkt_evt);

	assert(evt_data != NULL);

	if ((ptt_event_get_cmd(new_rf_pkt_evt) == PTT_CMD_REPORT) &&
	    ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_REPORT) == evt_data->len)) {
		ptt_timer_remove(ota_cmd_evt);

		cmd_fill_ctx_end_and_reset(&evt_data->arr[PTT_PAYLOAD_START],
					   PTT_PAYLOAD_LEN_REPORT);
	} else {
		PTT_TRACE("%s: invalid length %d\n", __func__, evt_data->len);
	}

	PTT_TRACE_FUNC_EXIT();
}

static enum ptt_ret cmd_get_hardware_version(void)
{
	PTT_TRACE_FUNC_ENTER();

	cmd_change_ota_cmd_state(CMD_OTA_STATE_GET_HARDWARE_SENDING);
	enum ptt_ret ret = cmd_make_and_send_rf_packet(PTT_CMD_GET_HARDWARE_VERSION);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static void cmd_get_hardware_timeout(ptt_evt_id_t timer_evt_id)
{
	PTT_TRACE_FUNC_ENTER();

	assert(true == cmd_is_ota_cmd_locked_by(timer_evt_id));

	cmd_send_timeout_and_reset();

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_get_hardware_sent(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret =
		ptt_timer_add(ptt_ctrl_get_rsp_timeout(), cmd_get_hardware_timeout, ota_cmd_evt);

	if (ret == PTT_RET_SUCCESS) {
		cmd_change_ota_cmd_state(CMD_OTA_STATE_GET_HARDWARE_WAITING_FOR_RSP);
	} else {
		PTT_TRACE("%s: add_timer return error code: %d\n", __func__, ret);
		cmd_send_timeout_and_reset();
	}

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_get_hardware_response(ptt_evt_id_t new_rf_pkt_evt)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(new_rf_pkt_evt);

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(new_rf_pkt_evt);

	assert(evt_data != NULL);

	if ((ptt_event_get_cmd(new_rf_pkt_evt) == PTT_CMD_GET_HARDWARE_VERSION_RESPONSE) &&
	    ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_GET_HARDWARE) ==
	     evt_data->len)) {
		ptt_timer_remove(ota_cmd_evt);

		cmd_fill_ctx_end_and_reset(&evt_data->arr[PTT_PAYLOAD_START],
					   PTT_PAYLOAD_LEN_GET_HARDWARE);
	} else {
		PTT_TRACE("%s: invalid length %d\n", __func__, evt_data->len);
	}

	PTT_TRACE_FUNC_EXIT();
}

static enum ptt_ret cmd_get_software_version(void)
{
	PTT_TRACE_FUNC_ENTER();

	cmd_change_ota_cmd_state(CMD_OTA_STATE_GET_SOFTWARE_SENDING);
	enum ptt_ret ret = cmd_make_and_send_rf_packet(PTT_CMD_GET_SOFTWARE_VERSION);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static void cmd_get_software_timeout(ptt_evt_id_t timer_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(timer_evt_id);

	assert(true == cmd_is_ota_cmd_locked_by(timer_evt_id));

	cmd_send_timeout_and_reset();

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_get_software_sent(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret =
		ptt_timer_add(ptt_ctrl_get_rsp_timeout(), cmd_get_software_timeout, ota_cmd_evt);

	if (ret == PTT_RET_SUCCESS) {
		cmd_change_ota_cmd_state(CMD_OTA_STATE_GET_SOFTWARE_WAITING_FOR_RSP);
	} else {
		PTT_TRACE("%s: add_timer return error code: %d\n", __func__, ret);
		cmd_send_timeout_and_reset();
	}

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_get_software_response(ptt_evt_id_t new_rf_pkt_evt)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(new_rf_pkt_evt);

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(new_rf_pkt_evt);

	assert(evt_data != NULL);

	if ((ptt_event_get_cmd(new_rf_pkt_evt) == PTT_CMD_GET_SOFTWARE_VERSION_RESPONSE) &&
	    ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_GET_SOFTWARE) ==
	     evt_data->len)) {
		ptt_timer_remove(ota_cmd_evt);

		cmd_fill_ctx_end_and_reset(&evt_data->arr[PTT_PAYLOAD_START],
					   PTT_PAYLOAD_LEN_GET_SOFTWARE);
	} else {
		PTT_TRACE("%s: invalid length %d\n", __func__, evt_data->len);
	}

	PTT_TRACE_FUNC_EXIT();
}

static enum ptt_ret cmd_stream(void)
{
	PTT_TRACE_FUNC_ENTER();

	cmd_change_ota_cmd_state(CMD_OTA_STATE_STREAM_SENDING);
	enum ptt_ret ret = cmd_make_and_send_rf_packet(PTT_CMD_STREAM);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_start_rx_test(void)
{
	PTT_TRACE_FUNC_ENTER();

	cmd_change_ota_cmd_state(CMD_OTA_STATE_START_RX_TEST_SENDING);
	enum ptt_ret ret = cmd_make_and_send_rf_packet(PTT_CMD_START_RX_TEST);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static inline void cmd_ota_cmd_lock(ptt_evt_id_t new_ota_cmd)
{
	assert(ota_cmd_evt == PTT_EVENT_UNASSIGNED);

	ota_cmd_evt = new_ota_cmd;
}

static inline void cmd_change_ota_cmd_state(ptt_evt_state_t state)
{
	PTT_TRACE("%s: %d", __func__, state);

	ptt_event_set_state(ota_cmd_evt, state);
}

static inline ptt_evt_id_t cmd_get_ota_cmd_and_reset(void)
{
	ptt_evt_id_t evt_id = ota_cmd_evt;

	cmd_ota_cmd_unlock();
	return evt_id;
}

static void cmd_ota_cmd_unlock(void)
{
	ota_cmd_evt = PTT_EVENT_UNASSIGNED;
}

static inline bool cmd_is_ota_cmd_locked(void)
{
	return (ota_cmd_evt == PTT_EVENT_UNASSIGNED) ? false : true;
}

static inline bool cmd_is_ota_cmd_locked_by(ptt_evt_id_t evt_id)
{
	return (evt_id == ota_cmd_evt) ? true : false;
}

static void cmd_fill_ctx_end_and_reset(const uint8_t *data, ptt_pkt_len_t len)
{
	ptt_event_set_ctx_data(ota_cmd_evt, data, len);
	cmd_send_finish_and_reset();
}

static void cmd_send_finish_and_reset(void)
{
	ptt_evt_id_t evt_id = cmd_get_ota_cmd_and_reset();

	cmt_uart_ota_cmd_finish_notify(evt_id);
}

static void cmd_send_timeout_and_reset(void)
{
	ptt_evt_id_t evt_id = cmd_get_ota_cmd_and_reset();

	cmt_uart_ota_cmd_timeout_notify(evt_id);
}

static enum ptt_ret cmd_make_and_send_rf_packet(enum ptt_cmd cmd)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;
	struct ptt_evt_data_s *evt_data = ptt_event_get_data(ota_cmd_evt);

	assert(evt_data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(ota_cmd_evt);

	assert(ctx_data != NULL);

	evt_data->len = ptt_proto_construct_header(evt_data->arr, cmd, PTT_EVENT_DATA_SIZE);

	if (evt_data->len != (PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN)) {
		ret = PTT_RET_INVALID_VALUE;
	} else {
		memcpy(&evt_data->arr[evt_data->len], ctx_data->arr, ctx_data->len);
		evt_data->len += ctx_data->len;

		ret = ptt_rf_send_packet(ota_cmd_evt, evt_data->arr, evt_data->len);
	}

	return ret;
}
