/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: radio module interface intended for external usage */

#ifndef PTT_RF_API_H__
#define PTT_RF_API_H__

#include <stdint.h>

#include "ptt_types.h"

/** @brief Provides library RF module with a received packet
 *
 *  @param pkt - pointer to a packet data
 *  @param len - length of a received data
 *  @param rssi - RSSI
 *  @param lqi - LQI
 *
 *  @return none
 */
void ptt_rf_push_packet(const uint8_t *pkt, ptt_pkt_len_t len, int8_t rssi, uint8_t lqi);

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
void ptt_rf_cca_done(bool result);

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
extern void ptt_rf_promiscuous_set_ext(bool value);
/* external provided function to start receive on current radio */
extern bool ptt_rf_receive_ext(void);
/* external provided function to put current radio on sleep */
extern bool ptt_rf_sleep_ext(void);
/* external provided function to start sending continuous carrier on current radio */
extern bool ptt_rf_continuous_carrier_ext(void);
/* external provided function to start sending modulated carrier on current radio */
extern bool ptt_rf_modulated_stream_ext(const uint8_t *pkt, ptt_pkt_len_t len);
/* external provided function to send a packet on current radio */
extern bool ptt_rf_send_packet_ext(const uint8_t *pkt, ptt_pkt_len_t len, bool cca);
/* external provided function to verify if the channel is clear */
extern bool ptt_rf_cca_ext(uint8_t mode);
/* external provided function to detects the maximum energy for a given time */
extern bool ptt_rf_ed_ext(uint32_t time_us);
/* external provided function to begin the RSSI measurement */
extern bool ptt_rf_rssi_measure_begin_ext(void);
/* external provided function to get the result of the last RSSI measurement */
extern ptt_rssi_t ptt_rf_rssi_last_get_ext(void);
/* external provided function to set PAN ID used by the device */
extern void ptt_rf_set_pan_id_ext(const uint8_t *pan_id);
/* external provided function to set extended address of the device */
extern void ptt_rf_set_extended_address_ext(const uint8_t *extended_address);
/* external provided function to set short address of the device */
extern void ptt_rf_set_short_address_ext(const uint8_t *short_address);
/* external provided function to set antenna on current radio */
extern void ptt_rf_set_antenna_ext(uint8_t antenna);
/* external provided function to set TX antenna on current radio */
extern void ptt_rf_set_tx_antenna_ext(uint8_t antenna);
/* external provided function to set RX antenna on current radio */
extern void ptt_rf_set_rx_antenna_ext(uint8_t antenna);
/* external provided function to get current rx antenna from current radio */
extern uint8_t ptt_rf_get_rx_antenna_ext(void);
/* external provided function to get current tx antenna from current radio */
extern uint8_t ptt_rf_get_tx_antenna_ext(void);
/* external provided function to get last best rx antenna from current radio */
extern uint8_t ptt_rf_get_lat_rx_best_antenna_ext(void);

#endif /* PTT_RF_API_H__ */
