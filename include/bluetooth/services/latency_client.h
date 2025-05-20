/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_LATENCY_CLIENT_H_
#define BT_LATENCY_CLIENT_H_

/**
 * @file
 * @defgroup bt_latency_c Bluetooth LE GATT Latency Client API
 * @{
 * @brief API for the Bluetooth LE GATT Latency Client.
 */

#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/gatt_dm.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Latency client callback structure. */
struct bt_latency_client_cb {
	/** @brief Latency received callback.
	 *
	 * This function is called when a GATT write response has been received
	 * by the Latency characteristic.
	 *
	 * It is mandatory for an application to handle the write response when
	 * measuring the latency.
	 *
	 * @param[in] buf Latency data.
	 * @param[in] len Latency data length.
	 */
	void (*latency_response)(const void *buf, uint16_t len);
};

/** @brief Latency client structure. */
struct bt_latency_client {
	/** Characteristic handle. */
	uint16_t handle;

	/** Latency parameter. */
	struct bt_gatt_write_params latency_params;

	/** Connection object. */
	struct bt_conn *conn;

	/** Internal state. */
	atomic_t state;
};

/** @brief Initialize the GATT latency client.
 *
 *  @param[in] latency Latency client instance.
 *  @param[in] cb Callbacks.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a negative error code is returned.
 *  @retval (-EINVAL) Special error code used when the input
 *          parameters are invalid.
 *  @retval (-EALREADY) Special error code used when the latency
 *          client has been initialed.
 */
int bt_latency_client_init(struct bt_latency_client *latency,
			   const struct bt_latency_client_cb *cb);

/** @brief Assign handles to the latency client instance.
 *
 *  This function should be called when a link with a peer has been established,
 *  to associate the link to this instance of the module. This makes it
 *  possible to handle several links and associate each link to a particular
 *  instance of this module. The GATT attribute handles are provided by the
 *  GATT Discovery Manager.
 *
 *  @param[in] dm Discovery object.
 *  @param[in,out] latency Latency client instance.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a negative error code is returned.
 *  @retval (-ENOTSUP) Special error code used when the UUID
 *          of the service does not match the expected UUID.
 *  @retval (-EINVAL) Special error code used when the UUID
 *          characteristic or value descriptor not found.
 */
int bt_latency_handles_assign(struct bt_gatt_dm *dm,
			      struct bt_latency_client *latency);

/** @brief Write data to the server.
 *
 *  @param[in] latency Latency client instance.
 *  @param[in] data Data.
 *  @param[in] len Data length.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a negative error code is returned.
 *  @retval (-EALREADY) Special error code used when the asynchronous
 *          request is waiting for a response.
 */
int bt_latency_request(struct bt_latency_client *latency,
		       const void *data, uint16_t len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_LATENCY_CLIENT_H_ */
