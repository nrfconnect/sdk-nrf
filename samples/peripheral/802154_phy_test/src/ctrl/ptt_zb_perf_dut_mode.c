/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: implementation of Zigbee RF Performance Test Plan DUT mode */

#include <stddef.h>
#include <stdio.h>

#include "ctrl/ptt_trace.h"
#include "rf/ptt_rf.h"

#include "ptt_proto.h"

#include "ptt_ctrl_internal.h"
#include "ptt_events_internal.h"
#include "ptt_modes.h"
#include "ptt_zb_perf_dut_mode.h"

#ifdef TESTS
#include "test_dut_conf.h"
#endif

/** currently processed event */
static ptt_evt_id_t cur_evt;

/* current event handlers */
static void dut_rf_tx_finished(ptt_evt_id_t evt_id);
static void dut_rf_tx_failed(ptt_evt_id_t evt_id);
static void dut_idle_packet_proc(void);
static enum ptt_ret dut_ping(void);
static enum ptt_ret dut_set_channel(void);
static enum ptt_ret dut_set_power(void);
static enum ptt_ret dut_get_power(void);
static enum ptt_ret dut_stream(void);
static enum ptt_ret dut_start_rx_test(void);
static enum ptt_ret dut_hw_version(void);
static enum ptt_ret dut_sw_version(void);
static enum ptt_ret dut_set_antenna(void);
static enum ptt_ret dut_set_tx_antenna(void);
static enum ptt_ret dut_set_rx_antenna(void);
static enum ptt_ret dut_get_tx_antenna(void);
static enum ptt_ret dut_get_rx_antenna(void);
static enum ptt_ret dut_get_last_best_rx_antenna(void);

/* new event handlers */
static void dut_rf_rx_done(ptt_evt_id_t new_evt_id);
static void dut_end_rx_test(ptt_evt_id_t new_evt_id);

static void dut_stream_stop(ptt_evt_id_t timer_evt_id);

static ptt_pkt_len_t dut_form_stat_report(uint8_t *pkt, ptt_pkt_len_t pkt_max_size);

static inline void dut_change_cur_state(ptt_evt_state_t state);
static inline void dut_store_cur_evt(ptt_evt_id_t evt_id);
static inline void dut_reset_cur_evt(void);

#ifdef TESTS
#include "test_dut_wrappers.c"
#endif

enum ptt_ret ptt_zb_perf_dut_mode_init(void)
{
	PTT_TRACE_FUNC_ENTER();

	cur_evt = PTT_EVENT_UNASSIGNED;
	struct ptt_ext_evts_handlers_s *handlers = ptt_ctrl_get_handlers();

	handlers->rf_tx_finished = dut_rf_tx_finished;
	handlers->rf_tx_failed = dut_rf_tx_failed;
	handlers->rf_rx_done = dut_rf_rx_done;

	PTT_TRACE_FUNC_EXIT();
	return PTT_RET_SUCCESS;
}

enum ptt_ret ptt_zb_perf_dut_mode_uninit(void)
{
	PTT_TRACE_FUNC_ENTER();

	if (cur_evt != PTT_EVENT_UNASSIGNED) {
		dut_reset_cur_evt();
	}

	PTT_TRACE_FUNC_EXIT();
	return PTT_RET_SUCCESS;
}

static void dut_rf_rx_done(ptt_evt_id_t new_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(new_evt_id);

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(new_evt_id);

	assert(evt_data != NULL);

	if (!ptt_proto_check_packet(evt_data->arr, evt_data->len)) {
		PTT_TRACE("%s not protocol packet received and ignored\n", __func__);
		ptt_event_free(new_evt_id);
	} else {
		ptt_event_set_cmd(new_evt_id, evt_data->arr[PTT_CMD_CODE_START]);

		if (cur_evt == PTT_EVENT_UNASSIGNED) {
			/* store the event as currently processed */
			dut_store_cur_evt(new_evt_id);
			dut_idle_packet_proc();
		} else {
			switch (ptt_event_get_state(cur_evt)) {
			case DUT_STATE_RX_TEST_WAIT_FOR_END:
				dut_end_rx_test(new_evt_id);
				break;

			default:
				PTT_TRACE("%s: state isn't idle cmd %d state %d\n", __func__,
					  ptt_event_get_cmd(cur_evt), ptt_event_get_state(cur_evt));
				ptt_event_free(new_evt_id);
				break;
			}
		}
	}

	PTT_TRACE_FUNC_EXIT();
}

static void dut_idle_packet_proc(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;

	PTT_TRACE("%s: cmd %d state %d\n", __func__, ptt_event_get_cmd(cur_evt),
		  ptt_event_get_state(cur_evt));

	switch (ptt_event_get_cmd(cur_evt)) {
	case PTT_CMD_PING:
		ret = dut_ping();
		break;

	case PTT_CMD_SET_CHANNEL:
		ret = dut_set_channel();
		dut_reset_cur_evt();
		break;

	case PTT_CMD_SET_POWER:
		ret = dut_set_power();
		dut_reset_cur_evt();
		break;

	case PTT_CMD_GET_POWER:
		ret = dut_get_power();
		break;

	case PTT_CMD_STREAM:
		ret = dut_stream();
		break;

	case PTT_CMD_START_RX_TEST:
		ret = dut_start_rx_test();
		break;

	case PTT_CMD_GET_HARDWARE_VERSION:
		ret = dut_hw_version();
		break;

	case PTT_CMD_GET_SOFTWARE_VERSION:
		ret = dut_sw_version();
		break;

	case PTT_CMD_SET_ANTENNA:
		ret = dut_set_antenna();
		dut_reset_cur_evt();
		break;

	case PTT_CMD_SET_TX_ANTENNA:
		ret = dut_set_tx_antenna();
		dut_reset_cur_evt();
		break;

	case PTT_CMD_SET_RX_ANTENNA:
		ret = dut_set_rx_antenna();
		dut_reset_cur_evt();
		break;

	case PTT_CMD_GET_RX_ANTENNA:
		ret = dut_get_rx_antenna();
		break;

	case PTT_CMD_GET_TX_ANTENNA:
		ret = dut_get_tx_antenna();
		break;

	case PTT_CMD_GET_LAST_BEST_RX_ANTENNA:
		ret = dut_get_last_best_rx_antenna();
		break;

	default:
		PTT_TRACE("%s: unknown command cmd %d state %d\n", __func__,
			  ptt_event_get_cmd(cur_evt), ptt_event_get_state(cur_evt));
		dut_reset_cur_evt();
		break;
	}

	if (ret != PTT_RET_SUCCESS) {
		dut_reset_cur_evt();
	}

	PTT_TRACE_FUNC_EXIT();
}

static void dut_rf_tx_finished(ptt_evt_id_t evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

	assert(evt_id == cur_evt);

	PTT_TRACE("%s: cmd %d state %d\n", __func__, ptt_event_get_cmd(cur_evt),
		  ptt_event_get_state(cur_evt));

	switch (ptt_event_get_state(cur_evt)) {
	case DUT_STATE_ACK_SENDING:
	case DUT_STATE_POWER_SENDING:
	case DUT_STATE_RX_TEST_REPORT_SENDING:
	case DUT_STATE_HW_VERSION_SENDING:
	case DUT_STATE_SW_VERSION_SENDING:
	case DUT_STATE_ANTENNA_SENDING:
		dut_reset_cur_evt();
		break;

	default:
		PTT_TRACE("%s: inappropriate state cmd %d state %d\n", __func__,
			  ptt_event_get_cmd(cur_evt), ptt_event_get_state(cur_evt));
		break;
	}

	PTT_TRACE_FUNC_EXIT();
}

static void dut_rf_tx_failed(ptt_evt_id_t evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

	assert(evt_id == cur_evt);

	PTT_TRACE("%s: cmd %d state %d\n", __func__, ptt_event_get_cmd(cur_evt),
		  ptt_event_get_state(cur_evt));

	switch (ptt_event_get_state(cur_evt)) {
	case DUT_STATE_ACK_SENDING:
	case DUT_STATE_POWER_SENDING:
	case DUT_STATE_RX_TEST_REPORT_SENDING:
	case DUT_STATE_HW_VERSION_SENDING:
	case DUT_STATE_SW_VERSION_SENDING:
	case DUT_STATE_ANTENNA_SENDING:
		dut_reset_cur_evt();
		break;

	default:
		PTT_TRACE("%s: inappropriate state cmd %d state %d\n", __func__,
			  ptt_event_get_cmd(cur_evt), ptt_event_get_state(cur_evt));
		break;
	}

	PTT_TRACE_FUNC_EXIT();
}

static enum ptt_ret dut_ping(void)
{
	PTT_TRACE_FUNC_ENTER();

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(cur_evt);

	assert(evt_data != NULL);

	uint8_t *p = evt_data->arr;

	dut_change_cur_state(DUT_STATE_ACK_SENDING);

	evt_data->len = ptt_proto_construct_header(p, PTT_CMD_ACK, PTT_EVENT_DATA_SIZE);

	enum ptt_ret ret = ptt_rf_send_packet(cur_evt, evt_data->arr, evt_data->len);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static enum ptt_ret dut_set_channel(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(cur_evt);

	assert(evt_data != NULL);

	if ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_SET_CHANNEL) != evt_data->len) {
		PTT_TRACE("%s: invalid length %d\n", __func__, evt_data->len);
		ret = PTT_RET_INVALID_COMMAND;
	} else {
		uint32_t mask = ptt_betoh32_val(&evt_data->arr[PTT_PAYLOAD_START]);

		ret = ptt_rf_set_channel_mask(cur_evt, mask);
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static enum ptt_ret dut_set_power(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(cur_evt);

	assert(evt_data != NULL);

	if ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_SET_POWER) != evt_data->len) {
		PTT_TRACE("%s: invalid length %d\n", __func__, evt_data->len);
		ret = PTT_RET_INVALID_COMMAND;
	} else {
		int8_t power = (int8_t)(evt_data->arr[PTT_PAYLOAD_START]);

		ret = ptt_rf_set_power(cur_evt, power);
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static enum ptt_ret dut_get_power(void)
{
	PTT_TRACE_FUNC_ENTER();

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(cur_evt);

	assert(evt_data != NULL);

	uint8_t *p = evt_data->arr;

	dut_change_cur_state(DUT_STATE_POWER_SENDING);

	evt_data->len =
		ptt_proto_construct_header(p, PTT_CMD_GET_POWER_RESPONSE, PTT_EVENT_DATA_SIZE);
	p[evt_data->len] = ptt_rf_get_power();
	evt_data->len++;

	enum ptt_ret ret = ptt_rf_send_packet(cur_evt, evt_data->arr, evt_data->len);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static enum ptt_ret dut_set_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(cur_evt);

	assert(evt_data != NULL);

	if ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_SET_ANTENNA) != evt_data->len) {
		PTT_TRACE("%s: invalid length %d\n", __func__, evt_data->len);
		ret = PTT_RET_INVALID_COMMAND;
	} else {
		uint8_t antenna = evt_data->arr[PTT_PAYLOAD_START];

		ret = ptt_rf_set_antenna(cur_evt, antenna);
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret dut_set_rx_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(cur_evt);

	assert(evt_data != NULL);

	if ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_SET_ANTENNA) != evt_data->len) {
		PTT_TRACE("%s: invalid length %d\n", __func__, evt_data->len);
		ret = PTT_RET_INVALID_COMMAND;
	} else {
		uint8_t antenna = evt_data->arr[PTT_PAYLOAD_START];

		ret = ptt_rf_set_rx_antenna(cur_evt, antenna);
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret dut_set_tx_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(cur_evt);

	assert(evt_data != NULL);

	if ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_SET_ANTENNA) != evt_data->len) {
		PTT_TRACE("%s: invalid length %d\n", __func__, evt_data->len);
		ret = PTT_RET_INVALID_COMMAND;
	} else {
		uint8_t antenna = evt_data->arr[PTT_PAYLOAD_START];

		ret = ptt_rf_set_tx_antenna(cur_evt, antenna);
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret dut_get_rx_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(cur_evt);

	assert(evt_data != NULL);

	uint8_t *p = evt_data->arr;

	dut_change_cur_state(DUT_STATE_ANTENNA_SENDING);

	evt_data->len =
		ptt_proto_construct_header(p, PTT_CMD_GET_ANTENNA_RESPONSE, PTT_EVENT_DATA_SIZE);
	p[evt_data->len] = ptt_rf_get_rx_antenna();
	evt_data->len++;

	enum ptt_ret ret = ptt_rf_send_packet(cur_evt, evt_data->arr, evt_data->len);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret dut_get_tx_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(cur_evt);

	assert(evt_data != NULL);

	uint8_t *p = evt_data->arr;

	dut_change_cur_state(DUT_STATE_ANTENNA_SENDING);

	evt_data->len =
		ptt_proto_construct_header(p, PTT_CMD_GET_ANTENNA_RESPONSE, PTT_EVENT_DATA_SIZE);
	p[evt_data->len] = ptt_rf_get_tx_antenna();
	evt_data->len++;

	enum ptt_ret ret = ptt_rf_send_packet(cur_evt, evt_data->arr, evt_data->len);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret dut_get_last_best_rx_antenna(void)
{
	PTT_TRACE_FUNC_ENTER();

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(cur_evt);

	assert(evt_data != NULL);

	uint8_t *p = evt_data->arr;

	dut_change_cur_state(DUT_STATE_ANTENNA_SENDING);

	evt_data->len =
		ptt_proto_construct_header(p, PTT_CMD_GET_ANTENNA_RESPONSE, PTT_EVENT_DATA_SIZE);

	p[evt_data->len] = ptt_rf_get_last_rx_best_antenna();
	evt_data->len++;

	enum ptt_ret ret = ptt_rf_send_packet(cur_evt, evt_data->arr, evt_data->len);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
	return ret;
}

static enum ptt_ret dut_stream(void)
{
	PTT_TRACE_FUNC_ENTER();

	enum ptt_ret ret = PTT_RET_SUCCESS;

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(cur_evt);

	assert(evt_data != NULL);

	if ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_STREAM) != evt_data->len) {
		PTT_TRACE("%s: invalid length %d\n", __func__, evt_data->len);
		ret = PTT_RET_INVALID_COMMAND;
	} else {
		uint16_t duration = ptt_betoh16_val(&evt_data->arr[PTT_PAYLOAD_START]);

		if (duration != 0) {
			ret = ptt_timer_add(duration, dut_stream_stop, cur_evt);

			if (ret == PTT_RET_SUCCESS) {
				struct ptt_ltx_payload_s *ltx_payload =
					ptt_rf_get_custom_ltx_payload();

				ltx_payload->len = PTT_CUSTOM_LTX_PAYLOAD_MAX_SIZE;

				ptt_random_vector_generate(ltx_payload->arr, ltx_payload->len);

				ret = ptt_rf_start_modulated_stream(cur_evt, ltx_payload->arr,
								    ltx_payload->len);

				if (ret == PTT_RET_SUCCESS) {
					dut_change_cur_state(DUT_STATE_STREAM_EMITTING);
				} else {
					ptt_timer_remove(cur_evt);
					PTT_TRACE("%s: start_modulated_stream returns %d. Aborting",
						  __func__, ret);
				}
			}
		} else {
			dut_reset_cur_evt();
		}
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static void dut_stream_stop(ptt_evt_id_t timer_evt_id)
{
	PTT_TRACE_FUNC_ENTER();

	assert(timer_evt_id == cur_evt);

	enum ptt_ret ret = ptt_rf_stop_modulated_stream(cur_evt);

	dut_reset_cur_evt();

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
}

static enum ptt_ret dut_start_rx_test(void)
{
	PTT_TRACE_FUNC_ENTER();

	/* we will store protocol packets counter in context of cur_evt,
	 * lets erase it just in case
	 */
	struct ptt_evt_ctx_data_s *cur_ctx_data = ptt_event_get_ctx_data(cur_evt);

	assert(cur_ctx_data != NULL);

	uint32_t *proto_pkts = (uint32_t *)cur_ctx_data;

	*proto_pkts = 0;

	enum ptt_ret ret = ptt_rf_start_statistic(cur_evt);

	if (ret != PTT_RET_SUCCESS) {
		PTT_TRACE("%s: ret %d\n", __func__, ret);
		ret = PTT_RET_INVALID_COMMAND;
	} else {
		dut_change_cur_state(DUT_STATE_RX_TEST_WAIT_FOR_END);
	}

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static void dut_end_rx_test(ptt_evt_id_t new_evt_id)
{
	PTT_TRACE_FUNC_ENTER_WITH_EVT(new_evt_id);

	enum ptt_ret ret = PTT_RET_SUCCESS;

	struct ptt_evt_ctx_data_s *cur_ctx_data = ptt_event_get_ctx_data(cur_evt);

	assert(cur_ctx_data != NULL);

	uint32_t *proto_pkts = (uint32_t *)cur_ctx_data;

	++(*proto_pkts);

	if (ptt_event_get_cmd(new_evt_id) == PTT_CMD_END_RX_TEST) {
		ret = ptt_rf_end_statistic(cur_evt);

		if (ret != PTT_RET_SUCCESS) {
			PTT_TRACE("%s: ret %d\n", __func__, ret);
		} else {
			struct ptt_evt_data_s *evt_data = ptt_event_get_data(cur_evt);

			assert(evt_data != NULL);

			uint8_t *p = evt_data->arr;

			dut_change_cur_state(DUT_STATE_RX_TEST_REPORT_SENDING);

			evt_data->len =
				ptt_proto_construct_header(p, PTT_CMD_REPORT, PTT_EVENT_DATA_SIZE);
			evt_data->len += dut_form_stat_report(&p[evt_data->len],
							      PTT_EVENT_DATA_SIZE - evt_data->len);

			ret = ptt_rf_send_packet(cur_evt, evt_data->arr, evt_data->len);
		}
	}

	ptt_event_free(new_evt_id);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
}

static enum ptt_ret dut_hw_version(void)
{
	PTT_TRACE_FUNC_ENTER();

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(cur_evt);

	assert(evt_data != NULL);

	uint8_t *p = evt_data->arr;

	dut_change_cur_state(DUT_STATE_HW_VERSION_SENDING);

	evt_data->len = ptt_proto_construct_header(p, PTT_CMD_GET_HARDWARE_VERSION_RESPONSE,
						   PTT_EVENT_DATA_SIZE);
	p[evt_data->len] = ptt_ctrl_get_hw_version();
	evt_data->len++;

	enum ptt_ret ret = ptt_rf_send_packet(cur_evt, evt_data->arr, evt_data->len);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static enum ptt_ret dut_sw_version(void)
{
	PTT_TRACE_FUNC_ENTER();

	struct ptt_evt_data_s *evt_data = ptt_event_get_data(cur_evt);

	assert(evt_data != NULL);

	uint8_t *p = evt_data->arr;

	dut_change_cur_state(DUT_STATE_SW_VERSION_SENDING);

	evt_data->len = ptt_proto_construct_header(p, PTT_CMD_GET_SOFTWARE_VERSION_RESPONSE,
						   PTT_EVENT_DATA_SIZE);
	p[evt_data->len] = ptt_ctrl_get_sw_version();
	evt_data->len++;

	enum ptt_ret ret = ptt_rf_send_packet(cur_evt, evt_data->arr, evt_data->len);

	PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

	return ret;
}

static ptt_pkt_len_t dut_form_stat_report(uint8_t *pkt, ptt_pkt_len_t pkt_max_size)
{
	assert(pkt != NULL);
	assert(pkt_max_size >= PTT_PAYLOAD_LEN_REPORT);

	ptt_pkt_len_t len = 0;
	uint8_t *p = pkt;
	struct ptt_rf_stat_s stat = ptt_rf_get_stat_report();

	struct ptt_evt_ctx_data_s *cur_ctx_data = ptt_event_get_ctx_data(cur_evt);

	assert(cur_ctx_data != NULL);

	uint32_t *proto_pkts = (uint32_t *)cur_ctx_data;

	ptt_htobe32((uint8_t *)&stat.total_pkts, &p[len]);
	len += sizeof(stat.total_pkts);

	ptt_htobe32((uint8_t *)proto_pkts, &p[len]);
	len += sizeof(*proto_pkts);

	ptt_htobe32((uint8_t *)&stat.total_lqi, &p[len]);
	len += sizeof(stat.total_lqi);

	ptt_htobe32((uint8_t *)&stat.total_rssi, &p[len]);
	len += sizeof(stat.total_rssi);

	assert(len == PTT_PAYLOAD_LEN_REPORT);

	return len;
}

static inline void dut_change_cur_state(ptt_evt_state_t state)
{
	PTT_TRACE("%s: state %d", __func__, state);

	ptt_event_set_state(cur_evt, state);
}

static inline void dut_store_cur_evt(ptt_evt_id_t evt_id)
{
	assert(cur_evt == PTT_EVENT_UNASSIGNED);

	cur_evt = evt_id;
}

static inline void dut_reset_cur_evt(void)
{
	ptt_event_free(cur_evt);
	cur_evt = PTT_EVENT_UNASSIGNED;
}
