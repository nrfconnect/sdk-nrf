/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdlib.h>
#include <bluetooth/mesh/sensor_cli.h>
#include <bluetooth/mesh/properties.h>
#include "model_utils.h"
#include "sensor.h"
#include "mesh/net.h"
#include "mesh/transport.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)
#define LOG_MODULE_NAME bt_mesh_sensor_cli
#include "common/log.h"

struct list_rsp {
	struct bt_mesh_sensor_info *sensors;
	u32_t count;
};

struct settings_rsp {
	u16_t id;
	u16_t *ids;
	u32_t count;
};

struct setting_rsp {
	u16_t id;
	u16_t setting_id;
	struct bt_mesh_sensor_setting_status *setting;
};

struct sensor_data_rsp {
	u16_t id;
	struct sensor_value *value;
};

struct series_data_rsp {
	struct bt_mesh_sensor_series_entry *entries;
	const struct bt_mesh_sensor_column *col;
	u16_t id;
	u32_t count;
};

struct cadence_rsp {
	u16_t id;
	struct bt_mesh_sensor_cadence_status *cadence;
};

static void tolerance_decode(u16_t encoded, struct sensor_value *tolerance)
{
	u32_t toll_mill = (encoded * 100ULL * 1000000ULL) / 4095ULL;

	tolerance->val1 = toll_mill / 1000000ULL;
	tolerance->val2 = toll_mill % 1000000ULL;
}

static void unknown_type(struct bt_mesh_sensor_cli *cli,
			 struct bt_mesh_msg_ctx *ctx, u16_t id, u32_t op)
{
	if (cli->cb && cli->cb->unknown_type) {
		cli->cb->unknown_type(cli, ctx, id, op);
	}
}

static void handle_descriptor_status(struct bt_mesh_model *mod,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	if (buf->len != 2 && buf->len % 8) {
		return;
	}

	struct bt_mesh_sensor_cli *cli = mod->user_data;
	struct list_rsp *ack_ctx = cli->ack.user_data;
	u32_t count = 0;
	bool is_rsp;

	is_rsp = model_ack_match(&cli->ack, BT_MESH_SENSOR_OP_DESCRIPTOR_STATUS,
				 ctx);

	/* A packet with only the sensor ID means that the given sensor doesn't
	 * exist on the sensor server.
	 */
	if (buf->len == 2) {
		ack_ctx->count = 0;
		goto yield_ack;
	}

	struct bt_mesh_sensor_info sensor;

	while (buf->len >= 8) {
		u32_t tolerances;

		sensor.id = net_buf_simple_pull_le16(buf);
		tolerances = net_buf_simple_pull_le24(buf);
		tolerance_decode(tolerances & BIT_MASK(12),
				 &sensor.descriptor.tolerance.positive);
		tolerance_decode(tolerances >> 12,
				 &sensor.descriptor.tolerance.negative);
		sensor.descriptor.sampling_type = net_buf_simple_pull_u8(buf);
		sensor.descriptor.period =
			sensor_powtime_decode(net_buf_simple_pull_u8(buf));
		sensor.descriptor.update_interval =
			sensor_powtime_decode(net_buf_simple_pull_u8(buf));

		if (cli->cb && cli->cb->sensor) {
			cli->cb->sensor(cli, ctx, &sensor);
		}

		if (is_rsp && count < ack_ctx->count) {
			ack_ctx->sensors[count++] = sensor;
		}
	}

yield_ack:
	if (is_rsp) {
		ack_ctx->count = count;
		model_ack_rx(&cli->ack);
	}
}

static void handle_status(struct bt_mesh_model *mod,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_cli *cli = mod->user_data;
	struct sensor_data_rsp *rsp = cli->ack.user_data;
	bool is_rsp;
	int err;

	is_rsp = model_ack_match(&cli->ack, BT_MESH_SENSOR_OP_STATUS, ctx);

	while (buf->len > 3) {
		const struct bt_mesh_sensor_type *type;
		u8_t length;
		u16_t id;

		sensor_status_id_decode(buf, &length, &id);
		if (length == 0) {
			if (is_rsp && rsp->id == id) {
				rsp->value = NULL;
				rsp->id = BT_MESH_PROP_ID_PROHIBITED;
				model_ack_rx(&cli->ack);
			}

			continue;
		}

		type = bt_mesh_sensor_type_get(id);
		if (!type) {
			net_buf_simple_pull(buf, length);
			unknown_type(cli, ctx, id, BT_MESH_SENSOR_OP_STATUS);
			continue;
		}

		u8_t expected_len = sensor_value_len(type);

		if (length != expected_len) {
			BT_WARN("Invalid length for 0x%04x: %u (expected %u)",
				id, length, expected_len);
			return;
		}

		struct sensor_value value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];

		err = sensor_value_decode(buf, type, value);
		if (err) {
			return; /* Invalid format, should ignore message */
		}

		if (cli->cb && cli->cb->data) {
			cli->cb->data(cli, ctx, type, value);
		}

		if (is_rsp && rsp->id == id) {
			memcpy(rsp->value, value,
			       sizeof(struct sensor_value) *
				       type->channel_count);
			rsp->id = BT_MESH_PROP_ID_PROHIBITED;
			model_ack_rx(&cli->ack);
		}
	}
}

static int parse_series_entry(const struct bt_mesh_sensor_type *type,
			      const struct bt_mesh_sensor_format *col_format,
			      struct net_buf_simple *buf,
			      struct bt_mesh_sensor_series_entry *entry)
{
	int err;

	err = sensor_ch_decode(buf, col_format, &entry->column.start);
	if (err) {
		return err;
	}

	if (buf->len == 0) {
		BT_WARN("The requested column didn't exist: %s",
			bt_mesh_sensor_ch_str(&entry->column.start));
		return -ENOENT;
	}

	struct sensor_value width;

	err = sensor_ch_decode(buf, col_format, &width);
	if (err) {
		return err;
	}

	u64_t end_mill = (entry->column.start.val1 + width.val1) * 1000000L +
			 (entry->column.start.val2 + width.val2);

	entry->column.end.val1 = end_mill / 1000000L;
	entry->column.end.val2 = end_mill % 1000000L;

	return sensor_value_decode(buf, type, entry->value);
}

static void handle_column_status(struct bt_mesh_model *mod,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_cli *cli = mod->user_data;
	struct series_data_rsp *rsp = cli->ack.user_data;
	const struct bt_mesh_sensor_format *col_format;
	const struct bt_mesh_sensor_type *type;
	int err;

	u16_t id = net_buf_simple_pull_le16(buf);

	type = bt_mesh_sensor_type_get(id);
	if (!type) {
		unknown_type(cli, ctx, id, BT_MESH_SENSOR_OP_COLUMN_STATUS);
		return;
	}

	col_format = bt_mesh_sensor_column_format_get(type);
	if (!col_format) {
		BT_WARN("Received unsupported column format 0x%04x", id);
		return;
	}

	struct bt_mesh_sensor_series_entry entry;

	err = parse_series_entry(type, col_format, buf, &entry);
	if (err == -ENOENT) {
		/* The entry doesn't exist */
		goto yield_ack;
	}

	if (err) {
		return; /* Invalid format, should ignore message */
	}

	if (cli->cb && cli->cb->series_entry) {
		cli->cb->series_entry(cli, ctx, type, 0, 1, &entry);
	}

yield_ack:
	if (model_ack_match(&cli->ack, BT_MESH_SENSOR_OP_COLUMN_STATUS, ctx) &&
	    rsp->col->start.val1 == entry.column.start.val1 &&
	    rsp->col->start.val2 == entry.column.start.val2) {
		if (err) {
			rsp->count = 0;
		} else {
			rsp->entries[0] = entry;
			rsp->count = 1;
		}
		model_ack_rx(&cli->ack);
	}
}

static void handle_series_status(struct bt_mesh_model *mod,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_cli *cli = mod->user_data;
	const struct bt_mesh_sensor_format *col_format;
	const struct bt_mesh_sensor_type *type;
	struct series_data_rsp *rsp = NULL;

	u16_t id = net_buf_simple_pull_le16(buf);

	type = bt_mesh_sensor_type_get(id);
	if (!type) {
		unknown_type(cli, ctx, id, BT_MESH_SENSOR_OP_SERIES_STATUS);
		return;
	}

	if (model_ack_match(&cli->ack, BT_MESH_SENSOR_OP_SERIES_STATUS, ctx) &&
	    rsp->id == id) {
		rsp = cli->ack.user_data;
	}

	col_format = bt_mesh_sensor_column_format_get(type);
	if (!col_format) {
		BT_WARN("Received unsupported column format 0x%04x", id);

		if (rsp) {
			rsp->count = 0;
			model_ack_rx(&cli->ack);
		}

		return;
	}

	u8_t count = buf->len / col_format->size * 2 + sensor_value_len(type);

	for (u8_t i = 0; i < count; i++) {
		struct bt_mesh_sensor_series_entry entry;

		(void)parse_series_entry(type, col_format, buf, &entry);

		if (cli->cb && cli->cb->series_entry) {
			cli->cb->series_entry(cli, ctx, type, i, count, &entry);
		}

		if (rsp && i < rsp->count) {
			rsp->entries[i] = entry;
		}
	}

	if (rsp) {
		rsp->count = count;
		model_ack_rx(&cli->ack);
	}
}

static void handle_cadence_status(struct bt_mesh_model *mod,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_cli *cli = mod->user_data;
	struct cadence_rsp *rsp = cli->ack.user_data;
	int err;

	u16_t id = net_buf_simple_pull_le16(buf);
	const struct bt_mesh_sensor_type *type = bt_mesh_sensor_type_get(id);

	if (!type) {
		unknown_type(cli, ctx, id, BT_MESH_SENSOR_OP_CADENCE_STATUS);
		return;
	}

	if (buf->len == 0 || type->channel_count != 1) {
		BT_WARN("Type 0x%04x doesn't support cadence", id);
		err = -ENOTSUP;
		goto yield_ack;
	}

	struct bt_mesh_sensor_cadence_status cadence;

	err = sensor_cadence_decode(buf, type, &cadence.fast_period_div,
				    &cadence.min_int, &cadence.threshold);
	if (err) {
		return;
	}

	if (cli->cb && cli->cb->cadence) {
		cli->cb->cadence(cli, ctx, type, &cadence);
	}

yield_ack:
	if (model_ack_match(&cli->ack, BT_MESH_SENSOR_OP_CADENCE_STATUS, ctx) &&
	    rsp->id == id) {
		if (err) {
			rsp->cadence = NULL;
		} else {
			*rsp->cadence = cadence;
		}

		model_ack_rx(&cli->ack);
	}
}

static void handle_settings_status(struct bt_mesh_model *mod,
				   struct bt_mesh_msg_ctx *ctx,
				   struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_cli *cli = mod->user_data;
	struct settings_rsp *rsp = cli->ack.user_data;

	if (buf->len % 2) {
		return;
	}

	u16_t id = net_buf_simple_pull_le16(buf);
	const struct bt_mesh_sensor_type *type = bt_mesh_sensor_type_get(id);

	if (!type) {
		unknown_type(cli, ctx, id, BT_MESH_SENSOR_OP_SETTINGS_STATUS);
		return;
	}

	/* The list may be unaligned: */
	u16_t ids[(BT_MESH_RX_SDU_MAX -
		   BT_MESH_MODEL_OP_LEN(BT_MESH_SENSOR_OP_SETTINGS_STATUS) -
		   2) /
		  sizeof(u16_t)];
	u32_t count = buf->len / 2;

	memcpy(ids, net_buf_simple_pull_mem(buf, buf->len),
	       count * sizeof(ids));

	if (cli->cb && cli->cb->settings) {
		cli->cb->settings(cli, ctx, type, ids, count);
	}

	if (model_ack_match(&cli->ack, BT_MESH_SENSOR_OP_SETTINGS_STATUS,
			    ctx) &&
	    rsp->id == id) {
		memcpy(rsp->ids, ids, sizeof(u16_t) * MIN(count, rsp->count));
		rsp->count = count;
		model_ack_rx(&cli->ack);
	}
}

static void handle_setting_status(struct bt_mesh_model *mod,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct bt_mesh_sensor_cli *cli = mod->user_data;
	struct setting_rsp *rsp = cli->ack.user_data;
	int err;

	u16_t id = net_buf_simple_pull_le16(buf);
	u16_t setting_id = net_buf_simple_pull_le16(buf);
	struct bt_mesh_sensor_setting_status setting;
	u8_t access;

	if (buf->len == 0) {
		setting.type = NULL;
		goto yield_ack;
	}

	access = net_buf_simple_pull_u8(buf);
	if (access != 0x01 && access != 0x03) {
		return;
	}

	const struct bt_mesh_sensor_type *type = bt_mesh_sensor_type_get(id);

	if (!type) {
		unknown_type(cli, ctx, id, BT_MESH_SENSOR_OP_SETTING_STATUS);
		return;
	}


	setting.type = bt_mesh_sensor_type_get(setting_id);
	if (!setting.type) {
		unknown_type(
			cli, ctx, setting_id, BT_MESH_SENSOR_OP_SETTING_STATUS);
		return;
	}

	setting.writable = (access == 0x03);

	/* If we attempted setting an unwritable value, the server omits the
	 * setting value. This is a legal response, but it should make the set()
	 * function return -EACCES.
	 */
	if (buf->len == 0 && !setting.writable) {
		goto yield_ack;
	}

	err = sensor_value_decode(buf, setting.type, setting.value);
	if (err) {
		return;
	}

	if (cli->cb && cli->cb->setting_status) {
		cli->cb->setting_status(cli, ctx, type, &setting);
	}

yield_ack:
	if (model_ack_match(&cli->ack, BT_MESH_SENSOR_OP_SETTING_STATUS, ctx) &&
	    rsp->id == id && rsp->setting_id == setting_id) {
		*rsp->setting = setting;
		model_ack_rx(&cli->ack);
	}
}

const struct bt_mesh_model_op _bt_mesh_sensor_cli_op[] = {
	{ BT_MESH_SENSOR_OP_DESCRIPTOR_STATUS,
	  BT_MESH_SENSOR_MSG_MINLEN_DESCRIPTOR_STATUS,
	  handle_descriptor_status },
	{ BT_MESH_SENSOR_OP_STATUS, BT_MESH_SENSOR_MSG_MINLEN_STATUS,
	  handle_status },
	{ BT_MESH_SENSOR_OP_COLUMN_STATUS,
	  BT_MESH_SENSOR_MSG_MINLEN_COLUMN_STATUS, handle_column_status },
	{ BT_MESH_SENSOR_OP_SERIES_STATUS,
	  BT_MESH_SENSOR_MSG_MINLEN_SERIES_STATUS, handle_series_status },
	{ BT_MESH_SENSOR_OP_CADENCE_STATUS,
	  BT_MESH_SENSOR_MSG_MINLEN_CADENCE_STATUS, handle_cadence_status },
	{ BT_MESH_SENSOR_OP_SETTINGS_STATUS,
	  BT_MESH_SENSOR_MSG_MINLEN_SETTINGS_STATUS, handle_settings_status },
	{ BT_MESH_SENSOR_OP_SETTING_STATUS,
	  BT_MESH_SENSOR_MSG_MINLEN_SETTING_STATUS, handle_setting_status },
};

static int sensor_cli_init(struct bt_mesh_model *mod)
{
	struct bt_mesh_sensor_cli *cli = mod->user_data;

	cli->mod = mod;

	net_buf_simple_init(cli->pub.msg, 0);

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_sensor_cli_cb = {
	.init = sensor_cli_init,
};

int bt_mesh_sensor_cli_list_get(struct bt_mesh_sensor_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				struct bt_mesh_sensor_info *sensors,
				u32_t *count)
{
	int err;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_SENSOR_OP_DESCRIPTOR_GET, 0);
	bt_mesh_model_msg_init(&msg, BT_MESH_SENSOR_OP_DESCRIPTOR_GET);

	struct list_rsp list_rsp = {
		.sensors = sensors,
		.count = *count,
	};

	err = model_ackd_send(cli->mod, ctx, &msg, sensors ? &cli->ack : NULL,
			      BT_MESH_SENSOR_OP_DESCRIPTOR_STATUS, &list_rsp);

	*count = list_rsp.count;
	return err;
}

int bt_mesh_sensor_cli_desc_get(struct bt_mesh_sensor_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				struct bt_mesh_sensor_type *sensor,
				struct bt_mesh_sensor_descriptor *rsp)
{
	int err;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_SENSOR_OP_DESCRIPTOR_GET, 2);
	bt_mesh_model_msg_init(&msg, BT_MESH_SENSOR_OP_DESCRIPTOR_GET);

	net_buf_simple_add_le16(&msg, sensor->id);

	struct bt_mesh_sensor_info info = {
		sensor->id,
	};
	struct list_rsp list_rsp = {
		.sensors = &info,
		.count = 1,
	};

	err = model_ackd_send(cli->mod, ctx, &msg, rsp ? &cli->ack : NULL,
			      BT_MESH_SENSOR_OP_DESCRIPTOR_STATUS, &list_rsp);
	if (err) {
		return err;
	}

	if (list_rsp.count == 0) {
		return -ENOTSUP;
	}

	if (rsp) {
		*rsp = info.descriptor;
	}

	return 0;
}

int bt_mesh_sensor_cli_cadence_get(struct bt_mesh_sensor_cli *cli,
				   struct bt_mesh_msg_ctx *ctx,
				   struct bt_mesh_sensor_type *sensor,
				   struct bt_mesh_sensor_cadence_status *rsp)
{
	if (sensor->channel_count != 1) {
		return -ENOTSUP;
	}

	int err;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_SENSOR_OP_CADENCE_GET, 2);
	bt_mesh_model_msg_init(&msg, BT_MESH_SENSOR_OP_CADENCE_GET);

	net_buf_simple_add_le16(&msg, sensor->id);

	struct cadence_rsp rsp_data = {
		.id = sensor->id,
		.cadence = rsp,
	};

	err = model_ackd_send(cli->mod, ctx, &msg, rsp ? &cli->ack : NULL,
			       BT_MESH_SENSOR_OP_CADENCE_STATUS, &rsp_data);
	if (err) {
		return err;
	}

	/* Status handler clears the cadence member if server indicates that
	 * cadence isn't supported.
	 */
	if (rsp && !rsp_data.cadence) {
		return -ENOTSUP;
	}

	return 0;
}

int bt_mesh_sensor_cli_cadence_set(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	struct bt_mesh_sensor_type *sensor,
	const struct bt_mesh_sensor_cadence_status *cadence,
	struct bt_mesh_sensor_cadence_status *rsp)
{
	if (sensor->channel_count != 1) {
		return -ENOTSUP;
	}

	int err;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_SENSOR_OP_CADENCE_SET,
				 BT_MESH_SENSOR_MSG_MAXLEN_CADENCE_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_SENSOR_OP_CADENCE_SET);

	net_buf_simple_add_le16(&msg, sensor->id);

	struct cadence_rsp rsp_data = {
		.id = sensor->id,
		.cadence = rsp,
	};

	err = sensor_cadence_encode(&msg, sensor, cadence->fast_period_div,
				    cadence->min_int, &cadence->threshold);
	if (err) {
		return err;
	}

	err = model_ackd_send(cli->mod, ctx, &msg, rsp ? &cli->ack : NULL,
			      BT_MESH_SENSOR_OP_CADENCE_STATUS, &rsp_data);
	if (err) {
		return err;
	}

	/* Status handler clears the cadence member if server indicates that
	 * cadence isn't supported.
	 */
	if (rsp && !rsp_data.cadence) {
		return -ENOTSUP;
	}

	return 0;
}

int bt_mesh_sensor_cli_cadence_set_unack(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	struct bt_mesh_sensor_type *sensor,
	const struct bt_mesh_sensor_cadence_status *cadence)
{
	if (sensor->channel_count != 1) {
		return -ENOTSUP;
	}

	int err;

	BT_MESH_MODEL_BUF_DEFINE(msg,
				 BT_MESH_SENSOR_OP_CADENCE_SET_UNACKNOWLEDGED,
				 BT_MESH_SENSOR_MSG_MAXLEN_CADENCE_SET);
	bt_mesh_model_msg_init(&msg,
			       BT_MESH_SENSOR_OP_CADENCE_SET_UNACKNOWLEDGED);

	net_buf_simple_add_le16(&msg, sensor->id);

	err = sensor_cadence_encode(&msg, sensor, cadence->fast_period_div,
				    cadence->min_int, &cadence->threshold);
	if (err) {
		return err;
	}

	return model_send(cli->mod, ctx, &msg);
}

int bt_mesh_sensor_cli_settings_get(struct bt_mesh_sensor_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    struct bt_mesh_sensor_type *sensor,
				    u16_t *ids, u32_t *count)
{
	int err;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_SENSOR_OP_SETTINGS_GET,
				 BT_MESH_SENSOR_MSG_LEN_SETTINGS_GET);
	bt_mesh_model_msg_init(&msg, BT_MESH_SENSOR_OP_SETTINGS_GET);

	net_buf_simple_add_le16(&msg, sensor->id);

	struct settings_rsp rsp = {
		.id = sensor->id,
		.ids = ids,
		.count = *count,
	};

	err = model_ackd_send(cli->mod, ctx, &msg, ids ? &cli->ack : NULL,
			      BT_MESH_SENSOR_OP_SETTINGS_STATUS, &rsp);

	if (ids && !err) {
		*count = rsp.count;
	}

	return err;
}

int bt_mesh_sensor_cli_setting_get(struct bt_mesh_sensor_cli *cli,
				   struct bt_mesh_msg_ctx *ctx,
				   struct bt_mesh_sensor_type *sensor,
				   const struct bt_mesh_sensor_type *setting,
				   struct bt_mesh_sensor_setting_status *rsp)
{
	int err;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_SENSOR_OP_SETTING_GET,
				 BT_MESH_SENSOR_MSG_LEN_SETTING_GET);
	bt_mesh_model_msg_init(&msg, BT_MESH_SENSOR_OP_SETTING_GET);

	net_buf_simple_add_le16(&msg, sensor->id);
	net_buf_simple_add_le16(&msg, setting->id);

	struct setting_rsp rsp_data = {
		.id = sensor->id,
		.setting_id = setting->id,
		.setting = rsp,
	};

	err = model_ackd_send(cli->mod, ctx, &msg, rsp ? &cli->ack : NULL,
			      BT_MESH_SENSOR_OP_SETTING_STATUS, &rsp_data);
	if (err) {
		return err;
	}

	if (!rsp->type) {
		return -ENOENT;
	}

	return 0;
}

int bt_mesh_sensor_cli_setting_set(struct bt_mesh_sensor_cli *cli,
				   struct bt_mesh_msg_ctx *ctx,
				   struct bt_mesh_sensor_type *sensor,
				   const struct bt_mesh_sensor_type *setting,
				   const struct sensor_value *value,
				   struct bt_mesh_sensor_setting_status *rsp)
{
	int err;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_SENSOR_OP_SETTING_SET,
				 BT_MESH_SENSOR_MSG_MAXLEN_SETTING_SET);
	bt_mesh_model_msg_init(&msg, BT_MESH_SENSOR_OP_SETTING_SET);

	net_buf_simple_add_le16(&msg, sensor->id);
	net_buf_simple_add_le16(&msg, setting->id);
	err = sensor_value_encode(&msg, setting, value);
	if (err) {
		return err;
	}

	struct setting_rsp rsp_data = {
		.id = sensor->id,
		.setting_id = setting->id,
		.setting = rsp,
	};

	err = model_ackd_send(cli->mod, ctx, &msg, rsp ? &cli->ack : NULL,
			       BT_MESH_SENSOR_OP_SETTING_STATUS, &rsp_data);
	if (err) {
		return err;
	}

	if (!rsp->type)  {
		return -ENOENT;
	}

	if (!rsp->writable) {
		return -EACCES;
	}

	return 0;
}

int bt_mesh_sensor_cli_setting_set_unack(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	struct bt_mesh_sensor_type *sensor,
	const struct bt_mesh_sensor_type *setting,
	const struct sensor_value *value)
{
	int err;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_SENSOR_OP_SETTING_GET,
				 BT_MESH_SENSOR_MSG_LEN_SETTING_GET);
	bt_mesh_model_msg_init(&msg, BT_MESH_SENSOR_OP_SETTING_GET);

	net_buf_simple_add_le16(&msg, sensor->id);
	net_buf_simple_add_le16(&msg, setting->id);
	err = sensor_value_encode(&msg, setting, value);
	if (err) {
		return err;
	}

	return model_send(cli->mod, ctx, &msg);
}

int bt_mesh_sensor_cli_get(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	struct bt_mesh_sensor_type *sensor,
	struct sensor_value *rsp)
{
	int err;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_SENSOR_OP_GET,
				 BT_MESH_SENSOR_MSG_MAXLEN_GET);
	bt_mesh_model_msg_init(&msg, BT_MESH_SENSOR_OP_GET);
	net_buf_simple_add_le16(&msg, sensor->id);

	struct sensor_data_rsp rsp_data = { .id = sensor->id, .value = rsp };

	err = model_ackd_send(cli->mod, ctx, &msg, rsp ? &cli->ack : NULL,
			       BT_MESH_SENSOR_OP_STATUS, &rsp_data);
	if (err) {
		return err;
	}

	if (!rsp_data.value) {
		return -ENODEV;
	}

	return 0;
}

int bt_mesh_sensor_cli_series_entry_get(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	struct bt_mesh_sensor_type *sensor,
	const struct bt_mesh_sensor_column *column,
	struct bt_mesh_sensor_series_entry *rsp)
{
	int err;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_SENSOR_OP_COLUMN_GET,
				 BT_MESH_SENSOR_MSG_MAXLEN_COLUMN_GET);
	bt_mesh_model_msg_init(&msg, BT_MESH_SENSOR_OP_COLUMN_GET);
	net_buf_simple_add_le16(&msg, sensor->id);
	err = sensor_ch_encode(&msg, bt_mesh_sensor_column_format_get(sensor),
			       &column->start);
	if (err) {
		return err;
	}

	struct series_data_rsp rsp_data = {
		.entries = rsp,
		.id = sensor->id,
		.count = 1,
		.col = column,
	};

	err = model_ackd_send(cli->mod, ctx, &msg, rsp ? &cli->ack : NULL,
			      BT_MESH_SENSOR_OP_COLUMN_STATUS, &rsp_data);
	if (err) {
		return err;
	}

	if (rsp && rsp_data.count == 0) {
		return -ENOENT;
	}

	return 0;
}

int bt_mesh_sensor_cli_series_entries_get(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	struct bt_mesh_sensor_type *sensor,
	const struct bt_mesh_sensor_column *range,
	struct bt_mesh_sensor_series_entry *rsp, u32_t *count)
{
	const struct bt_mesh_sensor_format *col_format;
	int err;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_SENSOR_OP_SERIES_GET,
				 BT_MESH_SENSOR_MSG_MAXLEN_SERIES_GET);
	bt_mesh_model_msg_init(&msg, BT_MESH_SENSOR_OP_SERIES_GET);

	net_buf_simple_add_le16(&msg, sensor->id);

	col_format = bt_mesh_sensor_column_format_get(sensor);

	err = sensor_ch_encode(&msg, col_format, &range->start);
	if (err) {
		return err;
	}

	err = sensor_ch_encode(&msg, col_format, &range->end);
	if (err) {
		return err;
	}

	struct series_data_rsp rsp_data = {
		.entries = rsp,
		.id = sensor->id,
		.count = *count,
	};

	err = model_ackd_send(cli->mod, ctx, &msg, rsp ? &cli->ack : NULL,
			       BT_MESH_SENSOR_OP_SERIES_STATUS, &rsp_data);
	if (err) {
		return err;
	}

	*count = rsp_data.count;

	return 0;
}
