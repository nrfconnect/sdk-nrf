/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_NUS_CLIENT_H_
#define BT_NUS_CLIENT_H_

/**
 * @file
 * @defgroup bt_nus_client Bluetooth LE GATT NUS Client API
 * @{
 * @brief API for the Bluetooth LE GATT Nordic UART Service (NUS) Client.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/gatt_dm.h>

/** @brief Handles on the connected peer device that are needed to interact with
 * the device.
 */
struct bt_nus_client_handles {

        /** Handle of the NUS RX characteristic, as provided by
	 *  a discovery.
         */
	uint16_t rx;

        /** Handle of the NUS TX characteristic, as provided by
	 *  a discovery.
         */
	uint16_t tx;

        /** Handle of the CCC descriptor of the NUS TX characteristic,
	 *  as provided by a discovery.
         */
	uint16_t tx_ccc;
};

struct bt_nus_client;

/** @brief NUS Client callback structure. */
struct bt_nus_client_cb {
	/** @brief Data received callback.
	 *
	 * The data has been received as a notification of the NUS TX
	 * Characteristic.
	 *
	 * @param[in] nus  NUS Client instance.
	 * @param[in] data Received data.
	 * @param[in] len Length of received data.
	 *
	 * @retval BT_GATT_ITER_CONTINUE To keep notifications enabled.
	 * @retval BT_GATT_ITER_STOP To disable notifications.
	 */
	uint8_t (*received)(struct bt_nus_client *nus, const uint8_t *data, uint16_t len);

	/** @brief Data sent callback.
	 *
	 * The data has been sent and written to the NUS RX Characteristic.
	 *
	 * @param[in] nus  NUS Client instance.
	 * @param[in] err ATT error code.
	 * @param[in] data Transmitted data.
	 * @param[in] len Length of transmitted data.
	 */
	void (*sent)(struct bt_nus_client *nus, uint8_t err, const uint8_t *data, uint16_t len);

	/** @brief TX notifications disabled callback.
	 *
	 * TX notifications have been disabled.
	 *
	 * @param[in] nus  NUS Client instance.
	 */
	void (*unsubscribed)(struct bt_nus_client *nus);
};

/** @brief NUS Client structure. */
struct bt_nus_client {

        /** Connection object. */
	struct bt_conn *conn;

        /** Internal state. */
	atomic_t state;

        /** Handles on the connected peer device that are needed
         * to interact with the device.
         */
	struct bt_nus_client_handles handles;

        /** GATT subscribe parameters for NUS TX Characteristic. */
	struct bt_gatt_subscribe_params tx_notif_params;

        /** GATT write parameters for NUS RX Characteristic. */
	struct bt_gatt_write_params rx_write_params;

        /** Application callbacks. */
	struct bt_nus_client_cb cb;
};

/** @brief NUS Client initialization structure. */
struct bt_nus_client_init_param {

        /** Callbacks provided by the user. */
	struct bt_nus_client_cb cb;
};

/** @brief Initialize the NUS Client module.
 *
 * This function initializes the NUS Client module with callbacks provided by
 * the user.
 *
 * @param[in,out] nus    NUS Client instance.
 * @param[in] init_param NUS Client initialization parameters.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_nus_client_init(struct bt_nus_client *nus,
		       const struct bt_nus_client_init_param *init_param);

/** @brief Send data to the server.
 *
 * This function writes to the RX Characteristic of the server.
 *
 * @note This procedure is asynchronous. Therefore, the data to be sent must
 * remain valid while the function is active.
 *
 * @param[in,out] nus NUS Client instance.
 * @param[in] data Data to be transmitted.
 * @param[in] len Length of data.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_nus_client_send(struct bt_nus_client *nus, const uint8_t *data,
		       uint16_t len);

/** @brief Assign handles to the NUS Client instance.
 *
 * This function should be called when a link with a peer has been established
 * to associate the link to this instance of the module. This makes it
 * possible to handle several links and associate each link to a particular
 * instance of this module. The GATT attribute handles are provided by the
 * GATT DB discovery module.
 *
 * @param[in] dm Discovery object.
 * @param[in,out] nus NUS Client instance.
 *
 * @retval 0 If the operation was successful.
 * @retval (-ENOTSUP) Special error code used when UUID
 *         of the service does not match the expected UUID.
 * @retval Otherwise, a negative error code is returned.
 */
int bt_nus_handles_assign(struct bt_gatt_dm *dm,
			  struct bt_nus_client *nus);

/** @brief Request the peer to start sending notifications for the TX
 *	   Characteristic.
 *
 * This function enables notifications for the NUS TX Characteristic at the peer
 * by writing to the CCC descriptor of the NUS TX Characteristic.
 *
 * @param[in,out] nus NUS Client instance.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a negative error code is returned.
 */
int bt_nus_subscribe_receive(struct bt_nus_client *nus);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_NUS_CLIENT_H_ */
