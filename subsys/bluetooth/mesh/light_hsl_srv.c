/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <sys/byteorder.h>
#include <bluetooth/mesh/light_hsl.h>
#include <bluetooth/mesh/light_hsl_srv.h>
#include <bluetooth/mesh/light_sat_srv.h>
#include <bluetooth/mesh/light_hue_srv.h>
#include <bluetooth/mesh/lightness.h>
#include <bluetooth/mesh/gen_dtt_srv.h>
#include "light_hsl_internal.h"
#include "lightness_internal.h"
#include "model_utils.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)
#define LOG_MODULE_NAME bt_mesh_light_hsl_srv
#include "common/log.h"

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

static void hsl_status_rsp(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   const struct bt_mesh_light_hsl_status *status)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_LIGHT_HSL_OP_STATUS,
				 BT_MESH_LIGHT_HSL_MSG_MAXLEN_STATUS);
	hsl_status_encode(&buf, BT_MESH_LIGHT_HSL_OP_STATUS, status);

	bt_mesh_model_send(model, ctx, &buf, NULL, NULL);
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
	srv->lightness.handlers->light_get(&srv->lightness, ctx, &lightness);

	*status = (struct bt_mesh_light_hsl_status)HSL_STATUS_INIT(
		&hue, &sat, &lightness, current);
}

static void hsl_get_handle(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_light_hsl_srv *srv = model->user_data;
	struct bt_mesh_light_hsl_status status;

	if (buf->len != BT_MESH_LIGHT_HSL_MSG_LEN_GET) {
		return;
	}

	hsl_get(srv, ctx, &status);
	hsl_status_rsp(model, ctx, &status);
}

static void hsl_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf, bool ack)
{
	struct bt_mesh_light_hsl_srv *srv = model->user_data;
	struct bt_mesh_model_transition transition;
	struct bt_mesh_lightness_status lightness;
	struct bt_mesh_light_hue_status hue;
	struct bt_mesh_light_sat_status sat;
	struct {
		struct bt_mesh_light_hue h;
		struct bt_mesh_light_sat s;
		struct bt_mesh_lightness_set l;
	} set = {
		.h = { .transition = &transition },
		.s = { .transition = &transition },
		.l = { .transition = &transition },
	};
	uint8_t tid;

	if (buf->len != BT_MESH_LIGHT_HSL_MSG_MINLEN_SET &&
	    buf->len != BT_MESH_LIGHT_HSL_MSG_MAXLEN_SET) {
		return;
	}

	set.l.lvl = repr_to_light(net_buf_simple_pull_le16(buf), ACTUAL);
	set.h.lvl = net_buf_simple_pull_le16(buf);
	set.s.lvl = net_buf_simple_pull_le16(buf);

	tid = net_buf_simple_pull_u8(buf);
	if (tid_check_and_update(&srv->prev_transaction, tid, ctx)) {
		if (ack) {
			struct bt_mesh_light_hsl_status status;

			hsl_get(srv, NULL, &status);
			hsl_status_rsp(model, ctx, &status);
		}

		return;
	}

	if (buf->len) {
		model_transition_buf_pull(buf, &transition);
	} else {
		bt_mesh_dtt_srv_transition_get(model, &transition);
	}

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
	lightness_srv_change_lvl(&srv->lightness, ctx, &set.l, &lightness);

	struct bt_mesh_light_hsl_status hsl =
		HSL_STATUS_INIT(&hue, &sat, &lightness, current);

	if (ack) {
		hsl_status_rsp(model, ctx, &hsl);
	}

	/* Publish on state change: */
	srv->pub_pending = false;
	(void)bt_mesh_light_hsl_srv_pub(srv, NULL, &hsl);
}

static void hsl_set_handle(struct bt_mesh_model *model,
			   struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	hsl_set(model, ctx, buf, true);
}

static void hsl_set_unack_handle(struct bt_mesh_model *model,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	hsl_set(model, ctx, buf, false);
}

static void hsl_target_get_handle(struct bt_mesh_model *model,
				  struct bt_mesh_msg_ctx *ctx,
				  struct net_buf_simple *buf)
{
	struct bt_mesh_light_hsl_srv *srv = model->user_data;
	struct bt_mesh_lightness_status lightness;
	struct bt_mesh_light_hue_status hue;
	struct bt_mesh_light_sat_status sat;

	if (buf->len != BT_MESH_LIGHT_HSL_MSG_LEN_GET) {
		return;
	}

	srv->hue.handlers->get(&srv->hue, ctx, &hue);
	srv->sat.handlers->get(&srv->sat, ctx, &sat);
	srv->lightness.handlers->light_get(&srv->lightness, ctx, &lightness);

	struct bt_mesh_light_hsl_status status =
		HSL_STATUS_INIT(&hue, &sat, &lightness, target);

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LIGHT_HSL_OP_TARGET_STATUS,
				 BT_MESH_LIGHT_HSL_MSG_MAXLEN_STATUS);
	hsl_status_encode(&rsp, BT_MESH_LIGHT_HSL_OP_TARGET_STATUS, &status);

	bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);
}

static void default_rsp(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *rx_ctx)
{
	struct bt_mesh_light_hsl_srv *srv = model->user_data;
	const struct bt_mesh_light_hsl hsl = {
		.lightness = srv->lightness.default_light,
		.hue = srv->hue.dflt,
		.saturation = srv->sat.dflt,
	};

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_HSL_OP_DEFAULT_STATUS,
				 BT_MESH_LIGHT_HSL_MSG_LEN_DEFAULT);
	bt_mesh_model_msg_init(&msg, BT_MESH_LIGHT_HSL_OP_DEFAULT_STATUS);
	light_hsl_buf_push(&msg, &hsl);

	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static void default_set(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
	struct bt_mesh_light_hsl_srv *srv = model->user_data;
	struct bt_mesh_light_hsl val;

	light_hsl_buf_pull(buf, &val);

	lightness_srv_default_set(&srv->lightness, ctx, val.lightness);
	bt_mesh_light_hue_srv_default_set(&srv->hue, ctx, val.hue);
	bt_mesh_light_sat_srv_default_set(&srv->sat, ctx, val.saturation);
}

static void default_get_handle(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_HSL_MSG_LEN_GET) {
		return;
	}

	default_rsp(model, ctx);
}

static void default_set_handle(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_HSL_MSG_LEN_DEFAULT) {
		return;
	}

	default_set(model, ctx, buf);
	default_rsp(model, ctx);
}

static void default_set_unack_handle(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_HSL_MSG_LEN_DEFAULT) {
		return;
	}

	default_set(model, ctx, buf);
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

static void range_rsp(struct bt_mesh_model *model,
		      struct bt_mesh_msg_ctx *rx_ctx,
		      enum bt_mesh_model_status status)
{
	struct bt_mesh_light_hsl_srv *srv = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_LIGHT_HSL_OP_RANGE_STATUS,
				 BT_MESH_LIGHT_HSL_MSG_LEN_RANGE_STATUS);
	range_encode_status(&msg, srv, status);
	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static enum bt_mesh_model_status range_set(struct bt_mesh_model *model,
					   struct bt_mesh_msg_ctx *ctx,
					   struct net_buf_simple *buf)
{
	struct bt_mesh_light_hsl_srv *srv = model->user_data;
	struct bt_mesh_light_hsl_range hue, sat;

	hue.min = net_buf_simple_pull_le16(buf);
	hue.max = net_buf_simple_pull_le16(buf);
	if (hue.max < hue.min) {
		return BT_MESH_MODEL_ERROR_INVALID_RANGE_MIN;
	}


	sat.min = net_buf_simple_pull_le16(buf);
	sat.max = net_buf_simple_pull_le16(buf);
	if (sat.max < sat.min) {
		return BT_MESH_MODEL_ERROR_INVALID_RANGE_MIN;
	}

	bt_mesh_light_hue_srv_range_set(&srv->hue, ctx, &hue);
	bt_mesh_light_sat_srv_range_set(&srv->sat, ctx, &sat);

	return BT_MESH_MODEL_SUCCESS;
}

static void range_get_handle(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_HSL_MSG_LEN_GET) {
		return;
	}

	range_rsp(model, ctx, BT_MESH_MODEL_SUCCESS);
}

static void range_set_handle(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_HSL_MSG_LEN_RANGE_SET) {
		return;
	}

	/* According to Test specification MMDL/SR/LHSLSE/BI-01-C, ignore
	 * message if max < min:
	 */
	if (range_set(model, ctx, buf) == BT_MESH_MODEL_SUCCESS) {
		range_rsp(model, ctx, BT_MESH_MODEL_SUCCESS);
	}
}

static void range_set_unack_handle(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	if (buf->len != BT_MESH_LIGHT_HSL_MSG_LEN_RANGE_SET) {
		return;
	}

	range_set(model, ctx, buf);
}

const struct bt_mesh_model_op _bt_mesh_light_hsl_srv_op[] = {
	{
		BT_MESH_LIGHT_HSL_OP_GET,
		BT_MESH_LIGHT_HSL_MSG_LEN_GET,
		hsl_get_handle,
	},
	{
		BT_MESH_LIGHT_HSL_OP_SET,
		BT_MESH_LIGHT_HSL_MSG_MINLEN_SET,
		hsl_set_handle,
	},
	{
		BT_MESH_LIGHT_HSL_OP_SET_UNACK,
		BT_MESH_LIGHT_HSL_MSG_MINLEN_SET,
		hsl_set_unack_handle,
	},
	{
		BT_MESH_LIGHT_HSL_OP_TARGET_GET,
		BT_MESH_LIGHT_HSL_MSG_LEN_GET,
		hsl_target_get_handle,
	},
	{
		BT_MESH_LIGHT_HSL_OP_DEFAULT_GET,
		BT_MESH_LIGHT_HSL_MSG_LEN_GET,
		default_get_handle,
	},
	{
		BT_MESH_LIGHT_HSL_OP_RANGE_GET,
		BT_MESH_LIGHT_HSL_MSG_LEN_GET,
		range_get_handle,
	},
	BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op _bt_mesh_light_hsl_setup_srv_op[] = {
	{
		BT_MESH_LIGHT_HSL_OP_DEFAULT_SET,
		BT_MESH_LIGHT_HSL_MSG_LEN_DEFAULT,
		default_set_handle,
	},
	{
		BT_MESH_LIGHT_HSL_OP_DEFAULT_SET_UNACK,
		BT_MESH_LIGHT_HSL_MSG_LEN_DEFAULT,
		default_set_unack_handle,
	},
	{
		BT_MESH_LIGHT_HSL_OP_RANGE_SET,
		BT_MESH_LIGHT_HSL_MSG_LEN_RANGE_SET,
		range_set_handle,
	},
	{
		BT_MESH_LIGHT_HSL_OP_RANGE_SET_UNACK,
		BT_MESH_LIGHT_HSL_MSG_LEN_RANGE_SET,
		range_set_unack_handle,
	},
	BT_MESH_MODEL_OP_END,
};

static int hsl_srv_pub_update(struct bt_mesh_model *mod)
{
	struct bt_mesh_light_hsl_srv *srv = mod->user_data;
	struct bt_mesh_light_hsl_status status;

	hsl_get(srv, NULL, &status);
	hsl_status_encode(&srv->buf, BT_MESH_LIGHT_HSL_OP_STATUS, &status);

	return 0;
}

static int bt_mesh_light_hsl_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_light_hsl_srv *srv = model->user_data;

	srv->model = model;
	srv->pub.update = hsl_srv_pub_update;
	srv->pub.msg = &srv->buf;
	srv->hue.hsl = srv;
	srv->sat.hsl = srv;
	srv->pub_pending = true;
	net_buf_simple_init_with_data(&srv->buf, srv->pub_data,
				      ARRAY_SIZE(srv->pub_data));

	if (IS_ENABLED(CONFIG_BT_MESH_MODEL_EXTENSIONS)) {
		/* Model extensions:
		 * To simplify the model extension tree, we're flipping the
		 * relationship between the Light HSL server and the Light HSL
		 * setup server. In the specification, the Light HSL setup
		 * server extends the time server, which is the opposite of
		 * what we're doing here. This makes no difference for the mesh
		 * stack, but it makes it a lot easier to extend this model, as
		 * we won't have to support multiple extenders.
		 */
		bt_mesh_model_extend(model, srv->lightness.lightness_model);
		bt_mesh_model_extend(
			model, bt_mesh_model_find(
				       bt_mesh_model_elem(model),
				       BT_MESH_MODEL_ID_LIGHT_HSL_SETUP_SRV));
	}

	return 0;
}

static int bt_mesh_light_hsl_srv_start(struct bt_mesh_model *mod)
{
	struct bt_mesh_light_hsl_srv *srv = mod->user_data;
	struct bt_mesh_model_transition transition;
	struct bt_mesh_light_hue hue = { .transition = &transition };
	struct bt_mesh_light_sat sat = { .transition = &transition };

	if (!srv->sat.model ||
	    (srv->model->elem_idx > srv->sat.model->elem_idx)) {
		BT_ERR("Light HSL srv[%d]: Sat. srv not properly initialized",
		       srv->model->elem_idx);
		return -EINVAL;
	}

	if (!srv->hue.model ||
	    (srv->model->elem_idx > srv->hue.model->elem_idx)) {
		BT_ERR("Light HSL srv[%d]: Hue srv not properly initialized",
		       srv->model->elem_idx);
		return -EINVAL;
	}

	bt_mesh_dtt_srv_transition_get(mod, &transition);

	switch (srv->lightness.ponoff.on_power_up) {
	case BT_MESH_ON_POWER_UP_ON:
	case BT_MESH_ON_POWER_UP_OFF:
		hue.lvl = srv->hue.dflt;
		sat.lvl = srv->sat.dflt;
		break;
	case BT_MESH_ON_POWER_UP_RESTORE:
		hue.lvl = srv->hue.last;
		sat.lvl = srv->sat.last;
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

	struct bt_mesh_light_hsl_status status = {
		.params = {
			.hue = hue.lvl,
			.saturation = sat.lvl,
			/* The lightness server updated its "last" value in its
			 * start function, which has already completed:
			 */
			.lightness = srv->lightness.last,
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

	return model_send(srv->model, ctx, &buf);
}
