/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef GATT_DB_DISCOVERY_H__
#define GATT_DB_DISCOVERY_H__

/**
 * @file
 * @defgroup nrf_bt_gatt_db_discovery GATT Database Discovery API
 * @{
 * @brief Module for GATT Database Discovery.
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
struct gatt_db_discovery_cb {
	/** @brief Discovery completed callback.
	 *
	 * The discovery procedure has completed successfully.
	 *
	 * @note You need to release the discovery data with
	 * @ref gatt_db_discovery_data_release if you want to start another
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
 * This function is asynchronous and discovery results are passed through
 * supplied callback.
 *
 * @note Only one discovery procedure can be started simultaneously. To start
 * another one, you need to wait for the result of previous procedure to finish
 * and call @ref gatt_db_discovery_data_release if it was successful.
 *
 * @param[in] conn Connection object.
 * @param[in] svc_uuid UUID of target service.
 * @param[in] cb Callback structure.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error
 *         code is returned.
 */
int gatt_db_discovery_start(struct bt_conn *conn,
			    const struct bt_uuid *svc_uuid,
			    const struct gatt_db_discovery_cb *cb);

/** @brief Release data associated with service discovery.
 *
 * After calling this function, you cannot rely on the discovery data passed
 * with discovery completed callback (see @ref gatt_db_discovery_cb).
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error
 *         code is returned.
 */
int gatt_db_discovery_data_release(void);

/** @brief Print service discovery data.
 *
 * This function prints GATT attibutes that belong to the discovered service.
 */
void gatt_db_discovery_data_print(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* GATT_DB_DISCOVERY_H__ */
