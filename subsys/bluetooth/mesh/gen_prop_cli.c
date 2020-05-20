/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <bluetooth/mesh/gen_prop_cli.h>
#include <sys/util.h>
#include "gen_prop_internal.h"
#include "model_utils.h"
#include "mesh/net.h"
#include "mesh/transport.h"

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

static void properties_status(struct bt_mesh_model *mod,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf,
			      enum bt_mesh_prop_srv_kind kind)
{
	if ((buf->len % 2) != 0) {
		return;
	}

	struct bt_mesh_prop_cli *cli = mod->user_data;
	struct bt_mesh_prop_list list;

	list.count = buf->len / 2;
	list.ids = (u16_t *)net_buf_simple_pull_mem(buf, buf->len);

	if (model_ack_match(&cli->ack_ctx,
			    op_get(BT_MESH_PROP_OP_PROPS_STATUS, kind), ctx)) {
		struct prop_list_ctx *rsp = cli->ack_ctx.user_data;

		if (list.count > rsp->list->count) {
			/* Buffer can't hold all entries */
			rsp->status = -ENOBUFS;
			list.count = rsp->list->count;
		}

		memcpy(rsp->list->ids, list.ids, list.count * 2);
		rsp->list->count = list.count;
	}

	if (cli->prop_list) {
		cli->prop_list(cli, ctx, kind, &list);
	}
}

static void handle_mfr_properties_status(struct bt_mesh_model *mod,
					 struct bt_mesh_msg_ctx *ctx,
					 struct net_buf_simple *buf)
{
	properties_status(mod, ctx, buf, BT_MESH_PROP_SRV_KIND_MFR);
}

static void handle_admin_properties_status(struct bt_mesh_model *mod,
					   struct bt_mesh_msg_ctx *ctx,
					   struct net_buf_simple *buf)
{
	properties_status(mod, ctx, buf, BT_MESH_PROP_SRV_KIND_ADMIN);
}

static void handle_user_properties_status(struct bt_mesh_model *mod,
					  struct bt_mesh_msg_ctx *ctx,
					  struct net_buf_simple *buf)
{
	properties_status(mod, ctx, buf, BT_MESH_PROP_SRV_KIND_USER);
}

static void property_status(struct bt_mesh_model *mod,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf,
			    enum bt_mesh_prop_srv_kind kind)
{
	if (buf->len < BT_MESH_PROP_MSG_MINLEN_PROP_STATUS ||
	    buf->len > BT_MESH_PROP_MSG_MAXLEN_PROP_STATUS) {
		return;
	}

	struct bt_mesh_prop_cli *cli = mod->user_data;
	struct bt_mesh_prop_val val;

	val.meta.id = net_buf_simple_pull_le16(buf);
	val.meta.user_access = net_buf_simple_pull_u8(buf);
	val.size = MIN(buf->len, sizeof(val.value));
	memcpy(val.value, net_buf_simple_pull_mem(buf, val.size), val.size);

	if (model_ack_match(&cli->ack_ctx,
			    op_get(BT_MESH_PROP_OP_PROP_STATUS, kind), ctx)) {
		struct bt_mesh_prop_val *rsp = cli->ack_ctx.user_data;

		*rsp = val;
	}

	if (cli->prop_list) {
		cli->prop_status(cli, ctx, kind, &val);
	}
}

static void handle_mfr_property_status(struct bt_mesh_model *mod,
				       struct bt_mesh_msg_ctx *ctx,
				       struct net_buf_simple *buf)
{
	property_status(mod, ctx, buf, BT_MESH_PROP_SRV_KIND_MFR);
}

static void handle_admin_property_status(struct bt_mesh_model *mod,
					 struct bt_mesh_msg_ctx *ctx,
					 struct net_buf_simple *buf)
{
	property_status(mod, ctx, buf, BT_MESH_PROP_SRV_KIND_ADMIN);
}

static void handle_user_property_status(struct bt_mesh_model *mod,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	property_status(mod, ctx, buf, BT_MESH_PROP_SRV_KIND_USER);
}

const struct bt_mesh_model_op _bt_mesh_prop_cli_op[] = {
	{ BT_MESH_PROP_OP_MFR_PROPS_STATUS,
	  BT_MESH_PROP_MSG_MINLEN_PROPS_STATUS, handle_mfr_properties_status },
	{ BT_MESH_PROP_OP_ADMIN_PROPS_STATUS,
	  BT_MESH_PROP_MSG_MINLEN_PROPS_STATUS,
	  handle_admin_properties_status },
	{ BT_MESH_PROP_OP_USER_PROPS_STATUS,
	  BT_MESH_PROP_MSG_MINLEN_PROPS_STATUS, handle_user_properties_status },
	{ BT_MESH_PROP_OP_MFR_PROP_STATUS, BT_MESH_PROP_MSG_MINLEN_PROP_STATUS,
	  handle_mfr_property_status },
	{ BT_MESH_PROP_OP_ADMIN_PROP_STATUS,
	  BT_MESH_PROP_MSG_MINLEN_PROP_STATUS, handle_admin_property_status },
	{ BT_MESH_PROP_OP_USER_PROP_STATUS, BT_MESH_PROP_MSG_MINLEN_PROP_STATUS,
	  handle_user_property_status },
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_prop_cli_init(struct bt_mesh_model *mod)
{
	struct bt_mesh_prop_cli *cli = mod->user_data;

	cli->model = mod;
	net_buf_simple_init(mod->pub->msg, 0);
	model_ack_init(&cli->ack_ctx);

	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_prop_cli_cb = {
	.init = bt_mesh_prop_cli_init,
};

int bt_mesh_prop_cli_props_get(struct bt_mesh_prop_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       enum bt_mesh_prop_srv_kind kind,
			       struct bt_mesh_prop_list *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PROP_OP_ADMIN_PROPS_GET,
				 BT_MESH_PROP_MSG_LEN_PROPS_GET);

	bt_mesh_model_msg_init(&msg, op_get(BT_MESH_PROP_OP_PROPS_GET, kind));

	struct prop_list_ctx block_ctx = {
		.list = rsp,
	};
	int status = model_ackd_send(cli->model, ctx, &msg,
				     rsp ? &cli->ack_ctx : NULL,
				     op_get(BT_MESH_PROP_OP_PROPS_STATUS, kind),
				     &block_ctx);

	if (status == 0) {
		status = block_ctx.status;
	}
	return status;
}

int bt_mesh_prop_cli_prop_get(struct bt_mesh_prop_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      enum bt_mesh_prop_srv_kind kind, u16_t id,
			      struct bt_mesh_prop_val *rsp)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PROP_OP_ADMIN_PROP_GET,
				 BT_MESH_PROP_MSG_LEN_PROP_GET);

	bt_mesh_model_msg_init(&msg, op_get(BT_MESH_PROP_OP_PROP_GET, kind));
	net_buf_simple_add_le16(&msg, id);

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       op_get(BT_MESH_PROP_OP_PROP_STATUS, kind), rsp);
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

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_PROP_OP_PROPS_STATUS, rsp);
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

	return model_send(cli->model, ctx, &msg);
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

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_PROP_OP_PROPS_STATUS, rsp);
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

	return model_send(cli->model, ctx, &msg);
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

	return model_ackd_send(cli->model, ctx, &msg,
			       rsp ? &cli->ack_ctx : NULL,
			       BT_MESH_PROP_OP_PROPS_STATUS, rsp);
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

	return model_send(cli->model, ctx, &msg);
}
