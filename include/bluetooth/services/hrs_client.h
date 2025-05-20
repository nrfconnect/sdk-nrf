/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_HRS_CLIENT_H_
#define BT_HRS_CLIENT_H_

/**
 * @file
 * @defgroup bt_hrs_client Heart Rate Service Client
 * @{
 * @brief Heart Rate (HR) Service Client API.
 */

#include <zephyr/bluetooth/conn.h>
#include <bluetooth/gatt_dm.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Heart Rate Service error codes
 *
 * This service defines the following Attribute Protocol Application error codes.
 */
enum bt_hrs_client_error {
	/** Control Point value not supported. */
	BT_HRS_CLIENT_ERROR_CP_NOT_SUPPORTED = 0x80
};

/**@brief Heart Rate Control Point values
 *
 * The values of the Control Point characteristic.
 */
enum bt_hrs_client_cp_value {
	/** Resets the value if the Energy Expended field in
	 * the Heart Rate Measurement characteristic to 0.
	 */
	BT_HRS_CLIENT_CP_VALUE_RESET_EE = 0x01
};

/**@brief Body sensor location values.
 */
enum bt_hrs_client_sensor_location {
	/** Body Sensor Location: Other. */
	BT_HRS_CLIENT_SENSOR_LOCATION_OTHER,

	/** Body Sensor Location: Chest. */
	BT_HRS_CLIENT_SENSOR_LOCATION_CHEST,

	/** Body Sensor Location: Wrist. */
	BT_HRS_CLIENT_SENSOR_LOCATION_WRIST,

	/** Body Sensor Location: Finger. */
	BT_HRS_CLIENT_SENSOR_LOCATION_FINGER,

	/** Body Sensor Location: Hand. */
	BT_HRS_CLIENT_SENSOR_LOCATION_HAND,

	/** Body Sensor Location: Ear Lobe. */
	BT_HRS_CLIENT_SENSOR_LOCATION_EAR_LOBE,

	/** Body Sensor Location: Foot. */
	BT_HRS_CLIENT_SENSOR_LOCATION_FOOT
};

/**@brief Heart Measurement flags structure.
 */
struct bt_hrs_flags {
	/** Value Format flag. */
	uint8_t value_format : 1;

	/** Sensor Contact detected flag. */
	uint8_t sensor_contact_detected : 1;

	/** Sensor Contact supported flag. */
	uint8_t sensor_contact_supported : 1;

	/** Energy Expended preset flag. */
	uint8_t energy_expended_present : 1;

	/** RR-intervals present flag. */
	uint8_t rr_intervals_present : 1;
};

/**@brief Data structure of the Heart Rate Measurement characteristic.
 */
struct bt_hrs_client_measurement {
	/** Flags structure. */
	struct bt_hrs_flags flags;

	/** RR-intervals count. */
	uint8_t rr_intervals_count;

	/** RR-intervals represented by 1/1024 second as unit. Present if
	 * @ref bt_hrs_flags.rr_intervals_present is set. The interval with index 0 is older than
	 * the interval with index 1.
	 */
	uint16_t rr_intervals[CONFIG_BT_HRS_CLIENT_RR_INTERVALS_COUNT];

	/** Heart Rate Measurement Value in beats per minute unit. */
	uint16_t hr_value;

	/** Energy Expended in joule unit. Present if @ref bt_hrs_flags.energy_expended_present
	 * is set.
	 */
	uint16_t energy_expended;
};

/* Helper forward structure declaration representing Heart Rate Service Client instance.
 * Needed for callback declaration that are using instance structure as argument.
 */
struct bt_hrs_client;

/**@brief Heart Rate Measurement notification callback.
 *
 * This function is called every time the client receives a notification
 * with Heart Rate Measurement data.
 *
 * @param[in] hrs_c Heart Rate Service Client instance.
 * @param[in] meas Heart Rate Measurement received data.
 * @param[in] err 0 if the notification is valid.
 *                Otherwise, contains a (negative) error code.
 */
typedef void (*bt_hrs_client_notify_cb)(struct bt_hrs_client *hrs_c,
					const struct bt_hrs_client_measurement *meas,
					int err);

/**@brief Heart Rate Body Sensor Location read callback.
 *
 * @param[in] hrs_c Heart Rate Service Client instance.
 * @param[in] location Body Sensor Location value.
 * @param[in] err 0 if read operation succeeded and data is valid.
 *                Otherwise, contains an error code.
 */
typedef void (*bt_hrs_client_read_sensor_location_cb)(struct bt_hrs_client *hrs_c,
						      enum bt_hrs_client_sensor_location location,
						      int err);

/**@brief Heart Rate Control Point write callback.
 *
 * @param[in] hrs_c Heart Rate client instance.
 * @param[in] err 0 if write operation succeeded.
 *                Otherwise, contains an error code.
 */
typedef void (*bt_hrs_client_write_cb)(struct bt_hrs_client *hrs_c,
				       uint8_t err);

/**@brief Heart Rate Measurement characteristic structure.
 */
struct bt_hrs_client_hr_meas {
	/** Value handle. */
	uint16_t handle;

	/** Handle of the characteristic CCC descriptor. */
	uint16_t ccc_handle;

	/** GATT subscribe parameters for notification. */
	struct bt_gatt_subscribe_params notify_params;

	/** Notification callback. */
	bt_hrs_client_notify_cb notify_cb;
};

/**@brief Body Sensor Location characteristic structure.
 */
struct bt_hrs_client_body_sensor_location {
	/** Value handle. */
	uint16_t handle;

	/** Read parameters. */
	struct bt_gatt_read_params read_params;

	/** Read complete callback. */
	bt_hrs_client_read_sensor_location_cb read_cb;
};

/**@brief Heart Rate Control Point characteristic structure.
 */
struct bt_hrs_client_control_point {
	/** Value handle. */
	uint16_t handle;

	/** Write parameters. */
	struct bt_gatt_write_params write_params;

	/** Write complete callback. */
	bt_hrs_client_write_cb write_cb;
};

/**@brief Heart Rate Service Client instance structure.
 *        This structure contains status information for the client.
 */
struct bt_hrs_client {
	/** Connection object. */
	struct bt_conn *conn;

	/** Heart Rate Measurement characteristic. */
	struct bt_hrs_client_hr_meas measurement_char;

	/** Sensor Body Location characteristic. */
	struct bt_hrs_client_body_sensor_location sensor_location_char;

	/** Heart Rate Control Point characteristic. */
	struct bt_hrs_client_control_point cp_char;

	/** Internal state. */
	atomic_t state;
};

/**@brief Function for initializing the Heart Rate Service Client.
 *
 * @param[in, out] hrs_c Heart Rate Service Client instance. This structure must be
 *                       supplied by the application. It is initialized by
 *                       this function and will later be used to identify
 *                       this particular client instance.
 *
 * @retval 0 If the client was initialized successfully.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_hrs_client_init(struct bt_hrs_client *hrs_c);

/**@brief Subscribe to Heart Rate Measurement notification.
 *
 * This function writes CCC descriptor of the Heart Rate Measurement characteristic
 * to enable notification.
 *
 * @param[in] hrs_c Heart Rate Service Client instance.
 * @param[in] notify_cb   Notification callback.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_hrs_client_measurement_subscribe(struct bt_hrs_client *hrs_c,
					bt_hrs_client_notify_cb notify_cb);

/**@brief Remove subscription to the Heart Rate Measurement notification.
 *
 * This function writes CCC descriptor of the Heart Rate Measurement characteristic
 * to disable notification.
 *
 * @param[in] hrs_c Heart Rate Service Client instance.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_hrs_client_measurement_unsubscribe(struct bt_hrs_client *hrs_c);

/**@brief Read Body Sensor Location characteristic.
 *
 * @param[in] hrs_c   Heart Rate Service Client instance.
 * @param[in] read_cb Read complete callback.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_hrs_client_sensor_location_read(struct bt_hrs_client *hrs_c,
				       bt_hrs_client_read_sensor_location_cb read_cb);

/**@brief Check if Heart Rate Service has the Body Sensor Location characteristic.
 *
 * This function can be called after @ref bt_hrs_client_handles_assign to check
 * if the Body Sensor Location characteristic was found during service discovery.
 *
 * @retval true  If the Body Sensor Location is found.
 * @retval false If the Body Sensor Location is not found.
 */
bool bt_hrs_client_has_sensor_location(struct bt_hrs_client *hrs_c);

/**@brief Write Heart Rate Control Point characteristic.
 *
 * @param[in] hrs_c    Heart Rate Service Client instance.
 * @param[in] value    Control Point value to write.
 * @param[in] write_cb Write complete callback.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_hrs_client_control_point_write(struct bt_hrs_client *hrs_c,
				      enum bt_hrs_client_cp_value value,
				      bt_hrs_client_write_cb write_cb);

/**@brief Check if Heart Rate Service has the Heart Rate Control Point characteristic.
 *
 * This function can be called after @ref bt_hrs_client_handles_assign to check
 * if the Heart Rate Control Point characteristic was found during service discovery.
 *
 * @retval true  If the Heart Rate Control Point is found.
 * @retval false If the Heart Rate Control Point is not found.
 */
bool bt_hrs_client_has_control_point(struct bt_hrs_client *hrs_c);

/**@brief Function for assigning handles to Heart Rate Service Client instance.
 *
 * @details Call this function when a link has been established with a peer to
 *          associate the link to this instance of the module. This makes it
 *          possible to handle several links and associate each link to a particular
 *          instance of this module.
 *
 * @param[in]     dm     Discovery object.
 * @param[in,out] hrs_c  Heart Rate Service Client instance for associating the link.
 *
 * @retval 0 If the operation is successful.
 *           Otherwise, a (negative) error code is returned.
 */
int bt_hrs_client_handles_assign(struct bt_gatt_dm *dm, struct bt_hrs_client *hrs_c);

#ifdef __cplusplus
}
#endif

/**
 *@}
 */

#endif /* BT_HRS_CLIENT_H_ */
