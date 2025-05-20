/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_GATTP_H_
#define BT_GATTP_H_

/** @file
 *
 * @defgroup bt_gattp Bluetooth LE Generic Attribute Profile API
 * @{
 *
 * @brief API for the Bluetooth LE Generic Attribute (GATT) Profile.
 */

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/gatt_dm.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Structure for Service Changed handle range. */
struct bt_gattp_handle_range {
	/** Start handle. */
	uint16_t start_handle;

	/** End handle. */
	uint16_t end_handle;
};

struct bt_gattp;

/**@brief Service Changed Indication callback function.
 *
 * @param[in] gattp GATT profile instance.
 * @param[in] handle_range Affected handle range.
 * @param[in] err 0 if the indication is valid.
 *                Otherwise, contains a (negative) error code.
 */
typedef void (*bt_gattp_indicate_cb)(
				struct bt_gattp *gattp,
				const struct bt_gattp_handle_range *handle_range,
				int err);

/**@brief GATT profile instance. */
struct bt_gattp {
	/** Connection object. */
	struct bt_conn *conn;

	/** Handle of the Service Changed Characteristic. */
	uint16_t handle_sc;

	/** Handle of the CCCD of the Service Changed Characteristic. */
	uint16_t handle_sc_ccc;

	/** GATT subscribe parameters for the Service Changed Characteristic. */
	struct bt_gatt_subscribe_params indicate_params;

	/** Callback function for the Service Changed indication. */
	bt_gattp_indicate_cb indicate_cb;

	/** Internal state. */
	atomic_t state;
};

/**@brief Initialize the GATT profile instance.
 *
 * @param[in,out] gattp GATT profile instance.
 *
 * @retval 0 If the instance is initialized successfully.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_gattp_init(struct bt_gattp *gattp);

/**@brief Assign handles to the GATT profile instance.
 *
 * @details Call this function when a link has been established with a peer to
 *          associate the link to this instance of the module. This makes it
 *          possible to handle several links and associate each link to a particular
 *          instance of this module.
 *
 * @param[in] dm Discovery object.
 * @param[in,out] gattp GATT profile instance.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_gattp_handles_assign(struct bt_gatt_dm *dm,
			    struct bt_gattp *gattp);

/**@brief Subscribe to the Service Changed indication.
 *
 * @param[in] gattp GATT profile instance.
 * @param[in] func Indication callback function handler.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_gattp_subscribe_service_changed(struct bt_gattp *gattp,
				       bt_gattp_indicate_cb func);

/**@brief Unsubscribe from the Service Changed indication.
 *
 * @param[in] gattp GATT profile instance.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_gattp_unsubscribe_service_changed(struct bt_gattp *gattp);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_GATTP_H_ */
