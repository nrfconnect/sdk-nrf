/*
 * Copyright (c) 2018 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef __BAS_C_H
#define __BAS_C_H

/**
 * @file
 * @defgroup bt_bas_client_api Battery Service Client API
 * @{
 * @brief API for the Bluetooth LE GATT Battery Service (BAS) Client.
 */

#include <zephyr/kernel.h>
#include <sys/types.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/gatt_dm.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Value that shows that the battery level is invalid.
 *
 * This value is stored in the BAS Client object when the battery level
 * information is unknown.
 *
 * The same value is used to mark the fact that a notification has been
 * aborted (see @ref bt_bas_notify_cb).
 */
#define BT_BAS_VAL_INVALID (255)

/**
 * @brief Maximum allowed value for battery level.
 *
 * Any value above that limit is treated as invalid.
 */
#define BT_BAS_VAL_MAX (100)

struct bt_bas_client;

/**
 * @brief Value notification callback.
 *
 * This function is called every time the server sends a notification
 * for a changed value.
 *
 * @param bas           BAS Client object.
 * @param battery_level The notified battery level value, or
 *                      @ref BT_BAS_VAL_INVALID if the notification
 *                      was interrupted by the server
 *                      (NULL received from the stack).
 */
typedef void (*bt_bas_notify_cb)(struct bt_bas_client *bas,
				 uint8_t battery_level);

/**
 * @brief Read complete callback.
 *
 * This function is called when the read operation finishes.
 *
 * @param bas           BAS Client object.
 * @param battery_level The battery level value that was read.
 * @param err           ATT error code or 0.
 */
typedef void (*bt_bas_read_cb)(struct bt_bas_client *bas,
			       uint8_t battery_level,
			       int err);

/* @brief Battery Service Client characteristic periodic read. */
struct bt_bas_periodic_read {
	/** Work queue used to measure the read interval. */
	struct k_work_delayable read_work;
	/** Read parameters. */
	struct bt_gatt_read_params params;
	/** Read value interval. */
	atomic_t interval;
};

/** @brief Battery Service Client instance. */
struct bt_bas_client {
	/** Connection handle. */
	struct bt_conn *conn;
	/** Notification parameters. */
	struct bt_gatt_subscribe_params notify_params;
	/** Read parameters. */
	struct bt_gatt_read_params read_params;
	/** Read characteristic value timing. Used when characteristic do not
	 *  have a CCCD descriptor.
	 */
	struct bt_bas_periodic_read periodic_read;
	/** Notification callback. */
	bt_bas_notify_cb notify_cb;
	/** Read value callback. */
	bt_bas_read_cb read_cb;
	/** Handle of the Battery Level Characteristic. */
	uint16_t val_handle;
	/** Handle of the CCCD of the Battery Level Characteristic. */
	uint16_t ccc_handle;
	/** Current battery value. */
	uint8_t battery_level;
	/** Properties of the service. */
	uint8_t properties;
	/** Notification supported. */
	bool notify;
};

/**
 * @brief Initialize the BAS Client instance.
 *
 * You must call this function on the BAS Client object before
 * any other function.
 *
 * @param bas  BAS Client object.
 */
void bt_bas_client_init(struct bt_bas_client *bas);

/**
 * @brief Assign handles to the BAS Client instance.
 *
 * This function should be called when a connection with a peer has been
 * established, to associate the connection to this instance of the module.
 * This makes it possible to handle multiple connections and associate each
 * connection to a particular instance of this module.
 * The GATT attribute handles are provided by the GATT Discovery Manager.
 *
 * @param dm    Discovery object.
 * @param bas BAS Client object.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval (-ENOTSUP) Special error code used when the UUID
 *         of the service does not match the expected UUID.
 */
int bt_bas_handles_assign(struct bt_gatt_dm *dm,
			  struct bt_bas_client *bas);

/**
 * @brief Subscribe to the battery level value change notification.
 *
 * @param bas BAS Client object.
 * @param func  Callback function handler.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval -ENOTSUP Special error code used if the connected server
 *         does not support notifications.
 */
int bt_bas_subscribe_battery_level(struct bt_bas_client *bas,
				   bt_bas_notify_cb func);

/**
 * @brief Remove the subscription.
 *
 * @param bas BAS Client object.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_bas_unsubscribe_battery_level(struct bt_bas_client *bas);

/**
 * @brief Get the connection object that is used with a given BAS Client.
 *
 * @param bas BAS Client object.
 *
 * @return Connection object.
 */
struct bt_conn *bt_bas_conn(const struct bt_bas_client *bas);


/**
 * @brief Read the battery level value from the device.
 *
 * This function sends a read request to the connected device.
 *
 * @param bas BAS Client object.
 * @param func  The callback function.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_bas_read_battery_level(struct bt_bas_client *bas, bt_bas_read_cb func);

/**
 * @brief Get the last known battery level.
 *
 * This function returns the last known battery level value.
 * The battery level is stored when a notification or read response is
 * received.
 *
 * @param bas BAS Client object.
 *
 * @return Battery level or negative error code.
 */
int bt_bas_get_last_battery_level(struct bt_bas_client *bas);

/**
 * @brief Check whether notification is supported by the service.
 *
 * @param bas BAS Client object.
 *
 * @retval true If notifications are supported.
 *              Otherwise, @c false is returned.
 */
static inline bool bt_bas_notify_supported(struct bt_bas_client *bas)
{
	return bas->notify;
}

/**
 * @brief Periodically read the battery level value from the device with
 *        specific time interval.
 *
 * @param bas BAS Client object.
 * @param interval Characteristic Read interval in milliseconds.
 * @param func The callback function.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_bas_start_per_read_battery_level(struct bt_bas_client *bas,
					int32_t interval,
					bt_bas_notify_cb func);

/**
 * @brief Stop periodic reading of the battery value from the device.
 *
 * @param bas BAS Client object.
 */
void bt_bas_stop_per_read_battery_level(struct bt_bas_client *bas);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */


#endif /* __BAS_C_H  */
