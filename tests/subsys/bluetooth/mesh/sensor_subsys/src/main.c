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

/****************** mock section **********************************/
/****************** mock section **********************************/

/****************** callback section ******************************/
/****************** callback section ******************************/
static int raw_scalar_value_get(int r, int m, int d, int b)
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

static void test_motion_sensor(void)
{
	const struct bt_mesh_sensor_type *sensor_type;
	struct sensor_value in_value = {0};
	struct sensor_value out_value = {0};
	int err;
	int test_vector[] = {0, 25, 50, 75, 100, 0xFF};

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_MOTION_SENSED);
	sensor_type_sanitize(sensor_type);

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_SENSOR_OP_SETTING_SET,
					BT_MESH_SENSOR_MSG_MAXLEN_SETTING_SET);
		bt_mesh_model_msg_init(&msg, BT_MESH_SENSOR_OP_SETTING_SET);

		in_value.val1 = test_vector[i];

		int expected = test_vector[i] == 0xFF ?
			0xFF : raw_scalar_value_get(test_vector[i], 1, 0, -1);

		err = sensor_value_encode(&msg, sensor_type, &in_value);
		zassert_ok(err, "Encoding failed with error code: %i", err);

		zassert_equal(msg.data[1], expected,
			"Encoded value: %i is not equal expected: %i",
			msg.data[1], expected);

		(void)net_buf_simple_pull_u8(&msg);

		err = sensor_value_decode(&msg, sensor_type, &out_value);
		zassert_ok(err, "Decoding failed with error code: %i", err);

		zassert_equal(in_value.val1, out_value.val1,
				"Encoded value: %i is not equal decoded: %i",
				in_value.val1, out_value.val1);
	}
}

void test_main(void)
{
	ztest_test_suite(sensor_types_test,
			ztest_unit_test(test_motion_sensor)
			 );

	ztest_run_test_suite(sensor_types_test);
}
