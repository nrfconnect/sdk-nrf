/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <string.h>
#include <stdio.h>
#include "sensor.h"
#include <toolchain/common.h>
#include <bluetooth/mesh/properties.h>
#include <bluetooth/mesh/sensor_types.h>

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
	const Z_STRUCT_SECTION_ITERABLE(bt_mesh_sensor_type,                   \
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
UNIT(db) = { "Decibel", "dB" };
UNIT(lux) = { "Lux", "lx" };
UNIT(lux_hour) = { "Lux hours", "lxh" };
UNIT(lumen) = { "Lumen", "lm" };
UNIT(lumen_per_watt) = { "Lumen per Watt", "lm/W" };
UNIT(lumen_hour) = { "Lumen hours", "lmh" };
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

#define SCALAR_REPR_RANGED(_scalar, _flags, _max)                              \
	{                                                                      \
		.flags = ((_flags) | (SCALAR_IS_DIV(_scalar) ? DIVIDE : 0)),   \
		.max = _max,                                                   \
		.value = (s64_t)((SCALAR_IS_DIV(_scalar) ? (1.0 / (_scalar)) : \
							   (_scalar)) +        \
				 0.5),                                         \
	}

#define SCALAR_REPR(_scalar, _flags) SCALAR_REPR_RANGED(_scalar, _flags, 0)

#ifdef CONFIG_BT_MESH_SENSOR_LABELS

#define SCALAR_FORMAT_MAX(_size, _flags, _unit, _scalar, _max)                 \
	{                                                                      \
		.unit = &bt_mesh_sensor_unit_##_unit,                          \
		.encode = scalar_encode, .decode = scalar_decode,              \
		.size = _size,                                                 \
		.user_data = (void *)&(                                        \
			(const struct scalar_repr)SCALAR_REPR_RANGED(          \
				_scalar, ((_flags) | HAS_MAX), _max)),         \
	}

#define SCALAR_FORMAT(_size, _flags, _unit, _scalar)                           \
	{                                                                      \
		.unit = &bt_mesh_sensor_unit_##_unit,                          \
		.encode = scalar_encode, .decode = scalar_decode,              \
		.size = _size,                                                 \
		.user_data = (void *)&((const struct scalar_repr)SCALAR_REPR(  \
			_scalar, _flags)),                                     \
	}
#else

#define SCALAR_FORMAT_MAX(_size, _flags, _unit, _scalar, _max)                 \
	{                                                                      \
		.encode = scalar_encode, .decode = scalar_decode,              \
		.size = _size,                                                 \
		.user_data = (void *)&(                                        \
			(const struct scalar_repr)SCALAR_REPR_RANGED(          \
				_scalar, ((_flags) | HAS_MAX), _max)),         \
	}

#define SCALAR_FORMAT(_size, _flags, _unit, _scalar)                           \
	{                                                                      \
		.encode = scalar_encode, .decode = scalar_decode,              \
		.size = _size,                                                 \
		.user_data = (void *)&((const struct scalar_repr)SCALAR_REPR(  \
			_scalar, _flags)),                                     \
	}
#endif

enum scalar_repr_flags {
	UNSIGNED = 0,
	SIGNED = BIT(1),
	DIVIDE = BIT(2),
	/** The highest encoded value represents "undefined" */
	HAS_UNDEFINED = BIT(3),
	/**
	 * The second highest encoded value represents
	 * "value is higher than xxx".
	 */
	HAS_HIGHER_THAN = (BIT(4)),
	/** The second highest encoded value represents "value is invalid" */
	HAS_INVALID = (BIT(5)),
	HAS_MAX = BIT(6),
};

struct scalar_repr {
	enum scalar_repr_flags flags;
	u32_t max; /**< Highest encoded value */
	s64_t value;
};

static s64_t mul_scalar(s64_t val, const struct scalar_repr *repr)
{
	return (repr->flags & DIVIDE) ? (val / repr->value) :
	       (val * repr->value);
}

static s64_t div_scalar(s64_t val, const struct scalar_repr *repr)
{
	return (repr->flags & DIVIDE) ? (val * repr->value) :
	       (val / repr->value);
}

static u32_t scalar_max(const struct bt_mesh_sensor_format *format)
{
	const struct scalar_repr *repr = format->user_data;

	if (repr->flags & HAS_MAX) {
		return repr->max;
	}

	if (repr->flags & SIGNED) {
		return BIT64(8 * format->size - 1) - 1;
	}

	u32_t max_value = BIT64(8 * format->size) - 1;

	if (repr->flags & (HAS_HIGHER_THAN | HAS_INVALID)) {
		max_value -= 2;
	} else if (repr->flags & HAS_UNDEFINED) {
		max_value -= 1;
	}

	return max_value;
}

static s32_t scalar_min(const struct bt_mesh_sensor_format *format)
{
	const struct scalar_repr *repr = format->user_data;

	if (repr->flags & SIGNED) {
		return -BIT64(8 * format->size - 1);
	}

	return 0;
}

static int scalar_encode(const struct bt_mesh_sensor_format *format,
			 const struct sensor_value *val,
			 struct net_buf_simple *buf)
{
	const struct scalar_repr *repr = format->user_data;

	if (net_buf_simple_tailroom(buf) < format->size) {
		return -ENOMEM;
	}

	s64_t raw = div_scalar(val->val1, repr) +
		    div_scalar(val->val2, repr) / 1000000LL;

	u32_t max_value = scalar_max(format);
	s32_t min_value = scalar_min(format);

	if (raw > max_value || raw < min_value) {
		u32_t type_max = BIT64(8 * format->size) - 1;

		if (repr->flags & (HAS_HIGHER_THAN | HAS_INVALID)) {
			raw = type_max - 2;
		} else if (repr->flags & HAS_UNDEFINED) {
			raw = type_max - 1;
		} else {
			return -ERANGE;
		}
	}

	switch (format->size) {
	case 1:
		net_buf_simple_add_u8(buf, raw);
		break;
	case 2:
		net_buf_simple_add_le16(buf, raw);
		break;
	case 3:
		net_buf_simple_add_le24(buf, raw);
		break;
	case 4:
		net_buf_simple_add_le32(buf, raw);
		break;
	default:
		return -EIO;
	}

	return 0;
}

static int scalar_decode(const struct bt_mesh_sensor_format *format,
			 struct net_buf_simple *buf, struct sensor_value *val)
{
	const struct scalar_repr *repr = format->user_data;

	if (buf->len < format->size) {
		return -ENOMEM;
	}

	s32_t raw;

	switch (format->size) {
	case 1:
		if (repr->flags & SIGNED) {
			raw = (s8_t) net_buf_simple_pull_u8(buf);
		} else {
			raw = net_buf_simple_pull_u8(buf);
		}
		break;
	case 2:
		if (repr->flags & SIGNED) {
			raw = (s16_t) net_buf_simple_pull_le16(buf);
		} else {
			raw = net_buf_simple_pull_le16(buf);
		}
		break;
	case 3:
		raw = net_buf_simple_pull_le24(buf);

		if ((repr->flags & SIGNED) && (raw & BIT(24))) {
			raw |= (BIT_MASK(8) << 24);
		}
		break;
	case 4:
		raw = net_buf_simple_pull_le32(buf);
		break;
	default:
		return -ERANGE;
	}

	u32_t max_value = scalar_max(format);
	s32_t min_value = scalar_min(format);

	if (raw < min_value || raw > max_value) {
		return -ERANGE;
	}

	s64_t million = mul_scalar(raw * 1000000LL, repr);

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

	u8_t b = net_buf_simple_pull_u8(buf);

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

	net_buf_simple_add_u8(buf, sensor_powtime_encode((val->val1 * 1000) +
							 (val->val2 / 1000)));
	return 0;
}

static int exp_1_1_decode(const struct bt_mesh_sensor_format *format,
			  struct net_buf_simple *buf, struct sensor_value *val)
{
	if (buf->len < 1) {
		return -ENOMEM;
	}

	u64_t time_ns = sensor_powtime_decode_ns(net_buf_simple_pull_u8(buf));

	val->val1 = time_ns / 1000000L;
	val->val2 = time_ns % 1000000L;
	return 0;
}

static int float32_encode(const struct bt_mesh_sensor_format *format,
			  const struct sensor_value *val,
			  struct net_buf_simple *buf)
{
	if (net_buf_simple_tailroom(buf) < 1) {
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
	if (buf->len < 1) {
		return -ENOMEM;
	}

	float fvalue;

	memcpy(&fvalue, net_buf_simple_pull_mem(buf, sizeof(float)),
	       sizeof(float));

	val->val1 = (u32_t)fvalue;
	val->val2 = ((u32_t)(fvalue * 1000000.0f)) / 1000000L;
	return 0;
}

/*******************************************************************************
 * Sensor Formats
 *
 * Formats define how single channels are encoded and decoded, and are generally
 * represented as a scalar format with multipliers.
 *
 * See Mesh Model Specification Section 2.3 for details.
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
					  200);

/*******************************************************************************
 * Environmental formats
 ******************************************************************************/
FORMAT(temp_8)		  = SCALAR_FORMAT(1,
					  SIGNED,
					  celsius,
					  SCALAR(1, -1));
FORMAT(temp)		  = SCALAR_FORMAT(2,
					  SIGNED,
					  celsius,
					  SCALAR(1e-2, 0));
FORMAT(co2_concentration) = SCALAR_FORMAT(2,
					  (HAS_HIGHER_THAN | HAS_UNDEFINED),
					  ppm,
					  SCALAR(1, 0));
FORMAT(noise)		  = SCALAR_FORMAT(1,
					  (UNSIGNED |
					   HAS_HIGHER_THAN |
					   HAS_UNDEFINED),
					  db,
					  SCALAR(1, 0));
FORMAT(voc_concentration) = SCALAR_FORMAT_MAX(2,
					      (UNSIGNED |
					       HAS_HIGHER_THAN |
					       HAS_UNDEFINED),
					      ppb,
					      SCALAR(1, 0),
					      65533);
FORMAT(humidity)          = SCALAR_FORMAT_MAX(2,
					      UNSIGNED,
					      percent,
					      SCALAR(1e-2, 0),
					      10000);

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
					    SCALAR(1e-1, 0));
FORMAT(time_second_16)	    = SCALAR_FORMAT(2,
					    (UNSIGNED | HAS_UNDEFINED),
					    seconds,
					    SCALAR(1, 0));
FORMAT(time_exp_8)	    = {
	 .size = 1,
#ifdef CONFIG_BT_MESH_SENSOR_LABELS
	.unit = &bt_mesh_sensor_unit_seconds,
#endif
	.encode = exp_1_1_encode,
	.decode = exp_1_1_decode,
};

/*******************************************************************************
 * Electrical formats
 ******************************************************************************/
FORMAT(electric_current) = SCALAR_FORMAT(2,
					 (UNSIGNED | HAS_UNDEFINED),
					 ampere,
					 SCALAR(1e-2, 0));
FORMAT(voltage)		 = SCALAR_FORMAT(2,
					 (UNSIGNED | HAS_UNDEFINED),
					 volt,
					 SCALAR(1, -6));
FORMAT(energy32)	 = SCALAR_FORMAT(4,
					 UNSIGNED | HAS_INVALID | HAS_UNDEFINED,
					 kwh,
					 SCALAR(1e-3, 0));
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
FORMAT(chromatic_distance)      = SCALAR_FORMAT_MAX(2,
						(SIGNED |
						 HAS_INVALID |
						 HAS_UNDEFINED),
						unitless, SCALAR(1e-5, 0),
						5000);
FORMAT(chromaticity_coordinate) = SCALAR_FORMAT(2,
						UNSIGNED,
						unitless,
						SCALAR(1, -16));
FORMAT(correlated_color_temp)	= SCALAR_FORMAT(2,
						(UNSIGNED | HAS_UNDEFINED),
						kelvin,
						SCALAR(1, 0));
FORMAT(illuminance)		= SCALAR_FORMAT(3,
						(UNSIGNED | HAS_UNDEFINED),
						lux,
						SCALAR(1e-2, 0));
FORMAT(luminous_efficacy)	= SCALAR_FORMAT(2,
						(UNSIGNED | HAS_UNDEFINED),
						lumen_per_watt,
						SCALAR(1e-1, 0));
FORMAT(luminous_energy)		= SCALAR_FORMAT(3,
						(UNSIGNED | HAS_UNDEFINED),
						lumen_hour,
						SCALAR(1e3, 0));
FORMAT(luminous_exposure)	= SCALAR_FORMAT(3,
						(UNSIGNED | HAS_UNDEFINED),
						lux_hour,
						SCALAR(1e3, 0));
FORMAT(luminous_flux)		= SCALAR_FORMAT(2,
						(UNSIGNED | HAS_UNDEFINED),
						lumen,
						SCALAR(1, 0));

/*******************************************************************************
 * Miscellaneous formats
 ******************************************************************************/
FORMAT(count_16)	 = SCALAR_FORMAT(2,
					 (UNSIGNED | HAS_UNDEFINED),
					 unitless,
					 SCALAR(1, 0));
FORMAT(gen_lvl)		 = SCALAR_FORMAT(2,
					 UNSIGNED,
					 unitless,
					 SCALAR(1, 0));
FORMAT(cos_of_the_angle) = SCALAR_FORMAT_MAX(1,
					     SIGNED,
					     unitless,
					     SCALAR(1, 0),
					     100);
FORMAT(boolean) = {
	.encode = boolean_encode,
	.decode = boolean_decode,
	.size	= 1,
#ifdef CONFIG_BT_MESH_SENSOR_LABELS
	.unit = &bt_mesh_sensor_unit_unitless,
#endif
};
FORMAT(coefficient) = {
	.encode = float32_encode,
	.decode = float32_decode,
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
	.flags = BT_MESH_SENSOR_TYPE_FLAG_SERIES,
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
SENSOR_TYPE(present_amb_rel_humidity) = {
	.id = BT_MESH_PROP_ID_PRESENT_AMB_REL_HUMIDITY,
	CHANNELS(CHANNEL("Present ambient relative humidity", humidity)),
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
	.flags = BT_MESH_SENSOR_TYPE_FLAG_SERIES,
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
	.flags = BT_MESH_SENSOR_TYPE_FLAG_SERIES,
	CHANNELS(CHANNEL("Relative runtime value", percentage_8),
		 CHANNEL("Min", electric_current),
		 CHANNEL("Max", electric_current)),
};

SENSOR_TYPE(rel_runtime_in_an_input_voltage_range) = {
	.id = BT_MESH_PROP_ID_REL_RUNTIME_IN_AN_INPUT_VOLTAGE_RANGE,
	.flags = BT_MESH_SENSOR_TYPE_FLAG_SERIES,
	CHANNELS(CHANNEL("Relative runtime value", percentage_8),
		 CHANNEL("Min", voltage),
		 CHANNEL("Max", voltage)),
};

/*******************************************************************************
 * Energy management
 ******************************************************************************/
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
	.flags = BT_MESH_SENSOR_TYPE_FLAG_SERIES,
	CHANNELS(CHANNEL("Energy", energy),
		 CHANNEL("Start time", time_decihour_8),
		 CHANNEL("End time", time_decihour_8)),
};
SENSOR_TYPE(rel_dev_runtime_in_a_generic_level_range) = {
	.id = BT_MESH_PROP_ID_REL_DEV_RUNTIME_IN_A_GENERIC_LEVEL_RANGE,
	.flags = BT_MESH_SENSOR_TYPE_FLAG_SERIES,
	CHANNELS(CHANNEL("Relative value", percentage_8),
		 CHANNEL("Min", gen_lvl),
		 CHANNEL("Max", gen_lvl)),
};

/*******************************************************************************
 * Photometry
 ******************************************************************************/
SENSOR_TYPE(present_amb_light_level) = {
	.id = BT_MESH_PROP_ID_PRESENT_AMB_LIGHT_LEVEL,
	CHANNELS(CHANNEL("Present ambient light level", illuminance)),
};
SENSOR_TYPE(present_cie_1931_chromaticity_coords) = {
	.id = BT_MESH_PROP_ID_PRESENT_CIE_1931_CHROMATICITY_COORDS,
	CHANNELS(CHANNEL("Chromaticity x-coordinate", chromaticity_coordinate),
		 CHANNEL("Chromaticity y-coordinate", chromaticity_coordinate)),
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
SENSOR_TYPE(present_luminous_flux) = {
	.id = BT_MESH_PROP_ID_PRESENT_LUMINOUS_FLUX,
	CHANNELS(CHANNEL("Present luminous flux", luminous_flux)),
};
SENSOR_TYPE(present_planckian_distance) = {
	.id = BT_MESH_PROP_ID_PRESENT_PLANCKIAN_DISTANCE,
	CHANNELS(CHANNEL("Present planckian distance", chromatic_distance)),
};
SENSOR_TYPE(rel_exposure_time_in_an_illuminance_range) = {
	.id = BT_MESH_PROP_ID_REL_EXPOSURE_TIME_IN_AN_ILLUMINANCE_RANGE,
	.flags = BT_MESH_SENSOR_TYPE_FLAG_SERIES,
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

SENSOR_TYPE(gain) = {
	.id = BT_MESH_PROP_ID_SENSOR_GAIN,
	CHANNELS(CHANNEL("Sensor gain", coefficient)),
};
/******************************************************************************/

const struct bt_mesh_sensor_type *bt_mesh_sensor_type_get(u16_t id)
{
	Z_STRUCT_SECTION_FOREACH(bt_mesh_sensor_type, type) {
		if (type->id == id) {
			return type;
		}
	}

	return NULL;
}
