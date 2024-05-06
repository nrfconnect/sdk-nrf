/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ISO_TIME_SYNC_H__
#define ISO_TIME_SYNC_H__

/** Definitions for the ISO time sync sample.
 *
 * This file contains common definitions and API declarations
 * used by the sample.
 */

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/bluetooth/iso.h>

#define SDU_SIZE_BYTES 5 /* SDU = [btn_val, 4 byte sdu_counter]*/

/** Start BIS transmitter demo.
 *
 * @param retransmission_number    Requested retransmission number.
 * @param max_transport_latency_ms Requested maximum transport latency.
 */
void bis_transmitter_start(uint8_t retransmission_number, uint16_t max_transport_latency_ms);

/** Start BIS receiver demo.
 *
 * @param bis_index_to_sync_to BIS index that it will sync to.
 */
void bis_receiver_start(uint8_t bis_index_to_sync_to);

/** Start CIS central demo.
 *
 * @param do_tx Set to true to send SDUs, otherwise it will receive.
 * @param retransmission_number    Requested retransmission number.
 * @param max_transport_latency_ms Requested maximum transport latency.
 */
void cis_central_start(bool do_tx,
		       uint8_t retransmission_number,
		       uint16_t max_transport_latency_ms);

/** Start CIS peripheral demo.
 *
 * @param do_tx Set to true to send SDUs, otherwise it will receive.
 */
void cis_peripheral_start(bool do_tx);

/** Initialize TX path channels.
 *
 * @param retransmission_number Requested retransmission number (if central or broadcaster).
 * @param iso_connected_cb Callback that is triggered when the ISO channel connects.
 *						   This can be set to NULL.
 */
void iso_tx_init(uint8_t retransmission_number, void (*iso_connected_cb)(void));

/** Initialize RX path channel.
 *
 * @param retransmission_number Requested retransmission number (if central).
 * @param Callback that is triggered when an ISO RX channel disconnects.
 *                 This can be set to NULL.
 */
void iso_rx_init(uint8_t retransmission_number, void (*iso_disconnected_cb)(void));

/** Obtain pointer to TX channels.
 *
 * @retval Pointer to the TX channels.
 */
struct bt_iso_chan **iso_tx_channels_get(void);

/** Obtain pointer to RX channels.
 *
 * @retval Pointer to the RX channels.
 */
struct bt_iso_chan **iso_rx_channels_get(void);

/** Print info ISO channel information.
 *
 * @param info ISO channel info.
 * @param role The role of the ISO channel.
 */
void iso_chan_info_print(struct bt_iso_info *info, uint8_t role);

/** Obtain the current Bluetooth controller time.
 *
 * The ISO timestamps are based upon this clock.
 *
 * @retval The current controller time.
 */
uint64_t controller_time_us_get(void);

/** Sets the controller to trigger a PPI event at the given timestamp.
 *
 * @param timestamp_us The timestamp where it will trigger.
 */
void controller_time_trigger_set(uint64_t timestamp_us);

/** Get the address of the event that will trigger.
 *
 * @retval The address of the event that will trigger.
 */
uint32_t controller_time_trigger_event_addr_get(void);

/** Initialize the module handling timed toggling of an LED.
 *
 * @retval 0 on success, failure otherwise.
 */
int timed_led_toggle_init(void);

/** Toggle the led to the give value at the given timestamp.
 *
 * @param value The LED value.
 * @param timestamp_us The time when the led will be set.
 *                     The time is specified in controller clock units.
 */
void timed_led_toggle_trigger_at(uint8_t value, uint32_t timestamp_us);

#endif
