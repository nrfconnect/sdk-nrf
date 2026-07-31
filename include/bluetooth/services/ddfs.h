/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_DDFS_H_
#define BT_DDFS_H_

#include <stdlib.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

/**@file
 * @defgroup bt_ddfs Direction and Distance Finding Service API
 * @{
 * @brief API for the Direction and Distance Finding Service (DDFS).
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief UUID of the DDF Service. **/
#define BT_UUID_DDFS_VAL \
	BT_UUID_128_ENCODE(0x21490000, 0x494a, 0x4573, 0x98af, 0xf126af76f490)

/** @brief UUID of the Distance Measurement Characteristic. **/
#define BT_UUID_DDFS_DISTANCE_MEAS_VAL \
	BT_UUID_128_ENCODE(0x21490001, 0x494a, 0x4573, 0x98af, 0xf126af76f490)

/** @brief UUID of the Azimuth Measurement Characteristic. **/
#define BT_UUID_DDFS_AZIMUTH_MEAS_VAL \
	BT_UUID_128_ENCODE(0x21490002, 0x494a, 0x4573, 0x98af, 0xf126af76f490)

/** @brief UUID of the Elevation Measurement Characteristic. **/
#define BT_UUID_DDFS_ELEVATION_MEAS_VAL \
	BT_UUID_128_ENCODE(0x21490003, 0x494a, 0x4573, 0x98af, 0xf126af76f490)

/** @brief UUID of the Feature Characteristic. **/
#define BT_UUID_DDFS_FEATURE_VAL \
	BT_UUID_128_ENCODE(0x21490004, 0x494a, 0x4573, 0x98af, 0xf126af76f490)

/** @brief UUID of the Control Point Characteristic. **/
#define BT_UUID_DDFS_CTRL_POINT_VAL \
	BT_UUID_128_ENCODE(0x21490005, 0x494a, 0x4573, 0x98af, 0xf126af76f490)

#define BT_UUID_DDFS                 BT_UUID_DECLARE_128(BT_UUID_DDFS_VAL)
#define BT_UUID_DDFS_DISTANCE_MEAS   BT_UUID_DECLARE_128(BT_UUID_DDFS_DISTANCE_MEAS_VAL)
#define BT_UUID_DFFS_AZIMUTH_MEAS    BT_UUID_DECLARE_128(BT_UUID_DDFS_AZIMUTH_MEAS_VAL)
#define BT_UUID_DFFS_ELEVATION_MEAS  BT_UUID_DECLARE_128(BT_UUID_DDFS_ELEVATION_MEAS_VAL)
#define BT_UUID_DDFS_FEATURE         BT_UUID_DECLARE_128(BT_UUID_DDFS_FEATURE_VAL)
#define BT_UUID_DFFS_CTRL_POINT      BT_UUID_DECLARE_128(BT_UUID_DDFS_CTRL_POINT_VAL)

/** Ranging mode */
enum bt_ddfs_dm_ranging_mode {
	/** Round trip timing */
	BT_DDFS_DM_RANGING_MODE_RTT,

	/** Multi-carrier phase difference */
	BT_DDFS_DM_RANGING_MODE_MCPD,
};

/** Measurement quality definition. */
enum bt_ddfs_quality {
	/** Good measurement quality */
	BT_DDFS_QUALITY_OK,

	/** Poor measurement quality */
	BT_DDFS_QUALITY_POOR,

	/** Measurement not for use */
	BT_DDFS_QUALITY_DO_NOT_USE,

	/** Measurement quality not specified */
	BT_DDFS_QUALITY_NONE,
};
struct bt_ddfs_features {
	/** Distance Measurement RTT ranging mode supported bit. */
	uint8_t ranging_mode_rtt : 1;

	/** Distance Measurement MCPD ranging mode supported bit. */
	uint8_t ranging_mode_mcpd : 1;
};

/** @brief Distance Measurement configuration parameters. */
struct bt_ddfs_dm_config {
	/** Ranging mode */
	enum bt_ddfs_dm_ranging_mode mode;

	/** High-precision calculations enabled */
	bool high_precision;
};

/** @brief Pointers to the callback functions for service events. */
struct bt_ddfs_cb {
	/** @brief Indicates that the ranging mode for Distance Measurement is set.
	 *
	 * @param mode New ranging mode for distance measurement.
	 *
	 * @retval 0 If the operation was successful.
	 *         Otherwise, a negative error code is returned.
	 */
	int (*dm_ranging_mode_set)(uint8_t mode);

	/** @brief Indicates that the configuration for Distance Measurement is read.
	 *
	 * @param config Pointer to configuration object.
	 *
	 * @retval 0 If the operation was successful.
	 *         Otherwise, a negative error code is returned.
	 */
	int (*dm_config_read)(struct bt_ddfs_dm_config *config);

	/** @brief Azimuth Measurement state callback.
	 *
	 * Indicate the CCCD descriptor status of the Azimuth Measurement characteristic.
	 *
	 * @param enabled Azimuth Measurement notification status.
	 */
	void (*am_notification_config_changed)(bool enabled);

	/** @brief Distance Measurement state callback.
	 *
	 * Indicate the CCCD descriptor status of the Distance Measurement characteristic.
	 *
	 * @param enabled Distance Measurement notification status.
	 */
	void (*dm_notification_config_changed)(bool enabled);

	/** @brief Elevation Measurement state callback.
	 *
	 * Indicate the CCCD descriptor status of the Elevation Measurement characteristic.
	 *
	 * @param enabled Elevation Measurement notification status.
	 */
	void (*em_notification_config_changed)(bool enabled);
};

/** @brief Structure of distance measurement results. */
struct bt_ddfs_distance_measurement {
	/** Quality indicator */
	enum bt_ddfs_quality quality;

	/** Bluetooth LE Device Address */
	bt_addr_le_t bt_addr;

	/** Mode used for ranging */
	enum bt_ddfs_dm_ranging_mode ranging_mode;
	union {
		struct _mcpd {
			/** MCPD: Distance estimate based on IFFT of spectrum */
			uint16_t ifft;

			/** MCPD: Distance estimate based on average phase slope estimation */
			uint16_t phase_slope;

			/** RSSI: Distance estimate based on Friis path loss formula */
			uint16_t rssi_openspace;

			/** Best effort distance estimate */
			uint16_t best;

#ifdef CONFIG_DM_HIGH_PRECISION_CALC
			/* MCPD: Distance estimate based on advanced algorithms */
			uint16_t high_precision;
#endif
		} mcpd;
		struct _rtt {
			/** RTT: Distance estimate based on RTT measurement */
			uint16_t rtt;
		} rtt;
	} dist_estimates;
};

/** @brief Structure of azimuth measurement results. */
struct bt_ddfs_azimuth_measurement {
	/** Quality indicator */
	enum bt_ddfs_quality quality;

	/** Bluetooth LE Device Address */
	bt_addr_le_t bt_addr;

	/** Azimuth estimate */
	uint16_t value;
};

/** @brief Structure of elevation measurement results. */
struct bt_ddfs_elevation_measurement {
	/** Quality indicator */
	enum bt_ddfs_quality quality;

	/** Bluetooth LE Device Address */
	bt_addr_le_t bt_addr;

	/** Elevation estimate */
	int8_t value;
};

/** @brief Direction and Distance Finding Service initialization parameters. */
struct bt_ddfs_init_params {
	/** Initial value for features. */
	struct bt_ddfs_features dm_features;

	/** Callback functions structure.*/
	const struct bt_ddfs_cb *cb;
};

/** @brief Function for sending Distance Measurement value.
 *
 *  The application calls this function after having performed a Distance Measurement.
 *  If notification has been enabled, the measurement data is encoded and sent to the client.
 *
 *  @param[in] conn Pointer to connection object, or NULL if sent to all connected peers.
 *  @param[in] measurement Pointer to new distance measurement.
 *
 *  @retval 0 If the operation was successful.
 *          Otherwise, a negative error code is returned.
 */
int bt_ddfs_distance_measurement_notify(struct bt_conn *conn,
					const struct bt_ddfs_distance_measurement *measurement);

/** @brief Function for sending Azimuth Measurement value.
 *
 *  The application calls this function after having performed an Azimuth Measurement.
 *  If notification has been enabled, the measurement data is encoded and sent to the client.
 *
 *  @param[in] conn Pointer to connection object, or NULL if sent to all connected peers.
 *  @param[in] measurement Pointer to new azimuth measurement.
 *
 *  @retval 0 If the operation was successful.
 *          Otherwise, a negative error code is returned.
 */
int bt_ddfs_azimuth_measurement_notify(struct bt_conn *conn,
				       const struct bt_ddfs_azimuth_measurement *measurement);

/** @brief Function for sending Elevation Measurement value.
 *
 *  The application calls this function after having performed an Elevation Measurement.
 *  If notification has been enabled, the measurement data is encoded and sent to the client.
 *
 *  @param[in] conn Pointer to connection object, or NULL if sent to all connected peers.
 *  @param[in] measurement Pointer to new elevation measurement.
 *
 *  @retval 0 If the operation was successful.
 *          Otherwise, a negative error code is returned.
 */
int bt_ddfs_elevation_measurement_notify(struct bt_conn *conn,
					const struct bt_ddfs_elevation_measurement *measurement);

/** @brief Function for initializing the Direction and Distance Finding Service.
 *
 *  @param[in] init Initialization parameters.
 *
 *  @retval 0 If the operation was successful.
 *          Otherwise, a negative error code is returned.
 */
int bt_ddfs_init(const struct bt_ddfs_init_params *init);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_DDFS_H_ */
