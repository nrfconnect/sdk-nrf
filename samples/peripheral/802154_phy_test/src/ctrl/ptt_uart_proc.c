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
/* Purpose: processing external UART messages */

/* We expects a command string looks like "custom <command_name> <whitespace_separated_bytes>".
   Each whitespace-separated value fits into one byte and payload has big-endian byte order.
   For example: "custom setchannel 0x00 0x00 0x08 0x00" to set 11 channel.*/

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
static uart_text_cmd_t m_text_cmds[PTT_UART_CMD_N] = {
    {UART_CMD_R_PING_TEXT, PTT_UART_CMD_R_PING, UART_CMD_R_PING_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_PING_TIMEOUT_TEXT, PTT_UART_CMD_L_PING_TIMEOUT, UART_CMD_L_PING_TIMEOUT_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_SETCHANNEL_TEXT, PTT_UART_CMD_SETCHANNEL, UART_CMD_SETCHANNEL_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_SETCHANNEL_TEXT, PTT_UART_CMD_L_SETCHANNEL, UART_CMD_L_SETCHANNEL_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_R_SETCHANNEL_TEXT, PTT_UART_CMD_R_SETCHANNEL, UART_CMD_R_SETCHANNEL_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_GET_CHANNEL_TEXT, PTT_UART_CMD_L_GET_CHANNEL, UART_CMD_L_GET_CHANNEL_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_SET_POWER_TEXT, PTT_UART_CMD_L_SET_POWER, UART_CMD_L_SET_POWER_PAYLOAD_L,
     PTT_UART_PAYLOAD_RAW},
    {UART_CMD_R_SET_POWER_TEXT, PTT_UART_CMD_R_SET_POWER, UART_CMD_R_SET_POWER_PAYLOAD_L,
     PTT_UART_PAYLOAD_RAW},
    {UART_CMD_L_GET_POWER_TEXT, PTT_UART_CMD_L_GET_POWER, UART_CMD_L_GET_POWER_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_R_GET_POWER_TEXT, PTT_UART_CMD_R_GET_POWER, UART_CMD_R_GET_POWER_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_R_STREAM_TEXT, PTT_UART_CMD_R_STREAM, UART_CMD_R_STREAM_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_R_START_TEXT, PTT_UART_CMD_R_START, UART_CMD_R_START_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_R_END_TEXT, PTT_UART_CMD_R_END, UART_CMD_R_END_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_REBOOT_TEXT, PTT_UART_CMD_L_REBOOT, UART_CMD_L_REBOOT_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_FIND_TEXT, PTT_UART_CMD_FIND, UART_CMD_FIND_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_R_HW_VERSION_TEXT, PTT_UART_CMD_R_HW_VERSION, UART_CMD_R_HW_VERSION_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_R_SW_VERSION_TEXT, PTT_UART_CMD_R_SW_VERSION, UART_CMD_R_SW_VERSION_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_CHANGE_MODE_TEXT, PTT_UART_CMD_CHANGE_MODE, UART_CMD_CHANGE_MODE_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_GET_CCA_TEXT, PTT_UART_CMD_L_GET_CCA, UART_CMD_L_GET_CCA_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_GET_ED_TEXT, PTT_UART_CMD_L_GET_ED, UART_CMD_L_GET_ED_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_GET_LQI_TEXT, PTT_UART_CMD_L_GET_LQI, UART_CMD_L_GET_LQI_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_GET_RSSI_TEXT, PTT_UART_CMD_L_GET_RSSI, UART_CMD_L_GET_RSSI_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_SET_SHORT_TEXT, PTT_UART_CMD_L_SET_SHORT, UART_CMD_L_SET_SHORT_PAYLOAD_L,
     PTT_UART_PAYLOAD_RAW},
    {UART_CMD_L_SET_EXTENDED_TEXT, PTT_UART_CMD_L_SET_EXTENDED, UART_CMD_L_SET_EXTENDED_PAYLOAD_L,
     PTT_UART_PAYLOAD_RAW},
    {UART_CMD_L_SET_PAN_ID_TEXT, PTT_UART_CMD_L_SET_PAN_ID, UART_CMD_L_SET_PAN_ID_PAYLOAD_L,
     PTT_UART_PAYLOAD_RAW},
    {UART_CMD_L_SET_PAYLOAD_TEXT, PTT_UART_CMD_L_SET_PAYLOAD, UART_CMD_L_SET_PAYLOAD_PAYLOAD_L,
     PTT_UART_PAYLOAD_RAW},
    {UART_CMD_L_START_TEXT, PTT_UART_CMD_L_START, UART_CMD_L_START_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_END_TEXT, PTT_UART_CMD_L_END, UART_CMD_L_END_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_SET_ANTENNA_TEXT, PTT_UART_CMD_L_SET_ANTENNA, UART_CMD_L_SET_ANTENNA_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_GET_ANTENNA_TEXT, PTT_UART_CMD_L_GET_ANTENNA, UART_CMD_L_GET_ANTENNA_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_R_SET_ANTENNA_TEXT, PTT_UART_CMD_R_SET_ANTENNA, UART_CMD_R_SET_ANTENNA_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_R_GET_ANTENNA_TEXT, PTT_UART_CMD_R_GET_ANTENNA, UART_CMD_R_GET_ANTENNA_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_TX_END_TEXT, PTT_UART_CMD_L_TX_END, UART_CMD_L_TX_END_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_TX_TEXT, PTT_UART_CMD_L_TX, UART_CMD_L_TX_PAYLOAD_L,
     PTT_UART_PAYLOAD_RAW},
    {UART_CMD_L_CLK_TEXT, PTT_UART_CMD_L_CLK, UART_CMD_L_CLK_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_SET_GPIO_TEXT, PTT_UART_CMD_L_SET_GPIO, UART_CMD_L_SET_GPIO_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_GET_GPIO_TEXT, PTT_UART_CMD_L_GET_GPIO, UART_CMD_L_GET_GPIO_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_CARRIER_TEXT, PTT_UART_CMD_L_CARRIER, UART_CMD_L_CARRIER_PAYLOAD_L,
     PTT_UART_PAYLOAD_RAW},
    {UART_CMD_L_STREAM_TEXT, PTT_UART_CMD_L_STREAM, UART_CMD_L_STREAM_PAYLOAD_L,
     PTT_UART_PAYLOAD_RAW},
    {UART_CMD_L_SET_DCDC_TEXT, PTT_UART_CMD_L_SET_DCDC, UART_CMD_L_SET_DCDC_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_GET_DCDC_TEXT, PTT_UART_CMD_L_GET_DCDC, UART_CMD_L_GET_DCDC_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_SET_ICACHE_TEXT, PTT_UART_CMD_L_SET_ICACHE, UART_CMD_L_SET_ICACHE_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_GET_ICACHE_TEXT, PTT_UART_CMD_L_GET_ICACHE, UART_CMD_L_GET_ICACHE_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_GET_TEMP_TEXT, PTT_UART_CMD_L_GET_TEMP, UART_CMD_L_GET_TEMP_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_INDICATION_TEXT, PTT_UART_CMD_L_INDICATION, UART_CMD_L_INDICATION_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_SLEEP_TEXT, PTT_UART_CMD_L_SLEEP, UART_CMD_L_SLEEP_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
    {UART_CMD_L_RECEIVE_TEXT, PTT_UART_CMD_L_RECEIVE, UART_CMD_L_RECEIVE_PAYLOAD_L,
     PTT_UART_PAYLOAD_PARSED_AS_BYTES},
};

static ptt_uart_send_cb m_uart_send_cb = NULL;

static bool m_handler_busy = false;

static ptt_ret_t packet_parser(ptt_evt_id_t evt_id);
static ptt_ret_t text_cmd_parser(ptt_evt_id_t evt_id);
static ptt_ret_t payload_fill(ptt_evt_ctx_data_t * p_ctx_data, char * p_payload, uint8_t len);

void ptt_uart_init(ptt_uart_send_cb send_cb)
{
    m_uart_send_cb = send_cb;
    ptt_uart_print_prompt();
}

void ptt_uart_uninit(void)
{
    m_uart_send_cb = NULL;
}

inline void ptt_uart_print_prompt(void)
{
    if (NULL != m_uart_send_cb)
    {
        m_uart_send_cb(PTT_CAST_TO_UINT8_P(PTT_UART_PROMPT_STR), sizeof(PTT_UART_PROMPT_STR) - 1,
                       false);
    }
}

ptt_ret_t ptt_uart_send_packet(const uint8_t * p_pkt, ptt_pkt_len_t len)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if ((NULL == p_pkt) || (0 == len))
    {
        ret = PTT_RET_INVALID_VALUE;
    }

    if (PTT_RET_SUCCESS == ret)
    {
        if (NULL == m_uart_send_cb)
        {
            ret = PTT_RET_NULL_PTR;
        }
        else
        {
            int32_t size = m_uart_send_cb(p_pkt, len, true);

            if (size != len)
            {
                ret = PTT_RET_BUSY;
            }
        }
    }

    PTT_TRACE("ptt_uart_send_packet: ret %d", ret);
    return ret;
}

void ptt_uart_push_packet(const uint8_t * p_pkt, ptt_pkt_len_t len)
{
    ptt_evt_id_t evt_id;
    ptt_ret_t    ret;

    if ((NULL == p_pkt) || (0 == len))
    {
        ret = PTT_RET_INVALID_VALUE;
        PTT_TRACE("%s: invalid input value", __func__);
    }
    else
    {
        ret = ptt_event_alloc_and_fill(&evt_id, p_pkt, len);

        if (PTT_RET_SUCCESS != ret)
        {
            PTT_TRACE("%s: ptt_event_alloc_and_fill return error code: %d", __func__, ret);
        }
        else
        {
            ret = packet_parser(evt_id);
        }
    }

    /* to prevent sending prompt on invalid command when handler is busy processing previous command */
    if ((PTT_RET_SUCCESS != ret) && !m_handler_busy)
    {
        ptt_uart_print_prompt();
    }
}

/* @todo: refactor to let handler return result being busy */
void ptt_handler_busy(void)
{
    m_handler_busy = true;
}

void ptt_handler_free(void)
{
    m_handler_busy = false;
}

static ptt_ret_t packet_parser(ptt_evt_id_t evt_id)
{
    ptt_ret_t           ret     = PTT_RET_SUCCESS;
    ptt_ext_evt_handler handler = ptt_ctrl_get_handler_uart_pkt_received();

    ret = text_cmd_parser(evt_id);

    if (PTT_RET_SUCCESS == ret)
    {
        if (PTT_UART_CMD_CHANGE_MODE == ptt_event_get_cmd(evt_id))
        {
            ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

            assert(p_ctx_data != NULL);

            ret = ptt_mode_switch(p_ctx_data->arr[0]);

            ptt_uart_print_prompt();

            ptt_event_free(evt_id);
        }
        else if (NULL == handler)
        {
            ret = PTT_RET_INVALID_MODE;
            ptt_event_free(evt_id);
        }
        else
        {
            handler(evt_id);
        }
    }
    else
    {
        ptt_event_free(evt_id);
    }

    if (PTT_RET_SUCCESS != ret)
    {
        PTT_TRACE("packet parser ended with error code: %u", ret);
    }

    return ret;
}

/* fills evt_id with command and its parameters or return error */
static ptt_ret_t text_cmd_parser(ptt_evt_id_t evt_id)
{
    uint8_t              p_str;
    uint8_t              i;
    size_t               cmd_name_len;
    ptt_ret_t            ret        = PTT_RET_SUCCESS;
    ptt_evt_data_t     * p_data     = ptt_event_get_p_data(evt_id);
    ptt_evt_ctx_data_t * p_ctx_data = ptt_event_get_p_ctx_data(evt_id);

    assert(NULL != p_data);
    assert(NULL != p_ctx_data);

    for (i = 0; i < PTT_UART_CMD_N; ++i)
    {
        cmd_name_len = strlen(m_text_cmds[i].name);

        /* compare without '\0' symbol */
        p_str = strncmp(PTT_CAST_TO_STR(p_data->arr), m_text_cmds[i].name, cmd_name_len);

        if (0 != p_str)
        {
            ret = PTT_RET_INVALID_VALUE;
        }
        else
        {
            if (PTT_UART_PAYLOAD_RAW == m_text_cmds[i].payload_type)
            {
                /* raw payload already written in data, just pass */
                ret = PTT_RET_SUCCESS;
            }
            else if (PTT_UART_PAYLOAD_PARSED_AS_BYTES == m_text_cmds[i].payload_type)
            {
                /* after a command should be only whitespace in case of payload or \0 */
                if ((p_data->arr[cmd_name_len] == '\0') || (p_data->arr[cmd_name_len] == ' '))
                {
                    char * p_payload = PTT_CAST_TO_STR(p_data->arr + cmd_name_len);

                    ret = payload_fill(p_ctx_data, p_payload, m_text_cmds[i].payload_len);
                }
                else
                {
                    ret = PTT_RET_INVALID_VALUE;
                }
            }
            else
            {
                ret = PTT_RET_INVALID_VALUE;
            }

            if (PTT_RET_SUCCESS == ret)
            {
                ptt_event_set_cmd(evt_id, m_text_cmds[i].code);
            }

            /* we found command name, time to quit */
            break;
        }
    }

    return ret;
}

/* p_payload points to payload of text cmd */
static ptt_ret_t payload_fill(ptt_evt_ctx_data_t * p_ctx_data, char * p_payload, uint8_t len)
{
    if ((NULL == p_payload) || (NULL == p_ctx_data))
    {
        return PTT_RET_INVALID_VALUE;
    }

    ptt_ret_t     ret = PTT_RET_SUCCESS;
    static char * save;
    char        * p_next_word = strtok_r(p_payload, UART_TEXT_PAYLOAD_DELIMETERS, &save);

    for (uint8_t i = 0; i < len; ++i)
    {
        if (NULL == p_next_word)
        {
            /* expected more words in payload */
            ret = PTT_RET_INVALID_VALUE;
            break;
        }

        uint8_t out_num;

        /* signed values are processed in their handlers */
        ret = ptt_parser_string_to_uint8(p_next_word, &out_num, 0);

        if (PTT_RET_SUCCESS != ret)
        {
            break;
        }

        if (p_ctx_data->len >= PTT_EVENT_CTX_SIZE)
        {
            ret = PTT_RET_INVALID_VALUE;
            break;
        }

        p_ctx_data->arr[p_ctx_data->len] = out_num;
        ++(p_ctx_data->len);

        p_next_word = strtok_r(NULL, UART_TEXT_PAYLOAD_DELIMETERS, &save);
    }

    if (NULL != p_next_word)
    {
        ret = PTT_RET_INVALID_VALUE;
    }

    return ret;
}

