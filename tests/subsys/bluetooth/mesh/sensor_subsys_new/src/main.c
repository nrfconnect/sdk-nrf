/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <math.h>
#include <stdint.h>
#include <float.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/ztest.h>
#include <bluetooth/mesh/sensor_types.h>
#include <sensor.h> /* private header from the source folder */

#ifndef CONFIG_LITTLE_ENDIAN
BUILD_ASSERT(false, "This test is only supported on little endian platforms");
#endif

#define MICRO (1000000LL)

/****************** types section **********************************/
struct format_spec {
	size_t size;
	double ratio;
	double min;
	double max;
	struct {
		uint32_t *max_or_greater;
		uint32_t *min_or_less;
		uint32_t *unknown;
		uint32_t *invalid;
		uint32_t *prohibited; /* A single prohibited value to test */
	} specials;
};

#define SPECIAL(_name, _val) ._name = &(uint32_t){(_val)}

#define FORMAT_SPEC(_size, _M, _d, _b, _min, _max, ...)                        \
{                                                                              \
	.size = (_size),                                                       \
	.ratio = ((_M) * pow(10.0, (_d)) * exp2((_b))),                        \
	.min = (_min),                                                         \
	.max = (_max),                                                         \
	.specials = { __VA_ARGS__ }                                            \
}

struct channel_spec {
	int size;
	const struct bt_mesh_sensor_format *format;
};

#define CHANNEL(_format, _size) {                                              \
	.size = _size,                                                         \
	.format = &bt_mesh_sensor_format_##_format                             \
}
/****************** types section **********************************/

static void check_from_micro(const struct bt_mesh_sensor_format *format,
			     int64_t micro, int expected_err,
			     uint32_t expected_raw, bool exact)
{
	struct bt_mesh_sensor_value val = { 0 };
	int err = bt_mesh_sensor_value_from_micro(format, micro, &val);

	zassert_equal(err, expected_err);
	zassert_equal(val.format, format);

	uint32_t raw = sys_get_le32(val.raw);

	double error;

	if (expected_raw == 0 || exact) {
		zassert_mem_equal(val.raw, &expected_raw, 4);
	} else if (micro < 0) {
		int32_t expected_raw_signed = *(int32_t *)(&expected_raw);
		int32_t raw_signed = *(int32_t *)(&raw);

		error = (double)raw_signed / expected_raw_signed;
		zassert_between_inclusive(error, 0.99999, 1.00001);
	} else {
		error = (double)raw / expected_raw;
		zassert_between_inclusive(error, 0.99999, 1.00001);
	}
}

static void check_to_micro(const struct bt_mesh_sensor_format *format,
			   uint32_t raw,
			   enum bt_mesh_sensor_value_status expected_status,
			   int64_t expected_micro, bool exact)
{
	struct bt_mesh_sensor_value val = { .format = format };
	enum bt_mesh_sensor_value_status status;
	int64_t micro = 0;

	memcpy(val.raw, &raw, 4);
	status = bt_mesh_sensor_value_to_micro(&val, &micro);
	zassert_equal(status, expected_status);
	if (expected_micro == 0 || exact) {
		zassert_equal(micro, expected_micro);
	} else {
		double error = (double)micro / (double)expected_micro;

		zassert_between_inclusive(error, 0.99999, 1.00001);
	}
}

static void check_from_float(const struct bt_mesh_sensor_format *format,
			     float f, int expected_err,
			     uint32_t expected_raw, bool exact)
{
	struct bt_mesh_sensor_value val = { 0 };
	int err = bt_mesh_sensor_value_from_float(format, f, &val);

	zassert_equal(err, expected_err);
	zassert_equal(val.format, format);

	uint32_t raw = sys_get_le32(val.raw);

	double error;

	if (expected_raw == 0 || exact) {
		zassert_equal(raw, expected_raw);
	} else if (f < 0) {
		int32_t expected_raw_signed = *(int32_t *)(&expected_raw);
		int32_t raw_signed = *(int32_t *)(&raw);

		error = (double)raw_signed / expected_raw_signed;
		zassert_between_inclusive(error, 0.99999, 1.00001);
	} else {
		error = (double)raw / expected_raw;
		zassert_between_inclusive(error, 0.99999, 1.00001);
	}
}

static void check_to_float(const struct bt_mesh_sensor_format *format,
			   uint32_t raw,
			   enum bt_mesh_sensor_value_status expected_status,
			   float expected_f, bool exact)
{
	struct bt_mesh_sensor_value val = { .format = format };
	enum bt_mesh_sensor_value_status status;
	float f = 0.0f;

	memcpy(val.raw, &raw, 4);
	status = bt_mesh_sensor_value_to_float(&val, &f);
	zassert_equal(status, expected_status);
	if (expected_f == 0 || exact) {
		zassert_equal(f, expected_f);
	} else {
		double error = (double)f / (double)expected_f;

		zassert_between_inclusive(error, 0.99999, 1.00001);
	}
}

static void check_from_special(const struct bt_mesh_sensor_format *format,
			       enum bt_mesh_sensor_value_status special,
			       int expected_err, uint32_t expected_raw)
{
	struct bt_mesh_sensor_value val = { 0 };
	int err = bt_mesh_sensor_value_from_special_status(format, special, &val);

	zassert_equal(err, expected_err);
	if (expected_err == 0) {
		zassert_mem_equal(val.raw, &expected_raw, 4);
		zassert_equal(val.format, format);
	}
}

static void check_to_status(const struct bt_mesh_sensor_format *format,
			    uint32_t raw,
			    enum bt_mesh_sensor_value_status expected_status)
{
	struct bt_mesh_sensor_value val = { .format = format };
	enum bt_mesh_sensor_value_status status;

	memcpy(val.raw, &raw, 4);
	status = bt_mesh_sensor_value_get_status(&val);
	zassert_equal(status, expected_status);
}

static uint32_t encoded(const struct format_spec *spec, double value)
{
	int64_t val = round(value / spec->ratio);

	val &= GENMASK((spec->size * 8) - 1, 0);
	return (uint32_t)val;
}

/** Finds the biggest float less than or equal to @c limit. */
static float biggest_float_leq(double limit)
{
	float val = limit;

	return (double)val <= limit ? val : nextafterf(val, -INFINITY);
}

/** Finds the smallest float greater than or equal to @c limit. */
static float smallest_float_geq(double limit)
{
	float val = limit;

	return (double)val >= limit ? val : nextafterf(val, INFINITY);
}

static void check_scalar_format(const struct bt_mesh_sensor_format *format,
				const struct format_spec *spec)
{
	/* Decoded values inside [min, max] range */
	int64_t micro_max = round(spec->max * MICRO);
	int64_t micro_min = round(spec->min * MICRO);
	float float_max = biggest_float_leq(spec->max);
	float float_min = smallest_float_geq(spec->min);

	/* Checking encoding/decoding maximum value */
	check_from_micro(format, micro_max, 0, encoded(spec, spec->max), true);
	check_from_float(format, float_max, 0, encoded(spec, float_max), false);
	check_to_micro(format, encoded(spec, spec->max),
		       BT_MESH_SENSOR_VALUE_NUMBER, micro_max, true);
	check_to_float(format, encoded(spec, spec->max),
		       BT_MESH_SENSOR_VALUE_NUMBER, spec->max, false);
	check_to_status(format, encoded(spec, spec->max), BT_MESH_SENSOR_VALUE_NUMBER);

	/* Checking encoding/decoding minimum value */
	check_from_micro(format, micro_min, 0, encoded(spec, spec->min), true);
	check_from_float(format, float_min, 0, encoded(spec, float_min), false);
	check_to_micro(format, encoded(spec, spec->min),
		       BT_MESH_SENSOR_VALUE_NUMBER, micro_min, true);
	check_to_float(format, encoded(spec, spec->min),
		       BT_MESH_SENSOR_VALUE_NUMBER, spec->min, false);
	check_to_status(format, encoded(spec, spec->min), BT_MESH_SENSOR_VALUE_NUMBER);

	/* Decoded values outside [min, max] range */
	int64_t micro_max_plus_one = micro_max + round(MICRO * spec->ratio);
	int64_t micro_min_minus_one = micro_min - round(MICRO * spec->ratio);
	float float_max_plus_one = spec->max + spec->ratio;
	float float_min_minus_one = spec->min - spec->ratio;

	uint32_t special_raw;

	/* Checking values bigger than maximum */
	if (spec->specials.max_or_greater) {
		/* Checking special value "maximum or greater" */
		special_raw = *(spec->specials.max_or_greater);
		check_from_micro(format, micro_max_plus_one, 0, special_raw, true);
		check_from_float(format, float_max_plus_one, 0, special_raw, true);
		check_from_special(format, BT_MESH_SENSOR_VALUE_MAX_OR_GREATER, 0, special_raw);
		check_to_micro(format, special_raw, BT_MESH_SENSOR_VALUE_MAX_OR_GREATER,
			       micro_max_plus_one, true);
		check_to_float(format, special_raw, BT_MESH_SENSOR_VALUE_MAX_OR_GREATER,
			       float_max_plus_one, false);
		check_to_status(format, special_raw, BT_MESH_SENSOR_VALUE_MAX_OR_GREATER);

		/* Checking clamping of values out of range */
		int64_t micro_max_plus_two = micro_max + round(MICRO * 2 * spec->ratio);
		float float_max_plus_two = spec->max + (2 * spec->ratio);

		check_from_micro(format, micro_max_plus_two, -ERANGE, special_raw, true);
		check_from_float(format, float_max_plus_two, -ERANGE, special_raw, true);
	} else {
		/* Checking clamping of values out of range */
		check_from_micro(format, micro_max_plus_one, -ERANGE,
				 encoded(spec, spec->max), true);
		check_from_float(format, float_max_plus_one, -ERANGE,
				 encoded(spec, spec->max), true);

		/* Checking non-supported special value */
		check_from_special(format, BT_MESH_SENSOR_VALUE_MAX_OR_GREATER, -ERANGE, 0);
	}

	/* Checking values smaller than minimum */
	if (spec->specials.min_or_less) {
		/* Checking special value "minimum or less" */
		special_raw = *(spec->specials.min_or_less);
		check_from_micro(format, micro_min_minus_one, 0, special_raw, true);
		check_from_float(format, float_min_minus_one, 0, special_raw, true);
		check_from_special(format, BT_MESH_SENSOR_VALUE_MIN_OR_LESS, 0, special_raw);
		check_to_micro(format, special_raw, BT_MESH_SENSOR_VALUE_MIN_OR_LESS,
			       micro_min_minus_one, true);
		check_to_float(format, special_raw, BT_MESH_SENSOR_VALUE_MIN_OR_LESS,
			       float_min_minus_one, false);
		check_to_status(format, special_raw, BT_MESH_SENSOR_VALUE_MIN_OR_LESS);

		/* Checking clamping of values out of range */
		int64_t micro_min_minus_two = micro_min - round(MICRO * 2 * spec->ratio);
		float float_min_minus_two = spec->min - (2 * spec->ratio);

		check_from_micro(format, micro_min_minus_two, -ERANGE, special_raw, true);
		check_from_float(format, float_min_minus_two, -ERANGE, special_raw, true);
	} else {
		/* Checking clamping of values out of range */
		check_from_micro(format, micro_min_minus_one, -ERANGE,
				 encoded(spec, spec->min), true);
		check_from_float(format, float_min_minus_one, -ERANGE,
				 encoded(spec, spec->min), true);

		/* Checking non-supported special value */
		check_from_special(format, BT_MESH_SENSOR_VALUE_MIN_OR_LESS, -ERANGE, 0);
	}

	if (spec->specials.unknown) {
		/* Checking special value "value is not known" */
		special_raw = *(spec->specials.unknown);
		check_from_special(format, BT_MESH_SENSOR_VALUE_UNKNOWN, 0, special_raw);
		check_to_status(format, special_raw, BT_MESH_SENSOR_VALUE_UNKNOWN);
		check_to_float(format, special_raw, BT_MESH_SENSOR_VALUE_UNKNOWN, 0.0f, true);
		check_to_micro(format, special_raw, BT_MESH_SENSOR_VALUE_UNKNOWN, 0, true);
	} else {
		/* Checking non-supported special value "value is not known" */
		check_from_special(format, BT_MESH_SENSOR_VALUE_UNKNOWN, -ERANGE, 0);
	}

	if (spec->specials.invalid) {
		/* Checking special value "value is invalid" */
		special_raw = *(spec->specials.invalid);
		check_from_special(format, BT_MESH_SENSOR_VALUE_INVALID, 0, special_raw);
		check_to_status(format, special_raw, BT_MESH_SENSOR_VALUE_INVALID);
		check_to_float(format, special_raw, BT_MESH_SENSOR_VALUE_INVALID, 0.0f, true);
		check_to_micro(format, special_raw, BT_MESH_SENSOR_VALUE_INVALID, 0, true);
	} else {
		/* Checking non-supported special value "value is invalid" */
		check_from_special(format, BT_MESH_SENSOR_VALUE_INVALID, -ERANGE, 0);
	}

	if (spec->specials.prohibited) {
		/* Checking decoding a prohibited raw value */
		special_raw = *(spec->specials.prohibited);
		check_to_status(format, special_raw, BT_MESH_SENSOR_VALUE_CONVERSION_ERROR);
		check_to_float(format, special_raw, BT_MESH_SENSOR_VALUE_CONVERSION_ERROR,
			       0.0f, true);
		check_to_micro(format, special_raw, BT_MESH_SENSOR_VALUE_CONVERSION_ERROR,
			       0, true);
	}
}

#define TEST_SCALAR_FORMAT(_name, _spec)                                       \
ZTEST(sensor_types_test_new, test_format_##_name)                              \
{                                                                              \
	struct format_spec spec = _spec;                                       \
	check_scalar_format(&bt_mesh_sensor_format_##_name, &spec);            \
}

static void check_sensor_type(const struct bt_mesh_sensor_type *type,
			      uint16_t id,
			      const struct channel_spec *channels,
			      size_t channels_count)
{
	uint8_t test_data[BT_MESH_SENSOR_ENCODED_VALUE_MAXLEN];
	struct bt_mesh_sensor_value values[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];
	int total_size = 0;

	for (int i = 0; i < ARRAY_SIZE(test_data); i++) {
		test_data[i] = i + 1;
	}

	for (int i = 0; i < channels_count; i++) {
		total_size += channels[i].size;
	}

	zassert_equal(bt_mesh_sensor_type_get(id), type);
	zassert_true(total_size <= ARRAY_SIZE(test_data),
		     "Not enough test data available to test this type");

	NET_BUF_SIMPLE_DEFINE(buf, total_size);
	(void)net_buf_simple_add_mem(&buf, test_data, total_size);

	zassert_equal(buf.len, total_size);
	zassert_true(channels_count <= CONFIG_BT_MESH_SENSOR_CHANNELS_MAX);
	zassert_ok(sensor_value_decode(&buf, type, values));
	zassert_equal(buf.len, 0);

	const uint8_t *current_data = test_data;

	for (int i = 0; i < channels_count; i++) {
		zassert_equal(values[i].format, channels[i].format);
		zassert_equal(values[i].format->size, channels[i].size);
		zassert_mem_equal(values[i].raw, current_data, channels[i].size);
		current_data += channels[i].size;
	}

	net_buf_simple_reset(&buf);
	memset(buf.data, 0, total_size);
	zassert_ok(sensor_value_encode(&buf, type, values));
	zassert_equal(buf.len, total_size);
	zassert_mem_equal(buf.data, test_data, total_size);
}

#define TEST_SENSOR_TYPE(_name, _id, ...)                                      \
ZTEST(sensor_types_test_new, test_type_##_name)                                \
{                                                                              \
	check_sensor_type(                                                     \
		&bt_mesh_sensor_##_name,                                       \
		(_id),                                                         \
		((struct channel_spec[]){ __VA_ARGS__ }),                      \
		ARRAY_SIZE(((struct channel_spec[]){ __VA_ARGS__ })));         \
}

TEST_SCALAR_FORMAT(percentage_8,
		   FORMAT_SPEC(1, 1, 0, -1, 0.0, 100.0,
			       SPECIAL(unknown, 0xff),
			       SPECIAL(prohibited, 0xC9)))

TEST_SCALAR_FORMAT(percentage_16,
		   FORMAT_SPEC(2, 1, -2, 0, 0.0, 100.0,
			       SPECIAL(unknown, 0xffff),
			       SPECIAL(prohibited, 0x2711)))

TEST_SCALAR_FORMAT(percentage_delta_trigger,
		   FORMAT_SPEC(2, 1, -2, 0, 0.0, 655.35))

TEST_SCALAR_FORMAT(temp_8,
		   FORMAT_SPEC(1, 1, 0, -1, -64.0, 63.0,
			       SPECIAL(unknown, 0x7f)))

TEST_SCALAR_FORMAT(temp,
		   FORMAT_SPEC(2, 1, -2, 0, -273.15, 327.67,
			       SPECIAL(unknown, 0x8000),
			       SPECIAL(prohibited, 0x954C)))

TEST_SCALAR_FORMAT(co2_concentration,
		   FORMAT_SPEC(2, 1, 0, 0, 0, 65533,
			       SPECIAL(max_or_greater, 0xFFFE),
			       SPECIAL(unknown, 0xFFFF)))

TEST_SCALAR_FORMAT(noise,
		   FORMAT_SPEC(1, 1, 0, 0, 0, 253,
			       SPECIAL(max_or_greater, 0xFE),
			       SPECIAL(unknown, 0xFF)))

TEST_SCALAR_FORMAT(voc_concentration,
		   FORMAT_SPEC(2, 1, 0, 0, 0, 65533,
			       SPECIAL(max_or_greater, 0xFFFE),
			       SPECIAL(unknown, 0xFFFF)))

TEST_SCALAR_FORMAT(wind_speed,
		   FORMAT_SPEC(2, 1, -2, 0, 0, 655.35))

TEST_SCALAR_FORMAT(temp_8_wide,
		   FORMAT_SPEC(1, 1, 0, 0, -128, 127))

TEST_SCALAR_FORMAT(gust_factor,
		   FORMAT_SPEC(1, 1, -1, 0, 0, 25.5))

/* GSS specifies d = -7. Implemented in the stack as d = -1 with unit of
 * microtesla, to be able to work with full precision using to_micro/from_micro
 * and to/from sensor_value.
 * Testing using d = -1
 */
TEST_SCALAR_FORMAT(magnetic_flux_density,
		   FORMAT_SPEC(2, 1, -1, 0, -3276.8, 3276.7))

TEST_SCALAR_FORMAT(pollen_concentration,
		   FORMAT_SPEC(3, 1, 0, 0, 0, 16777215))

TEST_SCALAR_FORMAT(pressure,
		   FORMAT_SPEC(4, 1, -1, 0, 0, 429496729.5))

TEST_SCALAR_FORMAT(rainfall,
		   FORMAT_SPEC(2, 1, -3, 0, 0, 65.535))

TEST_SCALAR_FORMAT(uv_index,
		   FORMAT_SPEC(1, 1, 0, 0, 0, 255))

TEST_SCALAR_FORMAT(time_decihour_8,
		   FORMAT_SPEC(1, 1, -1, 0, 0, 23.9,
			       SPECIAL(unknown, 0xFF),
			       SPECIAL(prohibited, 0xF0)))

TEST_SCALAR_FORMAT(time_hour_24,
		   FORMAT_SPEC(3, 1, 0, 0, 0, 16777214,
			       SPECIAL(unknown, 0xFFFFFF)))

TEST_SCALAR_FORMAT(time_second_16,
		   FORMAT_SPEC(2, 1, 0, 0, 0, 65534,
			       SPECIAL(unknown, 0xFFFF)))

TEST_SCALAR_FORMAT(time_millisecond_24,
		   FORMAT_SPEC(3, 1, -3, 0, 0, 16777.214,
			       SPECIAL(unknown, 0xFFFFFF)))

ZTEST(sensor_types_test_new, test_format_time_exp_8)
{
	const struct bt_mesh_sensor_format *fmt = &bt_mesh_sensor_format_time_exp_8;

	/*                         Specially encoded 0
	 *                         |     Lowest normal value
	 *                         |     |     Encodes "1"
	 *                         |     |     |     Encodes "1.1"
	 *                         |     |     |     |     Middle of range
	 *                         |     |     |     |     |     Highest normal value
	 *                         v     v     v     v     v     v
	 */
	uint32_t test_vector[] = { 0x00, 0x01, 0x40, 0x41, 0x80, 0xfd};

	for (int i = 0; i < ARRAY_SIZE(test_vector); i++) {
		/* Representation: decoded value = 1.1^(encoded - 64) seconds */
		uint32_t raw = test_vector[i];
		/* raw value == 0 is defined to be 0 seconds */
		double exp_1_1 = raw == 0 ? 0.0 : pow(1.1, raw - 64.0);
		float f = exp_1_1;
		int64_t micro = exp_1_1 * MICRO;

		check_from_micro(fmt, micro, 0, raw, true);
		check_from_float(fmt, f, 0, raw, true);
		/* Checking with exact == false for to_micro because in this case it is computed
		 * via float.
		 */
		check_to_micro(fmt, raw, BT_MESH_SENSOR_VALUE_NUMBER, micro, false);
		check_to_float(fmt, raw, BT_MESH_SENSOR_VALUE_NUMBER, f, false);
		check_to_status(fmt, raw, BT_MESH_SENSOR_VALUE_NUMBER);
	}

	/* Checking special values*/
	uint32_t total_device_life_raw = 0xFE;
	uint32_t unknown_raw = 0xFF;

	check_from_special(fmt, BT_MESH_SENSOR_VALUE_TOTAL_DEVICE_LIFE, 0, total_device_life_raw);
	check_to_micro(fmt, total_device_life_raw,
		       BT_MESH_SENSOR_VALUE_TOTAL_DEVICE_LIFE, 0, true);
	check_to_float(fmt, total_device_life_raw,
		       BT_MESH_SENSOR_VALUE_TOTAL_DEVICE_LIFE, 0.0f, true);
	check_to_status(fmt, total_device_life_raw, BT_MESH_SENSOR_VALUE_TOTAL_DEVICE_LIFE);

	check_from_special(fmt, BT_MESH_SENSOR_VALUE_UNKNOWN, 0, unknown_raw);
	check_to_micro(fmt, unknown_raw, BT_MESH_SENSOR_VALUE_UNKNOWN, 0, true);
	check_to_float(fmt, unknown_raw, BT_MESH_SENSOR_VALUE_UNKNOWN, 0.0f, true);
	check_to_status(fmt, unknown_raw, BT_MESH_SENSOR_VALUE_UNKNOWN);
}

TEST_SCALAR_FORMAT(electric_current,
		   FORMAT_SPEC(2, 1, -2, 0, 0, 655.34,
			       SPECIAL(unknown, 0xFFFF)))

/* GSS defines this in a weird way compared to all other characteristics, where
 * the values for min (0.0) and max (1022.0) are actually the values for
 * "0.0 or less" and "1022.0 or greater". Since the test format spec expects
 * these special values to be excluded from the min and max range specified,
 * the min and max used here are 0 + 2^-6 and 1022.0 - 2^-6.
 */
TEST_SCALAR_FORMAT(voltage,
		   FORMAT_SPEC(2, 1, 0, -6, 0.015625, 1021.984375,
			       SPECIAL(unknown, 0xFFFF),
			       SPECIAL(max_or_greater, 0xFF80),
			       SPECIAL(min_or_less, 0x0000),
			       SPECIAL(prohibited, 0xFF81)))

TEST_SCALAR_FORMAT(energy32,
		   FORMAT_SPEC(4, 1, -3, 0, 0, 4294967.293,
			       SPECIAL(invalid, 0xFFFFFFFE),
			       SPECIAL(unknown, 0xFFFFFFFF)))

TEST_SCALAR_FORMAT(apparent_energy32,
		   FORMAT_SPEC(4, 1, -3, 0, 0, 4294967.293,
			       SPECIAL(invalid, 0xFFFFFFFE),
			       SPECIAL(unknown, 0xFFFFFFFF)))

TEST_SCALAR_FORMAT(apparent_power,
		   FORMAT_SPEC(3, 1, -1, 0, 0, 1677721.3,
			       SPECIAL(invalid, 0xFFFFFE),
			       SPECIAL(unknown, 0xFFFFFF)))

TEST_SCALAR_FORMAT(power,
		   FORMAT_SPEC(3, 1, -1, 0, 0, 1677721.4,
			       SPECIAL(unknown, 0xFFFFFF)))

TEST_SCALAR_FORMAT(energy,
		   FORMAT_SPEC(3, 1, 0, 0, 0, 16777214,
			       SPECIAL(unknown, 0xFFFFFF)))

TEST_SCALAR_FORMAT(chromatic_distance,
		   FORMAT_SPEC(2, 1, -5, 0, -0.05, 0.05,
			       SPECIAL(invalid, 0x7FFF),
			       SPECIAL(unknown, 0x7FFE),
			       SPECIAL(prohibited, 0x1389)))

/* Note: According to GSS, "Unit is unitless with a resolution of 1/65535" and
 * "Maximum: 1.0". However, it also states that "b = -16". Since this is
 * contradictory and the stack currently encodes/decodes according to
 * "b = -16", the max is recomputed in this test to 0.9999847 (~= 65535/65536)
 */
TEST_SCALAR_FORMAT(chromaticity_coordinate,
		   FORMAT_SPEC(2, 1, 0, -16, 0, 0.9999847))

TEST_SCALAR_FORMAT(correlated_color_temp,
		   FORMAT_SPEC(2, 1, 0, 0, 800, 65534,
			       SPECIAL(unknown, 0xFFFF),
			       SPECIAL(prohibited, 0x031f)))

TEST_SCALAR_FORMAT(illuminance,
		   FORMAT_SPEC(3, 1, -2, 0, 0, 167772.14,
			       SPECIAL(unknown, 0xFFFFFF)))

TEST_SCALAR_FORMAT(luminous_efficacy,
		   FORMAT_SPEC(2, 1, -1, 0, 0, 1800,
			       SPECIAL(unknown, 0xFFFF),
			       SPECIAL(prohibited, 0x4651)))

TEST_SCALAR_FORMAT(luminous_energy,
		   FORMAT_SPEC(3, 1, 3, 0, 0, 16777214000.0,
			       SPECIAL(unknown, 0xFFFFFF)))

TEST_SCALAR_FORMAT(luminous_exposure,
		   FORMAT_SPEC(3, 1, 3, 0, 0, 16777214000.0,
			       SPECIAL(unknown, 0xFFFFFF)))

TEST_SCALAR_FORMAT(luminous_flux,
		   FORMAT_SPEC(2, 1, 0, 0, 0, 65534,
			       SPECIAL(unknown, 0xFFFF)))

TEST_SCALAR_FORMAT(perceived_lightness,
		   FORMAT_SPEC(2, 1, 0, 0, 0, 65535))

TEST_SCALAR_FORMAT(direction_16,
		   FORMAT_SPEC(2, 1, -2, 0, 0, 359.99,
			       SPECIAL(prohibited, 0x8CA0)))

TEST_SCALAR_FORMAT(count_16,
		   FORMAT_SPEC(2, 1, 0, 0, 0, 65534,
			       SPECIAL(unknown, 0xFFFF)))

TEST_SCALAR_FORMAT(gen_lvl,
		   FORMAT_SPEC(2, 1, 0, 0, 0, 65535))

/* GSS specifies d = -2, however, it also says that the value is
 * "expressed as Cos(theta) x 100, with a resolution of 1", implying d = 1.
 * Min = -100 and max = 100 are also specified in terms of 100ths of the cosine.
 * The stack currently returns values in hundredths, i.e. d = 0, so this is
 * what is tested.
 */
TEST_SCALAR_FORMAT(cos_of_the_angle,
		   FORMAT_SPEC(1, 1, 0, 0, -100, 100,
			       SPECIAL(unknown, 0x7F),
			       SPECIAL(prohibited, 0x9B)))

ZTEST(sensor_types_test_new, test_format_boolean)
{
	const struct bt_mesh_sensor_format *fmt = &bt_mesh_sensor_format_boolean;

	/* Checking false (0) */
	check_from_micro(fmt, 0, 0, 0x00, true);
	check_from_float(fmt, 0.0f, 0, 0x00, true);
	check_to_micro(fmt, 0x00, BT_MESH_SENSOR_VALUE_NUMBER, 0, true);
	check_to_float(fmt, 0x00, BT_MESH_SENSOR_VALUE_NUMBER, 0.0f, true);
	check_to_status(fmt, 0x00, BT_MESH_SENSOR_VALUE_NUMBER);

	/* Checking true (1) */
	check_from_micro(fmt, 1 * MICRO, 0, 0x01, true);
	check_from_float(fmt, 1.0f, 0, 0x01, true);
	check_to_micro(fmt, 0x01, BT_MESH_SENSOR_VALUE_NUMBER, 1 * MICRO, true);
	check_to_float(fmt, 0x01, BT_MESH_SENSOR_VALUE_NUMBER, 1.0f, true);
	check_to_status(fmt, 0x01, BT_MESH_SENSOR_VALUE_NUMBER);

	/* Checking out of range float/micro */
	check_from_micro(fmt, 2 * MICRO, -ERANGE, 0x01, true);
	check_from_float(fmt, 2.0f, -ERANGE, 0x01, true);

	/* Checking prohibited */
	check_to_micro(fmt, 0x02, BT_MESH_SENSOR_VALUE_CONVERSION_ERROR, 0, true);
	check_to_float(fmt, 0x02, BT_MESH_SENSOR_VALUE_CONVERSION_ERROR, 0.0f, true);
}

ZTEST(sensor_types_test_new, test_format_coefficient)
{
	const struct bt_mesh_sensor_format *fmt = &bt_mesh_sensor_format_coefficient;

	/* Coefficient is encoded as a raw 32 bit float */
	float test_vector_float[] = {
		-FLT_MAX, -FLT_MIN, 0.0, FLT_MIN, *(float *)&(uint32_t){ 0x12345678 }, FLT_MAX
	};

	for (int i = 0; i < ARRAY_SIZE(test_vector_float); i++) {
		float f = test_vector_float[i];
		uint32_t raw = *(uint32_t *)&f;

		check_from_float(fmt, f, 0, raw, true);
		check_to_float(fmt, raw, BT_MESH_SENSOR_VALUE_NUMBER, f, true);
		check_to_status(fmt, raw, BT_MESH_SENSOR_VALUE_NUMBER);
	}

	int64_t test_vector_micro[] = { INT64_MIN, 0, 1, 1000000, INT64_MAX };

	for (int i = 0; i < ARRAY_SIZE(test_vector_micro); i++) {
		int64_t micro = test_vector_micro[i];
		float f = micro / (double)MICRO;
		uint32_t raw = *(uint32_t *)&f;

		check_from_micro(fmt, micro, 0, raw, false);
		check_to_micro(fmt, raw, BT_MESH_SENSOR_VALUE_NUMBER, micro, false);
		check_to_status(fmt, raw, BT_MESH_SENSOR_VALUE_NUMBER);
	}

	/* Check clamping behavior for converting large numbers to micro */
	float gt_i64_max = nextafterf(INT64_MAX, INFINITY);
	float lt_i64_min = nextafterf(INT64_MIN, -INFINITY);

	check_to_micro(fmt, *(uint32_t *)&gt_i64_max,
		       BT_MESH_SENSOR_VALUE_CLAMPED, INT64_MAX, true);
	check_to_micro(fmt, *(uint32_t *)&lt_i64_min,
		       BT_MESH_SENSOR_VALUE_CLAMPED, INT64_MIN, true);
}

TEST_SENSOR_TYPE(motion_sensed, 0x0042, CHANNEL(percentage_8, 1))

TEST_SENSOR_TYPE(motion_threshold, 0x0043, CHANNEL(percentage_8, 1))

TEST_SENSOR_TYPE(people_count, 0x004c, CHANNEL(count_16, 2))

TEST_SENSOR_TYPE(presence_detected, 0x004d, CHANNEL(boolean, 1))

TEST_SENSOR_TYPE(time_since_motion_sensed, 0x0068, CHANNEL(time_millisecond_24, 3))

TEST_SENSOR_TYPE(time_since_presence_detected, 0x0069, CHANNEL(time_second_16, 2))

TEST_SENSOR_TYPE(avg_amb_temp_in_day, 0x0001,
		 CHANNEL(temp_8, 1),
		 CHANNEL(time_decihour_8, 1),
		 CHANNEL(time_decihour_8, 1))

TEST_SENSOR_TYPE(indoor_amb_temp_stat_values, 0x001C,
		 CHANNEL(temp_8, 1),
		 CHANNEL(temp_8, 1),
		 CHANNEL(temp_8, 1),
		 CHANNEL(temp_8, 1),
		 CHANNEL(time_exp_8, 1))

TEST_SENSOR_TYPE(outdoor_stat_values, 0x0045,
		 CHANNEL(temp_8, 1),
		 CHANNEL(temp_8, 1),
		 CHANNEL(temp_8, 1),
		 CHANNEL(temp_8, 1),
		 CHANNEL(time_exp_8, 1))

TEST_SENSOR_TYPE(present_amb_temp, 0x004f, CHANNEL(temp_8, 1))

TEST_SENSOR_TYPE(present_indoor_amb_temp, 0x0056, CHANNEL(temp_8, 1))

TEST_SENSOR_TYPE(present_outdoor_amb_temp, 0x005b, CHANNEL(temp_8, 1))

TEST_SENSOR_TYPE(desired_amb_temp, 0x0071, CHANNEL(temp_8, 1))

TEST_SENSOR_TYPE(precise_present_amb_temp, 0x0075, CHANNEL(temp, 2))

TEST_SENSOR_TYPE(apparent_wind_direction, 0x0085, CHANNEL(direction_16, 2))

TEST_SENSOR_TYPE(apparent_wind_speed, 0x0086, CHANNEL(wind_speed, 2))

TEST_SENSOR_TYPE(dew_point, 0x0087, CHANNEL(temp_8_wide, 1))

TEST_SENSOR_TYPE(gust_factor, 0x008a, CHANNEL(gust_factor, 1))

TEST_SENSOR_TYPE(heat_index, 0x008b, CHANNEL(temp_8_wide, 1))

TEST_SENSOR_TYPE(present_amb_rel_humidity, 0x0076, CHANNEL(percentage_16, 2))

TEST_SENSOR_TYPE(present_amb_co2_concentration, 0x0077, CHANNEL(co2_concentration, 2))

TEST_SENSOR_TYPE(present_amb_voc_concentration, 0x0078, CHANNEL(voc_concentration, 2))

TEST_SENSOR_TYPE(present_amb_noise, 0x0079, CHANNEL(noise, 1))

TEST_SENSOR_TYPE(present_indoor_relative_humidity, 0x00a7, CHANNEL(percentage_16, 2))

TEST_SENSOR_TYPE(present_outdoor_relative_humidity, 0x00a8, CHANNEL(percentage_16, 2))

TEST_SENSOR_TYPE(magnetic_declination, 0x00a1, CHANNEL(direction_16, 2))

TEST_SENSOR_TYPE(magnetic_flux_density_2d, 0x00a2,
		 CHANNEL(magnetic_flux_density, 2),
		 CHANNEL(magnetic_flux_density, 2))

TEST_SENSOR_TYPE(magnetic_flux_density_3d, 0x00a3,
		 CHANNEL(magnetic_flux_density, 2),
		 CHANNEL(magnetic_flux_density, 2),
		 CHANNEL(magnetic_flux_density, 2))

TEST_SENSOR_TYPE(pollen_concentration, 0x00a6, CHANNEL(pollen_concentration, 3))

TEST_SENSOR_TYPE(air_pressure, 0x0082, CHANNEL(pressure, 4))

TEST_SENSOR_TYPE(pressure, 0x00a9, CHANNEL(pressure, 4))

TEST_SENSOR_TYPE(rainfall, 0x00aa, CHANNEL(rainfall, 2))

TEST_SENSOR_TYPE(true_wind_direction, 0x00af, CHANNEL(direction_16, 2))

TEST_SENSOR_TYPE(true_wind_speed, 0x00b0, CHANNEL(wind_speed, 2))

TEST_SENSOR_TYPE(uv_index, 0x00b1, CHANNEL(uv_index, 1))

TEST_SENSOR_TYPE(wind_chill, 0x00b2, CHANNEL(temp_8_wide, 1))

TEST_SENSOR_TYPE(dev_op_temp_range_spec, 0x0013, CHANNEL(temp, 2), CHANNEL(temp, 2))

TEST_SENSOR_TYPE(dev_op_temp_stat_values, 0x0014,
		 CHANNEL(temp, 2),
		 CHANNEL(temp, 2),
		 CHANNEL(temp, 2),
		 CHANNEL(temp, 2),
		 CHANNEL(time_exp_8, 1))

TEST_SENSOR_TYPE(present_dev_op_temp, 0x0054, CHANNEL(temp, 2))

TEST_SENSOR_TYPE(rel_runtime_in_a_dev_op_temp_range, 0x0064,
		 CHANNEL(percentage_8, 1),
		 CHANNEL(temp, 2),
		 CHANNEL(temp, 2))

TEST_SENSOR_TYPE(avg_input_current, 0x0002,
		 CHANNEL(electric_current, 2),
		 CHANNEL(time_exp_8, 1))

TEST_SENSOR_TYPE(avg_input_voltage, 0x0003,
		 CHANNEL(voltage, 2),
		 CHANNEL(time_exp_8, 1))

TEST_SENSOR_TYPE(input_current_range_spec, 0x0021,
		 CHANNEL(electric_current, 2),
		 CHANNEL(electric_current, 2),
		 CHANNEL(electric_current, 2))

TEST_SENSOR_TYPE(input_current_stat, 0x0022,
		 CHANNEL(electric_current, 2),
		 CHANNEL(electric_current, 2),
		 CHANNEL(electric_current, 2),
		 CHANNEL(electric_current, 2),
		 CHANNEL(time_exp_8, 1))

TEST_SENSOR_TYPE(input_voltage_range_spec, 0x0028,
		 CHANNEL(voltage, 2),
		 CHANNEL(voltage, 2),
		 CHANNEL(voltage, 2))

TEST_SENSOR_TYPE(input_voltage_stat, 0x002a,
		 CHANNEL(voltage, 2),
		 CHANNEL(voltage, 2),
		 CHANNEL(voltage, 2),
		 CHANNEL(voltage, 2),
		 CHANNEL(time_exp_8, 1))

TEST_SENSOR_TYPE(present_input_current, 0x0057, CHANNEL(electric_current, 2))

TEST_SENSOR_TYPE(present_input_ripple_voltage, 0x0058, CHANNEL(percentage_8, 1))

TEST_SENSOR_TYPE(present_input_voltage, 0x0059, CHANNEL(voltage, 2))

TEST_SENSOR_TYPE(rel_runtime_in_an_input_current_range, 0x0065,
		 CHANNEL(percentage_8, 1),
		 CHANNEL(electric_current, 2),
		 CHANNEL(electric_current, 2))

TEST_SENSOR_TYPE(rel_runtime_in_an_input_voltage_range, 0x0066,
		 CHANNEL(percentage_8, 1),
		 CHANNEL(voltage, 2),
		 CHANNEL(voltage, 2))

TEST_SENSOR_TYPE(dev_power_range_spec, 0x0016,
		 CHANNEL(power, 3),
		 CHANNEL(power, 3),
		 CHANNEL(power, 3))

TEST_SENSOR_TYPE(present_dev_input_power, 0x0052, CHANNEL(power, 3))

TEST_SENSOR_TYPE(present_dev_op_efficiency, 0x0053, CHANNEL(percentage_8, 1))

TEST_SENSOR_TYPE(tot_dev_energy_use, 0x006a, CHANNEL(energy, 3))

TEST_SENSOR_TYPE(precise_tot_dev_energy_use, 0x0072, CHANNEL(energy32, 4))

TEST_SENSOR_TYPE(dev_energy_use_since_turn_on, 0x000d, CHANNEL(energy, 3))

TEST_SENSOR_TYPE(power_factor, 0x0073, CHANNEL(cos_of_the_angle, 1))

TEST_SENSOR_TYPE(rel_dev_energy_use_in_a_period_of_day, 0x0060,
		 CHANNEL(energy, 3),
		 CHANNEL(time_decihour_8, 1),
		 CHANNEL(time_decihour_8, 1))

TEST_SENSOR_TYPE(apparent_energy, 0x0083, CHANNEL(apparent_energy32, 4))

TEST_SENSOR_TYPE(apparent_power, 0x0084, CHANNEL(apparent_power, 3))

TEST_SENSOR_TYPE(active_energy_loadside, 0x0080, CHANNEL(energy32, 4))

TEST_SENSOR_TYPE(active_power_loadside, 0x0081, CHANNEL(power, 3))

TEST_SENSOR_TYPE(present_amb_light_level, 0x004E, CHANNEL(illuminance, 3))

TEST_SENSOR_TYPE(initial_cie_1931_chromaticity_coords, 0x001d,
		 CHANNEL(chromaticity_coordinate, 2),
		 CHANNEL(chromaticity_coordinate, 2))

TEST_SENSOR_TYPE(present_cie_1931_chromaticity_coords, 0x0050,
		 CHANNEL(chromaticity_coordinate, 2),
		 CHANNEL(chromaticity_coordinate, 2))

TEST_SENSOR_TYPE(initial_correlated_col_temp, 0x001e, CHANNEL(correlated_color_temp, 2))

TEST_SENSOR_TYPE(present_correlated_col_temp, 0x0051, CHANNEL(correlated_color_temp, 2))

TEST_SENSOR_TYPE(present_illuminance, 0x0055, CHANNEL(illuminance, 3))

TEST_SENSOR_TYPE(initial_luminous_flux, 0x001f, CHANNEL(luminous_flux, 2))

TEST_SENSOR_TYPE(present_luminous_flux, 0x005a, CHANNEL(luminous_flux, 2))

TEST_SENSOR_TYPE(initial_planckian_distance, 0x0020, CHANNEL(chromatic_distance, 2))

TEST_SENSOR_TYPE(present_planckian_distance, 0x005e, CHANNEL(chromatic_distance, 2))

TEST_SENSOR_TYPE(rel_exposure_time_in_an_illuminance_range, 0x0062,
		 CHANNEL(percentage_8, 1),
		 CHANNEL(illuminance, 3),
		 CHANNEL(illuminance, 3))

TEST_SENSOR_TYPE(tot_light_exposure_time, 0x006f, CHANNEL(time_hour_24, 3))

TEST_SENSOR_TYPE(lumen_maintenance_factor, 0x003d, CHANNEL(percentage_8, 1))

TEST_SENSOR_TYPE(luminous_efficacy, 0x003e, CHANNEL(luminous_efficacy, 2))

TEST_SENSOR_TYPE(luminous_energy_since_turn_on, 0x003f, CHANNEL(luminous_energy, 3))

TEST_SENSOR_TYPE(luminous_exposure, 0x0040, CHANNEL(luminous_exposure, 3))

TEST_SENSOR_TYPE(luminous_flux_range, 0x0041,
		 CHANNEL(luminous_flux, 2),
		 CHANNEL(luminous_flux, 2))

TEST_SENSOR_TYPE(avg_output_current, 0x0004,
		 CHANNEL(electric_current, 2),
		 CHANNEL(time_exp_8, 1))

TEST_SENSOR_TYPE(avg_output_voltage, 0x0005,
		 CHANNEL(voltage, 2),
		 CHANNEL(time_exp_8, 1))

TEST_SENSOR_TYPE(output_current_range, 0x0046,
		 CHANNEL(electric_current, 2),
		 CHANNEL(electric_current, 2))

TEST_SENSOR_TYPE(output_current_stat, 0x0047,
		 CHANNEL(electric_current, 2),
		 CHANNEL(electric_current, 2),
		 CHANNEL(electric_current, 2),
		 CHANNEL(electric_current, 2),
		 CHANNEL(time_exp_8, 1))

TEST_SENSOR_TYPE(output_ripple_voltage_spec, 0x0048, CHANNEL(percentage_8, 1))

TEST_SENSOR_TYPE(output_voltage_range, 0x0049,
		 CHANNEL(voltage, 2),
		 CHANNEL(voltage, 2),
		 CHANNEL(voltage, 2))

TEST_SENSOR_TYPE(output_voltage_stat, 0x004a,
		 CHANNEL(voltage, 2),
		 CHANNEL(voltage, 2),
		 CHANNEL(voltage, 2),
		 CHANNEL(voltage, 2),
		 CHANNEL(time_exp_8, 1))

TEST_SENSOR_TYPE(present_output_current, 0x005c, CHANNEL(electric_current, 2))

TEST_SENSOR_TYPE(present_output_voltage, 0x005d, CHANNEL(voltage, 2))

TEST_SENSOR_TYPE(present_rel_output_ripple_voltage, 0x005f, CHANNEL(percentage_8, 1))

TEST_SENSOR_TYPE(gain, 0x0074, CHANNEL(coefficient, 4))

TEST_SENSOR_TYPE(rel_dev_runtime_in_a_generic_level_range, 0x0061,
		 CHANNEL(percentage_8, 1),
		 CHANNEL(gen_lvl, 2),
		 CHANNEL(gen_lvl, 2))

TEST_SENSOR_TYPE(total_dev_runtime, 0x006e, CHANNEL(time_hour_24, 3))

ZTEST_SUITE(sensor_types_test_new, NULL, NULL, NULL, NULL, NULL);
