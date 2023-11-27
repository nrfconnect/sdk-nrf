/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <bluetooth/mesh/gen_prop_cli.h>
#include <zephyr/sys/util.h>
#include "gen_prop_internal.h"
#include "model_utils.h"

BUILD_ASSERT(BT_MESH_MODEL_BUF_LEN(BT_MESH_PROP_OP_MFR_PROP_STATUS,
				   BT_MESH_PROP_MSG_MAXLEN_PROP_STATUS) <=
		     BT_MESH_RX_SDU_MAX,
	     "The property list must fit inside an application SDU.");
BUILD_ASSERT(BT_MESH_MODEL_BUF_LEN(BT_MESH_PROP_OP_MFR_PROPS_STATUS,
				   BT_MESH_PROP_MSG_MAXLEN_PROPS_STATUS) <=
		     BT_MESH_TX_SDU_MAX,
	     "The property value must fit inside an application SDU.");

struct prop_list_ctx {
	struct bt_mesh_prop_list *list;
	int status;
};

static int properties_status(const struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf,
			      enum bt_mesh_prop_srv_kind kind)
{
	if ((buf->len % 2) != 0) {
		return -EMSGSIZE;
	}

	struct bt_mesh_prop_cli *cli = model->rt->user_data;
	struct bt_mesh_prop_list list;
	struct prop_list_ctx *rsp;

	list.count = buf->len / 2;
	list.ids = (uint16_t *)net_buf_simple_pull_mem(buf, buf->len);

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, op_get(BT_MESH_PROP_OP_PROPS_STATUS, kind),
				      ctx->addr, (void **)&rsp)) {
		if (list.count > rsp->list->count) {
			/* Buffer can't hold all entries */
			rsp->status = -ENOBUFS;
			list.count = rsp->list->count;
		}

		memcpy(rsp->list->ids, list.ids, list.count * 2);
		rsp->list->count = list.count;
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->prop_list) {
		cli->prop_list(cli, ctx, kind, &list);
	}

	return 0;
}

static int handle_mfr_properties_status(const struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	return properties_status(model, ctx, buf, BT_MESH_PROP_SRV_KIND_MFR);
}

static int handle_admin_properties_status(const struct bt_mesh_model *model,
					  struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	return properties_status(model, ctx, buf, BT_MESH_PROP_SRV_KIND_ADMIN);
}

static int handle_user_properties_status(const struct bt_mesh_model *model,
					 struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	return properties_status(model, ctx, buf, BT_MESH_PROP_SRV_KIND_USER);
}

static int handle_client_properties_status(const struct bt_mesh_model *model,
					   struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	return properties_status(model, ctx, buf, BT_MESH_PROP_SRV_KIND_CLIENT);
}

static int property_status(const struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf,
			    enum bt_mesh_prop_srv_kind kind)
{
	if (buf->len < BT_MESH_PROP_MSG_MINLEN_PROP_STATUS ||
	    buf->len > BT_MESH_PROP_MSG_MAXLEN_PROP_STATUS) {
		return -EMSGSIZE;
	}

	struct bt_mesh_prop_cli *cli = model->rt->user_data;
	struct bt_mesh_prop_val val;
	struct bt_mesh_prop_val *rsp;

	val.meta.id = net_buf_simple_pull_le16(buf);
	val.meta.user_access = net_buf_simple_pull_u8(buf);
	val.size = buf->len;
	val.value = net_buf_simple_pull_mem(buf, val.size);

	if (bt_mesh_msg_ack_ctx_match(&cli->ack_ctx, op_get(BT_MESH_PROP_OP_PROP_STATUS, kind),
				      ctx->addr, (void **)&rsp)) {
		rsp->meta = val.meta;
		rsp->size = MIN(rsp->size, val.size);
		memcpy(rsp->value, val.value, rsp->size);
		bt_mesh_msg_ack_ctx_rx(&cli->ack_ctx);
	}

	if (cli->prop_status) {
		cli->prop_status(cli, ctx, kind, &val);
	}

	return 0;
}

static int handle_mfr_property_status(const struct bt_mesh_model *model,
				      struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	return property_status(model, ctx, buf, BT_MESH_PROP_SRV_KIND_MFR);
}

static int handle_admin_property_status(const struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	return property_status(model, ctx, buf, BT_MESH_PROP_SRV_KIND_ADMIN);
}

static int handle_user_property_status(const struct bt_mesh_model *model,
				       struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	return property_status(model, ctx, buf, BT_MESH_PROP_SRV_KIND_USER);
}

const struct bt_mesh_model_op _bt_mesh_prop_cli_op[] = {
	{
		BT_MESH_PROP_OP_MFR_PROPS_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_PROP_MSG_MINLEN_PROPS_STATUS),
		handle_mfr_properties_status,
	},
	{
		BT_MESH_PROP_OP_ADMIN_PROPS_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_PROP_MSG_MINLEN_PROPS_STATUS),
		handle_admin_properties_status,
	},
	{
		BT_MESH_PROP_OP_USER_PROPS_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_PROP_MSG_MINLEN_PROPS_STATUS),
		handle_user_properties_status,
	},
	{
		BT_MESH_PROP_OP_CLIENT_PROPS_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_PROP_MSG_MINLEN_PROPS_STATUS),
		handle_client_properties_status,
	},
	{
		BT_MESH_PROP_OP_MFR_PROP_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_PROP_MSG_MINLEN_PROP_STATUS),
		handle_mfr_property_status,
	},
	{
		BT_MESH_PROP_OP_ADMIN_PROP_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_PROP_MSG_MINLEN_PROP_STATUS),
		handle_admin_property_status,
	},
	{
		BT_MESH_PROP_OP_USER_PROP_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_PROP_MSG_MINLEN_PROP_STATUS),
		handle_user_property_status,
	},
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_prop_cli_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_prop_cli *cli = model->rt->user_data;

	cli->model = model;
	cli->pub.msg = &cli->pub_buf;
	net_buf_simple_init_with_data(&cli->pub_buf, cli->pub_data,
				      sizeof(cli->pub_data));
	bt_mesh_msg_ack_ctx_init(&cli->ack_ctx);

	return 0;
}

static void bt_mesh_prop_cli_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_prop_cli *cli = model->rt->user_data;

	net_buf_simple_reset(model->pub->msg);
	bt_mesh_msg_ack_ctx_reset(&cli->ack_ctx);
}

const struct bt_mesh_model_cb _bt_mesh_prop_cli_cb = {
	.init = bt_mesh_prop_cli_init,
	.reset = bt_mesh_prop_cli_reset,
};

static int props_get(struct bt_mesh_prop_cli *cli, struct bt_mesh_msg_ctx *ctx,
		     enum bt_mesh_prop_srv_kind kind, uint16_t id,
		     struct bt_mesh_prop_list *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PROP_OP_ADMIN_PROPS_GET,
				 BT_MESH_PROP_MSG_LEN_CLIENT_PROPS_GET);

	bt_mesh_model_msg_init(&msg, op_get(BT_MESH_PROP_OP_PROPS_GET, kind));

	if (kind == BT_MESH_PROP_SRV_KIND_CLIENT) {
		net_buf_simple_add_le16(&msg, id);
	}

	struct prop_list_ctx block_ctx = {
		.list = rsp,
	};
	int status;
	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = op_get(BT_MESH_PROP_OP_PROPS_STATUS, kind),
		.user_data = &block_ctx,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	status = bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);

	if (status == 0) {
		status = block_ctx.status;
	}
	return status;
}

int bt_mesh_prop_cli_client_props_get(struct bt_mesh_prop_cli *cli,
				      struct bt_mesh_msg_ctx *ctx, uint16_t id,
				      struct bt_mesh_prop_list *rsp)
{
	return props_get(cli, ctx, BT_MESH_PROP_SRV_KIND_CLIENT, id, rsp);
}

int bt_mesh_prop_cli_props_get(struct bt_mesh_prop_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       enum bt_mesh_prop_srv_kind kind,
			       struct bt_mesh_prop_list *rsp)
{
	return props_get(cli, ctx, kind, 0x0000, rsp);
}

int bt_mesh_prop_cli_prop_get(struct bt_mesh_prop_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      enum bt_mesh_prop_srv_kind kind, uint16_t id,
			      struct bt_mesh_prop_val *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PROP_OP_ADMIN_PROP_GET,
				 BT_MESH_PROP_MSG_LEN_PROP_GET);

	bt_mesh_model_msg_init(&msg, op_get(BT_MESH_PROP_OP_PROP_GET, kind));
	net_buf_simple_add_le16(&msg, id);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = op_get(BT_MESH_PROP_OP_PROP_STATUS, kind),
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_prop_cli_user_prop_set(struct bt_mesh_prop_cli *cli,
				   struct bt_mesh_msg_ctx *ctx,
				   const struct bt_mesh_prop_val *val,
				   struct bt_mesh_prop_val *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PROP_OP_USER_PROP_SET,
				 BT_MESH_PROP_MSG_MAXLEN_USER_PROP_SET);

	bt_mesh_model_msg_init(&msg, BT_MESH_PROP_OP_USER_PROP_SET);
	net_buf_simple_add_le16(&msg, val->meta.id);
	net_buf_simple_add_mem(&msg, val->value, val->size);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = op_get(BT_MESH_PROP_OP_PROP_STATUS, BT_MESH_PROP_SRV_KIND_USER),
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_prop_cli_user_prop_set_unack(struct bt_mesh_prop_cli *cli,
					 struct bt_mesh_msg_ctx *ctx,
					 const struct bt_mesh_prop_val *val)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PROP_OP_USER_PROP_SET_UNACK,
				 BT_MESH_PROP_MSG_MAXLEN_USER_PROP_SET);

	bt_mesh_model_msg_init(&msg, BT_MESH_PROP_OP_USER_PROP_SET_UNACK);
	net_buf_simple_add_le16(&msg, val->meta.id);
	net_buf_simple_add_mem(&msg, val->value, val->size);

	return bt_mesh_msg_send(cli->model, ctx, &msg);
}

int bt_mesh_prop_cli_admin_prop_set(struct bt_mesh_prop_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    const struct bt_mesh_prop_val *val,
				    struct bt_mesh_prop_val *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PROP_OP_ADMIN_PROP_SET,
				 BT_MESH_PROP_MSG_MAXLEN_ADMIN_PROP_SET);

	bt_mesh_model_msg_init(&msg, BT_MESH_PROP_OP_ADMIN_PROP_SET);
	net_buf_simple_add_le16(&msg, val->meta.id);
	net_buf_simple_add_u8(&msg, val->meta.user_access);
	net_buf_simple_add_mem(&msg, val->value, val->size);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = op_get(BT_MESH_PROP_OP_PROP_STATUS, BT_MESH_PROP_SRV_KIND_ADMIN),
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_prop_cli_admin_prop_set_unack(struct bt_mesh_prop_cli *cli,
					  struct bt_mesh_msg_ctx *ctx,
					  const struct bt_mesh_prop_val *val)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PROP_OP_ADMIN_PROP_SET_UNACK,
				 BT_MESH_PROP_MSG_MAXLEN_ADMIN_PROP_SET);

	bt_mesh_model_msg_init(&msg, BT_MESH_PROP_OP_ADMIN_PROP_SET_UNACK);
	net_buf_simple_add_le16(&msg, val->meta.id);
	net_buf_simple_add_u8(&msg, val->meta.user_access);
	net_buf_simple_add_mem(&msg, val->value, val->size);

	return bt_mesh_msg_send(cli->model, ctx, &msg);
}

int bt_mesh_prop_cli_mfr_prop_set(struct bt_mesh_prop_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  const struct bt_mesh_prop *prop,
				  struct bt_mesh_prop_val *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PROP_OP_MFR_PROP_SET,
				 BT_MESH_PROP_MSG_LEN_MFR_PROP_SET);

	bt_mesh_model_msg_init(&msg, BT_MESH_PROP_OP_MFR_PROP_SET);
	net_buf_simple_add_le16(&msg, prop->id);
	net_buf_simple_add_u8(&msg, prop->user_access);

	struct bt_mesh_msg_rsp_ctx rsp_ctx = {
		.ack = &cli->ack_ctx,
		.op = op_get(BT_MESH_PROP_OP_PROP_STATUS, BT_MESH_PROP_SRV_KIND_MFR),
		.user_data = rsp,
		.timeout = model_ackd_timeout_get(cli->model, ctx),
	};

	return bt_mesh_msg_ackd_send(cli->model, ctx, &msg, rsp ? &rsp_ctx : NULL);
}

int bt_mesh_prop_cli_mfr_prop_set_unack(struct bt_mesh_prop_cli *cli,
					struct bt_mesh_msg_ctx *ctx,
					const struct bt_mesh_prop *prop)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PROP_OP_MFR_PROP_SET_UNACK,
				 BT_MESH_PROP_MSG_LEN_MFR_PROP_SET);

	bt_mesh_model_msg_init(&msg, BT_MESH_PROP_OP_MFR_PROP_SET_UNACK);
	net_buf_simple_add_le16(&msg, prop->id);
	net_buf_simple_add_u8(&msg, prop->user_access);

	return bt_mesh_msg_send(cli->model, ctx, &msg);
}
