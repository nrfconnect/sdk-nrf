/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: UART response constructing for Zigbee RF Performance Test Plan CMD mode */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "ctrl/ptt_trace.h"
#include "ctrl/ptt_uart.h"
#include "rf/ptt_rf.h"

#include "ptt_proto.h"

#include "ptt_ctrl_internal.h"
#include "ptt_events_internal.h"
#include "ptt_zb_perf_cmd_mode.h"

#if defined(CMD_UART_UNIT_TEST)
#include "test_cmd_uart_conf.h"
#elif defined(CMD_RESPONSE_UNIT_TEST)
#include "test_cmd_response_conf.h"
#endif

#define CMD_UART_BUF 320

static void cmd_uart_send_rf_packet_with_num(ptt_evt_id_t new_rf_pkt_evt, uint32_t packet_n);

void cmd_uart_send_rsp_get_icache(ptt_evt_id_t evt_id)
{
	struct ptt_evt_data_s *evt_data = ptt_event_get_data(evt_id);

	assert(evt_data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

	assert(ctx_data != NULL);

	sprintf(PTT_CAST_TO_STR(evt_data->arr), PTT_GET_ICACHE_MES "%d", ctx_data->arr[0]);

	ptt_uart_send_packet(evt_data->arr, strlen(PTT_CAST_TO_STR(evt_data->arr)));
}

void cmd_uart_send_rsp_get_dcdc(ptt_evt_id_t evt_id)
{
	struct ptt_evt_data_s *evt_data = ptt_event_get_data(evt_id);

	assert(evt_data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

	assert(ctx_data != NULL);

	sprintf(PTT_CAST_TO_STR(evt_data->arr), PTT_GET_DCDC_MES "%d", ctx_data->arr[0]);

	ptt_uart_send_packet(evt_data->arr, strlen(PTT_CAST_TO_STR(evt_data->arr)));
}

void cmd_uart_send_rsp_get_temp(ptt_evt_id_t evt_id)
{
	struct ptt_evt_data_s *evt_data = ptt_event_get_data(evt_id);

	assert(evt_data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

	assert(ctx_data != NULL);

	sprintf(PTT_CAST_TO_STR(evt_data->arr), "%d", *(int32_t *)ctx_data->arr);

	ptt_uart_send_packet(evt_data->arr, strlen(PTT_CAST_TO_STR(evt_data->arr)));
}

void cmd_uart_send_rsp_antenna_error(ptt_evt_id_t evt_id)
{
	ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_GET_ANTENNA_ERROR_MES),
			     sizeof(PTT_GET_ANTENNA_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_antenna(ptt_evt_id_t evt_id)
{
	struct ptt_evt_data_s *evt_data = ptt_event_get_data(evt_id);

	assert(evt_data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

	assert(ctx_data != NULL);

	sprintf(PTT_CAST_TO_STR(evt_data->arr), PTT_GET_ANTENNA_MES "%d", ctx_data->arr[0]);

	ptt_uart_send_packet(evt_data->arr, strlen(PTT_CAST_TO_STR(evt_data->arr)));
}

static void cmd_uart_send_rf_packet_with_num(ptt_evt_id_t new_rf_pkt_evt, uint32_t packet_n)
{
	struct ptt_evt_data_s *rf_evt_data = ptt_event_get_data(new_rf_pkt_evt);

	assert(rf_evt_data != NULL);

	struct ptt_evt_ctx_data_s *rf_ctx_data = ptt_event_get_ctx_data(new_rf_pkt_evt);

	assert(rf_ctx_data != NULL);

	char out_str[CMD_UART_BUF] = { 0 };
	struct ptt_rf_packet_info_s *pkt_info = (struct ptt_rf_packet_info_s *)(rf_ctx_data->arr);

	sprintf(out_str, "C:%d L:%d Data:", packet_n, rf_evt_data->len);

	uint16_t filled_length = strlen(out_str);

	/* we have each byte coded as two ASCII symbols */
	if ((rf_evt_data->len * 2) < (CMD_UART_BUF - filled_length)) {
		for (int i = 0; i < rf_evt_data->len; ++i) {
			uint8_t lst = rf_evt_data->arr[i] & 0x0F;
			uint8_t mst = (rf_evt_data->arr[i] >> 4) & 0x0F;

			out_str[filled_length + 2 * i] = (mst >= 0xA) ? mst - 0xA + 'A' : mst + '0';
			out_str[filled_length + 2 * i + 1] =
				(lst >= 0xA) ? lst - 0xA + 'A' : lst + '0';
		}

		/* let's use rf_evt_data as temporary buffer, as we copied everything from there */
		sprintf(PTT_CAST_TO_STR(rf_evt_data->arr), " CRC:1 LQI:0x%02x RSSI:%d",
			pkt_info->lqi, pkt_info->rssi);

		/* concatenate everything together */
		if ((filled_length + rf_evt_data->len * 2 +
		     strlen(PTT_CAST_TO_STR(rf_evt_data->arr)) + sizeof('\0')) <= CMD_UART_BUF) {
			strcat(out_str, PTT_CAST_TO_STR(rf_evt_data->arr));
			ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(out_str), strlen(out_str));
		} else {
			PTT_TRACE("%s: response string is too long\n", __func__);
		}
	} else {
		PTT_TRACE("%s: there is not enough space to convert the array into ASCII\n",
			  __func__);
	}
}

void cmd_uart_send_rsp_ltx_ack(ptt_evt_id_t evt_id, uint32_t packet_n)
{
	cmd_uart_send_rf_packet_with_num(evt_id, packet_n);
}

void cmd_uart_send_rsp_l_start_new_packet(ptt_evt_id_t new_rf_pkt_evt)
{
	struct ptt_rf_stat_s stat = ptt_rf_get_stat_report();

	cmd_uart_send_rf_packet_with_num(new_rf_pkt_evt, stat.total_pkts);
}

void cmd_uart_send_rsp_l_start_rx_fail(ptt_evt_id_t new_rf_pkt_evt)
{
	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(new_rf_pkt_evt);

	assert(ctx_data != NULL);

	ptt_rf_rx_error_t rx_error = *(ptt_rf_rx_error_t *)ctx_data->arr;

	if (rx_error == PTT_RF_RX_ERROR_INVALID_FCS) {
		ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_L_START_CRC_ERROR_MES),
				     sizeof(PTT_L_START_CRC_ERROR_MES) - 1);
	} else {
		ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_L_START_ERROR_MES),
				     sizeof(PTT_L_START_ERROR_MES) - 1);
	}
}

void cmd_uart_send_rsp_l_end(ptt_evt_id_t evt_id, uint32_t proto_pkts)
{
	struct ptt_evt_data_s *evt_data = ptt_event_get_data(evt_id);

	assert(evt_data != NULL);

	struct ptt_rf_stat_s stat = ptt_rf_get_stat_report();

	sprintf(PTT_CAST_TO_STR(evt_data->arr), "0x%08x 0x%08x 0x%08x 0x%08x", stat.total_pkts,
		proto_pkts, stat.total_lqi, stat.total_rssi);

	ptt_uart_send_packet(evt_data->arr, strlen(PTT_CAST_TO_STR(evt_data->arr)));
}

void cmd_uart_send_rsp_lqi_failed(ptt_evt_id_t evt_id)
{
	ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_GET_LQI_ERROR_MES),
			     sizeof(PTT_GET_LQI_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_lqi_done(ptt_evt_id_t evt_id)
{
	struct ptt_evt_data_s *evt_data = ptt_event_get_data(evt_id);

	assert(evt_data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

	assert(ctx_data != NULL);

	struct ptt_rf_packet_info_s *pkt_info = (struct ptt_rf_packet_info_s *)(ctx_data->arr);

	sprintf(PTT_CAST_TO_STR(evt_data->arr), PTT_GET_LQI_SUCCESS_MES "0x%04x", pkt_info->lqi);

	ptt_uart_send_packet(evt_data->arr, strlen(PTT_CAST_TO_STR(evt_data->arr)));
}

void cmd_uart_send_rsp_ltx_failed(ptt_evt_id_t evt_id)
{
	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

	assert(ctx_data != NULL);

	ptt_rf_tx_error_t tx_error = *(ptt_rf_tx_error_t *)ctx_data->arr;

	switch (tx_error) {
	case PTT_RF_TX_ERROR_INVALID_ACK_FCS:
		ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_L_TX_CRC_ERROR_MES),
				     sizeof(PTT_L_TX_CRC_ERROR_MES) - 1);
		break;

	case PTT_RF_TX_ERROR_NO_ACK:
		ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_L_TX_NO_ACK_MES),
				     sizeof(PTT_L_TX_NO_ACK_MES) - 1);
		break;

	default:
		ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_L_TX_ERROR_MES),
				     sizeof(PTT_L_TX_ERROR_MES) - 1);
		break;
	}
}

void cmd_uart_send_rsp_rssi_done(ptt_evt_id_t evt_id)
{
	struct ptt_evt_data_s *evt_data = ptt_event_get_data(evt_id);

	assert(evt_data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

	assert(ctx_data != NULL);

	sprintf(PTT_CAST_TO_STR(evt_data->arr), PTT_GET_RSSI_SUCCESS_MES "%d",
		*(ptt_rssi_t *)ctx_data->arr);

	ptt_uart_send_packet(evt_data->arr, strlen(PTT_CAST_TO_STR(evt_data->arr)));
}

void cmd_uart_send_rsp_rssi_failed(ptt_evt_id_t evt_id)
{
	ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_GET_RSSI_ERROR_MES),
			     sizeof(PTT_GET_RSSI_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_ed_detected(ptt_evt_id_t evt_id)
{
	struct ptt_evt_data_s *evt_data = ptt_event_get_data(evt_id);

	assert(evt_data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

	assert(ctx_data != NULL);

	sprintf(PTT_CAST_TO_STR(evt_data->arr), PTT_GET_ED_SUCCESS_MES "0x%04x",
		*(ptt_ed_t *)ctx_data->arr);

	ptt_uart_send_packet(evt_data->arr, strlen(PTT_CAST_TO_STR(evt_data->arr)));
}

void cmd_uart_send_rsp_ed_failed(ptt_evt_id_t evt_id)
{
	ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_GET_ED_ERROR_MES),
			     sizeof(PTT_GET_ED_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_cca_done(ptt_evt_id_t evt_id)
{
	struct ptt_evt_data_s *evt_data = ptt_event_get_data(evt_id);

	assert(evt_data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

	assert(ctx_data != NULL);

	sprintf(PTT_CAST_TO_STR(evt_data->arr), PTT_GET_CCA_SUCCESS_MES "%d",
		*(bool *)ctx_data->arr);

	ptt_uart_send_packet(evt_data->arr, strlen(PTT_CAST_TO_STR(evt_data->arr)));
}

void cmd_uart_send_rsp_cca_failed(ptt_evt_id_t evt_id)
{
	ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_GET_CCA_ERROR_MES),
			     sizeof(PTT_GET_CCA_ERROR_MES) - 1);
}

void cmd_uart_send_rsack(ptt_evt_id_t evt_id)
{
	ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_PING_SUCCESS_MES),
			     sizeof(PTT_PING_SUCCESS_MES) - 1);
}

void cmd_uart_send_rsp_no_ack(ptt_evt_id_t evt_id)
{
	ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_PING_ERROR_MES),
			     sizeof(PTT_PING_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_l_channel(ptt_evt_id_t evt_id)
{
	struct ptt_evt_data_s *evt_data = ptt_event_get_data(evt_id);

	assert(evt_data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

	assert(ctx_data != NULL);

	sprintf(PTT_CAST_TO_STR(evt_data->arr), "%d", ctx_data->arr[0]);

	ptt_uart_send_packet(evt_data->arr, strlen(PTT_CAST_TO_STR(evt_data->arr)));
}

void cmd_uart_send_rsp_power_error(ptt_evt_id_t evt_id)
{
	ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_GET_POWER_ERROR_MES),
			     sizeof(PTT_GET_POWER_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_power(ptt_evt_id_t evt_id)
{
	struct ptt_evt_data_s *evt_data = ptt_event_get_data(evt_id);

	assert(evt_data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

	assert(ctx_data != NULL);

	/* @todo: figure out where to find mode */
	uint16_t mode = 0xffff;

	sprintf(PTT_CAST_TO_STR(evt_data->arr), "0x%04x 0x%02x", mode, ctx_data->arr[0]);

	ptt_uart_send_packet(evt_data->arr, strlen(PTT_CAST_TO_STR(evt_data->arr)));
}

void cmd_uart_send_rsp_rx_test_error(ptt_evt_id_t evt_id)
{
	ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_END_RX_TEST_ERROR_MES),
			     sizeof(PTT_END_RX_TEST_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_rx_test(ptt_evt_id_t evt_id)
{
	struct ptt_evt_data_s *evt_data = ptt_event_get_data(evt_id);

	assert(evt_data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

	assert(ctx_data != NULL);

	sprintf(PTT_CAST_TO_STR(evt_data->arr),
		"0x%02x%02x%02x%02x 0x%02x%02x%02x%02x 0x%02x%02x%02x%02x 0x%02x%02x%02x%02x",
		ctx_data->arr[0], ctx_data->arr[1], ctx_data->arr[2], ctx_data->arr[3],
		ctx_data->arr[4], ctx_data->arr[5], ctx_data->arr[6], ctx_data->arr[7],
		ctx_data->arr[8], ctx_data->arr[9], ctx_data->arr[10], ctx_data->arr[11],
		ctx_data->arr[12], ctx_data->arr[13], ctx_data->arr[14], ctx_data->arr[15]);

	ptt_uart_send_packet(evt_data->arr, strlen(PTT_CAST_TO_STR(evt_data->arr)));
}

void cmd_uart_send_rsp_hw_error(ptt_evt_id_t evt_id)
{
	ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_GET_HARDWARE_ERROR_MES),
			     sizeof(PTT_GET_HARDWARE_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_hw(ptt_evt_id_t evt_id)
{
	struct ptt_evt_data_s *evt_data = ptt_event_get_data(evt_id);

	assert(evt_data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

	assert(ctx_data != NULL);

	sprintf(PTT_CAST_TO_STR(evt_data->arr), "0x%02x", ctx_data->arr[0]);

	ptt_uart_send_packet(evt_data->arr, strlen(PTT_CAST_TO_STR(evt_data->arr)));
}

void cmd_uart_send_rsp_sw_error(ptt_evt_id_t evt_id)
{
	ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_GET_SOFTWARE_ERROR_MES),
			     sizeof(PTT_GET_SOFTWARE_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_sw(ptt_evt_id_t evt_id)
{
	struct ptt_evt_data_s *evt_data = ptt_event_get_data(evt_id);

	assert(evt_data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

	assert(ctx_data != NULL);

	sprintf(PTT_CAST_TO_STR(evt_data->arr), "0x%02x", ctx_data->arr[0]);

	ptt_uart_send_packet(evt_data->arr, strlen(PTT_CAST_TO_STR(evt_data->arr)));
}

void cmd_uart_send_rsp_find(ptt_evt_id_t evt_id)
{
	struct ptt_evt_data_s *evt_data = ptt_event_get_data(evt_id);

	assert(evt_data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

	assert(ctx_data != NULL);

	uint32_t mask = ptt_betoh32_val(ctx_data->arr);

	uint8_t channel = ptt_rf_convert_channel_mask_to_num(mask);

	sprintf(PTT_CAST_TO_STR(evt_data->arr), "channel %d find ack", channel);

	ptt_uart_send_packet(evt_data->arr, strlen(PTT_CAST_TO_STR(evt_data->arr)));
}

void cmd_uart_send_rsp_find_timeout(ptt_evt_id_t evt_id)
{
	ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_FIND_ERROR_MES),
			     sizeof(PTT_FIND_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_l_get_gpio(ptt_evt_id_t evt_id)
{
	struct ptt_evt_data_s *evt_data = ptt_event_get_data(evt_id);

	assert(evt_data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

	assert(ctx_data != NULL);

	sprintf(PTT_CAST_TO_STR(evt_data->arr), "PIN %d: %d", ctx_data->arr[0], ctx_data->arr[1]);

	ptt_uart_send_packet(evt_data->arr, strlen(PTT_CAST_TO_STR(evt_data->arr)));
}

void cmd_uart_send_rsp_l_get_gpio_error(ptt_evt_id_t evt_id)
{
	struct ptt_evt_data_s *evt_data = ptt_event_get_data(evt_id);

	assert(evt_data != NULL);

	struct ptt_evt_ctx_data_s *ctx_data = ptt_event_get_ctx_data(evt_id);

	assert(ctx_data != NULL);

	sprintf(PTT_CAST_TO_STR(evt_data->arr), "PIN %d: ERROR", ctx_data->arr[0]);

	ptt_uart_send_packet(evt_data->arr, strlen(PTT_CAST_TO_STR(evt_data->arr)));
}
