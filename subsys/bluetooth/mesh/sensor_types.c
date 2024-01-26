/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "sensor.h"
#include <zephyr/sys/byteorder.h>
#include <zephyr/toolchain.h>
#include <bluetooth/mesh/properties.h>
#include <bluetooth/mesh/sensor_types.h>

/* Constants: */

/* logf(1.1f) */
#define LOG_1_1 (0.095310203f)

/* Various shorthand macros to improve readability and maintainability of this
 * file:
 */

#define UNIT(name) const struct bt_mesh_sensor_unit bt_mesh_sensor_unit_##name

#define CHANNELS(...)                                                          \
	.channels = ((const struct bt_mesh_sensor_channel[]){ __VA_ARGS__ }),  \
	.channel_count = ARRAY_SIZE(                                           \
		((const struct bt_mesh_sensor_channel[]){ __VA_ARGS__ }))

#define FORMAT(_name)                                                          \
	const struct bt_mesh_sensor_format bt_mesh_sensor_format_##_name

#define SENSOR_TYPE(name)                                                      \
	const STRUCT_SECTION_ITERABLE(bt_mesh_sensor_type,                     \
					bt_mesh_sensor_##name)

#ifdef CONFIG_BT_MESH_SENSOR_LABELS

#define CHANNEL(_name, _format)                                                \
	{                                                                      \
		.format = &bt_mesh_sensor_format_##_format, .name = _name      \
	}
#else

#define CHANNEL(_name, _format)                                                \
	{                                                                      \
		.format = &bt_mesh_sensor_format_##_format,                    \
	}

#endif

/*******************************************************************************
 * Units
 ******************************************************************************/
UNIT(hours) = { "Hours", "h" };
UNIT(seconds) = { "Seconds", "s" };
UNIT(celsius) = { "Celsius", "C" };
UNIT(kelvin) = { "Kelvin", "K" };
UNIT(percent) = { "Percent", "%" };
UNIT(ppm) = { "Parts per million", "ppm" };
UNIT(ppb) = { "Parts per billion", "ppb" };
UNIT(volt) = { "Volt", "V" };
UNIT(ampere) = { "Ampere", "A" };
UNIT(watt) = { "Watt", "W" };
UNIT(kwh) = { "Kilo Watt hours", "kWh" };
UNIT(va) = { "Volt Ampere", "VA" };
UNIT(kvah) = { "Kilo Volt Ampere hours", "kVAh" };
UNIT(db) = { "Decibel", "dB" };
UNIT(lux) = { "Lux", "lx" };
UNIT(klxh) = { "Kilo Lux hours", "klxh" };
UNIT(lumen) = { "Lumen", "lm" };
UNIT(lumen_per_watt) = { "Lumen per Watt", "lm/W" };
UNIT(klmh) = { "Kilo Lumen hours", "klmh" };
UNIT(degrees) = { "Degrees", "degrees" };
UNIT(mps) = { "Metres per second", "m/s" };
UNIT(microtesla) = { "Microtesla", "uT" };
UNIT(concentration) = { "Concentration", "per m3" };
UNIT(pascal) = { "Pascal", "Pa" };
UNIT(metre) = { "Metre", "m" };
UNIT(unitless) = { "Unitless" };
/*******************************************************************************
 * Encoders and decoders
 ******************************************************************************/
#define BRANCHLESS_ABS8(x) (((x) + ((x) >> 7)) ^ ((x) >> 7))

#define SCALAR(mul, bin)                                                       \
	((double)(mul) *                                                       \
	 (double)((bin) >= 0 ? (1ULL << BRANCHLESS_ABS8(bin)) :                \
			       (1.0 / (1ULL << BRANCHLESS_ABS8(bin)))))

#define SCALAR_IS_DIV(_scalar) ((_scalar) > -1.0 && (_scalar) < 1.0)

#define SCALAR_REPR_RANGED(_scalar, _flags, _min, _max)                              \
	{                                                                      \
		.flags = ((_flags) | (SCALAR_IS_DIV(_scalar) ? DIVIDE : 0)),   \
		.min = _min,                                                   \
		.max = _max,                                                   \
		.value = (int64_t)((SCALAR_IS_DIV(_scalar) ? (1.0 / (_scalar)) : \
							   (_scalar)) +        \
				 0.5),                                         \
	}

#define SCALAR_REPR(_scalar, _flags) SCALAR_REPR_RANGED(_scalar, _flags, 0, 0)

#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
#define SCALAR_CALLBACKS .encode = scalar_encode, .decode = scalar_decode
#else
#define SCALAR_CALLBACKS .cb = &scalar_cb
#endif

#ifdef CONFIG_BT_MESH_SENSOR_LABELS

#define SCALAR_FORMAT_MIN_MAX(_size, _flags, _unit, _scalar, _min, _max)       \
	{                                                                      \
		SCALAR_CALLBACKS,                                              \
		.unit = &bt_mesh_sensor_unit_##_unit,                          \
		.size = _size,                                                 \
		.user_data = (void *)&(                                        \
			(const struct scalar_repr)SCALAR_REPR_RANGED(          \
				_scalar, ((_flags) | HAS_MIN | HAS_MAX), _min, _max)),         \
	}

#define SCALAR_FORMAT_MAX(_size, _flags, _unit, _scalar, _max)                 \
	{                                                                      \
		SCALAR_CALLBACKS,                                              \
		.unit = &bt_mesh_sensor_unit_##_unit,                          \
		.size = _size,                                                 \
		.user_data = (void *)&(                                        \
			(const struct scalar_repr)SCALAR_REPR_RANGED(          \
				_scalar, ((_flags) | HAS_MAX), 0, _max)),         \
	}

#define SCALAR_FORMAT(_size, _flags, _unit, _scalar)                           \
	{                                                                      \
		SCALAR_CALLBACKS,                                              \
		.unit = &bt_mesh_sensor_unit_##_unit,                          \
		.size = _size,                                                 \
		.user_data = (void *)&((const struct scalar_repr)SCALAR_REPR(  \
			_scalar, _flags)),                                     \
	}
#else

#define SCALAR_FORMAT_MIN_MAX(_size, _flags, _unit, _scalar, _min, _max)       \
	{                                                                      \
		SCALAR_CALLBACKS,                                              \
		.size = _size,                                                 \
		.user_data = (void *)&(                                        \
			(const struct scalar_repr)SCALAR_REPR_RANGED(          \
				_scalar, ((_flags) | HAS_MIN | HAS_MAX), _min, _max)),         \
	}

#define SCALAR_FORMAT_MAX(_size, _flags, _unit, _scalar, _max)                 \
	{                                                                      \
		SCALAR_CALLBACKS,                                              \
		.size = _size,                                                 \
		.user_data = (void *)&(                                        \
			(const struct scalar_repr)SCALAR_REPR_RANGED(          \
				_scalar, ((_flags) | HAS_MAX), 0, _max)),         \
	}

#define SCALAR_FORMAT(_size, _flags, _unit, _scalar)                           \
	{                                                                      \
		SCALAR_CALLBACKS,                                              \
		.size = _size,                                                 \
		.user_data = (void *)&((const struct scalar_repr)SCALAR_REPR(  \
			_scalar, _flags)),                                     \
	}
#endif

#define MUL_SCALAR(_val, _repr) (((repr)->flags & DIVIDE) ?                    \
				 ((_val) / (_repr)->value) :                   \
				 ((_val) * (_repr)->value))

#define DIV_SCALAR(_val, _repr) (((repr)->flags & DIVIDE) ?                    \
				 ((_val) * (_repr)->value) :                   \
				 ((_val) / (_repr)->value))

enum scalar_repr_flags {
	UNSIGNED = 0,
	SIGNED = BIT(1),
	DIVIDE = BIT(2),
	/** The type maximum represents "value is not known" */
	HAS_UNDEFINED = BIT(3),
	/** The maximum value represents the maximum value or higher. */
	HAS_HIGHER_THAN = (BIT(4)),
	/** The highest encoded value which doesn't represent "value is not
	 *  known" represents "value is invalid".
	 */
	HAS_INVALID = (BIT(5)),
	HAS_MAX = BIT(6),
	HAS_MIN = BIT(7),
	/** The type minimum represents "value is not known" */
	HAS_UNDEFINED_MIN = BIT(8),
	/** The type maximum minus 1 represents "value is not known" */
	HAS_UNDEFINED_MAX_MINUS_1 = BIT(9),
	/** The minimum value represents the minimum value or lower. */
	HAS_LOWER_THAN = BIT(10),
};

struct scalar_repr {
	enum scalar_repr_flags flags;
	int32_t min;
	uint32_t max; /**< Highest encoded value */
	int64_t value;
};

static uint32_t scalar_type_max(const struct bt_mesh_sensor_format *format)
{
	const struct scalar_repr *repr = format->user_data;

	return (repr->flags & SIGNED) ?
		BIT64(8 * format->size - 1) - 1 :
		BIT64(8 * format->size) - 1;
}

static int32_t scalar_type_min(const struct bt_mesh_sensor_format *format)
{
	const struct scalar_repr *repr = format->user_data;

	return (repr->flags & SIGNED) ? -BIT64(8 * format->size - 1) : 0;
}

static int64_t scalar_max(const struct bt_mesh_sensor_format *format)
{
	const struct scalar_repr *repr = format->user_data;

	if (repr->flags & HAS_MAX) {
		return repr->max;
	}

	uint32_t max_value = scalar_type_max(format);

	if (repr->flags & HAS_INVALID) {
		max_value -= 2;
	} else if (repr->flags & HAS_UNDEFINED) {
		max_value -= 1;
	}

	return max_value;
}

static int32_t scalar_min(const struct bt_mesh_sensor_format *format)
{
	const struct scalar_repr *repr = format->user_data;

	if (repr->flags & HAS_MIN) {
		return repr->min;
	}

	if (repr->flags & HAS_UNDEFINED_MIN) {
		return scalar_type_min(format) + 1;
	}

	return scalar_type_min(format);
}

static int scalar_decode_raw(const struct bt_mesh_sensor_format *format,
			     const uint8_t *src, int64_t *raw)
{
	const struct scalar_repr *repr = format->user_data;

	switch (format->size) {
	case 1:
		if (repr->flags & SIGNED) {
			*raw = (int8_t) *src;
		} else {
			*raw = *src;
		}
		break;
	case 2:
		if (repr->flags & SIGNED) {
			*raw = (int16_t) sys_get_le16(src);
		} else {
			*raw = sys_get_le16(src);
		}
		break;
	case 3:
		*raw = sys_get_le24(src);

		if ((repr->flags & SIGNED) && (*raw & BIT(24))) {
			*raw |= (BIT_MASK(8) << 24);
		}
		break;
	case 4:
		*raw = sys_get_le32(src);
		break;
	default:
		return -ERANGE;
	}

	return 0;
}

static int scalar_encode_raw(const struct bt_mesh_sensor_format *format,
			     int64_t raw, uint8_t *dst)
{
	switch (format->size) {
	case 1:
		*dst = raw;
		break;
	case 2:
		sys_put_le16(raw, dst);
		break;
	case 3:
		sys_put_le24(raw, dst);
		break;
	case 4:
		sys_put_le32(raw, dst);
		break;
	default:
		return -EIO;
	}

	return 0;
}

#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
static int scalar_encode(const struct bt_mesh_sensor_format *format,
			 const struct sensor_value *val,
			 struct net_buf_simple *buf)
{
	const struct scalar_repr *repr = format->user_data;

	if (net_buf_simple_tailroom(buf) < format->size) {
		return -ENOMEM;
	}

	int64_t raw = DIV_SCALAR(val->val1, repr) +
		    DIV_SCALAR(val->val2, repr) / 1000000LL;

	int64_t max_value = scalar_max(format);
	int32_t min_value = scalar_min(format);

	if (raw > max_value || raw < min_value) {
		uint32_t type_max = BIT64(8 * format->size) - 1;
		if (repr->flags & SIGNED) {
			type_max = BIT64(8 * format->size - 1) - 1;
		}

		if ((repr->flags & HAS_HIGHER_THAN) &&
			!((repr->flags & HAS_UNDEFINED) && (val->val1 == type_max))) {
			raw = max_value;
		} else if ((repr->flags & HAS_INVALID) && val->val1 == type_max - 1) {
			raw = type_max - 1;
		} else if ((repr->flags & HAS_UNDEFINED) && val->val1 == type_max) {
			raw = type_max;
		} else if (repr->flags & HAS_UNDEFINED_MIN) {
			raw = type_max + 1;
		} else {
			return -ERANGE;
		}
	}

	return scalar_encode_raw(format, raw, net_buf_simple_add(buf, format->size));
}

static int scalar_decode(const struct bt_mesh_sensor_format *format,
			 struct net_buf_simple *buf, struct sensor_value *val)
{
	const struct scalar_repr *repr = format->user_data;

	if (buf->len < format->size) {
		return -ENOMEM;
	}

	int64_t raw;
	int err;

	err = scalar_decode_raw(format,
				net_buf_simple_pull_mem(buf, format->size),
				&raw);
	if (err) {
		return err;
	}

	int64_t max_value = scalar_max(format);
	int64_t min_value = scalar_min(format);

	if (raw < min_value || raw > max_value) {
		if (!(repr->flags & (HAS_UNDEFINED | HAS_UNDEFINED_MIN |
				     HAS_HIGHER_THAN | HAS_INVALID))) {
			return -ERANGE;
		}

		uint32_t type_max = (repr->flags & SIGNED) ?
			BIT64(8 * format->size - 1) - 1 :
			BIT64(8 * format->size) - 1;

		if (repr->flags & HAS_UNDEFINED_MIN) {
			type_max += 1;
		} else if ((repr->flags & HAS_INVALID) && raw == type_max - 1) {
			type_max -= 1;
		} else if (raw != type_max) {
			return -ERANGE;
		}

		val->val1 = type_max;
		val->val2 = 0;
		return 0;
	}

	int64_t million = MUL_SCALAR(raw * 1000000LL, repr);

	val->val1 = million / 1000000LL;
	val->val2 = million % 1000000LL;

	return 0;
}

static int boolean_encode(const struct bt_mesh_sensor_format *format,
			  const struct sensor_value *val,
			  struct net_buf_simple *buf)
{
	if (net_buf_simple_tailroom(buf) < 1) {
		return -ENOMEM;
	}

	net_buf_simple_add_u8(buf, !!val->val1);
	return 0;
}

static int boolean_decode(const struct bt_mesh_sensor_format *format,
			  struct net_buf_simple *buf, struct sensor_value *val)
{
	if (buf->len < 1) {
		return -ENOMEM;
	}

	uint8_t b = net_buf_simple_pull_u8(buf);

	if (b > 1) {
		return -EINVAL;
	}

	val->val1 = b;
	val->val2 = 0;
	return 0;
}

static int exp_1_1_encode(const struct bt_mesh_sensor_format *format,
			  const struct sensor_value *val,
			  struct net_buf_simple *buf)
{
	if (net_buf_simple_tailroom(buf) < 1) {
		return -ENOMEM;
	}

	if (val->val1 == 0xffffffff && val->val2 == 0) {
		net_buf_simple_add_u8(buf, 0xff); // Unknown time
		return 0;
	}
	if (val->val1 == 0xfffffffe && val->val2 == 0) {
		net_buf_simple_add_u8(buf, 0xfe); // Total lifetime
		return 0;
	}

	net_buf_simple_add_u8(buf,
			      sensor_powtime_encode((val->val1 * 1000ULL) +
						    (val->val2 / 1000ULL)));
	return 0;
}

static int exp_1_1_decode(const struct bt_mesh_sensor_format *format,
			  struct net_buf_simple *buf, struct sensor_value *val)
{
	if (buf->len < 1) {
		return -ENOMEM;
	}

	uint8_t raw = net_buf_simple_pull_u8(buf);

	val->val2 = 0;
	if (raw == 0xff) {
		val->val1 = 0xffffffff;
	} else if (raw == 0xfe) {
		val->val1 = 0xfffffffe;
	} else {
		uint64_t time_us = sensor_powtime_decode_us(raw);

		val->val1 = time_us / USEC_PER_SEC;
		val->val2 = time_us % USEC_PER_SEC;
	}
	return 0;
}

static int float32_encode(const struct bt_mesh_sensor_format *format,
			  const struct sensor_value *val,
			  struct net_buf_simple *buf)
{
	if (net_buf_simple_tailroom(buf) < sizeof(float)) {
		return -ENOMEM;
	}

	/* IEEE-754 32-bit floating point */
	float fvalue = (float)val->val1 + (float)val->val2 / 1000000L;

	net_buf_simple_add_mem(buf, &fvalue, sizeof(float));

	return 0;
}

static int float32_decode(const struct bt_mesh_sensor_format *format,
			  struct net_buf_simple *buf, struct sensor_value *val)
{
	if (buf->len < sizeof(float)) {
		return -ENOMEM;
	}

	float fvalue;

	memcpy(&fvalue, net_buf_simple_pull_mem(buf, sizeof(float)),
	       sizeof(float));

	val->val1 = (int32_t)fvalue;
	val->val2 = ((int64_t)(fvalue * 1000000.0f)) % 1000000L;
	return 0;
}
#else /* !defined(CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE) */
static bool percentage_delta_check(const struct bt_mesh_sensor_value *delta,
				   float diff, float prev)
{
	__ASSERT_NO_MSG(delta->format == &bt_mesh_sensor_format_percentage_delta_trigger);
	if (diff == 0.0f) {
		return false;
	}

	if (prev == 0.0f) {
		/* All changes from zero are inf% and should trigger. */
		return true;
	}

	int64_t delta_raw;

	if (scalar_decode_raw(delta->format, delta->raw, &delta_raw) != 0) {
		return false;
	}

	float percentage_float = delta_raw / 10000.0f;

	return (diff / prev) > (percentage_float);
}

static bool scalar_unknown_raw(const struct bt_mesh_sensor_format *format,
			       int64_t *val)
{
	const struct scalar_repr *repr = format->user_data;

	if (repr->flags & HAS_UNDEFINED) {
		*val = scalar_type_max(format);
	} else if (repr->flags & HAS_UNDEFINED_MIN) {
		*val = scalar_type_min(format);
	} else if (repr->flags & HAS_UNDEFINED_MAX_MINUS_1) {
		*val = scalar_type_max(format) - 1;
	} else {
		return false;
	}

	return true;
}

static bool scalar_invalid_raw(const struct bt_mesh_sensor_format *format,
			       int64_t *val)
{
	const struct scalar_repr *repr = format->user_data;

	if (repr->flags & HAS_INVALID) {
		*val = scalar_type_max(format) - 1;
		if (repr->flags & HAS_UNDEFINED_MAX_MINUS_1) {
			*val += 1;
		}
		return true;
	}

	return false;
}

static enum bt_mesh_sensor_value_status
scalar_to_raw(const struct bt_mesh_sensor_value *sensor_val, int64_t *val)
{
	if (scalar_decode_raw(sensor_val->format, sensor_val->raw, val) != 0) {
		return BT_MESH_SENSOR_VALUE_CONVERSION_ERROR;
	}

	const struct scalar_repr *repr = sensor_val->format->user_data;
	int64_t max = scalar_max(sensor_val->format);
	int32_t min = scalar_min(sensor_val->format);

	if (*val == max && (repr->flags & HAS_HIGHER_THAN)) {
		return BT_MESH_SENSOR_VALUE_MAX_OR_GREATER;
	}

	if (*val == min && (repr->flags & HAS_LOWER_THAN)) {
		return BT_MESH_SENSOR_VALUE_MIN_OR_LESS;
	}

	if (IN_RANGE(*val, min, max)) {
		return BT_MESH_SENSOR_VALUE_NUMBER;
	}

	int64_t raw_nonnumeric;

	if (scalar_unknown_raw(sensor_val->format, &raw_nonnumeric) &&
	    *val == raw_nonnumeric) {
		return BT_MESH_SENSOR_VALUE_UNKNOWN;
	}

	if (scalar_invalid_raw(sensor_val->format, &raw_nonnumeric) &&
	    *val == raw_nonnumeric) {
		return BT_MESH_SENSOR_VALUE_INVALID;
	}

	return BT_MESH_SENSOR_VALUE_CONVERSION_ERROR;
}

static int scalar_from_raw(const struct bt_mesh_sensor_format *format,
			   int64_t val,
			   struct bt_mesh_sensor_value *sensor_val)
{
	int64_t max = scalar_max(format);
	int32_t min = scalar_min(format);
	int64_t clamped = CLAMP(val, min, max);

	if (scalar_encode_raw(format, clamped, sensor_val->raw) != 0) {
		return -EIO;
	}

	sensor_val->format = format;

	return clamped != val ? -ERANGE : 0;
}

static int scalar_compare(const struct bt_mesh_sensor_value *op1,
			  const struct bt_mesh_sensor_value *op2)
{
	enum bt_mesh_sensor_value_status status;
	int64_t op1_raw, op2_raw;

	status = scalar_to_raw(op1, &op1_raw);
	if (!bt_mesh_sensor_value_status_is_numeric(status)) {
		/* Not comparable, return -1. */
		return -1;
	}
	status = scalar_to_raw(op2, &op2_raw);
	if (!bt_mesh_sensor_value_status_is_numeric(status)) {
		/* Not comparable, return -1. */
		return -1;
	}

	if (op1_raw > op2_raw) {
		return 1;
	} else if (op1_raw == op2_raw) {
		return 0;
	} else {
		return -1;
	}
}

static bool scalar_delta_check(const struct bt_mesh_sensor_value *current,
			       const struct bt_mesh_sensor_value *previous,
			       const struct bt_mesh_sensor_deltas *deltas)
{
	int64_t current_raw, previous_raw, delta_raw;
	const struct bt_mesh_sensor_value *delta;
	enum bt_mesh_sensor_value_status curr_status, prev_status;

	curr_status = scalar_to_raw(current, &current_raw);
	prev_status = scalar_to_raw(previous, &previous_raw);

	if (!bt_mesh_sensor_value_status_is_numeric(curr_status) ||
	    !bt_mesh_sensor_value_status_is_numeric(prev_status)) {
		/* Always return true for changes involving non-numeric values.
		 */
		return curr_status != prev_status;
	}

	int64_t diff = current_raw - previous_raw;

	if (diff < 0) {
		diff = -diff;
		delta = &deltas->down;
	} else {
		delta = &deltas->up;
	}

	if (delta->format == &bt_mesh_sensor_format_percentage_delta_trigger) {
		return percentage_delta_check(delta, diff, previous_raw);
	}

	enum bt_mesh_sensor_value_status status = scalar_to_raw(delta, &delta_raw);

	if (!bt_mesh_sensor_value_status_is_numeric(status)) {
		return false;
	}

	return diff > delta_raw;
}

static enum bt_mesh_sensor_value_status
scalar_to_float(const struct bt_mesh_sensor_value *sensor_val, float *val)
{
	int64_t raw;
	enum bt_mesh_sensor_value_status status = scalar_to_raw(sensor_val, &raw);

	if (val && bt_mesh_sensor_value_status_is_numeric(status)) {
		const struct scalar_repr *repr = sensor_val->format->user_data;

		*val = MUL_SCALAR((float)raw, repr);
	}

	return status;
}

static int scalar_from_float(const struct bt_mesh_sensor_format *format,
			     float val, struct bt_mesh_sensor_value *sensor_val)
{
	const struct scalar_repr *repr = format->user_data;
	float raw = DIV_SCALAR(val, repr);

	return scalar_from_raw(format, CLAMP(raw, INT64_MIN, INT64_MAX), sensor_val);
}

static enum bt_mesh_sensor_value_status
scalar_to_micro(const struct bt_mesh_sensor_value *sensor_val, int64_t *val)
{
	int64_t raw;
	enum bt_mesh_sensor_value_status status = scalar_to_raw(sensor_val, &raw);

	if (val && bt_mesh_sensor_value_status_is_numeric(status)) {
		const struct scalar_repr *repr = sensor_val->format->user_data;

		*val = MUL_SCALAR(raw * 1000000LL, repr);
	}

	return status;
}

static int scalar_from_micro(const struct bt_mesh_sensor_format *format,
			     int64_t val,
			     struct bt_mesh_sensor_value *sensor_val)
{
	const struct scalar_repr *repr = format->user_data;
	int64_t raw = DIV_SCALAR(val / 1000000, repr) +
		      DIV_SCALAR(val % 1000000, repr) / 1000000LL;

	return scalar_from_raw(format, raw, sensor_val);
}

static int scalar_from_special_status(
	const struct bt_mesh_sensor_format *format,
	enum bt_mesh_sensor_value_status status,
	struct bt_mesh_sensor_value *sensor_val)
{
	const struct scalar_repr *repr = format->user_data;
	int64_t raw;

	switch (status) {
	case BT_MESH_SENSOR_VALUE_UNKNOWN:
		if (!scalar_unknown_raw(format, &raw)) {
			return -ERANGE;
		}
		break;
	case BT_MESH_SENSOR_VALUE_INVALID:
		if (!scalar_invalid_raw(format, &raw)) {
			return -ERANGE;
		}
		break;
	case BT_MESH_SENSOR_VALUE_MAX_OR_GREATER:
		if (!(repr->flags & HAS_HIGHER_THAN)) {
			return -ERANGE;
		}
		raw = scalar_max(format);
		break;
	case BT_MESH_SENSOR_VALUE_MIN_OR_LESS:
		if (!(repr->flags & HAS_LOWER_THAN)) {
			return -ERANGE;
		}
		raw = scalar_min(format);
		break;
	default:
		return -ERANGE;
	}

	sensor_val->format = format;
	return scalar_encode_raw(format, raw, sensor_val->raw);
}

static bool scalar_value_in_column(
	const struct bt_mesh_sensor_value *sensor_val,
	const struct bt_mesh_sensor_column *col)
{
	enum bt_mesh_sensor_value_status status;
	int64_t val_raw, start_raw, width_raw;

	status = scalar_to_raw(sensor_val, &val_raw);
	if (!bt_mesh_sensor_value_status_is_numeric(status)) {
		return false;
	}

	status = scalar_to_raw(&col->start, &start_raw);
	if (!bt_mesh_sensor_value_status_is_numeric(status)) {
		return false;
	}

	status = scalar_to_raw(&col->width, &width_raw);
	if (!bt_mesh_sensor_value_status_is_numeric(status)) {
		return false;
	}

	return (val_raw >= start_raw) && (val_raw <= (start_raw + width_raw));
}

static struct bt_mesh_sensor_format_cb scalar_cb = {
	.compare = scalar_compare,
	.delta_check = scalar_delta_check,
	.to_float = scalar_to_float,
	.from_float = scalar_from_float,
	.to_micro = scalar_to_micro,
	.from_micro = scalar_from_micro,
	.from_special_status = scalar_from_special_status,
	.value_in_column = scalar_value_in_column,
};

static int boolean_compare(const struct bt_mesh_sensor_value *op1,
			   const struct bt_mesh_sensor_value *op2)
{
	int diff = *(op1->raw) - *(op2->raw);

	if (diff > 0) {
		return 1;
	} else if (diff == 0) {
		return 0;
	} else {
		return -1;
	}
}

static bool boolean_delta_check(const struct bt_mesh_sensor_value *current,
				const struct bt_mesh_sensor_value *previous,
				const struct bt_mesh_sensor_deltas *deltas)
{
	uint8_t current_raw =  *(current->raw);
	uint8_t previous_raw = *(previous->raw);
	uint8_t diff;
	const struct bt_mesh_sensor_value *delta;

	if (current_raw < previous_raw) {
		diff = previous_raw - current_raw;
		delta = &deltas->down;
	} else {
		diff = current_raw - previous_raw;
		delta = &deltas->up;
	}

	if (delta->format == &bt_mesh_sensor_format_percentage_delta_trigger) {
		return percentage_delta_check(delta, diff, previous_raw);
	}

	return diff > *(delta->raw);
}

static enum bt_mesh_sensor_value_status
boolean_to_float(const struct bt_mesh_sensor_value *sensor_val, float *val)
{
	if (val) {
		*val = !!*(sensor_val->raw);
	}
	return BT_MESH_SENSOR_VALUE_NUMBER;
}

static int boolean_from_float(const struct bt_mesh_sensor_format *format,
			      float val, struct bt_mesh_sensor_value *sensor_val)
{
	sensor_val->format = format;
	*(sensor_val->raw) = !!val;

	if (val > 1.0f || val < 0.0f) {
		return -ERANGE;
	}

	return 0;
}

static enum bt_mesh_sensor_value_status
boolean_to_micro(const struct bt_mesh_sensor_value *sensor_val,
		 int64_t *val)
{
	if (val) {
		*val = (!!*(sensor_val->raw)) ? 1000000 : 0;
	}
	return BT_MESH_SENSOR_VALUE_NUMBER;
}

static int boolean_from_micro(const struct bt_mesh_sensor_format *format,
			      int64_t val,
			      struct bt_mesh_sensor_value *sensor_val)
{
	sensor_val->format = format;
	*(sensor_val->raw) = !!val;

	if (val != 0 && val != 1000000) {
		return -ERANGE;
	}

	return 0;
}

static int boolean_to_string(const struct bt_mesh_sensor_value *sensor_val,
			     char *str, size_t len)
{
	if (*(sensor_val->raw) == 0) {
		return snprintk(str, len, "%s", "False");
	}
	return snprintk(str, len, "%s", "True");
}

static bool boolean_value_in_column(
	const struct bt_mesh_sensor_value *sensor_val,
	const struct bt_mesh_sensor_column *col)
{
	uint8_t val_raw = *(sensor_val->raw);
	uint8_t start_raw = *(col->start.raw);
	uint8_t end_raw = start_raw + *(col->width.raw);

	return val_raw >= start_raw && val_raw <= end_raw;
}

static struct bt_mesh_sensor_format_cb boolean_cb = {
	.compare = boolean_compare,
	.delta_check = boolean_delta_check,
	.to_float = boolean_to_float,
	.from_float = boolean_from_float,
	.to_micro = boolean_to_micro,
	.from_micro = boolean_from_micro,
	.to_string = boolean_to_string,
	.value_in_column = boolean_value_in_column,
};

/* This is the negative of the ceiling of the function d_1.1(-(i+1)), where d_1.1
 * is the Gaussian logarithm difference function d_b(z) = log_b(|1 - b^z|).
 * -ceil(d_1.1(-(i+1))) is 0 for i+1 > 25.
 */
static const uint8_t gaussian_log_d11[] = { 25, 18, 14, 12, 10, 8, 7, 6, 5, 5, 4, 4,
					    3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1 };

/** Computes ceil(log_1.1(|1.1^(a-64) - 1.1^(b-64)|)) + 64 using Gaussian logarithms.
 *  Clamps the result to [1, 255] which is the range representable by the exp_1_1
 *  format.
 *
 *  If a == b, this returns 0, which is the encoded value for 0 in the exp_1_1 format.
 */
static uint8_t exp_1_1_ceil_abs_diff(uint8_t a, uint8_t b)
{
	if (a == b) {
		return 0;
	}

	uint8_t max = MAX(a, b);
	uint8_t min = MIN(a, b);

	if (min == 0) {
		/* Handle the fact that 0 encodes 0, not 1.1^-64. */
		return max;
	}

	uint8_t diff = max - min;
	uint8_t d = diff > 25 ? 0 : gaussian_log_d11[diff - 1];

	return MAX(max - d, 1);
}

static float exp_1_1_decode_float(uint8_t raw)
{
	if (raw == 0x00) {
		return 0.0f;
	}

	return powf(1.1f, raw - 64);
}

static int exp_1_1_compare(const struct bt_mesh_sensor_value *op1,
			   const struct bt_mesh_sensor_value *op2)
{
	uint8_t op1_raw = *(op1->raw);
	uint8_t op2_raw = *(op2->raw);

	if (op1_raw >= 0xfe || op2_raw >= 0xfe) {
		/* Not comparable. */
		return -1;
	}

	if (op1_raw > op2_raw) {
		return 1;
	} else if (op1_raw == op2_raw) {
		return 0;
	} else {
		return -1;
	}
}

static bool exp_1_1_delta_check(const struct bt_mesh_sensor_value *current,
				const struct bt_mesh_sensor_value *previous,
				const struct bt_mesh_sensor_deltas *deltas)
{
	const struct bt_mesh_sensor_value *delta;

	uint8_t curr_raw = *(current->raw);
	uint8_t prev_raw = *(previous->raw);

	if (curr_raw == prev_raw) {
		return false;
	}

	if (curr_raw >= 0xfe || prev_raw >= 0xfe) {
		/* Always trigger if change involves non-numeric value. */
		return curr_raw != prev_raw;
	}

	if (curr_raw < prev_raw) {
		delta = &deltas->down;
	} else {
		delta = &deltas->up;
	}

	if (delta->format == &bt_mesh_sensor_format_percentage_delta_trigger) {
		float prev_float = exp_1_1_decode_float(prev_raw);
		float diff = exp_1_1_decode_float(curr_raw) - prev_float;

		if (diff < 0) {
			diff = -diff;
		}

		return percentage_delta_check(delta, diff, prev_float);
	}

	return exp_1_1_ceil_abs_diff(prev_raw, curr_raw) > *(delta->raw);
}

static enum bt_mesh_sensor_value_status
exp_1_1_to_float(const struct bt_mesh_sensor_value *sensor_val, float *val)
{
	uint8_t raw = *(sensor_val->raw);

	if (raw == 0xff) {
		return BT_MESH_SENSOR_VALUE_UNKNOWN;
	}

	if (raw == 0xfe) {
		return BT_MESH_SENSOR_VALUE_TOTAL_DEVICE_LIFE;
	}

	if (val) {
		*val = exp_1_1_decode_float(raw);
	}
	return BT_MESH_SENSOR_VALUE_NUMBER;
}

static int exp_1_1_from_float(const struct bt_mesh_sensor_format *format,
			      float val, struct bt_mesh_sensor_value *sensor_val)
{
	sensor_val->format = format;
	if (val == 0.0f) {
		*(sensor_val->raw) = 0x00;
		return 0;
	}

	float result = roundf(logf(val)/LOG_1_1);

	*(sensor_val->raw) = CLAMP(result, 0x01, 0xfd);

	if (result < 1 || result > 253) {
		return -ERANGE;
	}

	return 0;
}

static int exp_1_1_from_special_status(
	const struct bt_mesh_sensor_format *format,
	enum bt_mesh_sensor_value_status status,
	struct bt_mesh_sensor_value *sensor_val)
{
	switch (status) {
	case BT_MESH_SENSOR_VALUE_UNKNOWN:
		*(sensor_val->raw) = 0xff;
		break;
	case BT_MESH_SENSOR_VALUE_TOTAL_DEVICE_LIFE:
		*(sensor_val->raw) = 0xfe;
		break;
	default:
		return -ERANGE;
	}

	return 0;
}

static struct bt_mesh_sensor_format_cb exp_1_1_cb = {
	.compare = exp_1_1_compare,
	.delta_check = exp_1_1_delta_check,
	.to_float = exp_1_1_to_float,
	.from_float = exp_1_1_from_float,
	.from_special_status = exp_1_1_from_special_status,
};

static enum bt_mesh_sensor_value_status
float32_to_float(const struct bt_mesh_sensor_value *sensor_val, float *val)
{
	if (val) {
		uint32_t raw = sys_get_le32(sensor_val->raw);

		*val = *(float *)(&raw);
	}
	return BT_MESH_SENSOR_VALUE_NUMBER;
}

static int float32_from_float(const struct bt_mesh_sensor_format *format,
			      float val, struct bt_mesh_sensor_value *sensor_val)
{
	uint32_t raw = *(uint32_t *)(&val);

	sensor_val->format = format;
	sys_put_le32(raw, sensor_val->raw);

	return 0;
}

static int float32_compare(const struct bt_mesh_sensor_value *op1,
			  const struct bt_mesh_sensor_value *op2)
{
	float op1_float, op2_float;

	(void)float32_to_float(op1, &op1_float);
	(void)float32_to_float(op2, &op2_float);

	float diff = op1_float - op2_float;

	if (diff > 0.0f) {
		return 1;
	} else if (diff == 0.0f) {
		return 0;
	} else {
		return -1;
	}
}

static bool float32_delta_check(const struct bt_mesh_sensor_value *current,
				const struct bt_mesh_sensor_value *previous,
				const struct bt_mesh_sensor_deltas *deltas)
{
	const struct bt_mesh_sensor_value *delta;

	float prev_float, curr_float;

	(void)float32_to_float(previous, &prev_float);
	(void)float32_to_float(current, &curr_float);

	float diff = curr_float - prev_float;

	if (diff < 0.0f) {
		diff = -diff;
		delta = &deltas->down;
	} else {
		delta = &deltas->up;
	}

	if (delta->format == &bt_mesh_sensor_format_percentage_delta_trigger) {
		return percentage_delta_check(delta, diff, prev_float);
	}

	float delta_float;

	(void)float32_to_float(delta, &delta_float);

	return diff > delta_float;
}

static struct bt_mesh_sensor_format_cb float32_cb = {
	.compare = float32_compare,
	.delta_check = float32_delta_check,
	.to_float = float32_to_float,
	.from_float = float32_from_float,
};
#endif /* defined(CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE) */

/*******************************************************************************
 * Sensor Formats
 *
 * Formats define how single channels are encoded and decoded, and are generally
 * represented as a scalar format with multipliers.
 *
 * See MshMDLv1.1: 2.3 for details.
 ******************************************************************************/


/*******************************************************************************
 * Percentage formats
 ******************************************************************************/
FORMAT(percentage_8)  = SCALAR_FORMAT_MAX(1,
					  (UNSIGNED | HAS_UNDEFINED),
					  percent,
					  SCALAR(1, -1),
					  200);
FORMAT(percentage_16) = SCALAR_FORMAT_MAX(2,
					  (UNSIGNED | HAS_UNDEFINED),
					  percent,
					  SCALAR(1e-2, 0),
					  10000);
FORMAT(percentage_delta_trigger) = SCALAR_FORMAT(2,
					  UNSIGNED,
					  percent,
					  SCALAR(1e-2, 0));

/*******************************************************************************
 * Environmental formats
 ******************************************************************************/
FORMAT(temp_8)		      = SCALAR_FORMAT(1,
					      (SIGNED | HAS_UNDEFINED),
					      celsius,
					      SCALAR(1, -1));
FORMAT(temp)		      = SCALAR_FORMAT(2,
					      (SIGNED | HAS_UNDEFINED_MIN),
					      celsius,
					      SCALAR(1e-2, 0));
FORMAT(co2_concentration)     = SCALAR_FORMAT_MAX(2,
					      (UNSIGNED |
					      HAS_HIGHER_THAN |
					      HAS_UNDEFINED),
					      ppm,
					      SCALAR(1, 0),
					      65534);
FORMAT(noise)			     = SCALAR_FORMAT_MAX(1,
					       (UNSIGNED |
					       HAS_HIGHER_THAN |
					       HAS_UNDEFINED),
					       db,
					       SCALAR(1, 0),
					       254);
FORMAT(voc_concentration)     = SCALAR_FORMAT_MAX(2,
					       (UNSIGNED |
					       HAS_HIGHER_THAN |
					       HAS_UNDEFINED),
					       ppb,
					       SCALAR(1, 0),
					       65534);
FORMAT(wind_speed)            = SCALAR_FORMAT(2,
					      UNSIGNED,
					      mps,
					      SCALAR(1e-2, 0));
FORMAT(temp_8_wide)           = SCALAR_FORMAT(1,
					      SIGNED,
					      celsius,
					      SCALAR(1, 0));
FORMAT(gust_factor)           = SCALAR_FORMAT(1,
					      UNSIGNED,
					      unitless,
					      SCALAR(1e-1, 0));
FORMAT(magnetic_flux_density) = SCALAR_FORMAT(2,
					      SIGNED,
					      microtesla,
					      SCALAR(1e-1, 0));
FORMAT(pollen_concentration)  = SCALAR_FORMAT(3,
					      UNSIGNED,
					      concentration,
					      SCALAR(1, 0));
FORMAT(pressure)              = SCALAR_FORMAT(4,
					      UNSIGNED,
					      pascal,
					      SCALAR(1e-1, 0));
FORMAT(rainfall)              = SCALAR_FORMAT(2,
					      UNSIGNED,
					      metre,
					      SCALAR(1e-3, 0));
FORMAT(uv_index)              = SCALAR_FORMAT(1,
					      UNSIGNED,
					      unitless,
					      SCALAR(1, 0));
/*******************************************************************************
 * Time formats
 ******************************************************************************/
FORMAT(time_decihour_8)	    = SCALAR_FORMAT_MAX(1,
						(UNSIGNED | HAS_UNDEFINED),
						hours,
						SCALAR(1e-1, 0),
						240);
FORMAT(time_hour_24)	    = SCALAR_FORMAT(3,
					    (UNSIGNED | HAS_UNDEFINED),
					    hours,
					    SCALAR(1, 0));
FORMAT(time_second_16)	    = SCALAR_FORMAT(2,
					    (UNSIGNED | HAS_UNDEFINED),
					    seconds,
					    SCALAR(1, 0));
FORMAT(time_millisecond_24) = SCALAR_FORMAT(3,
					    (UNSIGNED | HAS_UNDEFINED),
					    seconds,
					    SCALAR(1e-3, 0));
FORMAT(time_exp_8)	    = {
	 .size = 1,
#ifdef CONFIG_BT_MESH_SENSOR_LABELS
	.unit = &bt_mesh_sensor_unit_seconds,
#endif
#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	.encode = exp_1_1_encode,
	.decode = exp_1_1_decode,
#else
	.cb = &exp_1_1_cb,
#endif
};

/*******************************************************************************
 * Electrical formats
 ******************************************************************************/
FORMAT(electric_current) = SCALAR_FORMAT_MAX(2,
					 (UNSIGNED | HAS_UNDEFINED),
					 ampere,
					 SCALAR(1e-2, 0),
					 65534);
FORMAT(voltage)		 = SCALAR_FORMAT_MAX(2,
					 (UNSIGNED |
					 HAS_UNDEFINED |
					 HAS_HIGHER_THAN |
					 HAS_LOWER_THAN),
					 volt,
					 SCALAR(1, -6),
					 65408);
FORMAT(energy32)	 = SCALAR_FORMAT(4,
					 UNSIGNED | HAS_INVALID | HAS_UNDEFINED,
					 kwh,
					 SCALAR(1e-3, 0));
FORMAT(apparent_energy32)	 = SCALAR_FORMAT(4,
					 UNSIGNED | HAS_INVALID | HAS_UNDEFINED,
					 kvah,
					 SCALAR(1e-3, 0));
FORMAT(apparent_power)		 = SCALAR_FORMAT(3,
					 (UNSIGNED | HAS_INVALID | HAS_UNDEFINED),
					 va,
					 SCALAR(1e-1, 0));
FORMAT(power)		 = SCALAR_FORMAT(3,
					 (UNSIGNED | HAS_UNDEFINED),
					 watt,
					 SCALAR(1e-1, 0));
FORMAT(energy)           = SCALAR_FORMAT(3,
					 (UNSIGNED | HAS_UNDEFINED),
					 kwh,
					 SCALAR(1, 0));

/*******************************************************************************
 * Lighting formats
 ******************************************************************************/
#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
FORMAT(chromatic_distance)      = SCALAR_FORMAT_MIN_MAX(2,
							(SIGNED |
							HAS_INVALID |
							HAS_UNDEFINED),
							unitless, SCALAR(1e-5, 0),
							-5000, 5000);
#else
FORMAT(chromatic_distance)      = SCALAR_FORMAT_MIN_MAX(2,
							(SIGNED |
							HAS_INVALID |
							HAS_UNDEFINED_MAX_MINUS_1),
							unitless, SCALAR(1e-5, 0),
							-5000, 5000);
#endif
FORMAT(chromaticity_coordinate) = SCALAR_FORMAT(2,
						UNSIGNED,
						unitless,
						SCALAR(1, -16));
FORMAT(correlated_color_temp)	= SCALAR_FORMAT_MIN_MAX(2,
							(UNSIGNED | HAS_UNDEFINED),
							kelvin,
							SCALAR(1, 0),
							800, 65534);
FORMAT(illuminance)		= SCALAR_FORMAT(3,
						(UNSIGNED | HAS_UNDEFINED),
						lux,
						SCALAR(1e-2, 0));
FORMAT(luminous_efficacy)	= SCALAR_FORMAT_MAX(2,
						    (UNSIGNED | HAS_UNDEFINED),
						    lumen_per_watt,
						    SCALAR(1e-1, 0),
						    18000);
FORMAT(luminous_energy)		= SCALAR_FORMAT(3,
						(UNSIGNED | HAS_UNDEFINED),
						klmh,
						SCALAR(1, 0));
FORMAT(luminous_exposure)	= SCALAR_FORMAT(3,
						(UNSIGNED | HAS_UNDEFINED),
						klxh,
						SCALAR(1, 0));
FORMAT(luminous_flux)		= SCALAR_FORMAT(2,
						(UNSIGNED | HAS_UNDEFINED),
						lumen,
						SCALAR(1, 0));
FORMAT(perceived_lightness)	= SCALAR_FORMAT(2,
						UNSIGNED,
						unitless,
						SCALAR(1, 0));

/*******************************************************************************
 * Miscellaneous formats
 ******************************************************************************/
FORMAT(direction_16)     = SCALAR_FORMAT_MAX(2,
					     UNSIGNED,
					     degrees,
					     SCALAR(1e-2, 0),
					     35999);
FORMAT(count_16)	 = SCALAR_FORMAT(2,
					 (UNSIGNED | HAS_UNDEFINED),
					 unitless,
					 SCALAR(1, 0));
FORMAT(gen_lvl)		 = SCALAR_FORMAT(2,
					 UNSIGNED,
					 unitless,
					 SCALAR(1, 0));
FORMAT(cos_of_the_angle) = SCALAR_FORMAT_MIN_MAX(1,
						 (SIGNED | HAS_UNDEFINED),
						 unitless,
						 SCALAR(1, 0),
						 -100, 100);
FORMAT(boolean) = {
#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	.encode = boolean_encode,
	.decode = boolean_decode,
#else
	.cb = &boolean_cb,
#endif
	.size	= 1,
#ifdef CONFIG_BT_MESH_SENSOR_LABELS
	.unit = &bt_mesh_sensor_unit_unitless,
#endif
};
FORMAT(coefficient) = {
#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	.encode = float32_encode,
	.decode = float32_decode,
#else
	.cb = &float32_cb,
#endif
	.size	= 4,
#ifdef CONFIG_BT_MESH_SENSOR_LABELS
	.unit = &bt_mesh_sensor_unit_unitless,
#endif
};

/*******************************************************************************
 * Sensor types
 *
 * Sensor types are stored in a common flash section
 * ".bt_mesh_sensor_type.static.*". This forces them to be sequential, and lets
 * us do lookup of IDs without forcing all sensor types into existence. Only
 * sensor types that are referenced by the application will appear in the
 * section, the rest will be pruned by the linker.
 ******************************************************************************/
static const struct bt_mesh_sensor_channel electric_current_stats[] = {
	CHANNEL("Avg", electric_current),
	CHANNEL("Standard deviation", electric_current),
	CHANNEL("Min", electric_current),
	CHANNEL("Max", electric_current),
	CHANNEL("Sensing duration", time_exp_8),
};

static const struct bt_mesh_sensor_channel voltage_stats[] = {
	CHANNEL("Avg", voltage),
	CHANNEL("Standard deviation", voltage),
	CHANNEL("Min", voltage),
	CHANNEL("Max", voltage),
	CHANNEL("Sensing duration", time_exp_8),
};

/*******************************************************************************
 * Occupancy
 ******************************************************************************/
SENSOR_TYPE(motion_sensed) = {
	.id = BT_MESH_PROP_ID_MOTION_SENSED,
	CHANNELS(CHANNEL("Motion sensed", percentage_8)),
};
SENSOR_TYPE(motion_threshold) = {
	.id = BT_MESH_PROP_ID_MOTION_THRESHOLD,
	CHANNELS(CHANNEL("Motion threshold", percentage_8)),
};
SENSOR_TYPE(people_count) = {
	.id = BT_MESH_PROP_ID_PEOPLE_COUNT,
	CHANNELS(CHANNEL("People count", count_16)),
};
SENSOR_TYPE(presence_detected) = {
	.id = BT_MESH_PROP_ID_PRESENCE_DETECTED,
	CHANNELS(CHANNEL("Presence detected", boolean)),
};
SENSOR_TYPE(time_since_motion_sensed) = {
	.id = BT_MESH_PROP_ID_TIME_SINCE_MOTION_SENSED,
	CHANNELS(CHANNEL("Time since motion detected", time_second_16)),
};
SENSOR_TYPE(time_since_presence_detected) = {
	.id = BT_MESH_PROP_ID_TIME_SINCE_PRESENCE_DETECTED,
	CHANNELS(CHANNEL("Time since presence detected", time_second_16)),
};

/*******************************************************************************
 * Ambient temperature
 ******************************************************************************/
SENSOR_TYPE(avg_amb_temp_in_day) = {
	.id = BT_MESH_PROP_ID_AVG_AMB_TEMP_IN_A_PERIOD_OF_DAY,
	CHANNELS(CHANNEL("Temperature", temp_8),
		 CHANNEL("Start time", time_decihour_8),
		 CHANNEL("End time", time_decihour_8)),
};
SENSOR_TYPE(indoor_amb_temp_stat_values) = {
	.id = BT_MESH_PROP_ID_INDOOR_AMB_TEMP_STAT_VALUES,
	CHANNELS(CHANNEL("Avg", temp_8),
		 CHANNEL("Standard deviation", temp_8),
		 CHANNEL("Min", temp_8),
		 CHANNEL("Max", temp_8),
		 CHANNEL("Sensing duration", time_exp_8)),
};
SENSOR_TYPE(outdoor_stat_values) = {
	.id = BT_MESH_PROP_ID_OUTDOOR_STAT_VALUES,
	CHANNELS(CHANNEL("Avg", temp_8),
		 CHANNEL("Standard deviation", temp_8),
		 CHANNEL("Min", temp_8),
		 CHANNEL("Max", temp_8),
		 CHANNEL("Sensing duration", time_exp_8)),
};
SENSOR_TYPE(present_amb_temp) = {
	.id = BT_MESH_PROP_ID_PRESENT_AMB_TEMP,
	CHANNELS(CHANNEL("Present ambient temperature", temp_8)),
};
SENSOR_TYPE(present_indoor_amb_temp) = {
	.id = BT_MESH_PROP_ID_PRESENT_INDOOR_AMB_TEMP,
	CHANNELS(CHANNEL("Present indoor ambient temperature", temp_8)),
};
SENSOR_TYPE(present_outdoor_amb_temp) = {
	.id = BT_MESH_PROP_ID_PRESENT_OUTDOOR_AMB_TEMP,
	CHANNELS(CHANNEL("Present outdoor ambient temperature", temp_8)),
};
SENSOR_TYPE(desired_amb_temp) = {
	.id = BT_MESH_PROP_ID_DESIRED_AMB_TEMP,
	CHANNELS(CHANNEL("Desired ambient temperature", temp_8)),
};
SENSOR_TYPE(precise_present_amb_temp) = {
	.id = BT_MESH_PROP_ID_PRECISE_PRESENT_AMB_TEMP,
	CHANNELS(CHANNEL("Precise present ambient temperature", temp)),
};

/*******************************************************************************
 * Environmental
 ******************************************************************************/
SENSOR_TYPE(apparent_wind_direction) = {
	.id = BT_MESH_PROP_ID_APPARENT_WIND_DIRECTION,
	CHANNELS(CHANNEL("Apparent Wind Direction", direction_16)),
};
SENSOR_TYPE(apparent_wind_speed) = {
	.id = BT_MESH_PROP_ID_APPARENT_WIND_SPEED,
	CHANNELS(CHANNEL("Apparent Wind Speed", wind_speed)),
};
SENSOR_TYPE(dew_point) = {
	.id = BT_MESH_PROP_ID_DEW_POINT,
	CHANNELS(CHANNEL("Dew Point", temp_8_wide)),
};
SENSOR_TYPE(gust_factor) = {
	.id = BT_MESH_PROP_ID_GUST_FACTOR,
	CHANNELS(CHANNEL("Gust Factor", gust_factor)),
};
SENSOR_TYPE(heat_index) = {
	.id = BT_MESH_PROP_ID_HEAT_INDEX,
	CHANNELS(CHANNEL("Heat Index", temp_8_wide)),
};
SENSOR_TYPE(present_amb_rel_humidity) = {
	.id = BT_MESH_PROP_ID_PRESENT_AMB_REL_HUMIDITY,
	CHANNELS(CHANNEL("Present ambient relative humidity", percentage_16)),
};
SENSOR_TYPE(present_amb_co2_concentration) = {
	.id = BT_MESH_PROP_ID_PRESENT_AMB_CO2_CONCENTRATION,
	CHANNELS(CHANNEL("Present ambient CO2 concentration",
			 co2_concentration)),
};
SENSOR_TYPE(present_amb_voc_concentration) = {
	.id = BT_MESH_PROP_ID_PRESENT_AMB_VOC_CONCENTRATION,
	CHANNELS(CHANNEL("Present ambient VOC concentration",
			 voc_concentration)),
};
SENSOR_TYPE(present_amb_noise) = {
	.id = BT_MESH_PROP_ID_PRESENT_AMB_NOISE,
	CHANNELS(CHANNEL("Present ambient noise", noise)),
};
SENSOR_TYPE(present_indoor_relative_humidity) = {
	.id = BT_MESH_PROP_ID_PRESENT_INDOOR_RELATIVE_HUMIDITY,
	CHANNELS(CHANNEL("Humidity", percentage_16)),
};
SENSOR_TYPE(present_outdoor_relative_humidity) = {
	.id = BT_MESH_PROP_ID_PRESENT_OUTDOOR_RELATIVE_HUMIDITY,
	CHANNELS(CHANNEL("Humidity", percentage_16)),
};
SENSOR_TYPE(magnetic_declination) = {
	.id = BT_MESH_PROP_ID_MAGNETIC_DECLINATION,
	CHANNELS(CHANNEL("Magnetic Declination", direction_16)),
};
SENSOR_TYPE(magnetic_flux_density_2d) = {
	.id = BT_MESH_PROP_ID_MAGNETIC_FLUX_DENSITY_2D,
	CHANNELS(CHANNEL("X-axis", magnetic_flux_density),
		 CHANNEL("Y-axis", magnetic_flux_density)),
};
SENSOR_TYPE(magnetic_flux_density_3d) = {
	.id = BT_MESH_PROP_ID_MAGNETIC_FLUX_DENSITY_3D,
	CHANNELS(CHANNEL("X-axis", magnetic_flux_density),
		 CHANNEL("Y-axis", magnetic_flux_density),
		 CHANNEL("Z-axis", magnetic_flux_density)),
};
SENSOR_TYPE(pollen_concentration) = {
	.id = BT_MESH_PROP_ID_POLLEN_CONCENTRATION,
	CHANNELS(CHANNEL("Pollen Concentration", pollen_concentration)),
};
SENSOR_TYPE(air_pressure) = {
	.id = BT_MESH_PROP_ID_AIR_PRESSURE,
	CHANNELS(CHANNEL("Pressure", pressure)),
};
SENSOR_TYPE(pressure) = {
	.id = BT_MESH_PROP_ID_PRESSURE,
	CHANNELS(CHANNEL("Pressure", pressure)),
};
SENSOR_TYPE(rainfall) = {
	.id = BT_MESH_PROP_ID_RAINFALL,
	CHANNELS(CHANNEL("Rainfall", rainfall)),
};
SENSOR_TYPE(true_wind_direction) = {
	.id = BT_MESH_PROP_ID_TRUE_WIND_DIRECTION,
	CHANNELS(CHANNEL("True Wind Direction", direction_16)),
};
SENSOR_TYPE(true_wind_speed) = {
	.id = BT_MESH_PROP_ID_TRUE_WIND_SPEED,
	CHANNELS(CHANNEL("True Wind Speed", wind_speed)),
};
SENSOR_TYPE(uv_index) = {
	.id = BT_MESH_PROP_ID_UV_INDEX,
	CHANNELS(CHANNEL("UV Index", uv_index)),
};
SENSOR_TYPE(wind_chill) = {
	.id = BT_MESH_PROP_ID_WIND_CHILL,
	CHANNELS(CHANNEL("Wind Chill", temp_8_wide)),
};

/*******************************************************************************
 * Device operating temperature
 ******************************************************************************/
SENSOR_TYPE(dev_op_temp_range_spec) = {
	.id = BT_MESH_PROP_ID_DEV_OP_TEMP_RANGE_SPEC,
	CHANNELS(CHANNEL("Min", temp),
		 CHANNEL("Max", temp)),
};
SENSOR_TYPE(dev_op_temp_stat_values) = {
	.id = BT_MESH_PROP_ID_DEV_OP_TEMP_STAT_VALUES,
	CHANNELS(CHANNEL("Avg", temp),
		 CHANNEL("Standard deviation", temp),
		 CHANNEL("Min", temp),
		 CHANNEL("Max", temp),
		 CHANNEL("Sensing duration", time_exp_8)),
};
SENSOR_TYPE(present_dev_op_temp) = {
	.id = BT_MESH_PROP_ID_PRESENT_DEV_OP_TEMP,
	CHANNELS(CHANNEL("Temperature", temp)),
};

SENSOR_TYPE(rel_runtime_in_a_dev_op_temp_range) = {
	.id = BT_MESH_PROP_ID_REL_RUNTIME_IN_A_DEV_OP_TEMP_RANGE,
	CHANNELS(CHANNEL("Relative value", percentage_8),
		 CHANNEL("Min", temp),
		 CHANNEL("Max", temp))
};

/*******************************************************************************
 * Electrical input
 ******************************************************************************/
SENSOR_TYPE(avg_input_current) = {
	.id = BT_MESH_PROP_ID_AVG_INPUT_CURRENT,
	CHANNELS(CHANNEL("Electric current value", electric_current),
		 CHANNEL("Sensing duration", time_exp_8)),
};
SENSOR_TYPE(avg_input_voltage) = {
	.id = BT_MESH_PROP_ID_AVG_INPUT_VOLTAGE,
	CHANNELS(CHANNEL("Voltage value", voltage),
		 CHANNEL("Sensing duration", time_exp_8)),
};
SENSOR_TYPE(input_current_range_spec) = {
	.id = BT_MESH_PROP_ID_INPUT_CURRENT_RANGE_SPEC,
	CHANNELS(CHANNEL("Min", electric_current),
		 CHANNEL("Max", electric_current),
		 CHANNEL("Typical electric current value", electric_current)),
};
SENSOR_TYPE(input_current_stat) = {
	.id = BT_MESH_PROP_ID_INPUT_CURRENT_STAT,
	.channel_count = ARRAY_SIZE(electric_current_stats),
	.channels = electric_current_stats,
};
SENSOR_TYPE(input_voltage_range_spec) = {
	.id = BT_MESH_PROP_ID_INPUT_VOLTAGE_RANGE_SPEC,
	CHANNELS(CHANNEL("Min", voltage),
		 CHANNEL("Max", voltage),
		 CHANNEL("Typical voltage value", voltage)),
};
SENSOR_TYPE(input_voltage_stat) = {
	.id = BT_MESH_PROP_ID_INPUT_VOLTAGE_STAT,
	.channel_count = ARRAY_SIZE(voltage_stats),
	.channels = voltage_stats,
};
SENSOR_TYPE(present_input_current) = {
	.id = BT_MESH_PROP_ID_PRESENT_INPUT_CURRENT,
	CHANNELS(CHANNEL("Present input current", electric_current)),
};
SENSOR_TYPE(present_input_ripple_voltage) = {
	.id = BT_MESH_PROP_ID_PRESENT_INPUT_RIPPLE_VOLTAGE,
	CHANNELS(CHANNEL("Present input ripple voltage", percentage_8)),
};
SENSOR_TYPE(present_input_voltage) = {
	.id = BT_MESH_PROP_ID_PRESENT_INPUT_VOLTAGE,
	CHANNELS(CHANNEL("Present input voltage", voltage)),
};
SENSOR_TYPE(rel_runtime_in_an_input_current_range) = {
	.id = BT_MESH_PROP_ID_REL_RUNTIME_IN_AN_INPUT_CURRENT_RANGE,
	CHANNELS(CHANNEL("Relative runtime value", percentage_8),
		 CHANNEL("Min", electric_current),
		 CHANNEL("Max", electric_current)),
};

SENSOR_TYPE(rel_runtime_in_an_input_voltage_range) = {
	.id = BT_MESH_PROP_ID_REL_RUNTIME_IN_AN_INPUT_VOLTAGE_RANGE,
	CHANNELS(CHANNEL("Relative runtime value", percentage_8),
		 CHANNEL("Min", voltage),
		 CHANNEL("Max", voltage)),
};

/*******************************************************************************
 * Energy management
 ******************************************************************************/
SENSOR_TYPE(dev_power_range_spec) = {
	.id = BT_MESH_PROP_ID_DEV_POWER_RANGE_SPEC,
	CHANNELS(CHANNEL("Min power value", power),
		 CHANNEL("Typical power value", power),
		 CHANNEL("Max power value", power)),
};
SENSOR_TYPE(present_dev_input_power) = {
	.id = BT_MESH_PROP_ID_PRESENT_DEV_INPUT_POWER,
	CHANNELS(CHANNEL("Present device input power", power)),
};
SENSOR_TYPE(present_dev_op_efficiency) = {
	.id = BT_MESH_PROP_ID_PRESENT_DEV_OP_EFFICIENCY,
	CHANNELS(CHANNEL("Present device operating efficiency", percentage_8)),
};
SENSOR_TYPE(tot_dev_energy_use) = {
	.id = BT_MESH_PROP_ID_TOT_DEV_ENERGY_USE,
	CHANNELS(CHANNEL("Total device energy use", energy)),
};
SENSOR_TYPE(precise_tot_dev_energy_use) = {
	.id = BT_MESH_PROP_ID_PRECISE_TOT_DEV_ENERGY_USE,
	CHANNELS(CHANNEL("Total device energy use", energy32)),
};
SENSOR_TYPE(dev_energy_use_since_turn_on) = {
	.id = BT_MESH_PROP_ID_DEV_ENERGY_USE_SINCE_TURN_ON,
	CHANNELS(CHANNEL("Device energy use since turn on", energy)),
};
SENSOR_TYPE(power_factor) = {
	.id = BT_MESH_PROP_ID_POWER_FACTOR,
	CHANNELS(CHANNEL("Cosine of the angle", cos_of_the_angle)),
};
SENSOR_TYPE(rel_dev_energy_use_in_a_period_of_day) = {
	.id = BT_MESH_PROP_ID_REL_DEV_ENERGY_USE_IN_A_PERIOD_OF_DAY,
	CHANNELS(CHANNEL("Energy", energy),
		 CHANNEL("Start time", time_decihour_8),
		 CHANNEL("End time", time_decihour_8)),
};
SENSOR_TYPE(apparent_energy) = {
	.id = BT_MESH_PROP_ID_APPARENT_ENERGY,
	CHANNELS(CHANNEL("Apparent energy", apparent_energy32)),
};
SENSOR_TYPE(apparent_power) = {
	.id = BT_MESH_PROP_ID_APPARENT_POWER,
	CHANNELS(CHANNEL("Apparent power", apparent_power)),
};
SENSOR_TYPE(active_energy_loadside) = {
	.id = BT_MESH_PROP_ID_ACTIVE_ENERGY_LOADSIDE,
	CHANNELS(CHANNEL("Energy", energy32)),
};
SENSOR_TYPE(active_power_loadside) = {
	.id = BT_MESH_PROP_ID_ACTIVE_POWER_LOADSIDE,
	CHANNELS(CHANNEL("Power", power)),
};

/*******************************************************************************
 * Photometry
 ******************************************************************************/
SENSOR_TYPE(present_amb_light_level) = {
	.id = BT_MESH_PROP_ID_PRESENT_AMB_LIGHT_LEVEL,
	CHANNELS(CHANNEL("Present ambient light level", illuminance)),
};
SENSOR_TYPE(initial_cie_1931_chromaticity_coords) = {
	.id = BT_MESH_PROP_ID_INITIAL_CIE_1931_CHROMATICITY_COORDS,
	CHANNELS(CHANNEL("Initial CIE 1931 chromaticity x-coordinate", chromaticity_coordinate),
		 CHANNEL("Initial CIE 1931 chromaticity y-coordinate", chromaticity_coordinate)),
};
SENSOR_TYPE(present_cie_1931_chromaticity_coords) = {
	.id = BT_MESH_PROP_ID_PRESENT_CIE_1931_CHROMATICITY_COORDS,
	CHANNELS(CHANNEL("Present CIE 1931 chromaticity x-coordinate", chromaticity_coordinate),
		 CHANNEL("Present CIE 1931 chromaticity y-coordinate", chromaticity_coordinate)),
};
SENSOR_TYPE(initial_correlated_col_temp) = {
	.id = BT_MESH_PROP_ID_INITIAL_CORRELATED_COL_TEMP,
	CHANNELS(CHANNEL("Initial correlated color temperature",
			 correlated_color_temp)),
};
SENSOR_TYPE(present_correlated_col_temp) = {
	.id = BT_MESH_PROP_ID_PRESENT_CORRELATED_COL_TEMP,
	CHANNELS(CHANNEL("Present correlated color temperature",
			 correlated_color_temp)),
};
SENSOR_TYPE(present_illuminance) = {
	.id = BT_MESH_PROP_ID_PRESENT_ILLUMINANCE,
	CHANNELS(CHANNEL("Present illuminance", illuminance)),
};
SENSOR_TYPE(initial_luminous_flux) = {
	.id = BT_MESH_PROP_ID_INITIAL_LUMINOUS_FLUX,
	CHANNELS(CHANNEL("Initial luminous flux", luminous_flux)),
};
SENSOR_TYPE(present_luminous_flux) = {
	.id = BT_MESH_PROP_ID_PRESENT_LUMINOUS_FLUX,
	CHANNELS(CHANNEL("Present luminous flux", luminous_flux)),
};
SENSOR_TYPE(initial_planckian_distance) = {
	.id = BT_MESH_PROP_ID_INITIAL_PLANCKIAN_DISTANCE,
	CHANNELS(CHANNEL("Initial planckian distance", chromatic_distance)),
};
SENSOR_TYPE(present_planckian_distance) = {
	.id = BT_MESH_PROP_ID_PRESENT_PLANCKIAN_DISTANCE,
	CHANNELS(CHANNEL("Present planckian distance", chromatic_distance)),
};
SENSOR_TYPE(rel_exposure_time_in_an_illuminance_range) = {
	.id = BT_MESH_PROP_ID_REL_EXPOSURE_TIME_IN_AN_ILLUMINANCE_RANGE,
	CHANNELS(CHANNEL("Relative value", percentage_8),
		 CHANNEL("Min", illuminance),
		 CHANNEL("Max", illuminance))
};
SENSOR_TYPE(tot_light_exposure_time) = {
	.id = BT_MESH_PROP_ID_TOT_LIGHT_EXPOSURE_TIME,
	CHANNELS(CHANNEL("Total light exposure time", time_hour_24)),
};
SENSOR_TYPE(lumen_maintenance_factor) = {
	.id = BT_MESH_PROP_ID_LUMEN_MAINTENANCE_FACTOR,
	CHANNELS(CHANNEL("Lumen maintenance factor", percentage_8)),
};
SENSOR_TYPE(luminous_efficacy) = {
	.id = BT_MESH_PROP_ID_LUMINOUS_EFFICACY,
	CHANNELS(CHANNEL("Luminous efficacy", luminous_efficacy)),
};
SENSOR_TYPE(luminous_energy_since_turn_on) = {
	.id = BT_MESH_PROP_ID_LUMINOUS_ENERGY_SINCE_TURN_ON,
	CHANNELS(CHANNEL("Luminous energy since turn on", luminous_energy)),
};
SENSOR_TYPE(luminous_exposure) = {
	.id = BT_MESH_PROP_ID_LUMINOUS_EXPOSURE,
	CHANNELS(CHANNEL("Luminous exposure", luminous_exposure)),
};
SENSOR_TYPE(luminous_flux_range) = {
	.id = BT_MESH_PROP_ID_LUMINOUS_FLUX_RANGE,
	CHANNELS(CHANNEL("Min", luminous_flux),
		 CHANNEL("Max", luminous_flux)),
};

/*******************************************************************************
 * Power supply output
 ******************************************************************************/
SENSOR_TYPE(avg_output_current) = {
	.id = BT_MESH_PROP_ID_AVG_OUTPUT_CURRENT,
	CHANNELS(CHANNEL("Electric current value", electric_current),
		 CHANNEL("Sensing duration", time_exp_8)),
};
SENSOR_TYPE(avg_output_voltage) = {
	.id = BT_MESH_PROP_ID_AVG_OUTPUT_VOLTAGE,
	CHANNELS(CHANNEL("Voltage value", voltage),
		 CHANNEL("Sensing duration", time_exp_8)),
};
SENSOR_TYPE(output_current_range) = {
	.id = BT_MESH_PROP_ID_OUTPUT_CURRENT_RANGE,
	CHANNELS(CHANNEL("Min", electric_current),
		 CHANNEL("Max", electric_current)),
};
SENSOR_TYPE(output_current_stat) = {
	.id = BT_MESH_PROP_ID_OUTPUT_CURRENT_STAT,
	.channel_count = ARRAY_SIZE(electric_current_stats),
	.channels = electric_current_stats,
};
SENSOR_TYPE(output_ripple_voltage_spec) = {
	.id = BT_MESH_PROP_ID_OUTPUT_RIPPLE_VOLTAGE_SPEC,
	CHANNELS(CHANNEL("Output ripple voltage", percentage_8)),
};
SENSOR_TYPE(output_voltage_range) = {
	.id = BT_MESH_PROP_ID_OUTPUT_VOLTAGE_RANGE,
	CHANNELS(CHANNEL("Min", voltage),
		 CHANNEL("Max", voltage)),
};
SENSOR_TYPE(output_voltage_stat) = {
	.id = BT_MESH_PROP_ID_OUTPUT_VOLTAGE_STAT,
	.channel_count = ARRAY_SIZE(voltage_stats),
	.channels = voltage_stats,
};
SENSOR_TYPE(present_output_current) = {
	.id = BT_MESH_PROP_ID_PRESENT_OUTPUT_CURRENT,
	CHANNELS(CHANNEL("Present output current", electric_current)),
};
SENSOR_TYPE(present_output_voltage) = {
	.id = BT_MESH_PROP_ID_PRESENT_OUTPUT_VOLTAGE,
	CHANNELS(CHANNEL("Present output voltage", voltage)),
};
SENSOR_TYPE(present_rel_output_ripple_voltage) = {
	.id = BT_MESH_PROP_ID_PRESENT_REL_OUTPUT_RIPPLE_VOLTAGE,
	CHANNELS(CHANNEL("Output ripple voltage", percentage_8)),
};

/*******************************************************************************
 * Warranty and service
 ******************************************************************************/
SENSOR_TYPE(gain) = {
	.id = BT_MESH_PROP_ID_SENSOR_GAIN,
	CHANNELS(CHANNEL("Sensor gain", coefficient)),
};
SENSOR_TYPE(rel_dev_runtime_in_a_generic_level_range) = {
	.id = BT_MESH_PROP_ID_REL_DEV_RUNTIME_IN_A_GENERIC_LEVEL_RANGE,
	CHANNELS(CHANNEL("Relative value", percentage_8),
		 CHANNEL("Min", gen_lvl),
		 CHANNEL("Max", gen_lvl)),
};

SENSOR_TYPE(total_dev_runtime) = {
	.id = BT_MESH_PROP_ID_TOT_DEV_RUNTIME,
	CHANNELS(CHANNEL("Total device runtime", time_decihour_8)),
};

/******************************************************************************/

const struct bt_mesh_sensor_type *bt_mesh_sensor_type_get(uint16_t id)
{
	STRUCT_SECTION_FOREACH(bt_mesh_sensor_type, type) {
		if (type->id == id) {
			return type;
		}
	}

	return NULL;
}
