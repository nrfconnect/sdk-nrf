/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/gen_ponoff_srv.h>

#include <string.h>
#include <zephyr/settings/settings.h>
#include <stdlib.h>
#include "model_utils.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_mesh_gen_ponoff_srv);

/** Persistent storage handling */
struct ponoff_settings_data {
	uint8_t on_power_up;
	bool on_off;
} __packed;

#if CONFIG_BT_SETTINGS
static int store_data(struct bt_mesh_ponoff_srv *srv,
		      const struct bt_mesh_onoff_status *onoff_status)
{
	struct ponoff_settings_data data;
	ssize_t size;

	data.on_power_up = (uint8_t)srv->on_power_up;

	switch (srv->on_power_up) {
	case BT_MESH_ON_POWER_UP_OFF:
		data.on_off = false;
		break;
	case BT_MESH_ON_POWER_UP_ON:
		data.on_off = true;
		break;
	case BT_MESH_ON_POWER_UP_RESTORE:
		data.on_off = onoff_status->remaining_time > 0 ?
				      onoff_status->target_on_off :
				      onoff_status->present_on_off;
		break;
	default:
		return -EINVAL;
	}

	/* Models that extend Generic Power OnOff Setup Server and, which states are
	 * bound with Generic OnOff state, store the value of the bound state
	 * separately, therefore they don't need to store Generic OnOff state.
	 */
	if (bt_mesh_model_is_extended(srv->ponoff_setup_model)) {
		size = sizeof(data.on_power_up);
	} else {
		size = sizeof(data);
	}

	return bt_mesh_model_data_store(srv->ponoff_model, false, NULL, &data,
					size);

}

static void bt_mesh_ponoff_srv_pending_store(const struct bt_mesh_model *model)
{
	int err;

	struct bt_mesh_ponoff_srv *srv = model->rt->user_data;

	struct bt_mesh_onoff_status onoff_status = {0};

	if (!bt_mesh_model_is_extended(srv->ponoff_setup_model)) {
		srv->onoff.handlers->get(&srv->onoff, NULL, &onoff_status);
	}

	err = store_data(srv, &onoff_status);

	if (err) {
		LOG_ERR("Failed storing data: %d", err);
	}
}
#endif

static void store_state(struct bt_mesh_ponoff_srv *srv)
{
#if CONFIG_BT_SETTINGS
	bt_mesh_model_data_store_schedule(srv->ponoff_model);
#endif
}

static void send_rsp(const struct bt_mesh_model *model,
		     struct bt_mesh_msg_ctx *ctx)
{
	struct bt_mesh_ponoff_srv *srv = model->rt->user_data;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PONOFF_OP_STATUS,
				 BT_MESH_PONOFF_MSG_LEN_STATUS);
	bt_mesh_model_msg_init(&msg, BT_MESH_PONOFF_OP_STATUS);
	net_buf_simple_add_u8(&msg, srv->on_power_up);
	bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int handle_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	send_rsp(model, ctx);

	return 0;
}

static void set_on_power_up(struct bt_mesh_ponoff_srv *srv,
			    struct bt_mesh_msg_ctx *ctx,
			    enum bt_mesh_on_power_up new)
{
	if (new == srv->on_power_up) {
		return;
	}

	enum bt_mesh_on_power_up old = srv->on_power_up;

	srv->on_power_up = new;

	bt_mesh_model_msg_init(srv->ponoff_model->pub->msg,
			       BT_MESH_PONOFF_OP_STATUS);
	net_buf_simple_add_u8(srv->ponoff_model->pub->msg, srv->on_power_up);

	if (srv->update) {
		srv->update(srv, ctx, old, new);
	}

	store_state(srv);
}

static int handle_set_msg(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf, bool ack)
{
	struct bt_mesh_ponoff_srv *srv = model->rt->user_data;
	enum bt_mesh_on_power_up new = net_buf_simple_pull_u8(buf);

	if (new >= BT_MESH_ON_POWER_UP_INVALID) {
		return -EINVAL;
	}

	set_on_power_up(srv, ctx, new);

	if (ack) {
		send_rsp(model, ctx);
	}

	(void)bt_mesh_ponoff_srv_pub(srv, NULL);

	return 0;
}

static int handle_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	return handle_set_msg(model, ctx, buf, true);
}

static int handle_set_unack(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	return handle_set_msg(model, ctx, buf, false);
}

/* Need to intercept the onoff state to get the right value on power up. */
static void onoff_intercept_set(struct bt_mesh_onoff_srv *onoff_srv,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_onoff_set *set,
				struct bt_mesh_onoff_status *status)
{
	struct bt_mesh_ponoff_srv *srv =
		CONTAINER_OF(onoff_srv, struct bt_mesh_ponoff_srv, onoff);

	srv->onoff_handlers->set(onoff_srv, ctx, set, status);

	if ((srv->on_power_up == BT_MESH_ON_POWER_UP_RESTORE) &&
	    !bt_mesh_model_is_extended(srv->ponoff_setup_model)) {
		store_state(srv);
	}
}

static void onoff_intercept_get(struct bt_mesh_onoff_srv *onoff_srv,
				struct bt_mesh_msg_ctx *ctx,
				struct bt_mesh_onoff_status *out)
{
	struct bt_mesh_ponoff_srv *srv =
		CONTAINER_OF(onoff_srv, struct bt_mesh_ponoff_srv, onoff);

	srv->onoff_handlers->get(onoff_srv, ctx, out);
}

const struct bt_mesh_model_op _bt_mesh_ponoff_srv_op[] = {
	{
		BT_MESH_PONOFF_OP_GET,
		BT_MESH_LEN_EXACT(BT_MESH_PONOFF_MSG_LEN_GET),
		handle_get,
	},
	BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op _bt_mesh_ponoff_setup_srv_op[] = {
	{
		BT_MESH_PONOFF_OP_SET,
		BT_MESH_LEN_EXACT(BT_MESH_PONOFF_MSG_LEN_SET),
		handle_set,
	},
	{
		BT_MESH_PONOFF_OP_SET_UNACK,
		BT_MESH_LEN_EXACT(BT_MESH_PONOFF_MSG_LEN_SET),
		handle_set_unack,
	},
	BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_onoff_srv_handlers _bt_mesh_ponoff_onoff_intercept = {
	.set = onoff_intercept_set,
	.get = onoff_intercept_get,
};

static ssize_t scene_store(const struct bt_mesh_model *model, uint8_t data[])
{
	struct bt_mesh_ponoff_srv *srv = model->rt->user_data;
	struct bt_mesh_onoff_status status = { 0 };

	/* Only store the next stable on_off state: */
	srv->onoff_handlers->get(&srv->onoff, NULL, &status);
	data[0] = status.remaining_time ? status.target_on_off :
					  status.present_on_off;

	return 1;
}

static void scene_recall(const struct bt_mesh_model *model, const uint8_t data[],
			 size_t len, struct bt_mesh_model_transition *transition)
{
	struct bt_mesh_ponoff_srv *srv = model->rt->user_data;
	struct bt_mesh_onoff_status status = { 0 };
	struct bt_mesh_onoff_set set = {
		.on_off = data[0],
		.transition = transition,
	};

	srv->onoff_handlers->set(&srv->onoff, NULL, &set, &status);
}

static void scene_recall_complete(const struct bt_mesh_model *model)
{
	struct bt_mesh_ponoff_srv *srv = model->rt->user_data;
	struct bt_mesh_onoff_status status = { 0 };

	srv->onoff_handlers->get(&srv->onoff, NULL, &status);

	(void)bt_mesh_onoff_srv_pub(&srv->onoff, NULL, &status);
}

/*  MshMDLv1.1: 5.1.3.1.1:
 *  If a model is extending another model, the extending model shall determine
 *  the Stored with Scene behavior of that model.
 *
 *  Use Setup Model to handle Scene Store/Recall as it is not extended
 *  by other models.
 */
BT_MESH_SCENE_ENTRY_SIG(ponoff) = {
	.id.sig = BT_MESH_MODEL_ID_GEN_POWER_ONOFF_SETUP_SRV,
	.maxlen = 1,
	.store = scene_store,
	.recall = scene_recall,
	.recall_complete = scene_recall_complete,
};

static int update_handler(const struct bt_mesh_model *model)
{
	struct bt_mesh_ponoff_srv *srv = model->rt->user_data;

	bt_mesh_model_msg_init(srv->ponoff_model->pub->msg,
			       BT_MESH_PONOFF_OP_STATUS);
	net_buf_simple_add_u8(srv->ponoff_model->pub->msg, srv->on_power_up);
	return 0;
}

static int bt_mesh_ponoff_srv_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_ponoff_srv *srv = model->rt->user_data;

	srv->ponoff_model = model;
	srv->pub.msg = &srv->pub_buf;
	srv->pub.update = update_handler;
	net_buf_simple_init_with_data(&srv->pub_buf, srv->pub_data,
				      sizeof(srv->pub_data));

	return bt_mesh_model_extend(model, srv->onoff.model);
}

static void bt_mesh_ponoff_srv_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_ponoff_srv *srv = model->rt->user_data;

	srv->on_power_up = BT_MESH_ON_POWER_UP_OFF;
	net_buf_simple_reset(srv->pub.msg);
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		(void)bt_mesh_model_data_store(srv->ponoff_model, false, NULL,
					       NULL, 0);
	}
}

#ifdef CONFIG_BT_SETTINGS
static int bt_mesh_ponoff_srv_settings_set(const struct bt_mesh_model *model,
					   const char *name,
					   size_t len_rd,
					   settings_read_cb read_cb,
					   void *cb_arg)
{
	struct bt_mesh_ponoff_srv *srv = model->rt->user_data;
	struct bt_mesh_onoff_status dummy;
	struct ponoff_settings_data data;
	ssize_t size = MIN(len_rd, sizeof(data));

	if (name) {
		return -ENOENT;
	}

	if (read_cb(cb_arg, &data, size) != size) {
		return -EINVAL;
	}

	set_on_power_up(srv, NULL, (enum bt_mesh_on_power_up)data.on_power_up);

	/* Models that extend Generic Power OnOff Setup Server and, which states are
	 * bound with Generic OnOff state, store the value of the bound state
	 * separately, therefore they don't need to set Generic OnOff state.
	 */
	if (bt_mesh_model_is_extended(srv->ponoff_setup_model)) {
		return 0;
	}

	struct bt_mesh_onoff_set onoff_set = { .transition = NULL };

	switch (data.on_power_up) {
	case BT_MESH_ON_POWER_UP_OFF:
		onoff_set.on_off = false;
		break;
	case BT_MESH_ON_POWER_UP_ON:
		onoff_set.on_off = true;
		break;
	case BT_MESH_ON_POWER_UP_RESTORE:
		onoff_set.on_off = data.on_off;
		break;
	default:
		return -EINVAL;
	}

	srv->onoff.handlers->set(&srv->onoff, NULL, &onoff_set, &dummy);

	return 0;
}
#endif

const struct bt_mesh_model_cb _bt_mesh_ponoff_srv_cb = {
	.init = bt_mesh_ponoff_srv_init,
	.reset = bt_mesh_ponoff_srv_reset,
#ifdef CONFIG_BT_SETTINGS
	.settings_set = bt_mesh_ponoff_srv_settings_set,
	.pending_store = bt_mesh_ponoff_srv_pending_store,
#endif
};

static int bt_mesh_ponoff_setup_srv_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_ponoff_srv *srv = model->rt->user_data;
	int err;

	srv->ponoff_setup_model = model;

	err = bt_mesh_model_extend(model, srv->ponoff_model);
	if (err) {
		return err;
	}

#if defined(CONFIG_BT_MESH_COMP_PAGE_1)
	err = bt_mesh_model_correspond(model, srv->ponoff_model);
	if (err) {
		return err;
	}
#endif

	return bt_mesh_model_extend(model, srv->dtt.model);
}

const struct bt_mesh_model_cb _bt_mesh_ponoff_setup_srv_cb = {
	.init = bt_mesh_ponoff_setup_srv_init,
};

void bt_mesh_ponoff_srv_set(struct bt_mesh_ponoff_srv *srv,
			    enum bt_mesh_on_power_up on_power_up)
{
	set_on_power_up(srv, NULL, on_power_up);

	(void)bt_mesh_ponoff_srv_pub(srv, NULL);
}

int bt_mesh_ponoff_srv_pub(struct bt_mesh_ponoff_srv *srv,
			   struct bt_mesh_msg_ctx *ctx)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_PONOFF_OP_STATUS,
				 BT_MESH_PONOFF_MSG_LEN_STATUS);
	bt_mesh_model_msg_init(&msg, BT_MESH_PONOFF_OP_STATUS);
	net_buf_simple_add_u8(&msg, srv->on_power_up);

	return bt_mesh_msg_send(srv->ponoff_model, ctx, &msg);
}
