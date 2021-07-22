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

#ifndef PTT_CTRL_H__
#define PTT_CTRL_H__

#include "ptt_events.h"
#include "ptt_types.h"

/** @brief Push a received packet from rf to ctrl module
 *
 *  Provides the control module with a packet received over the air
 *
 *  @param p_pkt - packet received over the air
 *  @param len   - length of the packet
 *  @param rssi  - RSSI of the packet
 *  @param lqi   - LQI of the packet
 *
 *  @return none
 */
void ptt_ctrl_rf_push_packet(const uint8_t * p_pkt,
                             ptt_pkt_len_t   len,
                             int8_t          rssi,
                             uint8_t         lqi);

/** @brief Pass an event "Transmission started"
 *
 *  Pass to the control module external event about starting transmission
 *
 *  @param evt_id - id of currently processed event
 */
void ptt_ctrl_rf_tx_started(ptt_evt_id_t evt_id);

/** @brief Pass an event "Transmission finished"
 *
 *  Pass to the control module external event about finished transmission
 *
 *  @param evt_id - id of currently processed event
 */
void ptt_ctrl_rf_tx_finished(ptt_evt_id_t evt_id);

/** @brief Pass an event "Transmission failed"
 *
 *  Pass to the control module external event about failed transmission
 *
 *  @param evt_id - id of currently processed event
 *  @param tx_error - a reason of transmission fail
 */
void ptt_ctrl_rf_tx_failed(ptt_evt_id_t evt_id, ptt_rf_tx_error_t tx_error);

/** @brief Pass an event "Reception failed"
 *
 *  Pass to the control module information about failed reception
 *
 *  @param rx_error - a reason of reception fail
 */
void ptt_ctrl_rf_rx_failed(ptt_rf_rx_error_t rx_error);

/** @brief Pass an event "ED detected" and a result
 *
 *  Pass to the control module external event about finished ED procedure and its result
 *
 *  @param evt_id - id of currently processed event
 *  @param result - result of "ED detected" procedure
 */
void ptt_ctrl_rf_ed_detected(ptt_evt_id_t evt_id, ptt_ed_t result);

/** @brief Pass an event "ED failed"
 *
 *  Pass to the control module external event about failed ED procedure
 *
 *  @param evt_id - id of currently processed event
 */
void ptt_ctrl_rf_ed_failed(ptt_evt_id_t evt_id);

/** @brief Pass an event "CCA done" and a result
 *
 *  Pass to the control module external event about finished CCA procedure and its result
 *
 *  @param evt_id - id of currently processed event
 *  @param result - result of CCA procedure
 */
void ptt_ctrl_rf_cca_done(ptt_evt_id_t evt_id, ptt_cca_t result);

/** @brief Pass an event "CCA failed" and a result
 *
 *  Pass to the control module external event about finished CCA procedure and its result
 *
 *  @param evt_id - id of currently processed event
 */
void ptt_ctrl_rf_cca_failed(ptt_evt_id_t evt_id);

#endif /* PTT_CTRL_H__ */