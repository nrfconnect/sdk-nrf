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

/* Purpose: implementation of Zigbee RF Performance Test Plan CMD mode */

#include "ctrl/ptt_trace.h"

#include "ptt_ctrl_internal.h"
#include "ptt_modes.h"
#include "ptt_zb_perf_cmd_mode.h"


ptt_ret_t ptt_zb_perf_cmd_mode_init(void)
{
    PTT_TRACE("ptt_zb_perf_cmd_mode_init ->\n");

    cmd_ota_cmd_init();
    cmd_uart_cmd_init();

    ptt_ext_evts_handlers_t * p_handlers = ptt_ctrl_get_p_handlers();

    p_handlers->rf_tx_finished    = cmd_rf_tx_finished;
    p_handlers->rf_tx_failed      = cmd_rf_tx_failed;
    p_handlers->rf_rx_done        = cmd_rf_rx_done;
    p_handlers->rf_rx_failed      = cmd_rf_rx_failed;
    p_handlers->uart_pkt_received = cmd_uart_pkt_received;
    p_handlers->rf_cca_done       = cmd_rf_cca_done;
    p_handlers->rf_cca_failed     = cmd_rf_cca_failed;
    p_handlers->rf_ed_detected    = cmd_rf_ed_detected;
    p_handlers->rf_ed_failed      = cmd_rf_ed_failed;

    PTT_TRACE("ptt_zb_perf_cmd_mode_init -<\n");
    return PTT_RET_SUCCESS;
}

ptt_ret_t ptt_zb_perf_cmd_mode_uninit(void)
{
    PTT_TRACE("ptt_zb_perf_cmd_mode_uninit ->\n");

    cmd_ota_cmd_uninit();
    cmd_uart_cmd_uninit();

    PTT_TRACE("ptt_zb_perf_cmd_mode_uninit -<\n");
    return PTT_RET_SUCCESS;
}

