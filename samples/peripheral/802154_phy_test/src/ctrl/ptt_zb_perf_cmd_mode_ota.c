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

/* Purpose: OTA commands processing of Zigbee RF Performance Test Plan CMD mode */

/* The module processes an event containing OTA command from UART module and
   returns the event with result. The event locks the module as currently processed OTA command
   and the module rejects any other commands while processing the one.
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
static ptt_evt_id_t m_ota_cmd_evt;

/* current event handlers */
static ptt_ret_t cmd_ota_cmd_proc(void);
static ptt_ret_t cmd_set_channel(void);
static ptt_ret_t cmd_set_power(void);
static ptt_ret_t cmd_ping(void);
static void cmd_ping_sent(void);
static ptt_ret_t cmd_get_power(void);
static void cmd_get_power_sent(void);
static ptt_ret_t cmd_end_rx_test(void);
static void cmd_end_rx_test_sent(void);
static ptt_ret_t cmd_get_hardware_version(void);
static void cmd_get_hardware_sent(void);
static ptt_ret_t cmd_get_software_version(void);
static void cmd_get_software_sent(void);
static ptt_ret_t cmd_stream(void);
static ptt_ret_t cmd_start_rx_test(void);
static ptt_ret_t cmd_set_antenna(void);
static ptt_ret_t cmd_get_antenna(void);
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
static void cmd_fill_ctx_end_and_reset(const uint8_t * p_data, ptt_pkt_len_t len);
static void cmd_send_finish_and_reset(void);
static void cmd_send_timeout_and_reset(void);

static ptt_ret_t cmd_make_and_send_rf_packet(ptt_cmd_t cmd);

/* OTA command lock routines */
static inline void cmd_ota_cmd_lock(ptt_evt_id_t new_ota_cmd);
static inline void cmd_change_ota_cmd_state(ptt_evt_state_t state);
static inline ptt_evt_id_t cmd_get_ota_cmd_and_reset(void);
static inline ptt_bool_t cmd_is_ota_cmd_locked(void);
static inline ptt_bool_t cmd_is_ota_cmd_locked_by(ptt_evt_id_t evt_id);
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

ptt_ret_t cmd_ota_cmd_process(ptt_evt_id_t new_ota_cmd)
{
    PTT_TRACE_FUNC_ENTER();

    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (false == cmd_is_ota_cmd_locked())
    {
        /* store event as currently processed OTA command */
        cmd_ota_cmd_lock(new_ota_cmd);
        ret = cmd_ota_cmd_proc();
        if (ret != PTT_RET_SUCCESS)
        {
            cmd_ota_cmd_unlock();
        }
    }
    else
    {
        PTT_TRACE("%s: state isn't idle cmd %d state %d\n",
                  __func__,
                  ptt_event_get_cmd(m_ota_cmd_evt),
                  ptt_event_get_state(m_ota_cmd_evt));
        ret = PTT_RET_BUSY;
    }

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
    return ret;
}

static ptt_ret_t cmd_ota_cmd_proc(void)
{
    PTT_TRACE_FUNC_ENTER();

    ptt_ret_t ret = PTT_RET_SUCCESS;
    ptt_cmd_t cmd = ptt_event_get_cmd(m_ota_cmd_evt);

    PTT_TRACE("%s: cmd %d\n", __func__, cmd);

    switch (ptt_event_get_cmd(m_ota_cmd_evt))
    {
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

        case PTT_CMD_GET_ANTENNA:
            ret = cmd_get_antenna();
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

    PTT_TRACE("%s: cmd %d state %d\n",
              __func__,
              ptt_event_get_cmd(m_ota_cmd_evt),
              ptt_event_get_state(m_ota_cmd_evt));

    switch (ptt_event_get_state(m_ota_cmd_evt))
    {
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
            PTT_TRACE("%s: inappropriate state cmd %d state %d\n",
                      __func__,
                      ptt_event_get_cmd(m_ota_cmd_evt),
                      ptt_event_get_state(m_ota_cmd_evt));
            break;
    }

    PTT_TRACE_FUNC_EXIT();
}

void cmd_ota_rf_tx_failed(ptt_evt_id_t evt_id)
{
    PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

    assert(true == cmd_is_ota_cmd_locked_by(evt_id));

    PTT_TRACE("%s: cmd %d state %d\n",
              __func__,
              ptt_event_get_cmd(m_ota_cmd_evt),
              ptt_event_get_state(m_ota_cmd_evt));

    switch (ptt_event_get_state(m_ota_cmd_evt))
    {
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
            PTT_TRACE("%s: inappropriate state cmd %d state %d\n",
                      __func__,
                      ptt_event_get_cmd(m_ota_cmd_evt),
                      ptt_event_get_state(m_ota_cmd_evt));
            break;
    }

    PTT_TRACE_FUNC_EXIT();
}

void cmd_ota_rf_rx_done(ptt_evt_id_t new_rf_pkt_evt)
{
    PTT_TRACE_FUNC_ENTER_WITH_EVT(new_rf_pkt_evt);

    if (false == cmd_is_ota_cmd_locked())
    {
        PTT_TRACE("%s unexpected packet for idle state ignored\n", __func__);
    }
    else
    {
        ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(new_rf_pkt_evt);

        assert(NULL != p_evt_data);

        if (!ptt_proto_check_packet(p_evt_data->arr, p_evt_data->len))
        {
            PTT_TRACE("%s not protocol packet received and ignored\n", __func__);
        }
        else
        {
            PTT_TRACE("%s: protocol packet received. Current cmd %d state %d\n",
                      __func__,
                      ptt_event_get_cmd(m_ota_cmd_evt),
                      ptt_event_get_state(m_ota_cmd_evt));

            ptt_event_set_cmd(new_rf_pkt_evt, p_evt_data->arr[PTT_CMD_CODE_START]);

            switch (ptt_event_get_state(m_ota_cmd_evt))
            {
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
                    PTT_TRACE("%s: inappropriate state cmd %d state %d\n",
                              __func__,
                              ptt_event_get_cmd(m_ota_cmd_evt),
                              ptt_event_get_state(m_ota_cmd_evt));
                    break;
            }
        }
    }

    PTT_TRACE_FUNC_EXIT();
}

static ptt_ret_t cmd_ping(void)
{
    PTT_TRACE_FUNC_ENTER();

    cmd_change_ota_cmd_state(CMD_OTA_STATE_PING_SENDING);
    ptt_ret_t ret = cmd_make_and_send_rf_packet(PTT_CMD_PING);

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

    ptt_ret_t ret = ptt_timer_add(ptt_ctrl_get_rsp_timeout(), cmd_ping_timeout, m_ota_cmd_evt);

    if (PTT_RET_SUCCESS == ret)
    {
        cmd_change_ota_cmd_state(CMD_OTA_STATE_PING_WAITING_FOR_ACK);
    }
    else
    {
        PTT_TRACE("%s: add_timer return error code: %d\n", __func__, ret);
        cmd_send_timeout_and_reset();
    }

    PTT_TRACE_FUNC_EXIT();
}

static void cmd_ping_ack(ptt_evt_id_t new_evt_id)
{
    PTT_TRACE_FUNC_ENTER_WITH_EVT(new_evt_id);

    if (PTT_CMD_ACK == ptt_event_get_cmd(new_evt_id))
    {
        ptt_timer_remove(m_ota_cmd_evt);

        uint8_t result = 1;

        cmd_fill_ctx_end_and_reset(&result, sizeof(result));
    }
    else
    {
        PTT_TRACE("%s: command is not ACK\n", __func__);
    }

    PTT_TRACE_FUNC_EXIT();
}

static ptt_ret_t cmd_set_channel(void)
{
    PTT_TRACE_FUNC_ENTER();

    cmd_change_ota_cmd_state(CMD_OTA_STATE_SET_CHANNEL_SENDING);
    ptt_ret_t ret = cmd_make_and_send_rf_packet(PTT_CMD_SET_CHANNEL);

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
    return ret;
}

static ptt_ret_t cmd_set_power(void)
{
    PTT_TRACE_FUNC_ENTER();

    cmd_change_ota_cmd_state(CMD_OTA_STATE_SET_POWER_SENDING);
    ptt_ret_t ret = cmd_make_and_send_rf_packet(PTT_CMD_SET_POWER);

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
    return ret;
}

static ptt_ret_t cmd_get_power(void)
{
    PTT_TRACE_FUNC_ENTER();

    cmd_change_ota_cmd_state(CMD_OTA_STATE_GET_POWER_SENDING);
    ptt_ret_t ret = cmd_make_and_send_rf_packet(PTT_CMD_GET_POWER);

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

    ptt_ret_t ret = ptt_timer_add(ptt_ctrl_get_rsp_timeout(), cmd_get_power_timeout, m_ota_cmd_evt);

    if (PTT_RET_SUCCESS == ret)
    {
        cmd_change_ota_cmd_state(CMD_OTA_STATE_GET_POWER_WAITING_FOR_RSP);
    }
    else
    {
        PTT_TRACE("%s: add_timer return error code: %d\n", __func__, ret);
        cmd_send_timeout_and_reset();
    }

    PTT_TRACE_FUNC_EXIT();
}

static void cmd_get_power_response(ptt_evt_id_t new_rf_pkt_evt)
{
    PTT_TRACE_FUNC_ENTER_WITH_EVT(new_rf_pkt_evt);

    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(new_rf_pkt_evt);

    assert(NULL != p_evt_data);

    if ((PTT_CMD_GET_POWER_RESPONSE == ptt_event_get_cmd(new_rf_pkt_evt)) &&
        ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_GET_POWER) == p_evt_data->len))
    {
        ptt_timer_remove(m_ota_cmd_evt);

        cmd_fill_ctx_end_and_reset(&p_evt_data->arr[PTT_PAYLOAD_START], PTT_PAYLOAD_LEN_GET_POWER);
    }
    else
    {
        PTT_TRACE("%s: invalid length %d\n", __func__, p_evt_data->len);
    }

    PTT_TRACE_FUNC_EXIT();
}

static ptt_ret_t cmd_set_antenna(void)
{
    PTT_TRACE_FUNC_ENTER();

    cmd_change_ota_cmd_state(CMD_OTA_STATE_SET_ANTENNA_SENDING);
    ptt_ret_t ret = cmd_make_and_send_rf_packet(PTT_CMD_SET_ANTENNA);

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
    return ret;
}

static ptt_ret_t cmd_get_antenna(void)
{
    PTT_TRACE_FUNC_ENTER();

    cmd_change_ota_cmd_state(CMD_OTA_STATE_GET_ANTENNA_SENDING);
    ptt_ret_t ret = cmd_make_and_send_rf_packet(PTT_CMD_GET_ANTENNA);

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

    ptt_ret_t ret =
        ptt_timer_add(ptt_ctrl_get_rsp_timeout(), cmd_get_antenna_timeout, m_ota_cmd_evt);

    if (PTT_RET_SUCCESS == ret)
    {
        cmd_change_ota_cmd_state(CMD_OTA_STATE_GET_ANTENNA_WAITING_FOR_RSP);
    }
    else
    {
        PTT_TRACE("%s: add_timer return error code: %d\n", __func__, ret);
        cmd_send_timeout_and_reset();
    }

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
}

static void cmd_get_antenna_response(ptt_evt_id_t new_rf_pkt_evt)
{
    PTT_TRACE_FUNC_ENTER_WITH_EVT(new_rf_pkt_evt);

    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(new_rf_pkt_evt);

    assert(NULL != p_evt_data);

    if ((PTT_CMD_GET_ANTENNA_RESPONSE == ptt_event_get_cmd(new_rf_pkt_evt)) &&
        ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_GET_ANTENNA) == p_evt_data->len))
    {
        ptt_timer_remove(m_ota_cmd_evt);

        cmd_fill_ctx_end_and_reset(&p_evt_data->arr[PTT_PAYLOAD_START],
                                   PTT_PAYLOAD_LEN_GET_ANTENNA);
    }
    else
    {
        PTT_TRACE("%s: invalid length %d\n", __func__, p_evt_data->len);
    }

    PTT_TRACE_FUNC_EXIT();
}

static ptt_ret_t cmd_end_rx_test(void)
{
    PTT_TRACE_FUNC_ENTER();

    cmd_change_ota_cmd_state(CMD_OTA_STATE_END_RX_TEST_SENDING);
    ptt_ret_t ret = cmd_make_and_send_rf_packet(PTT_CMD_END_RX_TEST);

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

    ptt_ret_t ret = ptt_timer_add(ptt_ctrl_get_rsp_timeout(),
                                  cmd_end_rx_test_timeout,
                                  m_ota_cmd_evt);

    if (PTT_RET_SUCCESS == ret)
    {
        cmd_change_ota_cmd_state(CMD_OTA_STATE_END_RX_TEST_WAITING_FOR_REPORT);
    }
    else
    {
        PTT_TRACE("%s: add_timer return error code: %d\n", __func__, ret);
        cmd_send_timeout_and_reset();
    }

    PTT_TRACE_FUNC_EXIT();
}

static void cmd_end_rx_test_report(ptt_evt_id_t new_rf_pkt_evt)
{
    PTT_TRACE_FUNC_EXIT_WITH_VALUE(new_rf_pkt_evt);

    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(new_rf_pkt_evt);

    assert(NULL != p_evt_data);

    if ((PTT_CMD_REPORT == ptt_event_get_cmd(new_rf_pkt_evt)) &&
        ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_REPORT) == p_evt_data->len))
    {
        ptt_timer_remove(m_ota_cmd_evt);

        cmd_fill_ctx_end_and_reset(&p_evt_data->arr[PTT_PAYLOAD_START], PTT_PAYLOAD_LEN_REPORT);
    }
    else
    {
        PTT_TRACE("%s: invalid length %d\n", __func__, p_evt_data->len);
    }

    PTT_TRACE_FUNC_EXIT();
}

static ptt_ret_t cmd_get_hardware_version(void)
{
    PTT_TRACE_FUNC_ENTER();

    cmd_change_ota_cmd_state(CMD_OTA_STATE_GET_HARDWARE_SENDING);
    ptt_ret_t ret = cmd_make_and_send_rf_packet(PTT_CMD_GET_HARDWARE_VERSION);

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

    ptt_ret_t ret = ptt_timer_add(ptt_ctrl_get_rsp_timeout(),
                                  cmd_get_hardware_timeout,
                                  m_ota_cmd_evt);

    if (PTT_RET_SUCCESS == ret)
    {
        cmd_change_ota_cmd_state(CMD_OTA_STATE_GET_HARDWARE_WAITING_FOR_RSP);
    }
    else
    {
        PTT_TRACE("%s: add_timer return error code: %d\n", __func__, ret);
        cmd_send_timeout_and_reset();
    }

    PTT_TRACE_FUNC_EXIT();
}

static void cmd_get_hardware_response(ptt_evt_id_t new_rf_pkt_evt)
{
    PTT_TRACE_FUNC_ENTER_WITH_EVT(new_rf_pkt_evt);

    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(new_rf_pkt_evt);

    assert(NULL != p_evt_data);

    if ((PTT_CMD_GET_HARDWARE_VERSION_RESPONSE == ptt_event_get_cmd(new_rf_pkt_evt)) &&
        ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_GET_HARDWARE) ==
         p_evt_data->len))
    {
        ptt_timer_remove(m_ota_cmd_evt);

        cmd_fill_ctx_end_and_reset(&p_evt_data->arr[PTT_PAYLOAD_START],
                                   PTT_PAYLOAD_LEN_GET_HARDWARE);
    }
    else
    {
        PTT_TRACE("%s: invalid length %d\n", __func__, p_evt_data->len);
    }

    PTT_TRACE_FUNC_EXIT();
}

static ptt_ret_t cmd_get_software_version(void)
{
    PTT_TRACE_FUNC_ENTER();

    cmd_change_ota_cmd_state(CMD_OTA_STATE_GET_SOFTWARE_SENDING);
    ptt_ret_t ret = cmd_make_and_send_rf_packet(PTT_CMD_GET_SOFTWARE_VERSION);

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

    ptt_ret_t ret = ptt_timer_add(ptt_ctrl_get_rsp_timeout(),
                                  cmd_get_software_timeout,
                                  m_ota_cmd_evt);

    if (PTT_RET_SUCCESS == ret)
    {
        cmd_change_ota_cmd_state(CMD_OTA_STATE_GET_SOFTWARE_WAITING_FOR_RSP);
    }
    else
    {
        PTT_TRACE("%s: add_timer return error code: %d\n", __func__, ret);
        cmd_send_timeout_and_reset();
    }

    PTT_TRACE_FUNC_EXIT();
}

static void cmd_get_software_response(ptt_evt_id_t new_rf_pkt_evt)
{
    PTT_TRACE_FUNC_ENTER_WITH_EVT(new_rf_pkt_evt);

    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(new_rf_pkt_evt);

    assert(NULL != p_evt_data);

    if ((PTT_CMD_GET_SOFTWARE_VERSION_RESPONSE == ptt_event_get_cmd(new_rf_pkt_evt)) &&
        ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_GET_SOFTWARE) ==
         p_evt_data->len))
    {
        ptt_timer_remove(m_ota_cmd_evt);

        cmd_fill_ctx_end_and_reset(&p_evt_data->arr[PTT_PAYLOAD_START],
                                   PTT_PAYLOAD_LEN_GET_SOFTWARE);
    }
    else
    {
        PTT_TRACE("%s: invalid length %d\n", __func__, p_evt_data->len);
    }

    PTT_TRACE_FUNC_EXIT();
}

static ptt_ret_t cmd_stream(void)
{
    PTT_TRACE_FUNC_ENTER();

    cmd_change_ota_cmd_state(CMD_OTA_STATE_STREAM_SENDING);
    ptt_ret_t ret = cmd_make_and_send_rf_packet(PTT_CMD_STREAM);

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
    return ret;
}

static ptt_ret_t cmd_start_rx_test(void)
{
    PTT_TRACE_FUNC_ENTER();

    cmd_change_ota_cmd_state(CMD_OTA_STATE_START_RX_TEST_SENDING);
    ptt_ret_t ret = cmd_make_and_send_rf_packet(PTT_CMD_START_RX_TEST);

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
    return ret;
}

static inline void cmd_ota_cmd_lock(ptt_evt_id_t new_ota_cmd)
{
    assert(PTT_EVENT_UNASSIGNED == m_ota_cmd_evt);

    m_ota_cmd_evt = new_ota_cmd;
}

static inline void cmd_change_ota_cmd_state(ptt_evt_state_t state)
{
    PTT_TRACE("%s: %d", __func__, state);

    ptt_event_set_state(m_ota_cmd_evt, state);
}

static inline ptt_evt_id_t cmd_get_ota_cmd_and_reset(void)
{
    ptt_evt_id_t evt_id = m_ota_cmd_evt;

    cmd_ota_cmd_unlock();
    return evt_id;
}

static void cmd_ota_cmd_unlock(void)
{
    m_ota_cmd_evt = PTT_EVENT_UNASSIGNED;
}

static inline ptt_bool_t cmd_is_ota_cmd_locked(void)
{
    return (PTT_EVENT_UNASSIGNED == m_ota_cmd_evt) ? false : true;
}

static inline ptt_bool_t cmd_is_ota_cmd_locked_by(ptt_evt_id_t evt_id)
{
    return (evt_id == m_ota_cmd_evt) ? true : false;
}

static void cmd_fill_ctx_end_and_reset(const uint8_t * p_data, ptt_pkt_len_t len)
{
    ptt_event_set_ctx_data(m_ota_cmd_evt, p_data, len);
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

static ptt_ret_t cmd_make_and_send_rf_packet(ptt_cmd_t cmd)
{
    ptt_ret_t        ret        = PTT_RET_SUCCESS;
    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(m_ota_cmd_evt);

    assert(NULL != p_evt_data);

    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(m_ota_cmd_evt);

    assert(NULL != p_ctx_data);

    p_evt_data->len = ptt_proto_construct_header(p_evt_data->arr, cmd, PTT_EVENT_DATA_SIZE);

    if (p_evt_data->len != (PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN))
    {
        ret = PTT_RET_INVALID_VALUE;
    }
    else
    {
        memcpy(&p_evt_data->arr[p_evt_data->len], p_ctx_data->arr, p_ctx_data->len);
        p_evt_data->len += p_ctx_data->len;

        ret = ptt_rf_send_packet(m_ota_cmd_evt, p_evt_data->arr, p_evt_data->len);
    }

    return ret;
}