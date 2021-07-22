/* Copyright (c) 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Use in source and binary forms, redistribution in binary form only, with
 * or without modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 2. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 3. This software, with or without modification, must only be used with a Nordic
 *    Semiconductor ASA integrated circuit.
 *
 * 4. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
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

#if   defined(CMD_UART_UNIT_TEST)
#include "test_cmd_uart_conf.h"
#elif defined(CMD_RESPONSE_UNIT_TEST)
#include "test_cmd_response_conf.h"
#endif

#define CMD_UART_BUF 320

static void cmd_uart_send_rf_packet_with_num(ptt_evt_id_t new_rf_pkt_evt, uint32_t packet_n);

void cmd_uart_send_rsp_get_icache(ptt_evt_id_t evt_id)
{
    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(evt_id);

    assert(NULL != p_evt_data);

    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

    assert(NULL != p_ctx_data);

    sprintf(PTT_CAST_TO_STR(p_evt_data->arr), PTT_GET_ICACHE_MES "%d", p_ctx_data->arr[0]);

    ptt_uart_send_packet(p_evt_data->arr,
                         strlen(PTT_CAST_TO_STR(p_evt_data->arr)));
}

void cmd_uart_send_rsp_get_dcdc(ptt_evt_id_t evt_id)
{
    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(evt_id);

    assert(NULL != p_evt_data);

    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

    assert(NULL != p_ctx_data);

    sprintf(PTT_CAST_TO_STR(p_evt_data->arr), PTT_GET_DCDC_MES "%d", p_ctx_data->arr[0]);

    ptt_uart_send_packet(p_evt_data->arr,
                         strlen(PTT_CAST_TO_STR(p_evt_data->arr)));
}

void cmd_uart_send_rsp_get_temp(ptt_evt_id_t evt_id)
{
    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(evt_id);

    assert(NULL != p_evt_data);

    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

    assert(NULL != p_ctx_data);

    sprintf(PTT_CAST_TO_STR(p_evt_data->arr),
            "%d", *(int32_t *)p_ctx_data->arr);

    ptt_uart_send_packet(p_evt_data->arr,
                         strlen(PTT_CAST_TO_STR(p_evt_data->arr)));
}

void cmd_uart_send_rsp_antenna_error(ptt_evt_id_t evt_id)
{
    ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_GET_ANTENNA_ERROR_MES),
                         sizeof(PTT_GET_ANTENNA_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_antenna(ptt_evt_id_t evt_id)
{
    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(evt_id);

    assert(NULL != p_evt_data);

    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

    assert(NULL != p_ctx_data);

    sprintf(PTT_CAST_TO_STR(p_evt_data->arr), PTT_GET_ANTENNA_MES "%d", p_ctx_data->arr[0]);

    ptt_uart_send_packet(p_evt_data->arr,
                         strlen(PTT_CAST_TO_STR(p_evt_data->arr)));
}

static void cmd_uart_send_rf_packet_with_num(ptt_evt_id_t new_rf_pkt_evt, uint32_t packet_n)
{
    ptt_evt_data_t * p_rf_evt_data = ptt_event_get_p_data(new_rf_pkt_evt);

    assert(NULL != p_rf_evt_data);

    ptt_evt_ctx_data_t * p_rf_ctx_data = ptt_event_get_p_ctx_data(new_rf_pkt_evt);

    assert(NULL != p_rf_ctx_data);

    char                   out_str[CMD_UART_BUF] = {0};
    ptt_rf_packet_info_t * p_pkt_info            = (ptt_rf_packet_info_t *)(p_rf_ctx_data->arr);

    sprintf(out_str,
            "C:%d L:%d Data:",
            packet_n,
            p_rf_evt_data->len);

    uint16_t filled_length = strlen(out_str);

    /* we have each byte coded as two ASCII symbols */
    if ((p_rf_evt_data->len * 2) < (CMD_UART_BUF - filled_length))
    {
        for (int i = 0; i < p_rf_evt_data->len; ++i)
        {
            uint8_t lst = p_rf_evt_data->arr[i] & 0x0F;
            uint8_t mst = (p_rf_evt_data->arr[i] >> 4) & 0x0F;

            out_str[filled_length + 2 * i]     = (mst >= 0xA) ? mst - 0xA + 'A' : mst + '0';
            out_str[filled_length + 2 * i + 1] = (lst >= 0xA) ? lst - 0xA + 'A' : lst + '0';
        }

        /* let's use rf_evt_data as temporary buffer, as we copied everything from there */
        sprintf(PTT_CAST_TO_STR(p_rf_evt_data->arr),
                " CRC:1 LQI:0x%02x RSSI:%d",
                p_pkt_info->lqi,
                p_pkt_info->rssi);

        /* concatenate everything together */
        if ((filled_length + p_rf_evt_data->len * 2 + strlen(PTT_CAST_TO_STR(p_rf_evt_data->arr)) +
             sizeof('\0')) <= CMD_UART_BUF)
        {
            strcat(out_str, PTT_CAST_TO_STR(p_rf_evt_data->arr));
            ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(out_str),
                                 strlen(out_str));
        }
        else
        {
            PTT_TRACE("%s: response string is too long\n", __func__);
        }
    }
    else
    {
        PTT_TRACE("%s: there is not enough space to convert the array into ASCII\n", __func__);
    }
}

void cmd_uart_send_rsp_ltx_ack(ptt_evt_id_t evt_id, uint32_t packet_n)
{
    cmd_uart_send_rf_packet_with_num(evt_id, packet_n);
}

void cmd_uart_send_rsp_l_start_new_packet(ptt_evt_id_t new_rf_pkt_evt)
{
    ptt_rf_stat_t stat = ptt_rf_get_stat_report();

    cmd_uart_send_rf_packet_with_num(new_rf_pkt_evt, stat.total_pkts);
}

void cmd_uart_send_rsp_l_start_rx_fail(ptt_evt_id_t new_rf_pkt_evt)
{
    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(new_rf_pkt_evt);

    assert(NULL != p_ctx_data);

    ptt_rf_rx_error_t rx_error = *(ptt_rf_rx_error_t *)p_ctx_data->arr;

    if (PTT_RF_RX_ERROR_INVALID_FCS == rx_error)
    {
        ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_L_START_CRC_ERROR_MES),
                             sizeof(PTT_L_START_CRC_ERROR_MES) - 1);
    }
    else
    {
        ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_L_START_ERROR_MES),
                             sizeof(PTT_L_START_ERROR_MES) - 1);
    }
}

void cmd_uart_send_rsp_l_end(ptt_evt_id_t evt_id, uint32_t proto_pkts)
{
    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(evt_id);

    assert(NULL != p_evt_data);

    ptt_rf_stat_t stat = ptt_rf_get_stat_report();

    sprintf(PTT_CAST_TO_STR(p_evt_data->arr),
            "0x%08x 0x%08x 0x%08x 0x%08x",
            stat.total_pkts,
            proto_pkts,
            stat.total_lqi,
            stat.total_rssi);

    ptt_uart_send_packet(p_evt_data->arr,
                         strlen(PTT_CAST_TO_STR(p_evt_data->arr)));
}

void cmd_uart_send_rsp_lqi_failed(ptt_evt_id_t evt_id)
{
    ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_GET_LQI_ERROR_MES),
                         sizeof(PTT_GET_LQI_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_lqi_done(ptt_evt_id_t evt_id)
{
    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(evt_id);

    assert(NULL != p_evt_data);

    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

    assert(NULL != p_ctx_data);

    ptt_rf_packet_info_t * p_pkt_info = (ptt_rf_packet_info_t *)(p_ctx_data->arr);

    sprintf(PTT_CAST_TO_STR(p_evt_data->arr),
            PTT_GET_LQI_SUCCESS_MES "0x%04x",
            p_pkt_info->lqi);

    ptt_uart_send_packet(p_evt_data->arr,
                         strlen(PTT_CAST_TO_STR(p_evt_data->arr)));
}

void cmd_uart_send_rsp_ltx_failed(ptt_evt_id_t evt_id)
{
    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

    assert(NULL != p_ctx_data);

    ptt_rf_tx_error_t tx_error = *(ptt_rf_tx_error_t *)p_ctx_data->arr;

    switch (tx_error)
    {
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
    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(evt_id);

    assert(NULL != p_evt_data);

    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

    assert(NULL != p_ctx_data);

    sprintf(PTT_CAST_TO_STR(p_evt_data->arr),
            PTT_GET_RSSI_SUCCESS_MES "%d",
            *(ptt_rssi_t *)p_ctx_data->arr);

    ptt_uart_send_packet(p_evt_data->arr,
                         strlen(PTT_CAST_TO_STR(p_evt_data->arr)));
}

void cmd_uart_send_rsp_rssi_failed(ptt_evt_id_t evt_id)
{
    ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_GET_RSSI_ERROR_MES),
                         sizeof(PTT_GET_RSSI_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_ed_detected(ptt_evt_id_t evt_id)
{
    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(evt_id);

    assert(NULL != p_evt_data);

    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

    assert(NULL != p_ctx_data);

    sprintf(PTT_CAST_TO_STR(p_evt_data->arr),
            PTT_GET_ED_SUCCESS_MES "0x%04x",
            *(ptt_ed_t *)p_ctx_data->arr);

    ptt_uart_send_packet(p_evt_data->arr,
                         strlen(PTT_CAST_TO_STR(p_evt_data->arr)));
}

void cmd_uart_send_rsp_ed_failed(ptt_evt_id_t evt_id)
{
    ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_GET_ED_ERROR_MES),
                         sizeof(PTT_GET_ED_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_cca_done(ptt_evt_id_t evt_id)
{
    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(evt_id);

    assert(NULL != p_evt_data);

    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

    assert(NULL != p_ctx_data);

    sprintf(PTT_CAST_TO_STR(p_evt_data->arr),
            PTT_GET_CCA_SUCCESS_MES "%d",
            *(ptt_cca_t *)p_ctx_data->arr);

    ptt_uart_send_packet(p_evt_data->arr,
                         strlen(PTT_CAST_TO_STR(p_evt_data->arr)));
}

void cmd_uart_send_rsp_cca_failed(ptt_evt_id_t evt_id)
{
    ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_GET_CCA_ERROR_MES),
                         sizeof(PTT_GET_CCA_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_ack(ptt_evt_id_t evt_id)
{
    ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_PING_SUCCESS_MES),
                         sizeof(PTT_PING_SUCCESS_MES) - 1);
}

void cmd_uart_send_rsp_no_ack(ptt_evt_id_t evt_id)
{
    ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_PING_ERROR_MES), sizeof(PTT_PING_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_l_channel(ptt_evt_id_t evt_id)
{
    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(evt_id);

    assert(NULL != p_evt_data);

    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

    assert(NULL != p_ctx_data);

    sprintf(PTT_CAST_TO_STR(p_evt_data->arr), "%d", p_ctx_data->arr[0]);

    ptt_uart_send_packet(p_evt_data->arr,
                         strlen(PTT_CAST_TO_STR(p_evt_data->arr)));
}

void cmd_uart_send_rsp_power_error(ptt_evt_id_t evt_id)
{
    ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_GET_POWER_ERROR_MES),
                         sizeof(PTT_GET_POWER_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_power(ptt_evt_id_t evt_id)
{
    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(evt_id);

    assert(NULL != p_evt_data);

    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

    assert(NULL != p_ctx_data);

    /* @todo: figure out where to find mode */
    uint16_t mode = 0xffff;

    sprintf(PTT_CAST_TO_STR(p_evt_data->arr), "0x%04x 0x%02x", mode, p_ctx_data->arr[0]);

    ptt_uart_send_packet(p_evt_data->arr,
                         strlen(PTT_CAST_TO_STR(p_evt_data->arr)));
}

void cmd_uart_send_rsp_rx_test_error(ptt_evt_id_t evt_id)
{
    ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_END_RX_TEST_ERROR_MES),
                         sizeof(PTT_END_RX_TEST_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_rx_test(ptt_evt_id_t evt_id)
{
    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(evt_id);

    assert(NULL != p_evt_data);

    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

    assert(NULL != p_ctx_data);

    sprintf(PTT_CAST_TO_STR(p_evt_data->arr),
            "0x%02x%02x%02x%02x 0x%02x%02x%02x%02x 0x%02x%02x%02x%02x 0x%02x%02x%02x%02x",
            p_ctx_data->arr[0],
            p_ctx_data->arr[1],
            p_ctx_data->arr[2],
            p_ctx_data->arr[3],
            p_ctx_data->arr[4],
            p_ctx_data->arr[5],
            p_ctx_data->arr[6],
            p_ctx_data->arr[7],
            p_ctx_data->arr[8],
            p_ctx_data->arr[9],
            p_ctx_data->arr[10],
            p_ctx_data->arr[11],
            p_ctx_data->arr[12],
            p_ctx_data->arr[13],
            p_ctx_data->arr[14],
            p_ctx_data->arr[15]);

    ptt_uart_send_packet(p_evt_data->arr,
                         strlen(PTT_CAST_TO_STR(p_evt_data->arr)));
}

void cmd_uart_send_rsp_hw_error(ptt_evt_id_t evt_id)
{
    ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_GET_HARDWARE_ERROR_MES),
                         sizeof(PTT_GET_HARDWARE_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_hw(ptt_evt_id_t evt_id)
{
    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(evt_id);

    assert(NULL != p_evt_data);

    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

    assert(NULL != p_ctx_data);

    sprintf(PTT_CAST_TO_STR(p_evt_data->arr), "0x%02x", p_ctx_data->arr[0]);

    ptt_uart_send_packet(p_evt_data->arr,
                         strlen(PTT_CAST_TO_STR(p_evt_data->arr)));
}

void cmd_uart_send_rsp_sw_error(ptt_evt_id_t evt_id)
{
    ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_GET_SOFTWARE_ERROR_MES),
                         sizeof(PTT_GET_SOFTWARE_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_sw(ptt_evt_id_t evt_id)
{
    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(evt_id);

    assert(NULL != p_evt_data);

    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

    assert(NULL != p_ctx_data);

    sprintf(PTT_CAST_TO_STR(p_evt_data->arr), "0x%02x", p_ctx_data->arr[0]);

    ptt_uart_send_packet(p_evt_data->arr,
                         strlen(PTT_CAST_TO_STR(p_evt_data->arr)));
}

void cmd_uart_send_rsp_find(ptt_evt_id_t evt_id)
{
    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(evt_id);

    assert(NULL != p_evt_data);

    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

    assert(NULL != p_ctx_data);

    uint32_t mask = ptt_betoh32_val(p_ctx_data->arr);

    uint8_t channel = ptt_rf_convert_channel_mask_to_num(mask);

    sprintf(PTT_CAST_TO_STR(p_evt_data->arr), "channel %d find ack", channel);

    ptt_uart_send_packet(p_evt_data->arr,
                         strlen(PTT_CAST_TO_STR(p_evt_data->arr)));
}

void cmd_uart_send_rsp_find_timeout(ptt_evt_id_t evt_id)
{
    ptt_uart_send_packet(PTT_CAST_TO_UINT8_P(PTT_FIND_ERROR_MES), sizeof(PTT_FIND_ERROR_MES) - 1);
}

void cmd_uart_send_rsp_l_get_gpio(ptt_evt_id_t evt_id)
{
    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(evt_id);

    assert(NULL != p_evt_data);

    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

    assert(NULL != p_ctx_data);

    sprintf(PTT_CAST_TO_STR(p_evt_data->arr), "PIN %d: %d", p_ctx_data->arr[0], p_ctx_data->arr[1]);

    ptt_uart_send_packet(p_evt_data->arr,
                         strlen(PTT_CAST_TO_STR(p_evt_data->arr)));
}

void cmd_uart_send_rsp_l_get_gpio_error(ptt_evt_id_t evt_id)
{
    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(evt_id);

    assert(NULL != p_evt_data);

    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

    assert(NULL != p_ctx_data);

    sprintf(PTT_CAST_TO_STR(p_evt_data->arr), "PIN %d: ERROR", p_ctx_data->arr[0]);

    ptt_uart_send_packet(p_evt_data->arr,
                         strlen(PTT_CAST_TO_STR(p_evt_data->arr)));
}