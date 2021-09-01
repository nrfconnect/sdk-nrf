/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *//* Purpose: processing external UART messages */

/* We expects a command string looks like "custom <command_name> <whitespace_separated_bytes>".
 * Each whitespace-separated value fits into one byte and payload has big-endian byte order.
 * For example: "custom setchannel 0x00 0x00 0x08 0x00" to set 11 channel.
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "ctrl/ptt_ctrl.h"
#include "ctrl/ptt_events.h"
#include "ctrl/ptt_trace.h"
#include "ctrl/ptt_uart.h"

#include "ptt_proto.h"

#include "ptt_ctrl_internal.h"
#include "ptt_errors.h"
#include "ptt_events_internal.h"
#include "ptt_modes.h"
#include "ptt_uart_api.h"
#include "ptt_parser_internal.h"

#ifdef TESTS
#include "test_uart_conf.h"
#endif

/** array to parse text command */
static struct uart_text_cmd_s text_cmds[PTT_UART_CMD_N] = {
	{ UART_CMD_R_PING_TEXT, PTT_UART_CMD_R_PING, UART_CMD_R_PING_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_PING_TIMEOUT_TEXT, PTT_UART_CMD_L_PING_TIMEOUT,
	  UART_CMD_L_PING_TIMEOUT_PAYLOAD_L, PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_SETCHANNEL_TEXT, PTT_UART_CMD_SETCHANNEL, UART_CMD_SETCHANNEL_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_SETCHANNEL_TEXT, PTT_UART_CMD_L_SETCHANNEL, UART_CMD_L_SETCHANNEL_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_R_SETCHANNEL_TEXT, PTT_UART_CMD_R_SETCHANNEL, UART_CMD_R_SETCHANNEL_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_GET_CHANNEL_TEXT, PTT_UART_CMD_L_GET_CHANNEL, UART_CMD_L_GET_CHANNEL_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_SET_POWER_TEXT, PTT_UART_CMD_L_SET_POWER, UART_CMD_L_SET_POWER_PAYLOAD_L,
	  PTT_UART_PAYLOAD_RAW },
	{ UART_CMD_R_SET_POWER_TEXT, PTT_UART_CMD_R_SET_POWER, UART_CMD_R_SET_POWER_PAYLOAD_L,
	  PTT_UART_PAYLOAD_RAW },
	{ UART_CMD_L_GET_POWER_TEXT, PTT_UART_CMD_L_GET_POWER, UART_CMD_L_GET_POWER_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_R_GET_POWER_TEXT, PTT_UART_CMD_R_GET_POWER, UART_CMD_R_GET_POWER_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_R_STREAM_TEXT, PTT_UART_CMD_R_STREAM, UART_CMD_R_STREAM_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_R_START_TEXT, PTT_UART_CMD_R_START, UART_CMD_R_START_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_R_END_TEXT, PTT_UART_CMD_R_END, UART_CMD_R_END_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_REBOOT_TEXT, PTT_UART_CMD_L_REBOOT, UART_CMD_L_REBOOT_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_FIND_TEXT, PTT_UART_CMD_FIND, UART_CMD_FIND_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_R_HW_VERSION_TEXT, PTT_UART_CMD_R_HW_VERSION, UART_CMD_R_HW_VERSION_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_R_SW_VERSION_TEXT, PTT_UART_CMD_R_SW_VERSION, UART_CMD_R_SW_VERSION_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_CHANGE_MODE_TEXT, PTT_UART_CMD_CHANGE_MODE, UART_CMD_CHANGE_MODE_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_GET_CCA_TEXT, PTT_UART_CMD_L_GET_CCA, UART_CMD_L_GET_CCA_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_SET_CCA_TEXT, PTT_UART_CMD_L_SET_CCA, UART_CMD_L_SET_CCA_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_GET_ED_TEXT, PTT_UART_CMD_L_GET_ED, UART_CMD_L_GET_ED_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_GET_LQI_TEXT, PTT_UART_CMD_L_GET_LQI, UART_CMD_L_GET_LQI_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_GET_RSSI_TEXT, PTT_UART_CMD_L_GET_RSSI, UART_CMD_L_GET_RSSI_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_SET_SHORT_TEXT, PTT_UART_CMD_L_SET_SHORT, UART_CMD_L_SET_SHORT_PAYLOAD_L,
	  PTT_UART_PAYLOAD_RAW },
	{ UART_CMD_L_SET_EXTENDED_TEXT, PTT_UART_CMD_L_SET_EXTENDED,
	  UART_CMD_L_SET_EXTENDED_PAYLOAD_L, PTT_UART_PAYLOAD_RAW },
	{ UART_CMD_L_SET_PAN_ID_TEXT, PTT_UART_CMD_L_SET_PAN_ID, UART_CMD_L_SET_PAN_ID_PAYLOAD_L,
	  PTT_UART_PAYLOAD_RAW },
	{ UART_CMD_L_SET_PAYLOAD_TEXT, PTT_UART_CMD_L_SET_PAYLOAD, UART_CMD_L_SET_PAYLOAD_PAYLOAD_L,
	  PTT_UART_PAYLOAD_RAW },
	{ UART_CMD_L_START_TEXT, PTT_UART_CMD_L_START, UART_CMD_L_START_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_END_TEXT, PTT_UART_CMD_L_END, UART_CMD_L_END_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_SET_ANTENNA_TEXT, PTT_UART_CMD_L_SET_ANTENNA, UART_CMD_L_SET_ANTENNA_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_SET_TX_ANTENNA_TEXT, PTT_UART_CMD_L_SET_TX_ANTENNA,
	  UART_CMD_L_SET_TX_ANTENNA_PAYLOAD_L, PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_SET_RX_ANTENNA_TEXT, PTT_UART_CMD_L_SET_RX_ANTENNA,
	  UART_CMD_L_SET_RX_ANTENNA_PAYLOAD_L, PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_GET_RX_ANTENNA_TEXT, PTT_UART_CMD_L_GET_RX_ANTENNA,
	  UART_CMD_L_GET_RX_ANTENNA_PAYLOAD_L, PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_GET_TX_ANTENNA_TEXT, PTT_UART_CMD_L_GET_TX_ANTENNA,
	  UART_CMD_L_GET_TX_ANTENNA_PAYLOAD_L, PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_GET_LAST_BEST_RX_ANTENNA_TEXT, PTT_UART_CMD_L_GET_LAST_BEST_RX_ANTENNA,
	  UART_CMD_L_GET_LAST_BEST_RX_ANTENNA_PAYLOAD_L, PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_R_SET_ANTENNA_TEXT, PTT_UART_CMD_R_SET_ANTENNA, UART_CMD_R_SET_ANTENNA_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_R_SET_TX_ANTENNA_TEXT, PTT_UART_CMD_R_SET_TX_ANTENNA,
	  UART_CMD_R_SET_TX_ANTENNA_PAYLOAD_L, PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_R_SET_RX_ANTENNA_TEXT, PTT_UART_CMD_R_SET_RX_ANTENNA,
	  UART_CMD_R_SET_RX_ANTENNA_PAYLOAD_L, PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_R_GET_RX_ANTENNA_TEXT, PTT_UART_CMD_R_GET_RX_ANTENNA,
	  UART_CMD_R_GET_RX_ANTENNA_PAYLOAD_L, PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_R_GET_TX_ANTENNA_TEXT, PTT_UART_CMD_R_GET_TX_ANTENNA,
	  UART_CMD_R_GET_TX_ANTENNA_PAYLOAD_L, PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_R_GET_LAST_BEST_RX_ANTENNA_TEXT, PTT_UART_CMD_R_GET_LAST_BEST_RX_ANTENNA,
	  UART_CMD_R_GET_LAST_BEST_RX_ANTENNA_PAYLOAD_L, PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_TX_END_TEXT, PTT_UART_CMD_L_TX_END, UART_CMD_L_TX_END_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_TX_TEXT, PTT_UART_CMD_L_TX, UART_CMD_L_TX_PAYLOAD_L, PTT_UART_PAYLOAD_RAW },
	{ UART_CMD_L_CLK_TEXT, PTT_UART_CMD_L_CLK, UART_CMD_L_CLK_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_SET_GPIO_TEXT, PTT_UART_CMD_L_SET_GPIO, UART_CMD_L_SET_GPIO_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_GET_GPIO_TEXT, PTT_UART_CMD_L_GET_GPIO, UART_CMD_L_GET_GPIO_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_CARRIER_TEXT, PTT_UART_CMD_L_CARRIER, UART_CMD_L_CARRIER_PAYLOAD_L,
	  PTT_UART_PAYLOAD_RAW },
	{ UART_CMD_L_STREAM_TEXT, PTT_UART_CMD_L_STREAM, UART_CMD_L_STREAM_PAYLOAD_L,
	  PTT_UART_PAYLOAD_RAW },
	{ UART_CMD_L_SET_DCDC_TEXT, PTT_UART_CMD_L_SET_DCDC, UART_CMD_L_SET_DCDC_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_GET_DCDC_TEXT, PTT_UART_CMD_L_GET_DCDC, UART_CMD_L_GET_DCDC_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_SET_ICACHE_TEXT, PTT_UART_CMD_L_SET_ICACHE, UART_CMD_L_SET_ICACHE_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_GET_ICACHE_TEXT, PTT_UART_CMD_L_GET_ICACHE, UART_CMD_L_GET_ICACHE_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_GET_TEMP_TEXT, PTT_UART_CMD_L_GET_TEMP, UART_CMD_L_GET_TEMP_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_INDICATION_TEXT, PTT_UART_CMD_L_INDICATION, UART_CMD_L_INDICATION_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_SLEEP_TEXT, PTT_UART_CMD_L_SLEEP, UART_CMD_L_SLEEP_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
	{ UART_CMD_L_RECEIVE_TEXT, PTT_UART_CMD_L_RECEIVE, UART_CMD_L_RECEIVE_PAYLOAD_L,
	  PTT_UART_PAYLOAD_PARSED_AS_BYTES },
};

static ptt_uart_send_cb uart_send_cb;

static bool handler_busy;

static enum ptt_ret packet_parser(ptt_evt_id_t evt_id);
static enum ptt_ret text_cmd_parser(ptt_evt_id_t evt_id);
static enum ptt_ret payload_fill(struct ptt_evt_ctx_data_s *ctx_data, char *payload, uint8_t len);

void ptt_uart_init(ptt_uart_send_cb send_cb)
{
	uart_send_cb = send_cb;
	ptt_uart_print_prompt();
}

void ptt_uart_uninit(void)
{
	uart_send_cb = NULL;
}

inline void ptt_uart_print_prompt(void)
{
	if (uart_send_cb != NULL) {
		uart_send_cb(PTT_CAST_TO_UINT8_P(PTT_UART_PROMPT_STR),
			     sizeof(PTT_UART_PROMPT_STR) - 1, false);
	}
}

enum ptt_ret ptt_uart_send_packet(const uint8_t *pkt, ptt_pkt_len_t len)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if ((pkt == NULL) || (len == 0)) {
		ret = PTT_RET_INVALID_VALUE;
	}

	if (ret == PTT_RET_SUCCESS) {
		if (uart_send_cb == NULL) {
			ret = PTT_RET_NULL_PTR;
		} else {
			int32_t size = uart_send_cb(pkt, len, true);

			if (size != len) {
				ret = PTT_RET_BUSY;
			}
		}
	}

	PTT_TRACE("%s: ret %d", __func__, ret);
	return ret;
}

void ptt_uart_push_packet(const uint8_t *pkt, ptt_pkt_len_t len)
{
	ptt_evt_id_t evt_id;
	enum ptt_ret ret;

	if ((pkt == NULL) || (len == 0)) {
		ret = PTT_RET_INVALID_VALUE;
		PTT_TRACE("%s: invalid input value", __func__);
	} else {
		ret = ptt_event_alloc_and_fill(&evt_id, pkt, len);

		if (ret != PTT_RET_SUCCESS) {
			PTT_TRACE("%s: ptt_event_alloc_and_fill return error code: %d", __func__,
				  ret);
		} else {
			ret = packet_parser(evt_id);
		}
	}

	/* to prevent sending prompt on invalid command
	 * when handler is busy processing previous command
	 */
	if ((ret != PTT_RET_SUCCESS) && !handler_busy) {
		ptt_uart_print_prompt();
	}
}

/* @todo: refactor to let handler return result being busy */
void ptt_handler_busy(void)
{
	handler_busy = true;
}

void ptt_handler_free(void)
{
	handler_busy = false;
}

static enum ptt_ret packet_parser(ptt_evt_id_t evt_id)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;
	ptt_ext_evt_handler handler = ptt_ctrl_get_handler_uart_pkt_received();

	ret = text_cmd_parser(evt_id);

	if (ret == PTT_RET_SUCCESS) {
		if (ptt_event_get_cmd(evt_id) == PTT_UART_CMD_CHANGE_MODE) {
			struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

			assert(ctx_data != NULL);

			ret = ptt_mode_switch(ctx_data->arr[0]);

			ptt_uart_print_prompt();

			ptt_event_free(evt_id);
		} else if (handler == NULL) {
			ret = PTT_RET_INVALID_MODE;
			ptt_event_free(evt_id);
		} else {
			handler(evt_id);
		}
	} else {
		ptt_event_free(evt_id);
	}

	if (ret != PTT_RET_SUCCESS) {
		PTT_TRACE("packet parser ended with error code: %u", ret);
	}

	return ret;
}

/* fills evt_id with command and its parameters or return error */
static enum ptt_ret text_cmd_parser(ptt_evt_id_t evt_id)
{
	uint8_t str;
	uint8_t i;
	size_t cmd_name_len;
	enum ptt_ret ret = PTT_RET_SUCCESS;
	struct ptt_evt_data_s *data = ptt_event_get_data(evt_id);
	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

	assert(data != NULL);
	assert(ctx_data != NULL);

	for (i = 0; i < PTT_UART_CMD_N; ++i) {
		cmd_name_len = strlen(text_cmds[i].name);

		/* compare without '\0' symbol */
		str = strncmp(PTT_CAST_TO_STR(data->arr), text_cmds[i].name, cmd_name_len);

		if (str != 0) {
			ret = PTT_RET_INVALID_VALUE;
		} else {
			if (text_cmds[i].payload_type == PTT_UART_PAYLOAD_RAW) {
				/* raw payload already written in data, just pass */
				ret = PTT_RET_SUCCESS;
			} else if (text_cmds[i].payload_type == PTT_UART_PAYLOAD_PARSED_AS_BYTES) {
				/* after a command should be only whitespace
				 * in case of payload or \0
				 */
				if ((data->arr[cmd_name_len] == '\0') ||
				    (data->arr[cmd_name_len] == ' ')) {
					char *payload = PTT_CAST_TO_STR(data->arr + cmd_name_len);

					ret = payload_fill(ctx_data, payload,
							   text_cmds[i].payload_len);
				} else {
					ret = PTT_RET_INVALID_VALUE;
				}
			} else {
				ret = PTT_RET_INVALID_VALUE;
			}

			if (ret == PTT_RET_SUCCESS) {
				ptt_event_set_cmd(evt_id, text_cmds[i].code);
			}

			/* we found command name, time to quit */
			break;
		}
	}

	return ret;
}

/* payload points to payload of text cmd */
static enum ptt_ret payload_fill(struct ptt_evt_ctx_data_s *ctx_data, char *payload, uint8_t len)
{
	if ((payload == NULL) || (ctx_data == NULL)) {
		return PTT_RET_INVALID_VALUE;
	}

	enum ptt_ret ret = PTT_RET_SUCCESS;
	static char *save;
	char *next_word = strtok_r(payload, UART_TEXT_PAYLOAD_DELIMETERS, &save);

	for (uint8_t i = 0; i < len; ++i) {
		if (next_word == NULL) {
			/* expected more words in payload */
			ret = PTT_RET_INVALID_VALUE;
			break;
		}

		uint8_t out_num;

		/* signed values are processed in their handlers */
		ret = ptt_parser_string_to_uint8(next_word, &out_num, 0);

		if (ret != PTT_RET_SUCCESS) {
			break;
		}

		if (ctx_data->len >= PTT_EVENT_CTX_SIZE) {
			ret = PTT_RET_INVALID_VALUE;
			break;
		}

		ctx_data->arr[ctx_data->len] = out_num;
		++(ctx_data->len);

		next_word = strtok_r(NULL, UART_TEXT_PAYLOAD_DELIMETERS, &save);
	}

	if (next_word != NULL) {
		ret = PTT_RET_INVALID_VALUE;
	}

	return ret;
}
