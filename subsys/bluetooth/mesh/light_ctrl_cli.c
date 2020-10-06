/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bluetooth/mesh/light_ctrl_cli.h>
#include "model_utils.h"
#include "light_ctrl_internal.h"

union prop_value {
	struct sensor_value prop;
	float coeff;
};
struct prop_status_ctx {
	uint16_t id;
	union prop_value val;
};

static void handle_mode(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_cli *cli = mod->user_data;

	if (buf->len != 1) {
		return;
	}

	uint8_t enabled = net_buf_simple_pull_u8(buf);

	if (enabled > 1) {
		return;
	}

	if (model_ack_match(&cli->ack, BT_MESH_LIGHT_CTRL_OP_MODE_STATUS,
			    ctx)) {
		bool *ack_buf = cli->ack.user_data;

		*ack_buf = enabled;
		model_ack_rx(&cli->ack);
	}

	if (cli->handlers && cli->handlers->mode) {
		cli->handlers->mode(cli, ctx, enabled);
	}
}

static void handle_occupancy(struct bt_mesh_model *mod,
			     struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_cli *cli = mod->user_data;

	if (buf->len != 1) {
		return;
	}

	uint8_t enabled = net_buf_simple_pull_u8(buf);

	if (enabled > 1) {
		return;
	}

	if (model_ack_match(&cli->ack, BT_MESH_LIGHT_CTRL_OP_OM_STATUS, ctx)) {
		bool *ack_buf = cli->ack.user_data;

		*ack_buf = enabled;
		model_ack_rx(&cli->ack);
	}

	if (cli->handlers && cli->handlers->occupancy_mode) {
		cli->handlers->occupancy_mode(cli, ctx, enabled);
	}
}

static void handle_light_onoff(struct bt_mesh_model *mod,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_cli *cli = mod->user_data;
	struct bt_mesh_onoff_status status;

	uint8_t onoff = net_buf_simple_pull_u8(buf);

	if (onoff > 1) {
		return;
	}

	status.present_on_off = onoff;

	if (buf->len == 2) {
		onoff = net_buf_simple_pull_u8(buf);
		if (onoff > 1) {
			return;
		}

		status.target_on_off = onoff;
		status.remaining_time =
			model_transition_decode(net_buf_simple_pull_u8(buf));
	} else if (buf->len == 0) {
		status.remaining_time = 0;
	} else {
		return;
	}

	if (model_ack_match(&cli->ack, BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_STATUS,
			    ctx)) {
		struct bt_mesh_onoff_status *ack_buf = cli->ack.user_data;

		*ack_buf = status;
		model_ack_rx(&cli->ack);
	}

	if (cli->handlers && cli->handlers->light_onoff) {
		cli->handlers->light_onoff(cli, ctx, &status);
	}
}

static void handle_prop(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_cli *cli = mod->user_data;
	struct prop_status_ctx *rsp = cli->ack.user_data;
	const struct bt_mesh_sensor_format *format;
	union prop_value value;
	int err;

	uint16_t id = net_buf_simple_pull_le16(buf);
	bool coeff = (id == BT_MESH_LIGHT_CTRL_COEFF_KID ||
		      id == BT_MESH_LIGHT_CTRL_COEFF_KIU ||
		      id == BT_MESH_LIGHT_CTRL_COEFF_KPD ||
		      id == BT_MESH_LIGHT_CTRL_COEFF_KPU);

	if (coeff) {
		if (buf->len != sizeof(float)) {
			return;
		}

		memcpy(&value.coeff,
		       net_buf_simple_pull_mem(buf, sizeof(float)),
		       sizeof(float));
	} else {
		format = prop_format_get(id);
		if (!format) {
			return;
		}

		err = sensor_ch_decode(buf, format, &value.prop);
		if (err) {
			return;
		}
	}

	if (model_ack_match(&cli->ack, BT_MESH_LIGHT_CTRL_OP_PROP_STATUS,
			    ctx) && rsp->id == id) {

		rsp->val = value;
		model_ack_rx(&cli->ack);
	}

	if (coeff) {
		if (cli->handlers && cli->handlers->coeff) {
			cli->handlers->coeff(cli, ctx, id, value.coeff);
		}
	} else if (cli->handlers && cli->handlers->prop) {
		cli->handlers->prop(cli, ctx, id, &value.prop);
	}
}

const struct bt_mesh_model_op _bt_mesh_light_ctrl_cli_op[] = {
	{ BT_MESH_LIGHT_CTRL_OP_MODE_STATUS, 1, handle_mode },
	{ BT_MESH_LIGHT_CTRL_OP_OM_STATUS, 1, handle_occupancy },
	{ BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_STATUS, 1, handle_light_onoff },
	{ BT_MESH_LIGHT_CTRL_OP_PROP_STATUS, 2, handle_prop },
	BT_MESH_MODEL_OP_END,
};

static int light_ctrl_cli_init(struct bt_mesh_model *mod)
{
	struct bt_mesh_light_ctrl_cli *cli = mod->user_data;

	cli->model = mod;
	net_buf_simple_init(cli->pub.msg, 0);
	model_ack_init(&cli->ack);

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_light_ctrl_cli_cb = {
	.init = light_ctrl_cli_init,
};

/* Public API */

int bt_mesh_light_ctrl_cli_mode_get(struct bt_mesh_light_ctrl_cli *cli,
				    struct bt_mesh_msg_ctx *ctx, bool *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_MODE_GET, 0);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_MODE_GET);

	return model_ackd_send(cli->model, ctx, &buf, rsp ? &cli->ack : NULL,
			       BT_MESH_LIGHT_CTRL_OP_MODE_STATUS, rsp);
}

int bt_mesh_light_ctrl_cli_mode_set(struct bt_mesh_light_ctrl_cli *cli,
				    struct bt_mesh_msg_ctx *ctx, bool enabled,
				    bool *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_MODE_SET, 1);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_MODE_SET);
	net_buf_simple_add_u8(&buf, enabled);

	return model_ackd_send(cli->model, ctx, &buf, rsp ? &cli->ack : NULL,
			       BT_MESH_LIGHT_CTRL_OP_MODE_STATUS, rsp);
}

int bt_mesh_light_ctrl_cli_mode_set_unack(struct bt_mesh_light_ctrl_cli *cli,
					  struct bt_mesh_msg_ctx *ctx,
					  bool enabled)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_MODE_SET_UNACK, 1);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_MODE_SET_UNACK);
	net_buf_simple_add_u8(&buf, enabled);

	return model_send(cli->model, ctx, &buf);
}

int bt_mesh_light_ctrl_cli_occupancy_enabled_get(
	struct bt_mesh_light_ctrl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	bool *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_OM_GET, 0);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_OM_GET);

	return model_ackd_send(cli->model, ctx, &buf, rsp ? &cli->ack : NULL,
			       BT_MESH_LIGHT_CTRL_OP_OM_STATUS, rsp);
}

int bt_mesh_light_ctrl_cli_occupancy_enabled_set(
	struct bt_mesh_light_ctrl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	bool enabled, bool *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_OM_SET, 1);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_OM_SET);
	net_buf_simple_add_u8(&buf, enabled);

	return model_ackd_send(cli->model, ctx, &buf, rsp ? &cli->ack : NULL,
			       BT_MESH_LIGHT_CTRL_OP_OM_STATUS, rsp);
}

int bt_mesh_light_ctrl_cli_occupancy_enabled_set_unack(
	struct bt_mesh_light_ctrl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	bool enabled)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_OM_SET_UNACK, 1);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_OM_SET_UNACK);
	net_buf_simple_add_u8(&buf, enabled);

	return model_send(cli->model, ctx, &buf);
}

int bt_mesh_light_ctrl_cli_light_onoff_get(struct bt_mesh_light_ctrl_cli *cli,
					   struct bt_mesh_msg_ctx *ctx,
					   struct bt_mesh_onoff_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_GET, 0);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_GET);

	return model_ackd_send(cli->model, ctx, &buf, rsp ? &cli->ack : NULL,
			       BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_STATUS, rsp);
}

int bt_mesh_light_ctrl_cli_light_onoff_set(struct bt_mesh_light_ctrl_cli *cli,
					   struct bt_mesh_msg_ctx *ctx,
					   const struct bt_mesh_onoff_set *set,
					   struct bt_mesh_onoff_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_SET, 4);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_SET);
	net_buf_simple_add_u8(&buf, set->on_off);
	net_buf_simple_add_u8(&buf, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&buf, set->transition);
	}

	return model_ackd_send(cli->model, ctx, &buf, rsp ? &cli->ack : NULL,
			       BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_STATUS, rsp);
}

int bt_mesh_light_ctrl_cli_light_onoff_set_unack(
	struct bt_mesh_light_ctrl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_onoff_set *set)
{
	BT_MESH_MODEL_BUF_DEFINE(
		buf, BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_SET_UNACK, 4);
	bt_mesh_model_msg_init(&buf,
			       BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_SET_UNACK);
	net_buf_simple_add_u8(&buf, set->on_off);
	net_buf_simple_add_u8(&buf, cli->tid++);
	if (set->transition) {
		model_transition_buf_add(&buf, set->transition);
	}

	return model_send(cli->model, ctx, &buf);
}

int bt_mesh_light_ctrl_cli_prop_get(struct bt_mesh_light_ctrl_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    enum bt_mesh_light_ctrl_prop id,
				    struct sensor_value *rsp)
{
	struct prop_status_ctx ack = {
		.id = id,
	};
	int err;

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_PROP_GET, 2);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_PROP_GET);
	net_buf_simple_add_le16(&buf, id);

	err = model_ackd_send(cli->model, ctx, &buf, rsp ? &cli->ack : NULL,
			      BT_MESH_LIGHT_CTRL_OP_PROP_STATUS, &ack);
	if (err) {
		return err;
	}

	if (rsp) {
		*rsp = ack.val.prop;
	}

	return 0;
}

int bt_mesh_light_ctrl_cli_prop_set(struct bt_mesh_light_ctrl_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    enum bt_mesh_light_ctrl_prop id,
				    const struct sensor_value *val,
				    struct sensor_value *rsp)
{
	const struct bt_mesh_sensor_format *format;
	struct prop_status_ctx ack = {
		.id = id,
	};
	int err;

	BT_MESH_MODEL_BUF_DEFINE(
		buf, BT_MESH_LIGHT_CTRL_OP_PROP_SET,
		2 + CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_PROP_SET);
	net_buf_simple_add_le16(&buf, id);

	format = prop_format_get(id);
	if (!format) {
		return -EINVAL;
	}

	err = sensor_ch_encode(&buf, format, val);
	if (err) {
		return err;
	}

	err = model_ackd_send(cli->model, ctx, &buf, rsp ? &cli->ack : NULL,
			      BT_MESH_LIGHT_CTRL_OP_PROP_STATUS, &ack);
	if (err) {
		return err;
	}

	if (rsp) {
		*rsp = ack.val.prop;
	}

	return 0;
}

int bt_mesh_light_ctrl_cli_prop_set_unack(struct bt_mesh_light_ctrl_cli *cli,
					  struct bt_mesh_msg_ctx *ctx,
					  enum bt_mesh_light_ctrl_prop id,
					  const struct sensor_value *val)
{
	const struct bt_mesh_sensor_format *format;
	int err;

	BT_MESH_MODEL_BUF_DEFINE(
		buf, BT_MESH_LIGHT_CTRL_OP_PROP_SET_UNACK,
		2 + CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_PROP_SET_UNACK);
	net_buf_simple_add_le16(&buf, id);

	format = prop_format_get(id);
	if (!format) {
		return -EINVAL;
	}

	err = sensor_ch_encode(&buf, format, val);
	if (err) {
		return err;
	}

	return model_send(cli->model, ctx, &buf);
}

int bt_mesh_light_ctrl_cli_coeff_get(struct bt_mesh_light_ctrl_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     enum bt_mesh_light_ctrl_coeff id,
				     float *rsp)
{
	struct prop_status_ctx ack = {
		.id = id,
	};
	int err;

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_PROP_GET, 2);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_PROP_GET);
	net_buf_simple_add_le16(&buf, id);

	err = model_ackd_send(cli->model, ctx, &buf, rsp ? &cli->ack : NULL,
			      BT_MESH_LIGHT_CTRL_OP_PROP_STATUS, &ack);
	if (err) {
		return err;
	}

	if (rsp) {
		*rsp = ack.val.coeff;
	}

	return 0;
}

int bt_mesh_light_ctrl_cli_coeff_set(struct bt_mesh_light_ctrl_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     enum bt_mesh_light_ctrl_coeff id,
				     float val, float *rsp)
{
	struct prop_status_ctx ack = {
		.id = id,
	};
	int err;

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_PROP_SET,
				 2 + sizeof(float));
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_PROP_SET);
	net_buf_simple_add_le16(&buf, id);
	net_buf_simple_add_mem(&buf, &val, sizeof(float));

	err = model_ackd_send(cli->model, ctx, &buf, rsp ? &cli->ack : NULL,
			      BT_MESH_LIGHT_CTRL_OP_PROP_STATUS, &ack);
	if (err) {
		return err;
	}

	if (rsp) {
		*rsp = ack.val.coeff;
	}

	return 0;
}

int bt_mesh_light_ctrl_cli_coeff_set_unack(struct bt_mesh_light_ctrl_cli *cli,
					   struct bt_mesh_msg_ctx *ctx,
					   enum bt_mesh_light_ctrl_coeff id,
					   float val)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_PROP_SET_UNACK,
				 2 + sizeof(float));
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_PROP_SET_UNACK);
	net_buf_simple_add_le16(&buf, id);
	net_buf_simple_add_mem(&buf, &val, sizeof(float));

	return model_send(cli->model, ctx, &buf);
}
