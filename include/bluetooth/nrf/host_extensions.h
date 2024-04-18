/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_nrf_host_extensions Bluetooth vendor specific APIs
 * @{
 * @brief Bluetooth Host APIs for Nodic-LL vendor-specific commands.
 */

#ifndef BT_NRF_HOST_EXTENSIONS_H_
#define BT_NRF_HOST_EXTENSIONS_H_

#include <stdbool.h>
#include <stdint.h>

/** @brief Set remote (peer) transmit power.
 *
 *  @param conn         Connection object.
 *  @param phy          PHY bit number i.e. [1M, 2M, s8, s2] == [1, 2, 3, 4]
 *  @param delta        Requested adjustment (in dBm) for the remote to apply to
 *                      its transmit power.
 *                      The value can be 0 to utilize the response of the peer to update
 *                      the information on the transmit power setting of the remote.
 *                      Note that this is only a request to the peer, which is in control of
 *                      how, if at all, to apply changes to its transmit power.
 *
 *  @return Zero on success or (negative) error code on failure.
 *  @retval -ENOBUFS HCI command buffer is not available.
 */
int bt_conn_set_remote_tx_power_level(struct bt_conn *conn,
					 enum bt_conn_le_tx_power_phy phy, int8_t delta);

struct bt_conn_set_pcr_params {
	/** Enable or disable controller-initiated autonomous LE Power Control Request procedure.
	 *  Disabled by default.
	 */
	uint8_t auto_enable;
	/** Enable or disable Acceptable Power Reduction (APR) handling in controller during
	 *  LE Power Control Request procedure.
	 *  Disabled by default.
	 */
	uint8_t apr_enable;
	/** Value used to determine the weight of the previous average of RSSI values.
	 *  A higher value lowers how much the current RSSI weighs into the average,
	 *  flattening peaks, which also means the controller reacts slower on RSSI changes.
	 *  The valid range is [0, 4095].
	 *  Default value is 2048.
	 *
	 *  In the controller, the value is used in an exponential weighted
	 *  averaging of RSSI in a 12-bit fixed point fraction.
	 *
	 *   avg[n] = gamma * avg[n - 1] + (1 - gamma) * rssi[n]
	 *
	 *  Here, gamma equals beta/4096, and rssi[n] equals the current RSSI.
	 *
	 *  For example, for gamma to be 0.25, set the beta parameter in the command to 1024.
	 */
	uint16_t beta;
	/** The lower limit of the RSSI golden range. The RSSI golden range is explained in
	 *  Core_v5.4, Vol 6, Part B, Section 5.1.17.1.
	 *  Default value is -70 dBm.
	 */
	int8_t lower_limit;
	/** The upper limit of the RSSI golden range.
	 *  Default value is -30 dBm.
	 */
	int8_t upper_limit;
	/** Target RSSI level in dBm units when the average RSSI level is less than
	 *  the lower limit of the RSSI golden range.
	 *  Default value is -65 dBm.
	 */
	int8_t lower_target_rssi;
	/** Target RSSI level in dBm units when the average RSSI level is greater than
	 *  the upper limit of the RSSI golden range.
	 *  Default value is -35 dBm.
	 */
	int8_t upper_target_rssi;
	/** Duration in milliseconds to wait before initiating a new LE Power Control Request
	 *  procedure by the controller.
	 *  It is needed to not repeat send requests for transmit power change without the remote
	 *  having had the chance to react, as well as to avoid a busy controller.
	 *  This value should be set depending on needs.
	 *  0 milliseconds value is an invalid value.
	 *  Default value is 5000 milliseconds.
	 */
	uint16_t wait_period_ms;
	/** Margin between APR value received from peer in LL_POWER_CONTROL_RSP PDU
	 *  and actual reduction in Transmit power that is applied locally.
	 *  The applied decrease in local Transmit power will be
	 *  (received_apr - apr_margin)
	 *  if received_apr > apr_margin, otherwise no change.
	 *  Default value is 5 dB.
	 */
	uint8_t apr_margin;
};

/** @brief Set Power Control request parameter
 *
 *  @param params        See @ref bt_conn_set_pcr_params for details.
 *
 *  @return Zero on success or (negative) error code on failure.
 *  @retval -ENOBUFS HCI command buffer is not available.
 */
int bt_conn_set_power_control_request_params(struct bt_conn_set_pcr_params *params);

/** @brief Reduce the priority of the initiator when following AUX packets.
 *
 *  @param reduce Set to true to reduce the priority. Set to false to restore the default priority.
 *
 *  @return Zero on success or (negative) error code on failure.
 *  @retval -ENOBUFS HCI command buffer is not available.
 */
int bt_nrf_host_extension_reduce_initator_aux_channel_priority(bool reduce);

/**
 * @}
 */

#endif /* BT_NRF_HOST_EXTENSIONS_H_ */
