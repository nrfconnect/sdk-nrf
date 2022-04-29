/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <math.h>
#include <ztest.h>
#include <zephyr.h>
#include <cJSON_os.h>

/* We use a .c file here so that the mocks do not overshadow existing definitions for
 * intellisense and similar
 */
#include "mocks.c"

/* Include application.c directly to gain access to static functions */
#include "../../../src/application.c"

#define TEST_TIMESTAMP_A 1651097915356L
#define TEST_TIMESTAMP_B 1651103315274L
#define TEST_SENSOR_NAME_A "TEMP"
#define TEST_SENSOR_NAME_B "COUNT"
#define TEST_SENSOR_VALUE_A 1701
#define TEST_SENSOR_VALUE_B 31.9
#define TEST_SENSOR_VALUE_C 42
#define TEST_SENSOR_VALUE_D -100


/**
 * @brief Execute create_timestamped_data_message_object and perform minimal sanity checks
 *
 * @param ts - The timestamp to feed to create_timestamped_data_message_object
 * @param sensor - The sensor name to pass to create_timestamped_data_message_object
 * @param msg_obj - A pointer (to a pointer) where the returned message object pointer can be
 *		    stored.
 * @param msg - A pointer (to a pointer) where a pointer to the stringified message object can be
 *		stored.
 */
static void util_gen_msg(int64_t ts, const char *const sensor, cJSON **msg_obj, char **msg)
{
	/* We print the argument passed to crate_timestamped_data_message_object in our failure
	 * messages so that if any of these tests ever fail, it is easier to identify the calling
	 * testcase line.
	 */

	/* create_timestamped_data_message_object will call date_time_now, feed it a fake value */
	mocks_set_date_time_now(ts);

	/* Call create_timestamped_data_message_object with the requested sensor type.
	 * This a function under test!
	 */
	*msg_obj = create_timestamped_data_message_object(sensor);

	/* The result should definitely not be null. */
	zassert_not_null(*msg_obj, "create_timestamped_data_message_object(\"%s\") should not "
				   "return null.", sensor);

	/* The result should be a cJSON object. */
	zassert_true(cJSON_IsObject(*msg_obj), "create_timestamped_data_message_object(\"%s\") "
					       "should return a cJSON object.", sensor);

	/* Print the result as a string. */
	*msg = cJSON_PrintUnformatted(*msg_obj);

	/* The printed result should not be null. */
	zassert_not_null(msg, "Printing the result of "
			      "create_timestamped_data_message_object(\"%s\") "
			      "should succeed.", sensor);
}


/**
 * @brief Clean up a pair of msg_obj and msg storage pointers.
 *
 * @param msg_obj - The pointer-to-pointer where the message object reference was stored.
 * @param msg - The pointer-to-pointer where the stringified message reference was stored.
 */
static void util_clean_msg(cJSON **msg_obj, char **msg)
{
	if (*msg) {
		k_free(*msg);
	}
	if (*msg_obj) {
		cJSON_Delete(*msg_obj);
	}
	*msg = NULL;
	*msg_obj = NULL;
}

/**
 * @brief Generate a message using create_timestamped_data_message_object and ensure that the
 *	  created object matches expectations.
 *
 * @param ts - The timestamp to feed to create_timestamped_data_message_object.
 * @param sensor - The sensor name to pass to create_timestamped_data_message_object.
 */
static void subtest_single_msg(int64_t ts, const char *const sensor)
{
	cJSON *msg_obj;
	char *msg;
	char call_buf[200];
	char print_buf[100];

	snprintf(call_buf, sizeof(call_buf),
		 "create_timestamped_data_message_object(\"%s\") (timestamp: %lld)",
		 sensor, ts);

	/* Generate the requested message object and message. */
	util_gen_msg(ts, sensor, &msg_obj, &msg);


	/* The the result should have the expected properties. */
	zassert_true(cJSON_HasObjectItem(msg_obj, NRF_CLOUD_JSON_APPID_KEY), "The result of "
		     "%s must contain an appid. Full object: %s",
		     call_buf, msg);
	zassert_true(cJSON_HasObjectItem(msg_obj, NRF_CLOUD_JSON_MSG_TYPE_KEY), "The result of "
		     "%s must contain a message type. Full object: %s",
		     call_buf, msg);
	zassert_true(cJSON_HasObjectItem(msg_obj, NRF_CLOUD_MSG_TIMESTAMP_KEY), "The result of "
		     "%s must contain a timestamp. Full object: %s",
		     call_buf, msg);

	/* Grab the property objects. */
	cJSON *appid_obj =	cJSON_GetObjectItem(msg_obj,  NRF_CLOUD_JSON_APPID_KEY);
	cJSON *msg_type_obj =	cJSON_GetObjectItem(msg_obj,  NRF_CLOUD_JSON_MSG_TYPE_KEY);
	cJSON *timestamp_obj =	cJSON_GetObjectItem(msg_obj,  NRF_CLOUD_MSG_TIMESTAMP_KEY);

	/* Sanity check. Non-verbose assertion failure message since this should never fail. */
	zassert_not_null(appid_obj,	"appid_obj is null. This should be impossible.");
	zassert_not_null(msg_type_obj,	"msg_type_obj is null. This should be impossible.");
	zassert_not_null(timestamp_obj,	"timestamp_obj is null. This should be impossible.");

	/* The expected properties should all be of the correct type. */
	zassert_true(cJSON_IsString(appid_obj),
		     "The appid of the object returned by %s must be a string. "
		     "Full object: %s", call_buf, msg);
	zassert_true(cJSON_IsString(msg_type_obj),
		     "The message type of the object returned by %s must be a string. "
		     "Full object: %s", call_buf, msg);
	zassert_true(cJSON_IsNumber(timestamp_obj),
		     "The timestamp of the object returned by %s must be a number. "
		     "Full object: %s", call_buf, msg);

	/* The expected properties should all have the expected value. */
	zassert_true(strcmp(appid_obj->valuestring, sensor) == 0,
		     "The appid of the object returned by  %s must be \"%s\", "
		     "but was \"%s\" instead. Full object: %s",
		     call_buf, sensor, appid_obj->valuestring, msg);
	zassert_true(strcmp(msg_type_obj->valuestring, NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA) == 0,
		     "The message type of the object returned by %s must be \"%s\", "
		     "but was \"%s\" instead. Full object: %s",
		     call_buf, NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA, msg_type_obj->valuestring, msg);

	snprintf(print_buf, sizeof(print_buf), "%.3f", timestamp_obj->valuedouble);
	zassert_true(fabs(timestamp_obj->valuedouble - (double)ts) < 0.9,
		     "The timestamp of the object returned by %s "
		     "must be %lld, but was %s instead. Full object: %s",
		     call_buf, ts, print_buf, msg);

	util_clean_msg(&msg_obj, &msg);
}

/**
 * @brief Perform a single call to send_sensor_sample and verify that the sent device message
 * matches expectations.
 *
 * @param ts - The simulated timestamp to be used
 * @param sensor - The name of the sensor for whom a sample will be sent
 * @param value - The value of the sensor sample to be sent
 */
void subtest_single_sensor_sample_send_test(int64_t ts, const char *const sensor, double value)
{
	cJSON *msg_obj;
	char *msg;
	char call_buf[200];
	char expected_buf[100];
	char seen_buf[100];


	snprintf(call_buf, sizeof(call_buf),
		 "send_sensor_sample(\"%s\", %.3f) (timestamp: %lld)",
		 sensor, value, ts);

	/* Reset the sent device message cache, so that it is in a known state. */
	mocks_reset_sent_device_message();

	/* send_sensor_sample will indirectly call date_time_now, feed it a fake value. */
	mocks_set_date_time_now(ts);

	/* Call send_sensor_sample with the requested sensor type and value.
	 * This is the function under test!
	 */
	send_sensor_sample(sensor, value);

	/* Expect that send_device_message_cJSON was called. */
	zassert_true(mocks_was_device_message_sent(),
		     "%s should have called send_device_message().", call_buf);

	/* Retrieve the sent message objects. */
	mocks_get_sent_device_message(&msg_obj, &msg);

	/* The sent message object definitely not be null.
	 * Note that nullity here may indicate that send_device_message_cJSON was never called,
	 * rather than that it was passed a null device message object. Bear this in mind when
	 * diagnosing test failure.
	 *
	 * Note also that these objects are generated backwards; msg_obj is not the original
	 * message object passed to send_device_message_cJSON. It is, instead, cloned from
	 * msg, which was the stringification of the original message object passed to
	 * send_device_message_cJSON. We do this to safely break the original device message
	 * object out of its memory lifecycle.
	 */
	zassert_not_null(msg_obj, "Device message must be sent by %s "
				  "and should not be null.", call_buf);
	zassert_not_null(msg, "Stringification of device message sent by %s "
			      "should not be null.", call_buf);

	/* The result should be a cJSON object */
	zassert_true(cJSON_IsObject(msg_obj), "Device message sent by %s "
					      "should be a cJSON object.", call_buf);

	/* The result should have all expected properties.
	 * Most of these are already tested by the create_timestamped_data_message_object tests.
	 * We repeat those tests here as a smoke test to ensure that
	 * create_timestamped_data_message_object is actually being called.
	 *
	 * We do not validate the actual values they contain, except for the data entry.
	 */
	zassert_true(cJSON_HasObjectItem(msg_obj, NRF_CLOUD_JSON_APPID_KEY), "Device message "
		     "sent by %s must contain an appid. Full object: %s",
		     call_buf, msg);
	zassert_true(cJSON_HasObjectItem(msg_obj, NRF_CLOUD_JSON_MSG_TYPE_KEY), "Device message "
		     "sent by %s must contain a message type. Full object: %s",
		     call_buf, msg);
	zassert_true(cJSON_HasObjectItem(msg_obj, NRF_CLOUD_MSG_TIMESTAMP_KEY), "Device message "
		     "sent by %s must contain a timestamp. Full object: %s",
		     call_buf, msg);
	zassert_true(cJSON_HasObjectItem(msg_obj, NRF_CLOUD_JSON_DATA_KEY), "Device message "
		     "sent by %s must contain a data entry. Full object: %s",
		     call_buf, msg);

	/* Get the data entry object. */
	cJSON *data_obj = cJSON_GetObjectItem(msg_obj,  NRF_CLOUD_JSON_DATA_KEY);

	zassert_true(cJSON_IsNumber(data_obj), "The data entry in the device message sent by %s "
		     "must be a number. Full object: %s", call_buf, msg);

	/* The data entry should have the expected value. */
	snprintf(expected_buf, sizeof(expected_buf), "%.3f", value);
	snprintf(seen_buf, sizeof(seen_buf), "%.3f", data_obj->valuedouble);
	zassert_true(fabs(data_obj->valuedouble - (double)value) < 0.001,
		     "The data entry in the device message sent by %s "
		     "must be %s, but was %s instead. Full object: %s",
		     call_buf, expected_buf, seen_buf, msg);

	/* Clean up. */
	util_clean_msg(&msg_obj, &msg);
}


/**
 * @brief Call create_timestamped_data_message_object repeatedly with a variety of inputs,
 * making sure that the created objects always meet expectations.
 */
static void test_create_timestamped_data_message_object(void)
{
	/* Make sure cJSON is using the correct free/malloc. */
	cJSON_Init();

	/* Generate a variety of messages and make sure they conform to expectations. */
	subtest_single_msg(TEST_TIMESTAMP_A, TEST_SENSOR_NAME_A);
	subtest_single_msg(TEST_TIMESTAMP_B, TEST_SENSOR_NAME_B);
	subtest_single_msg(TEST_TIMESTAMP_A, TEST_SENSOR_NAME_B);
	subtest_single_msg(TEST_TIMESTAMP_B, TEST_SENSOR_NAME_A);
}


/**
 * @brief Simulate a date_time_now failure while calling create_timestamped_data_message_object
 * and ensure that create_timestamped_data_message_object fails.
 */
static void test_create_timestamped_data_message_object_fail_date_time(void)
{
	/* Make sure cJSON is using the correct free/malloc. */
	cJSON_Init();

	/* Set date_time_now up to fail. */
	mocks_set_date_time_now_fail();

	/* Attempt to generate a timestamped data message object.
	 * This is the function under test!
	 */
	cJSON *msg_obj = create_timestamped_data_message_object(TEST_SENSOR_NAME_A);

	/* This should not succeed. */
	zassert_is_null(msg_obj, "If date_time_now fails, so should "
				 "create_timestamped_data_message_object.");
}

/**
 * @brief attempt to call create_timestamped_data_message_object while simulating cJSON failure at
 * various points.
 *
 * cJSON failure is simulated by overriding cJSON's malloc function.
 *
 * This does not test failure at all possible points, but at some known / characterized points.
 */
static void test_create_timestamped_data_message_object_fail_cJSON(void)
{
	cJSON *msg_obj;

	/* Make cJSON fail immediately. */
	mocks_cJSON_Init_mocked(0);

	/* Set a valid timestamp. */
	mocks_set_date_time_now(TEST_TIMESTAMP_A);

	/* Attempt to send a sensor sample and expect failure. */
	msg_obj = create_timestamped_data_message_object(TEST_SENSOR_NAME_A);

	/* This should not succeed. */
	zassert_is_null(msg_obj, "cJSON failure should cause "
				 "create_timestamped_data_message_object to fail.");


	/* Make cJSON fail after one malloc. */
	mocks_cJSON_Init_mocked(1);

	/* Set a valid timestamp. */
	mocks_set_date_time_now(TEST_TIMESTAMP_A);

	/* Attempt to send a sensor sample and expect failure. */
	msg_obj = create_timestamped_data_message_object(TEST_SENSOR_NAME_A);

	/* This should not succeed */
	zassert_is_null(msg_obj, "cJSON failure should cause "
				 "create_timestamped_data_message_object to fail.");


	/* Make cJSON fail after two mallocs. */
	mocks_cJSON_Init_mocked(2);

	/* Set a valid timestamp. */
	mocks_set_date_time_now(TEST_TIMESTAMP_A);

	/* Attempt to send a sensor sample and expect failure. */
	msg_obj = create_timestamped_data_message_object(TEST_SENSOR_NAME_A);

	/* This should not succeed. */
	zassert_is_null(msg_obj, "cJSON failure should cause "
				 "create_timestamped_data_message_object to fail.");
}

/**
 * @brief Call create_timestamped_data_message_object with invalid parameters and make sure
 * that it fails.
 */
static void test_create_timestamped_data_message_object_fail_parameters(void)
{
	/* Make sure cJSON is using the correct free/malloc. */
	cJSON_Init();

	/* Set a valid timestamp. */
	mocks_set_date_time_now(TEST_TIMESTAMP_A);

	/* Attempt to generate a timestamped data message object
	 * This is the function under test!
	 */
	cJSON *msg_obj = create_timestamped_data_message_object(NULL);

	/* This should not succeed. */
	zassert_is_null(msg_obj, "create_timestamped_data_message_object should fail if passed "
				 "a NULL sensor name.");
}

/**
 * @brief Perform a number of paired calls to create_timestamped_data_message_object, each pair
 * with either identical or differing inputs, and ensure that the returned message objects either
 * differ or are identical once stringified.
 */
static void test_create_timestamped_data_message_object_deltas(void)
{
	cJSON *msg_obj_a;
	cJSON *msg_obj_b;
	char *msg_a;
	char *msg_b;

	/* Make sure cJSON is using the correct free/malloc. */
	cJSON_Init();

	/* Create two message objects with identical data. */
	util_gen_msg(TEST_TIMESTAMP_A, TEST_SENSOR_NAME_A, &msg_obj_a, &msg_a);
	util_gen_msg(TEST_TIMESTAMP_A, TEST_SENSOR_NAME_A, &msg_obj_b, &msg_b);

	/* Both objects should be identical */;
	zassert_true(strcmp(msg_a, msg_b) == 0, "Calling create_timestamped_data_message_object "
						"with identical inputs should yield identical "
						"results.");

	/* Cleanup */
	util_clean_msg(&msg_obj_a, &msg_a);
	util_clean_msg(&msg_obj_b, &msg_b);


	/* Create two message objects with differing timestamps. */
	util_gen_msg(TEST_TIMESTAMP_A, TEST_SENSOR_NAME_A, &msg_obj_a, &msg_a);
	util_gen_msg(TEST_TIMESTAMP_B, TEST_SENSOR_NAME_A, &msg_obj_b, &msg_b);

	/* Both objects should differ. */;
	zassert_false(strcmp(msg_a, msg_b) == 0, "Calling create_timestamped_data_message_object "
						 "with differing inputs should yield differing "
						 "results.");

	/* Cleanup. */
	util_clean_msg(&msg_obj_a, &msg_a);
	util_clean_msg(&msg_obj_b, &msg_b);


	/* Create two message objects with differing sensor names. */
	util_gen_msg(TEST_TIMESTAMP_A, TEST_SENSOR_NAME_A, &msg_obj_a, &msg_a);
	util_gen_msg(TEST_TIMESTAMP_A, TEST_SENSOR_NAME_B, &msg_obj_b, &msg_b);

	/* Both objects should differ. */;
	zassert_false(strcmp(msg_a, msg_b) == 0, "Calling create_timestamped_data_message_object "
						 "with differing inputs should yield differing "
						 "results.");

	/* Cleanup. */
	util_clean_msg(&msg_obj_a, &msg_a);
	util_clean_msg(&msg_obj_b, &msg_b);


	/* Create two message objects with differing timestamps and differing sensor names. */
	util_gen_msg(TEST_TIMESTAMP_A, TEST_SENSOR_NAME_A, &msg_obj_a, &msg_a);
	util_gen_msg(TEST_TIMESTAMP_B, TEST_SENSOR_NAME_B, &msg_obj_b, &msg_b);

	/* Both objects should differ. */;
	zassert_false(strcmp(msg_a, msg_b) == 0, "Calling create_timestamped_data_message_object "
						 "with differing inputs should yield differing "
						 "results.");

	/* Cleanup. */
	util_clean_msg(&msg_obj_a, &msg_a);
	util_clean_msg(&msg_obj_b, &msg_b);
}

/**
 * @brief Perform several calls to send_sensor_sample with various input parameters and ensure that
 * the sent device messages match expectations.
 */
void test_send_sensor_sample(void)
{
	/* Make sure cJSON is using the correct free/malloc. */
	cJSON_Init();

	subtest_single_sensor_sample_send_test(TEST_TIMESTAMP_A, TEST_SENSOR_NAME_A,
					      TEST_SENSOR_VALUE_A);
	subtest_single_sensor_sample_send_test(TEST_TIMESTAMP_B, TEST_SENSOR_NAME_B,
					      TEST_SENSOR_VALUE_B);
	subtest_single_sensor_sample_send_test(TEST_TIMESTAMP_A, TEST_SENSOR_NAME_B,
					      TEST_SENSOR_VALUE_C);
	subtest_single_sensor_sample_send_test(TEST_TIMESTAMP_B, TEST_SENSOR_NAME_A,
					      TEST_SENSOR_VALUE_D);
}


/**
 * @brief Perform a single call to send_sensor_sample expecting it to fail.
 *
 * @param sensor - The name of the sensor for whom we will attempt to send a sample
 * @param value - The value of the sensor sample to be sent
 */
void subtest_single_sensor_sample_send_test_fail(const char *const sensor, double value,
						const char *const failure_mode)
{
	/* Reset the sent device message cache, so that it is in a known state. */
	mocks_reset_sent_device_message();

	/* Attempt to send a sensor sample.
	 * This is the function under test!
	 */
	int res = send_sensor_sample(sensor, value);

	/* This should not succeed. */
	zassert_not_equal(res, 0, "If %s, calls to send_sensor_sample should fail.", failure_mode);

	/* No attempt should have been made to send a sensor sample device message. */
	zassert_false(mocks_was_device_message_sent(),
			      "If send_sensor_sample fails because %s, it should not attempt to "
			      "send a device message. ");
}


/**
 * @brief Perform a single call to send_sensor_sample while simulating a
 * date_time_now failure, expecting it to fail.
 */
void test_send_sensor_sample_fail_date_time(void)
{
	/* Make sure cJSON is using the correct free/malloc. */
	cJSON_Init();

	/* Set date_time_now up to fail. */
	mocks_set_date_time_now_fail();

	/* Attempt to send a sensor sample and expect failure. */
	subtest_single_sensor_sample_send_test_fail(TEST_SENSOR_NAME_A, TEST_SENSOR_VALUE_A,
						   "date_time_now fails");
}

/**
 * @brief attempt to call send_sensor_sample while simulating cJSON failure at various points.
 *
 * cJSON failure is simulated by overriding cJSON's malloc function.
 *
 * This does not test failure at all possible points, but at some known / characterized points.
 */
void test_send_sensor_sample_fail_cJSON(void)
{
	/* Make cJSON fail immediately. */
	mocks_cJSON_Init_mocked(0);

	/* Set a valid timestamp. */
	mocks_set_date_time_now(TEST_TIMESTAMP_A);

	/* Attempt to send a sensor sample and expect failure. */
	subtest_single_sensor_sample_send_test_fail(TEST_SENSOR_NAME_A, TEST_SENSOR_VALUE_A,
						   "cJSON fails immediately");

	/* Make cJSON fail after a few calls. */
	mocks_cJSON_Init_mocked(3);

	/* Set a valid timestamp. */
	mocks_set_date_time_now(TEST_TIMESTAMP_A);

	/* Attempt to send a sensor sample and expect failure. */
	subtest_single_sensor_sample_send_test_fail(TEST_SENSOR_NAME_A, TEST_SENSOR_VALUE_A,
						   "cJSON fails after a few calls");

	/* Make cJSON fail after half a dozen calls. */
	mocks_cJSON_Init_mocked(6);

	/* Set a valid timestamp. */
	mocks_set_date_time_now(TEST_TIMESTAMP_A);

	/* Attempt to send a sensor sample and expect failure. */
	subtest_single_sensor_sample_send_test_fail(TEST_SENSOR_NAME_A, TEST_SENSOR_VALUE_A,
						   "cJSON fails after half a dozen calls");
}

/**
 * @brief attempt to call send_sensor_sample with bad parameters, expecting it to fail.
 */
void test_send_sensor_sample_fail_parameters(void)
{
	/* Make sure cJSON is using the correct free/malloc. */
	cJSON_Init();

	/* Set date_time_now to fail */
	mocks_set_date_time_now_fail();

	/* Attempt to send a sensor sample and expect failure. */
	subtest_single_sensor_sample_send_test_fail(TEST_SENSOR_NAME_A, TEST_SENSOR_VALUE_A,
						   "date_time_now fails");


	/* Make sure cJSON is using the correct free/malloc. */
	cJSON_Init();

	/* Set a valid timestamp. */
	mocks_set_date_time_now(TEST_TIMESTAMP_A);

	/* Attempt to send a sensor sample and expect failure. */
	subtest_single_sensor_sample_send_test_fail(NULL, TEST_SENSOR_VALUE_A,
						   "invalid sensor name is given");
}

void test_main(void)
{
	ztest_test_suite(application,
		ztest_unit_test(test_create_timestamped_data_message_object),
		ztest_unit_test(test_create_timestamped_data_message_object_deltas),
		ztest_unit_test(test_create_timestamped_data_message_object_fail_date_time),
		ztest_unit_test(test_create_timestamped_data_message_object_fail_cJSON),
		ztest_unit_test(test_create_timestamped_data_message_object_fail_parameters),
		ztest_unit_test(test_send_sensor_sample),
		ztest_unit_test(test_send_sensor_sample_fail_date_time),
		ztest_unit_test(test_send_sensor_sample_fail_cJSON),
		ztest_unit_test(test_send_sensor_sample_fail_parameters)
	);
	ztest_run_test_suite(application);
}
