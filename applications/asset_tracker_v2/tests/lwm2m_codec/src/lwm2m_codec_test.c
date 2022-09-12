/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <unity.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <net/lwm2m.h>
#include <net/lwm2m_client_utils.h>

#include "lwm2m_client_utils/mock_lwm2m_client_utils.h"
#include "lwm2m/mock_lwm2m.h"
#include "lwm2m/mock_lwm2m_resource_ids.h"
#include "lte_lc/mock_lte_lc.h"
#include "date_time/mock_date_time.h"

#include "cloud_codec.h"
#include "lwm2m_codec_defines.h"

/* Default test configuration values. */
#define GNSS_TIMEOUT 30
#define ACTIVE_MODE true
#define ACTIVE_WAIT_TIMEOUT 120
#define MOVEMENT_RESOLUTION 120
#define MOVEMENT_TIMEOUT 3600
#define ACCELEROMETER_ACT_THRES 10
#define ACCELEROMETER_INACT_THRES 5
#define ACCELEROMETER_INACT_TIMEOUT 1
#define NOD_NEIGHBOR_CELL false
#define NOD_GNSS true

static uint8_t bearers[2] = { LTE_FDD_BEARER, NB_IOT_BEARER };
static int battery_voltage;
static int32_t pressure_ts;
static int32_t temperature_ts;
static int32_t humidity_ts;
static int32_t button_ts;
static int64_t current_time;
static double temp_min_range_val = BME680_TEMP_MIN_RANGE_VALUE;
static double temp_max_range_val = BME680_TEMP_MAX_RANGE_VALUE;
static double humid_min_range_val = BME680_HUMID_MIN_RANGE_VALUE;
static double humid_max_range_val = BME680_HUMID_MAX_RANGE_VALUE;
static double pressure_min_range_val = BME680_PRESSURE_MIN_RANGE_VALUE;
static double pressure_max_range_val = BME680_PRESSURE_MAX_RANGE_VALUE;

/* Callback handler used to trigger configuration updates in the uut. */
static lwm2m_engine_set_data_cb_t post_write_cb;

extern int unity_main(void);
extern int generic_suiteTearDown(int num_failures);

/* Suite teardown shall finalize with mandatory call to generic_suiteTearDown. */
int test_suiteTearDown(int num_failures)
{
	return generic_suiteTearDown(num_failures);
}

void setUp(void)
{
	mock_lwm2m_client_utils_Init();
	mock_lwm2m_Init();
	mock_date_time_Init();
	mock_lwm2m_resource_ids_Init();
	mock_lte_lc_Init();
}

void tearDown(void)
{
	mock_lwm2m_client_utils_Verify();
	mock_lwm2m_Verify();
	mock_date_time_Verify();
	mock_lwm2m_resource_ids_Init();
	mock_lte_lc_Init();
}

static int post_write_callback_stub(const char *pathstr,
				    lwm2m_engine_set_data_cb_t cb,
				    int no_of_calls)
{
	post_write_cb = cb;
	return 0;
}

/* Handler used to verify correct config update in the
 * test_lwm2m_codec_config_update test case.
 */
static void cloud_codec_event_handler(const struct cloud_codec_evt *evt)
{
	TEST_ASSERT_EQUAL(CLOUD_CODEC_EVT_CONFIG_UPDATE, evt->type);
	TEST_ASSERT_EQUAL(GNSS_TIMEOUT, evt->config_update.gnss_timeout);
	TEST_ASSERT_EQUAL(ACTIVE_MODE, evt->config_update.active_mode);
	TEST_ASSERT_EQUAL(ACTIVE_WAIT_TIMEOUT, evt->config_update.active_wait_timeout);
	TEST_ASSERT_EQUAL(MOVEMENT_RESOLUTION, evt->config_update.movement_resolution);
	TEST_ASSERT_EQUAL(MOVEMENT_TIMEOUT, evt->config_update.movement_timeout);
	TEST_ASSERT_EQUAL_FLOAT(ACCELEROMETER_ACT_THRES,
				evt->config_update.accelerometer_activity_threshold);
	TEST_ASSERT_EQUAL_FLOAT(ACCELEROMETER_INACT_THRES,
				evt->config_update.accelerometer_inactivity_threshold);
	TEST_ASSERT_EQUAL_FLOAT(ACCELEROMETER_INACT_TIMEOUT,
				evt->config_update.accelerometer_inactivity_timeout);
	TEST_ASSERT_EQUAL(NOD_GNSS, evt->config_update.no_data.gnss);
	TEST_ASSERT_EQUAL(NOD_NEIGHBOR_CELL, evt->config_update.no_data.neighbor_cell);
}

void test_lwm2m_cloud_codec_init(void)
{
	struct cloud_data_cfg cfg = {
		.gnss_timeout = GNSS_TIMEOUT,
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

	/* Create object instances. */
	__wrap_lwm2m_engine_create_obj_inst_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0), 0);

	__wrap_lwm2m_engine_create_obj_inst_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0), 0);

	__wrap_lwm2m_engine_create_obj_inst_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0), 0);

	__wrap_lwm2m_engine_create_obj_inst_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON1_OBJ_INST_ID), 0);

	__wrap_lwm2m_engine_create_obj_inst_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON2_OBJ_INST_ID), 0);

	/* Crate resource instances. */
	__wrap_lwm2m_engine_create_res_inst_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, AVAIL_NETWORK_BEARER_ID, 0),
		0);

	__wrap_lwm2m_engine_create_res_inst_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, AVAIL_NETWORK_BEARER_ID, 1),
		0);

	__wrap_lwm2m_engine_create_res_inst_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, IP_ADDRESSES, 0), 0);

	__wrap_lwm2m_engine_create_res_inst_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, APN, 0), 0);

	__wrap_lwm2m_engine_create_res_inst_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_VOLTAGE_RID, 0), 0);

	/* Set resource buffers. */
	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, CURRENT_TIME_RID),
		&current_time, sizeof(current_time), sizeof(current_time),
		LWM2M_RES_DATA_FLAG_RW, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_VOLTAGE_RID),
		&battery_voltage, sizeof(battery_voltage), sizeof(battery_voltage),
		LWM2M_RES_DATA_FLAG_RW, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, TIMESTAMP_RID),
		&pressure_ts, sizeof(pressure_ts), sizeof(pressure_ts),
		LWM2M_RES_DATA_FLAG_RW, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0, TIMESTAMP_RID),
		&temperature_ts, sizeof(temperature_ts), sizeof(temperature_ts),
		LWM2M_RES_DATA_FLAG_RW, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, TIMESTAMP_RID),
		&humidity_ts, sizeof(humidity_ts), sizeof(humidity_ts),
		LWM2M_RES_DATA_FLAG_RW, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_UNITS_RID), BME680_TEMP_UNIT,
		sizeof(BME680_TEMP_UNIT), sizeof(BME680_TEMP_UNIT), LWM2M_RES_DATA_FLAG_RO, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, SENSOR_UNITS_RID), BME680_HUMID_UNIT,
		sizeof(BME680_HUMID_UNIT), sizeof(BME680_HUMID_UNIT), LWM2M_RES_DATA_FLAG_RO, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_UNITS_RID), BME680_PRESSURE_UNIT,
		sizeof(BME680_PRESSURE_UNIT), sizeof(BME680_PRESSURE_UNIT), LWM2M_RES_DATA_FLAG_RO,
		0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON1_OBJ_INST_ID, APPLICATION_TYPE_RID),
		BUTTON1_APP_NAME, sizeof(BUTTON1_APP_NAME), sizeof(BUTTON1_APP_NAME),
		LWM2M_RES_DATA_FLAG_RO, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON1_OBJ_INST_ID, TIMESTAMP_RID),
		&button_ts, sizeof(button_ts), sizeof(button_ts),
		LWM2M_RES_DATA_FLAG_RW, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON2_OBJ_INST_ID, APPLICATION_TYPE_RID),
		BUTTON2_APP_NAME, sizeof(BUTTON2_APP_NAME), sizeof(BUTTON2_APP_NAME),
		LWM2M_RES_DATA_FLAG_RO, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID, BUTTON2_OBJ_INST_ID, TIMESTAMP_RID),
		&button_ts, sizeof(button_ts), sizeof(button_ts),
		LWM2M_RES_DATA_FLAG_RW, 0);

	/* Set floats. */
	__wrap_lwm2m_engine_set_float_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0, MIN_RANGE_VALUE_RID),
		&temp_min_range_val, 0);

	__wrap_lwm2m_engine_set_float_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0, MAX_RANGE_VALUE_RID),
		&temp_max_range_val, 0);

	__wrap_lwm2m_engine_set_float_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, MIN_RANGE_VALUE_RID),
		&humid_min_range_val, 0);

	__wrap_lwm2m_engine_set_float_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, MAX_RANGE_VALUE_RID),
		&humid_max_range_val, 0);

	__wrap_lwm2m_engine_set_float_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, MIN_RANGE_VALUE_RID),
		&pressure_min_range_val, 0);

	__wrap_lwm2m_engine_set_float_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, MAX_RANGE_VALUE_RID),
		&pressure_max_range_val, 0);

	__wrap_lwm2m_engine_set_float_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, ACCELEROMETER_ACT_THRESHOLD_RID),
		&cfg.accelerometer_activity_threshold, 0);

	__wrap_lwm2m_engine_set_float_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, ACCELEROMETER_INACT_THRESHOLD_RID),
		&cfg.accelerometer_inactivity_threshold, 0);

	__wrap_lwm2m_engine_set_float_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, ACCELEROMETER_INACT_TIMEOUT_RID),
		&cfg.accelerometer_inactivity_timeout, 0);

	/* Set integers and booleans. */
	__wrap_lwm2m_engine_set_s32_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, GNSS_TIMEOUT_RID),
		cfg.gnss_timeout, 0);

	__wrap_lwm2m_engine_set_s32_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, ACTIVE_WAIT_TIMEOUT_RID),
		cfg.active_wait_timeout, 0);

	__wrap_lwm2m_engine_set_s32_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_RESOLUTION_RID),
		cfg.movement_resolution, 0);

	__wrap_lwm2m_engine_set_s32_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_TIMEOUT_RID),
		cfg.movement_timeout, 0);

	__wrap_lwm2m_engine_set_bool_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, PASSIVE_MODE_RID),
		!cfg.active_mode, 0);

	__wrap_lwm2m_engine_set_bool_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, GNSS_ENABLE_RID),
		!cfg.no_data.gnss, 0);

	__wrap_lwm2m_engine_set_bool_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, NEIGHBOR_CELL_ENABLE_RID),
		!cfg.no_data.neighbor_cell, 0);

	/* Register callbacks from changes in device configurations. */
	__wrap_lwm2m_engine_register_post_write_callback_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, PASSIVE_MODE_RID),
		NULL, 0);
	__wrap_lwm2m_engine_register_post_write_callback_IgnoreArg_cb();

	__wrap_lwm2m_engine_register_post_write_callback_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, GNSS_TIMEOUT_RID),
		NULL, 0);
	__wrap_lwm2m_engine_register_post_write_callback_IgnoreArg_cb();

	__wrap_lwm2m_engine_register_post_write_callback_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, ACTIVE_WAIT_TIMEOUT_RID),
		NULL, 0);
	__wrap_lwm2m_engine_register_post_write_callback_IgnoreArg_cb();

	__wrap_lwm2m_engine_register_post_write_callback_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_RESOLUTION_RID),
		NULL, 0);
	__wrap_lwm2m_engine_register_post_write_callback_IgnoreArg_cb();

	__wrap_lwm2m_engine_register_post_write_callback_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_TIMEOUT_RID),
		NULL, 0);
	__wrap_lwm2m_engine_register_post_write_callback_IgnoreArg_cb();

	__wrap_lwm2m_engine_register_post_write_callback_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, ACCELEROMETER_ACT_THRESHOLD_RID),
		NULL, 0);
	__wrap_lwm2m_engine_register_post_write_callback_IgnoreArg_cb();

	__wrap_lwm2m_engine_register_post_write_callback_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, ACCELEROMETER_INACT_THRESHOLD_RID),
		NULL, 0);
	__wrap_lwm2m_engine_register_post_write_callback_IgnoreArg_cb();

	__wrap_lwm2m_engine_register_post_write_callback_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, ACCELEROMETER_INACT_TIMEOUT_RID),
		NULL, 0);
	__wrap_lwm2m_engine_register_post_write_callback_IgnoreArg_cb();

	__wrap_lwm2m_engine_register_post_write_callback_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, GNSS_ENABLE_RID),
		NULL, 0);
	__wrap_lwm2m_engine_register_post_write_callback_IgnoreArg_cb();

	__wrap_lwm2m_engine_register_post_write_callback_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, NEIGHBOR_CELL_ENABLE_RID),
		NULL, 0);
	__wrap_lwm2m_engine_register_post_write_callback_IgnoreArg_cb();

	__wrap_lwm2m_engine_register_post_write_callback_AddCallback(&post_write_callback_stub);

	TEST_ASSERT_NOT_NULL(post_write_callback_stub);
	TEST_ASSERT_EQUAL(0, cloud_codec_init(&cfg, cloud_codec_event_handler));
}

void test_lwm2m_cloud_codec_encode_neighbor_cells(void)
{
	struct cloud_codec_data codec = { 0 };
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

	__wrap_lwm2m_update_signal_meas_objects_ExpectAndReturn(
		(const struct lte_lc_cells_info *)cells, 0);

	__wrap_lwm2m_engine_set_s32_ExpectAndReturn(
		LWM2M_PATH(LOCATION_ASSIST_OBJECT_ID, 0, ASSIST_TYPE_RID),
		ASSISTANCE_REQUEST_TYPE_MULTICELL_REQUEST, 0);

	__wrap_lwm2m_engine_set_s8_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, RADIO_SIGNAL_STRENGTH),
		ncell.cell_data.current_cell.rsrp, 0);

	__wrap_lwm2m_engine_set_u32_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CELLID),
		ncell.cell_data.current_cell.id, 0);

	__wrap_lwm2m_engine_set_u16_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMNC),
		ncell.cell_data.current_cell.mnc, 0);

	__wrap_lwm2m_engine_set_u16_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMCC),
		ncell.cell_data.current_cell.mcc, 0);

	__wrap_lwm2m_engine_set_u16_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, LAC),
		ncell.cell_data.current_cell.tac, 0);

	const char *const path_list[] = {
		LWM2M_PATH(ECID_SIGNAL_MEASUREMENT_INFO_OBJECT_ID),
		LWM2M_PATH(LOCATION_ASSIST_OBJECT_ID, 0, ASSIST_TYPE_RID),
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, RADIO_SIGNAL_STRENGTH),
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CELLID),
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMNC),
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMCC),
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, LAC)
	};

	TEST_ASSERT_EQUAL(0, cloud_codec_encode_neighbor_cells(&codec, &ncell));
	TEST_ASSERT_EQUAL(ARRAY_SIZE(path_list), codec.valid_object_paths);

	for (int i = 0; i < ARRAY_SIZE(path_list); i++) {
		TEST_ASSERT_EQUAL(0, strcmp(path_list[i], codec.paths[i]));
	}
}

void test_lwm2m_cloud_codec_encode_agps_request(void)
{
	struct cloud_codec_data codec = { 0 };
	struct cloud_data_agps_request agps = {
		.mnc = 1,
		.mcc = 242,
		.area = 30601,
		.cell = 52118273,
		.request.sv_mask_ephe = 0xFFFFFFFFu,
		.request.sv_mask_alm = 0xFFFFFFFFu,
		.request.data_flags = 0xFFFFFFFFu,
		.queued = true,
	};

	/* Expected generated A-GPS mask when all A-GPS data is requested. */
	__wrap_location_assist_agps_request_set_Expect(0x000001FFu);

	__wrap_lwm2m_engine_set_u32_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CELLID), agps.cell, 0);

	__wrap_lwm2m_engine_set_u16_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMNC), agps.mnc, 0);

	__wrap_lwm2m_engine_set_u16_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMCC), agps.mcc, 0);

	__wrap_lwm2m_engine_set_u16_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, LAC), agps.area, 0);

	const char *const path_list[] = {
		LWM2M_PATH(LOCATION_ASSIST_OBJECT_ID, 0, ASSIST_TYPE_RID),
		LWM2M_PATH(LOCATION_ASSIST_OBJECT_ID, 0, AGPS_MASK_RID),
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CELLID),
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMNC),
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMCC),
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, LAC)
	};

	TEST_ASSERT_EQUAL(0, cloud_codec_encode_agps_request(&codec, &agps));
	TEST_ASSERT_EQUAL(ARRAY_SIZE(path_list), codec.valid_object_paths);

	for (int i = 0; i < ARRAY_SIZE(path_list); i++) {
		TEST_ASSERT_EQUAL(0, strcmp(path_list[i], codec.paths[i]));
	}
}

void test_lwm2m_cloud_codec_encode_data(void)
{
	struct cloud_codec_data codec = { 0 };
	struct cloud_data_battery battery = { .bat = 3600, .bat_ts = 1000, .queued = true };
	struct cloud_data_gnss gnss = {
		.pvt.longi = 10,
		.pvt.lat = 62,
		.pvt.acc = 24,
		.pvt.alt = 170,
		.pvt.spd = 1,
		.pvt.hdg = 176,
		.gnss_ts = 1000,
		.queued = true,
		.format = CLOUD_CODEC_GNSS_FORMAT_PVT,
	};
	struct cloud_data_modem_dynamic modem_dynamic = {
		.band = 3,
		.nw_mode = LTE_LC_LTE_MODE_NBIOT,
		.rsrp = -8,
		.area = 12,
		.mccmnc = "24202",
		.cell = 33703719,
		.ip = "10.81.183.99",
		.ts = 1000,
		.queued = true,
		.band_fresh = true,
		.nw_mode_fresh = true,
		.area_code_fresh = true,
		.cell_id_fresh = true,
		.rsrp_fresh = true,
		.ip_address_fresh = true,
		.mccmnc_fresh = true,
	};
	struct cloud_data_modem_static modem_static = {
		.imei = "352656106111232",
		.iccid = "89450421180216211234",
		.fw = "mfw_nrf9160_1.2.3",
		.brdv = "nrf9160dk_nrf9160",
		.appv = "v1.0.0-development",
		.ts = 1000,
		.queued = true,
	};
	struct cloud_data_sensors sensor = {
		.humidity = 50,
		.temperature = 23,
		.pressure = 80,
		.bsec_air_quality = 50,
		.env_ts = 1000,
		.queued = true,
	};
	struct cloud_data_impact impact = {
		.magnitude = 300.0,
		.ts = 1000,
		.queued = true,
	};
	double alt, acc, spd;

	alt = (double)gnss.pvt.alt;
	acc = (double)gnss.pvt.acc;
	spd = (double)gnss.pvt.spd;

	__wrap_date_time_uptime_to_unix_time_ms_ExpectAndReturn(&gnss.gnss_ts, 0);
	__wrap_date_time_uptime_to_unix_time_ms_ExpectAndReturn(&sensor.env_ts, 0);
	__wrap_date_time_now_ExpectAndReturn(&current_time, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, AVAIL_NETWORK_BEARER_ID, 0),
		&bearers[0], sizeof(bearers[0]), sizeof(bearers[0]), LWM2M_RES_DATA_FLAG_RO, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, AVAIL_NETWORK_BEARER_ID, 1),
		&bearers[1], sizeof(bearers[1]), sizeof(bearers[1]), LWM2M_RES_DATA_FLAG_RO, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, IP_ADDRESSES, 0),
		modem_dynamic.ip, sizeof(modem_dynamic.ip), sizeof(modem_dynamic.ip),
		LWM2M_RES_DATA_FLAG_RO, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, APN, 0), modem_dynamic.apn,
		sizeof(modem_dynamic.apn), sizeof(modem_dynamic.apn), LWM2M_RES_DATA_FLAG_RO, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, MODEL_NUMBER_RID), CONFIG_BOARD,
		sizeof(CONFIG_BOARD), sizeof(CONFIG_BOARD), LWM2M_RES_DATA_FLAG_RO, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, HARDWARE_VERSION_RID), CONFIG_SOC,
		sizeof(CONFIG_SOC), sizeof(CONFIG_SOC), LWM2M_RES_DATA_FLAG_RO, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, MANUFACTURER_RID),
		CONFIG_CLOUD_CODEC_MANUFACTURER, sizeof(CONFIG_CLOUD_CODEC_MANUFACTURER),
		sizeof(CONFIG_CLOUD_CODEC_MANUFACTURER), LWM2M_RES_DATA_FLAG_RO, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, FIRMWARE_VERSION_RID), modem_static.appv,
		sizeof(modem_static.appv), sizeof(modem_static.appv), LWM2M_RES_DATA_FLAG_RO, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, SOFTWARE_VERSION_RID), modem_static.fw,
		sizeof(modem_static.fw), sizeof(modem_static.fw), LWM2M_RES_DATA_FLAG_RO, 0);

	__wrap_lwm2m_engine_set_res_buf_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, DEVICE_SERIAL_NUMBER_ID), modem_static.imei,
		sizeof(modem_static.imei), sizeof(modem_static.imei), LWM2M_RES_DATA_FLAG_RO, 0);

	__wrap_lwm2m_engine_set_float_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0, LATITUDE_RID), &gnss.pvt.lat, 0);

	__wrap_lwm2m_engine_set_float_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0, LONGITUDE_RID), &gnss.pvt.longi, 0);

	__wrap_lwm2m_engine_set_float_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0, ALTITUDE_RID), &alt, 0);

	__wrap_lwm2m_engine_set_float_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0, RADIUS_RID), &acc, 0);

	__wrap_lwm2m_engine_set_float_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0, SPEED_RID), &spd, 0);

	__wrap_lwm2m_engine_set_float_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_VALUE_RID),
		&sensor.temperature, 0);

	__wrap_lwm2m_engine_set_float_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, SENSOR_VALUE_RID),
		&sensor.humidity, 0);

	__wrap_lwm2m_engine_set_float_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_VALUE_RID),
		&sensor.pressure, 0);

	__wrap_lwm2m_engine_set_s32_ExpectAndReturn(LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID, 0,
							       LOCATION_TIMESTAMP_RID),
						    (int32_t)(gnss.gnss_ts / MSEC_PER_SEC), 0);

	__wrap_lwm2m_engine_set_s32_ExpectAndReturn(LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0,
							       CURRENT_TIME_RID),
						    (int32_t)(current_time / MSEC_PER_SEC), 0);

	__wrap_lwm2m_engine_set_s32_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_VOLTAGE_RID),
		battery.bat, 0);

	__wrap_lwm2m_engine_set_s32_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0, TIMESTAMP_RID),
		(int32_t)(sensor.env_ts / MSEC_PER_SEC), 0);

	__wrap_lwm2m_engine_set_s32_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, TIMESTAMP_RID),
		(int32_t)(sensor.env_ts / MSEC_PER_SEC), 0);

	__wrap_lwm2m_engine_set_s32_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, TIMESTAMP_RID),
		(int32_t)(sensor.env_ts / MSEC_PER_SEC), 0);

	__wrap_lwm2m_engine_set_u32_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, CELLID),
		modem_dynamic.cell, 0);

	__wrap_lwm2m_engine_set_u8_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, NETWORK_BEARER_ID),
		NB_IOT_BEARER, 0);

	__wrap_lwm2m_engine_set_s8_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, RADIO_SIGNAL_STRENGTH),
		modem_dynamic.rsrp, 0);

	__wrap_lwm2m_engine_set_u16_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMNC), modem_dynamic.mnc, 0);

	__wrap_lwm2m_engine_set_u16_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, SMCC), modem_dynamic.mcc, 0);

	__wrap_lwm2m_engine_set_u16_ExpectAndReturn(
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID, 0, LAC), modem_dynamic.area, 0);

	const char *const path_list[] = {
		LWM2M_PATH(LWM2M_OBJECT_LOCATION_ID),
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, CURRENT_TIME_RID),
		LWM2M_PATH(LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID),
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, MANUFACTURER_RID),
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, MODEL_NUMBER_RID),
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, SOFTWARE_VERSION_RID),
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, FIRMWARE_VERSION_RID),
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, DEVICE_SERIAL_NUMBER_ID),
		LWM2M_PATH(LWM2M_OBJECT_DEVICE_ID, 0, POWER_SOURCE_VOLTAGE_RID),
		LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0, TIMESTAMP_RID),
		LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, TIMESTAMP_RID),
		LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, TIMESTAMP_RID),
		LWM2M_PATH(IPSO_OBJECT_TEMP_SENSOR_ID, 0, SENSOR_VALUE_RID),
		LWM2M_PATH(IPSO_OBJECT_HUMIDITY_SENSOR_ID, 0, SENSOR_VALUE_RID),
		LWM2M_PATH(IPSO_OBJECT_PRESSURE_ID, 0, SENSOR_VALUE_RID),
	};

	/* Accelerometer and user_interface data is not supported by the lwm2m codec backend
	 * when calling cloud_codec_encode_data(). Therefore they are set to NULL.
	 */
	TEST_ASSERT_EQUAL(0, cloud_codec_encode_data(&codec, &gnss, &sensor, &modem_static,
						     &modem_dynamic, NULL,
						     &impact, &battery));

	TEST_ASSERT_EQUAL(ARRAY_SIZE(path_list), codec.valid_object_paths);

	for (int i = 0; i < ARRAY_SIZE(path_list); i++) {
		TEST_ASSERT_EQUAL(0, strcmp(path_list[i], codec.paths[i]));
	}
}

void test_lwm2m_codec_encode_ui_data(void)
{
	struct cloud_codec_data codec = { 0 };
	struct cloud_data_ui user_interface = {
		.btn = 1,
		.btn_ts = 1000,
		.queued = true
	};

	__wrap_date_time_uptime_to_unix_time_ms_ExpectAndReturn(&user_interface.btn_ts, 0);

	__wrap_lwm2m_engine_set_bool_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID, 0, DIGITAL_INPUT_STATE_RID), true, 0);

	__wrap_lwm2m_engine_set_bool_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID, 0, DIGITAL_INPUT_STATE_RID), false, 0);

	__wrap_lwm2m_engine_set_s32_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID, 0, TIMESTAMP_RID),
		(int32_t)(user_interface.btn_ts / MSEC_PER_SEC), 0);

	const char *const path_list[] = {
		LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID),
	};

	TEST_ASSERT_EQUAL(0, cloud_codec_encode_ui_data(&codec, &user_interface));
	TEST_ASSERT_EQUAL(ARRAY_SIZE(path_list), codec.valid_object_paths);

	for (int i = 0; i < ARRAY_SIZE(path_list); i++) {
		TEST_ASSERT_EQUAL(0, strcmp(path_list[i], codec.paths[i]));
	}

	/* Clear codec output structure. */
	memset(&codec, 0, sizeof(struct cloud_codec_data));

	/* Button 2. */
	user_interface.btn = 2;
	user_interface.btn_ts = 1000;
	user_interface.queued = true;

	__wrap_date_time_uptime_to_unix_time_ms_ExpectAndReturn(&user_interface.btn_ts, 0);

	/* Button 2 corresponds to object instance 1. */
	__wrap_lwm2m_engine_set_bool_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID, 1, DIGITAL_INPUT_STATE_RID), true, 0);

	__wrap_lwm2m_engine_set_bool_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID, 1, DIGITAL_INPUT_STATE_RID), false, 0);

	__wrap_lwm2m_engine_set_s32_ExpectAndReturn(
		LWM2M_PATH(IPSO_OBJECT_PUSH_BUTTON_ID, 1, TIMESTAMP_RID),
		(int32_t)(user_interface.btn_ts / MSEC_PER_SEC), 0);

	TEST_ASSERT_EQUAL(0, cloud_codec_encode_ui_data(&codec, &user_interface));
	TEST_ASSERT_EQUAL(ARRAY_SIZE(path_list), codec.valid_object_paths);

	for (int i = 0; i < ARRAY_SIZE(path_list); i++) {
		TEST_ASSERT_EQUAL(0, strcmp(path_list[i], codec.paths[i]));
	}
}

void test_lwm2m_codec_config_update(void)
{
	struct cloud_data_cfg cfg = {
		.gnss_timeout = GNSS_TIMEOUT,
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

	/* Whenever the configuration object is written to, the uut will get all configuration
	 * parameter values (resources). The configuration values will be included in the
	 * CLOUD_CODEC_EVT_CONFIG_UPDATE event received in the cloud_codec_event_handler.
	 */
	__wrap_lwm2m_engine_get_s32_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, GNSS_TIMEOUT_RID), 0, 0);
	__wrap_lwm2m_engine_get_s32_IgnoreArg_value();
	__wrap_lwm2m_engine_get_s32_ReturnThruPtr_value(&cfg.gnss_timeout);

	__wrap_lwm2m_engine_get_s32_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, ACTIVE_WAIT_TIMEOUT_RID), 0, 0);
	__wrap_lwm2m_engine_get_s32_IgnoreArg_value();
	__wrap_lwm2m_engine_get_s32_ReturnThruPtr_value(&cfg.active_wait_timeout);

	__wrap_lwm2m_engine_get_s32_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_RESOLUTION_RID), 0, 0);
	__wrap_lwm2m_engine_get_s32_IgnoreArg_value();
	__wrap_lwm2m_engine_get_s32_ReturnThruPtr_value(&cfg.movement_resolution);

	__wrap_lwm2m_engine_get_s32_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, MOVEMENT_TIMEOUT_RID), 0, 0);
	__wrap_lwm2m_engine_get_s32_IgnoreArg_value();
	__wrap_lwm2m_engine_get_s32_ReturnThruPtr_value(&cfg.movement_timeout);

	__wrap_lwm2m_engine_get_float_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, ACCELEROMETER_ACT_THRESHOLD_RID), 0, 0);
	__wrap_lwm2m_engine_get_float_IgnoreArg_buf();
	__wrap_lwm2m_engine_get_float_ReturnThruPtr_buf(&cfg.accelerometer_activity_threshold);

	__wrap_lwm2m_engine_get_float_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, ACCELEROMETER_INACT_THRESHOLD_RID), 0, 0);
	__wrap_lwm2m_engine_get_float_IgnoreArg_buf();
	__wrap_lwm2m_engine_get_float_ReturnThruPtr_buf(&cfg.accelerometer_inactivity_threshold);

	__wrap_lwm2m_engine_get_float_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, ACCELEROMETER_INACT_TIMEOUT_RID), 0, 0);
	__wrap_lwm2m_engine_get_float_IgnoreArg_buf();
	__wrap_lwm2m_engine_get_float_ReturnThruPtr_buf(&cfg.accelerometer_inactivity_timeout);

	__wrap_lwm2m_engine_get_bool_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, PASSIVE_MODE_RID), 0, 0);
	__wrap_lwm2m_engine_get_bool_IgnoreArg_value();
	__wrap_lwm2m_engine_get_bool_ReturnThruPtr_value(&passive_mode_temp);

	__wrap_lwm2m_engine_get_bool_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, GNSS_ENABLE_RID), 0, 0);
	__wrap_lwm2m_engine_get_bool_IgnoreArg_value();
	__wrap_lwm2m_engine_get_bool_ReturnThruPtr_value(&gnss_enable_temp);

	__wrap_lwm2m_engine_get_bool_ExpectAndReturn(
		LWM2M_PATH(CONFIGURATION_OBJECT_ID, 0, NEIGHBOR_CELL_ENABLE_RID), 0, 0);
	__wrap_lwm2m_engine_get_bool_IgnoreArg_value();
	__wrap_lwm2m_engine_get_bool_ReturnThruPtr_value(&ncell_enable_temp);

	/* Trigger a configuration update. */
	post_write_cb(0, 0, 0, NULL, 0, false, 0);
}

/* Functions that are not supported. */
void test_lwm2m_codec_non_supported_funcs(void)
{
	TEST_ASSERT_EQUAL(-ENOTSUP, cloud_codec_decode_config(NULL, 0, NULL));
	TEST_ASSERT_EQUAL(-ENOTSUP, cloud_codec_encode_config(NULL, NULL));
	TEST_ASSERT_EQUAL(-ENOTSUP, cloud_codec_encode_pgps_request(NULL, NULL));
	TEST_ASSERT_EQUAL(-ENOTSUP, cloud_codec_encode_batch_data(
						NULL, NULL, NULL, NULL, NULL, NULL, NULL,
						NULL, 0, 0, 0, 0, 0, 0, 0));
}

void main(void)
{
	(void)unity_main();
}
