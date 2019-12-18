/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <string.h>
#include <stdlib.h>
#include <bluetooth/mesh/sensor_srv.h>
#include <bluetooth/mesh/properties.h>
#include "mesh/net.h"
#include "mesh/access.h"
#include "mesh/transport.h"
#include "model_utils.h"
#include "sensor.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)
#define LOG_MODULE_NAME bt_mesh_sensor_srv
#include "common/log.h"

#define SENSOR_FOR_EACH(_list, _node)                                          \
	SYS_SLIST_FOR_EACH_CONTAINER(_list, _node, state.node)

static struct bt_mesh_sensor *sensor_get(struct bt_mesh_sensor_srv *srv,
					 u16_t id)
{
	struct bt_mesh_sensor *sensor;

	SENSOR_FOR_EACH(&srv->sensors, sensor)
	{
		if (sensor->type->id == id) {
			return sensor;
		}
	}

	return NULL;
}

static u16_t tolerance_encode(const struct sensor_value *tol)
{
	u64_t tol_mill = 1000000L * tol->val1 + tol->val2;

	if (tol_mill > (1000000L * 100L)) {
		return 0;
	}

	return (tol_mill * 4095L) / (1000000L * 100L);
}

static void cadence_store(const struct bt_mesh_sensor_srv *srv)
{
	/* Cadence is stored as a sequence of cadence status messages */
	NET_BUF_SIMPLE_DEFINE(buf, (CONFIG_BT_MESH_SENSOR_SRV_SENSORS_MAX *
				    BT_MESH_SENSOR_MSG_MAXLEN_CADENCE_STATUS));

	for (int i = 0; i < srv->sensor_count; ++i) {
		const struct bt_mesh_sensor *s = srv->sensor_array[i];
		int err;

		net_buf_simple_add_le16(&buf, s->type->id);
		err = sensor_cadence_encode(&buf, s->type, s->state.pub_div,
					    s->state.min_int,
					    &s->state.threshold);
		if (err) {
			return;
		}
	}

	if (bt_mesh_model_data_store(srv->model, false, buf.data, buf.len)) {
		BT_ERR("Sensor server data store failed");
	}
}

static void sensor_descriptor_encode(struct net_buf_simple *buf,
				     struct bt_mesh_sensor *sensor)
{
	net_buf_simple_add_le16(buf, sensor->type->id);

	const struct bt_mesh_sensor_descriptor dummy = { 0 };
	const struct bt_mesh_sensor_descriptor *d =
		sensor->descriptor ? sensor->descriptor : &dummy;

	u16_t tol_pos = tolerance_encode(&d->tolerance.positive);
	u16_t tol_neg = tolerance_encode(&d->tolerance.negative);

	net_buf_simple_add_u8(buf, tol_pos & 0xff);
	net_buf_simple_add_u8(buf,
			      ((tol_pos >> 8) & BIT_MASK(4)) | (tol_neg << 4));
	net_buf_simple_add_u8(buf, tol_neg >> 4);
	net_buf_simple_add_u8(buf, d->sampling_type);

	net_buf_simple_add_u8(buf, sensor_powtime_encode(d->period));
	net_buf_simple_add_u8(buf, sensor_powtime_encode(d->update_interval));
}

static int value_get(struct bt_mesh_sensor *sensor, struct bt_mesh_msg_ctx *ctx,
		     struct sensor_value *value)
{
	int err;

	if (!sensor->get) {
		return -ENOTSUP;
	}

	err = sensor->get(sensor, ctx, value);
	if (err) {
		BT_WARN("Value get for 0x%04x: %d", sensor->type->id, err);
		return err;
	}

	sensor_cadence_update(sensor, value);

	return 0;
}

static int buf_status_add(struct bt_mesh_sensor *sensor,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct sensor_value value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];
	int err;

	err = value_get(sensor, ctx, value);
	if (err) {
		sensor_status_id_encode(buf, 0, sensor->type->id);
		return err;
	}

	err = sensor_status_encode(buf, sensor, value);
	if (err) {
		BT_WARN("Sensor value encode for 0x%04x: %d", sensor->type->id,
			err);
		sensor_status_id_encode(buf, 0, sensor->type->id);
	}

	return err;
}

static void handle_descriptor_get(struct bt_mesh_model *mod,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_srv *srv = mod->user_data;

	if (buf->len != 0 && buf->len != 2) {
		return;
	}

	NET_BUF_SIMPLE_DEFINE(rsp, BT_MESH_TX_SDU_MAX);
	bt_mesh_model_msg_init(&rsp, BT_MESH_SENSOR_OP_DESCRIPTOR_STATUS);

	struct bt_mesh_sensor *sensor;

	if (buf->len == 2) {
		u16_t id = net_buf_simple_pull_le16(buf);

		if (id == BT_MESH_PROP_ID_PROHIBITED) {
			return;
		}

		sensor = sensor_get(srv, id);
		if (sensor) {
			sensor_descriptor_encode(&rsp, sensor);
		} else {
			net_buf_simple_add_le16(&rsp, id);
		}

		goto respond;
	}

	SENSOR_FOR_EACH(&srv->sensors, sensor) {
		BT_DBG("Reporting ID 0x%04x", sensor->type->id);

		if (net_buf_simple_tailroom(&rsp) < (8 + BT_MESH_MIC_SHORT)) {
			BT_WARN("Not enough room for all descriptors");
			break;
		}

		sensor_descriptor_encode(&rsp, sensor);
	}

respond:
	bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
}

static void handle_get(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_srv *srv = mod->user_data;

	if (buf->len != 0 && buf->len != 2) {
		return;
	}

	NET_BUF_SIMPLE_DEFINE(rsp, BT_MESH_TX_SDU_MAX);
	bt_mesh_model_msg_init(&rsp, BT_MESH_SENSOR_OP_STATUS);

	struct bt_mesh_sensor *sensor;

	if (buf->len == 2) {
		u16_t id = net_buf_simple_pull_le16(buf);

		if (id == BT_MESH_PROP_ID_PROHIBITED) {
			return;
		}

		sensor = sensor_get(srv, id);
		if (sensor) {
			buf_status_add(sensor, ctx, &rsp);
		} else {
			BT_WARN("Unknown sensor ID 0x%04x", id);
			sensor_status_id_encode(&rsp, 0, id);
		}

		goto respond;
	}

	SENSOR_FOR_EACH(&srv->sensors, sensor) {
		buf_status_add(sensor, ctx, &rsp);
	}

respond:
	bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
}

static const struct bt_mesh_sensor_column *
column_get(const struct bt_mesh_sensor_series *series,
	   const struct sensor_value *val)
{
	for (u32_t i = 0; i < series->column_count; ++i) {
		if (series->columns[i].start.val1 == val->val1 &&
		    series->columns[i].start.val2 == val->val2) {
			return &series->columns[i];
		}
	}

	return NULL;
}

static void handle_column_get(struct bt_mesh_model *mod,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_srv *srv = mod->user_data;
	int err;

	if (buf->len < 2) {
		return;
	}

	u16_t id = net_buf_simple_pull_le16(buf);

	if (id == BT_MESH_PROP_ID_PROHIBITED) {
		return;
	}

	struct bt_mesh_sensor *sensor = sensor_get(srv, id);

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_SENSOR_OP_COLUMN_STATUS,
				 BT_MESH_SENSOR_MSG_MAXLEN_COLUMN_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_SENSOR_OP_COLUMN_STATUS);
	net_buf_simple_add_le16(&rsp, id);

	if (!sensor) {
		goto respond;
	}

	const struct bt_mesh_sensor_format *col_format;
	const struct bt_mesh_sensor_column *col;
	struct sensor_value col_x;

	col_format = bt_mesh_sensor_column_format_get(sensor->type);
	if (!col_format || !sensor->series.columns || !sensor->series.get) {
		BT_WARN("No series support in 0x%04x", sensor->type->id);
		goto respond;
	}

	err = sensor_ch_decode(buf, col_format, &col_x);
	if (err) {
		return;
	}

	BT_DBG("Column %s", bt_mesh_sensor_ch_str(&col_x));

	col = column_get(&sensor->series, &col_x);
	if (!col) {
		BT_WARN("Unknown column");
		sensor_ch_encode(&rsp, col_format, &col_x);
		goto respond;
	}

	err = sensor_column_encode(&rsp, sensor, ctx, col);
	if (err) {
		BT_WARN("Failed encoding sensor column: %d", err);
		return;
	}

respond:
	bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
}

static void handle_series_get(struct bt_mesh_model *mod,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_srv *srv = mod->user_data;
	const struct bt_mesh_sensor_format *col_format;

	if (buf->len < 2) {
		return;
	}

	u16_t id = net_buf_simple_pull_le16(buf);

	if (id == BT_MESH_PROP_ID_PROHIBITED) {
		return;
	}

	struct bt_mesh_sensor *sensor = sensor_get(srv, id);

	NET_BUF_SIMPLE_DEFINE(rsp, BT_MESH_TX_SDU_MAX);
	bt_mesh_model_msg_init(&rsp, BT_MESH_SENSOR_OP_SERIES_STATUS);
	net_buf_simple_add_le16(&rsp, id);

	if (!sensor) {
		goto respond;
	}

	col_format = bt_mesh_sensor_column_format_get(sensor->type);
	if (!col_format || !sensor->series.columns || !sensor->series.get) {
		BT_WARN("No series support in 0x%04x", sensor->type->id);
		goto respond;
	}

	struct bt_mesh_sensor_column range;
	bool ranged = (buf->len != 0);

	if (buf->len == col_format->size * 2) {
		int err;

		err = sensor_ch_decode(buf, col_format, &range.start);
		if (err) {
			BT_WARN("Range start decode failed: %d", err);
			return;
		}

		err = sensor_ch_decode(buf, col_format, &range.end);
		if (err) {
			BT_WARN("Range end decode failed: %d", err);
			return;
		}
	} else if (buf->len != 0) {
		/* invalid length */
		BT_WARN("Invalid length (%u)", buf->len);
		return;
	}

	for (u32_t i = 0; i < sensor->series.column_count; ++i) {
		const struct bt_mesh_sensor_column *col =
			&sensor->series.columns[i];

		if (ranged &&
		    !bt_mesh_sensor_value_in_column(&col->start, &range)) {
			continue;
		}

		BT_DBG("Column #%u", i);

		int err = sensor_column_encode(&rsp, sensor, ctx, col);

		if (err) {
			BT_WARN("Failed encoding: %d", err);
			return;
		}
	}

respond:
	bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
}

const struct bt_mesh_model_op _bt_mesh_sensor_srv_op[] = {
	{ BT_MESH_SENSOR_OP_DESCRIPTOR_GET,
	  BT_MESH_SENSOR_MSG_MINLEN_DESCRIPTOR_GET, handle_descriptor_get },
	{ BT_MESH_SENSOR_OP_GET, BT_MESH_SENSOR_MSG_MINLEN_GET, handle_get },
	{ BT_MESH_SENSOR_OP_COLUMN_GET, BT_MESH_SENSOR_MSG_MINLEN_COLUMN_GET,
	  handle_column_get },
	{ BT_MESH_SENSOR_OP_SERIES_GET, BT_MESH_SENSOR_MSG_MINLEN_SERIES_GET,
	  handle_series_get },
};

static void handle_cadence_get(struct bt_mesh_model *mod,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_srv *srv = mod->user_data;
	struct bt_mesh_sensor *sensor;
	u16_t id;
	int err;

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_SENSOR_OP_CADENCE_STATUS,
				 BT_MESH_SENSOR_MSG_MAXLEN_CADENCE_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_SENSOR_OP_CADENCE_STATUS);

	id = net_buf_simple_pull_le16(buf);
	if (id == BT_MESH_PROP_ID_PROHIBITED) {
		return;
	}

	net_buf_simple_add_le16(&rsp, id);

	sensor = sensor_get(srv, id);
	if (!sensor || sensor->type->channel_count != 1) {
		BT_WARN("Cadence not supported");
		goto respond;
	}

	err = sensor_cadence_encode(&rsp, sensor->type, sensor->state.pub_div,
				    sensor->state.min_int,
				    &sensor->state.threshold);
	if (err) {
		return;
	}

respond:
	bt_mesh_model_send(srv->model, ctx, &rsp, NULL, NULL);
}

static void cadence_set(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf, bool ack)
{
	struct bt_mesh_sensor_srv *srv = mod->user_data;
	struct bt_mesh_sensor *sensor;
	u16_t id;

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_SENSOR_OP_CADENCE_STATUS,
				 BT_MESH_SENSOR_MSG_MAXLEN_CADENCE_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_SENSOR_OP_CADENCE_STATUS);

	id = net_buf_simple_pull_le16(buf);
	if (id == BT_MESH_PROP_ID_PROHIBITED) {
		return;
	}

	net_buf_simple_add_le16(&rsp, id);

	sensor = sensor_get(srv, id);
	if (!sensor || sensor->type->channel_count != 1) {
		BT_WARN("Cadence not supported");
		goto respond;
	}

	struct bt_mesh_sensor_threshold threshold;
	u8_t period_div, min_int;
	int err;

	err = sensor_cadence_decode(buf, sensor->type, &period_div, &min_int,
				    &threshold);
	if (err) {
		BT_WARN("Invalid cadence");
		goto respond;
	}

	BT_DBG("Min int: %u div: %u "
	       "delta: %s to %s "
	       "range: %s to %s (%s)",
	       min_int, period_div,
	       bt_mesh_sensor_ch_str(&threshold.delta.down),
	       bt_mesh_sensor_ch_str(&threshold.delta.up),
	       bt_mesh_sensor_ch_str(&threshold.range.low),
	       bt_mesh_sensor_ch_str(&threshold.range.high),
	       (threshold.range.cadence == BT_MESH_SENSOR_CADENCE_FAST ?
			"fast" :
			"slow"));

	sensor->state.min_int = min_int;
	sensor->state.pub_div = period_div;
	sensor->state.threshold = threshold;

	cadence_store(srv);

	err = sensor_cadence_encode(&rsp, sensor->type, sensor->state.pub_div,
				    sensor->state.min_int,
				    &sensor->state.threshold);
	if (err) {
		return;
	}

	model_send(mod, NULL, &rsp);

respond:
	if (ack) {
		bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
	}
}

static void handle_cadence_set(struct bt_mesh_model *mod,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	cadence_set(mod, ctx, buf, true);
}

static void handle_cadence_set_unack(struct bt_mesh_model *mod,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	cadence_set(mod, ctx, buf, false);
}

static void handle_settings_get(struct bt_mesh_model *mod,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_srv *srv = mod->user_data;

	u16_t id = net_buf_simple_pull_le16(buf);

	if (id == BT_MESH_PROP_ID_PROHIBITED) {
		return;
	}

	BT_DBG("0x%04x", id);

	BT_MESH_MODEL_BUF_DEFINE(
		rsp, BT_MESH_SENSOR_OP_SETTINGS_STATUS,
		2 + 2 * CONFIG_BT_MESH_SENSOR_SRV_SETTINGS_MAX);
	bt_mesh_model_msg_init(&rsp, BT_MESH_SENSOR_OP_SETTINGS_STATUS);
	net_buf_simple_add_le16(&rsp, id);

	struct bt_mesh_sensor *sensor = sensor_get(srv, id);

	if (!sensor) {
		goto respond;
	}

	for (u32_t i = 0; i < MIN(CONFIG_BT_MESH_SENSOR_SRV_SETTINGS_MAX,
				  sensor->settings.count);
	     ++i) {
		net_buf_simple_add_le16(&rsp,
					sensor->settings.list[i].type->id);
	}

respond:
	bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
}

static const struct bt_mesh_sensor_setting *
setting_get(struct bt_mesh_sensor *sensor, u16_t setting_id)
{
	for (u32_t i = 0; i < sensor->settings.count; i++) {
		if (sensor->settings.list[i].type->id == setting_id) {
			return &sensor->settings.list[i];
		}
	}
	return NULL;
}

static void handle_setting_get(struct bt_mesh_model *mod,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_srv *srv = mod->user_data;
	u16_t id = net_buf_simple_pull_le16(buf);
	u16_t setting_id = net_buf_simple_pull_le16(buf);
	int err;

	if (id == BT_MESH_PROP_ID_PROHIBITED ||
	    setting_id == BT_MESH_PROP_ID_PROHIBITED) {
		return;
	}

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_SENSOR_OP_SETTING_STATUS,
				 BT_MESH_SENSOR_MSG_MAXLEN_SETTING_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_SENSOR_OP_SETTING_STATUS);
	net_buf_simple_add_le16(&rsp, id);
	net_buf_simple_add_le16(&rsp, setting_id);

	const struct bt_mesh_sensor_setting *setting;
	struct bt_mesh_sensor *sensor = sensor_get(srv, id);

	if (!sensor) {
		goto respond;
	}

	setting = setting_get(sensor, setting_id);
	if (!setting || !setting->get) {
		goto respond;
	}

	u8_t minlen = rsp.len;

	net_buf_simple_add_u8(&rsp, setting->set ? 0x03 : 0x01);

	struct sensor_value values[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];

	setting->get(sensor, setting, ctx, values);

	err = sensor_value_encode(&rsp, setting->type, values);
	if (err) {
		BT_WARN("Failed encoding sensor setting 0x%04x: %d",
			setting->type->id, err);

		/* Undo the access field */
		rsp.len = minlen;
	}

respond:
	bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
}

static void setting_set(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf, bool ack)
{
	struct bt_mesh_sensor_srv *srv = mod->user_data;
	u16_t id = net_buf_simple_pull_le16(buf);
	u16_t setting_id = net_buf_simple_pull_le16(buf);
	int err;

	if (id == BT_MESH_PROP_ID_PROHIBITED ||
	    setting_id == BT_MESH_PROP_ID_PROHIBITED) {
		return;
	}

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_SENSOR_OP_SETTING_STATUS,
				 BT_MESH_SENSOR_MSG_MAXLEN_SETTING_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_SENSOR_OP_SETTING_STATUS);
	net_buf_simple_add_le16(&rsp, id);
	net_buf_simple_add_le16(&rsp, setting_id);

	const struct bt_mesh_sensor_setting *setting;
	struct bt_mesh_sensor *sensor;

	sensor = sensor_get(srv, id);
	if (!sensor) {
		goto respond;
	}

	setting = setting_get(sensor, setting_id);
	if (!setting || !setting->set) {
		goto respond;
	}

	struct sensor_value values[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];

	err = sensor_value_decode(buf, setting->type, values);
	if (err) {
		BT_WARN("Error decoding sensor setting 0x%04x: %d",
			setting->type->id, err);

		/* Invalid parameters: ignore this message */
		return;
	}

	setting->set(sensor, setting, ctx, values);

	u8_t minlen = rsp.len;

	net_buf_simple_add_u8(&rsp, 0x03); /* RW */

	err = sensor_value_encode(&rsp, setting->type, values);
	if (err) {
		BT_WARN("Error encoding sensor setting 0x%04x: %d",
			setting->type->id, err);

		/* Undo the access field */
		rsp.len = minlen;
		goto respond;
	}

	BT_DBG("0x%04x: 0x%04x", id, setting_id);

	model_send(mod, NULL, &rsp);

respond:
	if (ack) {
		bt_mesh_model_send(mod, ctx, &rsp, NULL, NULL);
	}
}

static void handle_setting_set(struct bt_mesh_model *mod,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	setting_set(mod, ctx, buf, true);
}

static void handle_setting_set_unack(struct bt_mesh_model *mod,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	setting_set(mod, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_sensor_setup_srv_op[] = {

	{ BT_MESH_SENSOR_OP_CADENCE_GET, BT_MESH_SENSOR_MSG_LEN_CADENCE_GET,
	  handle_cadence_get },
	{ BT_MESH_SENSOR_OP_CADENCE_SET, BT_MESH_SENSOR_MSG_MINLEN_CADENCE_SET,
	  handle_cadence_set },
	{ BT_MESH_SENSOR_OP_CADENCE_SET_UNACKNOWLEDGED,
	  BT_MESH_SENSOR_MSG_MINLEN_CADENCE_SET, handle_cadence_set_unack },
	{ BT_MESH_SENSOR_OP_SETTINGS_GET, BT_MESH_SENSOR_MSG_LEN_SETTINGS_GET,
	  handle_settings_get },
	{ BT_MESH_SENSOR_OP_SETTING_GET, BT_MESH_SENSOR_MSG_LEN_SETTING_GET,
	  handle_setting_get },
	{ BT_MESH_SENSOR_OP_SETTING_SET, BT_MESH_SENSOR_MSG_MINLEN_SETTING_SET,
	  handle_setting_set },
	{ BT_MESH_SENSOR_OP_SETTING_SET_UNACKNOWLEDGED,
	  BT_MESH_SENSOR_MSG_MINLEN_SETTING_SET, handle_setting_set_unack },
};

static int sensor_srv_init(struct bt_mesh_model *mod)
{
	struct bt_mesh_sensor_srv *srv = mod->user_data;

	sys_slist_init(&srv->sensors);

	/* Establish a sorted list of sensors, as this is a requirement when
	 * sending multiple sensor values in one message.
	 */
	u16_t min_id = 0;

	for (int count = 0; count < srv->sensor_count; ++count) {
		struct bt_mesh_sensor *best = NULL;

		for (int j = 0; j < srv->sensor_count; ++j) {
			if (srv->sensor_array[j]->type->id >= min_id &&
			    (!best ||
			     srv->sensor_array[j]->type->id < best->type->id)) {
				best = srv->sensor_array[j];
			}
		}

		if (!best) {
			BT_ERR("Duplicate sensor ID");
			srv->sensor_count = count;
			break;
		}

		sys_slist_append(&srv->sensors, &best->state.node);
		BT_DBG("Sensor 0x%04x", best->type->id);
		min_id = best->type->id + 1;
	}

	srv->model = mod;

	net_buf_simple_init(srv->pub.msg, 0);
	net_buf_simple_init(srv->setup_pub.msg, 0);

	return 0;
}

static int sensor_srv_settings_set(struct bt_mesh_model *mod, size_t len_rd,
				   settings_read_cb read_cb, void *cb_arg)
{
	struct bt_mesh_sensor_srv *srv = mod->user_data;
	int err = 0;

	NET_BUF_SIMPLE_DEFINE(buf, (CONFIG_BT_MESH_SENSOR_SRV_SENSORS_MAX *
				    BT_MESH_SENSOR_MSG_MAXLEN_CADENCE_STATUS));

	ssize_t len = read_cb(cb_arg, net_buf_simple_add(&buf, len_rd), len_rd);

	if (len == 0) {
		return 0;
	}

	if (len != len_rd) {
		BT_ERR("Failed: %d (expected length %u)", len, len_rd);
		return -EINVAL;
	}

	while (buf.len) {
		struct bt_mesh_sensor *s;
		u8_t pub_div;
		u16_t id = net_buf_simple_pull_le16(&buf);

		s = sensor_get(srv, id);
		if (!s) {
			err = -ENODEV;
			break;
		}

		err = sensor_cadence_decode(&buf, s->type, &pub_div,
					    &s->state.min_int,
					    &s->state.threshold);
		if (err) {
			break;
		}

		s->state.pub_div = pub_div;
	}

	if (err) {
		BT_ERR("Failed: %d", err);
	}

	return err;
}

const struct bt_mesh_model_cb _bt_mesh_sensor_srv_cb = {
	.init = sensor_srv_init,
	.settings_set = sensor_srv_settings_set,
};

/** @brief Get the sensor publication interval (in number of publish messages).
 *
 *  @param sensor      Sensor instance
 *  @param period_div  Server's period divisor
 *
 *  @return The publish interval of the sensor measured in number of published
 *          messages by the server.
 */
static u16_t pub_int_get(const struct bt_mesh_sensor *sensor, u8_t period_div)
{
	u8_t div = (sensor->state.pub_div * sensor->state.fast_pub);

	return (1U << MAX(0, period_div - div));
}

/** @brief Get the sensor minimum interval (in number of publish messages).
 *
 *  @param sensor      Sensor instance
 *  @param period_div  Server's period divisor
 *  @param base_period Server's base period
 *
 *  @return The minimum interval of the sensor measured in number of published
 *          messages by the server.
 */
static u16_t min_int_get(const struct bt_mesh_sensor *sensor, u8_t period_div,
			 u32_t base_period)
{
	u32_t pub_int = (base_period >> period_div);
	u32_t min_int = (1 << sensor->state.min_int);

	return ceiling_fraction(min_int, pub_int);
}

/** @brief Conditionally add a sensor value to a publication.
 *
 *  A sensor message will be added to the publication if its minimum interval
 *  has expired and the value is outside its delta threshold or the
 *  publication interval has expired.
 *
 *  @param srv         Server sending the publication.
 *  @param s           Sensor to add data of.
 *  @param period_div  Server's original period divisor.
 *  @param base_period Server's original base period.
 */
static void pub_msg_add(struct bt_mesh_sensor_srv *srv,
			struct bt_mesh_sensor *s, u8_t period_div,
			u32_t base_period)
{
	u16_t min_int = min_int_get(s, period_div, base_period);
	int err;

	if (srv->seq - s->state.seq < min_int) {
		return;
	}

	struct sensor_value value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];

	err = value_get(s, NULL, value);
	if (err) {
		return;
	}

	bool delta_triggered = bt_mesh_sensor_delta_threshold(s, value);
	u16_t interval = pub_int_get(s, period_div);

	if (!delta_triggered && srv->seq - s->state.seq < interval) {
		return;
	}

	err = sensor_status_encode(srv->pub.msg, s, value);
	if (err) {
		return;
	}

	s->state.prev = value[0];
	s->state.seq = srv->seq;
}

int _bt_mesh_sensor_srv_update_handler(struct bt_mesh_model *mod)
{
	struct bt_mesh_sensor_srv *srv = mod->user_data;
	struct bt_mesh_sensor *s;

	bt_mesh_model_msg_init(srv->pub.msg, BT_MESH_SENSOR_OP_STATUS);

	u32_t original_len = srv->pub.msg->len;
	u8_t period_div = srv->pub.period_div;

	BT_DBG("#%u Period: %u ms Divisor: %u (%s)", srv->seq,
	       bt_mesh_model_pub_period_get(mod), period_div,
	       srv->pub.fast_period ? "fast" : "normal");

	srv->pub.period_div = 0;
	srv->pub.fast_period = 0;

	u32_t base_period = bt_mesh_model_pub_period_get(mod);

	SENSOR_FOR_EACH(&srv->sensors, s)
	{
		pub_msg_add(srv, s, period_div, base_period);

		if (s->state.fast_pub) {
			srv->pub.fast_period = true;
			srv->pub.period_div =
				MAX(srv->pub.period_div, s->state.pub_div);
		}
	}

	if (period_div != srv->pub.period_div) {
		BT_DBG("New interval: %u",
		       bt_mesh_model_pub_period_get(srv->model));
	}

	srv->seq++;

	return (srv->pub.msg->len > original_len) ? 0 : -ENOENT;
}

int bt_mesh_sensor_srv_pub(struct bt_mesh_sensor_srv *srv,
			   struct bt_mesh_msg_ctx *ctx,
			   struct bt_mesh_sensor *sensor,
			   const struct sensor_value *value)
{
	int err;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_SENSOR_OP_STATUS,
				 BT_MESH_SENSOR_STATUS_MAXLEN);
	bt_mesh_model_msg_init(&msg, BT_MESH_SENSOR_OP_STATUS);

	err = sensor_status_encode(&msg, sensor, value);
	if (err) {
		return err;
	}

	sensor_cadence_update(sensor, value);

	err = model_send(srv->model, ctx, &msg);
	if (err) {
		return err;
	}

	sensor->state.prev = value[0];
	return 0;
}

int bt_mesh_sensor_srv_sample(struct bt_mesh_sensor_srv *srv,
			      struct bt_mesh_sensor *sensor)
{
	struct sensor_value value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];
	int err;

	err = value_get(sensor, NULL, value);
	if (err) {
		return -EBUSY;
	}

	if (sensor->type->channel_count == 1 &&
	    !bt_mesh_sensor_delta_threshold(sensor, value)) {
		BT_WARN("Outside threshold");
		return -EALREADY;
	}

	BT_DBG("Publishing 0x%04x", sensor->type->id);

	return bt_mesh_sensor_srv_pub(srv, NULL, sensor, value);
}
