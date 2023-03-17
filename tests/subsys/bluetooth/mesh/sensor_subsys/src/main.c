/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <math.h>
#include <stdint.h>
#include <float.h>

#include <zephyr/ztest.h>
#include <bluetooth/mesh/properties.h>
#include <bluetooth/mesh/sensor_types.h>
#include <model_utils.h>
#include <sensor.h> // private header from the source folder

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

static void voltage_helper(uint16_t test_value, struct sensor_value *in_value, uint16_t *expected)
{
	in_value->val1 = test_value;
	in_value->val2 = 0;
	*expected = test_value > 1022 ?
			test_value == UINT16_MAX ? UINT16_MAX
			: raw_scalar_value_get(1022, 1, 0, -6)
			: raw_scalar_value_get(test_value, 1, 0, -6);
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

static void humidity_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	uint16_t test_vector[] = {0, 1000, 5000, 10000, UINT16_MAX};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		if (test_vector[i] == UINT16_MAX) {
			in_value.val1 = UINT16_MAX;
			in_value.val2 = 0;
		} else {
			in_value.val1 = test_vector[i] / 100;
			in_value.val2 = test_vector[i] % 100 * 10000;
		}

		encoding_checking_proceed(sensor_type, &in_value, &test_vector[i], 2);
		decoding_checking_proceed(sensor_type, &test_vector[i], 2, &in_value);
	}

	/* Test invalid range. */
	uint16_t invalid_test_vector = 10001;

	in_value.val1 = invalid_test_vector / 100;
	in_value.val2 = invalid_test_vector % 100 * 10000;
	invalid_encoding_checking_proceed(sensor_type, &in_value);
}

static void temp8_signed_celsius_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	int8_t test_vector[] = {-128, -50, 0, 50, 127};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		in_value.val1 = test_vector[i];

		encoding_checking_proceed(sensor_type, &in_value, &test_vector[i], 1);
		decoding_checking_proceed(sensor_type, &test_vector[i], 1, &in_value);
	}
}

static void plane_angle_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	uint16_t test_vector[] = {0, 1000, 5000, 10000, 35999};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		in_value.val1 = test_vector[i] / 100;
		in_value.val2 = test_vector[i] % 100 * 10000;

		encoding_checking_proceed(sensor_type, &in_value, &test_vector[i], 2);
		decoding_checking_proceed(sensor_type, &test_vector[i], 2, &in_value);
	}

	/* Test invalid range. */
	uint16_t invalid_test_vector = 36000;

	in_value.val1 = invalid_test_vector / 100;
	in_value.val2 = invalid_test_vector % 100 * 10000;
	invalid_encoding_checking_proceed(sensor_type, &in_value);
}

static void wind_speed_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	uint16_t test_vector[] = {0, 1000, 5432, 12345, UINT16_MAX};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		in_value.val1 = test_vector[i] / 100;
		in_value.val2 = test_vector[i] % 100 * 10000;

		encoding_checking_proceed(sensor_type, &in_value, &test_vector[i], 2);
		decoding_checking_proceed(sensor_type, &test_vector[i], 2, &in_value);
	}
}

static void pressure_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	uint32_t test_vector[] = {0, UINT32_MAX / 2, UINT32_MAX};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		in_value.val1 = test_vector[i] / 10;
		in_value.val2 = test_vector[i] % 10 * 100000;

		encoding_checking_proceed(sensor_type, &in_value, &test_vector[i], 4);
		decoding_checking_proceed(sensor_type, &test_vector[i], 4, &in_value);
	}
}

static void gust_factor_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	uint8_t test_vector[] = {0, 25, 50, 75, 100, UINT8_MAX};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		in_value.val1 = test_vector[i] / 10;
		in_value.val2 = test_vector[i] % 10 * 100000;

		encoding_checking_proceed(sensor_type, &in_value, &test_vector[i], 1);
		decoding_checking_proceed(sensor_type, &test_vector[i], 1, &in_value);
	}
}

static void concentration_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	uint32_t test_vector[] = {0, 1000,
		UINT16_MAX - 2, UINT16_MAX - 1, UINT16_MAX, UINT16_MAX + 1};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		in_value.val1 = test_vector[i];

		uint16_t expected = test_vector[i] > 65533 ?
			test_vector[i] == UINT16_MAX ?
				UINT16_MAX : UINT16_MAX - 1 : test_vector[i];

		encoding_checking_proceed(sensor_type, &in_value, &expected, 2);
		in_value.val1 = expected;
		decoding_checking_proceed(sensor_type, &expected, 2, &in_value);
	}
}

static void noise_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	uint16_t test_vector[] = {0, 50,
		UINT8_MAX - 2, UINT8_MAX - 1, UINT8_MAX, UINT8_MAX + 1};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		in_value.val1 = test_vector[i];

		uint8_t expected = test_vector[i] > 253 ?
			test_vector[i] == UINT8_MAX ?
				UINT8_MAX : UINT8_MAX - 1 : test_vector[i];

		encoding_checking_proceed(sensor_type, &in_value, &expected, 1);
		in_value.val1 = expected;
		decoding_checking_proceed(sensor_type, &expected, 1, &in_value);
	}
}

static void pollen_concentration_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	uint32_t test_vector[] = {0, 0xFF, 0xFFFF, 0xFFFFFF};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		in_value.val1 = test_vector[i];

		encoding_checking_proceed(sensor_type, &in_value, &test_vector[i], 3);
		decoding_checking_proceed(sensor_type, &test_vector[i], 3, &in_value);
	}
}

static void rainfall_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	uint16_t test_vector[] = {0, 1000, 15000, 30000, UINT16_MAX};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		in_value.val1 = test_vector[i] / 1000;
		in_value.val2 = test_vector[i] % 1000 * 1000;

		encoding_checking_proceed(sensor_type, &in_value, &test_vector[i], 2);
		decoding_checking_proceed(sensor_type, &test_vector[i], 2, &in_value);
	}
}

static void uv_index_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value = {0};
	uint8_t test_vector[] = {0, 25, 50, 200, UINT8_MAX};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		in_value.val1 = test_vector[i];

		encoding_checking_proceed(sensor_type, &in_value, &test_vector[i], 1);
		decoding_checking_proceed(sensor_type, &test_vector[i], 1, &in_value);
	}
}

static void flux_density_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value[sensor_type->channel_count];
	int16_t test_vector[] = {INT16_MIN, -30000, -15000, 0, 15000, 30000, INT16_MAX};

	memset(in_value, 0, sizeof(in_value));

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		int16_t expected[sensor_type->channel_count];

		for (int j = 0; j < sensor_type->channel_count; j++) {
			in_value[j].val1 = test_vector[i] / 10;
			in_value[j].val2 = test_vector[i] % 10 * 100000;
			expected[j] = test_vector[i];
		}

		encoding_checking_proceed(sensor_type, in_value, expected, sizeof(expected));
		decoding_checking_proceed(sensor_type, expected, sizeof(expected), in_value);
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
		{{0x7FFF, 0}, 0x7FFF},
		{{0x7FFE, 0}, 0x7FFE},
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
	 * and unknown value, respectively)
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
	struct sensor_value in_value[sensor_type->channel_count];
	int32_t test_vector[] = {0x8000, -273, 0, 100, 327};

	memset(in_value, 0, sizeof(in_value));

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		int16_t expected[sensor_type->channel_count];

		for (int j = 0; j < sensor_type->channel_count; j++) {
			in_value[j].val1 = test_vector[i];
			in_value[j].val2 = 0;
			expected[j] = test_vector[i] == 0x8000 ?
				0x8000 : raw_scalar_value_get(test_vector[i], 1, -2, 0);
		}

		encoding_checking_proceed(sensor_type, in_value, expected, sizeof(expected));
		decoding_checking_proceed(sensor_type, expected, sizeof(expected), in_value);
	}
}

static void power_specification_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented[3];
		struct uint24_t raw[3];
	} test_vector[] = {
		{
			{{0, 0}, {1, 0}, {65536, 0}},
			{{0}, {10}, {655360}}
		},
		{
			{{1234, 0}, {838860, 500000}, {1677721, 400000}},
			{{12340}, {8388605}, {16777214}}
		},
	};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		encoding_checking_proceed(sensor_type, &test_vector[i].represented[0],
					  &test_vector[i].raw, 9);
		decoding_checking_proceed(sensor_type, &test_vector[i].raw, 9,
					  &test_vector[i].represented[0]);
	}

	/* Test invalid range on encoding. */
	struct sensor_value invalid_encoding_test_vector[][3] = {
		{{1, 0}, {-1, 0}, {0, 0}},
		{{1234, 0}, {4567, 0}, {0x1000000, 0}},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_encoding_test_vector); i++) {
		invalid_encoding_checking_proceed(sensor_type, &invalid_encoding_test_vector[i][0]);
	}
}

static void power_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented;
		uint32_t raw;
	} test_vector[] = {
		{{0, 0}, 0},
		{{1234, 0}, 12340},
		{{87654, 300000}, 876543},
		{{1677721, 400000}, 0xFFFFFE},
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

static void energy_check(const struct bt_mesh_sensor_type *sensor_type)
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

static void energy32_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented;
		uint32_t raw;
	} test_vector[] = {
		{{0, 0}, 0},
		{{1000, 0}, 1000000},
		{{1234567, 890000}, 1234567890},
		{{4294967, 293000}, 0xFFFFFFFD},
		{{0xFFFFFFFE, 0}, 0xFFFFFFFE},
		{{0xFFFFFFFF, 0}, 0xFFFFFFFF},
	};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		encoding_checking_proceed(sensor_type, &test_vector[i].represented,
					  &test_vector[i].raw, 4);
		decoding_checking_proceed(sensor_type, &test_vector[i].raw, 4,
					  &test_vector[i].represented);
	}

	/* Test invalid range on encoding. */
	struct sensor_value invalid_encoding_test_vector[] = {
		{-3, 0},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_encoding_test_vector); i++) {
		invalid_encoding_checking_proceed(sensor_type, &invalid_encoding_test_vector[i]);
	}
}

static void cos_of_the_angle_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented;
		uint8_t raw;
	} test_vector[] = {
		{{-100, 0}, 0x9C},
		{{-50, 0}, 0xCE},
		{{0, 0}, 0},
		{{50, 0}, 0x32},
		{{100, 0}, 0x64},
		{{0x7F, 0}, 0x7F},
	};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		encoding_checking_proceed(sensor_type, &test_vector[i].represented,
					  &test_vector[i].raw, 1);
		decoding_checking_proceed(sensor_type, &test_vector[i].raw, 1,
					  &test_vector[i].represented);
	}

	/* Test invalid range on encoding. */
	struct sensor_value invalid_encoding_test_vector[] = {
		{-101, 0},
		{101, 0},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_encoding_test_vector); i++) {
		invalid_encoding_checking_proceed(sensor_type, &invalid_encoding_test_vector[i]);
	}

	/* Test invalid range on decoding. */
	uint32_t invalid_decoding_test_vector[] = {
		0x9B,
		0x7E,
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_decoding_test_vector); i++) {
		invalid_decoding_checking_proceed(sensor_type, &invalid_decoding_test_vector[i], 1);
	}
}

static void energy_in_a_period_of_day_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented[3];
		struct __packed {
			struct uint24_t energy;
			uint8_t start_time;
			uint8_t end_time;
		} raw;
	} test_vector[] = {
		{{{0, 0}, {0, 0}, {1, 0}}, {{0}, 0, 10}},
		{{{1000, 0}, {10, 100000}, {12, 200000}}, {{1000}, 101, 122}},
		{{{16777214, 0}, {15, 500000}, {24, 0}}, {{16777214}, 155, 240}},
		{{{1, 0}, {2, 0}, {0xFF, 0}}, {{1}, 20, 0xFF}},
	};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		encoding_checking_proceed(sensor_type, &test_vector[i].represented[0],
					  &test_vector[i].raw, 5);
		decoding_checking_proceed(sensor_type, &test_vector[i].raw, 5,
					  &test_vector[i].represented[0]);
	}

	/* Test invalid range on encoding. */
	struct sensor_value invalid_encoding_test_vector[][3] = {
		{{-1, 0}, {0, 0}, {0, 0}},
		{{0, 0}, {-1, 0}, {0, 0}},
		{{0, 0}, {0, 0}, {-1, 0}},
		{{1, 0}, {25, 0}, {0, 0}},
		{{1, 0}, {0, 0}, {25, 0}},
		{{0x1000000, 0}, {0, 0}, {0, 0}},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_encoding_test_vector); i++) {
		invalid_encoding_checking_proceed(sensor_type, &invalid_encoding_test_vector[i][0]);
	}

	/* Test invalid range on decoding. */
	struct __packed {
		struct uint24_t energy;
		uint8_t start_time;
		uint8_t end_time;
	} invalid_decoding_test_vector[] = {
		{{0}, 241, 0},
		{{0}, 0, 254},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_decoding_test_vector); i++) {
		invalid_decoding_checking_proceed(sensor_type, &invalid_decoding_test_vector[i], 5);
	}
}

static void relative_runtime_in_a_generic_level_range_check(
						const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented[3];
		struct __packed {
			uint8_t relative;
			uint16_t min;
			uint16_t max;
		} raw;
	} test_vector[] = {
		{{{0, 0}, {0, 0}, {0, 0}}, {0, 0, 0}},
		{{{50, 0}, {32767, 0}, {16384, 0}}, {100, 32767, 16384}},
		{{{100, 0}, {65535, 0}, {8192, 0}}, {200, 65535, 8192}},
	};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		encoding_checking_proceed(sensor_type, &test_vector[i].represented[0],
					  &test_vector[i].raw, 5);
		decoding_checking_proceed(sensor_type, &test_vector[i].raw, 5,
					  &test_vector[i].represented[0]);
	}

	/* Test invalid range on encoding. */
	struct sensor_value invalid_encoding_test_vector[][3] = {
		{{0, 0}, {-1, 0}, {0, 0}},
		{{0, 0}, {0, 0}, {-1, 0}},
		{{100, 500000}, {0, 0}, {0, 0}},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_encoding_test_vector); i++) {
		invalid_encoding_checking_proceed(sensor_type, &invalid_encoding_test_vector[i][0]);
	}

	/* Test invalid range on decoding. */
	struct __packed {
		uint8_t relative;
		uint16_t min;
		uint16_t max;
	} invalid_decoding_test_vector[] = {
		{251, 0, 0},
		{254, 0, 0},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_decoding_test_vector); i++) {
		invalid_decoding_checking_proceed(sensor_type, &invalid_decoding_test_vector[i], 5);
	}
}

static void apparent_energy32_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented;
		uint32_t raw;
	} test_vector[] = {
		{{0, 0}, 0},
		{{1000, 0}, 1000000},
		{{1234567, 890000}, 1234567890},
		{{4294967, 293000}, 0xFFFFFFFD},
		{{0xFFFFFFFE, 0}, 0xFFFFFFFE},
		{{0xFFFFFFFF, 0}, 0xFFFFFFFF},
	};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		encoding_checking_proceed(sensor_type, &test_vector[i].represented,
					  &test_vector[i].raw, 4);
		decoding_checking_proceed(sensor_type, &test_vector[i].raw, 4,
					  &test_vector[i].represented);
	}

	/* Test invalid range on encoding. */
	struct sensor_value invalid_encoding_test_vector[] = {
		{-3, 0},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_encoding_test_vector); i++) {
		invalid_encoding_checking_proceed(sensor_type, &invalid_encoding_test_vector[i]);
	}
}

static void apparent_power_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct {
		struct sensor_value represented;
		uint32_t raw;
	} test_vector[] = {
		{{0, 0}, 0},
		{{1234, 0}, 12340},
		{{87654, 300000}, 876543},
		{{1677721, 300000}, 0xFFFFFD},
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
		{1677721, 400000},
		{0x1000000, 0},
	};

	for (int i = 0; i < ARRAY_SIZE(invalid_encoding_test_vector); i++) {
		invalid_encoding_checking_proceed(sensor_type, &invalid_encoding_test_vector[i]);
	}
}

static void electric_current_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value[sensor_type->channel_count];
	uint16_t test_vector[] = {0, 1234, 5555, 11111, 65534, UINT16_MAX};

	memset(in_value, 0, sizeof(in_value));

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		uint16_t expected[sensor_type->channel_count];

		for (int j = 0; j < sensor_type->channel_count; j++) {
			expected[j] = test_vector[i];
			in_value[j].val1 = test_vector[i] == UINT16_MAX ?
					UINT16_MAX : test_vector[i] / 100;
			in_value[j].val2 = test_vector[i] == UINT16_MAX ?
					0 : test_vector[i] % 100 * 10000;
		}

		encoding_checking_proceed(sensor_type, in_value, expected, sizeof(expected));
		decoding_checking_proceed(sensor_type, expected, sizeof(expected), in_value);
	}
}

static void voltage_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value[sensor_type->channel_count];
	uint16_t test_vector[] = {0, 1, 159, 1000, 1022, 1023, UINT16_MAX};

	memset(in_value, 0, sizeof(in_value));

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		uint16_t expected[sensor_type->channel_count];

		for (int j = 0; j < sensor_type->channel_count; j++) {
			voltage_helper(test_vector[i], &in_value[j], &expected[j]);
		}

		encoding_checking_proceed(sensor_type, in_value, expected, sizeof(expected));
		for (int j = 0; j < sensor_type->channel_count; j++) {
			in_value[j].val1 = test_vector[i] > 1022 ?
				test_vector[i] == UINT16_MAX ? UINT16_MAX
				: 1022 : test_vector[i];
		}
		decoding_checking_proceed(sensor_type, expected, sizeof(expected), in_value);
	}
}

static void average_current_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value[sensor_type->channel_count];
	uint16_t test_vector_current[] = {0, 1234, 5555, 11111, 65534, UINT16_MAX};

	memset(in_value, 0, sizeof(in_value));

	/* Time exponential, represented = 1.1^(N-64), where N is raw value.
	 * Expected raw values precomputed. (0, 0xFE and 0xFF map
	 * to themselves as they represent 0 seconds, total device lifetime
	 * and unknown value, respectively)
	 */
	int32_t test_vector_time[] = {0, 1, 2999, 66560640, 0xFFFFFFFE,
				      0xFFFFFFFF};
	uint8_t expected_time[] = {0, 64, 148, 253, 0xFE, 0xFF};
	struct __packed {
		uint16_t expected_current;
		uint8_t expected_time;
	} expected;

	for (int i = 0; i < ARRAY_SIZE(test_vector_current); i++) {
		in_value[0].val1 = test_vector_current[i] == UINT16_MAX ?
				UINT16_MAX : test_vector_current[i] / 100;
		in_value[0].val2 = test_vector_current[i] == UINT16_MAX ?
				0 : test_vector_current[i] % 100 * 10000;

		in_value[1].val1 = test_vector_time[i];
		in_value[1].val2 = 0;

		expected.expected_current = test_vector_current[i];
		expected.expected_time = expected_time[i];

		encoding_checking_proceed(sensor_type, in_value, &expected, sizeof(expected));
		decoding_checking_proceed_with_tolerance(sensor_type, &expected,
				sizeof(expected), in_value, 0.001);
	}
}

static void average_voltage_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value[sensor_type->channel_count];
	uint16_t test_vector_voltage[] = {0, 1, 159, 1022, 1023, UINT16_MAX};

	memset(in_value, 0, sizeof(in_value));

	/* Time exponential, represented = 1.1^(N-64), where N is raw value.
	 * Expected raw values precomputed. (0, 0xFE and 0xFF map
	 * to themselves as they represent 0 seconds, total device lifetime
	 * and unknown value, respectively)
	 */
	int32_t test_vector_time[] = {0, 1, 2999, 66560640, 0xFFFFFFFE,
				      0xFFFFFFFF};
	uint8_t expected_time[] = {0, 64, 148, 253, 0xFE, 0xFF};
	struct __packed {
		uint16_t expected_voltage;
		uint8_t expected_time;
	} expected;

	for (int i = 0; i < ARRAY_SIZE(test_vector_voltage); i++) {
		uint16_t tmp;

		voltage_helper(test_vector_voltage[i], &in_value[0], &tmp);
		expected.expected_voltage = tmp;

		in_value[1].val1 = test_vector_time[i];
		in_value[1].val2 = 0;

		expected.expected_time = expected_time[i];

		encoding_checking_proceed(sensor_type, in_value, &expected, sizeof(expected));
		decoding_checking_proceed_with_tolerance(sensor_type, &expected,
				sizeof(expected), in_value, 0.001);
	}
}

static void current_stat_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value[sensor_type->channel_count];
	uint16_t test_vector_current[] = {0, 1234, 5555, 11111, 65534, UINT16_MAX};

	memset(in_value, 0, sizeof(in_value));

	/* Time exponential, represented = 1.1^(N-64), where N is raw value.
	 * Expected raw values precomputed. (0, 0xFE and 0xFF map
	 * to themselves as they represent 0 seconds, total device lifetime
	 * and unknown value, respectively)
	 */
	int32_t test_vector_time[] = {0, 1, 2999, 66560640, 0xFFFFFFFE,
				      0xFFFFFFFF};
	uint8_t expected_time[] = {0, 64, 148, 253, 0xFE, 0xFF};
	struct __packed {
		uint16_t expected_current[4];
		uint8_t expected_time;
	} expected;

	for (int i = 0; i < ARRAY_SIZE(test_vector_current); i++) {
		for (int j = 0; j < sensor_type->channel_count - 1; j++) {
			in_value[j].val1 = test_vector_current[i] == UINT16_MAX ?
					UINT16_MAX : test_vector_current[i] / 100;
			in_value[j].val2 = test_vector_current[i] == UINT16_MAX ?
					0 : test_vector_current[i] % 100 * 10000;
			expected.expected_current[j] = test_vector_current[i];
		}

		in_value[sensor_type->channel_count - 1].val1 = test_vector_time[i];
		in_value[sensor_type->channel_count - 1].val2 = 0;
		expected.expected_time = expected_time[i];

		encoding_checking_proceed(sensor_type, in_value, &expected, sizeof(expected));
		decoding_checking_proceed_with_tolerance(sensor_type, &expected,
				sizeof(expected), in_value, 0.001);
	}
}

static void voltage_stat_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value[sensor_type->channel_count];
	uint16_t test_vector_voltage[] = {0, 1, 159, 1022, 1023, UINT16_MAX};

	memset(in_value, 0, sizeof(in_value));

	/* Time exponential, represented = 1.1^(N-64), where N is raw value.
	 * Expected raw values precomputed. (0, 0xFE and 0xFF map
	 * to themselves as they represent 0 seconds, total device lifetime
	 * and unknown value, respectively)
	 */
	int32_t test_vector_time[] = {0, 1, 2999, 66560640, 0xFFFFFFFE,
				      0xFFFFFFFF};
	uint8_t expected_time[] = {0, 64, 148, 253, 0xFE, 0xFF};
	struct __packed {
		uint16_t expected_voltage[4];
		uint8_t expected_time;
	} expected;

	for (int i = 0; i < ARRAY_SIZE(test_vector_voltage); i++) {
		for (int j = 0; j < sensor_type->channel_count - 1; j++) {
			uint16_t tmp;

			voltage_helper(test_vector_voltage[i], &in_value[j], &tmp);
			expected.expected_voltage[j] = tmp;
		}

		in_value[sensor_type->channel_count - 1].val1 = test_vector_time[i];
		in_value[sensor_type->channel_count - 1].val2 = 0;
		expected.expected_time = expected_time[i];

		encoding_checking_proceed(sensor_type, in_value, &expected, sizeof(expected));
		decoding_checking_proceed_with_tolerance(sensor_type, &expected,
				sizeof(expected), in_value, 0.001);
	}
}

static void rel_runtime_in_current_range_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value[sensor_type->channel_count];
	uint16_t test_vector_current[] = {0, 1234, 5555, 11111, 65534, UINT16_MAX};
	uint8_t test_vector_percentage8[] = {0, 25, 50, 75, 100, 0xFF};
	struct __packed {
		uint8_t expected_percentage8;
		uint16_t expected_current[2];
	} expected;

	memset(in_value, 0, sizeof(in_value));

	for (int i = 0; i < ARRAY_SIZE(test_vector_current); i++) {
		for (int j = 1; j < sensor_type->channel_count; j++) {
			expected.expected_current[j - 1] = test_vector_current[i];
			in_value[j].val1 = test_vector_current[i] == UINT16_MAX ?
					UINT16_MAX : test_vector_current[i] / 100;
			in_value[j].val2 = test_vector_current[i] == UINT16_MAX ?
					0 : test_vector_current[i] % 100 * 10000;
		}

		in_value[0].val1 = test_vector_percentage8[i];
		in_value[0].val2 = 0;
		expected.expected_percentage8 = test_vector_percentage8[i] == 0xFF ?
			0xFF : raw_scalar_value_get(test_vector_percentage8[i], 1, 0, -1);

		encoding_checking_proceed(sensor_type, in_value, &expected, sizeof(expected));
		decoding_checking_proceed(sensor_type, &expected, sizeof(expected), in_value);
	}
}

static void rel_runtime_in_voltage_range_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value[sensor_type->channel_count];
	uint16_t test_vector_voltage[] = {0, 1, 159, 1022, 1023, UINT16_MAX};
	uint8_t test_vector_percentage8[] = {0, 25, 50, 75, 100, 0xFF};
	struct __packed {
		uint8_t expected_percentage8;
		uint16_t expected_voltage[2];
	} expected;

	memset(in_value, 0, sizeof(in_value));

	for (int i = 0; i < ARRAY_SIZE(test_vector_voltage); i++) {
		for (int j = 1; j < sensor_type->channel_count; j++) {
			uint16_t tmp;

			voltage_helper(test_vector_voltage[i], &in_value[j], &tmp);
			expected.expected_voltage[j - 1] = tmp;
		}

		in_value[0].val1 = test_vector_percentage8[i];
		in_value[0].val2 = 0;
		expected.expected_percentage8 = test_vector_percentage8[i] == 0xFF ?
			0xFF : raw_scalar_value_get(test_vector_percentage8[i], 1, 0, -1);

		for (int j = 1; j < sensor_type->channel_count; j++) {
			in_value[j].val1 = test_vector_voltage[i] > 1022 ?
				test_vector_voltage[i] == UINT16_MAX ? UINT16_MAX
				: 1022 : test_vector_voltage[i];
		}

		encoding_checking_proceed(sensor_type, in_value, &expected, sizeof(expected));
		decoding_checking_proceed(sensor_type, &expected, sizeof(expected), in_value);
	}
}

static void temp_stat_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value[sensor_type->channel_count];
	int32_t test_vector_temp[] = {0x8000, -273, 0, 100, 327};

	/* Time exponential, represented = 1.1^(N-64), where N is raw value.
	 * Expected raw values precomputed. (0, 0xFE and 0xFF map
	 * to themselves as they represent 0 seconds, total device lifetime
	 * and unknown value, respectively)
	 */
	int32_t test_vector_time[] = {0, 1, 66560640, 0xFFFFFFFE,
				      0xFFFFFFFF};
	uint8_t expected_time[] = {0, 64, 253, 0xFE, 0xFF};
	struct __packed {
		uint16_t expected_temp[4];
		uint8_t expected_time;
	} expected;

	memset(in_value, 0, sizeof(in_value));

	for (int i = 0; i < ARRAY_SIZE(test_vector_temp); i++) {
		for (int j = 0; j < 4; j++) {
			in_value[j].val1 = test_vector_temp[i];
			expected.expected_temp[j] = test_vector_temp[i] == 0x8000 ?
				0x8000 : raw_scalar_value_get(test_vector_temp[i], 1, -2, 0);
		}
		in_value[4].val1 = test_vector_time[i];
		expected.expected_time = expected_time[i];

		encoding_checking_proceed(sensor_type, in_value, &expected, sizeof(expected));
		decoding_checking_proceed_with_tolerance(sensor_type, &expected, sizeof(expected),
							 in_value, 0.001);
	}
}

static void rel_val_in_a_temp_range_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value[sensor_type->channel_count];
	int32_t test_vector_temp[] = {0x8000, -273, 0, 100, 327};
	int32_t test_vector_percentage8[] = {0, 25, 50, 100, 0xFF};

	struct __packed {
		uint8_t expected_percentage8;
		uint16_t expected_temp[2];
	} expected;

	memset(in_value, 0, sizeof(in_value));

	for (int i = 0; i < ARRAY_SIZE(test_vector_temp); i++) {
		in_value[0].val1 = test_vector_percentage8[i];
		in_value[1].val1 = test_vector_temp[i];
		in_value[2].val1 = test_vector_temp[i];

		uint16_t expected_temp = test_vector_temp[i] == 0x8000 ?
			0x8000 : raw_scalar_value_get(test_vector_temp[i], 1, -2, 0);

		expected.expected_percentage8 = test_vector_percentage8[i] == 0xFF ?
			0xFF : raw_scalar_value_get(test_vector_percentage8[i], 1, 0, -1);
		expected.expected_temp[0] = expected_temp;
		expected.expected_temp[1] = expected_temp;

		encoding_checking_proceed(sensor_type, in_value, &expected, sizeof(expected));
		decoding_checking_proceed(sensor_type, &expected, sizeof(expected), in_value);
	}
}

static void gain_check(const struct bt_mesh_sensor_type *sensor_type)
{
	struct sensor_value in_value[] = {{-10, -999999}, {0, -99999}, {0, 99999},
			{10, 1111}, {9, 999999}};

	for (int i = 0; i < ARRAY_SIZE(in_value); i++) {
		float expected = (float)in_value[i].val1 + in_value[i].val2 / 1000000.0f;

		encoding_checking_proceed(sensor_type, &in_value[i], &expected, sizeof(float));
		decoding_checking_proceed(sensor_type, &expected, sizeof(float), &in_value[i]);
	}
}

/* Occupancy sensors */

ZTEST(sensor_types_test, test_motion_sensor)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_MOTION_SENSED);
	sensor_type_sanitize(sensor_type);
	percentage8_check(sensor_type);
}

ZTEST(sensor_types_test, test_motion_threshold)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_MOTION_THRESHOLD);
	sensor_type_sanitize(sensor_type);
	percentage8_check(sensor_type);
}

ZTEST(sensor_types_test, test_people_count)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PEOPLE_COUNT);
	sensor_type_sanitize(sensor_type);
	count16_check(sensor_type);
}

ZTEST(sensor_types_test, test_presence_detected)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENCE_DETECTED);
	sensor_type_sanitize(sensor_type);
	boolean_check(sensor_type);
}

ZTEST(sensor_types_test, test_time_since_motion_sensed)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_TIME_SINCE_MOTION_SENSED);
	sensor_type_sanitize(sensor_type);
	time_second16_check(sensor_type);
}

ZTEST(sensor_types_test, test_time_since_presence_detected)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_TIME_SINCE_PRESENCE_DETECTED);
	sensor_type_sanitize(sensor_type);
	time_second16_check(sensor_type);
}

/* Environmental sensors */

ZTEST(sensor_types_test, test_apparent_wind_direction)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_APPARENT_WIND_DIRECTION);
	sensor_type_sanitize(sensor_type);
	plane_angle_check(sensor_type);
}

ZTEST(sensor_types_test, test_apparent_wind_speed)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_APPARENT_WIND_SPEED);
	sensor_type_sanitize(sensor_type);
	wind_speed_check(sensor_type);
}

ZTEST(sensor_types_test, test_dew_point)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_DEW_POINT);
	sensor_type_sanitize(sensor_type);
	temp8_signed_celsius_check(sensor_type);
}

ZTEST(sensor_types_test, test_gust_factor)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_GUST_FACTOR);
	sensor_type_sanitize(sensor_type);
	gust_factor_check(sensor_type);
}

ZTEST(sensor_types_test, test_heat_index)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_HEAT_INDEX);
	sensor_type_sanitize(sensor_type);
	temp8_signed_celsius_check(sensor_type);
}

ZTEST(sensor_types_test, test_present_amb_rel_humidity)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_AMB_REL_HUMIDITY);
	sensor_type_sanitize(sensor_type);
	humidity_check(sensor_type);
}

ZTEST(sensor_types_test, test_present_amb_co2_concentration)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_AMB_CO2_CONCENTRATION);
	sensor_type_sanitize(sensor_type);
	concentration_check(sensor_type);
}

ZTEST(sensor_types_test, test_present_amb_voc_concentration)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_AMB_VOC_CONCENTRATION);
	sensor_type_sanitize(sensor_type);
	concentration_check(sensor_type);
}

ZTEST(sensor_types_test, test_present_amb_noise)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_AMB_NOISE);
	sensor_type_sanitize(sensor_type);
	noise_check(sensor_type);
}

ZTEST(sensor_types_test, test_present_indoor_relative_humidity)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_INDOOR_RELATIVE_HUMIDITY);
	sensor_type_sanitize(sensor_type);
	humidity_check(sensor_type);
}

ZTEST(sensor_types_test, test_present_outdoor_relative_humidity)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_OUTDOOR_RELATIVE_HUMIDITY);
	sensor_type_sanitize(sensor_type);
	humidity_check(sensor_type);
}

ZTEST(sensor_types_test, test_magnetic_declination)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_MAGNETIC_DECLINATION);
	sensor_type_sanitize(sensor_type);
	plane_angle_check(sensor_type);
}

ZTEST(sensor_types_test, test_magnetic_flux_density_2d)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_MAGNETIC_FLUX_DENSITY_2D);
	sensor_type_sanitize(sensor_type);
	flux_density_check(sensor_type);
}

ZTEST(sensor_types_test, test_magnetic_flux_density_3d)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_MAGNETIC_FLUX_DENSITY_3D);
	sensor_type_sanitize(sensor_type);
	flux_density_check(sensor_type);
}

ZTEST(sensor_types_test, test_pollen_concentration)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_POLLEN_CONCENTRATION);
	sensor_type_sanitize(sensor_type);
	pollen_concentration_check(sensor_type);
}

ZTEST(sensor_types_test, test_air_pressure)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_AIR_PRESSURE);
	sensor_type_sanitize(sensor_type);
	pressure_check(sensor_type);
}

ZTEST(sensor_types_test, test_pressure)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESSURE);
	sensor_type_sanitize(sensor_type);
	pressure_check(sensor_type);
}

ZTEST(sensor_types_test, test_rainfall)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_RAINFALL);
	sensor_type_sanitize(sensor_type);
	rainfall_check(sensor_type);
}

ZTEST(sensor_types_test, test_true_wind_direction)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_TRUE_WIND_DIRECTION);
	sensor_type_sanitize(sensor_type);
	plane_angle_check(sensor_type);
}

ZTEST(sensor_types_test, test_true_wind_speed)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_TRUE_WIND_SPEED);
	sensor_type_sanitize(sensor_type);
	wind_speed_check(sensor_type);
}

ZTEST(sensor_types_test, test_uv_index)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_UV_INDEX);
	sensor_type_sanitize(sensor_type);
	uv_index_check(sensor_type);
}

ZTEST(sensor_types_test, test_wind_chill)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_WIND_CHILL);
	sensor_type_sanitize(sensor_type);
	temp8_signed_celsius_check(sensor_type);
}

/* Photometry sensors */

ZTEST(sensor_types_test, test_cie_1931_chromaticity_coords)
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

ZTEST(sensor_types_test, test_present_amb_light_level)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_AMB_LIGHT_LEVEL);
	sensor_type_sanitize(sensor_type);
	illuminance_check(sensor_type);
}

ZTEST(sensor_types_test, test_correlated_col_temp)
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

ZTEST(sensor_types_test, test_present_illuminance)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_ILLUMINANCE);
	sensor_type_sanitize(sensor_type);
	illuminance_check(sensor_type);
}

ZTEST(sensor_types_test, test_luminous_flux)
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

ZTEST(sensor_types_test, test_planckian_distance)
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

ZTEST(sensor_types_test, test_rel_exposure_time_in_an_illuminance_range)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(
					BT_MESH_PROP_ID_REL_EXPOSURE_TIME_IN_AN_ILLUMINANCE_RANGE);
	sensor_type_sanitize(sensor_type);
	percentage8_illuminance_check(sensor_type);
}

ZTEST(sensor_types_test, test_tot_light_exposure_time)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_TOT_LIGHT_EXPOSURE_TIME);
	sensor_type_sanitize(sensor_type);
	time_hour_24_check(sensor_type);
}

ZTEST(sensor_types_test, test_lumen_maintenance_factor)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_LUMEN_MAINTENANCE_FACTOR);
	sensor_type_sanitize(sensor_type);
	percentage8_check(sensor_type);
}

ZTEST(sensor_types_test, test_luminous_efficacy)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_LUMINOUS_EFFICACY);
	sensor_type_sanitize(sensor_type);
	luminous_efficacy_check(sensor_type);
}

ZTEST(sensor_types_test, test_luminous_energy_since_turn_on)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_LUMINOUS_ENERGY_SINCE_TURN_ON);
	sensor_type_sanitize(sensor_type);
	luminous_energy_check(sensor_type);
}

ZTEST(sensor_types_test, test_luminous_exposure)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_LUMINOUS_EXPOSURE);
	sensor_type_sanitize(sensor_type);
	luminous_exposure_check(sensor_type);
}

ZTEST(sensor_types_test, test_luminous_flux_range)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_LUMINOUS_FLUX_RANGE);
	sensor_type_sanitize(sensor_type);
	luminous_flux_range_check(sensor_type);
}

/* Ambient temperature sensors */

ZTEST(sensor_types_test, test_avg_amb_temp_in_period_of_day)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_AVG_AMB_TEMP_IN_A_PERIOD_OF_DAY);
	sensor_type_sanitize(sensor_type);
	temp8_in_period_of_day_check(sensor_type);
}

ZTEST(sensor_types_test, test_indoor_amb_temp_stat_values)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_INDOOR_AMB_TEMP_STAT_VALUES);
	sensor_type_sanitize(sensor_type);
	temp8_statistics_check(sensor_type);
}

ZTEST(sensor_types_test, test_outdoor_stat_values)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_OUTDOOR_STAT_VALUES);
	sensor_type_sanitize(sensor_type);
	temp8_statistics_check(sensor_type);
}

ZTEST(sensor_types_test, test_present_amb_temp)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_AMB_TEMP);
	sensor_type_sanitize(sensor_type);
	temp8_check(sensor_type);
}

ZTEST(sensor_types_test, test_present_indoor_amb_temp)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_INDOOR_AMB_TEMP);
	sensor_type_sanitize(sensor_type);
	temp8_check(sensor_type);
}

ZTEST(sensor_types_test, test_present_outdoor_amb_temp)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_OUTDOOR_AMB_TEMP);
	sensor_type_sanitize(sensor_type);
	temp8_check(sensor_type);
}

ZTEST(sensor_types_test, test_desired_amb_temp)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_DESIRED_AMB_TEMP);
	sensor_type_sanitize(sensor_type);
	temp8_check(sensor_type);
}

ZTEST(sensor_types_test, test_precise_present_amb_temp)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRECISE_PRESENT_AMB_TEMP);
	sensor_type_sanitize(sensor_type);
	temp_check(sensor_type);
}

/* Energy management sensors */

ZTEST(sensor_types_test, test_dev_power_range_spec)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_DEV_POWER_RANGE_SPEC);
	sensor_type_sanitize(sensor_type);
	power_specification_check(sensor_type);
}

ZTEST(sensor_types_test, test_present_dev_input_power)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_DEV_INPUT_POWER);
	sensor_type_sanitize(sensor_type);
	power_check(sensor_type);
}

ZTEST(sensor_types_test, test_present_dev_op_efficiency)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_DEV_OP_EFFICIENCY);
	sensor_type_sanitize(sensor_type);
	percentage8_check(sensor_type);
}

ZTEST(sensor_types_test, test_tot_dev_energy_use)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_TOT_DEV_ENERGY_USE);
	sensor_type_sanitize(sensor_type);
	energy_check(sensor_type);
}

ZTEST(sensor_types_test, test_precise_tot_dev_energy_use)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRECISE_TOT_DEV_ENERGY_USE);
	sensor_type_sanitize(sensor_type);
	energy32_check(sensor_type);
}

ZTEST(sensor_types_test, test_dev_energy_use_since_turn_on)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_DEV_ENERGY_USE_SINCE_TURN_ON);
	sensor_type_sanitize(sensor_type);
	energy_check(sensor_type);
}

ZTEST(sensor_types_test, test_power_factor)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_POWER_FACTOR);
	sensor_type_sanitize(sensor_type);
	cos_of_the_angle_check(sensor_type);
}

ZTEST(sensor_types_test, test_rel_dev_energy_use_in_a_period_of_day)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(
					BT_MESH_PROP_ID_REL_DEV_ENERGY_USE_IN_A_PERIOD_OF_DAY);
	sensor_type_sanitize(sensor_type);
	energy_in_a_period_of_day_check(sensor_type);
}

ZTEST(sensor_types_test, test_apparent_energy)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_APPARENT_ENERGY);
	sensor_type_sanitize(sensor_type);
	apparent_energy32_check(sensor_type);
}

ZTEST(sensor_types_test, test_apparent_power)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_APPARENT_POWER);
	sensor_type_sanitize(sensor_type);
	apparent_power_check(sensor_type);
}

ZTEST(sensor_types_test, test_active_energy_loadside)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_ACTIVE_ENERGY_LOADSIDE);
	sensor_type_sanitize(sensor_type);
	energy32_check(sensor_type);
}

ZTEST(sensor_types_test, test_active_power_loadside)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_ACTIVE_POWER_LOADSIDE);
	sensor_type_sanitize(sensor_type);
	power_check(sensor_type);
}

/* Warranty and service sensors */

ZTEST(sensor_types_test, test_rel_dev_runtime_in_a_generic_level_range)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(
					BT_MESH_PROP_ID_REL_DEV_RUNTIME_IN_A_GENERIC_LEVEL_RANGE);
	sensor_type_sanitize(sensor_type);
	relative_runtime_in_a_generic_level_range_check(sensor_type);
}

ZTEST(sensor_types_test, test_gain)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_SENSOR_GAIN);
	sensor_type_sanitize(sensor_type);
	gain_check(sensor_type);
}

/* Electrical input sensors */

ZTEST(sensor_types_test, test_avg_input_current)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_AVG_INPUT_CURRENT);
	sensor_type_sanitize(sensor_type);
	average_current_check(sensor_type);
}

ZTEST(sensor_types_test, test_avg_input_voltage)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_AVG_INPUT_VOLTAGE);
	sensor_type_sanitize(sensor_type);
	average_voltage_check(sensor_type);
}

ZTEST(sensor_types_test, test_input_current_range_spec)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_INPUT_CURRENT_RANGE_SPEC);
	sensor_type_sanitize(sensor_type);
	electric_current_check(sensor_type);
}

ZTEST(sensor_types_test, test_input_current_stat)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_INPUT_CURRENT_STAT);
	sensor_type_sanitize(sensor_type);
	current_stat_check(sensor_type);
}

ZTEST(sensor_types_test, test_input_voltage_range_spec)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_INPUT_VOLTAGE_RANGE_SPEC);
	sensor_type_sanitize(sensor_type);
	voltage_check(sensor_type);
}

ZTEST(sensor_types_test, test_input_voltage_stat)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_INPUT_VOLTAGE_STAT);
	sensor_type_sanitize(sensor_type);
	voltage_stat_check(sensor_type);
}

ZTEST(sensor_types_test, test_present_input_current)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_INPUT_CURRENT);
	sensor_type_sanitize(sensor_type);
	electric_current_check(sensor_type);
}

ZTEST(sensor_types_test, test_present_input_ripple_voltage)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_INPUT_RIPPLE_VOLTAGE);
	sensor_type_sanitize(sensor_type);
	percentage8_check(sensor_type);
}

ZTEST(sensor_types_test, test_present_input_voltage)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_INPUT_VOLTAGE);
	sensor_type_sanitize(sensor_type);
	voltage_check(sensor_type);
}

ZTEST(sensor_types_test, test_rel_runtime_in_an_input_current_range)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(
		BT_MESH_PROP_ID_REL_RUNTIME_IN_AN_INPUT_CURRENT_RANGE);
	sensor_type_sanitize(sensor_type);
	rel_runtime_in_current_range_check(sensor_type);
}

ZTEST(sensor_types_test, test_rel_runtime_in_an_input_voltage_range)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(
		BT_MESH_PROP_ID_REL_RUNTIME_IN_AN_INPUT_VOLTAGE_RANGE);
	sensor_type_sanitize(sensor_type);
	rel_runtime_in_voltage_range_check(sensor_type);
}

/* Device operating temperature sensors */

ZTEST(sensor_types_test, test_dev_op_temp_range_spec)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_DEV_OP_TEMP_RANGE_SPEC);
	sensor_type_sanitize(sensor_type);
	temp_check(sensor_type);
}

ZTEST(sensor_types_test, test_present_dev_op_temp)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_DEV_OP_TEMP);
	sensor_type_sanitize(sensor_type);
	temp_check(sensor_type);
}

ZTEST(sensor_types_test, test_dev_op_temp_stat_values)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_DEV_OP_TEMP_STAT_VALUES);
	sensor_type_sanitize(sensor_type);
	temp_stat_check(sensor_type);
}

ZTEST(sensor_types_test, test_rel_runtime_in_a_dev_op_temp_range)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_REL_RUNTIME_IN_A_DEV_OP_TEMP_RANGE);
	sensor_type_sanitize(sensor_type);
	rel_val_in_a_temp_range_check(sensor_type);
}

/* Power output supply sensors*/

ZTEST(sensor_types_test, test_avg_output_current)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_AVG_OUTPUT_CURRENT);
	sensor_type_sanitize(sensor_type);
	average_current_check(sensor_type);
}

ZTEST(sensor_types_test, test_avg_output_voltage)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_AVG_OUTPUT_VOLTAGE);
	sensor_type_sanitize(sensor_type);
	average_voltage_check(sensor_type);
}

ZTEST(sensor_types_test, test_output_current_range)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_OUTPUT_CURRENT_RANGE);
	sensor_type_sanitize(sensor_type);
	electric_current_check(sensor_type);
}

ZTEST(sensor_types_test, test_output_current_stat)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_OUTPUT_CURRENT_STAT);
	sensor_type_sanitize(sensor_type);
	current_stat_check(sensor_type);
}

ZTEST(sensor_types_test, test_output_ripple_voltage_spec)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_OUTPUT_RIPPLE_VOLTAGE_SPEC);
	sensor_type_sanitize(sensor_type);
	percentage8_check(sensor_type);
}

ZTEST(sensor_types_test, test_output_voltage_range)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_OUTPUT_VOLTAGE_RANGE);
	sensor_type_sanitize(sensor_type);
	voltage_check(sensor_type);
}

ZTEST(sensor_types_test, test_output_voltage_stat)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_OUTPUT_VOLTAGE_STAT);
	sensor_type_sanitize(sensor_type);
	voltage_stat_check(sensor_type);
}

ZTEST(sensor_types_test, test_present_output_current)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_OUTPUT_CURRENT);
	sensor_type_sanitize(sensor_type);
	electric_current_check(sensor_type);
}

ZTEST(sensor_types_test, test_present_output_voltage)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_OUTPUT_VOLTAGE);
	sensor_type_sanitize(sensor_type);
	voltage_check(sensor_type);
}

ZTEST(sensor_types_test, test_present_rel_output_ripple_voltage)
{
	const struct bt_mesh_sensor_type *sensor_type;

	sensor_type = bt_mesh_sensor_type_get(BT_MESH_PROP_ID_PRESENT_REL_OUTPUT_RIPPLE_VOLTAGE);
	sensor_type_sanitize(sensor_type);
	percentage8_check(sensor_type);
}

ZTEST_SUITE(sensor_types_test, NULL, NULL, NULL, NULL, NULL);
