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

/* Purpose: PTT interface part */

#include <assert.h>
#include <stddef.h>

#include "ptt.h"

#include "ctrl/ptt_trace.h"
#include "rf/ptt_rf.h"

#include "ptt_ctrl_internal.h"
#include "ptt_events_internal.h"
#include "ptt_modes.h"
#include "ptt_timers_internal.h"

#ifdef TESTS
#include "test_ctrl_conf.h"
#endif /* TESTS */

/** default timeout for waiting for any response */
#define PTT_DEFAULT_TIMEOUT (500u)

static ptt_ctrl_t m_ptt_ctrl_ctx;

void ptt_init(ptt_call_me_cb_t call_me_cb, ptt_time_t max_time)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    assert(call_me_cb != NULL);

    ptt_ctrl_handlers_reset_all();
    m_ptt_ctrl_ctx.call_me_cb  = call_me_cb;
    m_ptt_ctrl_ctx.rsp_timeout = PTT_DEFAULT_TIMEOUT;
    m_ptt_ctrl_ctx.max_time    = max_time;

    ptt_events_init();
    ptt_timers_init();
    ptt_rf_init();

    ret = ptt_mode_default_init();

    assert(PTT_RET_SUCCESS == ret);

    PTT_UNUSED(ret);
}

void ptt_uninit(void)
{
    ptt_mode_uninit();
    ptt_events_uninit();
    ptt_timers_uninit();
    ptt_rf_uninit();
}

void ptt_process(ptt_time_t current_time)
{
    ptt_timers_process(current_time);
}

ptt_timer_ctx_t * ptt_ctrl_get_p_timer_ctx(void)
{
    return &m_ptt_ctrl_ctx.timer_ctx;
}

void ptt_ctrl_call_me_cb(ptt_time_t timeout)
{
    if (m_ptt_ctrl_ctx.call_me_cb != NULL)
    {
        m_ptt_ctrl_ctx.call_me_cb(timeout);
    }
}

ptt_evt_ctx_t * ptt_ctrl_get_p_evt_ctx(void)
{
    return &m_ptt_ctrl_ctx.evt_ctx;
}

ptt_time_t ptt_ctrl_get_rsp_timeout(void)
{
    return m_ptt_ctrl_ctx.rsp_timeout;
}

void ptt_ctrl_set_rsp_timeout(ptt_time_t timeout)
{
    m_ptt_ctrl_ctx.rsp_timeout = timeout;
}

ptt_time_t ptt_ctrl_get_max_time(void)
{
    return m_ptt_ctrl_ctx.max_time;
}

ptt_mode_t ptt_ctrl_get_current_mode(void)
{
    return m_ptt_ctrl_ctx.current_mode;
}

void ptt_ctrl_set_current_mode(ptt_mode_t mode)
{
    m_ptt_ctrl_ctx.current_mode = mode;
}

ptt_mode_t ptt_ctrl_get_default_mode(void)
{
    uint32_t   mode_mask;
    ptt_mode_t mode = PTT_MODE_ZB_PERF_DUT;

    if (ptt_get_mode_mask_ext(&mode_mask))
    {
        for (uint8_t i = 0; i <= sizeof(mode_mask) * 8; i++)
        {
            if (((mode_mask >> i) & 1u) == 1u)
            {
                mode = i;
                break;
            }
        }
    }

    return mode;
}

uint8_t ptt_ctrl_get_hw_version(void)
{
    uint8_t hw_version = 0xFF;

    ptt_get_hw_version_ext(&hw_version);
    return hw_version;
}

uint8_t ptt_ctrl_get_sw_version(void)
{
    uint8_t sw_version = 0xFF;

    ptt_get_sw_version_ext(&sw_version);
    return sw_version;
}

void ptt_ctrl_set_dcdc(bool activate)
{
    ptt_set_dcdc_ext(activate);
}

bool ptt_ctrl_get_dcdc(void)
{
    return ptt_get_dcdc_ext();
}

ptt_ret_t ptt_ctrl_get_temp(int32_t * p_temp)
{
    ptt_ret_t ret = PTT_RET_SUCCESS;

    if (!ptt_get_temp_ext(p_temp))
    {
        ret = PTT_RET_INVALID_VALUE;
    }

    return ret;
}

ptt_ext_evts_handlers_t * ptt_ctrl_get_p_handlers(void)
{
    return &m_ptt_ctrl_ctx.mode_handlers;
}

void ptt_random_vector_generate(uint8_t * p_buff,
                                uint8_t   requested_size)
{
    for (int i = 0; i < requested_size; ++i)
    {
        p_buff[i] = ptt_random_get_ext() % UINT8_MAX;
    }
}

void ptt_ctrl_set_icache(bool enable)
{
    ptt_set_icache_ext(enable);
}

bool ptt_ctrl_get_icache(void)
{
    return ptt_get_icache_ext();
}

void ptt_ctrl_led_indication_on(void)
{
    ptt_ctrl_led_indication_on_ext();
}

void ptt_ctrl_led_indication_off(void)
{
    ptt_ctrl_led_indication_off_ext();
}

void ptt_ctrl_handlers_reset_all(void)
{
    m_ptt_ctrl_ctx.mode_handlers = (ptt_ext_evts_handlers_t){0};
}

ptt_ext_evt_handler ptt_ctrl_get_handler_uart_pkt_received(void)
{
    return m_ptt_ctrl_ctx.mode_handlers.uart_pkt_received;
}

ptt_ext_evt_handler ptt_ctrl_get_handler_rf_tx_finished(void)
{
    return m_ptt_ctrl_ctx.mode_handlers.rf_tx_finished;
}

ptt_ext_evt_handler ptt_ctrl_get_handler_rf_tx_failed(void)
{
    return m_ptt_ctrl_ctx.mode_handlers.rf_tx_failed;
}

ptt_ext_evt_handler ptt_ctrl_get_handler_rf_rx_failed(void)
{
    return m_ptt_ctrl_ctx.mode_handlers.rf_rx_failed;
}

ptt_ext_evt_handler ptt_ctrl_get_handler_rf_tx_started(void)
{
    return m_ptt_ctrl_ctx.mode_handlers.rf_tx_started;
}

ptt_ext_evt_handler ptt_ctrl_get_handler_rf_rx_done(void)
{
    return m_ptt_ctrl_ctx.mode_handlers.rf_rx_done;
}

ptt_ext_evt_handler ptt_ctrl_get_handler_rf_ed_detected(void)
{
    return m_ptt_ctrl_ctx.mode_handlers.rf_ed_detected;
}

ptt_ext_evt_handler ptt_ctrl_get_handler_rf_ed_failed(void)
{
    return m_ptt_ctrl_ctx.mode_handlers.rf_ed_failed;
}

ptt_ext_evt_handler ptt_ctrl_get_handler_rf_cca_done(void)
{
    return m_ptt_ctrl_ctx.mode_handlers.rf_cca_done;
}

ptt_ext_evt_handler ptt_ctrl_get_handler_rf_cca_failed(void)
{
    return m_ptt_ctrl_ctx.mode_handlers.rf_cca_failed;
}

#ifdef TESTS
#include "test_ctrl_wrappers.c"
#endif /* TESTS */