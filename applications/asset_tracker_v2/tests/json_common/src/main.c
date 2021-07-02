/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <cJSON.h>
#include <cJSON_os.h>

#include "json_helpers.h"
#include "json_common.h"
#include "cloud_codec.h"
#include "json_protocol_names.h"
#include "json_validate.h"

/* Structure used to generate cJSON objects and encoded output string buffers. */
static struct test_dummy {
	cJSON *root_obj;
	cJSON *array_obj;
	char *buffer;
} dummy;

/* Function used to check the return value from the encoding functions in JSON common API and the
 * encoded output.
 */
static int encoded_output_check(cJSON *object, char *validation_string, int8_t queued)
{
	dummy.buffer = cJSON_PrintUnformatted(object);
	if (dummy.buffer == NULL) {
		/* Buffer should not be NULL. */
		return -1;
	}

	if (strcmp(validation_string, dummy.buffer) != 0) {
		/* Dummy buffer should equal the validation string. */
		return -2;
	}

	if (queued == -1) {
		/* If queued flag is set to -1, the flag will not be checked. */
	} else if (queued == true) {
		/* Queued flag should be set to false. */
		return -3;
	}

	return 0;
}

/* Battery */

static void test_encode_battery_data_object(void)
{
	int ret;
	struct cloud_data_battery data = {
		.bat = 3600,
		.bat_ts = 1000,
		.queued = true
	};

	ret = json_common_battery_data_add(dummy.root_obj,
					   &data,
					   JSON_COMMON_ADD_DATA_TO_OBJECT,
					   DATA_BATTERY,
					   NULL);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_BATTERY_JSON_SCHEMA, data.queued);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_battery_data_add(dummy.root_obj,
					   &data,
					   JSON_COMMON_ADD_DATA_TO_OBJECT,
					   "",
					   NULL);
	zassert_equal(-ENODATA, ret, "Return value %d is wrong.", ret);

	data.queued = true;

	ret = json_common_battery_data_add(NULL,
					   &data,
					   JSON_COMMON_ADD_DATA_TO_OBJECT,
					   "",
					   NULL);
	zassert_equal(-EINVAL, ret, "Return value %d is wrong.", ret);

	ret = json_common_battery_data_add(dummy.root_obj,
					   &data,
					   JSON_COMMON_ADD_DATA_TO_OBJECT,
					   NULL,
					   NULL);
	zassert_equal(-EINVAL, ret, "Return value %d is wrong.", ret);
}

static void test_encode_battery_data_array(void)
{
	int ret;
	struct cloud_data_battery data = {
		.bat = 3600,
		.bat_ts = 1000,
		.queued = true
	};

	ret = json_common_battery_data_add(dummy.array_obj,
					   &data,
					   JSON_COMMON_ADD_DATA_TO_ARRAY,
					   NULL,
					   NULL);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = encoded_output_check(dummy.array_obj, TEST_VALIDATE_ARRAY_BATTERY_JSON_SCHEMA,
				   data.queued);
	zassert_equal(0, ret, "Return value %d is wrong", ret);
}

/* GPS */

static void test_encode_gps_data_object(void)
{
	int ret;
	/* Avoid using high precision floating point values to ease string comparison post
	 * encoding.
	 */
	struct cloud_data_gps data = {
		.pvt.longi = 10,
		.pvt.lat = 62,
		.pvt.acc = 24,
		.pvt.alt = 170,
		.pvt.spd = 1,
		.pvt.hdg = 176,
		.queued = true,
		.gps_ts = 1000,
		.format = CLOUD_CODEC_GPS_FORMAT_PVT
	};

	ret = json_common_gps_data_add(dummy.root_obj,
				       &data,
				       JSON_COMMON_ADD_DATA_TO_OBJECT,
				       DATA_GPS,
				       NULL);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_GPS_JSON_SCHEMA, data.queued);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_gps_data_add(dummy.root_obj,
				       &data,
				       JSON_COMMON_ADD_DATA_TO_OBJECT,
				       "",
				       NULL);
	zassert_equal(-ENODATA, ret, "Return value %d is wrong.", ret);

	data.queued = true;

	ret = json_common_gps_data_add(NULL,
				       &data,
				       JSON_COMMON_ADD_DATA_TO_OBJECT,
				       "",
				       NULL);
	zassert_equal(-EINVAL, ret, "Return value %d is wrong.", ret);

	ret = json_common_gps_data_add(dummy.root_obj,
				       &data,
				       JSON_COMMON_ADD_DATA_TO_OBJECT,
				       NULL,
				       NULL);
	zassert_equal(-EINVAL, ret, "Return value %d is wrong.", ret);
}

static void test_encode_gps_data_array(void)
{
	int ret;
	/* Avoid using high precision floating point values to ease string comparison post
	 * encoding.
	 */
	struct cloud_data_gps data = {
		.pvt.longi = 10,
		.pvt.lat = 62,
		.pvt.acc = 24,
		.pvt.alt = 170,
		.pvt.spd = 1,
		.pvt.hdg = 176,
		.queued = true,
		.gps_ts = 1000,
		.format = CLOUD_CODEC_GPS_FORMAT_PVT
	};

	ret = json_common_gps_data_add(dummy.array_obj,
				       &data,
				       JSON_COMMON_ADD_DATA_TO_ARRAY,
				       NULL,
				       NULL);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = encoded_output_check(dummy.array_obj, TEST_VALIDATE_ARRAY_GPS_JSON_SCHEMA,
				   data.queued);
	zassert_equal(0, ret, "Return value %d is wrong", ret);
}

/* Environmental */

static void test_encode_environmental_data_object(void)
{
	int ret;
	/* Avoid using high precision floating point values to ease string comparison post
	 * encoding.
	 */
	struct cloud_data_sensors data = {
		.hum = 50,
		.temp = 23,
		.env_ts = 1000,
		.queued = true
	};

	ret = json_common_sensor_data_add(dummy.root_obj,
					&data,
					JSON_COMMON_ADD_DATA_TO_OBJECT,
					DATA_ENVIRONMENTALS,
					NULL);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_ENVIRONMENTAL_JSON_SCHEMA,
				   data.queued);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_sensor_data_add(dummy.root_obj,
					  &data,
					  JSON_COMMON_ADD_DATA_TO_OBJECT,
					  "",
					  NULL);
	zassert_equal(-ENODATA, ret, "Return value %d is wrong.", ret);

	data.queued = true;

	ret = json_common_sensor_data_add(NULL,
					  &data,
					  JSON_COMMON_ADD_DATA_TO_OBJECT,
					  "",
					  NULL);
	zassert_equal(-EINVAL, ret, "Return value %d is wrong.", ret);

	ret = json_common_sensor_data_add(dummy.root_obj,
					  &data,
					  JSON_COMMON_ADD_DATA_TO_OBJECT,
					  NULL,
					  NULL);
	zassert_equal(-EINVAL, ret, "Return value %d is wrong.", ret);
}

static void test_encode_environmental_data_array(void)
{
	int ret;
	/* Avoid using high precision floating point values to ease string comparison post
	 * encoding.
	 */
	struct cloud_data_sensors data = {
		.hum = 50,
		.temp = 23,
		.env_ts = 1000,
		.queued = true
	};

	ret = json_common_sensor_data_add(dummy.array_obj,
					  &data,
					  JSON_COMMON_ADD_DATA_TO_ARRAY,
					  NULL,
					  NULL);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = encoded_output_check(dummy.array_obj, TEST_VALIDATE_ARRAY_ENVIRONMENTAL_JSON_SCHEMA,
				   data.queued);
	zassert_equal(0, ret, "Return value %d is wrong", ret);
}

/* Modem dynamic */

static void test_encode_modem_dynamic_data_object(void)
{
	int ret;
	struct cloud_data_modem_dynamic data = {
		.rsrp = -8,
		.area = 12,
		.mccmnc = "24202",
		.cell = 33703719,
		.ip = "10.81.183.99",
		.ts = 1000,
		.queued = true,
		.area_code_fresh = true,
		.cell_id_fresh = true,
		.rsrp_fresh = true,
		.ip_address_fresh = true,
		.mccmnc_fresh = true
	};

	ret = json_common_modem_dynamic_data_add(dummy.root_obj,
						 &data,
						 JSON_COMMON_ADD_DATA_TO_OBJECT,
						 DATA_MODEM_DYNAMIC,
						 NULL);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_MODEM_DYNAMIC_JSON_SCHEMA,
				   data.queued);
	zassert_equal(0, ret, "Return value %d is wrong, ret");

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_modem_dynamic_data_add(dummy.root_obj,
						 &data,
						 JSON_COMMON_ADD_DATA_TO_OBJECT,
						 "",
						 NULL);
	zassert_equal(-ENODATA, ret, "Return value %d is wrong.", ret);

	data.queued = true;

	ret = json_common_modem_dynamic_data_add(NULL,
						 &data,
						 JSON_COMMON_ADD_DATA_TO_OBJECT,
						 "",
						 NULL);
	zassert_equal(-EINVAL, ret, "Return value %d is wrong.", ret);

	ret = json_common_modem_dynamic_data_add(dummy.root_obj,
						 &data,
						 JSON_COMMON_ADD_DATA_TO_OBJECT,
						 NULL,
						 NULL);
	zassert_equal(-EINVAL, ret, "Return value %d is wrong.", ret);
}

static void test_encode_modem_dynamic_data_array(void)
{
	int ret;
	struct cloud_data_modem_dynamic data = {
		.rsrp = -8,
		.area = 12,
		.mccmnc = "24202",
		.cell = 33703719,
		.ip = "10.81.183.99",
		.ts = 1000,
		.queued = true,
		.area_code_fresh = true,
		.cell_id_fresh = true,
		.rsrp_fresh = true,
		.ip_address_fresh = true,
		.mccmnc_fresh = true
	};

	ret = json_common_modem_dynamic_data_add(dummy.array_obj,
						 &data,
						 JSON_COMMON_ADD_DATA_TO_ARRAY,
						 NULL,
						 NULL);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = encoded_output_check(dummy.array_obj, TEST_VALIDATE_ARRAY_MODEM_DYNAMIC_JSON_SCHEMA,
				   data.queued);
	zassert_equal(0, ret, "Return value %d is wrong", ret);
}

/* Modem static */

static void test_encode_modem_static_data_object(void)
{
	int ret;
	struct cloud_data_modem_static data = {
		.bnd = 3,
		.nw_nb_iot = 1,
		.nw_gps = 1,
		.iccid = "89450421180216216095",
		.fw = "mfw_nrf9160_1.2.3",
		.brdv = "nrf9160dk_nrf9160",
		.appv = "v1.0.0-development",
		.ts = 1000,
		.queued = true
	};

	ret = json_common_modem_static_data_add(dummy.root_obj,
						&data,
						JSON_COMMON_ADD_DATA_TO_OBJECT,
						DATA_MODEM_STATIC,
						NULL);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_MODEM_STATIC_JSON_SCHEMA,
				   data.queued);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_modem_static_data_add(dummy.root_obj,
						&data,
						JSON_COMMON_ADD_DATA_TO_OBJECT,
						"",
						NULL);
	zassert_equal(-ENODATA, ret, "Return value %d is wrong.", ret);

	data.queued = true;

	ret = json_common_modem_static_data_add(NULL,
						&data,
						JSON_COMMON_ADD_DATA_TO_OBJECT,
						"",
						NULL);
	zassert_equal(-EINVAL, ret, "Return value %d is wrong.", ret);

	ret = json_common_modem_static_data_add(dummy.root_obj,
						&data,
						JSON_COMMON_ADD_DATA_TO_OBJECT,
						NULL,
						NULL);
	zassert_equal(-EINVAL, ret, "Return value %d is wrong.", ret);
}

static void test_encode_modem_static_data_array(void)
{
	int ret;
	struct cloud_data_modem_static data = {
		.bnd = 3,
		.nw_nb_iot = 1,
		.nw_gps = 1,
		.iccid = "89450421180216216095",
		.fw = "mfw_nrf9160_1.2.3",
		.brdv = "nrf9160dk_nrf9160",
		.appv = "v1.0.0-development",
		.ts = 1000,
		.queued = true
	};

	ret = json_common_modem_static_data_add(dummy.array_obj,
						&data,
						JSON_COMMON_ADD_DATA_TO_ARRAY,
						NULL,
						NULL);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = encoded_output_check(dummy.array_obj, TEST_VALIDATE_ARRAY_MODEM_STATIC_JSON_SCHEMA,
				   data.queued);
	zassert_equal(0, ret, "Return value %d is wrong", ret);
}

/* UI */

static void test_encode_ui_data_object(void)
{
	int ret;
	struct cloud_data_ui data = {
		.btn = 1,
		.btn_ts = 1000,
		.queued = true
	};

	ret = json_common_ui_data_add(dummy.root_obj,
				      &data,
				      JSON_COMMON_ADD_DATA_TO_OBJECT,
				      DATA_BUTTON,
				      NULL);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_UI_JSON_SCHEMA, data.queued);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_ui_data_add(dummy.root_obj,
				      &data,
				      JSON_COMMON_ADD_DATA_TO_OBJECT,
				      "",
				      NULL);
	zassert_equal(-ENODATA, ret, "Return value %d is wrong.", ret);

	data.queued = true;

	ret = json_common_ui_data_add(NULL,
				      &data,
				      JSON_COMMON_ADD_DATA_TO_OBJECT,
				      "",
				      NULL);
	zassert_equal(-EINVAL, ret, "Return value %d is wrong.", ret);

	ret = json_common_ui_data_add(dummy.root_obj,
				      &data,
				      JSON_COMMON_ADD_DATA_TO_OBJECT,
				      NULL,
				      NULL);
	zassert_equal(-EINVAL, ret, "Return value %d is wrong.", ret);
}

/* Neighbor cell */

static void test_encode_neighbor_cells_data_object(void)
{
	int ret;
	struct cloud_data_neighbor_cells data = {
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

	ret = json_common_neighbor_cells_data_add(dummy.root_obj,
						  &data,
						  JSON_COMMON_ADD_DATA_TO_OBJECT);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_NEIGHBOR_CELLS_JSON_SCHEMA,
				   data.queued);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_neighbor_cells_data_add(dummy.root_obj,
					     &data,
					     JSON_COMMON_ADD_DATA_TO_OBJECT);
	zassert_equal(-ENODATA, ret, "Return value %d is wrong.", ret);
}

static void test_encode_ui_data_array(void)
{
	int ret;
	struct cloud_data_ui data = {
		.btn = 1,
		.btn_ts = 1000,
		.queued = true
	};

	ret = json_common_ui_data_add(dummy.array_obj,
				      &data,
				      JSON_COMMON_ADD_DATA_TO_ARRAY,
				      NULL,
				      NULL);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = encoded_output_check(dummy.array_obj, TEST_VALIDATE_ARRAY_UI_JSON_SCHEMA,
				   data.queued);
	zassert_equal(0, ret, "Return value %d is wrong", ret);
}

/* Accelerometer */

static void test_encode_accelerometer_data_object(void)
{
	int ret;
	struct cloud_data_accelerometer data = {
		.values[0] = 1,
		.values[1] = 2,
		.values[2] = 3,
		.ts = 1000,
		.queued = true
	};

	ret = json_common_accel_data_add(dummy.root_obj,
					 &data,
					 JSON_COMMON_ADD_DATA_TO_OBJECT,
					 DATA_MOVEMENT,
					 NULL);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_ACCELEROMETER_JSON_SCHEMA,
				   data.queued);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_accel_data_add(dummy.root_obj,
					 &data,
					 JSON_COMMON_ADD_DATA_TO_OBJECT,
					 "",
					 NULL);
	zassert_equal(-ENODATA, ret, "Return value %d is wrong.", ret);

	data.queued = true;

	ret = json_common_accel_data_add(NULL,
					 &data,
					 JSON_COMMON_ADD_DATA_TO_OBJECT,
					 "",
					 NULL);
	zassert_equal(-EINVAL, ret, "Return value %d is wrong.", ret);

	ret = json_common_accel_data_add(dummy.root_obj,
					 &data,
					 JSON_COMMON_ADD_DATA_TO_OBJECT,
					 NULL,
					 NULL);
	zassert_equal(-EINVAL, ret, "Return value %d is wrong.", ret);
}

static void test_encode_accelerometer_data_array(void)
{
	int ret;
	struct cloud_data_accelerometer data = {
		.values[0] = 1,
		.values[1] = 2,
		.values[2] = 3,
		.ts = 1000,
		.queued = true
	};

	ret = json_common_accel_data_add(dummy.array_obj,
					 &data,
					 JSON_COMMON_ADD_DATA_TO_ARRAY,
					 NULL,
					 NULL);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = encoded_output_check(dummy.array_obj, TEST_VALIDATE_ARRAY_ACCELEROMETER_JSON_SCHEMA,
				   data.queued);
	zassert_equal(0, ret, "Return value %d is wrong", ret);
}

/* Configuration encode */

static void test_encode_configuration_data_object(void)
{
	int ret;
	struct cloud_data_cfg data = {
		.active_mode = false,
		.active_wait_timeout = 120,
		.movement_resolution = 120,
		.movement_timeout = 3600,
		.gps_timeout = 60,
		.accelerometer_threshold = 2,
		.no_data.gnss = true,
		.no_data.neighbor_cell = true,
		.active_mode_fresh = true,
		.gps_timeout_fresh = true,
		.active_wait_timeout_fresh = true,
		.movement_resolution_fresh = true,
		.movement_timeout_fresh = true,
		.accelerometer_threshold_fresh = true,
		.nod_list_fresh = true,
	};

	ret = json_common_config_add(dummy.root_obj, &data, DATA_CONFIG);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_CONFIGURATION_JSON_SCHEMA, -1);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	/* Check for invalid input. */

	ret = json_common_config_add(dummy.root_obj, &data, NULL);
	zassert_equal(-EINVAL, ret, "Return value %d is wrong.", ret);
}

/* Configuration decode */

static void test_decode_configuration_data(void)
{
	cJSON *root_obj = NULL;
	cJSON *sub_group_obj = NULL;
	struct cloud_data_cfg data = {0};

	root_obj = cJSON_Parse(TEST_VALIDATE_CONFIGURATION_JSON_SCHEMA);
	zassert_not_null(root_obj, "Root object is NULL");

	sub_group_obj = json_object_decode(root_obj, OBJECT_CONFIG);
	zassert_not_null(sub_group_obj, "Sub group object is NULL");

	json_common_config_get(sub_group_obj, &data);

	zassert_equal(false, data.active_mode, "Configuration is wrong");
	zassert_equal(true, data.no_data.gnss, "Configuration is wrong");
	zassert_equal(true, data.no_data.neighbor_cell, "Configuration is wrong");
	zassert_equal(60, data.gps_timeout, "Configuration is wrong");
	zassert_equal(120, data.active_wait_timeout, "Configuration is wrong");
	zassert_equal(120, data.movement_resolution, "Configuration is wrong");
	zassert_equal(3600, data.movement_timeout, "Configuration is wrong");
	zassert_equal(2, data.accelerometer_threshold, "Configuration is wrong");

	cJSON_Delete(root_obj);
}

/* Batch data */

static void test_encode_batch_data_object(void)
{
	int ret;
	struct cloud_data_battery battery[2] = {
		[0].bat = 3600,
		[0].bat_ts = 1000,
		[0].queued = true,
		/* Second entry */
		[1].bat = 3600,
		[1].bat_ts = 1000,
		[1].queued = true
	};
	struct cloud_data_gps gps[2] = {
		[0].pvt.longi = 10,
		[0].pvt.lat = 62,
		[0].pvt.acc = 24,
		[0].pvt.alt = 170,
		[0].pvt.spd = 1,
		[0].pvt.hdg = 176,
		[0].gps_ts = 1000,
		[0].queued = true,
		[0].format = CLOUD_CODEC_GPS_FORMAT_PVT,
		/* Second entry */
		[1].pvt.longi = 10,
		[1].pvt.lat = 62,
		[1].pvt.acc = 24,
		[1].pvt.alt = 170,
		[1].pvt.spd = 1,
		[1].pvt.hdg = 176,
		[1].gps_ts = 1000,
		[1].queued = true,
		[1].format = CLOUD_CODEC_GPS_FORMAT_PVT
	};
	struct cloud_data_modem_dynamic modem_dynamic[2] = {
		[0].rsrp = -8,
		[0].area = 12,
		[0].mccmnc = "24202",
		[0].cell = 33703719,
		[0].ip = "10.81.183.99",
		[0].ts = 1000,
		[0].queued = true,
		[0].area_code_fresh = true,
		[0].cell_id_fresh = true,
		[0].rsrp_fresh = true,
		[0].ip_address_fresh = true,
		[0].mccmnc_fresh = true,
		/* Second entry */
		[1].rsrp = -5,
		[1].area = 12,
		[1].mccmnc = "24202",
		[1].cell = 33703719,
		[1].ip = "10.81.183.99",
		[1].ts = 1000,
		[1].queued = true,
		[1].area_code_fresh = true,
		[1].cell_id_fresh = true,
		[1].rsrp_fresh = true,
		[1].ip_address_fresh = true,
		[1].mccmnc_fresh = true,
	};
	struct cloud_data_modem_static modem_static[2] = {
		[0].bnd = 3,
		[0].nw_nb_iot = 1,
		[0].nw_gps = 1,
		[0].iccid = "89450421180216216095",
		[0].fw = "mfw_nrf9160_1.2.3",
		[0].brdv = "nrf9160dk_nrf9160",
		[0].appv = "v1.0.0-development",
		[0].ts = 1000,
		[0].queued = true,
		/* Second entry */
		[1].bnd = 3,
		[1].nw_nb_iot = 1,
		[1].nw_gps = 1,
		[1].iccid = "89450421180216216095",
		[1].fw = "mfw_nrf9160_1.2.3",
		[1].brdv = "nrf9160dk_nrf9160",
		[1].appv = "v1.0.0-development",
		[1].ts = 1000,
		[1].queued = true
	};
	struct cloud_data_ui ui[2] = {
		[0].btn = 1,
		[0].btn_ts = 1000,
		[0].queued = true,
		/* Second entry */
		[1].btn = 1,
		[1].btn_ts = 1000,
		[1].queued = true
	};
	struct cloud_data_accelerometer accelerometer[2] = {
		[0].values[0] = 1,
		[0].values[1] = 2,
		[0].values[2] = 3,
		[0].ts = 1000,
		[0].queued = true,
		/* Second entry */
		[1].values[0] = 1,
		[1].values[1] = 2,
		[1].values[2] = 3,
		[1].ts = 1000,
		[1].queued = true
	};
	struct cloud_data_sensors environmental[2] = {
		[0].hum = 50,
		[0].temp = 23,
		[0].env_ts = 1000,
		[0].queued = true,
		/* Second entry */
		[1].hum = 50,
		[1].temp = 23,
		[1].env_ts = 1000,
		[1].queued = true
	};

	ret = json_common_batch_data_add(dummy.root_obj,
					 JSON_COMMON_BATTERY,
					 &battery,
					 ARRAY_SIZE(battery),
					 DATA_BATTERY);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = json_common_batch_data_add(dummy.root_obj,
					 JSON_COMMON_UI,
					 &ui,
					 ARRAY_SIZE(ui),
					 DATA_BUTTON);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = json_common_batch_data_add(dummy.root_obj,
					 JSON_COMMON_GPS,
					 &gps,
					 ARRAY_SIZE(gps),
					 DATA_GPS);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = json_common_batch_data_add(dummy.root_obj,
					 JSON_COMMON_SENSOR,
					 &environmental,
					 ARRAY_SIZE(environmental),
					 DATA_ENVIRONMENTALS);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = json_common_batch_data_add(dummy.root_obj,
					 JSON_COMMON_ACCELEROMETER,
					 &accelerometer,
					 ARRAY_SIZE(accelerometer),
					 DATA_MOVEMENT);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = json_common_batch_data_add(dummy.root_obj,
					 JSON_COMMON_MODEM_DYNAMIC,
					 &modem_dynamic,
					 ARRAY_SIZE(modem_dynamic),
					 DATA_MODEM_DYNAMIC);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = json_common_batch_data_add(dummy.root_obj,
					 JSON_COMMON_MODEM_STATIC,
					 &modem_static,
					 ARRAY_SIZE(modem_static),
					 DATA_MODEM_STATIC);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_BATCH_JSON_SCHEMA, -1);
	zassert_equal(0, ret, "Return value %d is wrong", ret);

	/* Check for invalid inputs. */

	ret = json_common_batch_data_add(NULL, -1, NULL, 0, "");
	zassert_equal(-ENOMEM, ret, "Return value %d is wrong.", ret);

	ret = json_common_batch_data_add(dummy.root_obj, -1, NULL, 0, NULL);
	zassert_equal(-EINVAL, ret, "Return value %d is wrong.", ret);
}

/* Test used to verify encoding and decoding of data structures that contain floating point
 * values. Floating point values cannot be exactly represented in binary so they cannot be compared
 * with a predefined JSON string schema.
 */

static void test_floating_point_encoding_gps(void)
{
	int ret;
	cJSON *decoded_root_obj;
	cJSON *decoded_gps_obj;
	cJSON *decoded_value_obj;
	struct cloud_data_gps decoded_values = {0};
	struct cloud_data_gps data = {
		.pvt.longi = 10.417852141870654,
		.pvt.lat = 63.43278762669529,
		.pvt.acc = 15.455987930297852,
		.pvt.alt = 53.67230987548828,
		.pvt.spd = 0.4443884789943695,
		.pvt.hdg = 176.12345298374867,
		.gps_ts = 1000,
		.queued = true,
		.format = CLOUD_CODEC_GPS_FORMAT_PVT
	};

	ret = json_common_gps_data_add(dummy.root_obj,
				       &data,
				       JSON_COMMON_ADD_DATA_TO_OBJECT,
				       DATA_GPS,
				       NULL);
	zassert_equal(0, ret, "Return value %d is wrong", ret);
	zassert_false(data.queued, "Queued flag was not set to false by function %s", __func__);

	dummy.buffer = cJSON_PrintUnformatted(dummy.root_obj);
	zassert_not_null(dummy.buffer, "Printed JSON string is NULL");

	decoded_root_obj = cJSON_Parse(dummy.buffer);
	zassert_not_null(decoded_root_obj, "Root object is NULL");

	decoded_gps_obj = json_object_decode(decoded_root_obj, DATA_GPS);
	zassert_not_null(decoded_gps_obj, "Decoded GPS object is NULL");

	decoded_value_obj = json_object_decode(decoded_gps_obj, DATA_VALUE);
	zassert_not_null(decoded_value_obj, "Decoded value object is NULL");

	cJSON *longitude = cJSON_GetObjectItem(decoded_value_obj, DATA_GPS_LONGITUDE);
	cJSON *latitude = cJSON_GetObjectItem(decoded_value_obj, DATA_GPS_LATITUDE);
	cJSON *accuracy = cJSON_GetObjectItem(decoded_value_obj, DATA_MOVEMENT);
	cJSON *altitude = cJSON_GetObjectItem(decoded_value_obj, DATA_GPS_ALTITUDE);
	cJSON *speed = cJSON_GetObjectItem(decoded_value_obj, DATA_GPS_SPEED);
	cJSON *heading = cJSON_GetObjectItem(decoded_value_obj, DATA_GPS_HEADING);

	zassert_not_null(longitude, "Longitude is NULL");
	zassert_not_null(latitude, "Latitude is NULL");
	zassert_not_null(accuracy, "Accuracy is NULL");
	zassert_not_null(altitude, "Altitude is NULL");
	zassert_not_null(speed, "Speed is NULL");
	zassert_not_null(heading, "Heading is NULL");

	decoded_values.pvt.longi = longitude->valuedouble;
	decoded_values.pvt.lat = latitude->valuedouble;
	decoded_values.pvt.acc = accuracy->valuedouble;
	decoded_values.pvt.alt = altitude->valuedouble;
	decoded_values.pvt.spd = speed->valuedouble;
	decoded_values.pvt.hdg = heading->valuedouble;

	zassert_within(decoded_values.pvt.longi, data.pvt.longi, 0.1,
		       "Decoded value is not within delta");
	zassert_within(decoded_values.pvt.lat, data.pvt.lat, 0.1,
		       "Decoded value is not within delta");
	zassert_within(decoded_values.pvt.acc, data.pvt.acc, 0.1,
		       "Decoded value is not within delta");
	zassert_within(decoded_values.pvt.alt, data.pvt.alt, 0.1,
		       "Decoded value is not within delta");
	zassert_within(decoded_values.pvt.spd, data.pvt.spd, 0.1,
		       "Decoded value is not within delta");
	zassert_within(decoded_values.pvt.hdg, data.pvt.hdg, 0.1,
		       "Decoded value is not within delta");

	cJSON_Delete(decoded_root_obj);
}

static void test_floating_point_encoding_accelerometer(void)
{
	int ret;
	cJSON *decoded_root_obj;
	cJSON *decoded_acc_obj;
	cJSON *decoded_value_obj;
	struct cloud_data_accelerometer decoded_values = {0};
	struct cloud_data_accelerometer data = {
		.values[0] = 1.49061,
		.values[1] = 0.617818,
		.values[2] = -9.924329,
		.ts = 1000,
		.queued = true
	};

	ret = json_common_accel_data_add(dummy.root_obj,
					 &data,
					 JSON_COMMON_ADD_DATA_TO_OBJECT,
					 DATA_MOVEMENT,
					 NULL);
	zassert_equal(0, ret, "Return value %d is wrong", ret);
	zassert_false(data.queued, "Queued flag was not set to false by function %s", __func__);

	dummy.buffer = cJSON_PrintUnformatted(dummy.root_obj);
	zassert_not_null(dummy.buffer, "Printed JSON string is NULL");

	decoded_root_obj = cJSON_Parse(dummy.buffer);
	zassert_not_null(decoded_root_obj, "Root object is NULL");

	decoded_acc_obj = json_object_decode(decoded_root_obj, DATA_MOVEMENT);
	zassert_not_null(decoded_acc_obj, "Decoded Accelerometer object is NULL");

	decoded_value_obj = json_object_decode(decoded_acc_obj, DATA_VALUE);
	zassert_not_null(decoded_value_obj, "Decoded value object is NULL");

	cJSON *x_axis = cJSON_GetObjectItem(decoded_value_obj, DATA_MOVEMENT_X);
	cJSON *y_axis = cJSON_GetObjectItem(decoded_value_obj, DATA_MOVEMENT_Y);
	cJSON *z_axis = cJSON_GetObjectItem(decoded_value_obj, DATA_MOVEMENT_Z);

	zassert_not_null(x_axis, "X-axis is NULL");
	zassert_not_null(y_axis, "Y-axis is NULL");
	zassert_not_null(z_axis, "Z-axis is NULL");

	decoded_values.values[0] = x_axis->valuedouble;
	decoded_values.values[1] = y_axis->valuedouble;
	decoded_values.values[2] = z_axis->valuedouble;

	zassert_within(decoded_values.values[0], data.values[0], 0.1,
		       "Decoded value is not within delta");
	zassert_within(decoded_values.values[1], data.values[1], 0.1,
		       "Decoded value is not within delta");
	zassert_within(decoded_values.values[2], data.values[2], 0.1,
		       "Decoded value is not within delta");

	cJSON_Delete(decoded_root_obj);
}

static void test_floating_point_encoding_environmental(void)
{
	int ret;
	cJSON *decoded_root_obj;
	cJSON *decoded_env_obj;
	cJSON *decoded_value_obj;
	struct cloud_data_sensors decoded_values = {0};
	struct cloud_data_sensors data = {
		.temp = 26.27,
		.hum = 35.15,
		.env_ts = 1000,
		.queued = true
	};

	ret = json_common_sensor_data_add(dummy.root_obj,
					  &data,
					  JSON_COMMON_ADD_DATA_TO_OBJECT,
					  DATA_ENVIRONMENTALS,
					  NULL);
	zassert_equal(0, ret, "Return value %d is wrong", ret);
	zassert_false(data.queued, "Queued flag was not set to false by function %s", __func__);

	dummy.buffer = cJSON_PrintUnformatted(dummy.root_obj);
	zassert_not_null(dummy.buffer, "Printed JSON string is NULL");

	decoded_root_obj = cJSON_Parse(dummy.buffer);
	zassert_not_null(decoded_root_obj, "Root object is NULL");

	decoded_env_obj = json_object_decode(decoded_root_obj, DATA_ENVIRONMENTALS);
	zassert_not_null(decoded_env_obj, "Decoded Environmental object is NULL");

	decoded_value_obj = json_object_decode(decoded_env_obj, DATA_VALUE);
	zassert_not_null(decoded_value_obj, "Decoded value object is NULL");

	cJSON *temperature = cJSON_GetObjectItem(decoded_value_obj, DATA_TEMPERATURE);
	cJSON *humidity = cJSON_GetObjectItem(decoded_value_obj, DATA_HUMID);

	zassert_not_null(temperature, "Temperature is NULL");
	zassert_not_null(humidity, "Humidity is NULL");

	decoded_values.temp = temperature->valuedouble;
	decoded_values.hum = humidity->valuedouble;

	zassert_within(decoded_values.temp, data.temp, 0.1, "Decoded value is not within delta");
	zassert_within(decoded_values.hum, data.hum, 0.1, "Decoded value is not within delta");

	cJSON_Delete(decoded_root_obj);
}

static void test_floating_point_encoding_configuration(void)
{
	int ret;
	cJSON *decoded_root_obj;
	cJSON *decoded_config_obj;
	struct cloud_data_cfg decoded_values = {0};
	struct cloud_data_cfg data = {
		.accelerometer_threshold = 2.22,
		.accelerometer_threshold_fresh = true
	};

	ret = json_common_config_add(dummy.root_obj, &data, DATA_CONFIG);
	zassert_equal(0, ret, "Return value %d is wrong", ret);
	dummy.buffer = cJSON_PrintUnformatted(dummy.root_obj);

	zassert_not_null(dummy.buffer, "Printed JSON string is NULL");

	decoded_root_obj = cJSON_Parse(dummy.buffer);
	zassert_not_null(decoded_root_obj, "Root object is NULL");

	decoded_config_obj = json_object_decode(decoded_root_obj, DATA_CONFIG);
	zassert_not_null(decoded_config_obj, "Decoded Configruation object is NULL");

	cJSON *accel_thresh = cJSON_GetObjectItem(decoded_config_obj, CONFIG_ACC_THRESHOLD);

	zassert_not_null(accel_thresh, "Accelerometer threshold is NULL");

	decoded_values.accelerometer_threshold = accel_thresh->valuedouble;

	zassert_within(decoded_values.accelerometer_threshold, data.accelerometer_threshold, 0.1,
		       "Decoded value is not within delta");

	cJSON_Delete(decoded_root_obj);
}

/* Setup and teardown functions. Used to allocate root and array objects used in test and to
 * cleanup allocated memory afterwards.
 */

static void test_setup_object(void)
{
	dummy.root_obj = cJSON_CreateObject();
	zassert_not_null(dummy.root_obj, "Root object is NULL");
}

static void test_teardown_object(void)
{
	cJSON_FreeString(dummy.buffer);
	cJSON_Delete(dummy.root_obj);
}

static void test_setup_array(void)
{
	dummy.array_obj = cJSON_CreateArray();
	zassert_not_null(dummy.root_obj, "Root object is NULL");
}

static void test_teardown_array(void)
{
	cJSON_FreeString(dummy.buffer);
	cJSON_Delete(dummy.array_obj);
}

void test_main(void)
{
	cJSON_Init();

	ztest_test_suite(json_common,

		/* Battery */
		ztest_unit_test_setup_teardown(test_encode_battery_data_object,
					       test_setup_object,
					       test_teardown_object),
		ztest_unit_test_setup_teardown(test_encode_battery_data_array,
					       test_setup_array,
					       test_teardown_array),

		/* GPS */
		ztest_unit_test_setup_teardown(test_encode_gps_data_object,
					       test_setup_object,
					       test_teardown_object),
		ztest_unit_test_setup_teardown(test_encode_gps_data_array,
					       test_setup_array,
					       test_teardown_array),

		/* Environmental */
		ztest_unit_test_setup_teardown(test_encode_environmental_data_object,
					       test_setup_object,
					       test_teardown_object),
		ztest_unit_test_setup_teardown(test_encode_environmental_data_array,
					       test_setup_array,
					       test_teardown_array),

		/* Modem dynamic */
		ztest_unit_test_setup_teardown(test_encode_modem_dynamic_data_object,
					       test_setup_object,
					       test_teardown_object),
		ztest_unit_test_setup_teardown(test_encode_modem_dynamic_data_array,
					       test_setup_array,
					       test_teardown_array),

		/* Modem static */
		ztest_unit_test_setup_teardown(test_encode_modem_static_data_object,
					       test_setup_object,
					       test_teardown_object),
		ztest_unit_test_setup_teardown(test_encode_modem_static_data_array,
					       test_setup_array,
					       test_teardown_array),

		/* UI */
		ztest_unit_test_setup_teardown(test_encode_ui_data_object,
					       test_setup_object,
					       test_teardown_object),
		ztest_unit_test_setup_teardown(test_encode_ui_data_array,
					       test_setup_array,
					       test_teardown_array),

		/* Neighbor cell */
		ztest_unit_test_setup_teardown(test_encode_neighbor_cells_data_object,
					       test_setup_object,
					       test_teardown_object),

		/* Accelerometer */
		ztest_unit_test_setup_teardown(test_encode_accelerometer_data_object,
					       test_setup_object,
					       test_teardown_object),
		ztest_unit_test_setup_teardown(test_encode_accelerometer_data_array,
					       test_setup_array,
					       test_teardown_array),

		/* Configuration encode */
		ztest_unit_test_setup_teardown(test_encode_configuration_data_object,
					       test_setup_object,
					       test_teardown_object),

		/* Configuration decode */
		ztest_unit_test(test_decode_configuration_data),

		/* Batch */
		ztest_unit_test_setup_teardown(test_encode_batch_data_object,
					       test_setup_object,
					       test_teardown_object),

		/* GPS floating point values comparison */
		ztest_unit_test_setup_teardown(test_floating_point_encoding_gps,
					       test_setup_object,
					       test_teardown_object),

		/* Accelerometer floating point values comparison */
		ztest_unit_test_setup_teardown(test_floating_point_encoding_accelerometer,
					       test_setup_object,
					       test_teardown_object),

		/* Accelerometer floating point values comparison */
		ztest_unit_test_setup_teardown(test_floating_point_encoding_environmental,
					       test_setup_object,
					       test_teardown_object),

		/* Configuration floating point values comparison */
		ztest_unit_test_setup_teardown(test_floating_point_encoding_configuration,
					       test_setup_object,
					       test_teardown_object)
	);

	ztest_run_test_suite(json_common);
}
