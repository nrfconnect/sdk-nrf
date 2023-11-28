/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/byteorder.h>
#include <bluetooth/mesh/light_hsl.h>
#include <bluetooth/mesh/light_hsl_srv.h>
#include <bluetooth/mesh/light_sat_srv.h>
#include <bluetooth/mesh/light_hue_srv.h>
#include <bluetooth/mesh/lightness.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include "light_hsl_internal.h"
#include "lightness_internal.h"
#include "model_utils.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_mesh_light_hsl_srv);

#define HSL_STATUS_INIT(_hue, _sat, _lightness, _member)                       \
	{                                                                      \
		.params = {                                                    \
			.hue = (_hue)->_member,                                \
			.saturation = (_sat)->_member,                         \
			.lightness = (_lightness)->_member,                    \
		},                                                             \
		.remaining_time = MAX((_lightness)->remaining_time,            \
				      MAX((_hue)->remaining_time,              \
					  (_sat)->remaining_time)),            \
	}

static void hsl_status_encode(struct net_buf_simple *buf, uint32_t op,
			      const struct bt_mesh_light_hsl_status *status)
{
	bt_mesh_model_msg_init(buf, op);
	light_hsl_buf_push(buf,  &status->params);

	if (!status->remaining_time) {
		return;
	}

	net_buf_simple_add_u8(buf,
			      model_transition_encode(status->remaining_time));
}

static void hsl_get(struct bt_mesh_light_hsl_srv *srv,
		    struct bt_mesh_msg_ctx *ctx,
		    struct bt_mesh_light_hsl_status *status)
{
	struct bt_mesh_lightness_status lightness;
	struct bt_mesh_light_hue_status hue;
	struct bt_mesh_light_sat_status sat;

	srv->hue.handlers->get(&srv->hue, ctx, &hue);
	srv->sat.handlers->get(&srv->sat, ctx, &sat);
	srv->lightness->handlers->light_get(srv->lightness, ctx, &lightness);

	*status = (struct bt_mesh_light_hsl_status)HSL_STATUS_INIT(
		&hue, &sat, &lightness, current);
}

static int handle_hsl_get(const struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;
	struct bt_mesh_light_hsl_status status;

	hsl_get(srv, ctx, &status);
	(void)bt_mesh_light_hsl_srv_pub(srv, ctx, &status);

	return 0;
}

static int hsl_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		   struct net_buf_simple *buf, bool ack)
{
	struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;
	struct bt_mesh_model_transition transition;
	struct bt_mesh_lightness_status lightness;
	struct bt_mesh_light_hue_status hue;
	struct bt_mesh_light_sat_status sat;
	struct {
		struct bt_mesh_light_hue h;
		struct bt_mesh_light_sat s;
		struct bt_mesh_lightness_set l;
	} set;
	uint8_t tid;

	if (buf->len != BT_MESH_LIGHT_HSL_MSG_MINLEN_SET &&
	    buf->len != BT_MESH_LIGHT_HSL_MSG_MAXLEN_SET) {
		return -EMSGSIZE;
	}

	set.l.lvl = from_actual(net_buf_simple_pull_le16(buf));
	set.h.lvl = net_buf_simple_pull_le16(buf);
	set.s.lvl = net_buf_simple_pull_le16(buf);

	tid = net_buf_simple_pull_u8(buf);
	if (tid_check_and_update(&srv->prev_transaction, tid, ctx)) {
		if (ack) {
			struct bt_mesh_light_hsl_status status;

			hsl_get(srv, NULL, &status);
			(void)bt_mesh_light_hsl_srv_pub(srv, ctx, &status);
		}

		return 0;
	}

	set.h.transition = model_transition_get(srv->model, &transition, buf);
	set.s.transition = set.h.transition;
	set.l.transition = set.h.transition;

	/* In the documentation for the Hue, Saturation and Lightness Servers,
	 * the user is instructed to publish an HSL status message whenever
	 * their primary state changes. As this would trigger three publications
	 * in addition to the publication at the end of this function, we'll set
	 * the pub_pending flag before calling these functions. This way, we'll
	 * skip the HSL publication from the application callback of each
	 * server, and instead perform it once at the end of this function.
	 */
	srv->pub_pending = true;

	bt_mesh_light_hue_srv_set(&srv->hue, ctx, &set.h, &hue);
	bt_mesh_light_sat_srv_set(&srv->sat, ctx, &set.s, &sat);
	lightness_srv_disable_control(srv->lightness);
	lightness_srv_change_lvl(srv->lightness, ctx, &set.l, &lightness, true);

	srv->pub_pending = false;

	struct bt_mesh_light_hsl_status hsl =
		HSL_STATUS_INIT(&hue, &sat, &lightness, current);

	if (ack) {
		(void)bt_mesh_light_hsl_srv_pub(srv, ctx, &hsl);
	}

	/* Publish on state change: */
	(void)bt_mesh_light_hsl_srv_pub(srv, NULL, &hsl);

	if (IS_ENABLED(CONFIG_BT_MESH_SCENE_SRV)) {
		bt_mesh_scene_invalidate(srv->model);
	}

	return 0;
}

static int handle_hsl_set(const struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	return hsl_set(model, ctx, buf, true);
}

static int handle_hsl_set_unack(const struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	return hsl_set(model, ctx, buf, false);
}

static int handle_hsl_target_get(const struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;
	struct bt_mesh_lightness_status lightness;
	struct bt_mesh_light_hue_status hue;
	struct bt_mesh_light_sat_status sat;

	srv->hue.handlers->get(&srv->hue, ctx, &hue);
	srv->sat.handlers->get(&srv->sat, ctx, &sat);
	srv->lightness->handlers->light_get(srv->lightness, ctx, &lightness);

	struct bt_mesh_light_hsl_status status =
		HSL_STATUS_INIT(&hue, &sat, &lightness, target);

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LIGHT_HSL_OP_TARGET_STATUS,
				 BT_MESH_LIGHT_HSL_MSG_MAXLEN_STATUS);
	hsl_status_encode(&rsp, BT_MESH_LIGHT_HSL_OP_TARGET_STATUS, &status);

	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

static void default_rsp(const struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *rx_ctx)
{
	struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;
	const struct bt_mesh_light_hsl hsl = {
		.lightness = srv->lightness->default_light,
		.hue = srv->hue.dflt,
		.saturation = srv->sat.dflt,
	};

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_HSL_OP_DEFAULT_STATUS,
				 BT_MESH_LIGHT_HSL_MSG_LEN_DEFAULT);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_HSL_OP_DEFAULT_STATUS);
	light_hsl_buf_push(&msg, &hsl);

	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static int default_set(const struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;
	struct bt_mesh_light_hsl val;

	light_hsl_buf_pull(buf, &val);

	lightness_srv_default_set(srv->lightness, ctx, val.lightness);
	bt_mesh_light_hue_srv_default_set(&srv->hue, ctx, val.hue);
	bt_mesh_light_sat_srv_default_set(&srv->sat, ctx, val.saturation);

	return 0;
}

static int handle_default_get(const struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	default_rsp(model, ctx);

	return 0;
}

static int handle_default_set(const struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	default_set(model, ctx, buf);
	default_rsp(model, ctx);

	return 0;
}

static int handle_default_set_unack(const struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	default_set(model, ctx, buf);

	return 0;
}

static void range_encode_status(struct net_buf_simple *buf,
				struct bt_mesh_light_hsl_srv *srv,
				enum bt_mesh_model_status status)
{
	const struct bt_mesh_light_hue_sat_range range = {
		.min = {
			.hue = srv->hue.range.min,
			.saturation = srv->sat.range.min,
		},
		.max = {
			.hue = srv->hue.range.max,
			.saturation = srv->sat.range.max,
		},
	};

	bt_mesh_model_msg_init(buf, BT_MESH_LIGHT_HSL_OP_RANGE_STATUS);
	net_buf_simple_add_u8(buf, status);
	light_hue_sat_range_buf_push(buf, &range);
}

static void range_rsp(const struct bt_mesh_model *model,
		      struct bt_mesh_msg_ctx *rx_ctx,
		      enum bt_mesh_model_status status)
{
	struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_HSL_OP_RANGE_STATUS,
				 BT_MESH_LIGHT_HSL_MSG_LEN_RANGE_STATUS);
	range_encode_status(&msg, srv, status);
	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static int range_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf)
{
	struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;
	struct bt_mesh_light_hsl_range hue, sat;

	hue.min = net_buf_simple_pull_le16(buf);
	hue.max = net_buf_simple_pull_le16(buf);

	sat.min = net_buf_simple_pull_le16(buf);
	sat.max = net_buf_simple_pull_le16(buf);
	if (sat.max < sat.min) {
		return -EINVAL;
	}

	bt_mesh_light_hue_srv_range_set(&srv->hue, ctx, &hue);
	bt_mesh_light_sat_srv_range_set(&srv->sat, ctx, &sat);

	return 0;
}

static int handle_range_get(const struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	range_rsp(model, ctx, BT_MESH_MODEL_SUCCESS);

	return 0;
}

static int handle_range_set(const struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	int err;

	/* According to Test specification MMDL/SR/LHSLSE/BI-01-C, ignore
	 * message if max < min:
	 */
	err = range_set(model, ctx, buf);
	if (err) {
		return err;
	}

	range_rsp(model, ctx, BT_MESH_MODEL_SUCCESS);

	return 0;
}

static int handle_range_set_unack(const struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	/* According to Test specification MMDL/SR/LHSLSE/BI-01-C, ignore
	 * message if max < min:
	 */
	return range_set(model, ctx, buf);
}

const struct bt_mesh_model_op _bt_mesh_light_hsl_srv_op[] = {
	{
		BT_MESH_LIGHT_HSL_OP_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_HSL_MSG_LEN_GET),
		handle_hsl_get,
	},
	{
		BT_MESH_LIGHT_HSL_OP_SET,
		BT_MESH_LEN_MIN(BT_MESH_LIGHT_HSL_MSG_MINLEN_SET),
		handle_hsl_set,
	},
	{
		BT_MESH_LIGHT_HSL_OP_SET_UNACK,
		BT_MESH_LEN_MIN(BT_MESH_LIGHT_HSL_MSG_MINLEN_SET),
		handle_hsl_set_unack,
	},
	{
		BT_MESH_LIGHT_HSL_OP_TARGET_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_HSL_MSG_LEN_GET),
		handle_hsl_target_get,
	},
	{
		BT_MESH_LIGHT_HSL_OP_DEFAULT_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_HSL_MSG_LEN_GET),
		handle_default_get,
	},
	{
		BT_MESH_LIGHT_HSL_OP_RANGE_GET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_HSL_MSG_LEN_GET),
		handle_range_get,
	},
	BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op _bt_mesh_light_hsl_setup_srv_op[] = {
	{
		BT_MESH_LIGHT_HSL_OP_DEFAULT_SET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_HSL_MSG_LEN_DEFAULT),
		handle_default_set,
	},
	{
		BT_MESH_LIGHT_HSL_OP_DEFAULT_SET_UNACK,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_HSL_MSG_LEN_DEFAULT),
		handle_default_set_unack,
	},
	{
		BT_MESH_LIGHT_HSL_OP_RANGE_SET,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_HSL_MSG_LEN_RANGE_SET),
		handle_range_set,
	},
	{
		BT_MESH_LIGHT_HSL_OP_RANGE_SET_UNACK,
		BT_MESH_LEN_EXACT(BT_MESH_LIGHT_HSL_MSG_LEN_RANGE_SET),
		handle_range_set_unack,
	},
	BT_MESH_MODEL_OP_END,
};

static ssize_t scene_store(const struct bt_mesh_model *model, uint8_t data[])
{
	struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;
	struct bt_mesh_lightness_status status = { 0 };

	if (atomic_test_bit(&srv->lightness->flags,
			    LIGHTNESS_SRV_FLAG_EXTENDED_BY_LIGHT_CTRL)) {
		return 0;
	}

	srv->lightness->handlers->light_get(srv->lightness, NULL, &status);
	sys_put_le16(status.remaining_time ? to_actual(status.target) : status.current, &data[0]);

	return 2;
}

static void scene_recall(const struct bt_mesh_model *model, const uint8_t data[],
			 size_t len,
			 struct bt_mesh_model_transition *transition)
{
	struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;

	if (atomic_test_bit(&srv->lightness->flags,
			    LIGHTNESS_SRV_FLAG_EXTENDED_BY_LIGHT_CTRL)) {
		return;
	}

	struct bt_mesh_lightness_status light_status = { 0 };
	struct bt_mesh_lightness_set light_set = {
		.lvl = from_actual(sys_get_le16(data)),
		.transition = transition,
	};

	lightness_srv_change_lvl(srv->lightness, NULL, &light_set, &light_status, false);
}

static void scene_recall_complete(const struct bt_mesh_model *model)
{
	struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;
	struct bt_mesh_lightness_status light_status = { 0 };
	struct bt_mesh_light_hue_status hue_status = { 0 };
	struct bt_mesh_light_sat_status sat_status = { 0 };

	srv->hue.handlers->get(&srv->hue, NULL, &hue_status);
	srv->sat.handlers->get(&srv->sat, NULL, &sat_status);
	srv->lightness->handlers->light_get(srv->lightness, NULL, &light_status);

	struct bt_mesh_light_hsl_status hsl =
		HSL_STATUS_INIT(&hue_status, &sat_status, &light_status, current);

	(void)bt_mesh_light_hsl_srv_pub(srv, NULL, &hsl);
}

/*  MshMDLv1.1: 5.1.3.1.1:
 *  If a model is extending another model, the extending model shall determine
 *  the Stored with Scene behavior of that model.
 *
 *  Use Setup Model to handle Scene Store/Recall as it is not extended
 *  by other models.
 */
BT_MESH_SCENE_ENTRY_SIG(light_hsl) = {
	.id.sig = BT_MESH_MODEL_ID_LIGHT_HSL_SETUP_SRV,
	.maxlen = 2,
	.store = scene_store,
	.recall = scene_recall,
	.recall_complete = scene_recall_complete,
};

static int hsl_srv_pub_update(const struct bt_mesh_model *model)
{
	struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;
	struct bt_mesh_light_hsl_status status;

	hsl_get(srv, NULL, &status);
	hsl_status_encode(&srv->buf, BT_MESH_LIGHT_HSL_OP_STATUS, &status);

	return 0;
}

static int bt_mesh_light_hsl_srv_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;
	const struct bt_mesh_model *lightness_srv;

	srv->model = model;
	srv->pub.update = hsl_srv_pub_update;
	srv->pub.msg = &srv->buf;
	srv->hue.hsl = srv;
	srv->sat.hsl = srv;
	srv->pub_pending = true;
	net_buf_simple_init_with_data(&srv->buf, srv->pub_data,
				      ARRAY_SIZE(srv->pub_data));

	lightness_srv =
		bt_mesh_model_find(bt_mesh_model_elem(model), BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SRV);

	if (!lightness_srv) {
		LOG_ERR("Failed to find Lightness Server on element");
		return -EINVAL;
	}

	return bt_mesh_model_extend(model, lightness_srv);
}

static int bt_mesh_light_hsl_srv_start(const struct bt_mesh_model *model)
{
	struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;
	struct bt_mesh_model_transition transition;
	struct bt_mesh_light_hue hue = { .transition = &transition };
	struct bt_mesh_light_sat sat = { .transition = &transition };
	struct bt_mesh_lightness_status lightness = { 0 };

	if (!srv->sat.model ||
	    (srv->model->rt->elem_idx > srv->sat.model->rt->elem_idx)) {
		LOG_ERR("Light HSL srv[%d]: Sat. srv not properly initialized",
		       srv->model->rt->elem_idx);
		return -EINVAL;
	}

	if (!srv->hue.model ||
	    (srv->model->rt->elem_idx > srv->hue.model->rt->elem_idx)) {
		LOG_ERR("Light HSL srv[%d]: Hue srv not properly initialized",
		       srv->model->rt->elem_idx);
		return -EINVAL;
	}

	bt_mesh_dtt_srv_transition_get(model, &transition);

	switch (srv->lightness->ponoff.on_power_up) {
	case BT_MESH_ON_POWER_UP_ON:
	case BT_MESH_ON_POWER_UP_OFF:
		hue.lvl = srv->hue.dflt;
		sat.lvl = srv->sat.dflt;
		break;
	case BT_MESH_ON_POWER_UP_RESTORE:
		hue.lvl = srv->hue.transient.last;
		sat.lvl = srv->sat.transient.last;
		break;
	default:
		return -EINVAL;
	}

	/* Pass dummy status structs to avoid giving the app NULL pointers */
	bt_mesh_light_hue_srv_set(&srv->hue, NULL, &hue,
				  &(struct bt_mesh_light_hue_status){});
	bt_mesh_light_sat_srv_set(&srv->sat, NULL, &sat,
				  &(struct bt_mesh_light_sat_status){});
	/* The lightness server handles its own startup. */

	/* Lift publication block from init function: */
	srv->pub_pending = false;

	srv->lightness->handlers->light_get(srv->lightness, NULL, &lightness);

	struct bt_mesh_light_hsl_status status = {
		.params = {
			.hue = hue.lvl,
			.saturation = sat.lvl,
			.lightness = lightness.current,
		},
		.remaining_time = 0,
	};

	/* Ignore error: Will fail if there are no publication parameters, but
	 * it doesn't matter for the startup procedure.
	 */
	(void)bt_mesh_light_hsl_srv_pub(srv, NULL, &status);
	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_light_hsl_srv_cb = {
	.init = bt_mesh_light_hsl_srv_init,
	.start = bt_mesh_light_hsl_srv_start,
};

static int bt_mesh_light_hsl_setup_srv_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_light_hsl_srv *srv = model->rt->user_data;
	const struct bt_mesh_model *lightness_setup_srv;
	int err;

	err = bt_mesh_model_extend(model, srv->model);
	if (err) {
		return err;
	}

#if defined(CONFIG_BT_MESH_COMP_PAGE_1)
	err = bt_mesh_model_correspond(model, srv->model);
	if (err) {
		return err;
	}
#endif

	lightness_setup_srv = bt_mesh_model_find(bt_mesh_model_elem(model),
						 BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SETUP_SRV);

	if (!lightness_setup_srv) {
		LOG_ERR("Failed to find Lightness Setup Server on element");
		return -EINVAL;
	}

	return bt_mesh_model_extend(model, lightness_setup_srv);
}

const struct bt_mesh_model_cb _bt_mesh_light_hsl_setup_srv_cb = {
	.init = bt_mesh_light_hsl_setup_srv_init,
};

int bt_mesh_light_hsl_srv_pub(struct bt_mesh_light_hsl_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_light_hsl_status *status)
{
	if (srv->pub_pending) {
		/* A publication is already pending, skip this one. See the
		 * comment in hsl_set() for details.
		 */
		return 0;
	}

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_HSL_OP_STATUS,
				 BT_MESH_LIGHT_HSL_MSG_MAXLEN_STATUS);
	hsl_status_encode(&buf, BT_MESH_LIGHT_HSL_OP_STATUS, status);

	return bt_mesh_msg_send(srv->model, ctx, &buf);
}
