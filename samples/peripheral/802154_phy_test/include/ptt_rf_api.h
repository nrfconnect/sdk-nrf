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

/* Purpose: radio module interface intended for external usage */

#ifndef PTT_RF_API_H__
#define PTT_RF_API_H__

#include <stdint.h>

#include "ptt_types.h"

/** @brief Provides library RF module with a received packet
 *
 *  @param p_pkt - pointer to a packet data
 *  @param len - length of a received data
 *  @param rssi - RSSI
 *  @param lqi - LQI
 *
 *  @return none
 */
void ptt_rf_push_packet(const uint8_t * p_pkt, ptt_pkt_len_t len, int8_t rssi, uint8_t lqi);

/** @brief Provides library RF module with "Transmission started" event
 *
 *  @param none
 *
 *  @return none
 */
void ptt_rf_tx_started(void);

/** @brief Provides library RF module with "Transmission finished" event
 *
 *  @param none
 *
 *  @return none
 */
void ptt_rf_tx_finished(void);

/** @brief Provides library RF module with "Transmission failed" event
 *
 *  @param tx_error - reason of transmission fail
 *
 *  @return none
 */
void ptt_rf_tx_failed(ptt_rf_tx_error_t tx_error);

/** @brief Provides library RF module with "Reception failed" event
 *
 *  @param rx_error - reason of reception fail
 *
 *  @return none
 */
void ptt_rf_rx_failed(ptt_rf_rx_error_t rx_error);

/** @brief Provides library RF module with "CCA done" and a result
 *
 *  @param result - result of CCA procedure
 *
 *  @return none
 */
void ptt_rf_cca_done(ptt_cca_t result);

/** @brief Provides library RF module with "CCA failed"
 *
 *  @param none
 *
 *  @return none
 */
void ptt_rf_cca_failed(void);

/** @brief Provides library RF module with "ED detected" and a result
 *
 *  @param result - result of ED procedure
 *
 *  @return none
 */
void ptt_rf_ed_detected(ptt_ed_t result);

/** @brief Provides library RF module with "ED failed"
 *
 *  @param none
 *
 *  @return none
 */
void ptt_rf_ed_failed(void);

/* external provided function to set channel on current radio */
extern void ptt_rf_set_channel_ext(uint8_t channel);
/* external provided function to set power on current radio */
extern void ptt_rf_set_power_ext(int8_t power);
/* external provided function to get current power from current radio */
extern int8_t ptt_rf_get_power_ext(void);
/* external provided function to set promiscuous mode on current radio */
extern void ptt_rf_promiscuous_set_ext(ptt_bool_t value);
/* external provided function to start receive on current radio */
extern ptt_bool_t ptt_rf_receive_ext(void);
/* external provided function to put current radio on sleep */
extern ptt_bool_t ptt_rf_sleep_ext(void);
/* external provided function to start sending continuous carrier on current radio */
extern ptt_bool_t ptt_rf_continuous_carrier_ext(void);
/* external provided function to start sending modulated carrier on current radio */
extern ptt_bool_t ptt_rf_modulated_stream_ext(const uint8_t * p_pkt, ptt_pkt_len_t len);
/* external provided function to send a packet on current radio */
extern ptt_bool_t ptt_rf_send_packet_ext(const uint8_t * p_pkt, ptt_pkt_len_t len, ptt_bool_t cca);
/* external provided function to verify if the channel is clear */
extern ptt_bool_t ptt_rf_cca_ext(uint8_t mode);
/* external provided function to detects the maximum energy for a given time */
extern ptt_bool_t ptt_rf_ed_ext(uint32_t time_us);
/* external provided function to begin the RSSI measurement */
extern ptt_bool_t ptt_rf_rssi_measure_begin_ext(void);
/* external provided function to get the result of the last RSSI measurement */
extern ptt_rssi_t ptt_rf_rssi_last_get_ext(void);
/* external provided function to set PAN ID used by the device */
extern void ptt_rf_set_pan_id_ext(const uint8_t * p_pan_id);
/* external provided function to set extended address of the device */
extern void ptt_rf_set_extended_address_ext(const uint8_t * p_extended_address);
/* external provided function to set short address of the device */
extern void ptt_rf_set_short_address_ext(const uint8_t * p_short_address);
/* external provided function to set antenna on current radio */
extern void ptt_rf_set_antenna_ext(uint8_t antenna);
/* external provided function to get current antenna from current radio */
extern uint8_t ptt_rf_get_antenna_ext(void);

#endif /* PTT_RF_API_H__ */