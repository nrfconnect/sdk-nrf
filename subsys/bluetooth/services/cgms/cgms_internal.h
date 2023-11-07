/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_CGMS_INTERNAL_H_
#define BT_CGMS_INTERNAL_H_

#include <zephyr/types.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/services/cgms.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Continuous Glucose Monitoring feature */
enum cgms_feat {
	CGMS_FEAT_CALIBRATION_SUPPORTED = BIT(0),
	CGMS_FEAT_PATIENT_HIGH_LOW_ALERTS_SUPPORTED = BIT(1),
	CGMS_FEAT_HYPO_ALERTS_SUPPORTED = BIT(2),
	CGMS_FEAT_HYPER_ALERTS_SUPPORTED = BIT(3),
	CGMS_FEAT_RATE_OF_INCREASE_DECREASE_ALERTS_SUPPORTED = BIT(4),
	CGMS_FEAT_DEVICE_SPECIFIC_ALERT_SUPPORTED = BIT(5),
	CGMS_FEAT_SENSOR_MALFUNCTION_DETECTION_SUPPORTED = BIT(6),
	CGMS_FEAT_SENSOR_TEMPERATURE_HIGH_LOW_DETECTION_SUPPORTED = BIT(7),
	CGMS_FEAT_SENSOR_RESULT_HIGH_LOW_DETECTION_SUPPORTED = BIT(8),
	CGMS_FEAT_LOW_BATTERY_DETECTION_SUPPORTED = BIT(9),
	CGMS_FEAT_SENSOR_TYPE_ERROR_DETECTION_SUPPORTED = BIT(10),
	CGMS_FEAT_GENERAL_DEVICE_FAULT_SUPPORTED = BIT(11),
	CGMS_FEAT_E2E_CRC_SUPPORTED = BIT(12),
	CGMS_FEAT_MULTIPLE_BOND_SUPPORTED = BIT(13),
	CGMS_FEAT_MULTIPLE_SESSIONS_SUPPORTED = BIT(14),
	CGMS_FEAT_CGM_TREND_INFORMATION_SUPPORTED = BIT(15),
	CGMS_FEAT_CGM_QUALITY_SUPPORTED = BIT(16),
};

/* CGM sensor status bit position */
enum cgms_status_pos {
	CGMS_STATUS_POS_SESSION_STOPPED = 0,
	CGMS_STATUS_POS_DEVICE_BATTERY_LOW = 1,
	CGMS_STATUS_POS_SENSOR_TYPE_INCORRECT_FOR_DEVICE = 2,
	CGMS_STATUS_POS_SENSOR_MALFUNCTION = 3,
	CGMS_STATUS_POS_DEVICE_SPECIFIC_ALERT = 4,
	CGMS_STATUS_POS_GENERAL_DEVICE_FAULT = 5,
};

/* CGM Measurement flags */
enum cgms_meas_flag {
	CGMS_MEAS_FLAG_TREND_INFO_PRESENT = BIT(0),
	CGMS_MEAS_FLAG_QUALITY_PRESENT = BIT(1),
	CGMS_MEAS_FLAG_STATUS_WARNING_OCT_PRESENT = BIT(5),
	CGMS_MEAS_FLAG_STATUS_CALTEMP_OCT_PRESENT = BIT(6),
	CGMS_MEAS_FLAG_STATUS_STATUS_OCT_PRESENT = BIT(7),
};

struct cgms_sensor_annunc {
	/* Warning annunciation. */
	uint8_t warning;
	/* Calibration and Temperature annunciation. */
	uint8_t calib_temp;
	/* Status annunciation. */
	uint8_t status;
};

struct cgms_meas {
	/* Indicates the presence of optional fields and the Sensor Status Annunciation field. */
	uint8_t flag;
	/* Glucose concentration. */
	/* 16-bit word comprising 4-bit exponent and signed 12-bit mantissa. */
	uint16_t glucose_concentration;
	/* Time offset. Represents the time difference between measurements. */
	uint16_t time_offset;
	/* Sensor Status Annunciation.*/
	/* Variable length, can include Status, Cal/Temp, and Warning. */
	struct cgms_sensor_annunc sensor_status_annunciation;
};

struct cgms_feature {
	/* Indicates the feature the glucose sensor supports. */
	uint32_t feature;
	/* The type of the glucose sensor. */
	enum bt_cgms_feat_type type;
	/* The location of the glucose sensor. */
	enum bt_cgms_feat_loc sample_location;
};

struct cgms_session_start_time {
	/* Year of current date. */
	uint16_t year;
	/* Month of current date. */
	uint8_t month;
	/* Day of current date. */
	uint8_t day;
	/* Hour of current time. */
	uint8_t hour;
	/* Minute of current time. */
	uint8_t minute;
	/* Second of current time. */
	uint8_t second;
	/* Timezone of the place where the sensor is. */
	int8_t timezone;
	/* Indicates if daylight saving time is used. */
	uint8_t dst_offset;
};

struct cgms_server {
	/* The start time of current session. */
	struct cgms_session_start_time sst;
	/* The session run time, showing how many hours it lasts. */
	uint16_t srt;
	/* Sensor's local time. */
	uint32_t local_start_time;
	/* Warning annunciation. */
	atomic_t warning;
	/* Calibration and Temperature annunciation. */
	atomic_t calib_temp;
	/* Status annunciation. */
	atomic_t status;
	/* Sensor's feature. */
	struct cgms_feature feature;
	/* The communication interval between sensor and user. */
	uint16_t comm_interval;
	/* Minimum communication interval in minute. */
	uint16_t comm_interval_minimum;
	/* Callback structure to notify application. */
	struct bt_cgms_cb cb;
};

/* Function for handling SOCP request. */
int cgms_socp_recv_request(struct bt_conn *peer, struct cgms_server *instance,
			const uint8_t *req_data, uint16_t req_len);

/* Function for sending SOCP response. */
int cgms_socp_send_response(struct bt_conn *peer, struct net_buf_simple *rsp);

/* Function for handling RACP request. */
int cgms_racp_recv_request(struct bt_conn *peer, const uint8_t *req_data, uint16_t req_len);

/* Function for sending RACP response. */
int cgms_racp_send_response(struct bt_conn *peer, struct net_buf_simple *rsp);

/* Function for sending RACP records. */
int cgms_racp_send_record(struct bt_conn *peer, struct cgms_meas *entry);

/* Function for retrieving the newest RACP records. */
int cgms_racp_meas_get_latest(struct cgms_meas *meas);

/* Function for adding RACP records. */
int cgms_racp_meas_add(struct cgms_meas meas);

/* Function for initializing RACP module.
 * It must be called before using any RACP function.
 */
void cgms_racp_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BT_CGMS_INTERNAL_H_ */
