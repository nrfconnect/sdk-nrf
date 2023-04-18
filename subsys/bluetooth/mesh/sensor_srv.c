/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_mesh_sensor_srv);

#define SENSOR_FOR_EACH(_list, _node)                                          \
	SYS_SLIST_FOR_EACH_CONTAINER(_list, _node, state.node)

static struct bt_mesh_sensor *sensor_get(struct bt_mesh_sensor_srv *srv,
					 uint16_t id)
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

#if CONFIG_BT_SETTINGS
static void store_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_mesh_sensor_srv *srv = CONTAINER_OF(
		dwork, struct bt_mesh_sensor_srv, store_timer);

	/* Cadence is stored as a sequence of cadence status messages */
	NET_BUF_SIMPLE_DEFINE(buf, (CONFIG_BT_MESH_SENSOR_SRV_SENSORS_MAX *
				    BT_MESH_SENSOR_MSG_MAXLEN_CADENCE_STATUS));

	for (int i = 0; i < srv->sensor_count; ++i) {
		const struct bt_mesh_sensor *s = srv->sensor_array[i];
		int err;

		if (!s->state.configured) {
			continue;
		}

		net_buf_simple_add_le16(&buf, s->type->id);
		err = sensor_cadence_encode(&buf, s->type, s->state.pub_div,
					    s->state.min_int,
					    &s->state.threshold);
		if (err) {
			LOG_ERR("Failed encode data: %d", err);
			return;
		}
	}

	(void)bt_mesh_model_data_store(srv->model, false, NULL, buf.data,
				       buf.len);

}
#endif

static void cadence_store(struct bt_mesh_sensor_srv *srv)
{
#if CONFIG_BT_SETTINGS
	k_work_schedule(
		&srv->store_timer,
		K_SECONDS(CONFIG_BT_MESH_MODEL_SRV_STORE_TIMEOUT));
#endif
}


static int value_get(struct bt_mesh_sensor_srv *srv,
		     struct bt_mesh_sensor *sensor,
		     struct bt_mesh_msg_ctx *ctx,
		     struct sensor_value *value)
{
	int err;

	if (!sensor->get) {
		return -ENOTSUP;
	}

	err = sensor->get(srv, sensor, ctx, value);
	if (err) {
		LOG_WRN("Value get for 0x%04x: %d", sensor->type->id, err);
		return err;
	}

	sensor_cadence_update(sensor, value);

	return 0;
}

static int buf_status_add(struct bt_mesh_sensor_srv *srv,
			  struct bt_mesh_sensor *sensor,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct sensor_value value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX] = {};
	int err;

	err = value_get(srv, sensor, ctx, value);
	if (err) {
		sensor_status_id_encode(buf, 0, sensor->type->id);
		return err;
	}

	err = sensor_status_encode(buf, sensor, value);
	if (err) {
		LOG_WRN("Sensor value encode for 0x%04x: %d", sensor->type->id,
			err);
		sensor_status_id_encode(buf, 0, sensor->type->id);
	}

	return err;
}

static int handle_descriptor_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_srv *srv = model->user_data;

	if (buf->len != BT_MESH_SENSOR_MSG_MINLEN_DESCRIPTOR_GET &&
	    buf->len != BT_MESH_SENSOR_MSG_MAXLEN_DESCRIPTOR_GET) {
		return -EMSGSIZE;
	}

	NET_BUF_SIMPLE_DEFINE(rsp, BT_MESH_TX_SDU_MAX);
	bt_mesh_model_msg_init(&rsp, BT_MESH_SENSOR_OP_DESCRIPTOR_STATUS);

	struct bt_mesh_sensor *sensor;

	if (buf->len == 2) {
		uint16_t id = net_buf_simple_pull_le16(buf);

		if (id == BT_MESH_PROP_ID_PROHIBITED) {
			return -EINVAL;
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
		LOG_DBG("Reporting ID 0x%04x", sensor->type->id);

		if (net_buf_simple_tailroom(&rsp) < (8 + BT_MESH_MIC_SHORT)) {
			LOG_WRN("Not enough room for all descriptors");
			break;
		}

		sensor_descriptor_encode(&rsp, sensor);
	}

respond:
	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

static int handle_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_srv *srv = model->user_data;

	if (buf->len != BT_MESH_SENSOR_MSG_MINLEN_GET &&
	    buf->len != BT_MESH_SENSOR_MSG_MAXLEN_GET) {
		return -EMSGSIZE;
	}

	NET_BUF_SIMPLE_DEFINE(rsp, BT_MESH_TX_SDU_MAX);
	bt_mesh_model_msg_init(&rsp, BT_MESH_SENSOR_OP_STATUS);

	struct bt_mesh_sensor *sensor;

	if (buf->len == 2) {
		uint16_t id = net_buf_simple_pull_le16(buf);

		if (id == BT_MESH_PROP_ID_PROHIBITED) {
			return -EINVAL;
		}

		sensor = sensor_get(srv, id);
		if (sensor) {
			buf_status_add(srv, sensor, ctx, &rsp);
		} else {
			LOG_WRN("Unknown sensor ID 0x%04x", id);
			sensor_status_id_encode(&rsp, 0, id);
		}

		goto respond;
	}

	SENSOR_FOR_EACH(&srv->sensors, sensor) {
		buf_status_add(srv, sensor, ctx, &rsp);
	}

respond:
	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

static const struct bt_mesh_sensor_column *
column_get(const struct bt_mesh_sensor_series *series,
	   const struct sensor_value *val)
{
	for (uint32_t i = 0; i < series->column_count; ++i) {
		if (series->columns[i].start.val1 == val->val1 &&
		    series->columns[i].start.val2 == val->val2) {
			return &series->columns[i];
		}
	}

	return NULL;
}

static int handle_column_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_srv *srv = model->user_data;
	int err;

	uint16_t id = net_buf_simple_pull_le16(buf);

	if (id == BT_MESH_PROP_ID_PROHIBITED) {
		return -EINVAL;
	}

	struct bt_mesh_sensor *sensor = sensor_get(srv, id);

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_SENSOR_OP_COLUMN_STATUS,
				 BT_MESH_SENSOR_MSG_MAXLEN_COLUMN_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_SENSOR_OP_COLUMN_STATUS);
	net_buf_simple_add_le16(&rsp, id);

	if (!sensor || !sensor->series.get) {
		LOG_WRN("No series support in 0x%04x", sensor->type->id);
		goto respond;
	}

	if (sensor->type->channel_count < 3) {
		/* For sensors with one or two channels, pull the column index
		 * directly from the message
		 */
		uint32_t col_index = net_buf_simple_pull_le16(buf);

		if (col_index >= sensor->series.column_count) {
			LOG_WRN("Column out of range in 0x%04x",
				sensor->type->id);
			goto respond;
		}

		err = sensor_column_value_encode(&rsp, srv, sensor, ctx,
						 col_index);
		if (err) {
			LOG_WRN("Failed encoding sensor column: %d", err);
			return err;
		}
		goto respond;
	}

	if (!sensor->series.columns) {
		LOG_WRN("No series support in 0x%04x", sensor->type->id);
		goto respond;
	}

	const struct bt_mesh_sensor_format *col_format;
	const struct bt_mesh_sensor_column *col;
	struct sensor_value col_x;

	col_format = bt_mesh_sensor_column_format_get(sensor->type);

	if (col_format == NULL) {
		return -ENOTSUP;
	}

	err = sensor_ch_decode(buf, col_format, &col_x);
	if (err) {
		return err;
	}

	LOG_DBG("Column %s", bt_mesh_sensor_ch_str(&col_x));

	col = column_get(&sensor->series, &col_x);
	if (!col) {
		LOG_WRN("Unknown column");
		(void) sensor_ch_encode(&rsp, col_format, &col_x);
		goto respond;
	}

	err = sensor_column_encode(&rsp, srv, sensor, ctx, col);
	if (err) {
		LOG_WRN("Failed encoding sensor column: %d", err);
		return err;
	}

respond:
	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

static uint16_t max_column_count(const struct bt_mesh_sensor_type *sensor)
{
	uint16_t column_size = 0;

	for (int i = 0; i < sensor->channel_count; i++) {
		column_size += sensor->channels[i].format->size;
	}
	return (BT_MESH_TX_SDU_MAX - BT_MESH_MIC_SHORT - 3) / column_size;
}

static int handle_series_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_srv *srv = model->user_data;
	const struct bt_mesh_sensor_format *col_format;

	uint16_t id = net_buf_simple_pull_le16(buf);

	if (id == BT_MESH_PROP_ID_PROHIBITED) {
		return -EINVAL;
	}

	struct bt_mesh_sensor *sensor = sensor_get(srv, id);

	NET_BUF_SIMPLE_DEFINE(rsp, BT_MESH_TX_SDU_MAX);
	bt_mesh_model_msg_init(&rsp, BT_MESH_SENSOR_OP_SERIES_STATUS);
	net_buf_simple_add_le16(&rsp, id);

	if (!sensor || !sensor->series.get) {
		LOG_WRN("No series support in 0x%04x", sensor->type->id);
		goto respond;
	}

	if (sensor->type->channel_count < 3) {
		uint16_t start = 0;
		uint16_t end = sensor->series.column_count - 1;

		if (buf->len == sizeof(uint16_t) * 2) {
			start = net_buf_simple_pull_le16(buf);
			end = net_buf_simple_pull_le16(buf);
			if (start >= sensor->series.column_count ||
			    end >= sensor->series.column_count ||
			    start > end) {
				LOG_WRN("Invalid range");
				goto respond;
			}
		} else if (buf->len != 0) {
			LOG_WRN("Invalid length (%u)", buf->len);
			return -EMSGSIZE;
		}

		uint16_t max_columns = max_column_count(sensor->type);

		if (end - start + 1 > max_columns) {
			end = start + max_columns - 1;
		}

		for (uint32_t i = start; i <= end; i++) {
			int err = sensor_column_value_encode(&rsp, srv, sensor,
							     ctx, i);

			if (err) {
				LOG_WRN("Column encode failed");
				return err;
			}
		}
		goto respond;
	}

	col_format = bt_mesh_sensor_column_format_get(sensor->type);
	if (!col_format || !sensor->series.columns) {
		LOG_WRN("No series support in 0x%04x", sensor->type->id);
		goto respond;
	}

	struct bt_mesh_sensor_column range;
	bool ranged = (buf->len != 0);

	if (buf->len == col_format->size * 2) {
		int err;

		err = sensor_ch_decode(buf, col_format, &range.start);
		if (err) {
			LOG_WRN("Range start decode failed: %d", err);
			return err;
		}

		err = sensor_ch_decode(buf, col_format, &range.end);
		if (err) {
			LOG_WRN("Range end decode failed: %d", err);
			return err;
		}
	} else if (buf->len != 0) {
		/* invalid length */
		LOG_WRN("Invalid length (%u)", buf->len);
		return -EMSGSIZE;
	}

	for (uint32_t i = 0; i < sensor->series.column_count; ++i) {
		const struct bt_mesh_sensor_column *col =
			&sensor->series.columns[i];

		if (ranged &&
		    !bt_mesh_sensor_value_in_column(&col->start, &range)) {
			continue;
		}

		LOG_DBG("Column #%u", i);

		int err = sensor_column_encode(&rsp, srv, sensor, ctx, col);

		if (err) {
			LOG_WRN("Failed encoding: %d", err);
			return err;
		}
	}

respond:
	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_sensor_srv_op[] = {
	{
		BT_MESH_SENSOR_OP_DESCRIPTOR_GET,
		BT_MESH_LEN_MIN(BT_MESH_SENSOR_MSG_MINLEN_DESCRIPTOR_GET),
		handle_descriptor_get,
	},
	{
		BT_MESH_SENSOR_OP_GET,
		BT_MESH_LEN_MIN(BT_MESH_SENSOR_MSG_MINLEN_GET),
		handle_get,
	},
	{
		BT_MESH_SENSOR_OP_COLUMN_GET,
		BT_MESH_LEN_MIN(BT_MESH_SENSOR_MSG_MINLEN_COLUMN_GET),
		handle_column_get,
	},
	{
		BT_MESH_SENSOR_OP_SERIES_GET,
		BT_MESH_LEN_MIN(BT_MESH_SENSOR_MSG_MINLEN_SERIES_GET),
		handle_series_get,
	},
	BT_MESH_MODEL_OP_END,
};

static int handle_cadence_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_srv *srv = model->user_data;
	struct bt_mesh_sensor *sensor;
	uint16_t id;
	int err;

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_SENSOR_OP_CADENCE_STATUS,
				 BT_MESH_SENSOR_MSG_MAXLEN_CADENCE_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_SENSOR_OP_CADENCE_STATUS);

	id = net_buf_simple_pull_le16(buf);
	if (id == BT_MESH_PROP_ID_PROHIBITED) {
		return -EINVAL;
	}

	net_buf_simple_add_le16(&rsp, id);

	sensor = sensor_get(srv, id);
	if (!sensor || sensor->type->channel_count != 1 || !sensor->state.configured) {
		LOG_WRN("Cadence not supported or not configured");
		goto respond;
	}

	err = sensor_cadence_encode(&rsp, sensor->type, sensor->state.pub_div,
				    sensor->state.min_int,
				    &sensor->state.threshold);
	if (err) {
		return err;
	}

respond:
	bt_mesh_model_send(srv->model, ctx, &rsp, NULL, NULL);

	return 0;
}

static int cadence_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf, bool ack)
{
	struct bt_mesh_sensor_srv *srv = model->user_data;
	struct bt_mesh_sensor *sensor;
	uint16_t id;

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_SENSOR_OP_CADENCE_STATUS,
				 BT_MESH_SENSOR_MSG_MAXLEN_CADENCE_STATUS);
	bt_mesh_model_msg_init(&rsp, BT_MESH_SENSOR_OP_CADENCE_STATUS);

	id = net_buf_simple_pull_le16(buf);
	if (id == BT_MESH_PROP_ID_PROHIBITED) {
		return -EINVAL;
	}

	net_buf_simple_add_le16(&rsp, id);

	sensor = sensor_get(srv, id);
	if (!sensor || sensor->type->channel_count != 1) {
		LOG_WRN("Cadence not supported");
		goto respond;
	}

	struct bt_mesh_sensor_threshold threshold;
	uint8_t period_div, min_int;
	int err;

	err = sensor_cadence_decode(buf, sensor->type, &period_div, &min_int,
				    &threshold);
	if (err) {
		LOG_WRN("Invalid cadence");
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_MODEL_LOG_LEVEL_DBG)) {
		char delta_down_str[BT_MESH_SENSOR_CH_STR_LEN];
		char delta_up_str[BT_MESH_SENSOR_CH_STR_LEN];
		char range_low_str[BT_MESH_SENSOR_CH_STR_LEN];
		char range_high_str[BT_MESH_SENSOR_CH_STR_LEN];

		strcpy(delta_down_str, bt_mesh_sensor_ch_str(&threshold.delta.down));
		strcpy(delta_up_str, bt_mesh_sensor_ch_str(&threshold.delta.up));
		strcpy(range_low_str, bt_mesh_sensor_ch_str(&threshold.range.low));
		strcpy(range_high_str, bt_mesh_sensor_ch_str(&threshold.range.high));

		LOG_DBG("Min int: %u div: %u delta: %s to %s range: %s to %s (%s)", min_int,
			period_div, delta_down_str, delta_up_str, range_low_str, range_high_str,
			(threshold.range.cadence == BT_MESH_SENSOR_CADENCE_FAST ? "fast" : "slow"));
	}

	sensor->state.min_int = min_int;
	sensor->state.pub_div = period_div;
	sensor->state.threshold = threshold;
	sensor->state.configured = true;

	/** Reschedule publication timer if the cadence increased. */
	if (period_div > srv->pub.period_div) {
		int period_ms;

		srv->pub.period_div = period_div;
		period_ms = bt_mesh_model_pub_period_get(srv->model);

		if (period_ms > 0) {
			LOG_DBG("New publication interval: %u", period_ms);
			k_work_reschedule(&srv->model->pub->timer, K_MSEC(period_ms));
		}
	}

	cadence_store(srv);

	err = sensor_cadence_encode(&rsp, sensor->type, sensor->state.pub_div,
				    sensor->state.min_int,
				    &sensor->state.threshold);
	if (err) {
		return err;
	}

	bt_mesh_msg_send(model, NULL, &rsp);

respond:
	if (ack) {
		bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);
	}

	return 0;
}

static int handle_cadence_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	return cadence_set(model, ctx, buf, true);
}

static int handle_cadence_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	return cadence_set(model, ctx, buf, false);
}

static int handle_settings_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_srv *srv = model->user_data;

	uint16_t id = net_buf_simple_pull_le16(buf);

	if (id == BT_MESH_PROP_ID_PROHIBITED) {
		return -EINVAL;
	}

	LOG_DBG("0x%04x", id);

	BT_MESH_MODEL_BUF_DEFINE(
		rsp, BT_MESH_SENSOR_OP_SETTINGS_STATUS,
		2 + 2 * CONFIG_BT_MESH_SENSOR_SRV_SETTINGS_MAX);
	bt_mesh_model_msg_init(&rsp, BT_MESH_SENSOR_OP_SETTINGS_STATUS);
	net_buf_simple_add_le16(&rsp, id);

	struct bt_mesh_sensor *sensor = sensor_get(srv, id);

	if (!sensor) {
		goto respond;
	}

	for (uint32_t i = 0; i < MIN(CONFIG_BT_MESH_SENSOR_SRV_SETTINGS_MAX,
				  sensor->settings.count);
	     ++i) {
		net_buf_simple_add_le16(&rsp,
					sensor->settings.list[i].type->id);
	}

respond:
	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

static const struct bt_mesh_sensor_setting *
setting_get(struct bt_mesh_sensor *sensor, uint16_t setting_id)
{
	for (uint32_t i = 0; i < sensor->settings.count; i++) {
		if (sensor->settings.list[i].type->id == setting_id) {
			return &sensor->settings.list[i];
		}
	}
	return NULL;
}

static int handle_setting_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_srv *srv = model->user_data;
	uint16_t id = net_buf_simple_pull_le16(buf);
	uint16_t setting_id = net_buf_simple_pull_le16(buf);
	int err;

	if (id == BT_MESH_PROP_ID_PROHIBITED ||
	    setting_id == BT_MESH_PROP_ID_PROHIBITED) {
		return -EINVAL;
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

	uint8_t minlen = rsp.len;

	net_buf_simple_add_u8(&rsp, setting->set ? 0x03 : 0x01);

	struct sensor_value values[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX] = { 0 };

	setting->get(srv, sensor, setting, ctx, values);

	err = sensor_value_encode(&rsp, setting->type, values);
	if (err) {
		LOG_WRN("Failed encoding sensor setting 0x%04x: %d",
			setting->type->id, err);

		/* Undo the access field */
		rsp.len = minlen;
	}

respond:
	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

static int setting_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf, bool ack)
{
	struct bt_mesh_sensor_srv *srv = model->user_data;
	uint16_t id = net_buf_simple_pull_le16(buf);
	uint16_t setting_id = net_buf_simple_pull_le16(buf);
	int err;

	if (id == BT_MESH_PROP_ID_PROHIBITED ||
	    setting_id == BT_MESH_PROP_ID_PROHIBITED) {
		return -EINVAL;
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
		LOG_WRN("Error decoding sensor setting 0x%04x: %d",
			setting->type->id, err);

		/* Invalid parameters: ignore this message */
		return err;
	}

	setting->set(srv, sensor, setting, ctx, values);

	uint8_t minlen = rsp.len;

	net_buf_simple_add_u8(&rsp, 0x03); /* RW */

	err = sensor_value_encode(&rsp, setting->type, values);
	if (err) {
		LOG_WRN("Error encoding sensor setting 0x%04x: %d",
			setting->type->id, err);

		/* Undo the access field */
		rsp.len = minlen;
		goto respond;
	}

	LOG_DBG("0x%04x: 0x%04x", id, setting_id);

	bt_mesh_msg_send(model, NULL, &rsp);

respond:
	if (ack) {
		bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);
	}

	return 0;
}

static int handle_setting_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	return setting_set(model, ctx, buf, true);
}

static int handle_setting_set_unack(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	return setting_set(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_sensor_setup_srv_op[] = {

	{
		BT_MESH_SENSOR_OP_CADENCE_GET,
		BT_MESH_LEN_EXACT(BT_MESH_SENSOR_MSG_LEN_CADENCE_GET),
		handle_cadence_get,
	},
	{
		BT_MESH_SENSOR_OP_CADENCE_SET,
		BT_MESH_LEN_MIN(BT_MESH_SENSOR_MSG_MINLEN_CADENCE_SET),
		handle_cadence_set,
	},
	{
		BT_MESH_SENSOR_OP_CADENCE_SET_UNACKNOWLEDGED,
		BT_MESH_LEN_MIN(BT_MESH_SENSOR_MSG_MINLEN_CADENCE_SET),
		handle_cadence_set_unack,
	},
	{
		BT_MESH_SENSOR_OP_SETTINGS_GET,
		BT_MESH_LEN_EXACT(BT_MESH_SENSOR_MSG_LEN_SETTINGS_GET),
		handle_settings_get,
	},
	{
		BT_MESH_SENSOR_OP_SETTING_GET,
		BT_MESH_LEN_EXACT(BT_MESH_SENSOR_MSG_LEN_SETTING_GET),
		handle_setting_get,
	},
	{
		BT_MESH_SENSOR_OP_SETTING_SET,
		BT_MESH_LEN_MIN(BT_MESH_SENSOR_MSG_MINLEN_SETTING_SET),
		handle_setting_set,
	},
	{
		BT_MESH_SENSOR_OP_SETTING_SET_UNACKNOWLEDGED,
		BT_MESH_LEN_MIN(BT_MESH_SENSOR_MSG_MINLEN_SETTING_SET),
		handle_setting_set_unack,
	},
	BT_MESH_MODEL_OP_END,
};

/** @brief Get the sensor publication interval (in number of publish messages).
 *
 *  @param sensor      Sensor instance
 *  @param period_div  Server's period divisor
 *
 *  @return The publish interval of the sensor measured in number of published
 *          messages by the server.
 */
static uint16_t pub_int_get(const struct bt_mesh_sensor *sensor,
			    uint8_t period_div)
{
	uint8_t div = (sensor->state.pub_div * sensor->state.fast_pub);

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
static uint16_t min_int_get(const struct bt_mesh_sensor *sensor,
			    uint8_t period_div, uint32_t base_period)
{
	uint32_t pub_int = (base_period >> period_div);
	uint32_t min_int = (1 << sensor->state.min_int);

	return DIV_ROUND_UP(min_int, pub_int);
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
			struct bt_mesh_sensor *s, uint8_t period_div,
			uint32_t base_period)
{
	uint16_t min_int = min_int_get(s, period_div, base_period);
	uint16_t delta = srv->seq - s->state.seq;
	int err;

	if (delta < min_int) {
		return;
	}

	if (!s->state.configured &&
	    (delta < (1 << period_div))) {
		/** Don't publish a sensor value with not configured sensor cadence state more
		 * frequently than base periodic publication.
		 */
		return;
	}

	struct sensor_value value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX] = {};

	err = value_get(srv, s, NULL, value);
	if (err) {
		return;
	}

	if (s->state.configured) {
		bool delta_triggered = bt_mesh_sensor_delta_threshold(s, value);
		uint16_t interval = pub_int_get(s, period_div);

		if (!delta_triggered && delta < interval) {
			return;
		}
	}

	err = sensor_status_encode(srv->pub.msg, s, value);
	if (err) {
		return;
	}

	s->state.prev = value[0];
	s->state.seq = srv->seq;
}

static int update_handler(struct bt_mesh_model *model)
{
	struct bt_mesh_sensor_srv *srv = model->user_data;
	struct bt_mesh_sensor *s;

	bt_mesh_model_msg_init(srv->pub.msg, BT_MESH_SENSOR_OP_STATUS);

	uint32_t original_len = srv->pub.msg->len;
	uint8_t period_div = srv->pub.period_div;

	LOG_DBG("#%u Period: %u ms Divisor: %u (%s)", srv->seq,
	       bt_mesh_model_pub_period_get(model), period_div,
	       srv->pub.fast_period ? "fast" : "normal");

	/** Reset the publication divisor and the fast period divisor flag to get the actual
	 * publication period.
	 */
	srv->pub.period_div = 0;
	srv->pub.fast_period = 0;

	uint32_t base_period = bt_mesh_model_pub_period_get(model);

	srv->pub.fast_period = true;

	SENSOR_FOR_EACH(&srv->sensors, s)
	{
		pub_msg_add(srv, s, period_div, base_period);

		/** Update the publication divisor to a new value. This is needed to take new
		 * changes in a sensor cadence state, .e.g. when the cadence decreased.
		 */
		srv->pub.period_div =
			MAX(srv->pub.period_div, s->state.pub_div);
	}

	if (period_div != srv->pub.period_div) {
		LOG_DBG("New interval: %u",
		       bt_mesh_model_pub_period_get(srv->model));
	}

	srv->seq++;

	return (srv->pub.msg->len > original_len) ? 0 : -ENOENT;
}

static int sensor_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_sensor_srv *srv = model->user_data;

	sys_slist_init(&srv->sensors);

#if CONFIG_BT_SETTINGS
	k_work_init_delayable(&srv->store_timer, store_timeout);
#endif

	/* Establish a sorted list of sensors, as this is a requirement when
	 * sending multiple sensor values in one message.
	 */
	uint16_t min_id = 0;

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
			LOG_ERR("Duplicate sensor ID");
			srv->sensor_count = count;
			break;
		}

		sys_slist_append(&srv->sensors, &best->state.node);
		LOG_DBG("Sensor 0x%04x", best->type->id);
		min_id = best->type->id + 1;
	}

	srv->seq = 1;

	srv->model = model;

	srv->pub.update = update_handler;
	srv->pub.msg = &srv->pub_buf;

	/** Always use fast period to make the server sample sensors with the fastests cadence. */
	srv->pub.fast_period = true;

	srv->setup_pub.msg = &srv->setup_pub_buf;
	net_buf_simple_init_with_data(&srv->pub_buf, srv->pub_data,
				      sizeof(srv->pub_data));
	net_buf_simple_init_with_data(&srv->setup_pub_buf, srv->setup_pub_data,
				      sizeof(srv->setup_pub_data));

	return 0;
}

static void sensor_srv_reset(struct bt_mesh_model *model)
{
	struct bt_mesh_sensor_srv *srv = model->user_data;

	net_buf_simple_reset(srv->pub.msg);
	net_buf_simple_reset(srv->setup_pub.msg);

	for (int i = 0; i < srv->sensor_count; ++i) {
		struct bt_mesh_sensor *s = srv->sensor_array[i];

		s->state.pub_div = 0;
		s->state.min_int = 0;
		s->state.configured = false;
		memset(&s->state.threshold, 0, sizeof(s->state.threshold));
	}

	srv->pub.period_div = 0;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		(void)bt_mesh_model_data_store(srv->model, false, NULL, NULL,
					       0);
	}
}

static int sensor_srv_settings_set(struct bt_mesh_model *model, const char *name,
				   size_t len_rd, settings_read_cb read_cb,
				   void *cb_arg)
{
	struct bt_mesh_sensor_srv *srv = model->user_data;
	int err = 0;

	NET_BUF_SIMPLE_DEFINE(buf, (CONFIG_BT_MESH_SENSOR_SRV_SENSORS_MAX *
				    BT_MESH_SENSOR_MSG_MAXLEN_CADENCE_STATUS));

	if (name) {
		return -ENOENT;
	}

	ssize_t len = read_cb(cb_arg, net_buf_simple_add(&buf, len_rd), len_rd);

	if (len == 0) {
		return 0;
	}

	if (len != len_rd) {
		LOG_ERR("Failed: %d (expected length %u)", len, len_rd);
		return -EINVAL;
	}

	while (buf.len) {
		struct bt_mesh_sensor *s;
		uint8_t pub_div;
		uint16_t id = net_buf_simple_pull_le16(&buf);

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
		s->state.configured = true;

		/** Update the publication divisor so that the publication timer starts with
		 * the fastest cadence after settings are loaded.
		 */
		srv->pub.period_div = MAX(srv->pub.period_div, s->state.pub_div);
	}

	if (err) {
		LOG_ERR("Failed: %d", err);
	}

	return err;
}

const struct bt_mesh_model_cb _bt_mesh_sensor_srv_cb = {
	.init = sensor_srv_init,
	.reset = sensor_srv_reset,
	.settings_set = sensor_srv_settings_set,
};

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

	err = bt_mesh_msg_send(srv->model, ctx, &msg);
	if (err) {
		return err;
	}

	sensor->state.prev = value[0];
	return 0;
}

int bt_mesh_sensor_srv_sample(struct bt_mesh_sensor_srv *srv,
			      struct bt_mesh_sensor *sensor)
{
	struct sensor_value value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX] = {};
	int err;

	err = value_get(srv, sensor, NULL, value);
	if (err) {
		return -EBUSY;
	}

	if (sensor->state.configured &&
	    sensor->type->channel_count == 1 &&
	    !bt_mesh_sensor_delta_threshold(sensor, value)) {
		LOG_WRN("Outside threshold");
		return -EALREADY;
	}

	LOG_DBG("Publishing 0x%04x", sensor->type->id);

	return bt_mesh_sensor_srv_pub(srv, NULL, sensor, value);
}
