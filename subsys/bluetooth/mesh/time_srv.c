/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/time_srv.h>
#include "time_internal.h"
#include "model_utils.h"
#include "time_util.h"

#define SUBSEC_STEPS 256U

struct bt_mesh_time_srv_settings_data {
	uint8_t role;
	int16_t time_zone_offset_new;
	int16_t tai_utc_delta_new;
	uint64_t tai_of_zone_change;
	uint64_t tai_of_delta_change;
};

static inline int64_t zone_offset_to_sec(int16_t raw_zone)
{
	return raw_zone * 15 * SEC_PER_MIN;
}

static inline uint64_t tai_to_ms(const struct bt_mesh_time_tai *tai)
{
	return MSEC_PER_SEC * tai->sec +
	       (MSEC_PER_SEC * tai->subsec) / SUBSEC_STEPS;
}

static inline bool tai_is_unknown(const struct bt_mesh_time_tai *tai)
{
	return !tai->sec && !tai->subsec;
}

static inline struct bt_mesh_time_tai
tai_at(const struct bt_mesh_time_srv *srv, int64_t uptime)
{
	const struct bt_mesh_time_tai *sync = &srv->data.sync.status.tai;
	int64_t steps = (SUBSEC_STEPS * (uptime - srv->data.sync.uptime)) /
			MSEC_PER_SEC;

	if (tai_is_unknown(sync)) {
		return *sync;
	}

	return (struct bt_mesh_time_tai) {
		.sec = sync->sec + (steps / SUBSEC_STEPS),
		.subsec = sync->subsec + steps,
	};
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

static uint64_t get_uncertainty_ms(const struct bt_mesh_time_srv *srv,
				   int64_t uptime)
{
	uint64_t drift = ((uptime - srv->data.sync.uptime) *
			  CONFIG_BT_MESH_TIME_SRV_CLOCK_ACCURACY) /
			 USEC_PER_SEC;

	return srv->data.sync.status.uncertainty + drift;
}

static int16_t get_zone_offset(const struct bt_mesh_time_srv *srv,
			       int64_t uptime)
{
	struct bt_mesh_time_tai tai = tai_at(srv, uptime);

	if (srv->data.time_zone_change.timestamp &&
	    tai.sec >= srv->data.time_zone_change.timestamp) {
		return srv->data.time_zone_change.new_offset;
	}

	return srv->data.sync.status.time_zone_offset;
}

static int16_t get_utc_delta(const struct bt_mesh_time_srv *srv, int64_t uptime)
{
	struct bt_mesh_time_tai tai = tai_at(srv, uptime);

	if (srv->data.tai_utc_change.timestamp &&
	    tai.sec >= srv->data.tai_utc_change.timestamp) {
		return srv->data.tai_utc_change.delta_new;
	}

	return srv->data.sync.status.tai_utc_delta;
}

static int send_zone_status(struct bt_mesh_model *model,
			    struct bt_mesh_msg_ctx *ctx)
{
	struct bt_mesh_time_srv *srv = model->user_data;
	struct bt_mesh_time_zone_status resp = {
		.current_offset = get_zone_offset(srv, k_uptime_get()),
		.time_zone_change = srv->data.time_zone_change,
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
		.delta_current = get_utc_delta(srv, k_uptime_get()),
		.tai_utc_change = srv->data.tai_utc_change,
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
			    struct bt_mesh_msg_ctx *ctx, int64_t uptime)
{
	struct bt_mesh_time_srv *srv = model->user_data;
	struct bt_mesh_time_status status;
	int err;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_TIME_OP_TIME_STATUS,
				 BT_MESH_TIME_MSG_MAXLEN_TIME_STATUS);
	bt_mesh_model_msg_init(&msg, BT_MESH_TIME_OP_TIME_STATUS);

	err = bt_mesh_time_srv_status(srv, uptime, &status);
	if (err) {
		/* Mesh Model Specification 5.2.1.3: If the TAI Seconds field is
		 * 0, all other fields shall be omitted
		 */
		bt_mesh_time_buf_put_tai_sec(&msg, 0);
	} else {
		/* Account for delay in TX processing: */
		status.uncertainty += CONFIG_BT_MESH_TIME_MESH_HOP_UNCERTAINTY;
		bt_mesh_time_encode_time_params(&msg, &status);
	}

	return bt_mesh_msg_send(model, ctx, &msg);
}

static int handle_time_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_time_srv *srv = model->user_data;

	if ((srv->data.role != BT_MESH_TIME_CLIENT) &&
	    (srv->data.role != BT_MESH_TIME_RELAY)) {
		/* Not relevant for this role, ignore. */
		return 0;
	}

	struct bt_mesh_time_status status;

	bt_mesh_time_decode_time_params(buf, &status);

	if (status.is_authority <= srv->data.sync.status.is_authority &&
	    srv->data.sync.status.uncertainty < status.uncertainty) {
		/* The new time status is not an improvement, ignore. */
		return 0;
	}

	srv->data.sync.uptime = k_uptime_get();
	srv->data.sync.status.tai = status.tai;
	srv->data.sync.status.uncertainty = status.uncertainty;
	srv->data.sync.status.time_zone_offset = status.time_zone_offset;
	srv->data.sync.status.tai_utc_delta = status.tai_utc_delta;
	if (srv->time_update_cb != NULL) {
		srv->time_update_cb(srv, ctx, BT_MESH_TIME_SRV_STATUS_UPDATE);
	}

	if (srv->data.role == BT_MESH_TIME_RELAY) {
		(void)bt_mesh_time_srv_time_status_send(srv, NULL);
	}

	return 0;
}

static int handle_time_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	send_time_status(model, ctx, k_uptime_get());

	return 0;
}

static int handle_time_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_time_srv *srv = model->user_data;

	bt_mesh_time_decode_time_params(buf, &srv->data.sync.status);
	srv->data.sync.uptime = k_uptime_get();

	if (srv->time_update_cb != NULL) {
		srv->time_update_cb(srv, ctx, BT_MESH_TIME_SRV_SET_UPDATE);
	}

	send_time_status(model, ctx, srv->data.sync.uptime);

	return 0;
}

static int handle_zone_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	send_zone_status(model, ctx);

	return 0;
}

static int handle_zone_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
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

	return 0;
}

static int handle_tai_utc_delta_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				    struct net_buf_simple *buf)
{
	send_tai_utc_delta_status(model, ctx);

	return 0;
}

static int handle_tai_utc_delta_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
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

	return 0;
}

static int handle_role_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	send_role_status(model, ctx);

	return 0;
}

static int handle_role_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	struct bt_mesh_time_srv *srv = model->user_data;
	enum bt_mesh_time_role role;

	role = net_buf_simple_pull_u8(buf);
	if (role != BT_MESH_TIME_NONE && role != BT_MESH_TIME_AUTHORITY &&
	    role != BT_MESH_TIME_RELAY && role != BT_MESH_TIME_CLIENT) {
		return -EINVAL;
	}

	srv->data.role = role;

	store_state(srv);
	send_role_status(model, ctx);

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_time_srv_op[] = {
	{
		BT_MESH_TIME_OP_TIME_GET,
		BT_MESH_LEN_EXACT(BT_MESH_TIME_MSG_LEN_GET),
		handle_time_get,
	},
	{
		BT_MESH_TIME_OP_TIME_STATUS,
		BT_MESH_LEN_MIN(BT_MESH_TIME_MSG_MINLEN_TIME_STATUS),
		handle_time_status,
	},
	{
		BT_MESH_TIME_OP_TIME_ZONE_GET,
		BT_MESH_LEN_EXACT(BT_MESH_TIME_MSG_LEN_GET),
		handle_zone_get,
	},
	{
		BT_MESH_TIME_OP_TAI_UTC_DELTA_GET,
		BT_MESH_LEN_EXACT(BT_MESH_TIME_MSG_LEN_GET),
		handle_tai_utc_delta_get,
	},
	BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op _bt_mesh_time_setup_srv_op[] = {
	{
		BT_MESH_TIME_OP_TIME_SET,
		BT_MESH_LEN_EXACT(BT_MESH_TIME_MSG_LEN_TIME_SET),
		handle_time_set,
	},
	{
		BT_MESH_TIME_OP_TIME_ZONE_SET,
		BT_MESH_LEN_EXACT(BT_MESH_TIME_MSG_LEN_TIME_ZONE_SET),
		handle_zone_set,
	},
	{
		BT_MESH_TIME_OP_TAI_UTC_DELTA_SET,
		BT_MESH_LEN_EXACT(BT_MESH_TIME_MSG_LEN_TAI_UTC_DELTA_SET),
		handle_tai_utc_delta_set,
	},
	{
		BT_MESH_TIME_OP_TIME_ROLE_GET,
		BT_MESH_LEN_EXACT(BT_MESH_TIME_MSG_LEN_GET),
		handle_role_get,
	},
	{
		BT_MESH_TIME_OP_TIME_ROLE_SET,
		BT_MESH_LEN_EXACT(BT_MESH_TIME_MSG_LEN_TIME_ROLE_SET),
		handle_role_set,
	},
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_time_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_time_srv *srv = model->user_data;

	srv->model = model;
	net_buf_simple_init(srv->pub.msg, 0);

	return 0;
}

static void bt_mesh_time_srv_reset(struct bt_mesh_model *model)
{
	struct bt_mesh_time_srv *srv = model->user_data;
	struct bt_mesh_time_srv_data data = { 0 };

	srv->data = data;
	net_buf_simple_reset(srv->pub.msg);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		(void)bt_mesh_model_data_store(srv->model, false, NULL, NULL,
					       0);
	}
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
	.reset = bt_mesh_time_srv_reset,
#ifdef CONFIG_BT_MESH_TIME_SRV_PERSISTENT
	.settings_set = bt_mesh_time_srv_settings_set,
#endif
};

static int bt_mesh_time_setup_srv_init(struct bt_mesh_model *model)
{
	struct bt_mesh_time_srv *srv = model->user_data;

	return bt_mesh_model_extend(model, srv->model);
}


const struct bt_mesh_model_cb _bt_mesh_time_setup_srv_cb = {
	.init = bt_mesh_time_setup_srv_init,
};

int _bt_mesh_time_srv_update_handler(struct bt_mesh_model *model)
{
	struct bt_mesh_time_srv *srv = model->user_data;
	struct bt_mesh_time_status status;
	int err;

	if (srv->data.role != BT_MESH_TIME_AUTHORITY &&
	    srv->data.role != BT_MESH_TIME_RELAY) {
		return -EPERM;
	}

	err = bt_mesh_time_srv_status(srv, k_uptime_get(), &status);
	if (err) {
		return err;
	}

	/* Account for delay in TX processing: */
	status.uncertainty += CONFIG_BT_MESH_TIME_MESH_HOP_UNCERTAINTY;

	bt_mesh_model_msg_init(srv->pub.msg, BT_MESH_TIME_OP_TIME_STATUS);
	bt_mesh_time_encode_time_params(srv->pub.msg, &status);

	return 0;
}

int bt_mesh_time_srv_time_status_send(struct bt_mesh_time_srv *srv,
				      struct bt_mesh_msg_ctx *ctx)
{
	srv->model->pub->ttl = 0;

	/** Mesh Model Specification 5.3.1.2.2:
	 * The message (Time Status) may be sent as an unsolicited message at any time
	 * if the value of the Time Role state is 0x01 (Time Authority) or 0x02 (Time Relay).
	 */
	if ((srv->data.role != BT_MESH_TIME_AUTHORITY) && (srv->data.role != BT_MESH_TIME_RELAY)) {
		return -EOPNOTSUPP;
	}

	return send_time_status(srv->model, ctx, k_uptime_get());
}

int bt_mesh_time_srv_status(struct bt_mesh_time_srv *srv, uint64_t uptime,
			    struct bt_mesh_time_status *status)
{
	if ((srv->data.sync.uptime > uptime) ||
	    tai_is_unknown(&srv->data.sync.status.tai)) {
		return -EAGAIN;
	}

	status->tai = tai_at(srv, uptime),
	status->uncertainty = get_uncertainty_ms(srv, uptime);
	status->tai_utc_delta = get_utc_delta(srv, uptime);
	status->time_zone_offset = get_zone_offset(srv, uptime);
	status->is_authority = srv->data.sync.status.is_authority;
	return 0;
}

int64_t bt_mesh_time_srv_mktime(struct bt_mesh_time_srv *srv, struct tm *timeptr)
{
	int64_t curr_utc_delta = srv->data.sync.status.tai_utc_delta;
	int64_t curr_time_zone =
		zone_offset_to_sec(srv->data.sync.status.time_zone_offset);
	struct bt_mesh_time_tai tai;
	int64_t sec;
	int err;

	err = ts_to_tai(&tai, timeptr);
	if (err) {
		return err;
	}

	sec = tai.sec;

	int64_t new_time_zone =
		zone_offset_to_sec(srv->data.time_zone_change.new_offset);

	if (srv->data.time_zone_change.timestamp &&
	    sec >= srv->data.time_zone_change.timestamp + new_time_zone) {
		sec -= new_time_zone;
	} else {
		sec -= curr_time_zone;
	}

	int64_t new_utc_delta = srv->data.tai_utc_change.delta_new;

	if (srv->data.tai_utc_change.timestamp &&
	    sec >= srv->data.tai_utc_change.timestamp - new_utc_delta) {
		sec += new_utc_delta;
	} else {
		sec += curr_utc_delta;
	}

	if ((sec < 0) || (sec < srv->data.sync.status.tai.sec)) {
		return -EINVAL;
	}

	tai.sec = sec;

	return tai_to_ms(&tai) - tai_to_ms(&srv->data.sync.status.tai) +
	       srv->data.sync.uptime;
}

struct tm *bt_mesh_time_srv_localtime_r(struct bt_mesh_time_srv *srv,
					int64_t uptime, struct tm *timeptr)
{
	if (tai_is_unknown(&srv->data.sync.status.tai) ||
	    srv->data.sync.uptime > uptime) {
		return NULL;
	}

	struct bt_mesh_time_tai tai = tai_at(srv, uptime);

	tai.sec += zone_offset_to_sec(get_zone_offset(srv, uptime));
	tai.sec -= get_utc_delta(srv, uptime);

	tai_to_ts(&tai, timeptr);
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

	return get_uncertainty_ms(srv, uptime);
}

void bt_mesh_time_srv_time_set(struct bt_mesh_time_srv *srv, int64_t uptime,
			       const struct bt_mesh_time_status *status)
{
	srv->data.sync.uptime = uptime;
	srv->data.sync.status = *status;
}

void bt_mesh_time_srv_time_zone_change_set(
	struct bt_mesh_time_srv *srv,
	const struct bt_mesh_time_zone_change *data)
{
	srv->data.time_zone_change = *data;
	store_state(srv);
}

void bt_mesh_time_srv_tai_utc_change_set(
	struct bt_mesh_time_srv *srv,
	const struct bt_mesh_time_tai_utc_change *data)
{
	srv->data.tai_utc_change = *data;
	store_state(srv);
}

void bt_mesh_time_srv_role_set(struct bt_mesh_time_srv *srv,
			       enum bt_mesh_time_role role)
{
	srv->data.role = role;
	store_state(srv);
}
