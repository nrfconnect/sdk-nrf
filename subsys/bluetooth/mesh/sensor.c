/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "sensor.h"
#include <bluetooth/mesh/sensor_types.h>
#include <bluetooth/mesh/properties.h>
#include <stdlib.h>
#include <string.h>

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_mesh_sensor);

/** Scale away the sensor_value fraction, to allow integer math */
#define SENSOR_MILL(_val) ((1000000LL * (_val)->val1) + (_val)->val2)

#ifndef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
/** Delta threshold type. */
enum cadence_delta_type {
	/** Value based delta threshold.
	 *  The delta threshold values are represented as absolute value
	 *  changes.
	 */
	CADENCE_DELTA_TYPE_VALUE,
	/** Percent based delta threshold.
	 *  The delta threshold values are represented as percentages of their
	 *  old value (resolution: 0.01 %).
	 */
	CADENCE_DELTA_TYPE_PERCENT,
};
#endif

static enum bt_mesh_sensor_cadence
sensor_cadence(const struct bt_mesh_sensor_threshold *threshold,
	       const sensor_value_type *curr)
{
#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	int64_t high_mill = SENSOR_MILL(&threshold->range.high);
	int64_t low_mill = SENSOR_MILL(&threshold->range.low);

	if (high_mill == low_mill) {
		return BT_MESH_SENSOR_CADENCE_NORMAL;
	}

	int64_t curr_mill = SENSOR_MILL(curr);
	bool in_range = (curr_mill >= MIN(low_mill, high_mill) &&
			 curr_mill <= MAX(low_mill, high_mill));
#else
	const struct bt_mesh_sensor_value *high, *low;
	const struct bt_mesh_sensor_format *fmt = curr->format;

	if (fmt->cb->compare(&threshold->range.high,
			     &threshold->range.low) >= 0) {
		low = &threshold->range.low;
		high = &threshold->range.high;
	} else if (fmt->cb->compare(&threshold->range.low,
				    &threshold->range.high) >= 0) {
		low = &threshold->range.high;
		high = &threshold->range.low;
	} else {
		return BT_MESH_SENSOR_CADENCE_NORMAL;
	}

	bool in_range = BT_MESH_SENSOR_VALUE_IN_RANGE(curr, low, high);
#endif
	return in_range ? threshold->range.cadence : !threshold->range.cadence;
}

#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
bool bt_mesh_sensor_delta_threshold(const struct bt_mesh_sensor *sensor,
				    const struct sensor_value *curr)
{
	struct sensor_value delta = {
		curr->val1 - sensor->state.prev.val1,
		curr->val2 - sensor->state.prev.val2,
	};
	int64_t delta_mill = SENSOR_MILL(&delta);
	int64_t thrsh_mill;

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
		/* Store the Status Delta Trigger threshold in sensor_value first to avoid int64_t
		 * overflow caused by multiplication of values converted by the SENSOR_MILL() macro.
		 */
		struct sensor_value thrsh_delta = delta_mill >= 0 ?
			sensor->state.threshold.delta.up : sensor->state.threshold.delta.down;
		struct sensor_value thrsh = {
			.val1 = thrsh_delta.val1 * sensor->state.prev.val1,
			.val2 = thrsh_delta.val2 * sensor->state.prev.val2,
		};

		thrsh_mill = llabs(SENSOR_MILL(&thrsh)) / 100LL;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_MODEL_LOG_LEVEL_DBG)) {
		char delta_str[BT_MESH_SENSOR_CH_STR_LEN];
		char curr_str[BT_MESH_SENSOR_CH_STR_LEN];
		char prev_str[BT_MESH_SENSOR_CH_STR_LEN];
		char thrsh_str[BT_MESH_SENSOR_CH_STR_LEN];

		strcpy(delta_str, bt_mesh_sensor_ch_str(&delta));
		strcpy(curr_str, bt_mesh_sensor_ch_str(curr));
		strcpy(prev_str, bt_mesh_sensor_ch_str(&sensor->state.prev));
		strcpy(thrsh_str,
		       delta_mill < 0 ? bt_mesh_sensor_ch_str(&sensor->state.threshold.delta.down) :
					bt_mesh_sensor_ch_str(&sensor->state.threshold.delta.up));

		LOG_DBG("Delta: %s (%s - %s) thrsh: %s", delta_str, curr_str, prev_str, thrsh_str);
	}

	return (delta_mill > thrsh_mill);
}
#else /* !defined(CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE) */
bool bt_mesh_sensor_delta_threshold(const struct bt_mesh_sensor *sensor,
				    const struct bt_mesh_sensor_value *curr)
{
	if (!sensor->state.prev.format) {
		/* prev is uninitialized, return true as this is the first pub. */
		return true;
	}
	return curr->format->cb->delta_check(curr, &sensor->state.prev,
					     &sensor->state.threshold.deltas);
}
#endif /* defined(CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE) */

void bt_mesh_sensor_cadence_set(struct bt_mesh_sensor *sensor,
				enum bt_mesh_sensor_cadence cadence)
{
	sensor->state.fast_pub = (cadence == BT_MESH_SENSOR_CADENCE_FAST);
}

int sensor_status_id_encode(struct net_buf_simple *buf, uint8_t len, uint16_t id)
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

void sensor_status_id_decode(struct net_buf_simple *buf, uint8_t *len, uint16_t *id)
{
	uint8_t first = net_buf_simple_pull_u8(buf);

	if (first & BIT(0)) { /* long format */
		if (buf->len < 2) {
			*len = 0;
			*id = BT_MESH_PROP_ID_PROHIBITED;
			return;
		}

		*len = ((first >> 1) + 1) & 0x7f;
		*id = net_buf_simple_pull_le16(buf);
	} else if (buf->len < 1) {
		*len = 0;
		*id = BT_MESH_PROP_ID_PROHIBITED;
	} else {
		*len = ((first >> 1) & BIT_MASK(4)) + 1;
		*id = (first >> 5) | (net_buf_simple_pull_u8(buf) << 3);
	}
}

#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
static void tolerance_decode(uint16_t encoded, struct sensor_value *tolerance)
{
	uint32_t toll_mill = (encoded * 100ULL * 1000000ULL) / 4095ULL;

	tolerance->val1 = toll_mill / 1000000ULL;
	tolerance->val2 = toll_mill % 1000000ULL;
}
static uint16_t tolerance_encode(const struct sensor_value *tol)
{
	uint64_t tol_mill = 1000000ULL * tol->val1 + tol->val2;

	if (tol_mill > (1000000ULL * 100ULL)) {
		return 0;
	}
	return (tol_mill * 4095ULL + (1000000ULL * 50ULL)) / (1000000ULL * 100ULL);
}
#endif

void sensor_descriptor_decode(struct net_buf_simple *buf,
			struct bt_mesh_sensor_info *sensor)
{
	uint32_t tolerances;

	sensor->id = net_buf_simple_pull_le16(buf);
	tolerances = net_buf_simple_pull_le24(buf);
#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	tolerance_decode(tolerances & BIT_MASK(12),
			 &sensor->descriptor.tolerance.positive);
	tolerance_decode(tolerances >> 12,
			 &sensor->descriptor.tolerance.negative);
#else
	sensor->descriptor.tolerance.positive = tolerances & BIT_MASK(12);
	sensor->descriptor.tolerance.negative = tolerances >> 12;
#endif
	sensor->descriptor.sampling_type = net_buf_simple_pull_u8(buf);
	sensor->descriptor.period =
		sensor_powtime_decode(net_buf_simple_pull_u8(buf));
	sensor->descriptor.update_interval =
		sensor_powtime_decode(net_buf_simple_pull_u8(buf));
}

void sensor_descriptor_encode(struct net_buf_simple *buf,
				     struct bt_mesh_sensor *sensor)
{
	net_buf_simple_add_le16(buf, sensor->type->id);

	const struct bt_mesh_sensor_descriptor dummy = { 0 };
	const struct bt_mesh_sensor_descriptor *d =
		sensor->descriptor ? sensor->descriptor : &dummy;

#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	uint16_t tol_pos = tolerance_encode(&d->tolerance.positive);
	uint16_t tol_neg = tolerance_encode(&d->tolerance.negative);
#else
	uint16_t tol_pos = d->tolerance.positive;
	uint16_t tol_neg = d->tolerance.negative;
#endif

	net_buf_simple_add_u8(buf, tol_pos & 0xff);
	net_buf_simple_add_u8(buf,
			      ((tol_pos >> 8) & BIT_MASK(4)) | (tol_neg << 4));
	net_buf_simple_add_u8(buf, tol_neg >> 4);
	net_buf_simple_add_u8(buf, d->sampling_type);

	net_buf_simple_add_u8(buf, sensor_powtime_encode(d->period));
	net_buf_simple_add_u8(buf, sensor_powtime_encode(d->update_interval));
}

int sensor_value_encode(struct net_buf_simple *buf,
			const struct bt_mesh_sensor_type *type,
			const sensor_value_type *values)
{
	/* The API assumes that `values` array size is always CONFIG_BT_MESH_SENSOR_CHANNELS_MAX. */
	__ASSERT_NO_MSG(type->channel_count <= CONFIG_BT_MESH_SENSOR_CHANNELS_MAX);

	for (uint32_t i = 0; i < type->channel_count; ++i) {
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
			sensor_value_type *values)
{
	int err;

	/* The API assumes that `values` array size is always CONFIG_BT_MESH_SENSOR_CHANNELS_MAX. */
	__ASSERT_NO_MSG(type->channel_count <= CONFIG_BT_MESH_SENSOR_CHANNELS_MAX);

	for (uint32_t i = 0; i < type->channel_count; ++i) {
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
		     const sensor_value_type *value)
{
#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	return format->encode(format, value, buf);
#else
	if (format != value->format) {
		return -EINVAL;
	}

	if (net_buf_simple_tailroom(buf) < format->size) {
		return -ENOMEM;
	}

	net_buf_simple_add_mem(buf, value->raw, value->format->size);
	return 0;
#endif
}

int sensor_ch_decode(struct net_buf_simple *buf,
		     const struct bt_mesh_sensor_format *format,
		     sensor_value_type *value)
{
#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	return format->decode(format, buf, value);
#else
	if (buf->len < format->size) {
		return -ENOMEM;
	}
	value->format = format;
	memcpy(value->raw, net_buf_simple_pull_mem(buf, format->size),
	       format->size);
	return 0;
#endif
}

int sensor_status_encode(struct net_buf_simple *buf,
			 const struct bt_mesh_sensor *sensor,
			 const sensor_value_type *values)
{
	const struct bt_mesh_sensor_type *type = sensor->type;
	size_t size = 0;
	int err;

	for (uint32_t i = 0; i < type->channel_count; ++i) {
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
	if (type->channel_count > 2) {
		return type->channels[1].format;
	}

	return NULL;
}

int sensor_column_value_encode(struct net_buf_simple *buf,
			       struct bt_mesh_sensor_srv *srv,
			       struct bt_mesh_sensor *sensor,
			       struct bt_mesh_msg_ctx *ctx,
			       uint32_t column_index)
{
	sensor_value_type values[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];
	int err = sensor->series.get(srv, sensor, ctx, column_index, values);

	if (err) {
		return err;
	}

	return sensor_value_encode(buf, sensor->type, values);
}

int sensor_column_encode(struct net_buf_simple *buf,
			 struct bt_mesh_sensor_srv *srv,
			 struct bt_mesh_sensor *sensor,
			 struct bt_mesh_msg_ctx *ctx,
			 const struct bt_mesh_sensor_column *col)
{
	int col_index = col - sensor->series.columns;

	const struct bt_mesh_sensor_format *col_format =
		bt_mesh_sensor_column_format_get(sensor->type);

	if (!col_format) {
		return -ENOTSUP;
	}

#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	const uint64_t width_million =
		(col->end.val1 - col->start.val1) * 1000000ULL +
		(col->end.val2 - col->start.val2);
	const struct sensor_value width = {
		.val1 = width_million / 1000000ULL,
		.val2 = width_million % 1000000ULL,
	};

	LOG_DBG("Column width: %s", bt_mesh_sensor_ch_str(&width));
#endif

	int err;

	err = sensor_ch_encode(buf, col_format, &col->start);
	if (err) {
		return err;
	}

#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	/* The sensor columns are transmitted as start+width, not start+end: */
	err = sensor_ch_encode(buf, col_format, &width);
#else
	err = sensor_ch_encode(buf, col_format, &col->width);
#endif
	if (err) {
		return err;
	}

	return sensor_column_value_encode(buf, srv, sensor, ctx, col_index);
}

int sensor_column_decode(
	struct net_buf_simple *buf, const struct bt_mesh_sensor_type *type,
	struct bt_mesh_sensor_column *col,
	sensor_value_type value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX])
{
	const struct bt_mesh_sensor_format *col_format;
	int err;

	col_format = bt_mesh_sensor_column_format_get(type);
	if (!col_format) {
		return -ENOTSUP;
	}

	err = sensor_ch_decode(buf, col_format, &col->start);
	if (err) {
		return err;
	}

	if (buf->len == 0) {
		LOG_WRN("The requested column didn't exist: %s",
			bt_mesh_sensor_ch_str(&col->start));
		return -ENOENT;
	}

#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	struct sensor_value width;

	err = sensor_ch_decode(buf, col_format, &width);
	if (err) {
		return err;
	}

	uint64_t end_mill = (col->start.val1 + width.val1) * 1000000ULL +
			 (col->start.val2 + width.val2);

	col->end.val1 = end_mill / 1000000ULL;
	col->end.val2 = end_mill % 1000000ULL;
#else
	err = sensor_ch_decode(buf, col_format, &col->width);
	if (err) {
		return err;
	}
#endif

	return sensor_value_decode(buf, type, value);
}

uint8_t sensor_value_len(const struct bt_mesh_sensor_type *type)
{
	uint8_t sum = 0;

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
static const uint64_t powtime_lookup[] = {
	2243,	      10307,	    47362,	   217629,
	1000000,      4594972,	    21113776,	   97017233,
	445791568,    2048400214,   9412343651,	   43249464815,
	198730122503, 913159544478, 4195943439113, 19280246755010,
};

static const uint32_t powtime_mul[] = {
	100000, 110000, 121000, 133100, 146410, 161051, 177156, 194871,
	214358, 235794, 259374, 285311, 313842, 345227, 379749, 417724,
};

uint8_t sensor_powtime_encode(uint64_t raw)
{
	if (raw == 0) {
		return 0;
	}

	/* Search through the lookup table to find the highest encoding lower
	 * than the raw value.
	 */
	uint64_t raw_us = raw * USEC_PER_MSEC;

	if (raw_us < powtime_lookup[0]) {
		return 1;
	}

	const uint64_t *seed = &powtime_lookup[ARRAY_SIZE(powtime_lookup) - 1];

	for (int i = 1; i < ARRAY_SIZE(powtime_lookup); ++i) {
		if (raw_us < powtime_lookup[i]) {
			seed = &powtime_lookup[i - 1];
			break;
		}
	}

	int i;

	for (i = 0; (i < ARRAY_SIZE(powtime_mul) &&
		     raw_us > (*seed * powtime_mul[i]) / 100000);
	     i++) {
	}

	uint8_t encoded = ARRAY_SIZE(powtime_mul) * (seed - &powtime_lookup[0]) + i;

	return MIN(encoded, 253);
}

uint64_t sensor_powtime_decode_us(uint8_t val)
{
	if (val == 0) {
		return 0;
	}

	return (powtime_lookup[val / ARRAY_SIZE(powtime_mul)] *
		powtime_mul[val % ARRAY_SIZE(powtime_mul)]) /
	       100000L;
}

uint64_t sensor_powtime_decode(uint8_t val)
{
	return sensor_powtime_decode_us(val) / USEC_PER_MSEC;
}

int sensor_cadence_encode(struct net_buf_simple *buf,
			  const struct bt_mesh_sensor_type *sensor_type,
			  uint8_t fast_period_div, uint8_t min_int,
			  const struct bt_mesh_sensor_threshold *threshold)
{
#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	const struct bt_mesh_sensor_format *delta_format =
		(threshold->delta.type == BT_MESH_SENSOR_DELTA_PERCENT) ?
			&bt_mesh_sensor_format_percentage_delta_trigger :
			sensor_type->channels[0].format;

	enum bt_mesh_sensor_delta delta_type = !!threshold->delta.type;
#else
	const struct bt_mesh_sensor_format *delta_format = threshold->deltas.down.format;

	enum cadence_delta_type delta_type =
		delta_format == &bt_mesh_sensor_format_percentage_delta_trigger ?
		CADENCE_DELTA_TYPE_PERCENT : CADENCE_DELTA_TYPE_VALUE;
#endif
	net_buf_simple_add_u8(buf, (delta_type << 7) |
				   (BIT_MASK(7) & fast_period_div));

	int err;

#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	err = sensor_ch_encode(buf, delta_format, &threshold->delta.down);
#else
	err = sensor_ch_encode(buf, delta_format, &threshold->deltas.down);
#endif
	if (err) {
		return err;
	}

#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	err = sensor_ch_encode(buf, delta_format, &threshold->delta.up);
#else
	err = sensor_ch_encode(buf, delta_format, &threshold->deltas.up);
#endif
	if (err) {
		return err;
	}

	if (min_int > BT_MESH_SENSOR_INTERVAL_MAX) {
		return -EINVAL;
	}

	net_buf_simple_add_u8(buf, min_int);

	/* Flip the order if the cadence is fast outside. */
	const sensor_value_type *first, *second;

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
			  uint8_t *fast_period_div, uint8_t *min_int,
			  struct bt_mesh_sensor_threshold *threshold)
{
	const struct bt_mesh_sensor_format *delta_format;
#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	enum bt_mesh_sensor_delta delta_type;
#else
	enum cadence_delta_type delta_type;
#endif
	uint8_t div_and_type;
	int err;

	div_and_type = net_buf_simple_pull_u8(buf);
	delta_type = div_and_type >> 7;
#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	threshold->delta.type = delta_type;
	delta_format = (delta_type == BT_MESH_SENSOR_DELTA_PERCENT) ?
			&bt_mesh_sensor_format_percentage_delta_trigger :
			sensor_type->channels[0].format;
#else
	delta_format = (delta_type == CADENCE_DELTA_TYPE_PERCENT) ?
			&bt_mesh_sensor_format_percentage_delta_trigger :
			sensor_type->channels[0].format;
#endif
	*fast_period_div = div_and_type & BIT_MASK(7);
	if (*fast_period_div > BT_MESH_SENSOR_PERIOD_DIV_MAX) {
		return -EINVAL;
	}

#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	err = sensor_ch_decode(buf, delta_format, &threshold->delta.down);
#else
	err = sensor_ch_decode(buf, delta_format, &threshold->deltas.down);
#endif
	if (err) {
		return err;
	}

#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	err = sensor_ch_decode(buf, delta_format, &threshold->delta.up);
#else
	err = sensor_ch_decode(buf, delta_format, &threshold->deltas.up);
#endif
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

	/* According to MshMDLv1.1: 4.1.3.6, if range.high is lower than range.low, the cadence is
	 * fast outside the range. Swap the two in this case.
	 */
#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
	if (threshold->range.high.val1 < threshold->range.low.val1 ||
	    (threshold->range.high.val1 == threshold->range.low.val1 &&
	     threshold->range.high.val2 < threshold->range.low.val2)) {
		struct sensor_value temp;

		temp = threshold->range.high;
		threshold->range.high = threshold->range.low;
		threshold->range.low = temp;
#else
	if (threshold->range.high.format->cb->compare(&threshold->range.low,
						      &threshold->range.high) > 0) {
		uint8_t temp[CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX];

		memcpy(temp, &threshold->range.high.raw,
		       CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX);
		memcpy(&threshold->range.high.raw, &threshold->range.low.raw,
		       CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX);
		memcpy(&threshold->range.low.raw, temp,
		       CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX);
#endif
		threshold->range.cadence = BT_MESH_SENSOR_CADENCE_NORMAL;
	} else {
		threshold->range.cadence = BT_MESH_SENSOR_CADENCE_FAST;
	}

	return 0;
}

uint8_t sensor_pub_div_get(const struct bt_mesh_sensor *s, uint32_t base_period)
{
	uint8_t div = s->state.pub_div * s->state.fast_pub;

	while (div != 0 && (base_period >> div) < (1 << s->state.min_int)) {
		div--;
	}

	return div;
}

#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
bool bt_mesh_sensor_value_in_column(const struct sensor_value *value,
				    const struct bt_mesh_sensor_column *col)
{
	return (value->val1 > col->start.val1 ||
		(value->val1 == col->start.val1 &&
		 value->val2 >= col->start.val2)) &&
	       (value->val1 < col->end.val1 ||
		(value->val1 == col->end.val1 && value->val2 <= col->end.val2));
}
#else
bool bt_mesh_sensor_value_in_column(const struct bt_mesh_sensor_value *value,
				    const struct bt_mesh_sensor_column *col)
{
	if (value->format == col->start.format &&
	    value->format == col->width.format &&
	    value->format->cb->value_in_column) {
		return value->format->cb->value_in_column(value, col);
	}

	if (bt_mesh_sensor_value_compare(&col->start, value) > 0) {
		return false;
	}

	/* Fall back to comparing end using float */
	float widthf, startf, valuef;
	enum bt_mesh_sensor_value_status status;

	status = bt_mesh_sensor_value_to_float(&col->width, &widthf);
	if (!bt_mesh_sensor_value_status_is_numeric(status)) {
		return false;
	}

	status = bt_mesh_sensor_value_to_float(&col->start, &startf);
	if (!bt_mesh_sensor_value_status_is_numeric(status)) {
		return false;
	}

	status = bt_mesh_sensor_value_to_float(value, &valuef);
	if (!bt_mesh_sensor_value_status_is_numeric(status)) {
		return false;
	}

	return valuef <= (startf + widthf);
}
#endif

void sensor_cadence_update(struct bt_mesh_sensor *sensor,
			   const sensor_value_type *value)
{
	if (!sensor->state.configured) {
		/* Ignore any update before cadence is configured. */
		return;
	}

	enum bt_mesh_sensor_cadence new;

	new = sensor_cadence(&sensor->state.threshold, value);

	/** Use Fast Cadence Period Divisor when publishing when the change exceeds the delta,
	 * MshMDLv1.1: 4.3.1.2.4.3, E15551 and E15886.
	 */
	if (new == BT_MESH_SENSOR_CADENCE_NORMAL) {
		new = bt_mesh_sensor_delta_threshold(sensor, value) ?
			BT_MESH_SENSOR_CADENCE_FAST : BT_MESH_SENSOR_CADENCE_NORMAL;
	}

	if (sensor->state.fast_pub != new) {
		LOG_DBG("0x%04x new cadence: %s", sensor->type->id,
		       (new == BT_MESH_SENSOR_CADENCE_FAST) ? "fast" :
							      "normal");
	}

	sensor->state.fast_pub = new;
}

#ifdef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
const char *bt_mesh_sensor_ch_str(const struct sensor_value *ch)
#else
const char *bt_mesh_sensor_ch_str(const struct bt_mesh_sensor_value *ch)
#endif
{
	static char str[BT_MESH_SENSOR_CH_STR_LEN];

	if (bt_mesh_sensor_ch_to_str(ch, str, BT_MESH_SENSOR_CH_STR_LEN) <= 0) {
		strcpy(str, "<unprintable value>");
	}

	return str;
}

#ifndef CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE
int bt_mesh_sensor_ch_to_str(const struct bt_mesh_sensor_value *ch, char *str,
			     size_t len)
{
	enum bt_mesh_sensor_value_status status;
	float float_val;

	if (ch->format->cb->to_string) {
		return ch->format->cb->to_string(ch, str, len);
	}

	status = ch->format->cb->to_float(ch, &float_val);

	switch (status) {
	case BT_MESH_SENSOR_VALUE_NUMBER:
		return snprintk(str, len, "%g", (double)float_val);
	case BT_MESH_SENSOR_VALUE_MAX_OR_GREATER:
		return snprintk(str, len, ">=%g", (double)float_val);
	case BT_MESH_SENSOR_VALUE_MIN_OR_LESS:
		return snprintk(str, len, "<=%g", (double)float_val);
	case BT_MESH_SENSOR_VALUE_UNKNOWN:
		return snprintk(str, len, "%s", "<value is not known>");
	case BT_MESH_SENSOR_VALUE_INVALID:
		return snprintk(str, len, "%s", "<value is invalid>");
	case BT_MESH_SENSOR_VALUE_TOTAL_DEVICE_LIFE:
		return snprintk(str, len, "%s", "<total life of device>");
	default:
		return snprintk(str, len, "%s", "<unprintable value>");
	}
}

int bt_mesh_sensor_value_compare(const struct bt_mesh_sensor_value *a,
				 const struct bt_mesh_sensor_value *b)
{
	if (a->format == b->format) {
		return a->format->cb->compare(a, b);
	}

	/* Fall back to comparing floats. */
	float a_float, b_float;
	enum bt_mesh_sensor_value_status status;

	status = bt_mesh_sensor_value_to_float(a, &a_float);
	if (!bt_mesh_sensor_value_status_is_numeric(status)) {
		return -1;
	}

	status = bt_mesh_sensor_value_to_float(b, &b_float);
	if (!bt_mesh_sensor_value_status_is_numeric(status)) {
		return -1;
	}

	return a_float == b_float ? 0 : (a_float > b_float ? 1 : -1);
}

enum bt_mesh_sensor_value_status
bt_mesh_sensor_value_to_float(const struct bt_mesh_sensor_value *sensor_val,
			      float *val)
{
	if (!sensor_val->format) {
		return BT_MESH_SENSOR_VALUE_CONVERSION_ERROR;
	}

	return sensor_val->format->cb->to_float(sensor_val, val);
}

int bt_mesh_sensor_value_from_float(const struct bt_mesh_sensor_format *format,
				    float val,
				    struct bt_mesh_sensor_value *sensor_val)
{
	return format->cb->from_float(format, val, sensor_val);
}

enum bt_mesh_sensor_value_status
bt_mesh_sensor_value_to_micro(
	const struct bt_mesh_sensor_value *sensor_val,
	int64_t *val)
{
	if (!sensor_val->format) {
		return BT_MESH_SENSOR_VALUE_CONVERSION_ERROR;
	}

	if (sensor_val->format->cb->to_micro) {
		return sensor_val->format->cb->to_micro(sensor_val, val);
	}

	enum bt_mesh_sensor_value_status status;
	float f;

	status = sensor_val->format->cb->to_float(sensor_val, &f);
	if (bt_mesh_sensor_value_status_is_numeric(status)) {
		float micro_f = f * 1000000.0f;

		/* Clamp to max range for int64_t.
		 * (Can't use CLAMP here because it uses ternary which
		 * implicitly converts the return type to float which is then
		 * converted back again, causing overflow)
		 */
		if (micro_f >= INT64_MAX) {
			*val = INT64_MAX;
		} else if (micro_f <= INT64_MIN) {
			*val = INT64_MIN;
		} else {
			*val = micro_f;
		}

		if (micro_f > INT64_MAX || micro_f < INT64_MIN) {
			return BT_MESH_SENSOR_VALUE_CLAMPED;
		}
	}
	return status;
}

int bt_mesh_sensor_value_from_micro(
	const struct bt_mesh_sensor_format *format,
	int64_t val,
	struct bt_mesh_sensor_value *sensor_val)
{
	if (format->cb->from_micro) {
		return format->cb->from_micro(format, val, sensor_val);
	}

	return format->cb->from_float(format, val / 1000000.0f, sensor_val);
}

enum bt_mesh_sensor_value_status
bt_mesh_sensor_value_to_sensor_value(
	const struct bt_mesh_sensor_value *sensor_val,
	struct sensor_value *val)
{
	enum bt_mesh_sensor_value_status status;
	int64_t micro;

	status = bt_mesh_sensor_value_to_micro(sensor_val, &micro);
	if (bt_mesh_sensor_value_status_is_numeric(status)) {
		int64_t val1 = micro / 1000000;

		if (val1 > INT32_MAX) {
			val->val1 = INT32_MAX;
			val->val2 = 999999;
			return BT_MESH_SENSOR_VALUE_CLAMPED;
		} else if (val1 < INT32_MIN) {
			val->val1 = INT32_MIN;
			val->val2 = -999999;
			return BT_MESH_SENSOR_VALUE_CLAMPED;
		}
		val->val1 = val1;
		val->val2 = micro % 1000000;
	}
	return status;
}

int bt_mesh_sensor_value_from_sensor_value(
	const struct bt_mesh_sensor_format *format,
	const struct sensor_value *val,
	struct bt_mesh_sensor_value *sensor_val)
{
	return bt_mesh_sensor_value_from_micro(
		format, sensor_value_to_micro(val), sensor_val);
}

enum bt_mesh_sensor_value_status
bt_mesh_sensor_value_get_status(const struct bt_mesh_sensor_value *sensor_val)
{
	return bt_mesh_sensor_value_to_float(sensor_val, NULL);
}

int bt_mesh_sensor_value_from_special_status(
	const struct bt_mesh_sensor_format *format,
	enum bt_mesh_sensor_value_status status,
	struct bt_mesh_sensor_value *sensor_val)
{
	if (status == BT_MESH_SENSOR_VALUE_NUMBER ||
	    status == BT_MESH_SENSOR_VALUE_CONVERSION_ERROR ||
	    !format->cb->from_special_status) {
		return -EINVAL;
	}
	return format->cb->from_special_status(format, status, sensor_val);
}
#endif /* defined(CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE) */
