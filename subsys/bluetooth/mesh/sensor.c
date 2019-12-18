/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <bluetooth/mesh/sensor.h>
#include <bluetooth/mesh/properties.h>
#include "sensor.h"
#include <stdlib.h>
#include <string.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)
#define LOG_MODULE_NAME bt_mesh_sensor
#include "common/log.h"

/** Scale away the sensor_value fraction, to allow integer math */
#define SENSOR_MILL(_val) ((1000000LL * (_val)->val1) + (_val)->val2)

static enum bt_mesh_sensor_cadence
sensor_cadence(const struct bt_mesh_sensor_threshold *threshold,
	       const struct sensor_value *curr)
{
	s64_t high_mill = SENSOR_MILL(&threshold->range.high);
	s64_t low_mill = SENSOR_MILL(&threshold->range.low);

	if (high_mill == low_mill) {
		return BT_MESH_SENSOR_CADENCE_NORMAL;
	}

	s64_t curr_mill = SENSOR_MILL(curr);
	bool in_range = (curr_mill >= MIN(low_mill, high_mill) &&
			 curr_mill <= MAX(low_mill, high_mill));

	return in_range ? threshold->range.cadence : !threshold->range.cadence;
}

bool bt_mesh_sensor_delta_threshold(const struct bt_mesh_sensor *sensor,
				    const struct sensor_value *curr)
{
	struct sensor_value delta = {
		curr->val1 - sensor->state.prev.val1,
		curr->val2 - sensor->state.prev.val2,
	};
	s64_t delta_mill = SENSOR_MILL(&delta);
	s64_t thrsh_mill;

	if (delta_mill < 0) {
		delta_mill = -delta_mill;
		thrsh_mill = SENSOR_MILL(&sensor->state.threshold.delta.down);
	} else {
		thrsh_mill = SENSOR_MILL(&sensor->state.threshold.delta.up);
	}

	/* If the threshold value is a perentage, we should calculate the actual
	 * threshold value relative to the previous value.
	 */
	if (sensor->state.threshold.delta.type ==
	    BT_MESH_SENSOR_DELTA_PERCENT) {
		s64_t prev_mill = abs(SENSOR_MILL(&sensor->state.prev));

		thrsh_mill = (prev_mill * thrsh_mill) / (100LL * 1000000LL);
	}

	BT_DBG("Delta: %u (%d - %d) thrsh: %u", (u32_t)(delta_mill / 1000000L),
	       (s32_t)curr->val1, (s32_t)sensor->state.prev.val1,
	       (u32_t)(thrsh_mill / 1000000L));

	return (delta_mill > thrsh_mill);
}

void bt_mesh_sensor_cadence_set(struct bt_mesh_sensor *sensor,
				enum bt_mesh_sensor_cadence cadence)
{
	sensor->state.fast_pub = (cadence == BT_MESH_SENSOR_CADENCE_FAST);
}

int sensor_status_id_encode(struct net_buf_simple *buf, u8_t len, u16_t id)
{
	if ((len > 0 && len <= 16) && id < 2048) {
		if (net_buf_simple_tailroom(buf) < 2 + len) {
			return -ENOMEM;
		}

		net_buf_simple_add_le16(buf, ((len - 1) << 1) | (id << 5));
	} else {
		if (net_buf_simple_tailroom(buf) < 3 + len) {
			return -ENOMEM;
		}

		net_buf_simple_add_u8(buf, BIT(0) | (((len - 1) & BIT_MASK(7))
						     << 1));
		net_buf_simple_add_le16(buf, id);
	}

	return 0;
}

void sensor_status_id_decode(struct net_buf_simple *buf, u8_t *len, u16_t *id)
{
	u8_t first = net_buf_simple_pull_u8(buf);

	if (first & BIT(0)) { /* long format */
		*len = (first >> 1) + 1;
		*id = net_buf_simple_pull_le16(buf);
	} else {
		*len = ((first >> 1) & BIT_MASK(4)) + 1;
		*id = (first >> 5) | (net_buf_simple_pull_u8(buf) << 3);
	}
}

int sensor_value_encode(struct net_buf_simple *buf,
			const struct bt_mesh_sensor_type *type,
			const struct sensor_value *values)
{
	for (u32_t i = 0; i < type->channel_count; ++i) {
		int err;

		err = sensor_ch_encode(buf, type->channels[i].format,
				       &values[i]);
		if (err) {
			return err;
		}
	}

	return 0;
}

int sensor_value_decode(struct net_buf_simple *buf,
			const struct bt_mesh_sensor_type *type,
			struct sensor_value *values)
{
	int err;

	for (u32_t i = 0; i < type->channel_count; ++i) {
		err = sensor_ch_decode(buf, type->channels[i].format,
				       &values[i]);
		if (err) {
			return err;
		}
	}

	return 0;
}

int sensor_ch_encode(struct net_buf_simple *buf,
		     const struct bt_mesh_sensor_format *format,
		     const struct sensor_value *value)
{
	return format->encode(format, value, buf);
}

int sensor_ch_decode(struct net_buf_simple *buf,
		     const struct bt_mesh_sensor_format *format,
		     struct sensor_value *value)
{
	return format->decode(format, buf, value);
}

int sensor_status_encode(struct net_buf_simple *buf,
			 const struct bt_mesh_sensor *sensor,
			 const struct sensor_value *values)
{
	const struct bt_mesh_sensor_type *type = sensor->type;
	size_t size = 0;
	int err;

	for (u32_t i = 0; i < type->channel_count; ++i) {
		size += type->channels[i].format->size;
	}

	err = sensor_status_id_encode(buf, size, type->id);
	if (err) {
		return err;
	}

	return sensor_value_encode(buf, type, values);
}

const struct bt_mesh_sensor_format *
bt_mesh_sensor_column_format_get(const struct bt_mesh_sensor_type *type)
{
	if (type->flags & BT_MESH_SENSOR_TYPE_FLAG_SERIES &&
	    type->channel_count >= 2) {
		return type->channels[1].format;
	}

	return &bt_mesh_sensor_format_time_decihour_8;
}

int sensor_column_encode(struct net_buf_simple *buf,
			 struct bt_mesh_sensor *sensor,
			 struct bt_mesh_msg_ctx *ctx,
			 const struct bt_mesh_sensor_column *col)
{
	struct sensor_value values[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];
	const struct bt_mesh_sensor_format *col_format;
	const u64_t width_million =
		(col->end.val1 - col->start.val1) * 1000000L +
		(col->end.val2 - col->start.val2);
	const struct sensor_value width = {
		.val1 = width_million / 1000000L,
		.val2 = width_million % 1000000L,
	};
	int err;

	BT_DBG("Column width: %s", bt_mesh_sensor_ch_str(&width));

	col_format = bt_mesh_sensor_column_format_get(sensor->type);
	if (!col_format) {
		return -ENOTSUP;
	}

	err = sensor_ch_encode(buf, col_format, &col->start);
	if (err) {
		return err;
	}

	/* The sensor columns are transmitted as start+width, not start+end: */
	err = sensor_ch_encode(buf, col_format, &width);
	if (err) {
		return err;
	}

	err = sensor->series.get(sensor, ctx, col, values);
	if (err) {
		return err;
	}

	return sensor_value_encode(buf, sensor->type, values);
}

int sensor_column_decode(
	struct net_buf_simple *buf, const struct bt_mesh_sensor_type *type,
	struct bt_mesh_sensor_column *col,
	struct sensor_value value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX])
{
	const struct bt_mesh_sensor_format *col_format;
	int err;

	col_format = bt_mesh_sensor_column_format_get(type);
	err = sensor_ch_decode(buf, col_format, &col->start);
	if (err) {
		return err;
	}

	err = sensor_ch_decode(buf, col_format, &col->end);
	if (err) {
		return err;
	}

	return sensor_value_decode(buf, type, value);
}

u8_t sensor_value_len(const struct bt_mesh_sensor_type *type)
{
	u8_t sum = 0;

	for (int i = 0; i < type->channel_count; ++i) {
		sum += type->channels[i].format->size;
	}

	return sum;
}

/* Several timer values in sensors are encoded in the following scheme:
 *
 *     time = pow(1.1 seconds, encoded - 64)
 *
 * To keep the lookup table size and processing time down, this is implemented
 * as a lookup table of the raw value corresponding to every 16th encoded value
 * and a table of multiplicators. The values of the lookup table are
 * pow(1.1, (index * 16) - 64) nanoseconds. The values of the multiplicator
 * array are pow(1.1, index) * 100000. When multiplied by the values in
 * the lookup, the result is pow(1.1, encoded - 64), because of the following
 * property of powers:
 *
 *     pow(A, B + C) = pow(A, B) * pow(A, C)
 *
 * Where in our case, A is 1.1, B is floor((encoded-64), 16) and C is
 * (encoded % 16).
 */
static const u64_t powtime_lookup[] = {
	2243,	      10307,	    47362,	   217629,
	1000000,      4594972,	    21113776,	   97017233,
	445791568,    2048400214,   9412343651,	   43249464815,
	198730122503, 913159544478, 4195943439113, 19280246755010,
};

static const u32_t powtime_mul[] = {
	100000, 110000, 121000, 133100, 146410, 161051, 177156, 194871,
	214358, 235794, 259374, 285311, 313842, 345227, 379749, 417724,
};

u8_t sensor_powtime_encode(u64_t raw)
{
	if (raw == 0) {
		return 0;
	}

	/* Search through the lookup table to find the highest encoding lower
	 * than the raw value.
	 */
	u64_t raw_ns = raw * 1000;

	if (raw_ns < powtime_lookup[0]) {
		return 1;
	}

	const u64_t *seed = &powtime_lookup[0];

	for (int i = 1; i < ARRAY_SIZE(powtime_lookup); ++i) {
		if (raw_ns < powtime_lookup[i]) {
			seed = &powtime_lookup[i - 1];
			break;
		}
	}

	int i;

	for (i = 0; (i < ARRAY_SIZE(powtime_mul) &&
		     raw_ns >= (*seed * powtime_mul[i]) / 100000);
	     i++) {
	}

	return ARRAY_SIZE(powtime_mul) * (seed - &powtime_lookup[0]) + i - 1;
}

u64_t sensor_powtime_decode_ns(u8_t val)
{
	if (val == 0) {
		return 0;
	}

	return (powtime_lookup[val / ARRAY_SIZE(powtime_mul)] *
		powtime_mul[val % ARRAY_SIZE(powtime_mul)]) /
	       100000L;
}

u64_t sensor_powtime_decode(u8_t val)
{
	return sensor_powtime_decode_ns(val) / 1000L;
}

int sensor_cadence_encode(struct net_buf_simple *buf,
			  const struct bt_mesh_sensor_type *sensor_type,
			  u8_t fast_period_div, u8_t min_int,
			  const struct bt_mesh_sensor_threshold *threshold)
{
	net_buf_simple_add_u8(buf, ((!!threshold->delta.type) << 7) |
					   (BIT_MASK(7) & fast_period_div));

	const struct bt_mesh_sensor_format *delta_format =
		(threshold->delta.type == BT_MESH_SENSOR_DELTA_PERCENT) ?
			&bt_mesh_sensor_format_percentage_16 :
			sensor_type->channels[0].format;
	int err;

	err = sensor_ch_encode(buf, delta_format, &threshold->delta.down);
	if (err) {
		return err;
	}

	err = sensor_ch_encode(buf, delta_format, &threshold->delta.up);
	if (err) {
		return err;
	}

	net_buf_simple_add_u8(buf, min_int);

	/* Flip the order if the cadence is fast outside. */
	const struct sensor_value *first, *second;

	if (threshold->range.cadence == BT_MESH_SENSOR_CADENCE_FAST) {
		first = &threshold->range.low;
		second = &threshold->range.high;
	} else {
		first = &threshold->range.high;
		second = &threshold->range.low;
	}

	err = sensor_ch_encode(buf, sensor_type->channels[0].format, first);
	if (err) {
		return err;
	}

	return sensor_ch_encode(buf, sensor_type->channels[0].format, second);
}

int sensor_cadence_decode(struct net_buf_simple *buf,
			  const struct bt_mesh_sensor_type *sensor_type,
			  u8_t *fast_period_div, u8_t *min_int,
			  struct bt_mesh_sensor_threshold *threshold)
{
	const struct bt_mesh_sensor_format *delta_format;
	u8_t div_and_type;
	int err;

	div_and_type = net_buf_simple_pull_u8(buf);
	threshold->delta.type = div_and_type >> 7;
	*fast_period_div = div_and_type & BIT_MASK(7);
	if (*fast_period_div > BT_MESH_SENSOR_PERIOD_DIV_MAX) {
		return -EINVAL;
	}

	delta_format = (threshold->delta.type == BT_MESH_SENSOR_DELTA_PERCENT) ?
			       &bt_mesh_sensor_format_percentage_16 :
			       sensor_type->channels[0].format;

	err = sensor_ch_decode(buf, delta_format, &threshold->delta.down);
	if (err) {
		return err;
	}

	err = sensor_ch_decode(buf, delta_format, &threshold->delta.up);
	if (err) {
		return err;
	}

	*min_int = net_buf_simple_pull_u8(buf);
	if (*min_int > BT_MESH_SENSOR_INTERVAL_MAX) {
		return -EINVAL;
	}

	err = sensor_ch_decode(buf, sensor_type->channels[0].format,
			       &threshold->range.low);
	if (err) {
		return err;
	}

	err = sensor_ch_decode(buf, sensor_type->channels[0].format,
				&threshold->range.high);
	if (err) {
		return err;
	}

	/* According to the Bluetooth Mesh Model Specification v1.0.1, Section
	 * 4.1.3.6, if range.high is lower than range.low, the cadence is fast
	 * outside the range. Swap the two in this case.
	 */
	if (threshold->range.high.val1 < threshold->range.low.val1 ||
	    (threshold->range.high.val1 == threshold->range.low.val1 &&
	     threshold->range.high.val2 < threshold->range.low.val2)) {
		struct sensor_value temp;

		temp = threshold->range.high;
		threshold->range.high = threshold->range.low;
		threshold->range.low = temp;

		threshold->range.cadence = BT_MESH_SENSOR_CADENCE_NORMAL;
	} else {
		threshold->range.cadence = BT_MESH_SENSOR_CADENCE_FAST;
	}

	return 0;
}

u8_t sensor_pub_div_get(const struct bt_mesh_sensor *s, u32_t base_period)
{
	u8_t div = s->state.pub_div * s->state.fast_pub;

	while (div != 0 && (base_period >> div) < (1 << s->state.min_int)) {
		div--;
	}

	return div;
}

bool bt_mesh_sensor_value_in_column(const struct sensor_value *value,
				    const struct bt_mesh_sensor_column *col)
{
	return (value->val1 > col->start.val1 ||
		(value->val1 == col->start.val1 &&
		 value->val2 >= col->start.val2)) &&
	       (value->val1 < col->end.val1 ||
		(value->val1 == col->end.val1 && value->val2 <= col->end.val2));
}

void sensor_cadence_update(struct bt_mesh_sensor *sensor,
			   const struct sensor_value *value)
{
	enum bt_mesh_sensor_cadence new;

	new = sensor_cadence(&sensor->state.threshold, value);

	if (sensor->state.fast_pub != new) {
		BT_DBG("0x%04x new cadence: %s", sensor->type->id,
		       (new == BT_MESH_SENSOR_CADENCE_FAST) ? "fast" :
							      "normal");
	}

	sensor->state.fast_pub = new;
}

const char *bt_mesh_sensor_ch_str_real(const struct sensor_value *ch)
{
	static char str[BT_MESH_SENSOR_CH_STR_LEN];

	bt_mesh_sensor_ch_to_str(ch, str, sizeof(str));

	return str;
}
