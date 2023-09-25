/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "lwm2m_client_utils/cmock_lwm2m_client_utils.h"
#include "lwm2m_client_utils/cmock_lwm2m_client_utils_location.h"
#include "lwm2m/cmock_lwm2m.h"
#include "lwm2m_resource_ids.h"
#include "lte_lc/cmock_lte_lc.h"
#include "date_time/cmock_date_time.h"

#include "lwm2m_codec_helpers.h"
#include "lwm2m_codec_defines.h"

/* Default test configuration values. */
#define LOCATION_TIMEOUT 30
#define ACTIVE_MODE true
#define ACTIVE_WAIT_TIMEOUT 120
#define MOVEMENT_RESOLUTION 120
#define MOVEMENT_TIMEOUT 3600
#define ACCELEROMETER_ACT_THRES 10
#define ACCELEROMETER_INACT_THRES 5
#define ACCELEROMETER_INACT_TIMEOUT 1
#define NOD_NEIGHBOR_CELL false
#define NOD_GNSS true

/* Mon Dec 05 2022 09:38:04 */
#define UNIX_TIMESTAMP_DUMMY 1670233084418

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

/* Used to verify that the uut properly sets the value that is returned by date_time_now() to
 * the Current Time resource 3/0/13.
 */
static int date_time_now_stub(int64_t *unix_time_ms, int no_of_calls)
{
	ARG_UNUSED(no_of_calls);

	*unix_time_ms = UNIX_TIMESTAMP_DUMMY;
	return 0;
}

/* Used to verify that callbacks are correctly set in testcase:
 * test_codec_helpers_setup_configuration_object_handler.
 */
static int callback(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
		    uint8_t *data, uint16_t data_len, bool last_block, size_t total_size)
{
	ARG_UNUSED(obj_inst_id);
	ARG_UNUSED(res_id);
	ARG_UNUSED(res_inst_id);
	ARG_UNUSED(data);
	ARG_UNUSED(data_len);
	ARG_UNUSED(last_block);
	ARG_UNUSED(total_size);

	return 0;
}

void test_create_objects_and_resources(void)
{
	/* Create object instances. */
	__cmock_lwm2m_create_object_inst_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0), 0);

	__cmock_lwm2m_create_object_inst_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0), 0);

	__cmock_lwm2m_create_object_inst_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0), 0);

	__cmock_lwm2m_create_object_inst_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON1_OBJ_INST_ID), 0);

	__cmock_lwm2m_create_object_inst_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON2_OBJ_INST_ID), 0);

	/* Crate resource instances. */
	__cmock_lwm2m_create_res_inst_ExpectAnyArgsAndReturn(0);
	__cmock_lwm2m_create_res_inst_ExpectAnyArgsAndReturn(0);
	__cmock_lwm2m_create_res_inst_ExpectAnyArgsAndReturn(0);
	__cmock_lwm2m_create_res_inst_ExpectAnyArgsAndReturn(0);
	__cmock_lwm2m_create_res_inst_ExpectAnyArgsAndReturn(0);

	TEST_ASSERT_EQUAL(0, lwm2m_codec_helpers_create_objects_and_resources());
}

void test_codec_helpers_setup_resource_buffers(void)
{
	time_t pressure_ts = 0;
	time_t temperature_ts = 0;
	time_t humidity_ts = 0;
	time_t button_ts = 0;
	int battery_voltage = 0;

	/* Ignore setting of sensor measurement boundaries. */
	__cmock_lwm2m_set_f64_IgnoreAndReturn(0);

	__cmock_lwm2m_set_res_buf_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_VOLTAGE_RID),
		&battery_voltage, sizeof(battery_voltage), sizeof(battery_voltage),
		LWM2M_RES_DATA_FLAG_RW, 0);

	__cmock_lwm2m_set_res_buf_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, TIMESTAMP_RID),
		&pressure_ts, sizeof(pressure_ts), sizeof(pressure_ts),
		LWM2M_RES_DATA_FLAG_RW, 0);

	__cmock_lwm2m_set_res_buf_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, TIMESTAMP_RID),
		&temperature_ts, sizeof(temperature_ts), sizeof(temperature_ts),
		LWM2M_RES_DATA_FLAG_RW, 0);

	__cmock_lwm2m_set_res_buf_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, TIMESTAMP_RID),
		&humidity_ts, sizeof(humidity_ts), sizeof(humidity_ts),
		LWM2M_RES_DATA_FLAG_RW, 0);

	__cmock_lwm2m_set_res_buf_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_UNITS_RID), BME680_TEMP_UNIT,
		strlen(BME680_TEMP_UNIT), strlen(BME680_TEMP_UNIT), LWM2M_RES_DATA_FLAG_RO, 0);

	__cmock_lwm2m_set_res_buf_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, SENSOR_UNITS_RID), BME680_HUMID_UNIT,
		strlen(BME680_HUMID_UNIT), strlen(BME680_HUMID_UNIT), LWM2M_RES_DATA_FLAG_RO, 0);

	__cmock_lwm2m_set_res_buf_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_UNITS_RID), BME680_PRESSURE_UNIT,
		strlen(BME680_PRESSURE_UNIT), strlen(BME680_PRESSURE_UNIT), LWM2M_RES_DATA_FLAG_RO,
		0);

	__cmock_lwm2m_set_res_buf_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON1_OBJ_INST_ID, APPLICATION_TYPE_RID),
		BUTTON1_APP_NAME, strlen(BUTTON1_APP_NAME), strlen(BUTTON1_APP_NAME),
		LWM2M_RES_DATA_FLAG_RO, 0);

	__cmock_lwm2m_set_res_buf_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON1_OBJ_INST_ID, TIMESTAMP_RID),
		&button_ts, sizeof(button_ts), sizeof(button_ts),
		LWM2M_RES_DATA_FLAG_RW, 0);

	__cmock_lwm2m_set_res_buf_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON2_OBJ_INST_ID, APPLICATION_TYPE_RID),
		BUTTON2_APP_NAME, strlen(BUTTON2_APP_NAME), strlen(BUTTON2_APP_NAME),
		LWM2M_RES_DATA_FLAG_RO, 0);

	__cmock_lwm2m_set_res_buf_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON2_OBJ_INST_ID, TIMESTAMP_RID),
		&button_ts, sizeof(button_ts), sizeof(button_ts),
		LWM2M_RES_DATA_FLAG_RW, 0);

	TEST_ASSERT_EQUAL(0, lwm2m_codec_helpers_setup_resources());
}

void test_codec_helpers_set_sensor_boundaries(void)
{
	double temp_min_range_val = BME680_TEMP_MIN_RANGE_VALUE;
	double temp_max_range_val = BME680_TEMP_MAX_RANGE_VALUE;
	double humid_min_range_val = BME680_HUMID_MIN_RANGE_VALUE;
	double humid_max_range_val = BME680_HUMID_MAX_RANGE_VALUE;
	double pressure_min_range_val = BME680_PRESSURE_MIN_RANGE_VALUE;
	double pressure_max_range_val = BME680_PRESSURE_MAX_RANGE_VALUE;

	/* Ignore setting of resource buffers. */
	__cmock_lwm2m_set_res_buf_IgnoreAndReturn(0);

	__cmock_lwm2m_set_f64_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, MIN_RANGE_VALUE_RID),
		temp_min_range_val, 0);

	__cmock_lwm2m_set_f64_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, MAX_RANGE_VALUE_RID),
		temp_max_range_val, 0);

	__cmock_lwm2m_set_f64_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, MIN_RANGE_VALUE_RID),
		humid_min_range_val, 0);

	__cmock_lwm2m_set_f64_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, MAX_RANGE_VALUE_RID),
		humid_max_range_val, 0);

	__cmock_lwm2m_set_f64_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, MIN_RANGE_VALUE_RID),
		pressure_min_range_val, 0);

	__cmock_lwm2m_set_f64_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, MAX_RANGE_VALUE_RID),
		pressure_max_range_val, 0);

	TEST_ASSERT_EQUAL(0, lwm2m_codec_helpers_setup_resources());
}

void test_codec_helpers_set_agps_data(void)
{
	struct cloud_data_agps_request agps = {
		.mnc = 1,
		.mcc = 242,
		.area = 30601,
		.cell = 52118273,
		.request.system[0].sv_mask_ephe = 0xFFFFFFFFu,
		.request.system[0].sv_mask_alm = 0xFFFFFFFFu,
		.request.data_flags = 0xFFFFFFFFu,
		.queued = true,
	};

	__cmock_location_assistance_agps_set_mask_ExpectAndReturn(&agps.request, 0);

	__cmock_location_assist_agps_set_elevation_mask_Expect(-1);

	__cmock_lwm2m_set_u32_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CELLID), agps.cell, 0);

	__cmock_lwm2m_set_u16_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMNC), agps.mnc, 0);

	__cmock_lwm2m_set_u16_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMCC), agps.mcc, 0);

	__cmock_lwm2m_set_u16_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, LAC), agps.area, 0);

	TEST_ASSERT_EQUAL(0, lwm2m_codec_helpers_set_agps_data(&agps));
}

void test_codec_helpers_set_pgps_data(void)
{
	struct cloud_data_pgps_request pgps = {
		.count = 42,
		.interval = 240,
		.day = 15160,
		.time = 40655,
		.queued = true
	};

	__cmock_location_assist_pgps_set_prediction_count_ExpectAndReturn(pgps.count, 0);
	__cmock_location_assist_pgps_set_prediction_interval_ExpectAndReturn(pgps.interval, 0);
	__cmock_location_assist_pgps_set_start_time_ExpectAndReturn(pgps.time, 0);
	__cmock_location_assist_pgps_set_start_gps_day_Expect(pgps.day);

	TEST_ASSERT_EQUAL(0, lwm2m_codec_helpers_set_pgps_data(&pgps));
}

void test_codec_helpers_setup_configuration_object(void)
{
	struct cloud_data_cfg cfg = {
		.location_timeout = LOCATION_TIMEOUT,
		.active_mode = ACTIVE_MODE,
		.active_wait_timeout = ACTIVE_WAIT_TIMEOUT,
		.movement_resolution = MOVEMENT_RESOLUTION,
		.movement_timeout = MOVEMENT_TIMEOUT,
		.accelerometer_activity_threshold = ACCELEROMETER_ACT_THRES,
		.accelerometer_inactivity_threshold = ACCELEROMETER_INACT_THRES,
		.accelerometer_inactivity_timeout = ACCELEROMETER_INACT_TIMEOUT,
		.no_data.gnss = NOD_GNSS,
		.no_data.neighbor_cell = NOD_NEIGHBOR_CELL
	};

	/* Ignore registering of post write callback for configuration object. */
	__cmock_lwm2m_register_post_write_callback_IgnoreAndReturn(0);

	__cmock_lwm2m_set_f64_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, ACCELEROMETER_ACT_THRESHOLD_RID),
		cfg.accelerometer_activity_threshold, 0);

	__cmock_lwm2m_set_f64_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, ACCELEROMETER_INACT_THRESHOLD_RID),
		cfg.accelerometer_inactivity_threshold, 0);

	__cmock_lwm2m_set_f64_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, ACCELEROMETER_INACT_TIMEOUT_RID),
		cfg.accelerometer_inactivity_timeout, 0);

	__cmock_lwm2m_set_s32_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, LOCATION_TIMEOUT_RID),
		cfg.location_timeout, 0);

	__cmock_lwm2m_set_s32_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, ACTIVE_WAIT_TIMEOUT_RID),
		cfg.active_wait_timeout, 0);

	__cmock_lwm2m_set_s32_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_RESOLUTION_RID),
		cfg.movement_resolution, 0);

	__cmock_lwm2m_set_s32_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_TIMEOUT_RID),
		cfg.movement_timeout, 0);

	__cmock_lwm2m_set_bool_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, PASSIVE_MODE_RID),
		!cfg.active_mode, 0);

	__cmock_lwm2m_set_bool_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, GNSS_ENABLE_RID),
		!cfg.no_data.gnss, 0);

	__cmock_lwm2m_set_bool_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, NEIGHBOR_CELL_ENABLE_RID),
		!cfg.no_data.neighbor_cell, 0);

	TEST_ASSERT_EQUAL(0, lwm2m_codec_helpers_setup_configuration_object(&cfg, NULL));
}

void test_codec_helpers_setup_configuration_object_handler(void)
{
	struct cloud_data_cfg cfg = { 0 };

	/* Ignore setting of configuration parameters. */
	__cmock_lwm2m_set_f64_IgnoreAndReturn(0);
	__cmock_lwm2m_set_s32_IgnoreAndReturn(0);
	__cmock_lwm2m_set_bool_IgnoreAndReturn(0);

	/* Check that callbacks are registered for every supported resource in the configuration
	 * object.
	 */
	__cmock_lwm2m_register_post_write_callback_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, PASSIVE_MODE_RID),
		&callback, 0);

	__cmock_lwm2m_register_post_write_callback_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, LOCATION_TIMEOUT_RID),
		&callback, 0);

	__cmock_lwm2m_register_post_write_callback_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, ACTIVE_WAIT_TIMEOUT_RID),
		&callback, 0);

	__cmock_lwm2m_register_post_write_callback_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_RESOLUTION_RID),
		&callback, 0);

	__cmock_lwm2m_register_post_write_callback_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_TIMEOUT_RID),
		&callback, 0);

	__cmock_lwm2m_register_post_write_callback_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, ACCELEROMETER_ACT_THRESHOLD_RID),
		&callback, 0);

	__cmock_lwm2m_register_post_write_callback_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, ACCELEROMETER_INACT_THRESHOLD_RID),
		&callback, 0);

	__cmock_lwm2m_register_post_write_callback_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, ACCELEROMETER_INACT_TIMEOUT_RID),
		&callback, 0);

	__cmock_lwm2m_register_post_write_callback_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, GNSS_ENABLE_RID),
		&callback, 0);

	__cmock_lwm2m_register_post_write_callback_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, NEIGHBOR_CELL_ENABLE_RID),
		&callback, 0);

	TEST_ASSERT_EQUAL(0, lwm2m_codec_helpers_setup_configuration_object(&cfg, &callback));
}

void test_codec_helpers_get_configuration_object(void)
{
	struct cloud_data_cfg cfg = {
		.location_timeout = LOCATION_TIMEOUT,
		.active_mode = ACTIVE_MODE,
		.active_wait_timeout = ACTIVE_WAIT_TIMEOUT,
		.movement_resolution = MOVEMENT_RESOLUTION,
		.movement_timeout = MOVEMENT_TIMEOUT,
		.accelerometer_activity_threshold = ACCELEROMETER_ACT_THRES,
		.accelerometer_inactivity_threshold = ACCELEROMETER_INACT_THRES,
		.accelerometer_inactivity_timeout = ACCELEROMETER_INACT_TIMEOUT,
		.no_data.gnss = NOD_GNSS,
		.no_data.neighbor_cell = NOD_NEIGHBOR_CELL
	};

	bool gnss_enable_temp = !cfg.no_data.gnss;
	bool ncell_enable_temp = !cfg.no_data.neighbor_cell;
	bool passive_mode_temp = !cfg.active_mode;

	__cmock_lwm2m_get_s32_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, LOCATION_TIMEOUT_RID), 0, 0);
	__cmock_lwm2m_get_s32_IgnoreArg_value();
	__cmock_lwm2m_get_s32_ReturnThruPtr_value(&cfg.location_timeout);

	__cmock_lwm2m_get_s32_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, ACTIVE_WAIT_TIMEOUT_RID), 0, 0);
	__cmock_lwm2m_get_s32_IgnoreArg_value();
	__cmock_lwm2m_get_s32_ReturnThruPtr_value(&cfg.active_wait_timeout);

	__cmock_lwm2m_get_s32_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_RESOLUTION_RID), 0, 0);
	__cmock_lwm2m_get_s32_IgnoreArg_value();
	__cmock_lwm2m_get_s32_ReturnThruPtr_value(&cfg.movement_resolution);

	__cmock_lwm2m_get_s32_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_TIMEOUT_RID), 0, 0);
	__cmock_lwm2m_get_s32_IgnoreArg_value();
	__cmock_lwm2m_get_s32_ReturnThruPtr_value(&cfg.movement_timeout);

	__cmock_lwm2m_get_f64_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, ACCELEROMETER_ACT_THRESHOLD_RID), 0, 0);
	__cmock_lwm2m_get_f64_IgnoreArg_value();
	__cmock_lwm2m_get_f64_ReturnThruPtr_value(&cfg.accelerometer_activity_threshold);

	__cmock_lwm2m_get_f64_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, ACCELEROMETER_INACT_THRESHOLD_RID), 0, 0);
	__cmock_lwm2m_get_f64_IgnoreArg_value();
	__cmock_lwm2m_get_f64_ReturnThruPtr_value(&cfg.accelerometer_inactivity_threshold);

	__cmock_lwm2m_get_f64_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, ACCELEROMETER_INACT_TIMEOUT_RID), 0, 0);
	__cmock_lwm2m_get_f64_IgnoreArg_value();
	__cmock_lwm2m_get_f64_ReturnThruPtr_value(&cfg.accelerometer_inactivity_timeout);

	__cmock_lwm2m_get_bool_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, PASSIVE_MODE_RID), 0, 0);
	__cmock_lwm2m_get_bool_IgnoreArg_value();
	__cmock_lwm2m_get_bool_ReturnThruPtr_value(&passive_mode_temp);

	__cmock_lwm2m_get_bool_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, GNSS_ENABLE_RID), 0, 0);
	__cmock_lwm2m_get_bool_IgnoreArg_value();
	__cmock_lwm2m_get_bool_ReturnThruPtr_value(&gnss_enable_temp);

	__cmock_lwm2m_get_bool_ExpectAndReturn(
		&LWM2M_OBJ(CONFIGURATION_OBJECT_ID, 0, NEIGHBOR_CELL_ENABLE_RID), 0, 0);
	__cmock_lwm2m_get_bool_IgnoreArg_value();
	__cmock_lwm2m_get_bool_ReturnThruPtr_value(&ncell_enable_temp);

	TEST_ASSERT_EQUAL(0, lwm2m_codec_helpers_get_configuration_object(&cfg));

	/* Check that the configuration structure has been populated with expected values. */
	TEST_ASSERT_EQUAL(LOCATION_TIMEOUT, cfg.location_timeout);
	TEST_ASSERT_EQUAL(ACTIVE_MODE, cfg.active_mode);
	TEST_ASSERT_EQUAL(ACTIVE_WAIT_TIMEOUT, cfg.active_wait_timeout);
	TEST_ASSERT_EQUAL(MOVEMENT_RESOLUTION, cfg.movement_resolution);
	TEST_ASSERT_EQUAL(MOVEMENT_TIMEOUT, cfg.movement_timeout);
	TEST_ASSERT_EQUAL_FLOAT(ACCELEROMETER_ACT_THRES, cfg.accelerometer_activity_threshold);
	TEST_ASSERT_EQUAL_FLOAT(ACCELEROMETER_INACT_THRES, cfg.accelerometer_inactivity_threshold);
	TEST_ASSERT_EQUAL_FLOAT(ACCELEROMETER_INACT_TIMEOUT, cfg.accelerometer_inactivity_timeout);
	TEST_ASSERT_EQUAL(NOD_GNSS, cfg.no_data.gnss);
	TEST_ASSERT_EQUAL(NOD_NEIGHBOR_CELL, cfg.no_data.neighbor_cell);
}

void test_codec_helpers_set_gnss_data(void)
{
	struct cloud_data_gnss gnss = {
		.pvt.longi = 10,
		.pvt.lat = 62,
		.pvt.acc = 24,
		.pvt.alt = 170,
		.pvt.spd = 1,
		.pvt.hdg = 176,
		.gnss_ts = 1000,
		.queued = true,
	};

	double alt = (double)gnss.pvt.alt;
	double acc = (double)gnss.pvt.acc;
	double spd = (double)gnss.pvt.spd;

	__cmock_date_time_uptime_to_unix_time_ms_ExpectAndReturn(&gnss.gnss_ts, 0);

	__cmock_lwm2m_set_f64_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0, LATITUDE_RID), gnss.pvt.lat, 0);

	__cmock_lwm2m_set_f64_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0, LONGITUDE_RID), gnss.pvt.longi, 0);

	__cmock_lwm2m_set_f64_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0, ALTITUDE_RID), alt, 0);

	__cmock_lwm2m_set_f64_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0, RADIUS_RID), acc, 0);

	__cmock_lwm2m_set_f64_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0, SPEED_RID), spd, 0);

	__cmock_lwm2m_set_time_ExpectAndReturn(&LWM2M_OBJ(LWM2M_OBJECT_LOCATION_ID, 0,
							  LOCATION_TIMESTAMP_RID),
					       (gnss.gnss_ts / MSEC_PER_SEC), 0);

	TEST_ASSERT_EQUAL(0, lwm2m_codec_helpers_set_gnss_data(&gnss));
}

void test_codec_helpers_set_sensor_data(void)
{
	struct cloud_data_sensors sensor = {
		.humidity = 50,
		.temperature = 23,
		.pressure = 80,
		.bsec_air_quality = 50,
		.env_ts = 1000,
		.queued = true,
	};

	__cmock_date_time_uptime_to_unix_time_ms_ExpectAndReturn(&sensor.env_ts, 0);

	__cmock_lwm2m_set_f64_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_VALUE_RID),
		sensor.temperature, 0);

	__cmock_lwm2m_set_f64_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, SENSOR_VALUE_RID),
		sensor.humidity, 0);

	__cmock_lwm2m_set_f64_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_VALUE_RID),
		sensor.pressure, 0);

	__cmock_lwm2m_set_time_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, TIMESTAMP_RID),
		(sensor.env_ts / MSEC_PER_SEC), 0);

	__cmock_lwm2m_set_time_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, TIMESTAMP_RID),
		(sensor.env_ts / MSEC_PER_SEC), 0);

	__cmock_lwm2m_set_time_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, TIMESTAMP_RID),
		(sensor.env_ts / MSEC_PER_SEC), 0);

	TEST_ASSERT_EQUAL(0, lwm2m_codec_helpers_set_sensor_data(&sensor));
}

void test_codec_helpers_set_modem_dynamic_data(void)
{
	struct cloud_data_modem_dynamic modem_dynamic = {
		.band = 3,
		.nw_mode = LTE_LC_LTE_MODE_NBIOT,
		.rsrp = -8,
		.area = 12,
		.mccmnc = "24202",
		.cell = 33703719,
		.ip = "10.81.183.99",
		.apn = "telenor.iot",
		.ts = 1000,
		.queued = true,
	};
	int64_t current_time = UNIX_TIMESTAMP_DUMMY;

	__cmock_date_time_now_Stub(date_time_now_stub);
	__cmock_date_time_now_ExpectAndReturn(&current_time, 0);

	__cmock_lwm2m_set_u8_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, NETWORK_BEARER_ID),
		NB_IOT_BEARER, 0);

	__cmock_lwm2m_set_res_buf_ExpectAnyArgsAndReturn(0);
	__cmock_lwm2m_set_res_buf_ExpectAnyArgsAndReturn(0);
	__cmock_lwm2m_set_res_buf_ExpectAnyArgsAndReturn(0);
	__cmock_lwm2m_set_res_buf_ExpectAnyArgsAndReturn(0);

	__cmock_lwm2m_set_s8_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, RSS),
		modem_dynamic.rsrp, 0);

	__cmock_lwm2m_set_u32_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CELLID),
		modem_dynamic.cell, 0);

	__cmock_lwm2m_set_u16_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMNC), modem_dynamic.mnc, 0);

	__cmock_lwm2m_set_u16_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMCC), modem_dynamic.mcc, 0);

	__cmock_lwm2m_set_u16_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, LAC), modem_dynamic.area, 0);

	__cmock_lwm2m_set_time_ExpectAndReturn(&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0,
							  CURRENT_TIME_RID),
					       (current_time / MSEC_PER_SEC), 0);

	TEST_ASSERT_EQUAL(0, lwm2m_codec_helpers_set_modem_dynamic_data(&modem_dynamic));
}

void test_codec_helpers_set_modem_static_data(void)
{
	struct cloud_data_modem_static modem_static = {
		.imei = "352656106111232",
		.iccid = "89450421180216211234",
		.fw = "mfw_nrf9160_1.2.3",
		.brdv = "nrf9160dk_nrf9160",
		.appv = "v1.0.0-development",
		.ts = 1000,
		.queued = true,
	};

	__cmock_lwm2m_set_res_buf_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, MODEL_NUMBER_RID), CONFIG_BOARD,
		strlen(CONFIG_BOARD), strlen(CONFIG_BOARD), LWM2M_RES_DATA_FLAG_RO, 0);

	__cmock_lwm2m_set_res_buf_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, HARDWARE_VERSION_RID), CONFIG_SOC,
		strlen(CONFIG_SOC), strlen(CONFIG_SOC), LWM2M_RES_DATA_FLAG_RO, 0);

	__cmock_lwm2m_set_res_buf_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, MANUFACTURER_RID),
		CONFIG_CLOUD_CODEC_MANUFACTURER, strlen(CONFIG_CLOUD_CODEC_MANUFACTURER),
		strlen(CONFIG_CLOUD_CODEC_MANUFACTURER), LWM2M_RES_DATA_FLAG_RO, 0);

	__cmock_lwm2m_set_res_buf_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, FIRMWARE_VERSION_RID), modem_static.appv,
		strlen(modem_static.appv), strlen(modem_static.appv), LWM2M_RES_DATA_FLAG_RO, 0);

	__cmock_lwm2m_set_res_buf_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, SOFTWARE_VERSION_RID), modem_static.fw,
		strlen(modem_static.fw), strlen(modem_static.fw), LWM2M_RES_DATA_FLAG_RO, 0);

	__cmock_lwm2m_set_res_buf_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, DEVICE_SERIAL_NUMBER_ID), modem_static.imei,
		strlen(modem_static.imei), strlen(modem_static.imei), LWM2M_RES_DATA_FLAG_RO, 0);

	TEST_ASSERT_EQUAL(0, lwm2m_codec_helpers_set_modem_static_data(&modem_static));
}

void test_codec_helpers_set_battery_data(void)
{
	struct cloud_data_battery battery = {
		.bat = 3600,
		.bat_ts = 1000,
		.queued = true
	};

	__cmock_lwm2m_set_s32_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_VOLTAGE_RID),
		battery.bat, 0);

	TEST_ASSERT_EQUAL(0, lwm2m_codec_helpers_set_battery_data(&battery));
}

void test_codec_helpers_set_user_interface_data_button_1(void)
{
	struct cloud_data_ui user_interface = {
		.btn = 1,
		.btn_ts = 1000,
		.queued = true
	};

	__cmock_date_time_uptime_to_unix_time_ms_ExpectAndReturn(&user_interface.btn_ts, 0);

	__cmock_lwm2m_set_bool_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, 0, DIGITAL_INPUT_STATE_RID), true, 0);

	__cmock_lwm2m_set_bool_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, 0, DIGITAL_INPUT_STATE_RID), false, 0);

	__cmock_lwm2m_set_time_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, 0, TIMESTAMP_RID),
		(user_interface.btn_ts / MSEC_PER_SEC), 0);

	TEST_ASSERT_EQUAL(0, lwm2m_codec_helpers_set_user_interface_data(&user_interface));
}

void test_codec_helpers_set_user_interface_data_button_2(void)
{
	struct cloud_data_ui user_interface = {
		.btn = 2,
		.btn_ts = 1000,
		.queued = true
	};

	__cmock_date_time_uptime_to_unix_time_ms_ExpectAndReturn(&user_interface.btn_ts, 0);

	__cmock_lwm2m_set_bool_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, 1, DIGITAL_INPUT_STATE_RID), true, 0);

	__cmock_lwm2m_set_bool_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, 1, DIGITAL_INPUT_STATE_RID), false, 0);

	__cmock_lwm2m_set_time_ExpectAndReturn(
		&LWM2M_OBJ(IPSO_OBJECT_PUSH_BUTTON_ID, 1, TIMESTAMP_RID),
		(user_interface.btn_ts / MSEC_PER_SEC), 0);

	TEST_ASSERT_EQUAL(0, lwm2m_codec_helpers_set_user_interface_data(&user_interface));
}

void test_codec_helpers_set_neighbor_cell_data(void)
{
	struct cloud_data_neighbor_cells ncell = {
		.cell_data.current_cell.mcc = 242,
		.cell_data.current_cell.mnc = 1,
		.cell_data.current_cell.id = 21679716,
		.cell_data.current_cell.tac = 40401,
		.cell_data.current_cell.earfcn = 6446,
		.cell_data.current_cell.timing_advance = 80,
		.cell_data.current_cell.rsrp = -7,
		.cell_data.current_cell.rsrq = 28,
		.cell_data.ncells_count = 2,
		.neighbor_cells[0].earfcn = 262143,
		.neighbor_cells[0].phys_cell_id = 501,
		.neighbor_cells[0].rsrp = -8,
		.neighbor_cells[0].rsrq = 25,
		.neighbor_cells[1].earfcn = 262265,
		.neighbor_cells[1].phys_cell_id = 503,
		.neighbor_cells[1].rsrp = -5,
		.neighbor_cells[1].rsrq = 20,
		.ts = 1000,
		.queued = true
	};
	struct lte_lc_cells_info *cells = &ncell.cell_data;

	cells->neighbor_cells = ncell.neighbor_cells;
	cells->ncells_count = ncell.cell_data.ncells_count;

	__cmock_lwm2m_update_signal_meas_objects_ExpectAndReturn(
		(const struct lte_lc_cells_info *)cells, 0);

	__cmock_lwm2m_set_s8_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, RSS),
		ncell.cell_data.current_cell.rsrp, 0);

	__cmock_lwm2m_set_u32_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CELLID),
		ncell.cell_data.current_cell.id, 0);

	__cmock_lwm2m_set_u16_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMNC),
		ncell.cell_data.current_cell.mnc, 0);

	__cmock_lwm2m_set_u16_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMCC),
		ncell.cell_data.current_cell.mcc, 0);

	__cmock_lwm2m_set_u16_ExpectAndReturn(
		&LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, LAC),
		ncell.cell_data.current_cell.tac, 0);

	TEST_ASSERT_EQUAL(0, lwm2m_codec_helpers_set_neighbor_cell_data(&ncell));
}

void test_codec_helpers_object_path_list_generate(void)
{
	struct cloud_codec_data output = { 0 };
	static const struct lwm2m_obj_path path_list_1[] = {
		LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, TIMESTAMP_RID),
		LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, TIMESTAMP_RID),
		LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, TIMESTAMP_RID),
		LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_VALUE_RID),
		LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, SENSOR_VALUE_RID),
		LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_VALUE_RID)
	};
	static const struct lwm2m_obj_path path_list_2[] = {
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CELLID),
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMNC),
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMCC),
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, LAC)
	};
	static const struct lwm2m_obj_path path_list_expected[] = {
		LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, TIMESTAMP_RID),
		LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, TIMESTAMP_RID),
		LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, TIMESTAMP_RID),
		LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_VALUE_RID),
		LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, SENSOR_VALUE_RID),
		LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_VALUE_RID),
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CELLID),
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMNC),
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMCC),
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, LAC)
	};

	TEST_ASSERT_EQUAL(0, lwm2m_codec_helpers_object_path_list_add(&output,
								      path_list_1,
								      ARRAY_SIZE(path_list_1)));

	/* Verify that the correct output has been generated and the correct number of
	 * valid object paths has been set.
	 */
	TEST_ASSERT_EQUAL(ARRAY_SIZE(path_list_1), output.valid_object_paths);

	for (int i = 0; i < ARRAY_SIZE(path_list_1); i++) {
		TEST_ASSERT_EQUAL(path_list_1[i].level, output.paths[i].level);
		if (output.paths[i].level > 0) {
			TEST_ASSERT_EQUAL(path_list_1[i].obj_id, output.paths[i].obj_id);
		}
		if (output.paths[i].level > 1) {
			TEST_ASSERT_EQUAL(path_list_1[i].obj_inst_id, output.paths[i].obj_inst_id);
		}
		if (output.paths[i].level > 2) {
			TEST_ASSERT_EQUAL(path_list_1[i].res_id, output.paths[i].res_id);
		}
		if (output.paths[i].level > 3) {
			TEST_ASSERT_EQUAL(path_list_1[i].res_inst_id, output.paths[i].res_inst_id);
		}
	}

	/* Verify that the output contains the accumulated paths and that the correct number of
	 * valid object paths has been increased.
	 */
	TEST_ASSERT_EQUAL(0, lwm2m_codec_helpers_object_path_list_add(&output,
								      path_list_2,
								      ARRAY_SIZE(path_list_2)));

	TEST_ASSERT_EQUAL(ARRAY_SIZE(path_list_expected), output.valid_object_paths);

	for (int i = 0; i < ARRAY_SIZE(path_list_expected); i++) {
		TEST_ASSERT_EQUAL(path_list_expected[i].level, output.paths[i].level);
		if (output.paths[i].level > 0) {
			TEST_ASSERT_EQUAL(path_list_expected[i].obj_id, output.paths[i].obj_id);
		}
		if (output.paths[i].level > 1) {
			TEST_ASSERT_EQUAL(path_list_expected[i].obj_inst_id,
					  output.paths[i].obj_inst_id);
		}
		if (output.paths[i].level > 2) {
			TEST_ASSERT_EQUAL(path_list_expected[i].res_id, output.paths[i].res_id);
		}
		if (output.paths[i].level > 3) {
			TEST_ASSERT_EQUAL(path_list_expected[i].res_inst_id,
					  output.paths[i].res_inst_id);
		}
	}
}

void test_codec_helpers_object_path_list_generate_too_many_paths(void)
{
	struct cloud_codec_data output = { 0 };
	static const struct lwm2m_obj_path path_list[] = {
		LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, TIMESTAMP_RID),
		LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, TIMESTAMP_RID),
		LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, TIMESTAMP_RID),
		LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_VALUE_RID),
		LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, SENSOR_VALUE_RID),
		LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_VALUE_RID),
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CELLID),
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMNC),
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMCC),
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, LAC),
		LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, TIMESTAMP_RID),
		LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, TIMESTAMP_RID),
		LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, TIMESTAMP_RID),
		LWM2M_OBJ(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_VALUE_RID),
		LWM2M_OBJ(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, SENSOR_VALUE_RID),
		LWM2M_OBJ(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_VALUE_RID),
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CELLID),
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMNC),
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMCC),
		LWM2M_OBJ(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, LAC)
	};

	/* Verify that the API returns -ENOMEM if paths are added to a full list.
	 * The size of the generated path list in output is set by
	 * CONFIG_CLOUD_CODEC_LWM2M_PATH_LIST_ENTRIES_MAX
	 */
	TEST_ASSERT_EQUAL(-ENOMEM, lwm2m_codec_helpers_object_path_list_add(&output,
									    path_list,
									    ARRAY_SIZE(path_list)));
}

int main(void)
{
	(void)unity_main();
	return 0;
}
