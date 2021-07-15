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

#ifndef PTT_MODE_ZB_PERF_DUT_H__
#define PTT_MODE_ZB_PERF_DUT_H__

#include <assert.h>

#include "ctrl/ptt_ctrl.h"
#include "ctrl/ptt_events.h"
#include "ctrl/ptt_timers.h"
#include "ctrl/ptt_uart.h"

#include "ptt_errors.h"

/** possible states of library in DUT mode */
typedef enum
{
    DUT_STATE_IDLE = 0,               /**< waiting for next command */
    DUT_STATE_ACK_SENDING,            /**< rf module send ack command */
    DUT_STATE_POWER_SENDING,          /**< DUT sends power */
    DUT_STATE_STREAM_EMITTING,        /**< DUT emits modulated stream */
    DUT_STATE_RX_TEST_WAIT_FOR_END,   /**< rf module wait for end_rx_test command */
    DUT_STATE_RX_TEST_REPORT_SENDING, /**< rf module send end_rx_test command */
    DUT_STATE_HW_VERSION_SENDING,     /**< DUT sends hardware version */
    DUT_STATE_SW_VERSION_SENDING,     /**< DUT sends software version */
    DUT_STATE_ANTENNA_SENDING,        /**< DUT sends antenna */
    DUT_STATE_N                       /**< states count */
} dut_state_t;

#endif  /* PTT_MODE_ZB_PERF_DUT_H__ */

