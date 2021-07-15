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

/* Purpose: control module interface intended for internal usage on library
 *          level
 */

#ifndef PTT_CTRL_INTERNAL_H__
#define PTT_CTRL_INTERNAL_H__

#include "ctrl/ptt_ctrl.h"

#include "ptt_modes.h"
#include "ptt_events_internal.h"
#include "ptt_timers_internal.h"

/** type definition of function to handle event */
typedef void (* ptt_ext_evt_handler)(ptt_evt_id_t evt_id);

/** handlers for each event
 *  NULL if unsupported by current mode
 */
typedef struct
{
    ptt_ext_evt_handler rf_tx_finished;    /**< packet transmission finished */
    ptt_ext_evt_handler rf_tx_failed;      /**< packet transmission failed */
    ptt_ext_evt_handler rf_tx_started;     /**< packet transmission started */
    ptt_ext_evt_handler rf_rx_done;        /**< packet received over radio */
    ptt_ext_evt_handler rf_rx_failed;      /**< packet reception failed */
    ptt_ext_evt_handler rf_cca_done;       /**< CCA procedure finished */
    ptt_ext_evt_handler rf_cca_failed;     /**< CCA procedure failed */
    ptt_ext_evt_handler rf_ed_detected;    /**< ED procedure finished */
    ptt_ext_evt_handler rf_ed_failed;      /**< ED procedure failed */
    ptt_ext_evt_handler uart_pkt_received; /**< packet received over UART */
} ptt_ext_evts_handlers_t;

/** control module context */
typedef struct
{
    ptt_call_me_cb_t        call_me_cb;    /**< callback from application to be call when need to update nearest timeout */
    ptt_mode_t              current_mode;  /**< current device mode */
    ptt_ext_evts_handlers_t mode_handlers; /**< event handlers */
    ptt_timer_ctx_t         timer_ctx;     /**< timers context */
    ptt_evt_ctx_t           evt_ctx;       /**< events context */
    ptt_time_t              rsp_timeout;   /**< common timeout for all responses */
    ptt_time_t              max_time;      /**< maximum time supported by application timer */
} ptt_ctrl_t;

/* Get timer context */
ptt_timer_ctx_t * ptt_ctrl_get_p_timer_ctx(void);

/* Get event context */
ptt_evt_ctx_t * ptt_ctrl_get_p_evt_ctx(void);

/* Call call_me_cb - an application callback with given timeout */
void ptt_ctrl_call_me_cb(ptt_time_t timeout);

/* Get response timeout */
ptt_time_t ptt_ctrl_get_rsp_timeout(void);

/* Set response timeout */
void ptt_ctrl_set_rsp_timeout(ptt_time_t timeout);

/* Get max time */
ptt_time_t ptt_ctrl_get_max_time(void);

/* Get SoC temperature */
ptt_ret_t ptt_ctrl_get_temp(int32_t * p_temp);

/* Get current mode */
ptt_mode_t ptt_ctrl_get_current_mode(void);

/* Set current mode */
void ptt_ctrl_set_current_mode(ptt_mode_t mode);

/* Get default mode */
ptt_mode_t ptt_ctrl_get_default_mode(void);

/* Get HW version */
uint8_t ptt_ctrl_get_hw_version(void);

/* Get SW version */
uint8_t ptt_ctrl_get_sw_version(void);

/* Get external events handlers structure */
ptt_ext_evts_handlers_t * ptt_ctrl_get_p_handlers(void);

/* Disable/enable DC/DC mode */
void ptt_ctrl_set_dcdc(bool activate);

/* Get current DC/DC mode */
bool ptt_ctrl_get_dcdc(void);

/* Enable/Disable ICACHE */
void ptt_ctrl_set_icache(bool enable);

/* Get current state of ICACHE */
bool ptt_ctrl_get_icache(void);

/* Reset external events structure */
void ptt_ctrl_handlers_reset_all(void);

/* Get current handler for a packet received through UART */
ptt_ext_evt_handler ptt_ctrl_get_handler_uart_pkt_received(void);

/* Get current handler for Transmission finished event */
ptt_ext_evt_handler ptt_ctrl_get_handler_rf_tx_finished(void);

/* Get current handler for Transmission failed event */
ptt_ext_evt_handler ptt_ctrl_get_handler_rf_tx_failed(void);

/* Get current handler for Reception failed event */
ptt_ext_evt_handler ptt_ctrl_get_handler_rf_rx_failed(void);

/* Get current handler for Transmission started event */
ptt_ext_evt_handler ptt_ctrl_get_handler_rf_tx_started(void);

/* Get current handler for a packet received through RF */
ptt_ext_evt_handler ptt_ctrl_get_handler_rf_rx_done(void);

/* Get current handler for CCA done procedure */
ptt_ext_evt_handler ptt_ctrl_get_handler_rf_cca_done(void);

/* Get current handler for CCA failed procedure */
ptt_ext_evt_handler ptt_ctrl_get_handler_rf_cca_failed(void);

/* Get current handler for ED done procedure*/
ptt_ext_evt_handler ptt_ctrl_get_handler_rf_ed_detected(void);

/* Get current handler for ED failed procedure */
ptt_ext_evt_handler ptt_ctrl_get_handler_rf_ed_failed(void);

/** Get vector of random values
 *
 *  @param p_buff         - buffer to store vector, must be at least requested_size long
 *  @param requested_size - number of random bytes to store in p_buff
 *
 *  @return - None
 */
void ptt_random_vector_generate(uint8_t * p_buff,
                                uint8_t   requested_size);

/* Change state of LED indicating received packet to ON */
void ptt_ctrl_led_indication_on(void);

/* Change state of LED indicating received packet to OFF */
void ptt_ctrl_led_indication_off(void);

#endif /* PTT_CTRL_INTERNAL_H__ */

