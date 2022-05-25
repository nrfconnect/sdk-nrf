/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BLE_TRANS_H_
#define _BLE_TRANS_H_

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/conn.h>

/* Connection interval is calculated as x*1.25 */
#if ((CONFIG_AUDIO_FRAME_DURATION_US == 7500) && CONFIG_SW_CODEC_LC3)
/* Connection interval of 7.5 ms */
#define BLE_ISO_CONN_INTERVAL 6
#elif (CONFIG_AUDIO_FRAME_DURATION_US == 10000)
/* Connection interval of 10 ms */
#define BLE_ISO_CONN_INTERVAL 8
#else
#error "Invalid frame duration"
#endif /* ((CONFIG_AUDIO_FRAME_DURATION_US == 7500) && CONFIG_SW_CODEC_LC3) */

enum ble_trans_chan_type {
	BLE_TRANS_CHANNEL_RETURN_MONO = 0,
	BLE_TRANS_CHANNEL_LEFT = 0,
	BLE_TRANS_CHANNEL_RIGHT,
	BLE_TRANS_CHANNEL_STEREO,
	BLE_TRANS_CHANNEL_NUM,
};

enum iso_transport {
	TRANS_TYPE_NOT_SET, //< Default transport type. Not in use.
	TRANS_TYPE_BIS, //< Broadcast isochronous stream.
	TRANS_TYPE_CIS, //< Connected isochronous stream.
	TRANS_TYPE_NUM, //< Number of transport types.
};

enum iso_direction {
	DIR_NOT_SET,
	DIR_RX,
	DIR_TX,
	DIR_BIDIR,
	DIR_NUM,
};

/**@brief  BLE events
 */
enum ble_evt_type {
	BLE_EVT_CONNECTED,
	BLE_EVT_DISCONNECTED,
	BLE_EVT_LINK_READY,
	BLE_EVT_STREAMING,
	BLE_EVT_NUM_EVTS
};

/**@brief	BLE data callback type.
 *
 * @param data			Pointer to received data
 * @param size			Size of received data
 * @param bad_frame		Indicating if the frame is a bad frame or not
 * @param sdu_ref		ISO timestamp
 */
typedef void (*ble_trans_iso_rx_cb_t)(const uint8_t *const data, size_t size, bool bad_frame,
				      uint32_t sdu_ref);

/**@brief	Enable the ISO packet lost notify feature
 *
 * @return	0 for success, error otherwise.
 */
int ble_trans_iso_lost_notify_enable(void);

/**@brief	Send data over the ISO transport
 *		Could be either CIS or BIS dependent on configuration
 * @param data	Data to send
 * @param size	Size of data to send
 * @param chan_type Channel type (stereo or mono)
 *
 * @return	0 for success, error otherwise.
 */
int ble_trans_iso_tx(uint8_t const *const data, size_t size, enum ble_trans_chan_type chan_type);

/**@brief	Start iso stream
 *
 * @note	Type and direction is set by init
 *
 * @return	0 for success, error otherwise
 */
int ble_trans_iso_start(void);

/**@brief	Stop iso stream
 *
 * @note	Type and direction is set by init
 *
 * @return	0 for success, error otherwise
 */
int ble_trans_iso_stop(void);

/**
 * @brief    Trigger the scan for BIS
 *
 * @return	0 for success, error otherwise
 */
int ble_trans_iso_bis_rx_sync_get(void);

/**@brief Create ISO CIG
 *
 * @return 0 if successful, error otherwise
 */
int ble_trans_iso_cig_create(void);

/**@brief	Connect CIS ISO channel
 *
 * @param	conn ACL connection for CIS to connect
 *
 * @return	0 if successful, error otherwise
 */
int ble_trans_iso_cis_connect(struct bt_conn *conn);

/**
 * @brief    Get most recent anchor point for ISO TX
 *
 * @details This function uses BT_HCI_OP_LE_READ_ISO_TX_SYNC HCI command to retrieve
 *          anchor point information from the controller.
 *          Connections in the same group will have the same anchor, and either channel
 *          (when connected) can be used as a handle reference.
 *          Use @ref BLE_TRANS_CHANNEL_STEREO as a wildcard if the exact channel does not matter.
 *
 * @note Should not be called directly from an ISR, or context that could deadlock HCI.
 *
 * @param[in]	chan_type Channel to get anchor point from.
 * @param[out]	timestamp Timestamp value from HCI response. Can be NULL.
 * @param[out]	offset Contains offset value from HCI response. Can be NULL.
 *
 * @return	0 for success, error otherwise
 */
int ble_trans_iso_tx_anchor_get(enum ble_trans_chan_type chan_type, uint32_t *timestamp,
				uint32_t *offset);

/**@brief	Initialize either a CIS or BIS transport
 *
 * @return	0 for success, error otherwise
 */

int ble_trans_iso_init(enum iso_transport trans_type, enum iso_direction dir,
		       ble_trans_iso_rx_cb_t rx_cb);

#endif /* _BLE_TRANS_H_ */
