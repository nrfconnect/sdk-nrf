/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LWM2M_CODEC_DEFINES_H__
#define LWM2M_CODEC_DEFINES_H__

/* LwM2M read-write flag. */
#define LWM2M_RES_DATA_FLAG_RW 0

/* Location object RIDs. */
#define LATITUDE_RID		0
#define LONGITUDE_RID		1
#define ALTITUDE_RID		2
#define RADIUS_RID		3
#define LOCATION_TIMESTAMP_RID	5
#define SPEED_RID		6

/* Connectivity monitoring object RIDs. */
#define NETWORK_BEARER_ID		0
#define AVAIL_NETWORK_BEARER_ID		1
/* Radio Signal Strength */
#define RSS				2
#define IP_ADDRESSES			4
#define APN				7
#define CELLID				8
#define SMNC				9
#define SMCC				10
#define LAC				12

/* Device object RIDs. */
#define FIRMWARE_VERSION_RID		3
#define SOFTWARE_VERSION_RID		19
#define DEVICE_SERIAL_NUMBER_ID		2
#define CURRENT_TIME_RID		13
#define POWER_SOURCE_VOLTAGE_RID	7
#define MODEL_NUMBER_RID		1
#define MANUFACTURER_RID		0
#define HARDWARE_VERSION_RID		18

#define CONFIGURATION_OBJECT_ID			50009
#define PASSIVE_MODE_RID			0
#define LOCATION_TIMEOUT_RID			1
#define ACTIVE_WAIT_TIMEOUT_RID			2
#define MOVEMENT_RESOLUTION_RID			3
#define MOVEMENT_TIMEOUT_RID			4
#define ACCELEROMETER_ACT_THRESHOLD_RID		5
#define GNSS_ENABLE_RID				6
#define NEIGHBOR_CELL_ENABLE_RID		7
#define ACCELEROMETER_INACT_THRESHOLD_RID	8
#define ACCELEROMETER_INACT_TIMEOUT_RID		9

/* LTE-FDD (LTE-M) bearer & NB-IoT bearer. */
#define LTE_FDD_BEARER 6U
#define NB_IOT_BEARER 7U

/* Temperature sensor metadata. */
#define BME680_TEMP_MIN_RANGE_VALUE -40.0
#define BME680_TEMP_MAX_RANGE_VALUE 85.0
#define BME680_TEMP_UNIT "Celsius degrees"

/* Humidity sensor metadata. */
#define BME680_HUMID_MIN_RANGE_VALUE 0.0
#define BME680_HUMID_MAX_RANGE_VALUE 100.0
#define BME680_HUMID_UNIT "%"

/* Pressure sensor metadata. */
#define BME680_PRESSURE_MIN_RANGE_VALUE 30.0
#define BME680_PRESSURE_MAX_RANGE_VALUE 110.0
#define BME680_PRESSURE_UNIT "kPa"

/* Button object. */
#define BUTTON1_OBJ_INST_ID 0
#define BUTTON1_APP_NAME "Push button 1"
#define BUTTON2_OBJ_INST_ID 1
#define BUTTON2_APP_NAME "Push button 2"

#endif /* LWM2M_CODEC_DEFINES_H */
