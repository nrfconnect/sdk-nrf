/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: UART processing of Zigbee RF Performance Test Plan CMD mode */

/* The module processes an event containing UART command. The event locks the module as
 * currently processed UART command and the module rejects any other UART commands while
 * processing the one.
 * The module allocates event for OTA cmd, passes it to OTA cmd processing module,
 * gets it back with result and then frees the event.
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "ptt.h"

#include "ctrl/ptt_trace.h"
#include "ctrl/ptt_uart.h"
#include "rf/ptt_rf.h"

#include "ptt_proto.h"
#include "ptt_config.h"

#include "ptt_ctrl_internal.h"
#include "ptt_events_internal.h"
#include "ptt_parser_internal.h"
#include "ptt_zb_perf_cmd_mode.h"

#ifdef TESTS
#include "test_cmd_uart_conf.h"
#endif

/** currently processed UART command */
static ptt_evt_id_t uart_cmd_evt;

/** @brief: Event for "custom lindicate" command
 *
 *  Used as:
 *  1. Event to arm timer;
 *  2. Command state: validity of it value will tell is led indication currently active;
 *  3. Storage: all command related information will be stored in it context.
 */
static ptt_evt_id_t led_indication_evt;

static void cmd_uart_idle_packet_proc(void);

static enum ptt_ret cmd_uart_call_ota_cmd(enum ptt_cmd cmd, const struct ptt_evt_ctx_data_s *ctx);

static enum ptt_ret cmd_uart_parse_waveform_timings(const uint8_t *uart_cmd_payload,
						    int32_t *pulse_duration, int32_t *interval,
						    int32_t *duration_p);

/* "find" command routines */
static enum ptt_ret cmd_uart_find_idle(void);
static void cmd_uart_find_channel_was_set(void);
static void cmd_uart_find_delay(ptt_evt_id_t timer_evt_id);
static void cmd_uart_find_send_set_channel(void);
static void cmd_uart_find_process_finish(ptt_evt_id_t ota_cmd_evt);
static void cmd_uart_find_process_timeout(ptt_evt_id_t ota_cmd_evt);

/* "setchannel" command routines */
static enum ptt_ret cmd_uart_set_channel(void);
static void cmd_uart_set_channel_was_set(void);
static void cmd_uart_set_channel_delay(ptt_evt_id_t timer_evt_id);
static void cmd_uart_set_channel_process_timeout(ptt_evt_id_t ota_cmd_evt);
static void cmd_uart_set_channel_process_finish(ptt_evt_id_t ota_cmd_evt);
static enum ptt_ret cmd_uart_set_channel_payload_check(void);

/* "lsetchannel" command routine */
static enum ptt_ret cmd_uart_l_set_channel(void);

/* "rsetchannel" command routine */
static enum ptt_ret cmd_uart_r_set_channel(void);

/* "lgetchannel" command routine */
static enum ptt_ret cmd_uart_l_get_channel(void);

/* "lsetpower" command routine */
static enum ptt_ret cmd_uart_l_set_power(void);
static enum ptt_ret parse_power(const uint8_t *uart_cmd_payload, uint16_t *mode, int8_t *power_p);

/* "lsetpanid" command routine */
static enum ptt_ret cmd_uart_l_set_pan_id(void);

/* "lsetextended" command routine */
static enum ptt_ret cmd_uart_l_set_extended_address(void);

/* "lsetshort" command routine */
static enum ptt_ret cmd_uart_l_set_short_address(void);

/* "lsetpayload" command routine */
static enum ptt_ret cmd_uart_l_set_payload(void);

/* "rsetpower" command routine */
static enum ptt_ret cmd_uart_r_set_power(void);

/* "lgetpower" command routine */
static enum ptt_ret cmd_uart_l_get_power(void);

/* "lpingtimeout" command routine */
static enum ptt_ret cmd_uart_l_ping_timeout(void);

/* "lgetcca" command routine*/
static enum ptt_ret cmd_uart_l_get_cca(void);

/* "lsetcca" command routine*/
static enum ptt_ret cmd_uart_l_set_cca(void);

/* "lgeted" command routine*/
static enum ptt_ret cmd_uart_l_get_ed(void);

/* "lgetrssi" command routine */
static enum ptt_ret cmd_uart_l_get_rssi(void);
static void cmd_uart_l_get_rssi_delay(ptt_evt_id_t timer_evt_id);

/* "lstart" command routine */
static enum ptt_ret cmd_uart_l_start(void);
static void cmd_uart_l_start_process_rf_packet(ptt_evt_id_t new_rf_pkt_evt);

/* "lend" command routine */
static void cmd_uart_l_end(ptt_evt_id_t new_uart_cmd);

/* "lgetlqi" command routine */
static enum ptt_ret cmd_uart_l_get_lqi(void);
static void cmd_uart_l_get_lqi_delay(ptt_evt_id_t timer_evt_id);
static void cmd_uart_l_get_lqi_process_rf_packet(ptt_evt_id_t new_rf_pkt_evt);

/* "lsetantenna" command routine */
static enum ptt_ret cmd_uart_l_set_antenna(void);

/* "lsettxantenna" command routine */
static enum ptt_ret cmd_uart_l_set_tx_antenna(void);

/* "lsetrxantenna" command routine */
static enum ptt_ret cmd_uart_l_set_rx_antenna(void);

/* "lgetrxantenna" command routine */
static enum ptt_ret cmd_uart_l_get_rx_antenna(void);

/* "lgettxantenna" command routine */
static enum ptt_ret cmd_uart_l_get_tx_antenna(void);

/* "lgetbestrxantenna" command routine */
static enum ptt_ret cmd_uart_l_get_last_best_rx_antenna(void);

/* "rsetantenna" command routine */
static enum ptt_ret cmd_uart_r_set_antenna(void);

/* "rsettxantenna" command routine */
static enum ptt_ret cmd_uart_r_set_tx_antenna(void);

/* "rsetrxantenna" command routine */
static enum ptt_ret cmd_uart_r_set_rx_antenna(void);

/* "ltx" command routine */
static enum ptt_ret cmd_uart_l_tx(void);
static void cmd_uart_l_tx_process_next_packet(void);
static void cmd_uart_l_tx_delay(ptt_evt_id_t timer_evt_id);
static void cmd_uart_l_tx_process_ack(ptt_evt_id_t evt_id);
static void cmd_uart_l_tx_finished(ptt_evt_id_t evt_id);
static void cmd_uart_l_tx_failed(ptt_evt_id_t evt_id);
static void cmd_uart_l_tx_end(ptt_evt_id_t new_uart_cmd);

/* "lclk" command routine */
static enum ptt_ret cmd_uart_l_clk(void);
static void cmd_uart_l_clk_stop(ptt_evt_id_t new_uart_cmd);

/* "lsetgpio" command routine */
static enum ptt_ret cmd_uart_l_set_gpio(void);

/* "lgetgpio" command routine */
static enum ptt_ret cmd_uart_l_get_gpio(void);

/* "lcarrier" command routine */
static enum ptt_ret cmd_uart_l_carrier(void);
static void cmd_uart_l_carrier_duration_delay(ptt_evt_id_t timer_evt_id);
static void cmd_uart_l_carrier_pulse_delay(ptt_evt_id_t timer_evt_id);
static void cmd_uart_l_carrier_interval_delay(ptt_evt_id_t timer_evt_id);

/* "lstream" command routine */
static enum ptt_ret cmd_uart_l_stream(void);
static void cmd_uart_l_stream_duration_delay(ptt_evt_id_t timer_evt_id);
static void cmd_uart_l_stream_pulse_delay(ptt_evt_id_t timer_evt_id);
static void cmd_uart_l_stream_interval_delay(ptt_evt_id_t timer_evt_id);
static enum ptt_ret cmd_uart_l_stream_start(void);

/* "lsetdcdc" command routine */
static enum ptt_ret cmd_uart_l_set_dcdc(void);

/* "lgetdcdc" command routine */
static enum ptt_ret cmd_uart_l_get_dcdc(void);

/* "lseticache" command routine */
static enum ptt_ret cmd_uart_l_set_icache(void);

/* "lgeticache" command routine */
static enum ptt_ret cmd_uart_l_get_icache(void);

/* "lgettemp" command routine */
static enum ptt_ret cmd_uart_l_get_temp(void);

/* "lindication" command routine */
static enum ptt_ret cmd_uart_l_indication(void);
static void cmd_uart_l_indication_off(ptt_evt_id_t evt_id);

void cmd_uart_l_indication_proc(void);

/* "lsleep" command routine */
static enum ptt_ret cmd_uart_l_sleep(void);

/* "lreceive" command routine */
static enum ptt_ret cmd_uart_l_receive(void);

/* UART command lock routines */
static inline void cmd_uart_cmd_lock(ptt_evt_id_t new_uart_cmd);
static inline void cmd_change_uart_cmd_state(ptt_evt_state_t state);
static inline bool cmd_is_uart_cmd_locked(void);
static inline bool cmd_is_uart_cmd_locked_by(ptt_evt_id_t evt_id);
static void cmd_uart_cmd_unlock(void);

#ifdef TESTS
#include "test_cmd_uart_wrappers.c"
#endif

void cmd_uart_cmd_init(void)
{
	uart_cmd_evt = PTT_EVENT_UNASSIGNED;
	led_indication_evt = PTT_EVENT_UNASSIGNED;
}

void cmd_uart_cmd_uninit(void)
{
	cmd_uart_cmd_unlock();
	led_indication_evt = PTT_EVENT_UNASSIGNED;
}

void cmd_rf_cca_done(ptt_evt_id_t evt_id)
{
	assert(cmd_is_uart_cmd_locked_by(evt_id));

	cmd_uart_send_rsp_cca_done(evt_id);
	cmd_uart_cmd_unlock();
}

void cmd_rf_cca_failed(ptt_evt_id_t evt_id)
{
	assert(cmd_is_uart_cmd_locked_by(evt_id));

	cmd_uart_send_rsp_cca_failed(evt_id);
	cmd_uart_cmd_unlock();
}

void cmd_rf_ed_detected(ptt_evt_id_t evt_id)
{
	assert(cmd_is_uart_cmd_locked_by(evt_id));

	cmd_uart_send_rsp_ed_detected(evt_id);
	cmd_uart_cmd_unlock();
}

void cmd_rf_ed_failed(ptt_evt_id_t evt_id)
{
	assert(cmd_is_uart_cmd_locked_by(evt_id));

	cmd_uart_send_rsp_ed_failed(evt_id);
	cmd_uart_cmd_unlock();
}

void cmd_uart_pkt_received(ptt_evt_id_t new_uart_cmd)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(new_uart_cmd);

	if (false == cmd_is_uart_cmd_locked()) {
		/* store UART command as currently processed */
		cmd_uart_cmd_lock(new_uart_cmd);
		cmd_uart_idle_packet_proc();
	} else {
		switch (ptt_event_get_state(uart_cmd_evt)) {
		case CMD_UART_STATE_WAITING_FOR_LEND:
			cmd_uart_l_end(new_uart_cmd);
			ptt_event_free(new_uart_cmd);
			break;

		case CMD_UART_STATE_L_CLK_OUT:
			cmd_uart_l_clk_stop(new_uart_cmd);
			ptt_event_free(new_uart_cmd);
			break;

		case CMD_UART_STATE_LTX_WAITING_FOR_ACK:
		case CMD_UART_STATE_LTX:
			cmd_uart_l_tx_end(new_uart_cmd);
			ptt_event_free(new_uart_cmd);
			break;

		default:
			PTT_TRACE("%s: state isn't idle cmd %d state %d\n", __func__,
				  ptt_event_get_cmd(uart_cmd_evt),
				  ptt_event_get_state(uart_cmd_evt));
			ptt_event_free(new_uart_cmd);
			break;
		}
	}

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_uart_idle_packet_proc(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;
	enum ptt_uart_cmd uart_cmd = ptt_event_get_cmd(uart_cmd_evt);

	PTT_TRACE("%s: cmd %d\n", __func__, uart_cmd);

	/* handlers are responsible for cleaning current event */
	switch (uart_cmd) {
	case PTT_UART_CMD_R_PING:
		ret = cmd_uart_call_ota_cmd(PTT_CMD_PING, NULL);
		break;

	case PTT_UART_CMD_L_PING_TIMEOUT:
		ret = cmd_uart_l_ping_timeout();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_SETCHANNEL:
		ret = cmd_uart_set_channel();
		break;

	case PTT_UART_CMD_L_SETCHANNEL:
		ret = cmd_uart_l_set_channel();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_R_SETCHANNEL:
		ret = cmd_uart_r_set_channel();
		break;

	case PTT_UART_CMD_L_GET_CHANNEL:
		ret = cmd_uart_l_get_channel();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_SET_POWER:
		ret = cmd_uart_l_set_power();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_R_SET_POWER:
		ret = cmd_uart_r_set_power();
		break;

	case PTT_UART_CMD_L_GET_POWER:
		ret = cmd_uart_l_get_power();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_R_GET_POWER:
		ret = cmd_uart_call_ota_cmd(PTT_CMD_GET_POWER, NULL);
		break;

	case PTT_UART_CMD_R_STREAM:
		ret = cmd_uart_call_ota_cmd(PTT_CMD_STREAM, ptt_event_get_ctx_data(uart_cmd_evt));
		break;

	case PTT_UART_CMD_R_START:
		ret = cmd_uart_call_ota_cmd(PTT_CMD_START_RX_TEST, NULL);
		break;

	case PTT_UART_CMD_R_END:
		ret = cmd_uart_call_ota_cmd(PTT_CMD_END_RX_TEST, NULL);
		break;

	case PTT_UART_CMD_L_REBOOT:
		ptt_do_reset_ext();
		/* there is no return from function above */
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_FIND:
		ret = cmd_uart_find_idle();
		break;

	case PTT_UART_CMD_R_HW_VERSION:
		ret = cmd_uart_call_ota_cmd(PTT_CMD_GET_HARDWARE_VERSION, NULL);
		break;

	case PTT_UART_CMD_R_SW_VERSION:
		ret = cmd_uart_call_ota_cmd(PTT_CMD_GET_SOFTWARE_VERSION, NULL);
		break;

	case PTT_UART_CMD_L_GET_CCA:
		ret = cmd_uart_l_get_cca();
		break;

	case PTT_UART_CMD_L_SET_CCA:
		ret = cmd_uart_l_set_cca();
		break;

	case PTT_UART_CMD_L_GET_ED:
		ret = cmd_uart_l_get_ed();
		break;

	case PTT_UART_CMD_L_GET_RSSI:
		ret = cmd_uart_l_get_rssi();
		break;

	case PTT_UART_CMD_L_SET_PAN_ID:
		ret = cmd_uart_l_set_pan_id();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_SET_EXTENDED:
		ret = cmd_uart_l_set_extended_address();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_SET_SHORT:
		ret = cmd_uart_l_set_short_address();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_SET_PAYLOAD:
		ret = cmd_uart_l_set_payload();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_START:
		ret = cmd_uart_l_start();
		break;

	case PTT_UART_CMD_L_GET_LQI:
		ret = cmd_uart_l_get_lqi();
		break;

	case PTT_UART_CMD_L_SET_ANTENNA:
		ret = cmd_uart_l_set_antenna();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_SET_TX_ANTENNA:
		ret = cmd_uart_l_set_tx_antenna();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_SET_RX_ANTENNA:
		ret = cmd_uart_l_set_rx_antenna();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_GET_RX_ANTENNA:
		ret = cmd_uart_l_get_rx_antenna();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_GET_TX_ANTENNA:
		ret = cmd_uart_l_get_tx_antenna();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_GET_LAST_BEST_RX_ANTENNA:
		ret = cmd_uart_l_get_last_best_rx_antenna();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_R_SET_ANTENNA:
		ret = cmd_uart_r_set_antenna();
		break;

	case PTT_UART_CMD_R_SET_TX_ANTENNA:
		ret = cmd_uart_r_set_tx_antenna();
		break;

	case PTT_UART_CMD_R_SET_RX_ANTENNA:
		ret = cmd_uart_r_set_rx_antenna();
		break;

	case PTT_UART_CMD_R_GET_RX_ANTENNA:
		ret = cmd_uart_call_ota_cmd(PTT_CMD_GET_RX_ANTENNA, NULL);
		break;

	case PTT_UART_CMD_R_GET_TX_ANTENNA:
		ret = cmd_uart_call_ota_cmd(PTT_CMD_GET_TX_ANTENNA, NULL);
		break;

case PTT_UART_CMD_R_GET_LAST_BEST_RX_ANTENNA:
		ret = cmd_uart_call_ota_cmd(PTT_CMD_GET_LAST_BEST_RX_ANTENNA, NULL);
		break;

	case PTT_UART_CMD_L_TX:
		ret = cmd_uart_l_tx();
		break;

	case PTT_UART_CMD_L_CLK:
		ret = cmd_uart_l_clk();
		break;

	case PTT_UART_CMD_L_SET_GPIO:
		ret = cmd_uart_l_set_gpio();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_GET_GPIO:
		ret = cmd_uart_l_get_gpio();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_CARRIER:
		ret = cmd_uart_l_carrier();
		break;

	case PTT_UART_CMD_L_STREAM:
		ret = cmd_uart_l_stream();
		break;

	case PTT_UART_CMD_L_SET_DCDC:
		ret = cmd_uart_l_set_dcdc();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_GET_DCDC:
		ret = cmd_uart_l_get_dcdc();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_SET_ICACHE:
		ret = cmd_uart_l_set_icache();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_GET_ICACHE:
		ret = cmd_uart_l_get_icache();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_GET_TEMP:
		ret = cmd_uart_l_get_temp();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_INDICATION:
		ret = cmd_uart_l_indication();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_SLEEP:
		ret = cmd_uart_l_sleep();
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_L_RECEIVE:
		ret = cmd_uart_l_receive();
		cmd_uart_cmd_unlock();
		break;

	default:
		PTT_TRACE("%s: unknown command cmd %d\n ignored", __func__, uart_cmd);
		cmd_uart_cmd_unlock();
		break;
	}

	if (ret != PTT_RET_SUCCESS) {
		cmd_uart_cmd_unlock();
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
}

void cmt_uart_ota_cmd_timeout_notify(ptt_evt_id_t ota_cmd_evt)
{
	PTT_TRACE("%s: cmd %d state %d\n", __func__, ptt_event_get_cmd(uart_cmd_evt),
		  ptt_event_get_state(uart_cmd_evt));

	enum ptt_uart_cmd uart_cmd = ptt_event_get_cmd(uart_cmd_evt);

	switch (uart_cmd) {
	case PTT_UART_CMD_R_PING:
		cmd_uart_send_rsp_no_ack(uart_cmd_evt);
		cmd_uart_cmd_unlock();
		break;

	case PTT_CMD_SET_CHANNEL:
		cmd_uart_set_channel_process_timeout(ota_cmd_evt);
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_R_SETCHANNEL:
	case PTT_UART_CMD_R_SET_POWER:
	case PTT_UART_CMD_R_STREAM:
	case PTT_UART_CMD_R_START:
	case PTT_UART_CMD_R_SET_ANTENNA:
	case PTT_UART_CMD_R_SET_TX_ANTENNA:
	case PTT_UART_CMD_R_SET_RX_ANTENNA:
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_R_GET_POWER:
		cmd_uart_send_rsp_power_error(ota_cmd_evt);
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_R_END:
		cmd_uart_send_rsp_rx_test_error(uart_cmd_evt);
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_FIND:
		cmd_uart_find_process_timeout(ota_cmd_evt);
		break;

	case PTT_UART_CMD_R_HW_VERSION:
		cmd_uart_send_rsp_hw_error(uart_cmd_evt);
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_R_SW_VERSION:
		cmd_uart_send_rsp_sw_error(uart_cmd_evt);
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_R_GET_RX_ANTENNA:
		cmd_uart_send_rsp_antenna_error(ota_cmd_evt);
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_R_GET_TX_ANTENNA:
		cmd_uart_send_rsp_antenna_error(ota_cmd_evt);
		cmd_uart_cmd_unlock();
		break;

	default:
		PTT_TRACE("%s: unexpected notify ignored\n", __func__);
		cmd_uart_cmd_unlock();
		break;
	}

	/* it's final destination for event allocated inside call_ota_cmd */
	ptt_event_free(ota_cmd_evt);

	PTT_TRACE_FUNC_EXIT();
}

/* ota_cmd result inside ota_cmd_evt data context */
void cmt_uart_ota_cmd_finish_notify(ptt_evt_id_t ota_cmd_evt)
{
	PTT_TRACE("%s: cmd %d state %d\n", __func__, ptt_event_get_cmd(uart_cmd_evt),
		  ptt_event_get_state(uart_cmd_evt));

	enum ptt_uart_cmd uart_cmd = ptt_event_get_cmd(uart_cmd_evt);

	switch (uart_cmd) {
	case PTT_UART_CMD_R_PING:
		cmd_uart_send_rsack(uart_cmd_evt);
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_SETCHANNEL:
		cmd_uart_set_channel_process_finish(ota_cmd_evt);
		break;

	case PTT_UART_CMD_R_SETCHANNEL:
	case PTT_UART_CMD_R_SET_POWER:
	case PTT_UART_CMD_R_STREAM:
	case PTT_UART_CMD_R_START:
	case PTT_UART_CMD_R_SET_ANTENNA:
	case PTT_UART_CMD_R_SET_TX_ANTENNA:
	case PTT_UART_CMD_R_SET_RX_ANTENNA:
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_R_GET_POWER:
		cmd_uart_send_rsp_power(ota_cmd_evt);
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_R_END:
		cmd_uart_send_rsp_rx_test(ota_cmd_evt);
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_FIND:
		cmd_uart_find_process_finish(ota_cmd_evt);
		break;

	case PTT_UART_CMD_R_HW_VERSION:
		cmd_uart_send_rsp_hw(ota_cmd_evt);
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_R_SW_VERSION:
		cmd_uart_send_rsp_sw(ota_cmd_evt);
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_R_GET_RX_ANTENNA:
		cmd_uart_send_rsp_antenna(ota_cmd_evt);
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_R_GET_TX_ANTENNA:
		cmd_uart_send_rsp_antenna(ota_cmd_evt);
		cmd_uart_cmd_unlock();
		break;

	case PTT_UART_CMD_R_GET_LAST_BEST_RX_ANTENNA:
		cmd_uart_send_rsp_antenna(ota_cmd_evt);
		cmd_uart_cmd_unlock();
		break;

	default:
		PTT_TRACE("%s: unexpected notify ignored\n", __func__);
		cmd_uart_cmd_unlock();
		break;
	}

	/* it's final destination for event allocated inside call_ota_cmd */
	ptt_event_free(ota_cmd_evt);

	PTT_TRACE_FUNC_EXIT();
}

void cmd_rf_rx_done(ptt_evt_id_t new_rf_pkt_evt)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(new_rf_pkt_evt);

	if (false == cmd_is_uart_cmd_locked()) {
		cmd_ota_rf_rx_done(new_rf_pkt_evt);
		PTT_TRACE("%s: pass event to OTA module\n", __func__);
	} else {
		ptt_evt_state_t state = ptt_event_get_state(uart_cmd_evt);

		switch (state) {
		case CMD_UART_STATE_LTX_WAITING_FOR_ACK:
			cmd_uart_l_tx_process_ack(new_rf_pkt_evt);
			break;

		case CMD_UART_STATE_WAITING_FOR_LEND:
			cmd_uart_l_start_process_rf_packet(new_rf_pkt_evt);
			break;

		case CMD_UART_STATE_WAITING_FOR_RF_PACKET:
			cmd_uart_l_get_lqi_process_rf_packet(new_rf_pkt_evt);
			break;

		default:
			cmd_ota_rf_rx_done(new_rf_pkt_evt);
			PTT_TRACE("%s: pass event to OTA module\n", __func__);
		}
	}

	cmd_uart_l_indication_proc();

	ptt_event_free(new_rf_pkt_evt);
	PTT_TRACE_FUNC_EXIT();
}

void cmd_rf_rx_failed(ptt_evt_id_t new_rf_pkt_evt)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(new_rf_pkt_evt);

	if (true == cmd_is_uart_cmd_locked()) {
		ptt_evt_state_t state = ptt_event_get_state(uart_cmd_evt);

		switch (state) {
		case CMD_UART_STATE_WAITING_FOR_LEND:
			cmd_uart_send_rsp_l_start_rx_fail(new_rf_pkt_evt);
			break;

		default:
			PTT_TRACE("%s: unexpected event for current state\n", __func__);
			break;
		}
	}

	ptt_event_free(new_rf_pkt_evt);
	PTT_TRACE_FUNC_EXIT();
}

void cmd_rf_tx_finished(ptt_evt_id_t evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

	if (false == cmd_is_uart_cmd_locked()) {
		PTT_TRACE("%s: passing evt to ota module\n", __func__);
		cmd_ota_rf_tx_finished(evt_id);
	} else {
		ptt_evt_state_t state = ptt_event_get_state(uart_cmd_evt);

		switch (state) {
		case CMD_UART_STATE_LTX_WAITING_FOR_ACK:
			cmd_uart_l_tx_finished(evt_id);
			break;

		default:
			PTT_TRACE("%s: passing evt to ota module\n", __func__);
			cmd_ota_rf_tx_finished(evt_id);
			break;
		}
	}

	PTT_TRACE_FUNC_EXIT();
}

void cmd_rf_tx_failed(ptt_evt_id_t evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

	if (false == cmd_is_uart_cmd_locked()) {
		PTT_TRACE("%s: passing evt to ota module\n", __func__);
		cmd_ota_rf_tx_failed(evt_id);
	} else {
		ptt_evt_state_t state = ptt_event_get_state(uart_cmd_evt);

		switch (state) {
		case CMD_UART_STATE_LTX_WAITING_FOR_ACK:
			cmd_uart_l_tx_failed(evt_id);
			break;

		default:
			PTT_TRACE("%s: passing evt to ota module\n", __func__);
			cmd_ota_rf_tx_failed(evt_id);
			break;
		}
	}

	PTT_TRACE_FUNC_EXIT();
}

static enum ptt_ret cmd_uart_l_ping_timeout(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	/* by protocol, timeout consists from two bytes */
	uint16_t timeout = ptt_betoh16_val(ctx_data->arr);

	ptt_ctrl_set_rsp_timeout(timeout);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static enum ptt_ret cmd_uart_set_channel_payload_check(void)
{
	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	uint32_t mask = ptt_betoh32_val(ctx_data->arr);
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if ((mask < (1u << PTT_CHANNEL_MIN)) || (mask > (1u << PTT_CHANNEL_MAX)) ||
	    (!(mask && !(mask & (mask - 1))))) {
		/* check that more that one bit is set*/
		ret = PTT_RET_INVALID_VALUE;
	}

	return ret;
}

static enum ptt_ret cmd_uart_set_channel(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = cmd_uart_set_channel_payload_check();

	if (ret == PTT_RET_SUCCESS) {
		cmd_change_uart_cmd_state(CMD_UART_STATE_SET_CHANNEL);

		ret = cmd_uart_call_ota_cmd(PTT_CMD_SET_CHANNEL,
					    ptt_event_get_ctx_data(uart_cmd_evt));
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static void cmd_uart_set_channel_was_set(void)
{
	PTT_TRACE_FUNC_ENTER();

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	uint32_t mask = ptt_betoh32_val(ctx_data->arr);

	enum ptt_ret ret = PTT_RET_SUCCESS;

	/* set channel to CMD */
	ret = ptt_rf_set_channel_mask(uart_cmd_evt, mask);

	if (ret == PTT_RET_SUCCESS) {
		cmd_change_uart_cmd_state(CMD_UART_STATE_WAIT_DELAY);
		ret = ptt_timer_add(ptt_ctrl_get_rsp_timeout(), cmd_uart_set_channel_delay,
				    uart_cmd_evt);
	}

	if (ret != PTT_RET_SUCCESS) {
		cmd_uart_cmd_unlock();
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
}

static void cmd_uart_set_channel_delay(ptt_evt_id_t timer_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(timer_evt_id);

	assert(true == cmd_is_uart_cmd_locked_by(timer_evt_id));

	cmd_change_uart_cmd_state(CMD_UART_STATE_PING);
	enum ptt_ret ret = cmd_uart_call_ota_cmd(PTT_CMD_PING, NULL);

	if (ret != PTT_RET_SUCCESS) {
		PTT_TRACE("%s: cmd_uart_call_ota_cmd returns error code: %d\n", __func__, ret);
		cmd_uart_cmd_unlock();
	}

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_uart_set_channel_process_timeout(ptt_evt_id_t ota_cmd_evt)
{
	PTT_TRACE_FUNC_ENTER();

	enum cmd_uart_state_t state = ptt_event_get_state(uart_cmd_evt);

	PTT_TRACE("%s: state %d\n", __func__, state);

	switch (state) {
	case CMD_UART_STATE_PING:
		cmd_uart_send_rsp_no_ack(uart_cmd_evt);
		break;

	default:
		PTT_TRACE("%s: unexpected timeout\n", __func__);
		break;
	}

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_uart_set_channel_process_finish(ptt_evt_id_t ota_cmd_evt)
{
	PTT_TRACE_FUNC_ENTER();

	enum cmd_uart_state_t state = ptt_event_get_state(uart_cmd_evt);

	PTT_TRACE("%s state %d\n", __func__, state);

	switch (state) {
	case CMD_UART_STATE_SET_CHANNEL:
		cmd_uart_set_channel_was_set();
		break;

	case CMD_UART_STATE_PING:
		/* uart_cmd_evt data ctx has current channel */
		cmd_uart_send_rsack(uart_cmd_evt);
		cmd_uart_cmd_unlock();
		break;

	default:
		PTT_TRACE("%s: unexpected finish\n", __func__);
		cmd_uart_cmd_unlock();
		break;
	}

	PTT_TRACE_FUNC_EXIT();
}

static enum ptt_ret cmd_uart_l_set_channel(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = cmd_uart_set_channel_payload_check();

	if (ret == PTT_RET_SUCCESS) {
		struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

		assert(ctx_data != NULL);

		uint32_t mask = ptt_betoh32_val(ctx_data->arr);

		/* set channel to CMD */
		ret = ptt_rf_set_channel_mask(uart_cmd_evt, mask);
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static enum ptt_ret cmd_uart_r_set_channel(void)
{
	PTT_TRACE_FUNC_ENTER();

	cmd_change_uart_cmd_state(CMD_UART_STATE_SET_CHANNEL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	uint8_t channel = ctx_data->arr[0];
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if ((channel < PTT_CHANNEL_MIN) || (channel > PTT_CHANNEL_MAX)) {
		ret = PTT_RET_INVALID_VALUE;
	} else {
		uint32_t mask = 1u << channel;

		ptt_htobe32((uint8_t *)(&mask), ctx_data->arr);
		ctx_data->len = sizeof(mask);

		ret = cmd_uart_call_ota_cmd(PTT_CMD_SET_CHANNEL,
					    ptt_event_get_ctx_data(uart_cmd_evt));
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static enum ptt_ret cmd_uart_l_get_channel(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	ctx_data->arr[0] = ptt_rf_get_channel();
	ctx_data->len = 1;

	cmd_uart_send_rsp_l_channel(uart_cmd_evt);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static enum ptt_ret parse_power(const uint8_t *uart_cmd_payload, uint16_t *mode_p, int8_t *power_p)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;
	uint16_t mode;
	uint8_t *p = (uint8_t *)&mode;
	int8_t power;

	do {
		if ((uart_cmd_payload == NULL) || (mode_p == NULL) || (power_p == NULL)) {
			ret = PTT_RET_INVALID_VALUE;
			break;
		}
		static char *save;
		char *token_str = strtok_r(PTT_CAST_TO_STR(uart_cmd_payload),
					   UART_TEXT_PAYLOAD_DELIMETERS, &save);

		if (token_str == NULL) {
			ret = PTT_RET_INVALID_VALUE;
			break;
		}
		ret = ptt_parser_string_to_uint8(token_str, &p[0], 0);

		if (ret == PTT_RET_SUCCESS) {
			token_str = strtok_r(NULL, UART_TEXT_PAYLOAD_DELIMETERS, &save);

			if (token_str == NULL) {
				ret = PTT_RET_INVALID_VALUE;
				break;
			}
			ret = ptt_parser_string_to_uint8(token_str, &p[1], 0);

			if (ret == PTT_RET_SUCCESS) {
				token_str = strtok_r(NULL, UART_TEXT_PAYLOAD_DELIMETERS, &save);

				if ((token_str == NULL) ||
				    (strtok_r(NULL, UART_TEXT_PAYLOAD_DELIMETERS, &save) != NULL)) {
					ret = PTT_RET_INVALID_VALUE;
				} else {
					ret = ptt_parser_string_to_int8(token_str, &power, 10);
				}
			}
		}
	} while (false);

	if (ret == PTT_RET_SUCCESS) {
		ptt_betoh16(p, (uint8_t *)mode_p);
		*power_p = power;
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static enum ptt_ret cmd_uart_l_set_power(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;
	struct ptt_evt_data_s *data = ptt_event_get_data(uart_cmd_evt);
	uint16_t mode;
	int8_t power;

	assert(data != NULL);

	uint8_t *uart_cmd_payload = data->arr + sizeof(UART_CMD_L_SET_POWER_TEXT);

	ret = parse_power(uart_cmd_payload, &mode, &power);

	if (ret == PTT_RET_SUCCESS) {
		/* @todo: handle mode */
		/* set power to CMD */
		ret = ptt_rf_set_power(uart_cmd_evt, power);
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static enum ptt_ret cmd_uart_r_set_power(void)
{
	PTT_TRACE_FUNC_ENTER();

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);
	enum ptt_ret ret = PTT_RET_SUCCESS;
	struct ptt_evt_data_s *data = ptt_event_get_data(uart_cmd_evt);
	uint16_t mode;
	int8_t power;

	assert(ctx_data != NULL);

	assert(data != NULL);

	uint8_t *uart_cmd_payload = data->arr + sizeof(UART_CMD_R_SET_POWER_TEXT);

	ret = parse_power(uart_cmd_payload, &mode, &power);

	if (ret == PTT_RET_SUCCESS) {
		/* @todo: handle mode */
		ctx_data->arr[0] = (uint8_t)power;
		ctx_data->len = sizeof(power);

		ret = cmd_uart_call_ota_cmd(PTT_CMD_SET_POWER,
					    ptt_event_get_ctx_data(uart_cmd_evt));
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static enum ptt_ret cmd_uart_l_get_power(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	ctx_data->arr[0] = (uint8_t)ptt_rf_get_power();
	ctx_data->len = 1;

	cmd_uart_send_rsp_power(uart_cmd_evt);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static enum ptt_ret cmd_uart_l_get_cca(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;
	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	if (ctx_data->len == 0) {
		cmd_uart_send_rsp_cca_failed(uart_cmd_evt);
		ret = PTT_RET_INVALID_VALUE;
	} else {
		ret = ptt_rf_cca(uart_cmd_evt, ctx_data->arr[0]);
	}

	if (ret != PTT_RET_SUCCESS) {
		cmd_uart_send_rsp_cca_failed(uart_cmd_evt);
	}

	PTT_TRACE_FUNC_EXIT();
	return ret;
}

static enum ptt_ret cmd_uart_l_set_cca(void)
{
	PTT_TRACE_FUNC_ENTER();

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	enum ptt_ret ret = PTT_RET_SUCCESS;
	uint8_t activate = ctx_data->arr[0];

	if ((activate == 0) || (activate == 1)) {
		ptt_rf_set_cca(uart_cmd_evt, activate);
	} else {
		ret = PTT_RET_INVALID_VALUE;
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_l_get_ed(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;

	ret = ptt_rf_ed(uart_cmd_evt, PTT_ED_TIME_US);

	if (ret != PTT_RET_SUCCESS) {
		cmd_uart_send_rsp_ed_failed(uart_cmd_evt);
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_l_get_rssi(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = ptt_rf_rssi_measure_begin(uart_cmd_evt);

	if (ret == PTT_RET_SUCCESS) {
		cmd_change_uart_cmd_state(CMD_UART_STATE_WAIT_DELAY);
		ret = ptt_timer_add(PTT_RSSI_TIME_MS, cmd_uart_l_get_rssi_delay, uart_cmd_evt);
	}

	if (ret != PTT_RET_SUCCESS) {
		cmd_uart_send_rsp_rssi_failed(uart_cmd_evt);
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static void cmd_uart_l_get_rssi_delay(ptt_evt_id_t timer_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(timer_evt_id);

	assert(true == cmd_is_uart_cmd_locked_by(timer_evt_id));

	ptt_rssi_t rssi;
	enum ptt_ret ret = ptt_rf_rssi_last_get(timer_evt_id, &rssi);

	if ((ret == PTT_RET_SUCCESS) && (rssi != PTT_RSSI_ERROR_VALUE)) {
		ptt_event_set_ctx_data(timer_evt_id, PTT_CAST_TO_UINT8_P(&rssi), sizeof(rssi));

		cmd_uart_send_rsp_rssi_done(timer_evt_id);
	} else {
		cmd_uart_send_rsp_rssi_failed(timer_evt_id);
	}

	cmd_uart_cmd_unlock();
	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
}

static enum ptt_ret cmd_uart_l_set_pan_id(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;
	uint8_t pan_id[PTT_PANID_ADDRESS_SIZE];
	uint8_t pan_id_le[PTT_PANID_ADDRESS_SIZE];
	struct ptt_evt_data_s *data = ptt_event_get_data(uart_cmd_evt);

	assert(data != NULL);

	uint8_t *payload = data->arr + sizeof(UART_CMD_L_SET_PAN_ID_TEXT);

	static char *save;
	char *id_str = strtok_r(PTT_CAST_TO_STR(payload), UART_TEXT_PAYLOAD_DELIMETERS, &save);

	if ((id_str == NULL) || (strtok_r(NULL, UART_TEXT_PAYLOAD_DELIMETERS, &save) != NULL)) {
		/* lets be sure that there are no extra parameters */
		ret = PTT_RET_INVALID_VALUE;
	} else {
		/* to pass only 0xYYYY type of values */
		if ((strlen(id_str) == UART_SET_PAN_ID_SYM_NUM) && (id_str[0] == '0') &&
		    (id_str[1] == 'x')) {
			ret = ptt_parser_string_to_uint16(id_str, (uint16_t *)pan_id, 16);

			if (ret == PTT_RET_SUCCESS) {
				/* parser will return host endian number,
				 * we need to be sure than it is little endian
				 */
				ptt_htole16(pan_id, pan_id_le);

				ret = ptt_rf_set_pan_id(uart_cmd_evt, pan_id_le);
			}
		}
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_l_set_extended_address(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;
	uint8_t extended_addr[PTT_EXTENDED_ADDRESS_SIZE] = { 0 };
	uint8_t extended_addr_le[PTT_EXTENDED_ADDRESS_SIZE] = { 0 };
	struct ptt_evt_data_s *data = ptt_event_get_data(uart_cmd_evt);

	assert(data != NULL);

	uint8_t *payload = data->arr + sizeof(UART_CMD_L_SET_EXTENDED_TEXT);

	static char *save;
	char *addr_str = strtok_r(PTT_CAST_TO_STR(payload), UART_TEXT_PAYLOAD_DELIMETERS, &save);

	if ((addr_str == NULL) || (strtok_r(NULL, UART_TEXT_PAYLOAD_DELIMETERS, &save) != NULL)) {
		/* lets be sure that there are no extra parameters */
		ret = PTT_RET_INVALID_VALUE;
	} else {
		/* to pass only 0xYYYYYYYYYYYYYYYY type of values */
		if ((strlen(addr_str) == UART_SET_EXTENDED_SYM_NUM) && (addr_str[0] == '0') &&
		    (addr_str[1] == 'x')) {
			uint8_t written_len = 0;

			ret = ptt_parser_hex_string_to_uint8_array(
				addr_str, extended_addr,
				ARRAY_SIZE(extended_addr), &written_len);

			if (written_len != PTT_EXTENDED_ADDRESS_SIZE) {
				ret = PTT_RET_INVALID_VALUE;
			}
			if (ret == PTT_RET_SUCCESS) {
				/* parser works only with bytes,
				 * so endianness is unchanged => it is big endian
				 */
				/* converting big endian address to little endian */
				for (uint8_t i = 0; i < PTT_EXTENDED_ADDRESS_SIZE; i++) {
					extended_addr_le[i] =
						extended_addr[PTT_EXTENDED_ADDRESS_SIZE - 1 - i];
				}

				ret = ptt_rf_set_extended_address(uart_cmd_evt, extended_addr_le);
			}
		}
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_l_set_short_address(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;
	uint8_t short_addr[PTT_SHORT_ADDRESS_SIZE];
	uint8_t short_addr_le[PTT_SHORT_ADDRESS_SIZE];
	struct ptt_evt_data_s *data = ptt_event_get_data(uart_cmd_evt);

	assert(data != NULL);

	uint8_t *payload = data->arr + sizeof(UART_CMD_L_SET_SHORT_TEXT);

	static char *save;
	char *addr_str = strtok_r(PTT_CAST_TO_STR(payload), UART_TEXT_PAYLOAD_DELIMETERS, &save);

	if ((addr_str == NULL) || (strtok_r(NULL, UART_TEXT_PAYLOAD_DELIMETERS, &save) != NULL)) {
		/* lets be sure that there are no extra parameters */
		ret = PTT_RET_INVALID_VALUE;
	} else {
		/* to pass only 0xYYYY type of values */
		if ((strlen(addr_str) == UART_SET_SHORT_SYM_NUM) && (addr_str[0] == '0') &&
		    (addr_str[1] == 'x')) {
			ret = ptt_parser_string_to_uint16(addr_str, (uint16_t *)short_addr, 16);

			if (ret == PTT_RET_SUCCESS) {
				/* parser will return host endian number,
				 * we need to be sure than it is little endian
				 */
				ptt_htole16(short_addr, short_addr_le);

				ret = ptt_rf_set_short_address(uart_cmd_evt, short_addr_le);
			}
		}
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_l_set_payload(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;
	struct ptt_evt_data_s *data = ptt_event_get_data(uart_cmd_evt);
	uint8_t payload[PTT_CUSTOM_LTX_PAYLOAD_MAX_SIZE] = { 0 };

	assert(data != NULL);

	uint8_t *uart_cmd_payload = data->arr + sizeof(UART_CMD_L_SET_PAYLOAD_TEXT);

	static char *save;
	char *token_str =
		strtok_r(PTT_CAST_TO_STR(uart_cmd_payload), UART_TEXT_PAYLOAD_DELIMETERS, &save);

	if (token_str == NULL) {
		ret = PTT_RET_INVALID_VALUE;
	} else {
		uint8_t payload_len;

		ret = ptt_parser_string_to_uint8(token_str, &payload_len, 0);

		if (ret == PTT_RET_SUCCESS) {
			struct ptt_ltx_payload_s *ltx_payload;

			ltx_payload = ptt_rf_get_custom_ltx_payload();

			assert(ltx_payload != NULL);

			token_str = strtok_r(NULL, UART_TEXT_PAYLOAD_DELIMETERS, &save);

			if ((token_str == NULL) ||
			    (strtok_r(NULL, UART_TEXT_PAYLOAD_DELIMETERS, &save) != NULL)) {
				/* lets be sure that there are no extra parameters */
				ret = PTT_RET_INVALID_VALUE;
			} else {
				uint8_t actual_len = 0;

				ret = ptt_parser_hex_string_to_uint8_array(
					token_str, payload, PTT_CUSTOM_LTX_PAYLOAD_MAX_SIZE,
					&actual_len);
				if (actual_len != payload_len) {
					ret = PTT_RET_INVALID_VALUE;
				}

				if (ret == PTT_RET_SUCCESS) {
					ltx_payload->len = actual_len;
					memcpy(ltx_payload->arr, payload, ltx_payload->len);
				}
			}
		}
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_l_start(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = ptt_rf_start_statistic(uart_cmd_evt);

	if (ret == PTT_RET_SUCCESS) {
		/* we will store counter for protocol packets in context.
		 * Lets erase it just in case
		 */
		struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

		assert(ctx_data != NULL);

		uint32_t *proto_pkts = (uint32_t *)ctx_data->arr;

		*proto_pkts = 0;

		cmd_change_uart_cmd_state(CMD_UART_STATE_WAITING_FOR_LEND);
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static void cmd_uart_l_start_process_rf_packet(ptt_evt_id_t new_rf_pkt_evt)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(new_rf_pkt_evt);

	struct ptt_evt_data_s *rf_data = ptt_event_get_data(new_rf_pkt_evt);

	assert(rf_data != NULL);

	if (ptt_proto_check_packet(rf_data->arr, rf_data->len)) {
		struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

		assert(ctx_data != NULL);

		uint32_t *proto_pkts = (uint32_t *)ctx_data->arr;

		++(*proto_pkts);
	}

	cmd_uart_send_rsp_l_start_new_packet(new_rf_pkt_evt);

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_uart_l_end(ptt_evt_id_t new_uart_cmd)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_uart_cmd uart_cmd = ptt_event_get_cmd(new_uart_cmd);

	if (uart_cmd == PTT_UART_CMD_L_END) {
		enum ptt_ret ret = ptt_rf_end_statistic(uart_cmd_evt);

		if (ret != PTT_RET_SUCCESS) {
			PTT_TRACE("%s: ptt_rf_end_statistic returns %d", __func__, ret);
		} else {
			struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

			assert(ctx_data != NULL);

			uint32_t *proto_pkts = (uint32_t *)ctx_data->arr;

			cmd_uart_send_rsp_l_end(new_uart_cmd, *proto_pkts);
		}

		cmd_uart_cmd_unlock();
	}

	PTT_TRACE_FUNC_EXIT();
}

static enum ptt_ret cmd_uart_l_get_lqi(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;

	ret = ptt_timer_add(PTT_LQI_DELAY_MS, cmd_uart_l_get_lqi_delay, uart_cmd_evt);

	if (ret == PTT_RET_SUCCESS) {
		cmd_change_uart_cmd_state(CMD_UART_STATE_WAITING_FOR_RF_PACKET);
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static void cmd_uart_l_get_lqi_delay(ptt_evt_id_t timer_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(timer_evt_id);

	assert(true == cmd_is_uart_cmd_locked_by(timer_evt_id));

	cmd_uart_send_rsp_lqi_failed(timer_evt_id);

	cmd_uart_cmd_unlock();
	PTT_TRACE_FUNC_EXIT();
}

static void cmd_uart_l_get_lqi_process_rf_packet(ptt_evt_id_t new_rf_pkt_evt)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(new_rf_pkt_evt);

	ptt_timer_remove(uart_cmd_evt);
	cmd_uart_send_rsp_lqi_done(new_rf_pkt_evt);

	cmd_uart_cmd_unlock();
	PTT_TRACE_FUNC_EXIT();
}

static enum ptt_ret cmd_uart_l_set_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	uint8_t antenna = ctx_data->arr[0];

	enum ptt_ret ret = ptt_rf_set_antenna(uart_cmd_evt, antenna);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_l_set_tx_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	uint8_t antenna = ctx_data->arr[0];

	enum ptt_ret ret = ptt_rf_set_tx_antenna(uart_cmd_evt, antenna);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_l_set_rx_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	uint8_t antenna = ctx_data->arr[0];

	enum ptt_ret ret = ptt_rf_set_rx_antenna(uart_cmd_evt, antenna);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_l_set_dcdc(void)
{
	PTT_TRACE_FUNC_ENTER();

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	enum ptt_ret ret = PTT_RET_SUCCESS;
	uint8_t activate = ctx_data->arr[0];

	if ((activate == 0) || (activate == 1)) {
		ptt_ctrl_set_dcdc(activate);
	} else {
		ret = PTT_RET_INVALID_VALUE;
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_l_get_dcdc(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;
	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	bool dcdc_active = ptt_ctrl_get_dcdc();

	ctx_data->arr[0] = dcdc_active;
	ctx_data->len = sizeof(dcdc_active);

	cmd_uart_send_rsp_get_dcdc(uart_cmd_evt);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_l_set_icache(void)
{
	PTT_TRACE_FUNC_ENTER();

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	uint8_t enable = ctx_data->arr[0];
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if ((enable == 0) || (enable == 1)) {
		ptt_ctrl_set_icache(enable);
	} else {
		ret = PTT_RET_INVALID_VALUE;
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_l_get_icache(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;
	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	bool icache_active = ptt_ctrl_get_icache();

	ctx_data->arr[0] = icache_active;
	ctx_data->len = sizeof(icache_active);

	cmd_uart_send_rsp_get_icache(uart_cmd_evt);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_l_indication(void)
{
	PTT_TRACE_FUNC_ENTER();

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	uint8_t activate = ctx_data->arr[0];
	enum ptt_ret ret = PTT_RET_SUCCESS;

	if ((activate == 1) && (led_indication_evt == PTT_EVENT_UNASSIGNED)) {
		ret = ptt_event_alloc(&led_indication_evt);

		if (ret == PTT_RET_SUCCESS) {
			/* setting default value just in case */
			struct ptt_evt_ctx_data_s *ctx_data =
				ptt_event_get_ctx_data(led_indication_evt);

			assert(ctx_data != NULL);

			bool *is_timer_active = (bool *)ctx_data->arr;

			*is_timer_active = false;
		}
	} else if ((activate == 0) && (led_indication_evt != PTT_EVENT_UNASSIGNED)) {
		struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(led_indication_evt);

		assert(ctx_data != NULL);

		bool is_timer_active = *((bool *)ctx_data->arr);

		if (is_timer_active) {
			ptt_timer_remove(led_indication_evt);
			cmd_uart_l_indication_off(led_indication_evt);
		}

		ptt_event_free(led_indication_evt);
		led_indication_evt = PTT_EVENT_UNASSIGNED;
	} else {
		ret = PTT_RET_INVALID_VALUE;
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

/* this function used as a callback for timer */
static void cmd_uart_l_indication_off(ptt_evt_id_t evt_id)
{
	PTT_TRACE_FUNC_ENTER();

	PTT_UNUSED(evt_id);

	ptt_ctrl_led_indication_off();

	assert(led_indication_evt != PTT_EVENT_UNASSIGNED);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(led_indication_evt);

	assert(ctx_data != NULL);

	bool *is_timer_active = (bool *)ctx_data->arr;

	if (false == *is_timer_active) {
		PTT_TRACE("%s: WARNING: is_timer_active variable is false\n", __func__);
	}

	*is_timer_active = false;

	PTT_TRACE_FUNC_EXIT();
}

void cmd_uart_l_indication_proc(void)
{
	if (led_indication_evt != PTT_EVENT_UNASSIGNED) {
		struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(led_indication_evt);

		assert(ctx_data != NULL);

		bool *is_timer_active = (bool *)ctx_data->arr;

		if (false == *is_timer_active) {
			enum ptt_ret led_ret =
				ptt_timer_add(PTT_LED_INDICATION_BLINK_TIMEOUT_MS,
					      cmd_uart_l_indication_off, led_indication_evt);

			if (led_ret != PTT_RET_SUCCESS) {
				PTT_TRACE("%s: ptt_timer_add for LED indication returns error: %d",
					  __func__, led_ret);
			} else {
				ptt_ctrl_led_indication_on();
				*is_timer_active = true;
			}
		}
	}
}

static enum ptt_ret cmd_uart_l_tx(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;
	struct ptt_evt_data_s *data = ptt_event_get_data(uart_cmd_evt);

	assert(data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	uint8_t *uart_cmd_payload = data->arr + sizeof(UART_CMD_L_TX_TEXT);
	static char *save;
	char *token_str =
		strtok_r(PTT_CAST_TO_STR(uart_cmd_payload), UART_TEXT_PAYLOAD_DELIMETERS, &save);

	struct cmd_uart_ltx_info_s *ltx_info = (struct cmd_uart_ltx_info_s *)ctx_data->arr;

	ltx_info->is_stop_requested = false;

	if (token_str == NULL) {
		ltx_info->max_repeats_cnt = PTT_CUSTOM_LTX_DEFAULT_REPEATS;
		ltx_info->timeout = PTT_CUSTOM_LTX_DEFAULT_TIMEOUT_MS;
	} else {
		ret = ptt_parser_string_to_uint8(token_str, &(ltx_info->max_repeats_cnt), 0);

		if (ret == PTT_RET_SUCCESS) {
			token_str = strtok_r(NULL, UART_TEXT_PAYLOAD_DELIMETERS, &save);

			if ((token_str == NULL) ||
			    (strtok_r(NULL, UART_TEXT_PAYLOAD_DELIMETERS, &save) != NULL)) {
				/* lets be sure that there are no extra parameters */
				ret = PTT_RET_INVALID_VALUE;
			} else {
				ret = ptt_parser_string_to_uint16(token_str, &(ltx_info->timeout),
								  0);

				if (ltx_info->timeout > PTT_CUSTOM_LTX_TIMEOUT_MAX_VALUE) {
					ret = PTT_RET_INVALID_VALUE;
				}
			}
		}
	}

	if (ret == PTT_RET_SUCCESS) {
		if (ltx_info->max_repeats_cnt == 0) {
			ltx_info->is_infinite = true;
		} else {
			ltx_info->is_infinite = false;
		}

		ltx_info->repeats_cnt = 0;
		ltx_info->ack_cnt = 0;
		cmd_change_uart_cmd_state(CMD_UART_STATE_LTX);
		cmd_uart_l_tx_process_next_packet();
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static void cmd_uart_l_tx_end(ptt_evt_id_t new_uart_cmd)
{
	PTT_TRACE_FUNC_ENTER();

	if (ptt_event_get_cmd(new_uart_cmd) == PTT_UART_CMD_L_TX_END) {
		struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

		assert(ctx_data != NULL);

		struct cmd_uart_ltx_info_s *ltx_info = (struct cmd_uart_ltx_info_s *)ctx_data->arr;

		if (ltx_info->is_infinite) {
			/* This will cause the ltx routine to stop sending packets
			 * Requesting it ensures that all replies are received before stopping
			 */
			ltx_info->is_stop_requested = true;
		}
	}

	PTT_TRACE_FUNC_EXIT();
}

static enum ptt_ret cmd_uart_l_get_rx_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	uint8_t antenna = ptt_rf_get_rx_antenna();

	ctx_data->arr[0] = antenna;
	ctx_data->len = sizeof(antenna);

	cmd_uart_send_rsp_antenna(uart_cmd_evt);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_l_get_tx_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	uint8_t antenna = ptt_rf_get_tx_antenna();

	ctx_data->arr[0] = antenna;
	ctx_data->len = sizeof(antenna);

	cmd_uart_send_rsp_antenna(uart_cmd_evt);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_l_get_last_best_rx_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	uint8_t antenna = ptt_rf_get_last_rx_best_antenna();

	ctx_data->arr[0] = antenna;
	ctx_data->len = sizeof(antenna);

	cmd_uart_send_rsp_antenna(uart_cmd_evt);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_l_get_temp(void)
{
	PTT_TRACE_FUNC_ENTER();

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	int32_t temp;
	enum ptt_ret ret = ptt_ctrl_get_temp(&temp);

	if (ret == PTT_RET_SUCCESS) {
		*((int32_t *)ctx_data) = temp;
		ctx_data->len = sizeof(temp);

		cmd_uart_send_rsp_get_temp(uart_cmd_evt);
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static void cmd_uart_l_tx_process_next_packet(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;
	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	struct cmd_uart_ltx_info_s *ltx_info = (struct cmd_uart_ltx_info_s *)ctx_data->arr;

	if ((!ltx_info->is_infinite && (ltx_info->max_repeats_cnt <= ltx_info->repeats_cnt)) ||
	    ltx_info->is_stop_requested) {
		cmd_uart_cmd_unlock();
	} else {
		++(ltx_info->repeats_cnt);

		ret = ptt_timer_add(ltx_info->timeout, cmd_uart_l_tx_delay, uart_cmd_evt);

		if (ret != PTT_RET_SUCCESS) {
			PTT_TRACE("%s: ptt_timer_add returned %d, aborting ltx\n", __func__, ret);
			cmd_uart_cmd_unlock();
		}
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
}

static void cmd_uart_l_tx_delay(ptt_evt_id_t timer_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(timer_evt_id);

	assert(cmd_is_uart_cmd_locked_by(timer_evt_id));

	enum ptt_ret ret = PTT_RET_SUCCESS;
	struct ptt_ltx_payload_s *ltx_payload = ptt_rf_get_custom_ltx_payload();
	/* we will use payload_len instead of ltx_payload->len to be able to generate new random
	 * payload for every sent packet
	 */
	uint8_t payload_len = ltx_payload->len;

	if (payload_len == 0) {
		ptt_random_vector_generate(&payload_len, sizeof(payload_len));

		/* make sure that size we get is not bigger that out buffer */
		payload_len %= PTT_CUSTOM_LTX_PAYLOAD_MAX_SIZE + 1;

		if (payload_len == 0) {
			payload_len = PTT_CUSTOM_LTX_PAYLOAD_MAX_SIZE / 2;
		}

		ptt_random_vector_generate(ltx_payload->arr, payload_len);
	}

	if (ret == PTT_RET_SUCCESS) {
		ptt_evt_id_t sender_event_id;

		ret = ptt_event_alloc(&sender_event_id);

		if (ret == PTT_RET_SUCCESS) {
			cmd_change_uart_cmd_state(CMD_UART_STATE_LTX_WAITING_FOR_ACK);
			ret = ptt_rf_send_packet(sender_event_id, ltx_payload->arr, payload_len);

			if (ret != PTT_RET_SUCCESS) {
				PTT_TRACE("%s: ptt_rf_send_packet returns %d, aborting ltx\n",
					  __func__, ret);
				ptt_event_free(sender_event_id);
				cmd_uart_cmd_unlock();
			}
		} else {
			PTT_TRACE("%s: ptt_event_alloc returns %d, aborting ltx\n", __func__, ret);
			cmd_uart_cmd_unlock();
			ret = PTT_RET_INVALID_VALUE;
		}
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
}

static void cmd_uart_l_tx_finished(ptt_evt_id_t evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

	if (!cmd_is_uart_cmd_locked()) {
		PTT_TRACE("%s: uart module is not locked, aborting ltx\n", __func__);
		cmd_uart_cmd_unlock();
	} else {
		ptt_evt_state_t state = ptt_event_get_state(uart_cmd_evt);

		if (state != CMD_UART_STATE_LTX_WAITING_FOR_ACK) {
			PTT_TRACE("%s: uart module state is unexpected, aborting ltx\n", __func__);
			cmd_uart_cmd_unlock();
		} else {
			cmd_change_uart_cmd_state(CMD_UART_STATE_LTX);
			cmd_uart_l_tx_process_next_packet();
		}
	}

	ptt_event_free(evt_id);
	PTT_TRACE_FUNC_EXIT();
}

static void cmd_uart_l_tx_process_ack(ptt_evt_id_t evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

	struct ptt_evt_data_s *data = ptt_event_get_data(evt_id);

	assert(data != NULL);
	assert(data->arr != NULL);

	/* if ACK is not NULL */
	if (data->len != 0) {
		struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

		assert(ctx_data != NULL);

		struct cmd_uart_ltx_info_s *ltx_info = (struct cmd_uart_ltx_info_s *)ctx_data->arr;

		cmd_uart_send_rsp_ltx_ack(evt_id, ltx_info->ack_cnt);

		++(ltx_info->ack_cnt);
	}

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_uart_l_tx_failed(ptt_evt_id_t evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

	if (!cmd_is_uart_cmd_locked()) {
		PTT_TRACE("%s: uart module is not locked, aborting ltx\n", __func__);
		cmd_uart_cmd_unlock();
	} else {
		ptt_evt_state_t state = ptt_event_get_state(uart_cmd_evt);

		if (state != CMD_UART_STATE_LTX_WAITING_FOR_ACK) {
			PTT_TRACE("%s: uart module state is unexpected, aborting ltx\n", __func__);
			cmd_uart_cmd_unlock();
		} else {
			cmd_change_uart_cmd_state(CMD_UART_STATE_LTX);
			cmd_uart_send_rsp_ltx_failed(evt_id);
			cmd_uart_l_tx_process_next_packet();
		}
	}

	ptt_event_free(evt_id);
	PTT_TRACE_FUNC_EXIT();
}

static enum ptt_ret cmd_uart_find_idle(void)
{
	uint32_t mask = 1u << PTT_CHANNEL_MIN;

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	/* set 0x0B channel as the first channel for find procedure */
	enum ptt_ret ret = ptt_rf_set_channel_mask(uart_cmd_evt, mask);

	if (ret == PTT_RET_SUCCESS) {
		ptt_htobe32((uint8_t *)(&mask), ctx_data->arr);
		ctx_data->len = sizeof(mask);

		cmd_change_uart_cmd_state(CMD_UART_STATE_SET_CHANNEL);

		ret = cmd_uart_call_ota_cmd(PTT_CMD_SET_CHANNEL,
					    ptt_event_get_ctx_data(uart_cmd_evt));
	}

	return ret;
}

static enum ptt_ret cmd_uart_r_set_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret =
		cmd_uart_call_ota_cmd(PTT_CMD_SET_ANTENNA, ptt_event_get_ctx_data(uart_cmd_evt));

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_r_set_tx_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret =
		cmd_uart_call_ota_cmd(PTT_CMD_SET_TX_ANTENNA, ptt_event_get_ctx_data(uart_cmd_evt));

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_r_set_rx_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret =
		cmd_uart_call_ota_cmd(PTT_CMD_SET_RX_ANTENNA, ptt_event_get_ctx_data(uart_cmd_evt));

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static void cmd_uart_find_channel_was_set(void)
{
	PTT_TRACE_FUNC_ENTER();

	cmd_change_uart_cmd_state(CMD_UART_STATE_WAIT_DELAY);
	enum ptt_ret ret =
		ptt_timer_add(ptt_ctrl_get_rsp_timeout(), cmd_uart_find_delay, uart_cmd_evt);

	if (ret != PTT_RET_SUCCESS) {
		PTT_TRACE("%s: add_timer returns error code: %d\n", __func__, ret);
		cmd_uart_cmd_unlock();
	}

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_uart_find_delay(ptt_evt_id_t timer_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(timer_evt_id);

	assert(true == cmd_is_uart_cmd_locked_by(timer_evt_id));

	cmd_change_uart_cmd_state(CMD_UART_STATE_PING);
	enum ptt_ret ret = cmd_uart_call_ota_cmd(PTT_CMD_PING, NULL);

	if (ret != PTT_RET_SUCCESS) {
		PTT_TRACE("%s: cmd_uart_call_ota_cmd returns error code: %d\n", __func__, ret);
		cmd_uart_cmd_unlock();
	}

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_uart_find_send_set_channel(void)
{
	PTT_TRACE_FUNC_ENTER();

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	uint32_t mask = ptt_betoh32_val(ctx_data->arr);

	if (mask >= (1u << PTT_CHANNEL_MAX)) {
		/* not found*/
		cmd_uart_send_rsp_find_timeout(uart_cmd_evt);
		cmd_uart_cmd_unlock();
	} else {
		mask <<= 1u;

		enum ptt_ret ret = ptt_rf_set_channel_mask(uart_cmd_evt, mask);

		if (ret == PTT_RET_SUCCESS) {
			ptt_htobe32((uint8_t *)(&mask), ctx_data->arr);

			cmd_change_uart_cmd_state(CMD_UART_STATE_SET_CHANNEL);

			/* first SET_CHANNEL call is from current CMD channel */
			ret = cmd_uart_call_ota_cmd(PTT_CMD_SET_CHANNEL,
						    ptt_event_get_ctx_data(uart_cmd_evt));
		}

		if (ret != PTT_RET_SUCCESS) {
			PTT_TRACE("%s: failed with error code: %d\n", __func__, ret);
			cmd_uart_cmd_unlock();
		}
	}

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_uart_find_process_finish(ptt_evt_id_t ota_cmd_evt)
{
	PTT_TRACE_FUNC_ENTER();

	enum cmd_uart_state_t state = ptt_event_get_state(uart_cmd_evt);

	PTT_TRACE("%s: state %d\n", __func__, state);

	switch (state) {
	case CMD_UART_STATE_SET_CHANNEL:
		cmd_uart_find_channel_was_set();
		break;

	case CMD_UART_STATE_PING:
		/* uart_cmd_evt data ctx has current channel */
		cmd_uart_send_rsp_find(uart_cmd_evt);
		cmd_uart_cmd_unlock();
		break;

	default:
		PTT_TRACE("%s: unexpected finish\n", __func__);
		cmd_uart_cmd_unlock();
		break;
	}

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_uart_find_process_timeout(ptt_evt_id_t ota_cmd_evt)
{
	PTT_TRACE_FUNC_ENTER();

	enum cmd_uart_state_t state = ptt_event_get_state(uart_cmd_evt);

	PTT_TRACE("%s: state %d\n", __func__, state);

	switch (state) {
	case CMD_UART_STATE_PING:
		cmd_uart_find_send_set_channel();
		break;

	default:
		PTT_TRACE("%s: unexpected timeout\n", __func__);
		cmd_uart_cmd_unlock();
		break;
	}

	PTT_TRACE_FUNC_EXIT();
}

static enum ptt_ret cmd_uart_l_clk(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;
	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	uint8_t pin = ctx_data->arr[0];
	bool mode = (bool)ctx_data->arr[1];

	/* if mode is false just skip it, output clk is already disabled */
	if (mode == PTT_VALUE_ON) {
		if (ptt_clk_out_ext(pin, mode)) {
			cmd_change_uart_cmd_state(CMD_UART_STATE_L_CLK_OUT);
		} else {
			ret = PTT_RET_INVALID_VALUE;
		}
	} else {
		ret = PTT_RET_INVALID_VALUE;
	}

	PTT_TRACE_FUNC_EXIT();
	return ret;
}

static void cmd_uart_l_clk_stop(ptt_evt_id_t new_uart_cmd)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_uart_cmd uart_cmd = ptt_event_get_cmd(new_uart_cmd);

	if (uart_cmd == PTT_UART_CMD_L_CLK) {
		struct ptt_evt_ctx_data_s *ctx_data_new = ptt_event_get_ctx_data(new_uart_cmd);

		assert(ctx_data_new != NULL);

		uint8_t pin_new = ctx_data_new->arr[0];
		bool mode_new = (bool)ctx_data_new->arr[1];

		struct ptt_evt_ctx_data_s *ctx_data_old = ptt_event_get_ctx_data(uart_cmd_evt);

		assert(ctx_data_old != NULL);

		uint8_t pin_old = ctx_data_old->arr[0];
		bool mode_old = (bool)ctx_data_old->arr[1];

		if ((pin_old == pin_new) && (mode_old) && (mode_new == PTT_VALUE_OFF)) {
			if (ptt_clk_out_ext(pin_new, mode_new)) {
				cmd_uart_cmd_unlock();
			}
		}
	}

	PTT_TRACE_FUNC_EXIT();
}

static enum ptt_ret cmd_uart_l_set_gpio(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;
	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	uint8_t pin = ctx_data->arr[0];
	uint8_t value = ctx_data->arr[1];

	if ((value == PTT_VALUE_OFF) || (value == PTT_VALUE_ON)) {
		if (!ptt_set_gpio_ext(pin, value)) {
			ret = PTT_RET_INVALID_VALUE;
		}
	} else {
		ret = PTT_RET_INVALID_VALUE;
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static enum ptt_ret cmd_uart_l_get_gpio(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;
	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	uint8_t pin = ctx_data->arr[0];
	uint8_t *value = &ctx_data->arr[1];

	if (!ptt_get_gpio_ext(pin, value)) {
		cmd_uart_send_rsp_l_get_gpio_error(uart_cmd_evt);
		ret = PTT_RET_INVALID_VALUE;
	} else {
		ctx_data->len++;
		cmd_uart_send_rsp_l_get_gpio(uart_cmd_evt);
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static enum ptt_ret cmd_uart_parse_waveform_timings(const uint8_t *uart_cmd_payload,
						    int32_t *pulse_duration, int32_t *interval,
						    int32_t *duration_p)
{
	if (uart_cmd_payload == NULL) {
		return PTT_RET_INVALID_VALUE;
	}

	enum ptt_ret ret = PTT_RET_SUCCESS;
	static char *save;
	char *token_str =
		strtok_r(PTT_CAST_TO_STR(uart_cmd_payload), UART_TEXT_PAYLOAD_DELIMETERS, &save);

	/* arguments parsing */
	if (token_str == NULL) {
		ret = PTT_RET_INVALID_VALUE;
	} else {
		ret = ptt_parser_string_to_int32(token_str, pulse_duration, 0);

		if (ret == PTT_RET_SUCCESS) {
			token_str = strtok_r(NULL, UART_TEXT_PAYLOAD_DELIMETERS, &save);

			if (token_str == NULL) {
				ret = PTT_RET_INVALID_VALUE;
			} else {
				ret = ptt_parser_string_to_int32(token_str, interval, 0);

				if (ret == PTT_RET_SUCCESS) {
					token_str =
						strtok_r(NULL, UART_TEXT_PAYLOAD_DELIMETERS, &save);

					if ((token_str == NULL) ||
					    /* lets be sure that there are no extra parameters */
					    (strtok_r(NULL, UART_TEXT_PAYLOAD_DELIMETERS, &save) !=
					     NULL)) {
						ret = PTT_RET_INVALID_VALUE;
					} else {
						ret = ptt_parser_string_to_int32(token_str,
										 duration_p, 0);
					}
				}
			}
		}
	}

	return ret;
}

static enum ptt_ret cmd_uart_l_carrier(void)
{
	PTT_TRACE_FUNC_ENTER();

	int32_t transmission_duration;
	struct cmd_uart_waveform_timings_s timings_info = { 0 };
	enum ptt_ret ret = PTT_RET_SUCCESS;
	struct ptt_evt_data_s *data = ptt_event_get_data(uart_cmd_evt);

	assert(data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	uint8_t *uart_cmd_payload = data->arr + sizeof(UART_CMD_L_CARRIER_TEXT);

	ret = cmd_uart_parse_waveform_timings(uart_cmd_payload, &(timings_info.pulse_duration),
					      &(timings_info.interval), &transmission_duration);

	if (ret != PTT_RET_SUCCESS) {
		PTT_TRACE("%s: error occurs due payload parsing: %d. Aborting command handling",
			  __func__, ret);
	} else {
		/* range checks */
		/* The values are only stored in the ctx_data if the parsing is successful */
		memcpy(ctx_data->arr, &timings_info, sizeof(struct cmd_uart_waveform_timings_s));

		bool is_pulse_duration_valid =
			(timings_info.pulse_duration >= PTT_L_CARRIER_PULSE_MIN) &&
			(timings_info.pulse_duration <= PTT_L_CARRIER_PULSE_MAX);
		bool is_interval_valid = (timings_info.interval >= PTT_L_CARRIER_INTERVAL_MIN) &&
					 (timings_info.interval <= PTT_L_CARRIER_INTERVAL_MAX);
		bool is_transmission_duration_valid =
			(transmission_duration == 0) ||
			((transmission_duration >= PTT_L_CARRIER_DURATION_MIN) &&
			 (transmission_duration <= PTT_L_CARRIER_DURATION_MAX));

		if (!is_pulse_duration_valid || !is_interval_valid ||
		    !is_transmission_duration_valid) {
			ret = PTT_RET_INVALID_VALUE;
			PTT_TRACE("%s: error during arg range checks: %d. Aborting", __func__, ret);
		} else {
			/* arm timers */
			/* Transmission duration equal zero means infinite transmission */
			if (transmission_duration != 0) {
				ret = ptt_timer_add(transmission_duration,
						    cmd_uart_l_carrier_duration_delay,
						    uart_cmd_evt);
			}

			if (ret == PTT_RET_SUCCESS) {
				ret = ptt_timer_add(timings_info.pulse_duration,
						    cmd_uart_l_carrier_pulse_delay, uart_cmd_evt);
			}

			if (ret != PTT_RET_SUCCESS) {
				ptt_timer_remove(uart_cmd_evt);

				PTT_TRACE("%s: error occurs during timers arming: %d. Aborting",
					  __func__, ret);
			} else {
				/* start sending carrier */
				ret = ptt_rf_start_continuous_carrier(uart_cmd_evt);

				if (ret != PTT_RET_SUCCESS) {
					ret = PTT_RET_INVALID_VALUE;
					PTT_TRACE(
						"%s: rf_start_continuous_carrier is false.Aborting",
						__func__);
				} else {
					cmd_change_uart_cmd_state(CMD_UART_STATE_L_CARRIER_PULSE);
				}
			}
		}
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static void cmd_uart_l_carrier_duration_delay(ptt_evt_id_t timer_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(timer_evt_id);

	assert(cmd_is_uart_cmd_locked_by(timer_evt_id));

	enum ptt_ret ret = ptt_rf_stop_continuous_carrier(uart_cmd_evt);

	if (ret != PTT_RET_SUCCESS) {
		PTT_TRACE("%s: WARNING ptt_rf_stop_continuous_carrier returns error %d\n", __func__,
			  ret);
	}

	ptt_timer_remove(timer_evt_id);

	cmd_uart_cmd_unlock();

	PTT_TRACE_FUNC_EXIT();
}

static void cmd_uart_l_carrier_pulse_delay(ptt_evt_id_t timer_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(timer_evt_id);

	assert(cmd_is_uart_cmd_locked_by(timer_evt_id));

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	struct cmd_uart_waveform_timings_s *info =
		(struct cmd_uart_waveform_timings_s *)(ctx_data->arr);

	enum ptt_ret ret =
		ptt_timer_add(info->interval, cmd_uart_l_carrier_interval_delay, uart_cmd_evt);

	if (ret == PTT_RET_SUCCESS) {
		ret = ptt_rf_stop_continuous_carrier(uart_cmd_evt);

		if (ret == PTT_RET_SUCCESS) {
			cmd_change_uart_cmd_state(CMD_UART_STATE_L_CARRIER_INTERVAL);
		} else {
			PTT_TRACE("%s: ptt_rf_stop_continuous_carrier returns error: %d. Aborting",
				  __func__, ret);
		}
	} else {
		PTT_TRACE("%s: error occurs during timer arming: %d. Aborting command handling",
			  __func__, ret);
	}

	if (ret != PTT_RET_SUCCESS) {
		ptt_timer_remove(uart_cmd_evt);
		cmd_uart_cmd_unlock();
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
}

static void cmd_uart_l_carrier_interval_delay(ptt_evt_id_t timer_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(timer_evt_id);

	assert(cmd_is_uart_cmd_locked_by(timer_evt_id));

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	struct cmd_uart_waveform_timings_s *info =
		(struct cmd_uart_waveform_timings_s *)(ctx_data->arr);

	enum ptt_ret ret =
		ptt_timer_add(info->pulse_duration, cmd_uart_l_carrier_pulse_delay, uart_cmd_evt);

	if (ret == PTT_RET_SUCCESS) {
		ret = ptt_rf_start_continuous_carrier(uart_cmd_evt);

		if (ret == PTT_RET_SUCCESS) {
			cmd_change_uart_cmd_state(CMD_UART_STATE_L_CARRIER_PULSE);
		} else {
			PTT_TRACE("%s: ptt_rf_start_continuous_carrier returns error: %d. Aborting",
				  __func__, ret);
		}
	} else {
		PTT_TRACE("%s: error occurs during timer arming: %d. Aborting command handling",
			  __func__, ret);
	}

	if (ret != PTT_RET_SUCCESS) {
		ptt_timer_remove(uart_cmd_evt);
		cmd_uart_cmd_unlock();
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
}

static enum ptt_ret cmd_uart_l_stream_start(void)
{
	enum ptt_ret ret = PTT_RET_SUCCESS;

	struct ptt_ltx_payload_s *ltx_payload = ptt_rf_get_custom_ltx_payload();
	/* we will use payload_len instead of ltx_payload->len to be able to generate new random
	 * payload
	 */
	uint8_t payload_len = ltx_payload->len;

	if (payload_len == 0) {
		ptt_random_vector_generate(&payload_len, sizeof(payload_len));

		payload_len %= PTT_CUSTOM_LTX_PAYLOAD_MAX_SIZE + 1;

		if (payload_len == 0) {
			payload_len = PTT_CUSTOM_LTX_PAYLOAD_MAX_SIZE / 2;
		}

		ptt_random_vector_generate(ltx_payload->arr, payload_len);
	}

	ret = ptt_rf_start_modulated_stream(uart_cmd_evt, ltx_payload->arr, payload_len);

	return ret;
}

static enum ptt_ret cmd_uart_l_stream(void)
{
	PTT_TRACE_FUNC_ENTER();

	int32_t transmission_duration;
	struct cmd_uart_waveform_timings_s timings_info = { 0 };
	enum ptt_ret ret = PTT_RET_SUCCESS;
	struct ptt_evt_data_s *data = ptt_event_get_data(uart_cmd_evt);

	assert(data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	uint8_t *uart_cmd_payload = data->arr + sizeof(UART_CMD_L_STREAM_TEXT);

	ret = cmd_uart_parse_waveform_timings(uart_cmd_payload, &(timings_info.pulse_duration),
					      &(timings_info.interval), &transmission_duration);

	if (ret != PTT_RET_SUCCESS) {
		PTT_TRACE("%s: error occurs due payload parsing: %d. Aborting command handling",
			  __func__, ret);
	} else {
		/* range checks */
		/* The values are only stored in the ctx_data if the parsing is successful */
		memcpy(ctx_data->arr, &timings_info, sizeof(struct cmd_uart_waveform_timings_s));

		bool is_pulse_duration_valid =
			(timings_info.pulse_duration >= PTT_L_CARRIER_PULSE_MIN) &&
			(timings_info.pulse_duration <= PTT_L_CARRIER_PULSE_MAX);
		bool is_interval_valid = (timings_info.interval >= PTT_L_CARRIER_INTERVAL_MIN) &&
					 (timings_info.interval <= PTT_L_CARRIER_INTERVAL_MAX);
		bool is_transmission_duration_valid =
			(transmission_duration == 0) ||
			((transmission_duration >= PTT_L_CARRIER_DURATION_MIN) &&
			 (transmission_duration <= PTT_L_CARRIER_DURATION_MAX));

		if (!is_pulse_duration_valid || !is_interval_valid ||
		    !is_transmission_duration_valid) {
			ret = PTT_RET_INVALID_VALUE;
			PTT_TRACE("%s: error occurs during arguments range checks: %d. Aborting",
				  __func__, ret);
		} else {
			/* arm timers */
			/* Transmission duration equal zero means infinite transmission */
			if (transmission_duration != 0) {
				ret = ptt_timer_add(transmission_duration,
						    cmd_uart_l_stream_duration_delay, uart_cmd_evt);
			}

			if (ret == PTT_RET_SUCCESS) {
				ret = ptt_timer_add(timings_info.pulse_duration,
						    cmd_uart_l_stream_pulse_delay, uart_cmd_evt);
			}

			if (ret != PTT_RET_SUCCESS) {
				PTT_TRACE("%s: error occurs during timers arming: %d. Aborting",
					  __func__, ret);
				ptt_timer_remove(uart_cmd_evt);
			} else {
				/* start sending stream */
				ret = cmd_uart_l_stream_start();

				if (ret != PTT_RET_SUCCESS) {
					PTT_TRACE("%s: cmd_uart_l_stream_start return %d. Aborting",
						  __func__, ret);
					ptt_timer_remove(uart_cmd_evt);
				} else {
					cmd_change_uart_cmd_state(CMD_UART_STATE_L_STREAM_PULSE);
				}
			}
		}
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static void cmd_uart_l_stream_duration_delay(ptt_evt_id_t timer_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(timer_evt_id);

	assert(cmd_is_uart_cmd_locked_by(timer_evt_id));

	enum ptt_ret ret = ptt_rf_stop_modulated_stream(uart_cmd_evt);

	if (ret != PTT_RET_SUCCESS) {
		PTT_TRACE("%s: WARNING ptt_rf_stop_modulated_stream returns error %d\n", __func__,
			  ret);
	}

	ptt_timer_remove(timer_evt_id);

	cmd_uart_cmd_unlock();

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
}

static void cmd_uart_l_stream_pulse_delay(ptt_evt_id_t timer_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(timer_evt_id);

	assert(cmd_is_uart_cmd_locked_by(timer_evt_id));

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	struct cmd_uart_waveform_timings_s *info =
		(struct cmd_uart_waveform_timings_s *)(ctx_data->arr);

	enum ptt_ret ret =
		ptt_timer_add(info->interval, cmd_uart_l_stream_interval_delay, uart_cmd_evt);

	if (ret == PTT_RET_SUCCESS) {
		ret = ptt_rf_stop_modulated_stream(uart_cmd_evt);

		if (ret == PTT_RET_SUCCESS) {
			cmd_change_uart_cmd_state(CMD_UART_STATE_L_STREAM_INTERVAL);
		} else {
			PTT_TRACE("%s: ptt_rf_stop_modulated_stream returns error: %d. Aborting",
				  __func__, ret);
		}
	} else {
		PTT_TRACE("%s: error occurs during timer arming: %d. Aborting command handling",
			  __func__, ret);
	}

	if (ret != PTT_RET_SUCCESS) {
		ptt_timer_remove(uart_cmd_evt);
		cmd_uart_cmd_unlock();
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
}

static void cmd_uart_l_stream_interval_delay(ptt_evt_id_t timer_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(timer_evt_id);

	assert(cmd_is_uart_cmd_locked_by(timer_evt_id));

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(uart_cmd_evt);

	assert(ctx_data != NULL);

	struct cmd_uart_waveform_timings_s *info =
		(struct cmd_uart_waveform_timings_s *)(ctx_data->arr);

	enum ptt_ret ret =
		ptt_timer_add(info->pulse_duration, cmd_uart_l_stream_pulse_delay, uart_cmd_evt);

	if (ret == PTT_RET_SUCCESS) {
		ret = cmd_uart_l_stream_start();

		if (ret == PTT_RET_SUCCESS) {
			cmd_change_uart_cmd_state(CMD_UART_STATE_L_STREAM_PULSE);
		} else {
			PTT_TRACE("%s: cmd_uart_l_stream_start returns error: %d. Aborting",
				  __func__, ret);
		}
	} else {
		PTT_TRACE("%s: error occurs during timer arming: %d. Aborting command handling",
			  __func__, ret);
	}

	if (ret != PTT_RET_SUCCESS) {
		ptt_timer_remove(uart_cmd_evt);
		cmd_uart_cmd_unlock();
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
}

static enum ptt_ret cmd_uart_l_sleep(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = ptt_rf_sleep();

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_l_receive(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = ptt_rf_receive();

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret cmd_uart_call_ota_cmd(enum ptt_cmd cmd, const struct ptt_evt_ctx_data_s *ctx)
{
	PTT_TRACE_FUNC_ENTER();

	ptt_evt_id_t new_ota_cmd;

	enum ptt_ret ret = ptt_event_alloc(&new_ota_cmd);

	if (ret == PTT_RET_SUCCESS) {
		ptt_event_set_cmd(new_ota_cmd, cmd);

		if (ctx != NULL) {
			memcpy(ptt_event_get_ctx_data(new_ota_cmd), ctx,
			       sizeof(struct ptt_evt_ctx_data_s));
		}

		ret = cmd_ota_cmd_process(new_ota_cmd);

		if (ret != PTT_RET_SUCCESS) {
			ptt_event_free(new_ota_cmd);
		}
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static inline void cmd_change_uart_cmd_state(ptt_evt_state_t state)
{
	PTT_TRACE("%s: %d", __func__, state);

	ptt_event_set_state(uart_cmd_evt, state);
}

static inline void cmd_uart_cmd_lock(ptt_evt_id_t new_uart_cmd)
{
	assert(uart_cmd_evt == PTT_EVENT_UNASSIGNED);

	uart_cmd_evt = new_uart_cmd;
	ptt_handler_busy();
}

static void cmd_uart_cmd_unlock(void)
{
	if (uart_cmd_evt != PTT_EVENT_UNASSIGNED) {
		ptt_event_free(uart_cmd_evt);
		uart_cmd_evt = PTT_EVENT_UNASSIGNED;
		ptt_handler_free();
		ptt_uart_print_prompt();
	}
}

static inline bool cmd_is_uart_cmd_locked(void)
{
	return (uart_cmd_evt == PTT_EVENT_UNASSIGNED) ? false : true;
}

static inline bool cmd_is_uart_cmd_locked_by(ptt_evt_id_t evt_id)
{
	return (evt_id == uart_cmd_evt) ? true : false;
}
