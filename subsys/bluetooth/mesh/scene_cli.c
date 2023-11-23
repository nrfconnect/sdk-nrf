/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/sys/byteorder.h>
#include <bluetooth/mesh/models.h>
#include "model_utils.h"

static int handle_scene_status(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	struct bt_mesh_scene_state *rsp;
	struct bt_mesh_scene_cli *cli = model->user_data;
	struct bt_mesh_scene_state state;

	state.status = net_buf_simple_pull_u8(buf);
	state.current = net_buf_simple_pull_le16(buf);
	if (buf->len == 3) {
		state.target = net_buf_simple_pull_le16(buf);
		state.remaining_time = model_transition_decode(net_buf_simple_pull_u8(buf));
	} else if (!buf->len) {
		state.target = BT_MESH_SCENE_NONE;
		state.remaining_time = 0;
	} else {
		/* Invalid size */
		return -EMSGSIZE;
	}

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_SCENE_OP_STATUS, ctx->addr,
				      (void **)&rsp)) {
		*rsp = state;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->status) {
		cli->status(cli, ctx, &state);
	}

	return 0;
}

static int handle_scene_reg(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct bt_mesh_scene_register *rsp;
	struct bt_mesh_scene_cli *cli = model->user_data;
	struct bt_mesh_scene_register reg;

	reg.status = net_buf_simple_pull_u8(buf);
	reg.current = net_buf_simple_pull_le16(buf);
	if (buf->len % 2) {
		return -EMSGSIZE;
	}

	reg.count = buf->len / sizeof(uint16_t);
	reg.scenes = net_buf_simple_pull_mem(buf, buf->len);

	/* Fix endianness for big endian systems. This gets optimized to nothing
	 * on little endian systems (like the nRF5x chips).
	 */
	for (int i = 0; i < reg.count; i++) {
		reg.scenes[i] = sys_le16_to_cpu(reg.scenes[i]);
	}

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, BT_MESH_SCENE_OP_REGISTER_STATUS, ctx->addr,
				      (void **)&rsp)) {
		rsp->status = reg.status;
		rsp->current = reg.current;
		if (rsp->scenes) {
			rsp->count = MIN(rsp->count, reg.count);
			memcpy(rsp->scenes, reg.scenes,
			       rsp->count * sizeof(uint16_t));
		} else {
			rsp->count = 0;
		}

		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->scene_register) {
		cli->scene_register(cli, ctx, &reg);
	}

	/* Undo the endianness reversal, in case other Scene Clients need to
	 * read this message:
	 */
	for (int i = 0; i < reg.count; i++) {
		reg.scenes[i] = sys_cpu_to_le16(reg.scenes[i]);
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_scene_cli_op[] = {
	{
		BT_MESH_SCENE_OP_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_SCENE_MSG_MINLEN_STATUS),
		handle_scene_status,
	},
	{
		BT_MESH_SCENE_OP_REGISTER_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_SCENE_MSG_MINLEN_REGISTER_STATUS),
		handle_scene_reg,
	},
	BT_MESH_MODEL_OP_END,
};

static int scene_cli_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_scene_cli *cli = model->user_data;

	if (!cli) {
		return -EINVAL;
	}

	cli->model = model;

	net_buf_simple_init_with_data(&cli->pub_msg, cli->buf,
				      sizeof(cli->buf));
	cli->pub.msg = &cli->pub_msg;
	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);

	return 0;
}


static void scene_cli_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_scene_cli *cli = model->user_data;

	net_buf_simple_reset(cli->pub.msg);
	bt_mesh_msg_ack_ctx_reset(&cli->ack_ctx);
}

const struct bt_mesh_model_cb _bt_mesh_scene_cli_cb = {
	.init = scene_cli_init,
	.reset = scene_cli_reset,
};

int bt_mesh_scene_cli_get(struct bt_mesh_scene_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_scene_state *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCENE_OP_GET,
				 BT_MESH_SCENE_MSG_LEN_GET);
	bt_mesh_model_msg_init(&buf, BT_MESH_SCENE_OP_GET);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_SCENE_OP_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_scene_cli_register_get(struct bt_mesh_scene_cli *cli,
				   struct bt_mesh_msg_ctx *ctx,
				   struct bt_mesh_scene_register *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCENE_OP_REGISTER_GET,
				 BT_MESH_SCENE_MSG_LEN_REGISTER_GET);
	bt_mesh_model_msg_init(&buf, BT_MESH_SCENE_OP_REGISTER_GET);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_SCENE_OP_REGISTER_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_scene_cli_store(struct bt_mesh_scene_cli *cli,
			    struct bt_mesh_msg_ctx *ctx, uint16_t scene,
			    struct bt_mesh_scene_register *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCENE_OP_STORE,
				 BT_MESH_SCENE_MSG_LEN_STORE);
	bt_mesh_model_msg_init(&buf, BT_MESH_SCENE_OP_STORE);

	if (scene == BT_MESH_SCENE_NONE) {
		return -EINVAL;
	}

	net_buf_simple_add_le16(&buf, scene);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_SCENE_OP_REGISTER_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_scene_cli_store_unack(struct bt_mesh_scene_cli *cli,
				  struct bt_mesh_msg_ctx *ctx, uint16_t scene)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCENE_OP_STORE_UNACK,
				 BT_MESH_SCENE_MSG_LEN_STORE);
	bt_mesh_model_msg_init(&buf, BT_MESH_SCENE_OP_STORE_UNACK);

	if (scene == BT_MESH_SCENE_NONE) {
		return -EINVAL;
	}

	net_buf_simple_add_le16(&buf, scene);

	return bt_mesh_msg_send(cli->model, ctx, &buf);
}

int bt_mesh_scene_cli_delete(struct bt_mesh_scene_cli *cli,
			     struct bt_mesh_msg_ctx *ctx, uint16_t scene,
			     struct bt_mesh_scene_register *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCENE_OP_DELETE,
				 BT_MESH_SCENE_MSG_LEN_DELETE);
	bt_mesh_model_msg_init(&buf, BT_MESH_SCENE_OP_DELETE);

	if (scene == BT_MESH_SCENE_NONE) {
		return -EINVAL;
	}

	net_buf_simple_add_le16(&buf, scene);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_SCENE_OP_REGISTER_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_scene_cli_delete_unack(struct bt_mesh_scene_cli *cli,
				   struct bt_mesh_msg_ctx *ctx, uint16_t scene)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCENE_OP_DELETE_UNACK,
				 BT_MESH_SCENE_MSG_LEN_DELETE);
	bt_mesh_model_msg_init(&buf, BT_MESH_SCENE_OP_DELETE_UNACK);

	if (scene == BT_MESH_SCENE_NONE) {
		return -EINVAL;
	}

	net_buf_simple_add_le16(&buf, scene);

	return bt_mesh_msg_send(cli->model, ctx, &buf);
}

int bt_mesh_scene_cli_recall(struct bt_mesh_scene_cli *cli,
			     struct bt_mesh_msg_ctx *ctx, uint16_t scene,
			     const struct bt_mesh_model_transition *transition,
			     struct bt_mesh_scene_state *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCENE_OP_RECALL,
				 BT_MESH_SCENE_MSG_MAXLEN_RECALL);
	bt_mesh_model_msg_init(&buf, BT_MESH_SCENE_OP_RECALL);

	if (scene == BT_MESH_SCENE_NONE ||
	    model_transition_is_invalid(transition)) {
		return -EINVAL;
	}

	net_buf_simple_add_le16(&buf, scene);
	net_buf_simple_add_u8(&buf, cli->tid++);
	if (transition) {
		model_transition_buf_add(&buf, transition);
	}

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = BT_MESH_SCENE_OP_STATUS,
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &buf, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_scene_cli_recall_unack(
	struct bt_mesh_scene_cli *cli, struct bt_mesh_msg_ctx *ctx,
	uint16_t scene, const struct bt_mesh_model_transition *transition)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCENE_OP_RECALL_UNACK,
				 BT_MESH_SCENE_MSG_MAXLEN_RECALL);
	bt_mesh_model_msg_init(&buf, BT_MESH_SCENE_OP_RECALL_UNACK);

	if (scene == BT_MESH_SCENE_NONE ||
	    model_transition_is_invalid(transition)) {
		return -EINVAL;
	}

	net_buf_simple_add_le16(&buf, scene);
	net_buf_simple_add_u8(&buf, cli->tid++);
	if (transition) {
		model_transition_buf_add(&buf, transition);
	}

	return bt_mesh_msg_send(cli->model, ctx, &buf);
}
