/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>
#include <zephyr/kernel.h>
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

/* The unity_main is not declared in any header file. It is only defined in the generated test
 * runner because of ncs' unity configuration. It is therefore declared here to avoid a compiler
 * warning.
 */
extern int unity_main(void);

/* Setup and teardown functions. Used to allocate root and array objects used in test and to
 * cleanup allocated memory afterwards.
 */

void setUp(void)
{
	dummy.root_obj = cJSON_CreateObject();
	dummy.array_obj = cJSON_CreateArray();
	TEST_ASSERT_NOT_NULL(dummy.root_obj);
	TEST_ASSERT_NOT_NULL(dummy.array_obj);
}

void tearDown(void)
{
	cJSON_Delete(dummy.root_obj);
	cJSON_Delete(dummy.array_obj);
	if (dummy.buffer != NULL) {
		cJSON_FreeString(dummy.buffer);
	}
	memset(&dummy, 0, sizeof(dummy));
}

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
		printk("validation: %s, buffer: %s", validation_string, dummy.buffer);
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

void test_encode_battery_data_object(void)
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
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_BATTERY_JSON_SCHEMA, data.queued);
	TEST_ASSERT_EQUAL(0, ret);

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_battery_data_add(dummy.root_obj,
					   &data,
					   JSON_COMMON_ADD_DATA_TO_OBJECT,
					   "",
					   NULL);
	TEST_ASSERT_EQUAL(-ENODATA, ret);

	data.queued = true;

	ret = json_common_battery_data_add(NULL,
					   &data,
					   JSON_COMMON_ADD_DATA_TO_OBJECT,
					   "",
					   NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = json_common_battery_data_add(dummy.root_obj,
					   &data,
					   JSON_COMMON_ADD_DATA_TO_OBJECT,
					   NULL,
					   NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_encode_battery_data_array(void)
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
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.array_obj, TEST_VALIDATE_ARRAY_BATTERY_JSON_SCHEMA,
				   data.queued);
	TEST_ASSERT_EQUAL(0, ret);
}

/* GNSS */

void test_encode_gnss_data_object(void)
{
	int ret;
	/* Avoid using high precision floating point values to ease string comparison post
	 * encoding.
	 */
	struct cloud_data_gnss data = {
		.pvt.longi = 10,
		.pvt.lat = 62,
		.pvt.acc = 24,
		.pvt.alt = 170,
		.pvt.spd = 1,
		.pvt.hdg = 176,
		.queued = true,
		.gnss_ts = 1000
	};

	ret = json_common_gnss_data_add(dummy.root_obj,
				       &data,
				       JSON_COMMON_ADD_DATA_TO_OBJECT,
				       DATA_GNSS,
				       NULL);
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_GNSS_JSON_SCHEMA, data.queued);
	TEST_ASSERT_EQUAL(0, ret);

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_gnss_data_add(dummy.root_obj,
				       &data,
				       JSON_COMMON_ADD_DATA_TO_OBJECT,
				       "",
				       NULL);
	TEST_ASSERT_EQUAL(-ENODATA, ret);

	data.queued = true;

	ret = json_common_gnss_data_add(NULL,
				       &data,
				       JSON_COMMON_ADD_DATA_TO_OBJECT,
				       "",
				       NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = json_common_gnss_data_add(dummy.root_obj,
				       &data,
				       JSON_COMMON_ADD_DATA_TO_OBJECT,
				       NULL,
				       NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_encode_gnss_data_array(void)
{
	int ret;
	/* Avoid using high precision floating point values to ease string comparison post
	 * encoding.
	 */
	struct cloud_data_gnss data = {
		.pvt.longi = 10,
		.pvt.lat = 62,
		.pvt.acc = 24,
		.pvt.alt = 170,
		.pvt.spd = 1,
		.pvt.hdg = 176,
		.queued = true,
		.gnss_ts = 1000
	};

	ret = json_common_gnss_data_add(dummy.array_obj,
				       &data,
				       JSON_COMMON_ADD_DATA_TO_ARRAY,
				       NULL,
				       NULL);
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.array_obj, TEST_VALIDATE_ARRAY_GNSS_JSON_SCHEMA,
				   data.queued);
	TEST_ASSERT_EQUAL(0, ret);
}

/* Environmental */

void test_encode_environmental_data_object(void)
{
	int ret;
	/* Avoid using high precision floating point values to ease string comparison post
	 * encoding.
	 * The values for humidity, temperature, and pressure are actually floating point values,
	 * env_ts is an integer.
	 */
	struct cloud_data_sensors data = {
		.humidity = 50,
		.temperature = 23,
		.pressure = 101,
		.bsec_air_quality = 50,
		.env_ts = 1000,
		.queued = true
	};

	ret = json_common_sensor_data_add(dummy.root_obj,
					&data,
					JSON_COMMON_ADD_DATA_TO_OBJECT,
					DATA_ENVIRONMENTALS,
					NULL);
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_ENVIRONMENTAL_JSON_SCHEMA,
				   data.queued);
	TEST_ASSERT_EQUAL(0, ret);

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_sensor_data_add(dummy.root_obj,
					  &data,
					  JSON_COMMON_ADD_DATA_TO_OBJECT,
					  "",
					  NULL);
	TEST_ASSERT_EQUAL(-ENODATA, ret);

	data.queued = true;

	ret = json_common_sensor_data_add(NULL,
					  &data,
					  JSON_COMMON_ADD_DATA_TO_OBJECT,
					  "",
					  NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = json_common_sensor_data_add(dummy.root_obj,
					  &data,
					  JSON_COMMON_ADD_DATA_TO_OBJECT,
					  NULL,
					  NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_encode_environmental_data_object_besec_disabled(void)
{
	int ret;
	struct cloud_data_sensors data = {
		.humidity = 50,
		.temperature = 23,
		.pressure = 101,
		.bsec_air_quality = -1,
		.env_ts = 1000,
		.queued = true
	};

	ret = json_common_sensor_data_add(dummy.root_obj,
					  &data,
					  JSON_COMMON_ADD_DATA_TO_OBJECT,
					  DATA_ENVIRONMENTALS,
					  NULL);
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.root_obj,
				   TEST_VALIDATE_ENVIRONMENTAL_JSON_SCHEMA_AIR_QUALITY_DISABLED,
				   data.queued);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_encode_environmental_data_array(void)
{
	int ret;
	/* Avoid using high precision floating point values to ease string comparison post
	 * encoding.
	 */
	struct cloud_data_sensors data = {
		.humidity = 50,
		.temperature = 23,
		.pressure = 101,
		.bsec_air_quality = 55,
		.env_ts = 1000,
		.queued = true
	};

	ret = json_common_sensor_data_add(dummy.array_obj,
					  &data,
					  JSON_COMMON_ADD_DATA_TO_ARRAY,
					  NULL,
					  NULL);
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.array_obj, TEST_VALIDATE_ARRAY_ENVIRONMENTAL_JSON_SCHEMA,
				   data.queued);
	TEST_ASSERT_EQUAL(0, ret);
}

/* Modem dynamic */

void test_encode_modem_dynamic_data_object(void)
{
	int ret;
	struct cloud_data_modem_dynamic data = {
		.band = 3,
		.nw_mode = LTE_LC_LTE_MODE_NBIOT,
		.rsrp = -8,
		.area = 12,
		.mccmnc = "24202",
		.cell = 33703719,
		.ip = "10.81.183.99",
		.ts = 1000,
		.queued = true,
	};

	ret = json_common_modem_dynamic_data_add(dummy.root_obj,
						 &data,
						 JSON_COMMON_ADD_DATA_TO_OBJECT,
						 DATA_MODEM_DYNAMIC,
						 NULL);
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_MODEM_DYNAMIC_JSON_SCHEMA,
				   data.queued);
	TEST_ASSERT_EQUAL(0, ret);

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_modem_dynamic_data_add(dummy.root_obj,
						 &data,
						 JSON_COMMON_ADD_DATA_TO_OBJECT,
						 "",
						 NULL);
	TEST_ASSERT_EQUAL(-ENODATA, ret);

	data.queued = true;

	ret = json_common_modem_dynamic_data_add(NULL,
						 &data,
						 JSON_COMMON_ADD_DATA_TO_OBJECT,
						 "",
						 NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = json_common_modem_dynamic_data_add(dummy.root_obj,
						 &data,
						 JSON_COMMON_ADD_DATA_TO_OBJECT,
						 NULL,
						 NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_encode_modem_dynamic_data_array(void)
{
	int ret;
	struct cloud_data_modem_dynamic data = {
		.band = 20,
		.nw_mode = LTE_LC_LTE_MODE_LTEM,
		.rsrp = -8,
		.area = 12,
		.mccmnc = "24202",
		.cell = 33703719,
		.ip = "10.81.183.99",
		.ts = 1000,
		.queued = true,
	};

	ret = json_common_modem_dynamic_data_add(dummy.array_obj,
						 &data,
						 JSON_COMMON_ADD_DATA_TO_ARRAY,
						 NULL,
						 NULL);
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.array_obj, TEST_VALIDATE_ARRAY_MODEM_DYNAMIC_JSON_SCHEMA,
				   data.queued);
	TEST_ASSERT_EQUAL(0, ret);
}

/* Modem static */

void test_encode_modem_static_data_object(void)
{
	int ret;
	struct cloud_data_modem_static data = {
		.imei = "352656106111232",
		.iccid = "89450421180216211234",
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
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_MODEM_STATIC_JSON_SCHEMA,
				   data.queued);
	TEST_ASSERT_EQUAL(0, ret);

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_modem_static_data_add(dummy.root_obj,
						&data,
						JSON_COMMON_ADD_DATA_TO_OBJECT,
						"",
						NULL);
	TEST_ASSERT_EQUAL(-ENODATA, ret);

	data.queued = true;

	ret = json_common_modem_static_data_add(NULL,
						&data,
						JSON_COMMON_ADD_DATA_TO_OBJECT,
						"",
						NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = json_common_modem_static_data_add(dummy.root_obj,
						&data,
						JSON_COMMON_ADD_DATA_TO_OBJECT,
						NULL,
						NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

void test_encode_modem_static_data_array(void)
{
	int ret;
	struct cloud_data_modem_static data = {
		.imei = "352656106111232",
		.iccid = "89450421180216211234",
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
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.array_obj, TEST_VALIDATE_ARRAY_MODEM_STATIC_JSON_SCHEMA,
				   data.queued);
	TEST_ASSERT_EQUAL(0, ret);
}

/* UI */

void test_encode_ui_data_object(void)
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
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_UI_JSON_SCHEMA, data.queued);
	TEST_ASSERT_EQUAL(0, ret);

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_ui_data_add(dummy.root_obj,
				      &data,
				      JSON_COMMON_ADD_DATA_TO_OBJECT,
				      "",
				      NULL);
	TEST_ASSERT_EQUAL(-ENODATA, ret);

	data.queued = true;

	ret = json_common_ui_data_add(NULL,
				      &data,
				      JSON_COMMON_ADD_DATA_TO_OBJECT,
				      "",
				      NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = json_common_ui_data_add(dummy.root_obj,
				      &data,
				      JSON_COMMON_ADD_DATA_TO_OBJECT,
				      NULL,
				      NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

/* Impact */

void test_encode_impact_data_object(void)
{
	int ret;
	struct cloud_data_impact data = {
		.magnitude = 300.0,
		.ts = 1000,
		.queued = true
	};

	ret = json_common_impact_data_add(dummy.root_obj,
				      &data,
				      JSON_COMMON_ADD_DATA_TO_OBJECT,
				      DATA_IMPACT,
				      NULL);
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_IMPACT_JSON_SCHEMA, data.queued);
	TEST_ASSERT_EQUAL(0, ret);

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_impact_data_add(dummy.root_obj,
				      &data,
				      JSON_COMMON_ADD_DATA_TO_OBJECT,
				      "",
				      NULL);
	TEST_ASSERT_EQUAL(-ENODATA, ret);

	data.queued = true;

	ret = json_common_impact_data_add(NULL,
				      &data,
				      JSON_COMMON_ADD_DATA_TO_OBJECT,
				      "",
				      NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);

	ret = json_common_impact_data_add(dummy.root_obj,
				      &data,
				      JSON_COMMON_ADD_DATA_TO_OBJECT,
				      NULL,
				      NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

/* Neighbor cell */

void test_encode_neighbor_cells_data_object(void)
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
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_NEIGHBOR_CELLS_JSON_SCHEMA,
				   data.queued);
	TEST_ASSERT_EQUAL(0, ret);

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_neighbor_cells_data_add(dummy.root_obj,
					     &data,
					     JSON_COMMON_ADD_DATA_TO_OBJECT);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
}

void test_encode_wifi_aps_data_object(void)
{
	int ret;
	struct cloud_data_wifi_access_points data = {
		.ap_info = {
			{.mac = {0x13, 0x00, 0xa5, 0xa0, 0xd2, 0x9c}},
			{.mac = {0x5c, 0x35, 0xb5, 0xc2, 0x7b, 0x3e}},
			{.mac = {0x73, 0x44, 0xf6, 0xc9, 0x00, 0xcd}},
			{.mac = {0x54, 0x5e, 0x8d, 0x44, 0x3d, 0x81}},
		},
		.cnt = 4,
		.ts = 1000,
		.queued = true,
	};

	ret = json_common_wifi_ap_data_add(dummy.root_obj,
					   &data,
					   JSON_COMMON_ADD_DATA_TO_OBJECT);
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_WIFI_AP_JSON_DATA,
				   data.queued);
	TEST_ASSERT_EQUAL(0, ret);

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_wifi_ap_data_add(dummy.root_obj,
					   &data,
					   JSON_COMMON_ADD_DATA_TO_OBJECT);
	TEST_ASSERT_EQUAL(-ENODATA, ret);

	data.queued = true;
	data.cnt = 1;

	ret = json_common_wifi_ap_data_add(dummy.root_obj,
					   &data,
					   JSON_COMMON_ADD_DATA_TO_OBJECT);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
}

void test_encode_agps_request_data_object(void)
{
	int ret;
	struct cloud_data_agps_request data = {
		.mcc = 242,
		.mnc = 1,
		.cell = 21679716,
		.area = 40401,
		.request.system[0].sv_mask_ephe = UINT32_MAX,
		.request.system[0].sv_mask_alm = UINT32_MAX,
		.request.data_flags =
			NRF_MODEM_GNSS_AGNSS_GPS_UTC_REQUEST |
			NRF_MODEM_GNSS_AGNSS_KLOBUCHAR_REQUEST |
			NRF_MODEM_GNSS_AGNSS_NEQUICK_REQUEST |
			NRF_MODEM_GNSS_AGNSS_GPS_SYS_TIME_AND_SV_TOW_REQUEST |
			NRF_MODEM_GNSS_AGNSS_POSITION_REQUEST |
			NRF_MODEM_GNSS_AGNSS_INTEGRITY_REQUEST,
		.queued = true
	};

	ret = json_common_agps_request_data_add(dummy.root_obj,
						&data,
						JSON_COMMON_ADD_DATA_TO_OBJECT);
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_AGPS_REQUEST_JSON_SCHEMA,
				   data.queued);
	TEST_ASSERT_EQUAL(0, ret);

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_agps_request_data_add(dummy.root_obj,
						&data,
						JSON_COMMON_ADD_DATA_TO_OBJECT);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
}

void test_encode_pgps_request_data_object(void)
{
	int ret;
	struct cloud_data_pgps_request data = {
		.count = 42,
		.interval = 240,
		.day = 15160,
		.time = 40655,
		.queued = true
	};

	ret = json_common_pgps_request_data_add(dummy.root_obj, &data);
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_PGPS_REQUEST_JSON_SCHEMA,
				   data.queued);
	TEST_ASSERT_EQUAL(0, ret);

	/* Check for invalid inputs. */

	data.queued = false;

	ret = json_common_pgps_request_data_add(dummy.root_obj, &data);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
}

void test_encode_ui_data_array(void)
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
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.array_obj, TEST_VALIDATE_ARRAY_UI_JSON_SCHEMA,
				   data.queued);
	TEST_ASSERT_EQUAL(0, ret);
}

void test_encode_impact_data_array(void)
{
	int ret;
	struct cloud_data_impact data = {
		.magnitude = 300.0,
		.ts = 1000,
		.queued = true
	};

	ret = json_common_impact_data_add(dummy.array_obj,
				      &data,
				      JSON_COMMON_ADD_DATA_TO_ARRAY,
				      NULL,
				      NULL);
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.array_obj, TEST_VALIDATE_ARRAY_IMPACT_JSON_SCHEMA,
				   data.queued);
	TEST_ASSERT_EQUAL(0, ret);
}

/* Configuration encode */

void test_encode_configuration_data_object(void)
{
	int ret;
	struct cloud_data_cfg data = {
		.active_mode = false,
		.active_wait_timeout = 120,
		.movement_resolution = 120,
		.movement_timeout = 3600,
		.location_timeout = 60,
		.accelerometer_activity_threshold = 10,
		.accelerometer_inactivity_threshold = 5,
		.accelerometer_inactivity_timeout = 80,
		.no_data.gnss = true,
		.no_data.neighbor_cell = true
	};

	ret = json_common_config_add(dummy.root_obj, &data, DATA_CONFIG);
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_CONFIGURATION_JSON_SCHEMA, -1);
	TEST_ASSERT_EQUAL(0, ret);

	/* Check for invalid input. */

	ret = json_common_config_add(dummy.root_obj, &data, NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

/* Configuration decode */

void test_decode_configuration_data(void)
{
	cJSON *root_obj = NULL;
	cJSON *sub_group_obj = NULL;
	struct cloud_data_cfg data = {0};

	root_obj = cJSON_Parse(TEST_VALIDATE_CONFIGURATION_JSON_SCHEMA);
	TEST_ASSERT_NOT_NULL(root_obj);
	sub_group_obj = json_object_decode(root_obj, OBJECT_CONFIG);
	TEST_ASSERT_NOT_NULL(sub_group_obj);

	json_common_config_get(sub_group_obj, &data);

	TEST_ASSERT_EQUAL(false, data.active_mode);
	TEST_ASSERT_EQUAL(true, data.no_data.gnss);
	TEST_ASSERT_EQUAL(true, data.no_data.neighbor_cell);
	TEST_ASSERT_EQUAL(60, data.location_timeout);
	TEST_ASSERT_EQUAL(120, data.active_wait_timeout);
	TEST_ASSERT_EQUAL(120, data.movement_resolution);
	TEST_ASSERT_EQUAL(3600, data.movement_timeout);
	TEST_ASSERT_EQUAL(10, data.accelerometer_activity_threshold);
	TEST_ASSERT_EQUAL(5, data.accelerometer_inactivity_threshold);
	TEST_ASSERT_EQUAL(80, data.accelerometer_inactivity_timeout);

	cJSON_Delete(root_obj);
}

/* Batch data */

void test_encode_batch_data_object(void)
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
	struct cloud_data_gnss gnss[2] = {
		[0].pvt.longi = 10,
		[0].pvt.lat = 62,
		[0].pvt.acc = 24,
		[0].pvt.alt = 170,
		[0].pvt.spd = 1,
		[0].pvt.hdg = 176,
		[0].gnss_ts = 1000,
		[0].queued = true,
		/* Second entry */
		[1].pvt.longi = 10,
		[1].pvt.lat = 62,
		[1].pvt.acc = 24,
		[1].pvt.alt = 170,
		[1].pvt.spd = 1,
		[1].pvt.hdg = 176,
		[1].gnss_ts = 1000,
		[1].queued = true
	};
	struct cloud_data_modem_dynamic modem_dynamic[2] = {
		[0].band = 3,
		[0].nw_mode = LTE_LC_LTE_MODE_NBIOT,
		[0].rsrp = -8,
		[0].area = 12,
		[0].mccmnc = "24202",
		[0].cell = 33703719,
		[0].ip = "10.81.183.99",
		[0].ts = 1000,
		[0].queued = true,
		/* Second entry */
		[1].band = 20,
		[1].nw_mode = LTE_LC_LTE_MODE_LTEM,
		[1].rsrp = -5,
		[1].area = 12,
		[1].mccmnc = "24202",
		[1].cell = 33703719,
		[1].ip = "10.81.183.99",
		[1].ts = 1000,
		[1].queued = true,
	};
	struct cloud_data_modem_static modem_static[2] = {
		[0].imei = "352656106111232",
		[0].iccid = "89450421180216211234",
		[0].fw = "mfw_nrf9160_1.2.3",
		[0].brdv = "nrf9160dk_nrf9160",
		[0].appv = "v1.0.0-development",
		[0].ts = 1000,
		[0].queued = true,
		/* Second entry */
		[1].imei = "352656106111232",
		[1].iccid = "89450421180216211234",
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
	struct cloud_data_impact impact[2] = {
		[0].magnitude = 300.0,
		[0].ts = 1000,
		[0].queued = true,
		/* Second entry */
		[1].magnitude = 300.0,
		[1].ts = 1000,
		[1].queued = true
	};
	struct cloud_data_sensors environmental[2] = {
		[0].humidity = 50,
		[0].temperature = 23,
		[0].pressure = 80,
		[0].bsec_air_quality = 50,
		[0].env_ts = 1000,
		[0].queued = true,
		/* Second entry */
		[1].humidity = 50,
		[1].temperature = 23,
		[1].pressure = 101,
		[1].bsec_air_quality = 55,
		[1].env_ts = 1000,
		[1].queued = true
	};

	ret = json_common_batch_data_add(dummy.root_obj,
					 JSON_COMMON_BATTERY,
					 &battery,
					 ARRAY_SIZE(battery),
					 DATA_BATTERY);
	TEST_ASSERT_EQUAL(0, ret);

	ret = json_common_batch_data_add(dummy.root_obj,
					 JSON_COMMON_UI,
					 &ui,
					 ARRAY_SIZE(ui),
					 DATA_BUTTON);
	TEST_ASSERT_EQUAL(0, ret);

	ret = json_common_batch_data_add(dummy.root_obj,
					 JSON_COMMON_IMPACT,
					 &impact,
					 ARRAY_SIZE(impact),
					 DATA_IMPACT);
	TEST_ASSERT_EQUAL(0, ret);

	ret = json_common_batch_data_add(dummy.root_obj,
					 JSON_COMMON_GNSS,
					 &gnss,
					 ARRAY_SIZE(gnss),
					 DATA_GNSS);
	TEST_ASSERT_EQUAL(0, ret);

	ret = json_common_batch_data_add(dummy.root_obj,
					 JSON_COMMON_SENSOR,
					 &environmental,
					 ARRAY_SIZE(environmental),
					 DATA_ENVIRONMENTALS);
	TEST_ASSERT_EQUAL(0, ret);

	ret = json_common_batch_data_add(dummy.root_obj,
					 JSON_COMMON_MODEM_DYNAMIC,
					 &modem_dynamic,
					 ARRAY_SIZE(modem_dynamic),
					 DATA_MODEM_DYNAMIC);
	TEST_ASSERT_EQUAL(0, ret);

	ret = json_common_batch_data_add(dummy.root_obj,
					 JSON_COMMON_MODEM_STATIC,
					 &modem_static,
					 ARRAY_SIZE(modem_static),
					 DATA_MODEM_STATIC);
	TEST_ASSERT_EQUAL(0, ret);

	ret = encoded_output_check(dummy.root_obj, TEST_VALIDATE_BATCH_JSON_SCHEMA, -1);
	TEST_ASSERT_EQUAL(0, ret);

	/* Check for invalid inputs. */

	ret = json_common_batch_data_add(NULL, -1, NULL, 0, "");
	TEST_ASSERT_EQUAL(-ENOMEM, ret);

	ret = json_common_batch_data_add(dummy.root_obj, -1, NULL, 0, NULL);
	TEST_ASSERT_EQUAL(-EINVAL, ret);
}

/* Test used to verify encoding and decoding of data structures that contain floating point
 * values. Floating point values cannot be exactly represented in binary so they cannot be compared
 * with a predefined JSON string schema.
 */

void test_floating_point_encoding_gnss(void)
{
	int ret;
	cJSON *decoded_root_obj;
	cJSON *decoded_gnss_obj;
	cJSON *decoded_value_obj;
	struct cloud_data_gnss decoded_values = {0};
	struct cloud_data_gnss data = {
		.pvt.longi = 10.417852141870654,
		.pvt.lat = 63.43278762669529,
		.pvt.acc = 15.455987930297852,
		.pvt.alt = 53.67230987548828,
		.pvt.spd = 0.4443884789943695,
		.pvt.hdg = 176.12345298374867,
		.gnss_ts = 1000,
		.queued = true
	};

	ret = json_common_gnss_data_add(dummy.root_obj,
				       &data,
				       JSON_COMMON_ADD_DATA_TO_OBJECT,
				       DATA_GNSS,
				       NULL);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_FALSE(data.queued);

	dummy.buffer = cJSON_PrintUnformatted(dummy.root_obj);
	TEST_ASSERT_NOT_NULL(dummy.buffer);

	decoded_root_obj = cJSON_Parse(dummy.buffer);
	TEST_ASSERT_NOT_NULL(decoded_root_obj);

	decoded_gnss_obj = json_object_decode(decoded_root_obj, DATA_GNSS);
	TEST_ASSERT_NOT_NULL(decoded_gnss_obj);

	decoded_value_obj = json_object_decode(decoded_gnss_obj, DATA_VALUE);
	TEST_ASSERT_NOT_NULL(decoded_value_obj);

	cJSON *longitude = cJSON_GetObjectItem(decoded_value_obj, DATA_GNSS_LONGITUDE);
	cJSON *latitude = cJSON_GetObjectItem(decoded_value_obj, DATA_GNSS_LATITUDE);
	cJSON *accuracy = cJSON_GetObjectItem(decoded_value_obj, DATA_MOVEMENT);
	cJSON *altitude = cJSON_GetObjectItem(decoded_value_obj, DATA_GNSS_ALTITUDE);
	cJSON *speed = cJSON_GetObjectItem(decoded_value_obj, DATA_GNSS_SPEED);
	cJSON *heading = cJSON_GetObjectItem(decoded_value_obj, DATA_GNSS_HEADING);

	TEST_ASSERT_NOT_NULL(longitude);
	TEST_ASSERT_NOT_NULL(latitude);
	TEST_ASSERT_NOT_NULL(accuracy);
	TEST_ASSERT_NOT_NULL(altitude);
	TEST_ASSERT_NOT_NULL(speed);
	TEST_ASSERT_NOT_NULL(heading);

	decoded_values.pvt.longi = longitude->valuedouble;
	decoded_values.pvt.lat = latitude->valuedouble;
	decoded_values.pvt.acc = accuracy->valuedouble;
	decoded_values.pvt.alt = altitude->valuedouble;
	decoded_values.pvt.spd = speed->valuedouble;
	decoded_values.pvt.hdg = heading->valuedouble;

	TEST_ASSERT_DOUBLE_WITHIN(0.1, data.pvt.longi, decoded_values.pvt.longi);
	TEST_ASSERT_DOUBLE_WITHIN(0.1, data.pvt.lat, decoded_values.pvt.lat);
	TEST_ASSERT_FLOAT_WITHIN(0.1, data.pvt.acc, decoded_values.pvt.acc);
	TEST_ASSERT_FLOAT_WITHIN(0.1, data.pvt.alt, decoded_values.pvt.alt);
	TEST_ASSERT_FLOAT_WITHIN(0.1, data.pvt.spd, decoded_values.pvt.spd);
	TEST_ASSERT_FLOAT_WITHIN(0.1, data.pvt.hdg, decoded_values.pvt.hdg);

	cJSON_Delete(decoded_root_obj);
}

void test_floating_point_encoding_environmental(void)
{
	int ret;
	cJSON *decoded_root_obj;
	cJSON *decoded_env_obj;
	cJSON *decoded_value_obj;
	struct cloud_data_sensors decoded_values = {0};
	struct cloud_data_sensors data = {
		.temperature = 26.27,
		.humidity = 35.15,
		.pressure = 101.36,
		.env_ts = 1000,
		.queued = true
	};

	ret = json_common_sensor_data_add(dummy.root_obj,
					  &data,
					  JSON_COMMON_ADD_DATA_TO_OBJECT,
					  DATA_ENVIRONMENTALS,
					  NULL);
	TEST_ASSERT_EQUAL(0, ret);
	TEST_ASSERT_FALSE(data.queued);

	dummy.buffer = cJSON_PrintUnformatted(dummy.root_obj);
	TEST_ASSERT_NOT_NULL(dummy.buffer);

	decoded_root_obj = cJSON_Parse(dummy.buffer);
	TEST_ASSERT_NOT_NULL(decoded_root_obj);

	decoded_env_obj = json_object_decode(decoded_root_obj, DATA_ENVIRONMENTALS);
	TEST_ASSERT_NOT_NULL(decoded_env_obj);

	decoded_value_obj = json_object_decode(decoded_env_obj, DATA_VALUE);
	TEST_ASSERT_NOT_NULL(decoded_value_obj);

	cJSON *temperature = cJSON_GetObjectItem(decoded_value_obj, DATA_TEMPERATURE);
	cJSON *humidity = cJSON_GetObjectItem(decoded_value_obj, DATA_HUMIDITY);
	cJSON *pressure = cJSON_GetObjectItem(decoded_value_obj, DATA_PRESSURE);

	TEST_ASSERT_NOT_NULL(temperature);
	TEST_ASSERT_NOT_NULL(humidity);
	TEST_ASSERT_NOT_NULL(pressure);

	decoded_values.temperature = temperature->valuedouble;
	decoded_values.humidity = humidity->valuedouble;
	decoded_values.pressure = pressure->valuedouble;

	TEST_ASSERT_DOUBLE_WITHIN(0.1, data.temperature, decoded_values.temperature);
	TEST_ASSERT_DOUBLE_WITHIN(0.1, data.humidity, decoded_values.humidity);
	TEST_ASSERT_DOUBLE_WITHIN(0.1, data.pressure, decoded_values.pressure);

	cJSON_Delete(decoded_root_obj);
}

void test_floating_point_encoding_configuration(void)
{
	int ret;
	cJSON *decoded_root_obj;
	cJSON *decoded_config_obj;
	struct cloud_data_cfg decoded_values = {0};
	struct cloud_data_cfg data = {
		.accelerometer_activity_threshold = 2.22,
		.accelerometer_inactivity_threshold = 1.11,
		.accelerometer_inactivity_timeout = 20.0,
	};

	ret = json_common_config_add(dummy.root_obj, &data, DATA_CONFIG);
	TEST_ASSERT_EQUAL(0, ret);
	dummy.buffer = cJSON_PrintUnformatted(dummy.root_obj);

	TEST_ASSERT_NOT_NULL(dummy.buffer);

	decoded_root_obj = cJSON_Parse(dummy.buffer);
	TEST_ASSERT_NOT_NULL(decoded_root_obj);

	decoded_config_obj = json_object_decode(decoded_root_obj, DATA_CONFIG);
	TEST_ASSERT_NOT_NULL(decoded_config_obj);

	cJSON *accel_act_thresh =
		cJSON_GetObjectItem(decoded_config_obj, CONFIG_ACC_ACT_THRESHOLD);
	cJSON *accel_inact_thresh =
		cJSON_GetObjectItem(decoded_config_obj, CONFIG_ACC_INACT_THRESHOLD);
	cJSON *accel_inact_timeout =
		cJSON_GetObjectItem(decoded_config_obj, CONFIG_ACC_INACT_TIMEOUT);

	TEST_ASSERT_NOT_NULL(accel_act_thresh);
	TEST_ASSERT_NOT_NULL(accel_inact_thresh);
	TEST_ASSERT_NOT_NULL(accel_inact_timeout);

	decoded_values.accelerometer_activity_threshold = accel_act_thresh->valuedouble;
	decoded_values.accelerometer_inactivity_threshold = accel_inact_thresh->valuedouble;
	decoded_values.accelerometer_inactivity_timeout = accel_inact_timeout->valuedouble;

	TEST_ASSERT_DOUBLE_WITHIN(0.1, data.accelerometer_activity_threshold,
				decoded_values.accelerometer_activity_threshold);

	TEST_ASSERT_DOUBLE_WITHIN(0.1, data.accelerometer_inactivity_threshold,
				decoded_values.accelerometer_inactivity_threshold);

	TEST_ASSERT_DOUBLE_WITHIN(0.1, data.accelerometer_inactivity_timeout,
				decoded_values.accelerometer_inactivity_timeout);

	cJSON_Delete(decoded_root_obj);
}


int main(void)
{
	cJSON_Init();
	(void)unity_main();
	return 0;
}
