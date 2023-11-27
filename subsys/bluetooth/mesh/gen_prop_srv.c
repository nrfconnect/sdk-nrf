/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <bluetooth/mesh/gen_prop_srv.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include "gen_prop_internal.h"
#include "model_utils.h"
#include "mesh/net.h"
#include "mesh/transport.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_mesh_gen_prop_srv);

#define PROP_FOREACH(_srv, _prop)                                              \
	for (_prop = &(_srv)->properties[0];                                   \
	     _prop != &(_srv)->properties[(_srv)->property_count]; _prop++)

#define IS_MFR_SRV(_mod)                                                       \
	((_mod)->id == BT_MESH_MODEL_ID_GEN_MANUFACTURER_PROP_SRV)

BUILD_ASSERT(BT_MESH_MODEL_BUF_LEN(BT_MESH_PROP_OP_MFR_PROP_STATUS,
				   BT_MESH_PROP_MSG_MAXLEN_PROP_STATUS) <=
		     BT_MESH_RX_SDU_MAX,
	     "The property list must fit inside an application SDU.");
BUILD_ASSERT(BT_MESH_MODEL_BUF_LEN(BT_MESH_PROP_OP_MFR_PROPS_STATUS,
				   BT_MESH_PROP_MSG_MAXLEN_PROPS_STATUS) <=
		     BT_MESH_TX_SDU_MAX,
	     "The property value must fit inside an application SDU.");

static struct bt_mesh_prop *prop_get(const struct bt_mesh_prop_srv *srv,
				     uint16_t id)
{
	if (!srv || id == BT_MESH_PROP_ID_PROHIBITED) {
		return NULL;
	}

	struct bt_mesh_prop *prop;

	PROP_FOREACH(srv, prop)
	{
		if (prop->id == id) {
			return prop;
		}
	}

	return NULL;
}

#if CONFIG_BT_SETTINGS
static void bt_mesh_prop_srv_pending_store(const struct bt_mesh_model *model)
{
	struct bt_mesh_prop_srv *srv = model->rt->user_data;

	uint8_t user_access[CONFIG_BT_MESH_PROP_MAXCOUNT];

	for (uint32_t i = 0; i < srv->property_count; ++i) {
		user_access[i] = srv->properties[i].user_access;
	}

	(void)bt_mesh_model_data_store(srv->model, false, NULL, user_access,
				       srv->property_count);

}
#endif

static void store_props(struct bt_mesh_prop_srv *srv)
{
#if CONFIG_BT_SETTINGS
	bt_mesh_model_data_store_schedule(srv->model);
#endif
}

static void set_user_access(struct bt_mesh_prop_srv *srv,
			    struct bt_mesh_prop *prop,
			    enum bt_mesh_prop_access user_access)
{
	enum bt_mesh_prop_access old_access = prop->user_access;

	if (IS_MFR_SRV(srv->model)) {
		/* Cannot write to manufacturer properties */
		user_access &= ~BT_MESH_PROP_ACCESS_WRITE;
	}

	prop->user_access = user_access;

	if (old_access != user_access) {
		store_props(srv);
	}
}

static void pub_list_build(const struct bt_mesh_prop_srv *srv,
			   struct net_buf_simple *buf, uint16_t start_prop)
{
	bt_mesh_model_msg_init(buf, op_get(BT_MESH_PROP_OP_PROPS_STATUS,
					   srv_kind(srv->model)));

	struct bt_mesh_prop *prop;

	PROP_FOREACH(srv, prop)
	{
		if (prop->id >= start_prop) {
			net_buf_simple_add_le16(buf, prop->id);
		}
	}
}

/* Owner properties */

static int handle_owner_properties_get(const struct bt_mesh_model *model,
				       struct bt_mesh_msg_ctx *ctx,
				       struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PROP_OP_ADMIN_PROPS_STATUS,
				 BT_MESH_PROP_MSG_MAXLEN_PROPS_STATUS);

	pub_list_build(model->rt->user_data, &rsp, 1);
	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

static int handle_owner_property_get(const struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	struct bt_mesh_prop_srv *srv = model->rt->user_data;
	uint16_t id = net_buf_simple_pull_le16(buf);

	if (id == BT_MESH_PROP_ID_PROHIBITED) {
		return -EINVAL;
	}

	const struct bt_mesh_prop *prop = prop_get(srv, id);

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PROP_OP_ADMIN_PROP_STATUS,
				 BT_MESH_PROP_MSG_MAXLEN_PROP_STATUS);

	bt_mesh_model_msg_init(&rsp, op_get(BT_MESH_PROP_OP_PROP_STATUS,
					    srv_kind(model)));
	net_buf_simple_add_le16(&rsp, id);

	if (!prop) {
		/* If the prop doesn't exist, we should only send the ID as a
		 * response.
		 */
		goto respond;
	}

	net_buf_simple_add_u8(&rsp, prop->user_access);

	struct bt_mesh_prop_val value = {
		.meta = *prop,
		.value = net_buf_simple_tail(&rsp),
		.size = CONFIG_BT_MESH_PROP_MAXSIZE,
	};

	srv->get(srv, ctx, &value);

	net_buf_simple_add(&rsp, value.size);

respond:
	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

static int owner_property_set(const struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf, bool ack)
{
	if ((IS_MFR_SRV(model) && (buf->len != 3)) ||
	    (buf->len > BT_MESH_PROP_MSG_MAXLEN_ADMIN_PROP_SET ||
	     buf->len < BT_MESH_PROP_MSG_MINLEN_ADMIN_PROP_SET)) {
		return -EMSGSIZE;
	}

	struct bt_mesh_prop_srv *srv = model->rt->user_data;
	uint16_t id = net_buf_simple_pull_le16(buf);
	enum bt_mesh_prop_access user_access = net_buf_simple_pull_u8(buf);

	if (id == BT_MESH_PROP_ID_PROHIBITED) {
		return -EINVAL;
	}

	if ((IS_MFR_SRV(model) && (user_access & BT_MESH_PROP_ACCESS_WRITE)) ||
	    user_access > BT_MESH_PROP_ACCESS_READ_WRITE) {
		return -EPERM;
	}

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PROP_OP_ADMIN_PROP_STATUS,
				 BT_MESH_PROP_MSG_MAXLEN_PROP_STATUS);

	bt_mesh_model_msg_init(&rsp, op_get(BT_MESH_PROP_OP_PROP_STATUS,
					    srv_kind(model)));
	net_buf_simple_add_le16(&rsp, id);

	struct bt_mesh_prop *prop = prop_get(srv, id);

	if (!prop) {
		/* If the prop doesn't exist, we should only send the ID as a
		 * response.
		 */
		goto respond;
	}

	set_user_access(srv, prop, user_access);

	net_buf_simple_add_u8(&rsp, prop->user_access);

	struct bt_mesh_prop_val value = {
		.meta = *prop,
		.value = net_buf_simple_tail(&rsp),
	};

	if (IS_MFR_SRV(model)) {
		/* Since the manufacturer properties can't be set, we'll just
		 * call the getter to fetch the response value.
		 */
		value.size = CONFIG_BT_MESH_PROP_MAXSIZE;
		srv->get(srv, ctx, &value);
	} else {
		value.size = buf->len;
		memcpy(value.value, net_buf_simple_pull_mem(buf, value.size),
		       value.size);
		srv->set(srv, ctx, &value);

		if (value.meta.id == BT_MESH_PROP_ID_PROHIBITED) {
			/* User callbacks wipe the id to indicate invalid
			 * behavior
			 */
			return -EINVAL;
		}
	}

	bt_mesh_prop_srv_pub(srv, NULL, &value);

	net_buf_simple_add(&rsp, value.size);
respond:
	if (ack) {
		bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);
	}

	return 0;
}

static int handle_owner_property_set(const struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	return owner_property_set(model, ctx, buf, true);
}

static int handle_owner_property_set_unack(const struct bt_mesh_model *model,
					   struct bt_mesh_msg_ctx *ctx,
					   struct net_buf_simple *buf)
{
	return owner_property_set(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_prop_admin_srv_op[] = {
	{
		BT_MESH_PROP_OP_ADMIN_PROPS_GET,
		BT_MESH_LEN_EXACT(BT_MESH_PROP_MSG_LEN_PROPS_GET),
		handle_owner_properties_get,
	},
	{
		BT_MESH_PROP_OP_ADMIN_PROP_GET,
		BT_MESH_LEN_EXACT(BT_MESH_PROP_MSG_LEN_PROP_GET),
		handle_owner_property_get,
	},
	{
		BT_MESH_PROP_OP_ADMIN_PROP_SET,
		BT_MESH_LEN_MIN(BT_MESH_PROP_MSG_MINLEN_ADMIN_PROP_SET),
		handle_owner_property_set,
	},
	{
		BT_MESH_PROP_OP_ADMIN_PROP_SET_UNACK,
		BT_MESH_LEN_MIN(BT_MESH_PROP_MSG_MINLEN_ADMIN_PROP_SET),
		handle_owner_property_set_unack,
	},
	BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op _bt_mesh_prop_mfr_srv_op[] = {
	{
		BT_MESH_PROP_OP_MFR_PROPS_GET,
		BT_MESH_LEN_EXACT(BT_MESH_PROP_MSG_LEN_PROPS_GET),
		handle_owner_properties_get,
	},
	{
		BT_MESH_PROP_OP_MFR_PROP_GET,
		BT_MESH_LEN_EXACT(BT_MESH_PROP_MSG_LEN_PROP_GET),
		handle_owner_property_get,
	},
	{
		BT_MESH_PROP_OP_MFR_PROP_SET,
		BT_MESH_LEN_EXACT(BT_MESH_PROP_MSG_LEN_MFR_PROP_SET),
		handle_owner_property_set,
	},
	{
		BT_MESH_PROP_OP_MFR_PROP_SET_UNACK,
		BT_MESH_LEN_EXACT(BT_MESH_PROP_MSG_LEN_MFR_PROP_SET),
		handle_owner_property_set_unack,
	},
	BT_MESH_MODEL_OP_END,
};

static int handle_client_properties_get(const struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	uint16_t start_prop = net_buf_simple_pull_le16(buf);

	if (start_prop == BT_MESH_PROP_ID_PROHIBITED) {
		return -EINVAL;
	}

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PROP_OP_CLIENT_PROPS_STATUS,
				 BT_MESH_PROP_MSG_MAXLEN_PROPS_STATUS);

	pub_list_build(model->rt->user_data, &rsp, start_prop);
	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_prop_client_srv_op[] = {
	{
		BT_MESH_PROP_OP_CLIENT_PROPS_GET,
		BT_MESH_LEN_EXACT(BT_MESH_PROP_MSG_LEN_CLIENT_PROPS_GET),
		handle_client_properties_get,
	},
	BT_MESH_MODEL_OP_END,
};

/* User properties */

static struct bt_mesh_prop_srv *prop_srv_get(const struct bt_mesh_model *user_srv_mod,
					     uint16_t model_id)
{
	const struct bt_mesh_model *model =
		bt_mesh_model_find(bt_mesh_model_elem(user_srv_mod), model_id);

	return model ? model->rt->user_data : NULL;
}

static struct bt_mesh_prop *user_prop_get(const struct bt_mesh_model *model, uint16_t id,
					  struct bt_mesh_prop_srv **srv)
{
	struct bt_mesh_prop_srv *const srvs[] = {
		prop_srv_get(model, BT_MESH_MODEL_ID_GEN_MANUFACTURER_PROP_SRV),
		prop_srv_get(model, BT_MESH_MODEL_ID_GEN_ADMIN_PROP_SRV),
	};

	for (int i = 0; i < ARRAY_SIZE(srvs); ++i) {
		struct bt_mesh_prop *prop = prop_get(srvs[i], id);

		if (prop &&
		    prop->user_access != BT_MESH_PROP_ACCESS_PROHIBITED) {
			*srv = srvs[i];
			return prop;
		}
	}

	*srv = NULL;
	return NULL;
}

static int handle_user_properties_get(const struct bt_mesh_model *model,
				      struct bt_mesh_msg_ctx *ctx,
				      struct net_buf_simple *buf)
{
	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PROP_OP_PROPS_STATUS,
				 BT_MESH_PROP_MSG_MAXLEN_PROPS_STATUS);

	bt_mesh_model_msg_init(&rsp, BT_MESH_PROP_OP_USER_PROPS_STATUS);

	const struct bt_mesh_prop_srv *srvs[] = {
		prop_srv_get(model, BT_MESH_MODEL_ID_GEN_MANUFACTURER_PROP_SRV),
		prop_srv_get(model, BT_MESH_MODEL_ID_GEN_ADMIN_PROP_SRV),
	};

	for (int i = 0; i < ARRAY_SIZE(srvs); ++i) {
		if (!srvs[i]) {
			continue;
		}

		struct bt_mesh_prop *prop;

		PROP_FOREACH(srvs[i], prop)
		{
			if (net_buf_simple_tailroom(&rsp) < 6) {
				break;
			}

			if (prop->user_access ==
			    BT_MESH_PROP_ACCESS_PROHIBITED) {
				continue;
			}

			net_buf_simple_add_le16(&rsp, prop->id);
		}
	}

	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

static int handle_user_property_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	uint16_t id = net_buf_simple_pull_le16(buf);

	if (id == BT_MESH_PROP_ID_PROHIBITED) {
		return -EINVAL;
	}

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PROP_OP_USER_PROP_STATUS,
				 BT_MESH_PROP_MSG_MAXLEN_PROP_STATUS);

	bt_mesh_model_msg_init(&rsp, BT_MESH_PROP_OP_USER_PROP_STATUS);
	net_buf_simple_add_le16(&rsp, id);

	struct bt_mesh_prop_srv *srv;
	struct bt_mesh_prop *prop = user_prop_get(model, id, &srv);

	if (!prop) {
		/* If the prop doesn't exist, we should only send the ID as a
		 * response.
		 */
		goto respond;
	}

	net_buf_simple_add_u8(&rsp, prop->user_access);

	if (prop->user_access & BT_MESH_PROP_ACCESS_READ) {
		struct bt_mesh_prop_val value = {
			.meta = *prop,
			.value = net_buf_simple_tail(&rsp),
			.size = CONFIG_BT_MESH_PROP_MAXSIZE,
		};

		srv->get(srv, ctx, &value);

		net_buf_simple_add(&rsp, value.size);
	}

respond:
	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

static int user_property_set(const struct bt_mesh_model *model,
			      struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf, bool ack)
{
	if (buf->len > BT_MESH_PROP_MSG_MAXLEN_USER_PROP_SET) {
		return -EMSGSIZE;
	}

	uint16_t id = net_buf_simple_pull_le16(buf);

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_PROP_OP_USER_PROP_STATUS,
				 BT_MESH_PROP_MSG_MAXLEN_PROP_STATUS);

	bt_mesh_model_msg_init(&rsp, BT_MESH_PROP_OP_USER_PROP_STATUS);

	net_buf_simple_add_le16(&rsp, id);

	if (id == BT_MESH_PROP_ID_PROHIBITED) {
		return -EINVAL;
	}

	struct bt_mesh_prop_srv *user_srv = model->rt->user_data;
	struct bt_mesh_prop_srv *owner_srv;
	struct bt_mesh_prop *prop = user_prop_get(model, id, &owner_srv);

	if (!prop) {
		/* If the prop doesn't exist, we should only send the ID as a
		 * response.
		 */
		goto respond;
	}

	net_buf_simple_add_u8(&rsp, prop->user_access);

	if (!(prop->user_access & BT_MESH_PROP_ACCESS_WRITE) ||
	    IS_MFR_SRV(owner_srv->model)) {
		/* IF write is not supported, we should only send the ID
		 * and user_access as a response.
		 */
		goto respond;
	}

	struct bt_mesh_prop_val value = {
		.meta = *prop,
		.value = net_buf_simple_tail(&rsp),
		.size = buf->len,
	};

	memcpy(value.value, net_buf_simple_pull_mem(buf, value.size),
	       value.size);

	owner_srv->set(owner_srv, ctx, &value);

	if (value.meta.id == BT_MESH_PROP_ID_PROHIBITED) {
		/* User callbacks wipe the id to indicate invalid behavior */
		return -EINVAL;
	}

	net_buf_simple_add(&rsp, value.size);

	bt_mesh_prop_srv_pub(user_srv, NULL, &value);

respond:
	if (ack) {
		bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);
	}

	return 0;
}

static int handle_user_property_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	return user_property_set(model, ctx, buf, true);
}

static int handle_user_property_set_unack(const struct bt_mesh_model *model,
					  struct bt_mesh_msg_ctx *ctx,
					  struct net_buf_simple *buf)
{
	return user_property_set(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_prop_user_srv_op[] = {
	{
		BT_MESH_PROP_OP_USER_PROPS_GET,
		BT_MESH_LEN_EXACT(BT_MESH_PROP_MSG_LEN_PROPS_GET),
		handle_user_properties_get,
	},
	{
		BT_MESH_PROP_OP_USER_PROP_GET,
		BT_MESH_LEN_EXACT(BT_MESH_PROP_MSG_LEN_PROP_GET),
		handle_user_property_get,
	},
	{
		BT_MESH_PROP_OP_USER_PROP_SET,
		BT_MESH_LEN_MIN(BT_MESH_PROP_MSG_MINLEN_USER_PROP_SET),
		handle_user_property_set,
	},
	{
		BT_MESH_PROP_OP_USER_PROP_SET_UNACK,
		BT_MESH_LEN_MIN(BT_MESH_PROP_MSG_MINLEN_USER_PROP_SET),
		handle_user_property_set_unack,
	},
	BT_MESH_MODEL_OP_END,
};

static int update_handler(const struct bt_mesh_model *model)
{
	struct bt_mesh_prop_srv *srv = model->rt->user_data;
	struct bt_mesh_prop_val value;
	struct bt_mesh_prop *prop;

	switch (srv->pub_state) {
	case BT_MESH_PROP_SRV_STATE_NONE:
		return 0;
	case BT_MESH_PROP_SRV_STATE_LIST:
		pub_list_build(srv, srv->pub.msg, 1);
		return 0;
	case BT_MESH_PROP_SRV_STATE_PROP:
		bt_mesh_model_msg_init(srv->pub.msg,
				       op_get(BT_MESH_PROP_OP_PROP_STATUS,
					      srv_kind(model)));
		net_buf_simple_add_le16(srv->pub.msg, srv->pub_id);

		prop = prop_get(srv, srv->pub_id);
		if (!prop) {
			return -ENOENT;
		}

		net_buf_simple_add_u8(srv->pub.msg, prop->user_access);

		value.meta = *prop;
		value.value = net_buf_simple_tail(srv->pub.msg);
		value.size = CONFIG_BT_MESH_PROP_MAXSIZE;

		srv->get(srv, NULL, &value);

		net_buf_simple_add(srv->pub.msg, value.size);
	}

	return 0;
}

static int bt_mesh_prop_srv_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_prop_srv *srv = model->rt->user_data;

	srv->model = model;
	srv->pub.msg = &srv->pub_buf;
	srv->pub.update = update_handler;
	net_buf_simple_init_with_data(&srv->pub_buf, srv->pub_data,
				      sizeof(srv->pub_data));

	if (srv->property_count > CONFIG_BT_MESH_PROP_MAXCOUNT) {
		LOG_ERR("Property counter is larger than max allowed value");
		return -EINVAL;
	}

	struct bt_mesh_prop *prop;

	PROP_FOREACH(srv, prop)
	{
		if (prop->id == BT_MESH_PROP_ID_PROHIBITED) {
			LOG_ERR("Property ID cannot be 0");
			return -EINVAL;
		}
	}

	if ((model->id == BT_MESH_MODEL_ID_GEN_MANUFACTURER_PROP_SRV ||
	     model->id == BT_MESH_MODEL_ID_GEN_ADMIN_PROP_SRV)) {
		int err;
		const struct bt_mesh_model *usr_prop_srv = bt_mesh_model_find(
			bt_mesh_model_elem(model), BT_MESH_MODEL_ID_GEN_USER_PROP_SRV);

		if (!usr_prop_srv) {
			LOG_ERR("Failed to find Generic User Property Server on element");
			return -EINVAL;
		}

		err = bt_mesh_model_extend(model, usr_prop_srv);
		if (err) {
			return err;
		}
	}

	if (IS_MFR_SRV(model)) {
		/* Manufacturer properties aren't writable */
		struct bt_mesh_prop *prop;

		PROP_FOREACH(srv, prop)
		{
			prop->user_access &= ~BT_MESH_PROP_ACCESS_WRITE;
		}
	}
	return 0;
}

static void bt_mesh_prop_srv_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_prop_srv *srv = model->rt->user_data;

	net_buf_simple_reset(srv->pub.msg);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		(void)bt_mesh_model_data_store(srv->model, false, NULL, NULL, 0);
	}
}

#ifdef CONFIG_BT_SETTINGS
static int bt_mesh_prop_srv_settings_set(const struct bt_mesh_model *model,
					 const char *name, size_t len_rd,
					 settings_read_cb read_cb, void *cb_arg)
{
	struct bt_mesh_prop_srv *srv = model->rt->user_data;
	uint8_t entries[CONFIG_BT_MESH_PROP_MAXCOUNT];
	ssize_t size = MIN(sizeof(entries), len_rd);

	if (name) {
		return -ENOENT;
	}

	size = read_cb(cb_arg, &entries, size);

	if (size < srv->property_count) {
		return -EINVAL;
	}

	for (uint32_t i = 0; i < srv->property_count; ++i) {
		srv->properties[i].user_access = entries[i];
	}

	return 0;
}
#endif

const struct bt_mesh_model_cb _bt_mesh_prop_srv_cb = {
	.init = bt_mesh_prop_srv_init,
	.reset = bt_mesh_prop_srv_reset,
#ifdef CONFIG_BT_SETTINGS
	.settings_set = bt_mesh_prop_srv_settings_set,
	.pending_store = bt_mesh_prop_srv_pending_store,
#endif
};

int bt_mesh_prop_srv_pub_list(struct bt_mesh_prop_srv *srv,
			      struct bt_mesh_msg_ctx *ctx)
{
	srv->pub_id = 0;
	srv->pub_state = BT_MESH_PROP_SRV_STATE_LIST;
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PROP_OP_MFR_PROPS_STATUS,
				 BT_MESH_PROP_MSG_MAXLEN_PROPS_STATUS);

	pub_list_build(srv, &msg, 1);

	return bt_mesh_msg_send(srv->model, ctx, &msg);
}

int bt_mesh_prop_srv_pub(struct bt_mesh_prop_srv *srv,
			 struct bt_mesh_msg_ctx *ctx,
			 const struct bt_mesh_prop_val *val)
{
	if (srv->model->id == BT_MESH_MODEL_ID_GEN_CLIENT_PROP_SRV) {
		return -EINVAL;
	}

	if (val->size > CONFIG_BT_MESH_PROP_MAXSIZE) {
		return -EMSGSIZE;
	}

	srv->pub_id = val->meta.id;
	srv->pub_state = BT_MESH_PROP_SRV_STATE_PROP;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PROP_OP_ADMIN_PROP_STATUS,
				 BT_MESH_PROP_MSG_MAXLEN_PROP_STATUS);

	bt_mesh_model_msg_init(&msg, op_get(BT_MESH_PROP_OP_PROP_STATUS,
					    srv_kind(srv->model)));

	net_buf_simple_add_le16(&msg, val->meta.id);
	net_buf_simple_add_u8(&msg, val->meta.user_access);
	net_buf_simple_add_mem(&msg, val->value, val->size);

	return bt_mesh_msg_send(srv->model, ctx, &msg);
}
