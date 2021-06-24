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

#include "model_utils.h"

BUILD_ASSERT(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__,
	     "This test is only supported on little endian platforms");

/****************** types section **********************************/

struct __packed uint24_t  { uint32_t v:24; };

/****************** types section **********************************/


/****************** mock section **********************************/
/****************** mock section **********************************/

/****************** callback section ******************************/
/****************** callback section ******************************/

static int64_t int_pow(int a, int b)
{
	int64_t res = 1;

	for (int i = 0; i < b; i++) {
		res *= a;
	}

	return res;
}

static int64_t raw_scalar_value_get(int64_t r, int m, int d, int b)
{
	/* Powers < 1 will be clamped to 1, so this works for both
	 * positive and negative d and b
	 */
	int64_t div = m * int_pow(10, d) * int_pow(2, b);
	int64_t mul = int_pow(10, -d) * int_pow(2, -b);

	return ROUNDED_DIV(r, div) * mul;
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

static void encoding_checking_proceed(const struct bt_mesh_sensor_type *sensor_type,
				      const struct sensor_value *value,
				      const void *expected,
				      size_t size)
{
	int err;

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SENSOR_OP_SETTING_SET,
				BT_MESH_SENSOR_MSG_MAXLEN_SETTING_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_SENSOR_OP_SETTING_SET);

	err = sensor_value_encode(&buf, sensor_type, value);
	zassert_ok(err, "Encoding failed with error code: %i", err);

	zassert_mem_equal(&buf.data[1], expected, size,
		"Encoded value is not equal expected");
}

static void decoding_checking_proceed_with_tolerance(
			const struct bt_mesh_sensor_type *sensor_type,
			const void *value,
			size_t size,
			const struct sensor_value *expected,
			double tolerance)
{
	struct sensor_value out_value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX] = {};
	int err;

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SENSOR_OP_SETTING_SET,
				BT_MESH_SENSOR_MSG_MAXLEN_SETTING_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_SENSOR_OP_SETTING_SET);

	(void)net_buf_simple_add_mem(&buf, value, size);
	(void)net_buf_simple_pull_u8(&buf);

	err = sensor_value_decode(&buf, sensor_type, out_value);
	zassert_ok(err, "Decoding failed with error code: %i", err);

	for (int i = 0; i < sensor_type->channel_count; i++) {
		if (tolerance == 0.0 || (expected[i].val1 == 0 && expected[i].val2 == 0)) {
			zassert_equal(expected[i].val1, out_value[i].val1,
				"Channel: %i - encoded value val1: %i is not equal decoded: %i",
				i, expected[i].val1, out_value[i].val1);
			zassert_equal(expected[i].val2, out_value[i].val2,
				"Channel: %i - encoded value val2: %i is not equal decoded: %i",
				i, expected[i].val2, out_value[i].val2);
		} else {
			double in_d = expected[i].val1 + expected[i].val2 * 0.000001;
			double out_d = out_value[i].val1 + out_value[i].val2 * 0.000001;
			double ratio = out_d / in_d;

			zassert_within(ratio, 1.0, tolerance,
				"Channel: %i - decoded value %i.%i is more than %f away "
				"from encoded value %i.%i, and is outside tolerance.",
				i, out_value[i].val1, out_value[i].val2,
				tolerance, expected[i].val1, expected[i].val2);
		}
	}
}

static void decoding_checking_proceed(const struct bt_mesh_sensor_type *sensor_type,
				      const void *value,
				      size_t size,
				      const struct sensor_value *expected)
{
	decoding_checking_proceed_with_tolerance(sensor_type, value, size, expected, 0.0);
}

static void invalid_encoding_checking_proceed(const struct bt_mesh_sensor_type *sensor_type,
					      const struct sensor_value *value)
{
	int err;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_SENSOR_OP_SETTING_SET,
				BT_MESH_SENSOR_MSG_MAXLEN_SETTING_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_SENSOR_OP_SETTING_SET);

	err = sensor_value_encode(&msg, sensor_type, value);
	zassert_equal(err, -ERANGE, "Invalid error code when encoding: %i", err);
}

static void invalid_decoding_checking_proceed(const struct bt_mesh_sensor_type *sensor_type,
					      const void *value,
					      size_t size)
{
	struct sensor_value out_value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX] = {};
	int err;

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SENSOR_OP_SETTING_SET,
				BT_MESH_SENSOR_MSG_MAXLEN_SETTING_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_SENSOR_OP_SETTING_SET);

	(void)net_buf_simple_add_mem(&buf, value, size);
	(void)net_buf_simple_pull_u8(&buf);

	err = sensor_value_decode(&buf, sensor_type, out_value);
	zassert_equal(err, -ERANGE, "Invalid error code when decoding: %i", err);
}

static void percentage8_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	uint8_t test_vector[] = {0, 25, 50, 75, 100, 0xFF};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		in_value.val1 = test_vector[i];

		uint8_t expected = test_vector[i] == 0xFF ?
			0xFF : raw_scalar_value_get(test_vector[i], 1, 0, -1);

		encoding_checking_proceed(sensor_type, &in_value, &expected, 1);
		decoding_checking_proceed(sensor_type, &expected, 1, &in_value);
	}
}

static void count16_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	uint16_t test_vector[] = {0, 0x000F, 0x00FF, 0x0FFF, 0xFFFF};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		in_value.val1 = test_vector[i];

		uint16_t expected = raw_scalar_value_get(test_vector[i], 1, 0, 0);

		encoding_checking_proceed(sensor_type, &in_value, &expected, 2);
		decoding_checking_proceed(sensor_type, &expected, 2, &in_value);
	}
}

static void boolean_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	uint8_t test_vector[] = {0, 1};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		in_value.val1 = test_vector[i];

		uint8_t expected = test_vector[i];

		encoding_checking_proceed(sensor_type, &in_value, &expected, 1);
		decoding_checking_proceed(sensor_type, &expected, 1, &in_value);
	}
}

static void time_second16_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	uint16_t test_vector[] = {0, 0x000F, 0x00FF, 0x0FFF, 0xFFFF};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		in_value.val1 = test_vector[i];

		uint16_t expected = raw_scalar_value_get(test_vector[i], 1, 0, 0);

		encoding_checking_proceed(sensor_type, &in_value, &expected, 2);
		decoding_checking_proceed(sensor_type, &expected, 2, &in_value);
	}
}

static void chromaticity_coordinates_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented[2];
		uint16_t raw[2];
	} test_vector[] = {
		{{{0, 250000}, {0, 750000}}, {16384, 49152}},
		{{{0, 500000}, {0, 625000}}, {32768, 40960}},
		{{{0, 875000}, {0, 125000}}, {57344, 8192}},
	};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		encoding_checking_proceed(sensor_type, &test_vector[i].represented[0],
					  &test_vector[i].raw[0], 4);
		decoding_checking_proceed(sensor_type, &test_vector[i].raw[0], 4,
					  &test_vector[i].represented[0]);
	}

	/* Test invalid range on encoding. */
	struct sensor_value invalid_test_vector[][2] = {
		{{0, -100000}, {0, 250000}},
		{{1, 0}, {0, 250000}},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_test_vector); i++) {
		invalid_encoding_checking_proceed(sensor_type, invalid_test_vector[i]);
	}
}

static void illuminance_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented;
		uint32_t raw;
	} test_vector[] = {
		{{0, 0}, 0},
		{{83886, 70000}, 0x7FFFFF},
		{{3000, 0}, 0x493E0},
		{{10000, 500000}, 0xF4272},
		{{167772, 140000}, 0xFFFFFE},
		{{0xFFFFFF, 0}, 0xFFFFFF},
	};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		encoding_checking_proceed(sensor_type, &test_vector[i].represented,
					  &test_vector[i].raw, 3);
		decoding_checking_proceed(sensor_type, &test_vector[i].raw, 3,
					  &test_vector[i].represented);
	}

	/* Test invalid range on encoding. */
	struct sensor_value invalid_test_vector[] = {
		{-1, 0},
		{167772, 150000},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_test_vector); i++) {
		invalid_encoding_checking_proceed(sensor_type, &invalid_test_vector[i]);
	}
}

static void correlated_color_temp_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented;
		uint16_t raw;
	} test_vector[] = {
		{{800, 0}, 800},
		{{1234, 0}, 1234},
		{{65534, 0}, 65534},
		{{65535, 0}, 65535},
	};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		encoding_checking_proceed(sensor_type, &test_vector[i].represented,
					  &test_vector[i].raw, 2);
		decoding_checking_proceed(sensor_type, &test_vector[i].raw, 2,
					  &test_vector[i].represented);
	}

	/* Test invalid range on encoding. */
	struct sensor_value invalid_encoding_test_vector[] = {
		{0, 0},
		{799, 0},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_encoding_test_vector); i++) {
		invalid_encoding_checking_proceed(sensor_type, &invalid_encoding_test_vector[i]);
	}

	/* Test invalid range on decoding. */
	uint16_t invalid_decoding_test_vector[] = {
		0,
		799,
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_decoding_test_vector); i++) {
		invalid_decoding_checking_proceed(sensor_type, &invalid_decoding_test_vector[i], 2);
	}
}

static void luminous_flux_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented;
		uint16_t raw;
	} test_vector[] = {
		{{0, 0}, 0},
		{{800, 0}, 800},
		{{1234, 0}, 1234},
		{{65534, 0}, 65534},
		{{65535, 0}, 65535},
	};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		encoding_checking_proceed(sensor_type, &test_vector[i].represented,
					  &test_vector[i].raw, 2);
		decoding_checking_proceed(sensor_type, &test_vector[i].raw, 2,
					  &test_vector[i].represented);
	}

	/* Test invalid range. */
	struct sensor_value invalid_encoding_test_vector[] = {
		{-1, 0},
		{65536, 0},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_encoding_test_vector); i++) {
		invalid_encoding_checking_proceed(sensor_type, &invalid_encoding_test_vector[i]);
	}
}

static void luminous_flux_range_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented[2];
		uint16_t raw[2];
	} test_vector[] = {
		{{{0, 0}, {1, 0}}, {0, 1}},
		{{{800, 0}, {400, 0}}, {800, 400}},
		{{{1234, 0}, {5678, 0}}, {1234, 5678}},
		{{{65534, 0}, {65535, 0}}, {65534, 65535}},
	};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		encoding_checking_proceed(sensor_type, &test_vector[i].represented[0],
					  &test_vector[i].raw[0], 4);
		decoding_checking_proceed(sensor_type, &test_vector[i].raw[0], 4,
					  &test_vector[i].represented[0]);
	}

	/* Test invalid range. */
	struct sensor_value invalid_encoding_test_vector[][2] = {
		{{-1, 0}, {0, 0}},
		{{65536, 0}, {65534, 0}},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_encoding_test_vector); i++) {
		invalid_encoding_checking_proceed(sensor_type, &invalid_encoding_test_vector[i][0]);
	}
}

static void chromatic_distance_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented;
		uint32_t raw;
	} test_vector[] = {
		{{0, -50000}, -5000},
		{{0, 0}, 0},
		{{0, -10}, -1},
		{{0, 10}, 1},
		{{0, 50000}, 5000},
	};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		encoding_checking_proceed(sensor_type, &test_vector[i].represented,
					  &test_vector[i].raw, 2);
		decoding_checking_proceed(sensor_type, &test_vector[i].raw, 2,
					  &test_vector[i].represented);
	}

	/* Test invalid range on encoding. */
	struct sensor_value invalid_encoding_test_vector[] = {
		{0, -60000},
		{0, 60000},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_encoding_test_vector); i++) {
		invalid_encoding_checking_proceed(sensor_type, &invalid_encoding_test_vector[i]);
	}

	/* Test invalid range on decoding. */
	uint16_t invalid_decoding_test_vector[] = {
		5001,
		-5001,
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_decoding_test_vector); i++) {
		invalid_decoding_checking_proceed(sensor_type, &invalid_decoding_test_vector[i], 2);
	}

	/* 0xFFFF defined as 'value is not known' is in _valid_ range and can't be encoded or
	 * decoded correctly.
	 */
}

static void percentage8_illuminance_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented[3];
		struct __packed {
			uint8_t p;
			struct uint24_t i[2];
		} raw;
	} test_vector[] = {
		{{{1, 0}, {0, 0}, {83886, 70000}}, {2, {{0}, {0x7FFFFF}}}},
		{{{25, 0}, {10000, 500000}, {3000, 0}}, {50, {{0xF4272}, {0x493E0}}}},
	};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		encoding_checking_proceed(sensor_type, &test_vector[i].represented[0],
					  &test_vector[i].raw, 7);
		decoding_checking_proceed(sensor_type, &test_vector[i].raw, 7,
					  &test_vector[i].represented[0]);
	}

	/* Test invalid range on encoding. */
	struct sensor_value invalid_encoding_test_vector[][3] = {
		{{256, 0}, {0x1000000, 0}, {83886, 70000}},
		{{-1, 0}, {0, 0}, {-1, 0}},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_encoding_test_vector); i++) {
		invalid_encoding_checking_proceed(sensor_type, &invalid_encoding_test_vector[i][0]);
	}

	/* Test invalid range on decoding. */
	struct __packed {
		uint8_t p;
		struct uint24_t i[2];
	} invalid_decoding_test_vector[] = {
		{201, {{0x1FFFFF}, {0x7FFFFF}}},
		{253, {{0}, {0xFF}}},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_decoding_test_vector); i++) {
		invalid_decoding_checking_proceed(sensor_type, &invalid_decoding_test_vector[i], 7);
	}
}

static void time_hour_24_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented;
		uint32_t raw;
	} test_vector[] = {
		{{0, 0}, 0},
		{{1000, 0}, 1000},
		{{12345678, 0}, 12345678},
		{{16777214, 0}, 16777214},
		{{0xFFFFFF, 0}, 0xFFFFFF},
	};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		encoding_checking_proceed(sensor_type, &test_vector[i].represented,
					  &test_vector[i].raw, 3);
		decoding_checking_proceed(sensor_type, &test_vector[i].raw, 3,
					  &test_vector[i].represented);
	}

	/* Test invalid range on encoding. */
	struct sensor_value invalid_encoding_test_vector[] = {
		{-1, 0},
		{0x1000000, 0},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_encoding_test_vector); i++) {
		invalid_encoding_checking_proceed(sensor_type, &invalid_encoding_test_vector[i]);
	}
}

static void luminous_efficacy_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented;
		uint16_t raw;
	} test_vector[] = {
		{{0, 0}, 0},
		{{450, 500000}, 4505},
		{{900, 0}, 9000},
		{{1300, 200000}, 13002},
		{{1800, 0}, 18000},
		{{0xFFFF, 0}, 0xFFFF},
	};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		encoding_checking_proceed(sensor_type, &test_vector[i].represented,
					  &test_vector[i].raw, 2);
		decoding_checking_proceed(sensor_type, &test_vector[i].raw, 2,
					  &test_vector[i].represented);
	}

	/* Test invalid range on encoding. */
	struct sensor_value invalid_encoding_test_vector[] = {
		{1800, 100000},
		{0xFFFE, 0},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_encoding_test_vector); i++) {
		invalid_encoding_checking_proceed(sensor_type, &invalid_encoding_test_vector[i]);
	}

	/* Test invalid range on decoding. */
	uint32_t invalid_decoding_test_vector[] = {
		18001,
		0xFFFE,
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_decoding_test_vector); i++) {
		invalid_decoding_checking_proceed(sensor_type, &invalid_decoding_test_vector[i], 2);
	}
}

static void luminous_energy_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented;
		uint32_t raw;
	} test_vector[] = {
		{{0, 0}, 0},
		{{54321, 0}, 54321},
		{{8765432, 0}, 8765432},
		{{16777214, 0}, 16777214},
		{{0xFFFFFF, 0}, 0xFFFFFF},
	};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		encoding_checking_proceed(sensor_type, &test_vector[i].represented,
					  &test_vector[i].raw, 3);
		decoding_checking_proceed(sensor_type, &test_vector[i].raw, 3,
					  &test_vector[i].represented);
	}

	/* Test invalid range on encoding. */
	struct sensor_value invalid_encoding_test_vector[] = {
		{-1, 0},
		{0x1000000, 0},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_encoding_test_vector); i++) {
		invalid_encoding_checking_proceed(sensor_type, &invalid_encoding_test_vector[i]);
	}
}

static void luminous_exposure_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented;
		uint32_t raw;
	} test_vector[] = {
		{{0, 0}, 0},
		{{54321, 0}, 54321},
		{{8765432, 0}, 8765432},
		{{16777214, 0}, 16777214},
		{{0xFFFFFF, 0}, 0xFFFFFF},
	};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		encoding_checking_proceed(sensor_type, &test_vector[i].represented,
					  &test_vector[i].raw, 3);
		decoding_checking_proceed(sensor_type, &test_vector[i].raw, 3,
					  &test_vector[i].represented);
	}

	/* Test invalid range on encoding. */
	struct sensor_value invalid_encoding_test_vector[] = {
		{-1, 0},
		{0x1000000, 0},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_encoding_test_vector); i++) {
		invalid_encoding_checking_proceed(sensor_type, &invalid_encoding_test_vector[i]);
	}
}

static void temp8_in_period_of_day_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value[sensor_type->channel_count];
	int8_t test_vector_temp[] = {-64, -32, 0, 32, 53, 0x7F};
	uint8_t test_vector_time[] = {0, 5, 10, 17, 24, 0xff};
	uint8_t expected[3];

	memset(in_value, 0, sizeof(in_value));

	for (int i = 0; i < ARRAY_SIZE(test_vector_temp); i++) {
		in_value[0].val1 = test_vector_temp[i];
		in_value[1].val1 = test_vector_time[i];
		in_value[2].val1 = test_vector_time[i];

		expected[0] = test_vector_temp[i] == 0x7F ?
			0x7F : raw_scalar_value_get(test_vector_temp[i], 1, 0, -1);
		expected[1] = test_vector_time[i] == 0xFF ?
			0xFF : raw_scalar_value_get(test_vector_time[i], 1, -1, 0);
		expected[2] = expected[1];

		encoding_checking_proceed(sensor_type, in_value, expected, 3 * sizeof(uint8_t));
		decoding_checking_proceed(sensor_type, expected, 3*sizeof(uint8_t), in_value);
	}
}

static void temp8_statistics_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value[sensor_type->channel_count];
	int8_t test_vector[] = {-64, -32, 0, 32, 63, 0x7F};

	/* Time exponential, represented = 1.1^(N-64), where N is raw value.
	 * Expected raw values precomputed. (0, 0xFE and 0xFF map
	 * to themselves as they represent 0 seconds, total device lifetime
	 * and unknown value, respectively) 66560640
	 */
	int32_t test_vector_time[] = {0, 1, 2999, 66560640, 0xFFFFFFFE,
				      0xFFFFFFFF};
	uint8_t expected_time[] = {0, 64, 148, 253, 0xFE, 0xFF};
	uint8_t expected[5];

	memset(in_value, 0, sizeof(in_value));

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		for (int j = 0; j < (sensor_type->channel_count - 1); j++) {
			in_value[j].val1 = test_vector[i];
		}
		in_value[sensor_type->channel_count - 1].val1 =
				test_vector_time[i];
		int8_t expected_temp = test_vector[i] == 0x7F ?
			0x7F : raw_scalar_value_get(test_vector[i], 1, 0, -1);
		expected[0] = expected_temp;
		expected[1] = expected_temp;
		expected[2] = expected_temp;
		expected[3] = expected_temp;
		expected[4] = expected_time[i];

		encoding_checking_proceed(sensor_type, in_value, expected, 5 * sizeof(uint8_t));
		decoding_checking_proceed_with_tolerance(sensor_type, expected,
							 5 * sizeof(uint8_t), in_value, 0.001);
	}
}

static void temp8_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	int8_t test_vector[] = {-64, -32, 0, 32, 63, 0x7F};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		in_value.val1 = test_vector[i];

		int8_t expected = test_vector[i] == 0x7F ?
			0x7F : raw_scalar_value_get(test_vector[i], 1, 0, -1);

		encoding_checking_proceed(sensor_type, &in_value, &expected, sizeof(uint8_t));
		decoding_checking_proceed(sensor_type, &expected, sizeof(uint8_t), &in_value);
	}
}

static void temp_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	int32_t test_vector[] = {0x8000, -273, 0, 100, 327};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		in_value.val1 = test_vector[i];

		int16_t expected = test_vector[i] == 0x8000 ?
			0x8000 : raw_scalar_value_get(test_vector[i], 1, -2, 0);

		encoding_checking_proceed(sensor_type, &in_value, &expected, sizeof(uint16_t));
		decoding_checking_proceed(sensor_type, &expected, sizeof(uint16_t), &in_value);
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

/* Photometry sensors */

static void test_cie_1931_chromaticity_coords(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	uint16_t sensor[] = {
		BT_MESH_PROP_ID_INITIAL_CIE_1931_CHROMATICITY_COORDS,
		BT_MESH_PROP_ID_PRESENT_CIE_1931_CHROMATICITY_COORDS,
	};

	for (int i = 0; i < ARRAY_SIZE(sensor); i++) {
		sensor_type = bt_mesh_sensor_type_get(sensor[i]);
		sensor_type_sanitize(sensor_type);
		chromaticity_coordinates_check(sensor_type);
	}
}

static void test_present_amb_light_level(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_AMB_LIGHT_LEVEL);
	sensor_type_sanitize(sensor_type);
	illuminance_check(sensor_type);
}

static void test_correlated_col_temp(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	uint16_t sensor[] = {
		BT_MESH_PROP_ID_INITIAL_CORRELATED_COL_TEMP,
		BT_MESH_PROP_ID_PRESENT_CORRELATED_COL_TEMP,
	};

	for (int i = 0; i < ARRAY_SIZE(sensor); i++) {
		sensor_type = bt_mesh_sensor_type_get(sensor[i]);
		sensor_type_sanitize(sensor_type);
		correlated_color_temp_check(sensor_type);
	}
}

static void test_present_illuminance(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_ILLUMINANCE);
	sensor_type_sanitize(sensor_type);
	illuminance_check(sensor_type);
}

static void test_luminous_flux(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	uint16_t sensor[] = {
		BT_MESH_PROP_ID_INITIAL_LUMINOUS_FLUX,
		BT_MESH_PROP_ID_PRESENT_LUMINOUS_FLUX,
	};

	for (int i = 0; i < ARRAY_SIZE(sensor); i++) {
		sensor_type = bt_mesh_sensor_type_get(sensor[i]);
		sensor_type_sanitize(sensor_type);
		luminous_flux_check(sensor_type);
	}
}

static void test_planckian_distance(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	uint16_t sensor[] = {
		BT_MESH_PROP_ID_INITIAL_PLANCKIAN_DISTANCE,
		BT_MESH_PROP_ID_PRESENT_PLANCKIAN_DISTANCE,
	};

	for (int i = 0; i < ARRAY_SIZE(sensor); i++) {
		sensor_type = bt_mesh_sensor_type_get(sensor[i]);
		sensor_type_sanitize(sensor_type);
		chromatic_distance_check(sensor_type);
	}
}

static void test_rel_exposure_time_in_an_illuminance_range(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(
					BT_MESH_PROP_ID_REL_EXPOSURE_TIME_IN_AN_ILLUMINANCE_RANGE);
	sensor_type_sanitize(sensor_type);
	percentage8_illuminance_check(sensor_type);
}

static void test_tot_light_exposure_time(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_TOT_LIGHT_EXPOSURE_TIME);
	sensor_type_sanitize(sensor_type);
	time_hour_24_check(sensor_type);
}

static void test_lumen_maintenance_factor(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_LUMEN_MAINTENANCE_FACTOR);
	sensor_type_sanitize(sensor_type);
	percentage8_check(sensor_type);
}

static void test_luminous_efficacy(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_LUMINOUS_EFFICACY);
	sensor_type_sanitize(sensor_type);
	luminous_efficacy_check(sensor_type);
}

static void test_luminous_energy_since_turn_on(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_LUMINOUS_ENERGY_SINCE_TURN_ON);
	sensor_type_sanitize(sensor_type);
	luminous_energy_check(sensor_type);
}

static void test_luminous_exposure(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_LUMINOUS_EXPOSURE);
	sensor_type_sanitize(sensor_type);
	luminous_exposure_check(sensor_type);
}

static void test_luminous_flux_range(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_LUMINOUS_FLUX_RANGE);
	sensor_type_sanitize(sensor_type);
	luminous_flux_range_check(sensor_type);
}

static void test_avg_amb_temp_in_period_of_day(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_AVG_AMB_TEMP_IN_A_PERIOD_OF_DAY);
	sensor_type_sanitize(sensor_type);
	temp8_in_period_of_day_check(sensor_type);
}

static void test_indoor_amb_temp_stat_values(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_INDOOR_AMB_TEMP_STAT_VALUES);
	sensor_type_sanitize(sensor_type);
	temp8_statistics_check(sensor_type);
}

static void test_outdoor_stat_values(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_OUTDOOR_STAT_VALUES);
	sensor_type_sanitize(sensor_type);
	temp8_statistics_check(sensor_type);
}

static void test_present_amb_temp(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_AMB_TEMP);
	sensor_type_sanitize(sensor_type);
	temp8_check(sensor_type);
}

static void test_present_indoor_amb_temp(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_INDOOR_AMB_TEMP);
	sensor_type_sanitize(sensor_type);
	temp8_check(sensor_type);
}

static void test_present_outdoor_amb_temp(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_OUTDOOR_AMB_TEMP);
	sensor_type_sanitize(sensor_type);
	temp8_check(sensor_type);
}

static void test_desired_amb_temp(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_DESIRED_AMB_TEMP);
	sensor_type_sanitize(sensor_type);
	temp8_check(sensor_type);
}

static void test_precise_present_amb_temp(void)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRECISE_PRESENT_AMB_TEMP);
	sensor_type_sanitize(sensor_type);
	temp_check(sensor_type);
}

void test_main(void)
{
	ztest_test_suite(sensor_types_test,
			ztest_unit_test(test_motion_sensor),
			ztest_unit_test(test_motion_threshold),
			ztest_unit_test(test_people_count),
			ztest_unit_test(test_presence_detected),
			ztest_unit_test(test_time_since_motion_sensed),
			ztest_unit_test(test_time_since_presence_detected),

			/* Photometry sensors */
			ztest_unit_test(test_cie_1931_chromaticity_coords),
			ztest_unit_test(test_present_amb_light_level),
			ztest_unit_test(test_correlated_col_temp),
			ztest_unit_test(test_present_illuminance),
			ztest_unit_test(test_luminous_flux),
			ztest_unit_test(test_planckian_distance),
			ztest_unit_test(test_rel_exposure_time_in_an_illuminance_range),
			ztest_unit_test(test_tot_light_exposure_time),
			ztest_unit_test(test_lumen_maintenance_factor),
			ztest_unit_test(test_luminous_efficacy),
			ztest_unit_test(test_luminous_energy_since_turn_on),
			ztest_unit_test(test_luminous_exposure),
			ztest_unit_test(test_luminous_flux_range),
			ztest_unit_test(test_avg_amb_temp_in_period_of_day),
			ztest_unit_test(test_indoor_amb_temp_stat_values),
			ztest_unit_test(test_outdoor_stat_values),
			ztest_unit_test(test_present_amb_temp),
			ztest_unit_test(test_present_indoor_amb_temp),
			ztest_unit_test(test_present_outdoor_amb_temp),
			ztest_unit_test(test_desired_amb_temp),
			ztest_unit_test(test_precise_present_amb_temp)
			 );

	ztest_run_test_suite(sensor_types_test);
}
