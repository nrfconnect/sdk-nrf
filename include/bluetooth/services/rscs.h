/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_RSCS_H_
#define BT_RSCS_H_

#include <stdlib.h>
#include <zephyr/types.h>

/**@file
 * @defgroup bt_rscs Running Speed and Cadence Service API
 * @{
 * @brief API for the Running Speed and Cadence Service (RSCS).
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief RSC Sensor Locations. */
enum bt_rscs_location {
	RSC_LOC_OTHER,
	RSC_LOC_TOP_OF_SHOE,
	RSC_LOC_IN_SHOE,
	RSC_LOC_HIP,
	RSC_LOC_FRONT_WHEEL,
	RSC_LOC_LEFT_CRANK,
	RSC_LOC_RIGHT_CRANK,
	RSC_LOC_LEFT_PEDAL,
	RSC_LOC_RIGHT_PEDAL,
	RSC_LOC_FRONT_HUB,
	RSC_LOC_REAR_DROPOUT,
	RSC_LOC_CHAINSTAY,
	RSC_LOC_REAR_WHEEL,
	RSC_LOC_REAR_HUB,
	RSC_LOC_CHEST,
	RSC_LOC_SPIDER,
	RSC_LOC_CHAIN_RING,

	RSC_LOC_AMT,
};

/** @brief Bitmask set of supported location. */
struct bt_rsc_supported_loc {
	uint32_t other : 1;
	uint32_t top_of_shoe : 1;
	uint32_t in_shoe : 1;
	uint32_t hip : 1;
	uint32_t front_wheel : 1;
	uint32_t left_crank : 1;
	uint32_t right_crank : 1;
	uint32_t left_pedal : 1;
	uint32_t right_pedal : 1;
	uint32_t front_hub : 1;
	uint32_t rear_dropout : 1;
	uint32_t chainstay : 1;
	uint32_t rear_wheel : 1;
	uint32_t rear_hub : 1;
	uint32_t chest : 1;
	uint32_t spider : 1;
	uint32_t chain_ring : 1;
};

/** @brief Bitmask set of supported features. */
struct bt_rscs_features {
	/** Instantaneous Stride Length Measurement Supported bit. */
	uint8_t inst_stride_len : 1;

	/** Total Distance Measurement Supported bit. */
	uint8_t total_distance : 1;

	/** Walking or Running Status Supported bit. */
	uint8_t walking_or_running : 1;

	/** Calibration Procedure Supported bit. */
	uint8_t sensor_calib_proc : 1;

	/** Multiple Sensor Locations Supported bit. */
	uint8_t multi_sensor_loc : 1;
};

/** Running Speed and Cadence Service events. */
enum bt_rscs_evt {
	/** Measurement notification enable. */
	RSCS_EVT_MEAS_NOTIFY_ENABLE,

	/** Measurement notification disable. */
	RSCS_EVT_MEAS_NOTIFY_DISABLE,
};

/**@brief Running Speed and Cadence Service event handler type. */
typedef void (*bt_rscs_evt_handler) (enum bt_rscs_evt evt);

/** @brief SC Control Point callback structure.*/
struct bt_rscs_cp_cb {
	/** @brief Set Cumulative Value Procedure.
	 *
	 * @param total_distance New value for total distance.
	 *
	 * @retval 0 If the operation was successful.
	 *         Otherwise, a negative error code is returned.
	 */
	int (*set_cumulative)(uint32_t total_distance);

	/** @brief Start Sensor Calibration Procedure.
	 *
	 * @retval 0 If the operation was successful.
	 *         Otherwise, a negative error code is returned.
	 */
	int (*calibration)(void);

	/** @brief Update Sensor Location Procedure.
	 *
	 * @param location New sensor localization value.
	 */
	void (*update_loc)(uint8_t location);
};

/** @brief Running Speed and Cadence Service initialization parameters. */
struct bt_rscs_init_params {
	/** Initial value for features of sensor. */
	struct bt_rscs_features features;

	/** List of supported locations */
	struct bt_rsc_supported_loc supported_locations;

	/** Initial value for sensor location. */
	enum bt_rscs_location location;

	/** Event handler to be called for handling events in the RSC Service. */
	bt_rscs_evt_handler evt_handler;

	/** SC Control Point callback structure.*/
	const struct bt_rscs_cp_cb *cp_cb;
};

/** @brief Running Speed and Cadence Service measurement structure. */
struct bt_rscs_measurement {
	/** True if Instantaneous Stride Length is present in the measurement. */
	bool is_inst_stride_len;

	/** True if Total Distance is present in the measurement. */
	bool is_total_distance;

	/** True if running, False if walking. */
	bool is_running;

	/** Instantaneous Speed. 256 units = 1 meter/second */
	uint16_t inst_speed;

	/** Instantaneous Cadence. 1 unit = 1 stride/minute */
	uint8_t inst_cadence;

	/** Instantaneous Stride Length. 100 units = 1 meter */
	uint16_t inst_stride_length;

	/** Total Distance. 1 unit = 1 meter */
	uint32_t total_distance;
};

/** @brief Function for sending Running Speed and Cadence measurement.
 *
 * The application calls this function after having performed a Running Speed and Cadence
 * measurement. If notification has been enabled, the measurement data is encoded
 * and sent to the client.
 *
 * @param[in] conn Pointer to connection object, or NULL if sent to all connected peers.
 * @param[in] measurement Pointer to new running speed and cadence measurement.
 *
 * @retval 0 If the operation was successful.
 *          Otherwise, a negative error code is returned.
 */
int bt_rscs_measurement_send(struct bt_conn *conn, const struct bt_rscs_measurement *measurement);

/** @brief Function for initializing the Running Speed and Cadence Service.
 *
 *  @param[in] init Initialization parameters.
 *
 *  @retval 0 If the operation was successful.
 *          Otherwise, a negative error code is returned.
 */
int bt_rscs_init(const struct bt_rscs_init_params *init);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_RSCS_H_ */
