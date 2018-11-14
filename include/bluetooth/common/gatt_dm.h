/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef BT_GATT_DM_H_
#define BT_GATT_DM_H_

/**
 * @file
 * @defgroup bt_gatt_dm GATT Discovery Manager API
 * @{
 * @brief Module for GATT Discovery Manager.
 */

#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Discovery callback structure.
 *
 *  This structure is used for tracking the result of a discovery.
 */
struct bt_gatt_dm_cb {
	/** @brief Discovery completed callback.
	 *
	 * The discovery procedure has completed successfully.
	 *
	 * @note You must release the discovery data with
	 * @ref bt_gatt_dm_data_release if you want to start another
	 * discovery.
	 *
	 * @param[in] conn Connection object.
	 * @param[in] attrs GATT attributes that belong to the discovered
	 *                  service.
	 * @param[in] attrs_len Length of the attribute array.
	 */
	void (*completed)(struct bt_conn *conn,
			  const struct bt_gatt_attr *attrs,
			  size_t attrs_len);

	/** @brief Service not found callback.
	 *
	 * The targeted service could not be found during the discovery.
	 *
	 * @param[in] conn Connection object.
	 */
	void (*service_not_found)(struct bt_conn *conn);

	/** @brief Discovery error found callback.
	 *
	 * The discovery procedure has failed.
	 *
	 * @param[in] conn Connection object.
	 */
	void (*error_found)(struct bt_conn *conn, int err);
};

/** @brief Start service discovery.
 *
 * This function is asynchronous. Discovery results are passed through
 * the supplied callback.
 *
 * @note Only one discovery procedure can be started simultaneously. To start
 * another one, wait for the result of the previous procedure to finish
 * and call @ref bt_gatt_dm_data_release if it was successful.
 *
 * @param[in] conn Connection object.
 * @param[in] svc_uuid UUID of target service.
 * @param[in] cb Callback structure.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_gatt_dm_start(struct bt_conn *conn,
		     const struct bt_uuid *svc_uuid,
		     const struct bt_gatt_dm_cb *cb);

/** @brief Release data associated with service discovery.
 *
 * After calling this function, you cannot rely on the discovery data that was
 * passed with the discovery completed callback (see @ref bt_gatt_dm_cb).
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_gatt_dm_data_release(void);

/** @brief Print service discovery data.
 *
 * This function prints GATT attributes that belong to the discovered service.
 */
void bt_gatt_dm_data_print(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_GATT_DM_H_ */
