/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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

static int handle_mode(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	bool *ack_buf;
	struct bt_mesh_light_ctrl_cli *cli = model->rt->user_data;
	uint8_t enabled = net_buf_simple_pull_u8(buf);

	if (enabled > 1) {
		return -EINVAL;
	}

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_LIGHT_CTRL_OP_MODE_STATUS, ctx->addr,
				      (void **)&ack_buf)) {
		*ack_buf = enabled;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->mode) {
		cli->handlers->mode(cli, ctx, enabled);
	}

	return 0;
}

static int handle_occupancy(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	bool *ack_buf;
	struct bt_mesh_light_ctrl_cli *cli = model->rt->user_data;
	uint8_t enabled = net_buf_simple_pull_u8(buf);

	if (enabled > 1) {
		return -EINVAL;
	}

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_LIGHT_CTRL_OP_OM_STATUS, ctx->addr,
				      (void **)&ack_buf)) {
		*ack_buf = enabled;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->occupancy_mode) {
		cli->handlers->occupancy_mode(cli, ctx, enabled);
	}

	return 0;
}

static int handle_light_onoff(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_cli *cli = model->rt->user_data;
	struct bt_mesh_onoff_status status;
	struct bt_mesh_onoff_status *ack_buf;

	uint8_t onoff = net_buf_simple_pull_u8(buf);

	if (onoff > 1) {
		return -EINVAL;
	}

	status.present_on_off = onoff;

	if (buf->len == 2) {
		onoff = net_buf_simple_pull_u8(buf);
		if (onoff > 1) {
			return -EINVAL;
		}

		status.target_on_off = onoff;
		status.remaining_time =
			model_transition_decode(net_buf_simple_pull_u8(buf));
	} else if (buf->len == 0) {
		status.target_on_off = onoff;
		status.remaining_time = 0;
	} else {
		return -EMSGSIZE;
	}

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_STATUS,
				      ctx->addr, (void **)&ack_buf)) {
		*ack_buf = status;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->handlers && cli->handlers->light_onoff) {
		cli->handlers->light_onoff(cli, ctx, &status);
	}

	return 0;
}

static int handle_prop(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	struct bt_mesh_light_ctrl_cli *cli = model->rt->user_data;
	struct prop_status_ctx *rsp;
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
			return -EINVAL;
		}

		memcpy(&value.coeff,
		       net_buf_simple_pull_mem(buf, sizeof(float)),
		       sizeof(float));
	} else {
		format = prop_format_get(id);
		if (!format) {
			return -ENOENT;
		}

		err = sensor_ch_decode(buf, format, &value.prop);
		if (err) {
			return -EINVAL;
		}
	}

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_LIGHT_CTRL_OP_PROP_STATUS, ctx->addr,
				      (void **)&rsp) &&
	    rsp->id == id) {
		rsp->val = value;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (coeff) {
		if (cli->handlers && cli->handlers->coeff) {
			cli->handlers->coeff(cli, ctx, id, value.coeff);
		}
	} else if (cli->handlers && cli->handlers->prop) {
		cli->handlers->prop(cli, ctx, id, &value.prop);
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_light_ctrl_cli_op[] = {
	{
		BT_MESH_LIGHT_CTRL_OP_MODE_STATUS,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_CTRL_MSG_LEN_MODE_STATUS),
		handle_mode,
	},
	{
		BT_MESH_LIGHT_CTRL_OP_OM_STATUS,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_CTRL_MSG_LEN_OM_STATUS),
		handle_occupancy,
	},
	{
		BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_LIGHT_CTRL_MSG_MINLEN_LIGHT_ONOFF_STATUS),
		handle_light_onoff,
	},
	{
		BT_MESH_LIGHT_CTRL_OP_PROP_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_LIGHT_CTRL_MSG_MINLEN_PROP_STATUS),
		handle_prop,
	},
	BT_MESH_MODEL_OP_END,
};

static int light_ctrl_cli_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_light_ctrl_cli *cli = model->rt->user_data;

	cli->model = model;
	cli->pub.msg = &cli->pub_buf;
	net_buf_simple_init_with_data(&cli->pub_buf, cli->pub_data,
				      sizeof(cli->pub_data));
	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);

	return 0;
}

static void light_ctrl_cli_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_light_ctrl_cli *cli = model->rt->user_data;

	net_buf_simple_reset(cli->pub.msg);
	bt_mesh_msg_ack_ctx_reset(&cli->ack_ctx);
}

const struct bt_mesh_model_cb _bt_mesh_light_ctrl_cli_cb = {
	.init = light_ctrl_cli_init,
	.reset = light_ctrl_cli_reset,
};

/* Public API */

int bt_mesh_light_ctrl_cli_mode_get(struct bt_mesh_light_ctrl_cli *cli,
				    struct bt_mesh_msg_ctx *ctx, bool *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_MODE_GET, 0);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_MODE_GET);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHT_CTRL_OP_MODE_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_light_ctrl_cli_mode_set(struct bt_mesh_light_ctrl_cli *cli,
				    struct bt_mesh_msg_ctx *ctx, bool enabled,
				    bool *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_MODE_SET, 1);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_MODE_SET);
	net_buf_simple_add_u8(&buf, enabled);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHT_CTRL_OP_MODE_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_light_ctrl_cli_mode_set_unack(struct bt_mesh_light_ctrl_cli *cli,
					  struct bt_mesh_msg_ctx *ctx,
					  bool enabled)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_MODE_SET_UNACK, 1);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_MODE_SET_UNACK);
	net_buf_simple_add_u8(&buf, enabled);

	return bt_mesh_msg_send(cli->model, ctx, &buf);
}

int bt_mesh_light_ctrl_cli_occupancy_enabled_get(
	struct bt_mesh_light_ctrl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	bool *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_OM_GET, 0);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_OM_GET);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHT_CTRL_OP_OM_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_light_ctrl_cli_occupancy_enabled_set(
	struct bt_mesh_light_ctrl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	bool enabled, bool *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_OM_SET, 1);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_OM_SET);
	net_buf_simple_add_u8(&buf, enabled);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHT_CTRL_OP_OM_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_light_ctrl_cli_occupancy_enabled_set_unack(
	struct bt_mesh_light_ctrl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	bool enabled)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_OM_SET_UNACK, 1);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_OM_SET_UNACK);
	net_buf_simple_add_u8(&buf, enabled);

	return bt_mesh_msg_send(cli->model, ctx, &buf);
}

int bt_mesh_light_ctrl_cli_light_onoff_get(struct bt_mesh_light_ctrl_cli *cli,
					   struct bt_mesh_msg_ctx *ctx,
					   struct bt_mesh_onoff_status *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_GET, 0);
	bt_mesh_model_msg_init(&buf, BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_GET);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
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

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHT_CTRL_OP_LIGHT_ONOFF_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
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

	return bt_mesh_msg_send(cli->model, ctx, &buf);
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

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHT_CTRL_OP_PROP_STATUS,
		.user_data = &ack,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	err = bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
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

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHT_CTRL_OP_PROP_STATUS,
		.user_data = &ack,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	err = bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
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

	return bt_mesh_msg_send(cli->model, ctx, &buf);
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

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHT_CTRL_OP_PROP_STATUS,
		.user_data = &ack,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	err = bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
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

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_LIGHT_CTRL_OP_PROP_STATUS,
		.user_data = &ack,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	err = bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
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

	return bt_mesh_msg_send(cli->model, ctx, &buf);
}
