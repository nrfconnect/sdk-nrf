/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bluetooth/mesh/time_srv.h>
#include "time_internal.h"
#include <stdlib.h>
#include "model_utils.h"
#include "time_util.h"

#define TIME_UNKNOWN 0
#define ZONE_OFFSET_ZERO 64U
#define SEC_PER_MIN 60U
#define MSEC_PER_MIN ((SEC_PER_MIN) * (MSEC_PER_SEC))
#define TIME_ZONE_OFFSET_UNIT_STEP (15U * MSEC_PER_MIN)
#define MS_PER_15_MIN (15 * 60 * 1000)
#define MS_PER_SEC 1000

struct bt_mesh_time_srv_settings_data {
	uint8_t role;
	int16_t time_zone_offset_new;
	int16_t tai_utc_delta_new;
	uint64_t tai_of_zone_change;
	uint64_t tai_of_delta_change;
};

static inline int64_t utc_change_to_ms(int16_t raw_utc)
{
	return raw_utc * MS_PER_SEC;
}

static inline int64_t zone_offset_to_ms(int16_t raw_zone)
{
	return raw_zone * MS_PER_15_MIN;
}

static int store_state(struct bt_mesh_time_srv *srv)
{
	if (!IS_ENABLED(CONFIG_BT_SETTINGS)) {
		return 0;
	}

	struct bt_mesh_time_srv_settings_data data = {
		.role = srv->data.role,
		.time_zone_offset_new = srv->data.time_zone_change.new_offset,
		.tai_of_zone_change = srv->data.time_zone_change.timestamp,
		.tai_utc_delta_new = srv->data.tai_utc_change.delta_new,
		.tai_of_delta_change = srv->data.tai_utc_change.timestamp,
	};

	return bt_mesh_model_data_store(srv->model, false, NULL, &data, sizeof(data));
}

static uint64_t get_tai_ms(struct bt_mesh_time_srv_data *data, int64_t uptime)
{
	if (data->sync.status.tai == TIME_UNKNOWN) {
		return 0;
	}

	return data->sync.status.tai + (uptime - data->sync.uptime);
}

static uint64_t get_uncertainty_ms(struct bt_mesh_time_srv_data *data,
				int64_t uptime)
{
	return data->sync.status.uncertainty +
	       (((uptime - data->sync.uptime) *
		 CONFIG_BT_MESH_TIME_SRV_CLOCK_ACCURACY) /
		USEC_PER_SEC);
}

static int16_t get_zone_offset(struct bt_mesh_time_srv_data *data, int64_t uptime)
{
	uint64_t curr_tai = get_tai_ms(data, uptime);

	if ((curr_tai >= (data->time_zone_change.timestamp * MSEC_PER_SEC)) &&
	    (data->time_zone_change.timestamp != TIME_UNKNOWN)) {
		return data->time_zone_change.new_offset;
	} else {
		return data->sync.status.time_zone_offset;
	}
}

static int16_t get_utc_delta(struct bt_mesh_time_srv_data *data, int64_t uptime)
{
	uint64_t curr_tai = get_tai_ms(data, uptime);

	if ((curr_tai >= (data->tai_utc_change.timestamp * MSEC_PER_SEC)) &&
	    (data->tai_utc_change.timestamp != TIME_UNKNOWN)) {
		return data->tai_utc_change.delta_new;
	} else {
		return data->sync.status.tai_utc_delta;
	}
}

static void check_zone_utc_overflow(struct bt_mesh_time_srv *srv)
{
	bool store = false;

	if ((srv->data.time_zone_change.timestamp * MSEC_PER_SEC) <=
	    srv->data.sync.status.tai) {
		srv->data.time_zone_change.new_offset =
			srv->data.sync.status.time_zone_offset;
		store = true;
	}

	if ((srv->data.tai_utc_change.timestamp * MSEC_PER_SEC) <=
	    srv->data.sync.status.tai) {
		srv->data.tai_utc_change.delta_new =
			srv->data.sync.status.tai_utc_delta;
		store = true;
	}

	if (store) {
		store_state(srv);
	}
}

static int send_zone_status(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx)
{
	struct bt_mesh_time_srv *srv = model->user_data;
	struct bt_mesh_time_zone_status resp = {
		.current_offset = get_zone_offset(&srv->data, k_uptime_get()),
		.time_zone_change.new_offset =
			srv->data.time_zone_change.new_offset,
		.time_zone_change.timestamp =
			srv->data.time_zone_change.timestamp,
	};
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_TIME_OP_TIME_ZONE_STATUS,
				 BT_MESH_TIME_MSG_LEN_TIME_ZONE_STATUS);
	bt_mesh_model_msg_init(&msg, BT_MESH_TIME_OP_TIME_ZONE_STATUS);

	net_buf_simple_add_u8(
		&msg, (uint8_t)(resp.current_offset + ZONE_CHANGE_ZERO_POINT));
	net_buf_simple_add_u8(&msg, (uint8_t)(resp.time_zone_change.new_offset +
					   ZONE_CHANGE_ZERO_POINT));
	bt_mesh_time_buf_put_tai_sec(&msg, resp.time_zone_change.timestamp);

	return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int send_tai_utc_delta_status(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx)
{
	struct bt_mesh_time_srv *srv = model->user_data;
	struct bt_mesh_time_tai_utc_delta_status resp = {
		.delta_current = get_utc_delta(&srv->data, k_uptime_get()),
		.tai_utc_change.delta_new = srv->data.tai_utc_change.delta_new,
		.tai_utc_change.timestamp = srv->data.tai_utc_change.timestamp,
	};
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_TIME_OP_TAI_UTC_DELTA_STATUS,
				 BT_MESH_TIME_MSG_LEN_TAI_UTC_DELTA_STATUS);
	bt_mesh_model_msg_init(&msg, BT_MESH_TIME_OP_TAI_UTC_DELTA_STATUS);
	net_buf_simple_add_le16(
		&msg, (uint16_t)(resp.delta_current + UTC_CHANGE_ZERO_POINT));
	net_buf_simple_add_le16(&msg, (uint16_t)(resp.tai_utc_change.delta_new +
					      UTC_CHANGE_ZERO_POINT));
	bt_mesh_time_buf_put_tai_sec(&msg, resp.tai_utc_change.timestamp);

	return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int send_role_status(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx)
{
	struct bt_mesh_time_srv *srv = model->user_data;
	uint8_t resp = srv->data.role;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_TIME_OP_TIME_ROLE_STATUS,
				 BT_MESH_TIME_MSG_LEN_TIME_ROLE_STATUS);
	bt_mesh_model_msg_init(&msg, BT_MESH_TIME_OP_TIME_ROLE_STATUS);

	net_buf_simple_add_u8(&msg, resp);

	return bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int send_time_status(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *rx_ctx, int64_t uptime)
{
	struct bt_mesh_time_srv *srv = model->user_data;
	struct bt_mesh_time_status status = {
		.tai = get_tai_ms(&srv->data, uptime),
		.uncertainty = get_uncertainty_ms(&srv->data, uptime),
		.is_authority = srv->data.sync.status.is_authority,
		.time_zone_offset = get_zone_offset(&srv->data, uptime),
		.tai_utc_delta = get_utc_delta(&srv->data, uptime),
	};

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_TIME_OP_TIME_STATUS,
				 BT_MESH_TIME_MSG_LEN_TIME_STATUS);
	bt_mesh_model_msg_init(&msg, BT_MESH_TIME_OP_TIME_STATUS);
	bt_mesh_time_encode_time_params(&msg, &status);

	return bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static void handle_time_status(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	struct bt_mesh_time_srv *srv = model->user_data;

	if ((srv->data.role != BT_MESH_TIME_CLIENT) &&
	    (srv->data.role != BT_MESH_TIME_RELAY)) {
		return;
	}

	struct bt_mesh_time_status status;

	bt_mesh_time_decode_time_params(buf, &status);
	srv->data.sync.uptime = k_uptime_get();
	srv->data.sync.status.tai = status.tai;
	srv->data.sync.status.uncertainty = status.uncertainty;
	srv->data.sync.status.time_zone_offset = status.time_zone_offset;
	srv->data.sync.status.tai_utc_delta = status.tai_utc_delta;
	check_zone_utc_overflow(srv);
	if (srv->time_update_cb != NULL) {
		srv->time_update_cb(srv, ctx, BT_MESH_TIME_SRV_STATUS_UPDATE);
	}

	if (srv->data.role == BT_MESH_TIME_RELAY) {
		(void)bt_mesh_time_srv_time_status_send(srv, NULL);
	}
}

static void handle_time_get(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	send_time_status(model, ctx, k_uptime_get());
}

static void handle_time_set(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct bt_mesh_time_srv *srv = model->user_data;
	struct bt_mesh_time_status set_params;

	bt_mesh_time_decode_time_params(buf, &set_params);
	srv->data.sync.uptime = k_uptime_get();
	srv->data.sync.status.tai = set_params.tai;
	srv->data.sync.status.uncertainty = set_params.uncertainty;
	srv->data.sync.status.is_authority = set_params.is_authority;
	srv->data.sync.status.time_zone_offset = set_params.time_zone_offset;
	srv->data.sync.status.tai_utc_delta = set_params.tai_utc_delta;
	check_zone_utc_overflow(srv);
	if (srv->time_update_cb != NULL) {
		srv->time_update_cb(srv, ctx, BT_MESH_TIME_SRV_SET_UPDATE);
	}

	send_time_status(model, ctx, srv->data.sync.uptime);
}

static void handle_zone_get(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	send_zone_status(model, ctx);
}

static void handle_zone_set(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct bt_mesh_time_srv *srv = model->user_data;

	srv->data.time_zone_change.new_offset =
		net_buf_simple_pull_u8(buf) - ZONE_CHANGE_ZERO_POINT;
	srv->data.time_zone_change.timestamp =
		bt_mesh_time_buf_pull_tai_sec(buf);
	store_state(srv);
	if (srv->time_update_cb != NULL) {
		srv->time_update_cb(srv, ctx, BT_MESH_TIME_SRV_ZONE_UPDATE);
	}

	send_zone_status(model, ctx);
}

static void handle_tai_utc_delta_get(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	send_tai_utc_delta_status(model, ctx);
}

static void handle_tai_utc_delta_set(struct bt_mesh_model *model,
				     struct bt_mesh_msg_ctx *ctx,
				     struct net_buf_simple *buf)
{
	struct bt_mesh_time_srv *srv = model->user_data;

	srv->data.tai_utc_change.delta_new =
		net_buf_simple_pull_le16(buf) - UTC_CHANGE_ZERO_POINT;
	srv->data.tai_utc_change.timestamp = bt_mesh_time_buf_pull_tai_sec(buf);
	store_state(srv);
	if (srv->time_update_cb != NULL) {
		srv->time_update_cb(srv, ctx, BT_MESH_TIME_SRV_UTC_UPDATE);
	}

	send_tai_utc_delta_status(model, ctx);
}

static void handle_role_get(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	send_role_status(model, ctx);
}

static void handle_role_set(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	struct bt_mesh_time_srv *srv = model->user_data;

	srv->data.role = net_buf_simple_pull_u8(buf);
	store_state(srv);
	send_role_status(model, ctx);
}

const struct bt_mesh_model_op _bt_mesh_time_srv_op[] = {
	{ BT_MESH_TIME_OP_TIME_GET, BT_MESH_TIME_MSG_LEN_GET, handle_time_get },
	{ BT_MESH_TIME_OP_TIME_STATUS, BT_MESH_TIME_MSG_LEN_TIME_STATUS,
	  handle_time_status },
	{ BT_MESH_TIME_OP_TIME_ZONE_GET, BT_MESH_TIME_MSG_LEN_GET,
	  handle_zone_get },
	{ BT_MESH_TIME_OP_TAI_UTC_DELTA_GET, BT_MESH_TIME_MSG_LEN_GET,
	  handle_tai_utc_delta_get },
	BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op _bt_mesh_time_setup_srv_op[] = {
	{ BT_MESH_TIME_OP_TIME_SET, BT_MESH_TIME_MSG_LEN_TIME_SET,
	  handle_time_set },
	{ BT_MESH_TIME_OP_TIME_ZONE_SET, BT_MESH_TIME_MSG_LEN_TIME_ZONE_SET,
	  handle_zone_set },
	{ BT_MESH_TIME_OP_TAI_UTC_DELTA_SET,
	  BT_MESH_TIME_MSG_LEN_TAI_UTC_DELTA_SET, handle_tai_utc_delta_set },
	{ BT_MESH_TIME_OP_TIME_ROLE_GET, BT_MESH_TIME_MSG_LEN_GET,
	  handle_role_get },
	{ BT_MESH_TIME_OP_TIME_ROLE_SET, BT_MESH_TIME_MSG_LEN_TIME_ROLE_SET,
	  handle_role_set },
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_time_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_time_srv *srv = model->user_data;

	srv->model = model;
	net_buf_simple_init(srv->pub.msg, 0);
	if (IS_ENABLED(CONFIG_BT_MESH_MODEL_EXTENSIONS)) {
		/* Model extensions:
		 * To simplify the model extension tree, we're flipping the
		 * relationship between the time server and the time
		 * setup server. In the specification, the time setup
		 * server extends the time server, which is the opposite of
		 * what we're doing here. This makes no difference for the mesh
		 * stack, but it makes it a lot easier to extend this model, as
		 * we won't have to support multiple extenders.
		 */
		bt_mesh_model_extend(
			model,
			bt_mesh_model_find(bt_mesh_model_elem(model),
					   BT_MESH_MODEL_ID_TIME_SETUP_SRV));
	}

	return 0;
}

#ifdef CONFIG_BT_MESH_TIME_SRV_PERSISTENT
static int bt_mesh_time_srv_settings_set(struct bt_mesh_model *model,
					 const char *name, size_t len_rd,
					 settings_read_cb read_cb, void *cb_arg)
{
	struct bt_mesh_time_srv *srv = model->user_data;
	struct bt_mesh_time_srv_settings_data data;

	if (read_cb(cb_arg, &data, sizeof(data)) != sizeof(data)) {
		return -EINVAL;
	}

	srv->data.role = data.role;
	srv->data.time_zone_change.new_offset = data.time_zone_offset_new;
	srv->data.time_zone_change.timestamp = data.tai_of_zone_change;
	srv->data.tai_utc_change.delta_new = data.tai_utc_delta_new;
	srv->data.tai_utc_change.timestamp = data.tai_of_delta_change;

	return 0;
}
#endif

const struct bt_mesh_model_cb _bt_mesh_time_srv_cb = {
	.init = bt_mesh_time_srv_init,
#ifdef CONFIG_BT_MESH_TIME_SRV_PERSISTENT
	.settings_set = bt_mesh_time_srv_settings_set,
#endif
};

int _bt_mesh_time_srv_update_handler(struct bt_mesh_model *model)
{
	struct bt_mesh_time_srv *srv = model->user_data;

	(void)bt_mesh_time_srv_time_status_send(srv, NULL);
	return 0;
}

int bt_mesh_time_srv_time_status_send(struct bt_mesh_time_srv *srv,
				      struct bt_mesh_msg_ctx *ctx)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_TIME_OP_TIME_STATUS,
				 BT_MESH_TIME_MSG_LEN_TIME_STATUS);
	bt_mesh_model_msg_init(&msg, BT_MESH_TIME_OP_TIME_STATUS);
	struct bt_mesh_time_status status = {
		.tai = get_tai_ms(&srv->data, k_uptime_get()),
		.uncertainty = get_uncertainty_ms(&srv->data, k_uptime_get()) +
			       CONFIG_BT_MESH_TIME_MESH_HOP_UNCERTAINTY,
		.is_authority = srv->data.sync.status.is_authority,
		.time_zone_offset = get_zone_offset(&srv->data, k_uptime_get()),
		.tai_utc_delta = get_utc_delta(&srv->data, k_uptime_get()),
	};

	bt_mesh_time_encode_time_params(&msg, &status);
	srv->model->pub->ttl = 0;

	return model_send(srv->model, ctx, &msg);
}

int bt_mesh_time_srv_status(struct bt_mesh_time_srv *srv, uint64_t uptime,
			    struct bt_mesh_time_status *status)
{
	if ((srv->data.sync.uptime > uptime) ||
	    srv->data.sync.status.tai == TIME_UNKNOWN) {
		return -EAGAIN;
	}

	status->tai = get_tai_ms(&srv->data, uptime);
	status->uncertainty = get_uncertainty_ms(&srv->data, uptime);
	status->tai_utc_delta =
		status->tai < (srv->data.tai_utc_change.timestamp *
			       MSEC_PER_SEC) ?
			srv->data.sync.status.tai_utc_delta :
			srv->data.tai_utc_change.delta_new;
	status->time_zone_offset =
		status->tai < (srv->data.time_zone_change.timestamp *
			       MSEC_PER_SEC) ?
			srv->data.sync.status.time_zone_offset :
			srv->data.time_zone_change.new_offset;
	status->is_authority = srv->data.sync.status.is_authority;
	return 0;
}

int64_t bt_mesh_time_srv_mktime(struct bt_mesh_time_srv *srv, struct tm *timeptr)
{
	int err;
	int64_t mod_uptime;

	err = ts_to_tai(&mod_uptime, timeptr);
	if (err) {
		return err;
	}

	mod_uptime -= zone_offset_to_ms(srv->data.sync.status.time_zone_offset);
	mod_uptime += utc_change_to_ms(srv->data.sync.status.tai_utc_delta);

	int64_t curr_time_zone =
		zone_offset_to_ms(srv->data.sync.status.time_zone_offset);
	int64_t new_time_zone =
		zone_offset_to_ms(srv->data.time_zone_change.new_offset);

	if (srv->data.time_zone_change.timestamp &&
	    (mod_uptime >=
	     (srv->data.time_zone_change.timestamp * MSEC_PER_SEC +
	      new_time_zone))) {
		mod_uptime -= new_time_zone;
	} else {
		mod_uptime -= curr_time_zone;
	}

	int64_t curr_utc_delta =
		utc_change_to_ms(srv->data.sync.status.tai_utc_delta);
	int64_t new_utc_delta =
		utc_change_to_ms(srv->data.tai_utc_change.delta_new);

	if (srv->data.tai_utc_change.timestamp &&
	    (mod_uptime >= (srv->data.tai_utc_change.timestamp * MSEC_PER_SEC -
			    new_utc_delta))) {
		mod_uptime += new_utc_delta;
	} else {
		mod_uptime += curr_utc_delta;
	}

	if ((mod_uptime < 0) || (mod_uptime < srv->data.sync.status.tai)) {
		return -EINVAL;
	}

	return mod_uptime - srv->data.sync.status.tai + srv->data.sync.uptime;
}

struct tm *bt_mesh_time_srv_localtime_r(struct bt_mesh_time_srv *srv,
					int64_t uptime, struct tm *timeptr)
{
	if (srv->data.sync.uptime > uptime) {
		return NULL;
	}

	int64_t local_time_ms = get_tai_ms(&srv->data, uptime);
	int64_t current_zone_offset =
		zone_offset_to_ms(get_zone_offset(&srv->data, uptime));
	int64_t current_utc_offset =
		utc_change_to_ms(get_utc_delta(&srv->data, uptime));
	int64_t mod_uptime =
		local_time_ms + current_zone_offset - current_utc_offset;
	if (mod_uptime < 0) {
		return NULL;
	}

	tai_to_ts(mod_uptime, timeptr);
	return timeptr;
}

struct tm *bt_mesh_time_srv_localtime(struct bt_mesh_time_srv *srv,
				      int64_t uptime)
{
	static struct tm timeptr;

	return bt_mesh_time_srv_localtime_r(srv, uptime, &timeptr);
}

uint64_t bt_mesh_time_srv_uncertainty_get(struct bt_mesh_time_srv *srv,
				       int64_t uptime)
{
	if ((uptime - srv->data.sync.uptime) < 0) {
		return -EAGAIN;
	}

	return get_uncertainty_ms(&srv->data, uptime);
}

void bt_mesh_time_srv_time_set(struct bt_mesh_time_srv *srv, int64_t uptime,
			       const struct bt_mesh_time_status *status)
{
	srv->data.sync.uptime = uptime;
	memcpy(&srv->data.sync.status, status,
	       sizeof(struct bt_mesh_time_status));
}

void bt_mesh_time_srv_time_zone_change_set(
	struct bt_mesh_time_srv *srv,
	const struct bt_mesh_time_zone_change *data)
{
	memcpy(&srv->data.time_zone_change, data,
	       sizeof(struct bt_mesh_time_zone_change));
	store_state(srv);
}

void bt_mesh_time_srv_tai_utc_change_set(
	struct bt_mesh_time_srv *srv,
	const struct bt_mesh_time_tai_utc_change *data)
{
	memcpy(&srv->data.tai_utc_change, data,
	       sizeof(struct bt_mesh_time_tai_utc_change));
	store_state(srv);
}

void bt_mesh_time_srv_role_set(struct bt_mesh_time_srv *srv,
			       enum bt_mesh_time_role role)
{
	srv->data.role = role;
	store_state(srv);
}
