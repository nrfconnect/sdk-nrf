/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_latency Bluetooth LE GATT Latency Service API
 * @{
 * @brief API for the Bluetooth LE GATT Latency Service.
 */

#ifndef BT_LATENCY_H_
#define BT_LATENCY_H_

#include <bluetooth/uuid.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt_dm.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Latency callback structure. */
struct bt_latency_cb {
	/** @brief Latency received callback.
	 *
	 * This function is called when a GATT write request has been received
	 * by the Latency characteristic.
	 *
	 * It is optional for an application to handle the write request when
	 * measuring the latency.
	 *
	 * @param[in] buf Latency data.
	 * @param[in] len Latency data length.
	 */
	void (*latency_request)(const void *buf, uint16_t len);
};

/** @brief Latency structure. */
struct bt_latency {
	/** Characteristic handle. */
	uint16_t handle;

	/** Connection object. */
	struct bt_conn *conn;

	/** Internal state. */
	atomic_t state;
};

/** @brief Latency Service UUID. */

#define BT_UUID_LATENCY_VAL \
	UTIL_EXPAND(0x3A, 0x81, 0xC0, 0x58, 0xDE, 0xFD, 0x46, 0x34, \
		    0x9B, 0xF3, 0xDB, 0x58, 0x01, 0x6E, 0x13, 0x67)

#define BT_UUID_LATENCY BT_UUID_DECLARE_128(BT_UUID_LATENCY_VAL)

/** @brief UUID of the Latency Characteristic. **/
#define BT_UUID_LATENCY_CHAR \
	BT_UUID_DECLARE_128(0x3A, 0x81, 0xC0, 0x58, 0xDE, 0xFD, 0x46, 0x34, \
			    0x9B, 0xF3, 0xDB, 0x58, 0x02, 0x6E, 0x13, 0x67)

/** @brief Initialize the GATT latency service.
 *
 *  @param[in] latency Latency service instance.
 *  @param[in] cb Callbacks.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a negative error code is returned.
 *  @retval (-EINVAL) Special error code used when the input
 *          parameters are invalid.
 *  @retval (-EALREADY) Special error code used when the latency
 *          service has been initialed.
 */
int bt_latency_init(struct bt_latency *latency,
		    const struct bt_latency_cb *cb);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_LATENCY_H_ */
