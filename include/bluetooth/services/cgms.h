/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_CGMS_H_
#define BT_CGMS_H_

#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>
#include <sfloat.h>

/**@file
 * @defgroup bt_cgms Continuous Glucose Monitoring Service API
 * @{
 * @brief API for the Continuous Glucose Monitoring Service (CGMS).
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief CGMS Feature characteristic Type field. */
enum bt_cgms_feat_type {
	/** Capillary Whole blood */
	BT_CGMS_FEAT_TYPE_CAP_BLOOD = 0x01,
	/** Capillary Plasma */
	BT_CGMS_FEAT_TYPE_CAP_PLASMA = 0x02,
	/** Venous Whole blood */
	BT_CGMS_FEAT_TYPE_VEN_BLOOD = 0x03,
	/** Venous Plasma */
	BT_CGMS_FEAT_TYPE_VEN_PLASMA = 0x04,
	/** Arterial Whole blood */
	BT_CGMS_FEAT_TYPE_ART_BLOOD = 0x05,
	/** Arterial Plasma */
	BT_CGMS_FEAT_TYPE_ART_PLASMA = 0x06,
	/** Arterial Plasma */
	BT_CGMS_FEAT_TYPE_UNDET_BLOOD = 0x07,
	/** Arterial Plasma */
	BT_CGMS_FEAT_TYPE_UNDET_PLASMA = 0x08,
	/** Interstitial Fluid (ISF) */
	BT_CGMS_FEAT_TYPE_FLUID = 0x09,
	/** Control Solution */
	BT_CGMS_FEAT_TYPE_CONTROL = 0x0A,
};

/** @brief CGMS Feature characteristic Sample Location field. */
enum bt_cgms_feat_loc {
	/** Finger */
	BT_CGMS_FEAT_LOC_FINGER = 0x01,
	/** Alternate Site Test (AST) */
	BT_CGMS_FEAT_LOC_AST = 0x02,
	/** Earlobe */
	BT_CGMS_FEAT_LOC_EAR = 0x03,
	/** Control solution */
	BT_CGMS_FEAT_LOC_CONTROL = 0x04,
	/** Subcutaneous tissue */
	BT_CGMS_FEAT_LOC_SUB_TISSUE = 0x05,
	/** Sample Location value not available */
	BT_CGMS_FEAT_LOC_NOT_AVAIL = 0x0F,
};

/** @brief Continuous Glucose Monitoring service measurement record structure. */
struct bt_cgms_measurement {
	/** The glucose concentration measurement in milligram per deciliter (mg/dL).
	 *  It is in SFLOAT type defined in the IEEE 11073-20601 specification.
	 */
	struct sfloat glucose;
};

/** @brief Continuous Glucose Monitoring service callback structure. */
struct bt_cgms_cb {
	/** @brief Callback when the state of session changes.
	 *
	 *  The application may start or stop glucose measurement based on
	 *  the latest state of a session.
	 *
	 *  @param session_state The state of current session. True if session starts,
	 *         False if session stops.
	 */
	void (*session_state_changed)(const bool session_state);
};

/** @brief Continuous Glucose Monitoring service initialization structure. */
struct bt_cgms_init_param {
	/** Continuous Glucose Monitoring type. */
	enum bt_cgms_feat_type type;

	/** CGMS sample location. */
	enum bt_cgms_feat_loc sample_location;

	/** The session run time in uint of hour. It shall be positive. */
	uint16_t session_run_time;

	/** Initial communication interval of notification. It shall be positive. */
	uint16_t initial_comm_interval;

	/** CGMS callback structure. */
	struct bt_cgms_cb *cb;
};

/** @brief Submit glucose concentration measurement to CGM service.
 *
 * This will add the new glucose measurement into the database.
 *
 *  @param[in] measurement The measurement result.
 *
 *  @return Zero in case of success and error code in case of error.
 */
int bt_cgms_measurement_add(struct bt_cgms_measurement measurement);

/** @brief Initialize Continuous Glucose Monitoring service.
 *
 * This will initialize components used in CGMS.
 *
 *  @param[in] init_params The parameter used to initialize the corresponding
 *             values of CGMS module.
 *
 *  @return Zero in case of success and error code in case of error.
 */
int bt_cgms_init(struct bt_cgms_init_param *init_params);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_CMGS_H_ */
