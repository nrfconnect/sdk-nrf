/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <unity.h>
#include <stdbool.h>
#include <cJSON_os.h>
#include <errno.h>

#include "cloud_codec.h"

#include "cmock_json_helpers.h"
#include "cmock_date_time.h"
#include "cmock_cJSON_to_mock.h"
#include "float.h"

static struct cloud_codec_data codec;
static int ret;

void setUp(void)
{
	__cmock_cJSON_Init_Ignore();
	ret = cloud_codec_init(NULL, NULL);
	TEST_ASSERT_EQUAL(0, ret);

	memset(&codec, 0, sizeof(codec));
}

/**
 * @brief mocking-enabled tests for nrf cloud codec
 *
 * The following tests focus on covering error handling.
 * For all of the tests, a function call in a specific part of execution is set to fail.
 * You can detect these cases when looking at which mocks return NULL or an error code.
 *
 * There is special consideration for heap objects to detect irregularities
 * regarding allocation/deallocation.
 * The comments on the right side track lifetimes of cJSON objects.
 *
 */

void test_enc_ui_fail0(void)
{
	struct cloud_data_ui data = {
		.queued = true,
		.btn_ts = 1563968747123,
	};

	/* cloud_codec_encode_ui_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn(NULL);

	ret = cloud_codec_encode_ui_data(&codec, &data);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

void test_enc_ui_fail1(void)
{
	struct cloud_data_ui data = {
		.queued = true,
		.btn_ts = 1563968747123,
	};

	/* cloud_codec_encode_ui_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn((void *)1);				/*A*/
	__cmock_json_add_str_ExpectAnyArgsAndReturn(-ENOMEM);
	__cmock_cJSON_Delete_ExpectAnyArgs();						/*~A*/

	ret = cloud_codec_encode_ui_data(&codec, &data);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

void test_enc_ui_fail2(void)
{
	struct cloud_data_ui data = {
		.queued = true,
		.btn_ts = 1563968747123,
	};

	/* cloud_codec_encode_ui_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn((void *)1);				/*A*/
	__cmock_json_add_str_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	/* add_meta_data */
	__cmock_json_add_str_ExpectAnyArgsAndReturn(-ENOMEM);
	/* cloud_codec_encode_ui_data */
	__cmock_cJSON_Delete_ExpectAnyArgs();						/*~A*/

	ret = cloud_codec_encode_ui_data(&codec, &data);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

void test_enc_ui_fail3(void)
{
	struct cloud_data_ui data = {
		.queued = true,
		.btn_ts = 1563968747123,
	};

	/* cloud_codec_encode_ui_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn((void *)1);				/*A*/
	__cmock_json_add_str_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	/* add_meta_data */
	__cmock_json_add_str_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	__cmock_json_add_str_ExpectAnyArgsAndReturn(-ENOMEM);
	/* cloud_codec_encode_ui_data */
	__cmock_cJSON_Delete_ExpectAnyArgs();						/*~A*/

	ret = cloud_codec_encode_ui_data(&codec, &data);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

void test_enc_ui_fail4(void)
{
	struct cloud_data_ui data = {
		.queued = true,
		.btn_ts = 1563968747123,
	};

	/* cloud_codec_encode_ui_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn((void *)1);				/*A*/
	__cmock_json_add_str_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	/* add_meta_data */
	__cmock_json_add_str_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	__cmock_json_add_str_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	__cmock_date_time_uptime_to_unix_time_ms_ExpectAnyArgsAndReturn(-ENODATA);
	/* cloud_codec_encode_ui_data */
	__cmock_cJSON_Delete_ExpectAnyArgs();						/*~A*/

	ret = cloud_codec_encode_ui_data(&codec, &data);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
}

void test_enc_ui_fail5(void)
{
	struct cloud_data_ui data = {
		.queued = true,
		.btn_ts = 1563968747123,
	};

	/* cloud_codec_encode_ui_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn((void *)1);				/*A*/
	__cmock_json_add_str_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	/* add_meta_data */
	__cmock_json_add_str_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	__cmock_json_add_str_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	__cmock_date_time_uptime_to_unix_time_ms_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	__cmock_json_add_number_ExpectAnyArgsAndReturn(-ENOMEM);
	/* cloud_codec_encode_ui_data */
	__cmock_cJSON_Delete_ExpectAnyArgs();						/*~A*/

	ret = cloud_codec_encode_ui_data(&codec, &data);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

void test_enc_ui_fail6(void)
{
	struct cloud_data_ui data = {
		.queued = true,
		.btn = 1,
		.btn_ts = 1563968747123,
	};

	/* cloud_codec_encode_ui_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn((void *)1);				/*A*/
	__cmock_json_add_str_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	/* add_meta_data */
	__cmock_json_add_str_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	__cmock_json_add_str_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	__cmock_date_time_uptime_to_unix_time_ms_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	__cmock_json_add_number_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	/* cloud_codec_encode_ui_data */
	__cmock_cJSON_PrintUnformatted_ExpectAnyArgsAndReturn(NULL);
	__cmock_cJSON_Delete_ExpectAnyArgs();

	ret = cloud_codec_encode_ui_data(&codec, &data);				/*~A*/
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

void test_enc_batch_data_sensor_timefail(void)
{
	struct cloud_data_sensors sensor_buf = { .queued = true	};

	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_CreateArray_ExpectAndReturn((void *)1);				/*A*/
	/* add_batch_data */
	__cmock_date_time_uptime_to_unix_time_ms_ExpectAnyArgsAndReturn(-EINVAL);
	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_Delete_ExpectAnyArgs();						/*~A*/

	ret = cloud_codec_encode_batch_data(&codec,
					    NULL,
					    &sensor_buf,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    0, 1, 0, 0, 0, 0, 0);
	TEST_ASSERT_EQUAL(-EOVERFLOW, ret);
}

void test_enc_batch_data_sensor_big_values(void)
{
	struct cloud_data_sensors sensor_buf = {
		.queued = true,
		.humidity = DBL_MAX,
		.temperature = DBL_MAX,
		.pressure = DBL_MAX,
		.bsec_air_quality = INT_MAX,
	};

	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_CreateArray_ExpectAndReturn((void *)1);				/*A*/
	/* add_batch_data */
	__cmock_date_time_uptime_to_unix_time_ms_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	/* add_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn((void *)1);				/*B*/
	__cmock_json_add_obj_array_ExpectAnyArgs();					/*~B*/
	/* add_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn((void *)1);				/*C*/
	__cmock_json_add_obj_array_ExpectAnyArgs();					/*~C*/
	/* add_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn((void *)1);				/*D*/
	__cmock_json_add_obj_array_ExpectAnyArgs();					/*~D*/
	/* add_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn((void *)1);				/*E*/
	__cmock_json_add_obj_array_ExpectAnyArgs();					/*~E*/
	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_Delete_ExpectAnyArgs();						/*~A*/

	/* some functions we don't care about here */
	__cmock_cJSON_GetArraySize_IgnoreAndReturn(0);
	__cmock_json_add_str_IgnoreAndReturn(EXIT_SUCCESS);
	__cmock_json_add_number_IgnoreAndReturn(EXIT_SUCCESS);

	ret = cloud_codec_encode_batch_data(&codec,
					    NULL,
					    &sensor_buf,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    0, 1, 0, 0, 0, 0, 0);
	TEST_ASSERT_EQUAL(-ENODATA, ret);
}

void test_enc_batch_data_sensor_add_air_quality_fail(void)
{
	struct cloud_data_sensors sensor_buf = {
		.queued = true,
		.humidity = DBL_MAX,
		.temperature = DBL_MAX,
		.pressure = DBL_MAX,
		.bsec_air_quality = INT_MAX,
	};

	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_CreateArray_ExpectAndReturn((void *)1);				/*A*/
	/* add_batch_data */
	__cmock_date_time_uptime_to_unix_time_ms_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	/* add_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn(NULL);
	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_Delete_ExpectAnyArgs();						/*~A*/

	/* some functions we don't care about here */
	__cmock_cJSON_GetArraySize_IgnoreAndReturn(0);
	__cmock_json_add_str_IgnoreAndReturn(EXIT_SUCCESS);
	__cmock_json_add_number_IgnoreAndReturn(EXIT_SUCCESS);

	ret = cloud_codec_encode_batch_data(&codec,
					    NULL,
					    &sensor_buf,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    0, 1, 0, 0, 0, 0, 0);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

void test_enc_batch_data_sensor_add_humidity_fail(void)
{
	struct cloud_data_sensors sensor_buf = {
		.queued = true,
		.humidity = DBL_MAX,
		.temperature = DBL_MAX,
		.pressure = DBL_MAX,
		.bsec_air_quality = INT_MAX,
	};

	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_CreateArray_ExpectAndReturn((void *)1);				/*A*/
	/* add_batch_data */
	__cmock_date_time_uptime_to_unix_time_ms_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	/* add_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn((void *)1);				/*B*/
	__cmock_json_add_obj_array_ExpectAnyArgs();					/*~B*/
	/* add_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn(NULL);
	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_Delete_ExpectAnyArgs();						/*~A*/

	/* some functions we don't care about here */
	__cmock_cJSON_GetArraySize_IgnoreAndReturn(0);
	__cmock_json_add_str_IgnoreAndReturn(EXIT_SUCCESS);
	__cmock_json_add_number_IgnoreAndReturn(EXIT_SUCCESS);

	ret = cloud_codec_encode_batch_data(&codec,
					    NULL,
					    &sensor_buf,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    0, 1, 0, 0, 0, 0, 0);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

void test_enc_batch_data_sensor_add_temperature_fail(void)
{
	struct cloud_data_sensors sensor_buf = {
		.queued = true,
		.humidity = DBL_MAX,
		.temperature = DBL_MAX,
		.pressure = DBL_MAX,
		.bsec_air_quality = INT_MAX,
	};

	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_CreateArray_ExpectAndReturn((void *)1);				/*A*/
	/* add_batch_data */
	__cmock_date_time_uptime_to_unix_time_ms_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	/* add_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn((void *)1);				/*B*/
	__cmock_json_add_obj_array_ExpectAnyArgs();					/*~B*/
	/* add_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn((void *)1);				/*C*/
	__cmock_json_add_obj_array_ExpectAnyArgs();					/*~C*/
	/* add_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn(NULL);
	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_Delete_ExpectAnyArgs();						/*~A*/

	/* some functions we don't care about here */
	__cmock_cJSON_GetArraySize_IgnoreAndReturn(0);
	__cmock_json_add_str_IgnoreAndReturn(EXIT_SUCCESS);
	__cmock_json_add_number_IgnoreAndReturn(EXIT_SUCCESS);

	ret = cloud_codec_encode_batch_data(&codec,
					    NULL,
					    &sensor_buf,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    0, 1, 0, 0, 0, 0, 0);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

void test_enc_batch_data_sensor_add_pressure_fail(void)
{
	struct cloud_data_sensors sensor_buf = {
		.queued = true,
		.humidity = DBL_MAX,
		.temperature = DBL_MAX,
		.pressure = DBL_MAX,
		.bsec_air_quality = INT_MAX,
	};

	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_CreateArray_ExpectAndReturn((void *)1);				/*A*/
	/* add_batch_data */
	__cmock_date_time_uptime_to_unix_time_ms_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	/* add_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn((void *)1);				/*B*/
	__cmock_json_add_obj_array_ExpectAnyArgs();					/*~B*/
	/* add_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn((void *)1);				/*C*/
	__cmock_json_add_obj_array_ExpectAnyArgs();					/*~C*/
	/* add_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn((void *)1);				/*D*/
	__cmock_json_add_obj_array_ExpectAnyArgs();					/*~D*/
	/* add_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn(NULL);
	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_Delete_ExpectAnyArgs();						/*~A*/

	/* some functions we don't care about here */
	__cmock_cJSON_GetArraySize_IgnoreAndReturn(0);
	__cmock_json_add_str_IgnoreAndReturn(EXIT_SUCCESS);
	__cmock_json_add_number_IgnoreAndReturn(EXIT_SUCCESS);

	ret = cloud_codec_encode_batch_data(&codec,
					    NULL,
					    &sensor_buf,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    0, 1, 0, 0, 0, 0, 0);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

void test_enc_batch_data_gnss_no_array(void)
{
	struct cloud_data_gnss gnss_buf = {
		.queued = true
	};

	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_CreateArray_ExpectAndReturn(NULL);
	ret = cloud_codec_encode_batch_data(&codec,
					    &gnss_buf,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    1, 0, 0, 0, 0, 0, 0);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

void test_enc_batch_data_bat_no_data_obj(void)
{
	struct cloud_data_battery bat_buf = {
		.bat = 50,
		.bat_ts = 1563968747123,
		.queued = true,
	};

	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_CreateArray_ExpectAndReturn((void *)1);				/*A*/
	/* add_batch_data */
	/* add_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn(NULL);
	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_Delete_ExpectAnyArgs();						/*~A*/

	ret = cloud_codec_encode_batch_data(&codec,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    &bat_buf,
					    0, 0, 0, 0, 0, 0, 1);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

void test_enc_batch_data_gnss_no_data_obj(void)
{
	struct cloud_data_gnss gnss_buf = {
		.queued = true,
	};

	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_CreateArray_ExpectAndReturn((void *)1);				/*A*/
	/* add_batch_data */
	/* add_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn(NULL);
	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_Delete_ExpectAnyArgs();						/*~A*/

	ret = cloud_codec_encode_batch_data(&codec,
					    &gnss_buf,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    1, 0, 0, 0, 0, 0, 0);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

void test_enc_batch_data_ui_no_data_obj(void)
{
	struct cloud_data_ui ui_buf = {
		.queued = true
	};

	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_CreateArray_ExpectAndReturn((void *)1);				/*A*/
	/* add_batch_data */
	/* add_data */
	__cmock_cJSON_CreateObject_ExpectAndReturn(NULL);
	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_Delete_ExpectAnyArgs();						/*~A*/

	ret = cloud_codec_encode_batch_data(&codec,
					    NULL,
					    NULL,
					    NULL,
					    NULL,
					    &ui_buf,
					    NULL,
					    NULL,
					    0, 0, 0, 0, 1, 0, 0);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
	TEST_ASSERT_TRUE(ui_buf.queued);
}

void test_enc_batch_data_modem_dyn_no_data_obj(void)
{
	struct cloud_data_modem_dynamic modem_dyn_buf = {
		.queued = true
	};

	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_CreateArray_ExpectAndReturn((void *)1);				/*A*/
	/* add_batch_data */
	/* modem_dynamic_data_add */
	__cmock_date_time_uptime_to_unix_time_ms_ExpectAnyArgsAndReturn(EXIT_SUCCESS);
	__cmock_cJSON_CreateObject_ExpectAndReturn(NULL);
	/* cloud_codec_encode_batch_data */
	__cmock_cJSON_Delete_ExpectAnyArgs();						/*~A*/

	ret = cloud_codec_encode_batch_data(&codec,
					    NULL,
					    NULL,
					    NULL,
					    &modem_dyn_buf,
					    NULL,
					    NULL,
					    NULL,
					    0, 0, 0, 1, 0, 0, 0);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
	TEST_ASSERT_TRUE(modem_dyn_buf.queued);
}

void test_enc_config_nomem1(void)
{
	struct cloud_data_cfg data = {0};

	/* cloud_codec_encode_config */
	__cmock_cJSON_CreateObject_ExpectAndReturn(NULL);
	__cmock_cJSON_CreateObject_ExpectAndReturn(NULL);
	__cmock_cJSON_CreateObject_ExpectAndReturn(NULL);
	__cmock_cJSON_Delete_ExpectAnyArgs();
	__cmock_cJSON_Delete_ExpectAnyArgs();
	__cmock_cJSON_Delete_ExpectAnyArgs();

	ret = cloud_codec_encode_config(&codec, &data);
	TEST_ASSERT_EQUAL(-ENOMEM, ret);
}

/* It is required to be added to each test. That is because unity's
 * main may return nonzero, while zephyr's main currently must
 * return 0 in all cases (other values are reserved).
 */
extern int unity_main(void);

int main(void)
{
	(void)unity_main();
	return 0;
}
