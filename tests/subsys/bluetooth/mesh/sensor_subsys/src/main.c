/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <math.h>
#include <stdint.h>

#include <ztest.h>
#include <bluetooth/mesh/properties.h>
#include <bluetooth/mesh/sensor_types.h>
#include <sensor.h> // private header from the source folder

enum property_type {
	PERCENTAGE8,
	COUNT16,
	BOOLEAN,
	TIME_SECOND16
};

/****************** mock section **********************************/
/****************** mock section **********************************/

/****************** callback section ******************************/
/****************** callback section ******************************/

static int64_t raw_scalar_value_get(int64_t r, int64_t m, int64_t d, int64_t b)
{
	return r / (m * pow(10.0, d) * pow(2.0, b));
}

static void sensor_type_sanitize(const struct bt_mesh_sensor_type *sensor_type)
{
	zassert_not_null(sensor_type, "NULL sensor type pointer");
	zassert_not_null(sensor_type->channels, "NULL sensor channels pointer");
	zassert_true(sensor_type->channel_count > 0, "Wrong channel number");

	for (int i = 0; i < sensor_type->channel_count; i++) {
		printk("Sensor channel name: %s\n", sensor_type->channels[i].name);
	}
}

static void checking_proceed(const struct bt_mesh_sensor_type *sensor_type,
			struct net_buf_simple *buf,
			struct sensor_value *in_value,
			enum property_type type,
			void *expected)
{
	struct sensor_value out_value = {0};
	size_t size;
	int err;

	switch (type) {
	case PERCENTAGE8:
	case BOOLEAN:
		size = sizeof(uint8_t);
		break;
	case COUNT16:
	case TIME_SECOND16:
		size = sizeof(uint16_t);
		break;
	default:
		zassert_unreachable("Unknown property type");
		break;
	}

	err = sensor_value_encode(buf, sensor_type, in_value);
	zassert_ok(err, "Encoding failed with error code: %i", err);

	zassert_mem_equal(&buf->data[1], expected, size,
		"Encoded value is not equal expected");

	(void)net_buf_simple_pull_u8(buf);

	err = sensor_value_decode(buf, sensor_type, &out_value);
	zassert_ok(err, "Decoding failed with error code: %i", err);

	zassert_equal(in_value->val1, out_value.val1,
			"Encoded value val1: %i is not equal decoded: %i",
			in_value->val1, out_value.val1);
	zassert_equal(in_value->val2, out_value.val2,
			"Encoded value val2: %i is not equal decoded: %i",
			in_value->val2, out_value.val2);
}

static void percentage8_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	uint8_t test_vector[] = {0, 25, 50, 75, 100, 0xFF};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_SENSOR_OP_SETTING_SET,
					BT_MESH_SENSOR_MSG_MAXLEN_SETTING_SET);
		bt_mesh_model_msg_init(&msg, BT_MESH_SENSOR_OP_SETTING_SET);

		in_value.val1 = test_vector[i];

		uint8_t expected = test_vector[i] == 0xFF ?
			0xFF : raw_scalar_value_get(test_vector[i], 1, 0, -1);

		checking_proceed(sensor_type, &msg, &in_value, PERCENTAGE8,
				&expected);
	}
}

static void count16_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	uint16_t test_vector[] = {0, 0x000F, 0x00FF, 0x0FFF, 0xFFFF};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_SENSOR_OP_SETTING_SET,
					BT_MESH_SENSOR_MSG_MAXLEN_SETTING_SET);
		bt_mesh_model_msg_init(&msg, BT_MESH_SENSOR_OP_SETTING_SET);

		in_value.val1 = test_vector[i];

		uint16_t expected = raw_scalar_value_get(test_vector[i], 1, 0, 0);

		checking_proceed(sensor_type, &msg, &in_value, COUNT16,
				&expected);
	}
}

static void boolean_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	uint8_t test_vector[] = {0, 1};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_SENSOR_OP_SETTING_SET,
					BT_MESH_SENSOR_MSG_MAXLEN_SETTING_SET);
		bt_mesh_model_msg_init(&msg, BT_MESH_SENSOR_OP_SETTING_SET);

		in_value.val1 = test_vector[i];

		uint8_t expected = test_vector[i];

		checking_proceed(sensor_type, &msg, &in_value, BOOLEAN,
				&expected);
	}
}

static void time_second16_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	uint16_t test_vector[] = {0, 0x000F, 0x00FF, 0x0FFF, 0xFFFF};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_SENSOR_OP_SETTING_SET,
					BT_MESH_SENSOR_MSG_MAXLEN_SETTING_SET);
		bt_mesh_model_msg_init(&msg, BT_MESH_SENSOR_OP_SETTING_SET);

		in_value.val1 = test_vector[i];

		uint16_t expected = raw_scalar_value_get(test_vector[i], 1, 0, 0);

		checking_proceed(sensor_type, &msg, &in_value, TIME_SECOND16,
				&expected);
	}
}

static void test_motion_sensor(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_MOTION_SENSED);
	sensor_type_sanitize(sensor_type);
	percentage8_check(sensor_type);
}

static void test_motion_threshold(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_MOTION_THRESHOLD);
	sensor_type_sanitize(sensor_type);
	percentage8_check(sensor_type);
}

static void test_people_count(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PEOPLE_COUNT);
	sensor_type_sanitize(sensor_type);
	count16_check(sensor_type);
}

static void test_presence_detected(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENCE_DETECTED);
	sensor_type_sanitize(sensor_type);
	boolean_check(sensor_type);
}

static void test_time_since_motion_sensed(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_TIME_SINCE_MOTION_SENSED);
	sensor_type_sanitize(sensor_type);
	time_second16_check(sensor_type);
}

static void test_time_since_presence_detected(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_TIME_SINCE_PRESENCE_DETECTED);
	sensor_type_sanitize(sensor_type);
	time_second16_check(sensor_type);
}

void test_main(void)
{
	ztest_test_suite(sensor_types_test,
			ztest_unit_test(test_motion_sensor),
			ztest_unit_test(test_motion_threshold),
			ztest_unit_test(test_people_count),
			ztest_unit_test(test_presence_detected),
			ztest_unit_test(test_time_since_motion_sensed),
			ztest_unit_test(test_time_since_presence_detected)
			 );

	ztest_run_test_suite(sensor_types_test);
}
