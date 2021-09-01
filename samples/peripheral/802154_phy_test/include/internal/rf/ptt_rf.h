/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: radio module interface intended for internal usage on library
 *          level
 */

#ifndef PTT_RF_H__
#define PTT_RF_H__

#include <stdint.h>

#include "ptt_config.h"
#include "ctrl/ptt_events.h"
#include "ptt_errors.h"
#include "ptt_types.h"

#define PTT_CHANNEL_MIN (11u)
#define PTT_CHANNEL_MAX (26u)

#define PTT_PANID_ADDRESS_SIZE (2u)
#define PTT_SHORT_ADDRESS_SIZE (2u)
#define PTT_EXTENDED_ADDRESS_SIZE (8u)

/* received packets statistic */
struct ptt_rf_stat_s {
	uint32_t total_pkts; /**< total received packets */
	uint32_t total_lqi; /**< sum of lqi of received packets */
	uint32_t total_rssi; /**< sum of rssi of received packets */
};

/** payload for `custom ltx` command */
struct ptt_ltx_payload_s {
	uint8_t arr[PTT_CUSTOM_LTX_PAYLOAD_MAX_SIZE]; /**< raw PHY packet */
	uint8_t len; /**< length of used part of arr */
};

/* information that came with the packet */
struct ptt_rf_packet_info_s {
	uint8_t lqi;
	int8_t rssi;
};

/** @brief RF module context initialization
 *
 *  @param none
 *
 *  @return none
 */
void ptt_rf_init(void);

/** @brief RF module context unitialization
 *
 *  @param none
 *
 *  @return none
 */
void ptt_rf_uninit(void);

/** @brief RF module reset to default
 *
 *  @param none
 *
 *  @return none
 */
void ptt_rf_reset(void);

/** @brief Send a packet to a transmitter
 *
 *  @param evt_id - event id locking this transmission
 *  @param pkt - data to be transmitted
 *  @param len - length of the data
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_send_packet(ptt_evt_id_t evt_id, const uint8_t *pkt, ptt_pkt_len_t len);

/** @brief Set channel mask to radio driver
 *
 *  @param evt_id - event id locking setting mask
 *  @param mask - channel bit mask, if more than one bit enabled, least significant will be used
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_set_channel_mask(ptt_evt_id_t evt_id, uint32_t mask);

/** @brief Set channel to radio driver
 *
 *  @param evt_id - event id locking setting channel
 *  @param channel - channel number (11-26)
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_set_channel(ptt_evt_id_t evt_id, uint8_t channel);

/** @brief Set short address to radio driver
 *
 *  @param evt_id - event id locking setting address
 *  @param short_address  Pointer to the short address (2 bytes, little-endian)
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_set_short_address(ptt_evt_id_t evt_id, const uint8_t *short_addr);

/** @brief Set extended address to radio driver
 *
 *  @param evt_id - event id locking setting address
 *  @param extended_address  Pointer to the extended address (8 bytes, little-endian)
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_set_extended_address(ptt_evt_id_t evt_id, const uint8_t *extended_addr);

/** @brief Set pan id to radio driver
 *
 *  @param evt_id - event id locking setting PAN id
 *  @param  pan_id  Pointer to the PAN ID (2 bytes, little-endian)
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_set_pan_id(ptt_evt_id_t evt_id, const uint8_t *pan_id);

/** @brief Returns number of current channel
 *
 *  @param none
 *
 *  @return uint8_t - current channel number
 */
uint8_t ptt_rf_get_channel(void);

/** @brief Set power to radio driver
 *
 *  @param evt_id - event id locking setting power
 *  @param power - power in dbm
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_set_power(ptt_evt_id_t evt_id, int8_t power);

/** @brief Returns current power
 *
 *  @param none
 *
 *  @return int8_t - current power
 */
int8_t ptt_rf_get_power(void);

/** @brief Set antenna to radio driver
 *
 *  @param evt_id - event id locking setting antenna
 *  @param antenna - is value in range [0-255]
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_set_antenna(ptt_evt_id_t evt_id, uint8_t antenna);

/** @brief Set TX antenna to radio driver
 *
 *  @param evt_id - event id locking setting antenna
 *  @param antenna - is value in range [0-255]
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_set_tx_antenna(ptt_evt_id_t evt_id, uint8_t antenna);

/** @brief Set RX antenna to radio driver
 *
 *  @param evt_id - event id locking setting antenna
 *  @param antenna - is value in range [0-255]
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_set_rx_antenna(ptt_evt_id_t evt_id, uint8_t antenna);

/** @brief Returns current rx antenna
 *
 *  @param none
 *
 *  @return uint8_t - current rx antenna
 */
uint8_t ptt_rf_get_rx_antenna(void);

/** @brief Returns current tx antenna
 *
 *  @param none
 *
 *  @return uint8_t - current tx antenna
 */
uint8_t ptt_rf_get_tx_antenna(void);

/** @brief Returns last best rx antenna
 *
 *  @param none
 *
 *  @return uint8_t - last best rx antenna
 */
uint8_t ptt_rf_get_last_rx_best_antenna(void);

/** @brief Calls radio driver to verify if the channel is clear
 *
 *  @param evt_id - event id locking radio module
 *  @param mode - CCA mode
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_cca(ptt_evt_id_t evt_id, uint8_t mode);

/** @brief Calls radio driver to toggle performing CCA on TX
 *
 *  @param evt_id - event id locking radio module
 *  @param activate - CCA toggle
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_set_cca(ptt_evt_id_t evt_id, uint8_t activate);

/** @brief Calls radio driver to detect the maximum energy for a given time
 *
 *  @param evt_id - event id locking radio module
 *  @param time_us - duration of energy detection procedure.
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_ed(ptt_evt_id_t evt_id, uint32_t time_us);

/** @brief Calls radio driver to begin the RSSI measurement
 *
 *  @param evt_id - event id locking static module
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_rssi_measure_begin(ptt_evt_id_t evt_id);

/** @brief Calls radio driver to get the result of the last RSSI measurement
 *
 *  @param[IN]  evt_id - event id locking static module
 *  @param[OUT] rssi   - RSSI measurement result in dBm, unchanged if error code returned
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_rssi_last_get(ptt_evt_id_t evt_id, ptt_rssi_t *rssi);

/** @brief Convert channel bitmask into a channel number
 *
 *  @param mask - channel bit mask, if more than one bit enabled, least significant will be used
 *
 *  @return uint8_t - channel number or 0, if there is no valid channels inside mask
 */
uint8_t ptt_rf_convert_channel_mask_to_num(uint32_t mask);

/** @brief Clear statistic and enable statistic gathering in RF module
 *
 *  @param evt_id - event id locking statistic control
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_start_statistic(ptt_evt_id_t evt_id);

/** @brief Disable statistic gathering in RF module
 *
 *  @param evt_id - event id locking statistic control
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_end_statistic(ptt_evt_id_t evt_id);

/** @brief Get gathered statistic from RF module
 *
 *  @param none
 *
 *  @return struct ptt_rf_stat_s - structure with gathered statistic
 */
struct ptt_rf_stat_s ptt_rf_get_stat_report(void);

/** @brief Start modulated stream transmission
 *
 *  @param evt_id - event id locking this transmission
 *  @param pkt  - payload to modulate carrier with
 *  @param len    - length of payload to modulate carrier with
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_start_modulated_stream(ptt_evt_id_t evt_id, const uint8_t *pkt,
					   ptt_pkt_len_t len);

/** @brief Stop modulated stream transmission
 *
 *  @param evt_id - event id locking this transmission
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_stop_modulated_stream(ptt_evt_id_t evt_id);

/** @brief Start continuous carrier transmission
 *
 *  @param evt_id - event id locking this transmission
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_start_continuous_carrier(ptt_evt_id_t evt_id);

/** @brief Stop continuous carrier transmission
 *
 *  @param evt_id - event id locking this transmission
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_stop_continuous_carrier(ptt_evt_id_t evt_id);

/** @brief Getter for payload of custom ltx command
 *
 *  @param none
 *
 *  @return struct ptt_ltx_payload_s * pointer to payload
 */
struct ptt_ltx_payload_s *ptt_rf_get_custom_ltx_payload(void);

/** @brief Change radio state to receiving
 *
 *  @param evt_id - event id locking this operation
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_receive(void);

/** @brief Change radio state to sleeping
 *
 *  @param evt_id - event id locking this operation
 *
 *  @return enum ptt_ret - PTT_RET_SUCCESS or error
 */
enum ptt_ret ptt_rf_sleep(void);

#endif /* PTT_RF_H__ */
