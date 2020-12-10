/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_GADGETS_H_
#define BT_GADGETS_H_

/**
 * @file
 * @defgroup bt_gadgets Alexa Gadgets GATT Service
 * @{
 * @brief Alexa Gadgets GATT Service API.
 * @details Implements the Alexa Gadgets GATT Service,
 *          and handles the data format (streams) that wraps the data payload
 *          sent over this Service.
 *          See Alexa Gadgets documentation for further information:
 *          https://developer.amazon.com/en-US/docs/alexa/alexa-gadgets-toolkit/packet-ble.html
 */

#include <zephyr/types.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief UUID of the Gadgets Service. */
#define BT_UUID_GADGETS_VAL \
	BT_UUID_128_ENCODE(0x0000FE03, 0x0000, 0x1000, 0x8000, 0x00805F9B34FB)

/** @brief Short version of the Gadgets Service UUID.
 *         Used in advertisement packets.
 */
#define BT_UUID_GADGETS_SHORT_VAL 0xfe03

/** @brief UUID of the TX Characteristic. */
#define BT_UUID_GADGETS_TX_VAL \
	BT_UUID_128_ENCODE(0xF04EB177, 0x3005, 0x43A7, 0xAC61, 0xA390DDF83076)

/** @brief UUID of the RX Characteristic. */
#define BT_UUID_GADGETS_RX_VAL \
	BT_UUID_128_ENCODE(0x2BEEA05B, 0x1879, 0x4BB4, 0x8A2F, 0x72641F82420B)

#define BT_UUID_GADGETS    BT_UUID_DECLARE_128(BT_UUID_GADGETS_VAL)
#define BT_UUID_GADGETS_RX BT_UUID_DECLARE_128(BT_UUID_GADGETS_RX_VAL)
#define BT_UUID_GADGETS_TX BT_UUID_DECLARE_128(BT_UUID_GADGETS_TX_VAL)


/** @brief Gadgets Service result codes. */
enum bt_gadget_result {
	/** Gadget result code: success. */
	BT_GADGETS_RESULT_CODE_SUCCESS = 0x00,
	/** Gadget result code: unknown error. */
	BT_GADGETS_RESULT_CODE_UNKNOWN = 0x01,
	/** Gadget result code: unsupported error. */
	BT_GADGETS_RESULT_CODE_UNSUPPORTED = 0x03,
};

/** @brief Gadgets Service stream types.
 *         See Gadgets documentation for details:
 *         https://developer.amazon.com/en-US/docs/alexa/alexa-gadgets-toolkit/packet-ble.html#streams
 */
enum bt_gadgets_stream_id {
	/** Control stream ID */
	BT_GADGETS_STREAM_ID_CONTROL = 0b0000,
	/** Alexa stream ID */
	BT_GADGETS_STREAM_ID_ALEXA = 0b0110,
	/** OTA stream ID */
	BT_GADGETS_STREAM_ID_OTA = 0b0010,
};

/** @brief Callback type for data received.
 *         Returns false if type is unsupported.
 */
typedef bool (*bt_gadgets_stream_cb_t)(struct bt_conn *conn,
				       const uint8_t *const data,
				       uint16_t len,
				       bool more_data);

/** @brief Callback type for data sent. */
typedef void (*bt_gadgets_sent_cb_t)(struct bt_conn *conn,
				     const void *buf,
				     bool success);

/** @brief Callback type for CCCD updated. */
typedef void (*bt_gadgets_ccc_update_cb_t)(struct bt_conn *conn, bool enabled);

/** @brief Pointers to the callback functions for service events. */
struct bt_gadgets_cb {

	/** Callback for incoming control streams. */
	bt_gadgets_stream_cb_t control_stream_cb;

	/** Callback for incoming Alexa streams. */
	bt_gadgets_stream_cb_t alexa_stream_cb;

	/** Callback for stream and non-stream data sent. */
	bt_gadgets_sent_cb_t sent_cb;

	/** Callback for CCC descriptor updated. */
	bt_gadgets_ccc_update_cb_t ccc_cb;
};

/**@brief Initialize the service.
 *
 * @details This function registers the Gadgets BLE service with
 *          two characteristics, TX and RX.
 *
 * @param[in] callbacks  Struct with function pointers to callbacks for service
 *                       events.
 *
 * @retval 0 If initialization is successful.
 *           Otherwise, a negative value is returned.
 */
int bt_gadgets_init(const struct bt_gadgets_cb *callbacks);

/**@brief Send data in stream format.
 *
 * @details This function encapsulates the data in the stream format
 *          and transmits to the peer device.
 *
 * @param[in] conn Pointer to connection object.
 * @param[in] stream Stream type to use.
 * @param[in] data Pointer to a data buffer. Buffer must be kept valid until
 *          @ref bt_gadgets_sent_cb_t is triggered.
 * @param[in] size Size of the data in the buffer in bytes.
 *
 * @retval 0 If the data is sent.
 *           Otherwise, a negative value is returned.
 */
int bt_gadgets_stream_send(struct bt_conn *conn,
			   enum bt_gadgets_stream_id stream,
			   void *data,
			   uint16_t size);

/**@brief Send data without stream formatting.
 *
 * @details No encapsulation or fragmentation is done of this data. Data buffer
 *          does not need to be kept valid after this function returns.
 *
 * @param[in] conn Pointer to connection object.
 * @param[in] data Pointer to a data buffer.
 * @param[in] size Size of the data in the buffer in bytes.
 *
 * @retval 0 If the data is sent.
 *           Otherwise, a negative value is returned.
 */
int bt_gadgets_send(struct bt_conn *conn, void *data, uint16_t size);

/**@brief Get maximum data length that can be used for
 *        @ref bt_gadgets_send.
 *
 * @param[in] conn Pointer to connection object.
 *
 * @return Maximum data length.
 */
static inline uint16_t bt_gadgets_max_send(struct bt_conn *conn)
{
	/* According to 3.4.7.1 Handle Value Notification */
	/* off the ATT protocol. */
	/* Maximum supported notification is ATT_MTU - 3 */
	return bt_gatt_get_mtu(conn) - 3;
}

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* BT_GADGETS_H_ */
