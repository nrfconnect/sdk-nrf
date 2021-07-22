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
static ptt_evt_id_t m_cur_evt;

/* current event handlers */
static void dut_rf_tx_finished(ptt_evt_id_t evt_id);
static void dut_rf_tx_failed(ptt_evt_id_t evt_id);
static void dut_idle_packet_proc(void);
static ptt_ret_t dut_ping(void);
static ptt_ret_t dut_set_channel(void);
static ptt_ret_t dut_set_power(void);
static ptt_ret_t dut_get_power(void);
static ptt_ret_t dut_stream(void);
static ptt_ret_t dut_start_rx_test(void);
static ptt_ret_t dut_hw_version(void);
static ptt_ret_t dut_sw_version(void);
static ptt_ret_t dut_set_antenna(void);
static ptt_ret_t dut_get_antenna(void);

/* new event handlers */
static void dut_rf_rx_done(ptt_evt_id_t new_evt_id);
static void dut_end_rx_test(ptt_evt_id_t new_evt_id);

static void dut_stream_stop(ptt_evt_id_t timer_evt_id);

static ptt_pkt_len_t dut_form_stat_report(uint8_t * p_pkt, ptt_pkt_len_t pkt_max_size);

static inline void dut_change_cur_state(ptt_evt_state_t state);
static inline void dut_store_cur_evt(ptt_evt_id_t evt_id);
static inline void dut_reset_cur_evt(void);

#ifdef TESTS
#include "test_dut_wrappers.c"
#endif

ptt_ret_t ptt_zb_perf_dut_mode_init(void)
{
    PTT_TRACE_FUNC_ENTER();

    m_cur_evt = PTT_EVENT_UNASSIGNED;
    ptt_ext_evts_handlers_t * p_handlers = ptt_ctrl_get_p_handlers();

    p_handlers->rf_tx_finished = dut_rf_tx_finished;
    p_handlers->rf_tx_failed   = dut_rf_tx_failed;
    p_handlers->rf_rx_done     = dut_rf_rx_done;

    PTT_TRACE_FUNC_EXIT();
    return PTT_RET_SUCCESS;
}

ptt_ret_t ptt_zb_perf_dut_mode_uninit(void)
{
    PTT_TRACE_FUNC_ENTER();

    if (m_cur_evt != PTT_EVENT_UNASSIGNED)
    {
        dut_reset_cur_evt();
    }

    PTT_TRACE_FUNC_EXIT();
    return PTT_RET_SUCCESS;
}

static void dut_rf_rx_done(ptt_evt_id_t new_evt_id)
{
    PTT_TRACE_FUNC_ENTER_WITH_EVT(new_evt_id);

    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(new_evt_id);

    assert(NULL != p_evt_data);

    if (!ptt_proto_check_packet(p_evt_data->arr, p_evt_data->len))
    {
        PTT_TRACE("%s not protocol packet received and ignored\n", __func__);
        ptt_event_free(new_evt_id);
    }
    else
    {
        ptt_event_set_cmd(new_evt_id, p_evt_data->arr[PTT_CMD_CODE_START]);

        if (PTT_EVENT_UNASSIGNED == m_cur_evt)
        {
            /* store the event as currently processed */
            dut_store_cur_evt(new_evt_id);
            dut_idle_packet_proc();
        }
        else
        {
            switch (ptt_event_get_state(m_cur_evt))
            {
                case DUT_STATE_RX_TEST_WAIT_FOR_END:
                    dut_end_rx_test(new_evt_id);
                    break;

                default:
                    PTT_TRACE("%s: state isn't idle cmd %d state %d\n",
                              __func__,
                              ptt_event_get_cmd(m_cur_evt),
                              ptt_event_get_state(m_cur_evt));
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

    ptt_ret_t ret = PTT_RET_SUCCESS;

    PTT_TRACE("%s: cmd %d state %d\n",
              __func__,
              ptt_event_get_cmd(m_cur_evt),
              ptt_event_get_state(m_cur_evt));

    switch (ptt_event_get_cmd(m_cur_evt))
    {
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

        case PTT_CMD_GET_ANTENNA:
            ret = dut_get_antenna();
            break;

        default:
            PTT_TRACE("%s: unknown command cmd %d state %d\n",
                      __func__,
                      ptt_event_get_cmd(m_cur_evt),
                      ptt_event_get_state(m_cur_evt));
            dut_reset_cur_evt();
            break;
    }

    if (ret != PTT_RET_SUCCESS)
    {
        dut_reset_cur_evt();
    }

    PTT_TRACE_FUNC_EXIT();
}

static void dut_rf_tx_finished(ptt_evt_id_t evt_id)
{
    PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

    assert(evt_id == m_cur_evt);

    PTT_TRACE("%s: cmd %d state %d\n",
              __func__,
              ptt_event_get_cmd(m_cur_evt),
              ptt_event_get_state(m_cur_evt));

    switch (ptt_event_get_state(m_cur_evt))
    {
        case DUT_STATE_ACK_SENDING:
        case DUT_STATE_POWER_SENDING:
        case DUT_STATE_RX_TEST_REPORT_SENDING:
        case DUT_STATE_HW_VERSION_SENDING:
        case DUT_STATE_SW_VERSION_SENDING:
        case DUT_STATE_ANTENNA_SENDING:
            dut_reset_cur_evt();
            break;

        default:
            PTT_TRACE("%s: inappropriate state cmd %d state %d\n",
                      __func__,
                      ptt_event_get_cmd(m_cur_evt),
                      ptt_event_get_state(m_cur_evt));
            break;
    }

    PTT_TRACE_FUNC_EXIT();
}

static void dut_rf_tx_failed(ptt_evt_id_t evt_id)
{
    PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

    assert(evt_id == m_cur_evt);

    PTT_TRACE("%s: cmd %d state %d\n",
              __func__,
              ptt_event_get_cmd(m_cur_evt),
              ptt_event_get_state(m_cur_evt));

    switch (ptt_event_get_state(m_cur_evt))
    {
        case DUT_STATE_ACK_SENDING:
        case DUT_STATE_POWER_SENDING:
        case DUT_STATE_RX_TEST_REPORT_SENDING:
        case DUT_STATE_HW_VERSION_SENDING:
        case DUT_STATE_SW_VERSION_SENDING:
        case DUT_STATE_ANTENNA_SENDING:
            dut_reset_cur_evt();
            break;

        default:
            PTT_TRACE("%s: inappropriate state cmd %d state %d\n",
                      __func__,
                      ptt_event_get_cmd(m_cur_evt),
                      ptt_event_get_state(m_cur_evt));
            break;
    }

    PTT_TRACE_FUNC_EXIT();
}

static ptt_ret_t dut_ping(void)
{
    PTT_TRACE_FUNC_ENTER();

    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(m_cur_evt);

    assert(NULL != p_evt_data);

    uint8_t * p = p_evt_data->arr;

    dut_change_cur_state(DUT_STATE_ACK_SENDING);

    p_evt_data->len = ptt_proto_construct_header(p, PTT_CMD_ACK, PTT_EVENT_DATA_SIZE);

    ptt_ret_t ret = ptt_rf_send_packet(m_cur_evt, p_evt_data->arr, p_evt_data->len);

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

    return ret;
}

static ptt_ret_t dut_set_channel(void)
{
    PTT_TRACE_FUNC_ENTER();

    ptt_ret_t ret = PTT_RET_SUCCESS;

    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(m_cur_evt);

    assert(NULL != p_evt_data);

    if ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_SET_CHANNEL) != p_evt_data->len)
    {
        PTT_TRACE("%s: invalid length %d\n", __func__, p_evt_data->len);
        ret = PTT_RET_INVALID_COMMAND;
    }
    else
    {
        uint32_t mask = ptt_betoh32_val(&p_evt_data->arr[PTT_PAYLOAD_START]);

        ret = ptt_rf_set_channel_mask(m_cur_evt, mask);
    }

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

    return ret;
}

static ptt_ret_t dut_set_power(void)
{
    PTT_TRACE_FUNC_ENTER();

    ptt_ret_t ret = PTT_RET_SUCCESS;

    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(m_cur_evt);

    assert(NULL != p_evt_data);

    if ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_SET_POWER) != p_evt_data->len)
    {
        PTT_TRACE("%s: invalid length %d\n", __func__, p_evt_data->len);
        ret = PTT_RET_INVALID_COMMAND;
    }
    else
    {
        int8_t power = (int8_t)(p_evt_data->arr[PTT_PAYLOAD_START]);

        ret = ptt_rf_set_power(m_cur_evt, power);
    }

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

    return ret;
}

static ptt_ret_t dut_get_power(void)
{
    PTT_TRACE_FUNC_ENTER();

    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(m_cur_evt);

    assert(NULL != p_evt_data);

    uint8_t * p = p_evt_data->arr;

    dut_change_cur_state(DUT_STATE_POWER_SENDING);

    p_evt_data->len = ptt_proto_construct_header(p,
                                                 PTT_CMD_GET_POWER_RESPONSE,
                                                 PTT_EVENT_DATA_SIZE);
    p[p_evt_data->len] = ptt_rf_get_power();
    p_evt_data->len++;

    ptt_ret_t ret = ptt_rf_send_packet(m_cur_evt, p_evt_data->arr, p_evt_data->len);

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

    return ret;
}

static ptt_ret_t dut_set_antenna(void)
{
    PTT_TRACE_FUNC_ENTER();

    ptt_ret_t ret = PTT_RET_SUCCESS;

    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(m_cur_evt);

    assert(NULL != p_evt_data);

    if ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_SET_ANTENNA) != p_evt_data->len)
    {
        PTT_TRACE("%s: invalid length %d\n", __func__, p_evt_data->len);
        ret = PTT_RET_INVALID_COMMAND;
    }
    else
    {
        uint8_t antenna = p_evt_data->arr[PTT_PAYLOAD_START];

        ret = ptt_rf_set_antenna(m_cur_evt, antenna);
    }

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
    return ret;
}

static ptt_ret_t dut_get_antenna(void)
{
    PTT_TRACE_FUNC_ENTER();

    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(m_cur_evt);

    assert(NULL != p_evt_data);

    uint8_t * p = p_evt_data->arr;

    dut_change_cur_state(DUT_STATE_ANTENNA_SENDING);

    p_evt_data->len = ptt_proto_construct_header(p,
                                                 PTT_CMD_GET_ANTENNA_RESPONSE,
                                                 PTT_EVENT_DATA_SIZE);
    p[p_evt_data->len] = ptt_rf_get_antenna();
    p_evt_data->len++;

    ptt_ret_t ret = ptt_rf_send_packet(m_cur_evt, p_evt_data->arr, p_evt_data->len);

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
    return ret;
}

static ptt_ret_t dut_stream(void)
{
    PTT_TRACE_FUNC_ENTER();

    ptt_ret_t ret = PTT_RET_SUCCESS;

    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(m_cur_evt);

    assert(NULL != p_evt_data);

    if ((PTT_PREAMBLE_LEN + PTT_CMD_CODE_LEN + PTT_PAYLOAD_LEN_STREAM) != p_evt_data->len)
    {
        PTT_TRACE("%s: invalid length %d\n", __func__, p_evt_data->len);
        ret = PTT_RET_INVALID_COMMAND;
    }
    else
    {
        uint16_t duration = ptt_betoh16_val(&p_evt_data->arr[PTT_PAYLOAD_START]);

        if (duration != 0)
        {
            ret = ptt_timer_add(duration, dut_stream_stop, m_cur_evt);

            if (PTT_RET_SUCCESS == ret)
            {
                ptt_ltx_payload_t * p_ltx_payload = ptt_rf_get_custom_ltx_payload();

                p_ltx_payload->len = PTT_CUSTOM_LTX_PAYLOAD_MAX_SIZE;

                ptt_random_vector_generate(p_ltx_payload->arr, p_ltx_payload->len);

                ret = ptt_rf_start_modulated_stream(m_cur_evt,
                                                    p_ltx_payload->arr,
                                                    p_ltx_payload->len);

                if (PTT_RET_SUCCESS == ret)
                {
                    dut_change_cur_state(DUT_STATE_STREAM_EMITTING);
                }
                else
                {
                    ptt_timer_remove(m_cur_evt);
                    PTT_TRACE(
                        "%s: ptt_rf_start_modulated_stream returns %d. Aborting command handling",
                        __func__,
                        ret);
                }
            }
        }
        else
        {
            dut_reset_cur_evt();
        }
    }

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

    return ret;
}

static void dut_stream_stop(ptt_evt_id_t timer_evt_id)
{
    PTT_TRACE_FUNC_ENTER();

    assert(timer_evt_id == m_cur_evt);

    ptt_ret_t ret = ptt_rf_stop_modulated_stream(m_cur_evt);

    dut_reset_cur_evt();

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
}

static ptt_ret_t dut_start_rx_test(void)
{
    PTT_TRACE_FUNC_ENTER();

    /* we will store protocol packets counter in context of m_cur_evt, lets erase it just in case */
    ptt_evt_ctx_data_t * p_cur_ctx_data = ptt_event_get_p_ctx_data(m_cur_evt);

    assert(NULL != p_cur_ctx_data);

    uint32_t * p_proto_pkts = (uint32_t *)p_cur_ctx_data;

    *p_proto_pkts = 0;

    ptt_ret_t ret = ptt_rf_start_statistic(m_cur_evt);

    if (ret != PTT_RET_SUCCESS)
    {
        PTT_TRACE("%s: ret %d\n", __func__, ret);
        ret = PTT_RET_INVALID_COMMAND;
    }
    else
    {
        dut_change_cur_state(DUT_STATE_RX_TEST_WAIT_FOR_END);
    }

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

    return ret;
}

static void dut_end_rx_test(ptt_evt_id_t new_evt_id)
{
    PTT_TRACE_FUNC_ENTER_WITH_EVT(new_evt_id);

    ptt_ret_t ret = PTT_RET_SUCCESS;

    ptt_evt_ctx_data_t * p_cur_ctx_data = ptt_event_get_p_ctx_data(m_cur_evt);

    assert(NULL != p_cur_ctx_data);

    uint32_t * p_proto_pkts = (uint32_t *)p_cur_ctx_data;

    ++(*p_proto_pkts);

    if (PTT_CMD_END_RX_TEST == ptt_event_get_cmd(new_evt_id))
    {
        ret = ptt_rf_end_statistic(m_cur_evt);

        if (ret != PTT_RET_SUCCESS)
        {
            PTT_TRACE("%s: ret %d\n", __func__, ret);
        }
        else
        {
            ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(m_cur_evt);

            assert(NULL != p_evt_data);

            uint8_t * p = p_evt_data->arr;

            dut_change_cur_state(DUT_STATE_RX_TEST_REPORT_SENDING);

            p_evt_data->len  = ptt_proto_construct_header(p, PTT_CMD_REPORT, PTT_EVENT_DATA_SIZE);
            p_evt_data->len += dut_form_stat_report(&p[p_evt_data->len],
                                                    PTT_EVENT_DATA_SIZE - p_evt_data->len);

            ret = ptt_rf_send_packet(m_cur_evt, p_evt_data->arr, p_evt_data->len);
        }
    }

    ptt_event_free(new_evt_id);

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);
}

static ptt_ret_t dut_hw_version(void)
{
    PTT_TRACE_FUNC_ENTER();

    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(m_cur_evt);

    assert(NULL != p_evt_data);

    uint8_t * p = p_evt_data->arr;

    dut_change_cur_state(DUT_STATE_HW_VERSION_SENDING);

    p_evt_data->len = ptt_proto_construct_header(p,
                                                 PTT_CMD_GET_HARDWARE_VERSION_RESPONSE,
                                                 PTT_EVENT_DATA_SIZE);
    p[p_evt_data->len] = ptt_ctrl_get_hw_version();
    p_evt_data->len++;

    ptt_ret_t ret = ptt_rf_send_packet(m_cur_evt, p_evt_data->arr, p_evt_data->len);

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

    return ret;
}

static ptt_ret_t dut_sw_version(void)
{
    PTT_TRACE_FUNC_ENTER();

    ptt_evt_data_t * p_evt_data = ptt_event_get_p_data(m_cur_evt);

    assert(NULL != p_evt_data);

    uint8_t * p = p_evt_data->arr;

    dut_change_cur_state(DUT_STATE_SW_VERSION_SENDING);

    p_evt_data->len = ptt_proto_construct_header(p,
                                                 PTT_CMD_GET_SOFTWARE_VERSION_RESPONSE,
                                                 PTT_EVENT_DATA_SIZE);
    p[p_evt_data->len] = ptt_ctrl_get_sw_version();
    p_evt_data->len++;

    ptt_ret_t ret = ptt_rf_send_packet(m_cur_evt, p_evt_data->arr, p_evt_data->len);

    PTT_TRACE_FUNC_EXIT_WITH_VALUE(ret);

    return ret;
}

static ptt_pkt_len_t dut_form_stat_report(uint8_t * p_pkt, ptt_pkt_len_t pkt_max_size)
{
    assert(p_pkt != NULL);
    assert(pkt_max_size >= PTT_PAYLOAD_LEN_REPORT);

    ptt_pkt_len_t len  = 0;
    uint8_t     * p    = p_pkt;
    ptt_rf_stat_t stat = ptt_rf_get_stat_report();

    ptt_evt_ctx_data_t * p_cur_ctx_data = ptt_event_get_p_ctx_data(m_cur_evt);

    assert(NULL != p_cur_ctx_data);

    uint32_t * p_proto_pkts = (uint32_t *)p_cur_ctx_data;

    ptt_htobe32((uint8_t *)&stat.total_pkts, &p[len]);
    len += sizeof(stat.total_pkts);

    ptt_htobe32((uint8_t *)p_proto_pkts, &p[len]);
    len += sizeof(*p_proto_pkts);

    ptt_htobe32((uint8_t *)&stat.total_lqi, &p[len]);
    len += sizeof(stat.total_lqi);

    ptt_htobe32((uint8_t *)&stat.total_rssi, &p[len]);
    len += sizeof(stat.total_rssi);

    assert(PTT_PAYLOAD_LEN_REPORT == len);

    return len;
}

static inline void dut_change_cur_state(ptt_evt_state_t state)
{
    PTT_TRACE("%s: state %d", __func__, state);

    ptt_event_set_state(m_cur_evt, state);
}

static inline void dut_store_cur_evt(ptt_evt_id_t evt_id)
{
    assert(PTT_EVENT_UNASSIGNED == m_cur_evt);

    m_cur_evt = evt_id;
}

static inline void dut_reset_cur_evt(void)
{
    ptt_event_free(m_cur_evt);
    m_cur_evt = PTT_EVENT_UNASSIGNED;
}