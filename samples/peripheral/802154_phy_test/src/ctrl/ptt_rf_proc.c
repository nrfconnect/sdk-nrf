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

/* Purpose: processing external events from RF */

#include <stddef.h>

#include "ptt_ctrl_internal.h"
#include "ptt_events_internal.h"
#include "rf/ptt_rf.h"

#include "ptt_proto.h"

#include "ctrl/ptt_trace.h"

#ifdef TESTS
#include "test_rf_proc_conf.h"
#endif /* TESTS */

void ptt_ctrl_rf_tx_started(ptt_evt_id_t evt_id)
{
    PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

    ptt_ext_evt_handler handler = ptt_ctrl_get_handler_rf_tx_started();

    if (NULL != handler)
    {
        handler(evt_id);
    }
    else
    {
        ptt_event_free(evt_id);
    }

    PTT_TRACE_FUNC_EXIT();
}

void ptt_ctrl_rf_tx_finished(ptt_evt_id_t evt_id)
{
    PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

    ptt_ext_evt_handler handler = ptt_ctrl_get_handler_rf_tx_finished();

    if (NULL != handler)
    {
        handler(evt_id);
    }
    else
    {
        ptt_event_free(evt_id);
    }

    PTT_TRACE_FUNC_EXIT();
}

void ptt_ctrl_rf_tx_failed(ptt_evt_id_t evt_id, ptt_rf_tx_error_t tx_error)
{
    PTT_TRACE("%s -> evt %d res %d\n", __func__, evt_id, tx_error);

    ptt_ext_evt_handler handler = ptt_ctrl_get_handler_rf_tx_failed();

    ptt_event_set_ctx_data(evt_id, PTT_CAST_TO_UINT8_P(&tx_error), sizeof(tx_error));

    if (NULL != handler)
    {
        handler(evt_id);
    }
    else
    {
        ptt_event_free(evt_id);
    }

    PTT_TRACE_FUNC_EXIT();
}

void ptt_ctrl_rf_cca_done(ptt_evt_id_t evt_id, ptt_cca_t result)
{
    PTT_TRACE("%s -> evt %d res %d\n", __func__, evt_id, result);

    ptt_ext_evt_handler handler = ptt_ctrl_get_handler_rf_cca_done();

    ptt_event_set_ctx_data(evt_id, PTT_CAST_TO_UINT8_P(&result), sizeof(result));

    if (NULL != handler)
    {
        handler(evt_id);
    }
    else
    {
        ptt_event_free(evt_id);
    }

    PTT_TRACE_FUNC_EXIT();
}

void ptt_ctrl_rf_cca_failed(ptt_evt_id_t evt_id)
{
    PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

    ptt_ext_evt_handler handler = ptt_ctrl_get_handler_rf_cca_failed();

    if (NULL != handler)
    {
        handler(evt_id);
    }
    else
    {
        ptt_event_free(evt_id);
    }

    PTT_TRACE_FUNC_EXIT();
}

void ptt_ctrl_rf_ed_detected(ptt_evt_id_t evt_id, ptt_ed_t result)
{
    PTT_TRACE("%s -> evt %d res %d\n", __func__, evt_id, result);

    ptt_ext_evt_handler handler = ptt_ctrl_get_handler_rf_ed_detected();

    ptt_event_set_ctx_data(evt_id, PTT_CAST_TO_UINT8_P(&result), sizeof(result));

    if (NULL != handler)
    {
        handler(evt_id);
    }
    else
    {
        ptt_event_free(evt_id);
    }

    PTT_TRACE_FUNC_EXIT();
}

void ptt_ctrl_rf_ed_failed(ptt_evt_id_t evt_id)
{
    PTT_TRACE_FUNC_ENTER_WITH_EVT(evt_id);

    ptt_ext_evt_handler handler = ptt_ctrl_get_handler_rf_ed_failed();

    if (NULL != handler)
    {
        handler(evt_id);
    }
    else
    {
        ptt_event_free(evt_id);
    }

    PTT_TRACE_FUNC_EXIT();
}

void ptt_ctrl_rf_push_packet(const uint8_t * p_pkt,
                             ptt_pkt_len_t   len,
                             int8_t          rssi,
                             uint8_t         lqi)
{
    PTT_TRACE_FUNC_ENTER();

    ptt_ret_t           ret;
    ptt_evt_id_t        evt_id;
    ptt_ext_evt_handler handler = ptt_ctrl_get_handler_rf_rx_done();

    ret = ptt_event_alloc_and_fill(&evt_id, p_pkt, len);

    if (PTT_RET_SUCCESS != ret)
    {
        PTT_TRACE("%s: ptt_event_alloc_and_fill returned error code: %d", __func__, ret);
    }
    else
    {
        ptt_rf_packet_info_t pkt_info = {.rssi = rssi, .lqi = lqi};

        ptt_event_set_ctx_data(evt_id, (uint8_t *)(&pkt_info), sizeof(pkt_info));

        if (NULL != handler)
        {
            handler(evt_id);
        }
        else
        {
            ptt_event_free(evt_id);
        }
    }
}

void ptt_ctrl_rf_rx_failed(ptt_rf_rx_error_t rx_error)
{
    PTT_TRACE("%s -> res %d\n", __func__, rx_error);

    ptt_ext_evt_handler handler = ptt_ctrl_get_handler_rf_rx_failed();

    if (NULL != handler)
    {
        ptt_ret_t    ret;
        ptt_evt_id_t evt_id;

        ret = ptt_event_alloc(&evt_id);

        if (PTT_RET_SUCCESS != ret)
        {
            PTT_TRACE("%s: ptt_event_alloc returned error code: %d", __func__, ret);
        }
        else
        {
            ptt_event_set_ctx_data(evt_id, PTT_CAST_TO_UINT8_P(&rx_error), sizeof(rx_error));
            handler(evt_id);
        }
    }

    PTT_TRACE_FUNC_EXIT();
}

#ifdef TESTS
#include "test_rf_proc_wrappers.c"
#endif /* TESTS */