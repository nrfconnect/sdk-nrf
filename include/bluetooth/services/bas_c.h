/*
 * Copyright (c) 2018 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef __BAS_C_H
#define __BAS_C_H

/**
 * @file
 * @defgroup bt_gatt_bas_c_api Battery Service Client API
 * @{
 * @brief API for the BLE GATT Battery Service (BAS) Client.
 */

#include <kernel.h>
#include <sys/types.h>
#include <bluetooth/gatt.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt_dm.h>


/**
 * @brief Value that shows that the battery level is invalid.
 *
 * This value is stored in the BAS Client object when the battery level
 * information is unknown.
 *
 * The same value is used to mark the fact that a notification has been
 * aborted (see @ref bt_gatt_bas_c_notify_cb).
 */
#define BT_GATT_BAS_VAL_INVALID (255)

/**
 * @brief Maximum allowed value for battery level.
 *
 * Any value above that limit is treated as invalid.
 */
#define BT_GATT_BAS_VAL_MAX (100)


struct bt_gatt_bas_c;

/**
 * @brief Value notification callback.
 *
 * This function is called every time the server sends a notification
 * for a changed value.
 *
 * @param bas_c         BAS Client object.
 * @param battery_level The notified battery level value, or
 *                      @ref BT_GATT_BAS_VAL_INVALID if the notification
 *                      was interrupted by the server
 *                      (NULL received from the stack).
 */
typedef void (*bt_gatt_bas_c_notify_cb)(struct bt_gatt_bas_c *bas_c,
					u8_t battery_level);

/**
 * @brief Read complete callback.
 *
 * This function is called when the read operation finishes.
 *
 * @param bas_c         BAS Client object.
 * @param battery_level The battery level value that was read.
 * @param err           ATT error code or 0.
 */
typedef void (*bt_gatt_bas_c_read_cb)(struct bt_gatt_bas_c *bas_c,
				      u8_t battery_level,
				      int err);

/**
 * @brief Battery Service Client characteristic periodic read.
 */
struct bt_gatt_bas_c_periodic_read {
	/** Work queue used to measure the read interval. */
	struct k_delayed_work read_work;
	/** Read parameters. */
	struct bt_gatt_read_params params;
	/** Read value interval. */
	atomic_t interval;
	/** Active processing flag. */
	atomic_t process;
};

/**
 * @brief Battery Service Client instance.
 */
struct bt_gatt_bas_c {
	/** Connection handle. */
	struct bt_conn *conn;
	/** Notification parameters. */
	struct bt_gatt_subscribe_params notify_params;
	/** Read parameters. */
	struct bt_gatt_read_params read_params;
	/** Read characteristic value timing. Used when characteristic do not
	 *  have a CCCD descriptor.
	 */
	struct bt_gatt_bas_c_periodic_read periodic_read;
	/** Notification callback. */
	bt_gatt_bas_c_notify_cb notify_cb;
	/** Read value callback. */
	bt_gatt_bas_c_read_cb read_cb;
	/** Handle of the Battery Level Characteristic. */
	u16_t val_handle;
	/** Handle of the CCCD of the Battery Level Characteristic. */
	u16_t ccc_handle;
	/** Current battery value. */
	u8_t battery_level;
	/** Properties of the service. */
	u8_t properties;
	/** Notification supported. */
	bool notify;
};

/**
 * @brief Initialize the BAS Client instance.
 *
 * You must call this function on the BAS Client object before
 * any other function.
 *
 * @param bas_c  BAS Client object.
 */
void bt_gatt_bas_c_init(struct bt_gatt_bas_c *bas_c);

/**
 * @brief Assign handles to the BAS Client instance.
 *
 * This function should be called when a link with a peer has been established,
 * to associate the link to this instance of the module. This makes it
 * possible to handle several links and associate each link to a particular
 * instance of this module. The GATT attribute handles are provided by the
 * GATT Discovery Manager.
 *
 * @note
 * This function starts the report discovery process.
 * Wait for one of the functions in @ref bt_gatt_hids_c_init_params
 * before using the BAS Client object.
 *
 * @param dm    Discovery object.
 * @param bas_c BAS Client object.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval (-ENOTSUP) Special error code used when the UUID
 *         of the service does not match the expected UUID.
 */
int bt_gatt_bas_c_handles_assign(struct bt_gatt_dm *dm,
				 struct bt_gatt_bas_c *bas_c);

/**
 * @brief Subscribe to the battery level value change notification.
 *
 * @param bas_c BAS Client object.
 * @param func  Callback function handler.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 * @retval -ENOTSUP Special error code used if the connected server
 *         does not support notifications.
 */
int bt_gatt_bas_c_subscribe(struct bt_gatt_bas_c *bas_c,
			    bt_gatt_bas_c_notify_cb func);

/**
 * @brief Remove the subscription.
 *
 * @param bas_c BAS Client object.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_gatt_bas_c_unsubscribe(struct bt_gatt_bas_c *bas_c);

/**
 * @brief Get the connection object that is used with a given BAS Client.
 *
 * @param bas_c BAS Client object.
 *
 * @return Connection object.
 */
struct bt_conn *bt_gatt_bas_c_conn(const struct bt_gatt_bas_c *bas_c);


/**
 * @brief Read the battery level value from the device.
 *
 * This function sends a read request to the connected device.
 *
 * @param bas_c BAS Client object.
 * @param func  The callback function.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_gatt_bas_c_read(struct bt_gatt_bas_c *bas_c, bt_gatt_bas_c_read_cb func);

/**
 * @brief Get the last known battery level.
 *
 * This function returns the last known battery level value.
 * The battery level is stored when a notification or read response is
 * received.
 *
 * @param bas_c BAS Client object.
 *
 * @return Battery level or negative error code.
 */
int bt_gatt_bas_c_get(struct bt_gatt_bas_c *bas_c);

/**
 * @brief Check whether notification is supported by the service.
 *
 * @param bas_c BAS Client object.
 *
 * @retval true If notifications are supported.
 *              Otherwise, @c false is returned.
 */
static inline bool bt_gatt_bas_c_notify_supported(struct bt_gatt_bas_c *bas_c)
{
	return bas_c->notify;
}

/**
 * @brief Periodically read the battery level value from the device with
 *        specific time interval.
 *
 * @param bas_c BAS Client object.
 * @param interval Characteristic Read interval in milliseconds.
 * @param func The callback function.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_gatt_bas_c_periodic_read_start(struct bt_gatt_bas_c *bas_c,
				      s32_t interval,
				      bt_gatt_bas_c_notify_cb func);

/**
 * @brief Stop periodic reading of the battery value from the device.
 *
 * @param bas_c BAS Client object.
 */
void bt_gatt_bas_c_periodic_read_stop(struct bt_gatt_bas_c *bas_c);

/**
 * @}
 */


#endif /* __BAS_C_H  */
