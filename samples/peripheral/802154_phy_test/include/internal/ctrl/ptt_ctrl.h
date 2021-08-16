/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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
 *  @param pkt - packet received over the air
 *  @param len   - length of the packet
 *  @param rssi  - RSSI of the packet
 *  @param lqi   - LQI of the packet
 *
 *  @return none
 */
void ptt_ctrl_rf_push_packet(const uint8_t *pkt, ptt_pkt_len_t len, int8_t rssi, uint8_t lqi);

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
void ptt_ctrl_rf_cca_done(ptt_evt_id_t evt_id, bool result);

/** @brief Pass an event "CCA failed" and a result
 *
 *  Pass to the control module external event about finished CCA procedure and its result
 *
 *  @param evt_id - id of currently processed event
 */
void ptt_ctrl_rf_cca_failed(ptt_evt_id_t evt_id);

#endif /* PTT_CTRL_H__ */
