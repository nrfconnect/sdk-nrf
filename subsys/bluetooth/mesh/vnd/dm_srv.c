/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/mesh/vnd/dm_srv.h>
#include "../model_utils.h"
#include <dm.h>
#include "mesh/net.h"
#include "mesh/access.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_mesh_dm_srv);

BUILD_ASSERT(CONFIG_BT_MESH_DM_SRV_DEFAULT_TTL != 1, "Illegal TTL value for DM server.");
BUILD_ASSERT(CONFIG_MPSL_TIMESLOT_SESSION_COUNT >= 2, "Illegal MPSL timelsot cnt for DM server.");

struct bt_mesh_dm_srv *dm_srv;

struct settings_data {
	struct bt_mesh_dm_cfg cfg;
} __packed;

#if CONFIG_BT_SETTINGS
static void bt_mesh_dm_srv_pending_store(const struct bt_mesh_model *model)
{
	struct bt_mesh_dm_srv *srv = model->user_data;

	struct settings_data data = {
		.cfg = srv->cfg,
	};

	(void)bt_mesh_model_data_store(srv->model, true, NULL, &data,
				       sizeof(data));
}
#endif

static void store(struct bt_mesh_dm_srv *srv)
{
#if CONFIG_BT_SETTINGS
	bt_mesh_model_data_store_schedule(srv->model);
#endif
}

static uint16_t result_convert_cm(float val)
{
	int32_t temp = val * 100;

	if (temp < 0) {
		return 0;
	}

	return temp > UINT16_MAX ? UINT16_MAX : (uint16_t)temp;
}

/** Gets the pointer reference to the latest valid entry relative to the offset */
static struct bt_mesh_dm_res_entry *entry_get(struct bt_mesh_dm_srv *srv, uint8_t offset)
{
	return &srv->results.res[(srv->results.last_entry_idx + srv->results.entry_cnt - offset) %
				 srv->results.entry_cnt];
}

/* Update last entry index and return pointer to the new entry slot */
static struct bt_mesh_dm_res_entry *entry_create(struct bt_mesh_dm_srv *srv)
{
	srv->results.last_entry_idx = (srv->results.last_entry_idx + 1) % srv->results.entry_cnt;

	return &srv->results.res[srv->results.last_entry_idx];
}

static void new_entry_store(struct dm_result *result)
{
	struct bt_mesh_dm_res_entry *entry = entry_create(dm_srv);

	dm_srv->results.available_entries =
		(dm_srv->results.available_entries + 1) >= dm_srv->results.entry_cnt ?
			dm_srv->results.entry_cnt :
			(dm_srv->results.available_entries + 1);

	memset(entry, 0, sizeof(struct bt_mesh_dm_res_entry));

	entry->mode = result->ranging_mode;
	entry->addr = dm_srv->target_addr;
	entry->quality = result->quality;
	entry->err_occurred = !result->status;

	if (entry->err_occurred) {
		return;
	}

	if (entry->mode == DM_RANGING_MODE_RTT) {
		entry->res.rtt = result_convert_cm(result->dist_estimates.rtt.rtt);
	} else {
		entry->res.mcpd.best = result_convert_cm(result->dist_estimates.mcpd.best);
		entry->res.mcpd.ifft = result_convert_cm(result->dist_estimates.mcpd.ifft);
		entry->res.mcpd.phase_slope =
			result_convert_cm(result->dist_estimates.mcpd.phase_slope);
		entry->res.mcpd.rssi_openspace =
			result_convert_cm(result->dist_estimates.mcpd.rssi_openspace);
	}
}

static void cfg_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *rx_ctx,
			    uint8_t status)
{
	struct bt_mesh_dm_srv *srv = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_DM_CLI_OP,
				 BT_MESH_DM_CONFIG_STATUS_MSG_LEN);
	bt_mesh_model_msg_init(&msg, BT_MESH_DM_CLI_OP);
	net_buf_simple_add_u8(&msg, BT_MESH_DM_CONFIG_STATUS_OP);
	net_buf_simple_add_u8(&msg, status);
	net_buf_simple_add_u8(&msg, (srv->is_busy << 7) |
					    (srv->results.available_entries & 0x7F));
	net_buf_simple_add_u8(&msg, srv->cfg.delay);
	net_buf_simple_add_u8(&msg, srv->cfg.ttl);
	net_buf_simple_add_u8(&msg, srv->cfg.timeout);

	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static void result_pack(struct bt_mesh_dm_res_entry *entry, struct net_buf_simple *buf)
{
	uint8_t temp = (entry->mode << 7);

	temp |= ((entry->quality & 0x7) << 4);
	temp |= ((entry->err_occurred & 0x1) << 3);

	net_buf_simple_add_u8(buf, temp);
	net_buf_simple_add_le16(buf, entry->addr);

	if (entry->err_occurred) {
		return;
	}

	if (entry->mode == DM_RANGING_MODE_RTT) {
		net_buf_simple_add_le16(buf, entry->res.rtt);
	} else {
		net_buf_simple_add_le16(buf, entry->res.mcpd.best);
		net_buf_simple_add_le16(buf, entry->res.mcpd.ifft);
		net_buf_simple_add_le16(buf, entry->res.mcpd.phase_slope);
		net_buf_simple_add_le16(buf, entry->res.mcpd.rssi_openspace);
	}
}

static void result_status_populate(const struct bt_mesh_model *model, struct net_buf_simple *buf,
				   uint8_t cnt)
{
	struct bt_mesh_dm_srv *srv = model->user_data;

	for (int i = 0; i < MIN(cnt, srv->results.available_entries); i++) {
		struct bt_mesh_dm_res_entry *entry = entry_get(srv, i);

		result_pack(entry, buf);
	}
}

static void result_status_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *rx_ctx,
			       uint8_t cnt, int8_t status)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_DM_CLI_OP,
				 BT_MESH_DM_RESULT_STATUS_MSG_MAX_LEN);
	bt_mesh_model_msg_init(&msg, BT_MESH_DM_CLI_OP);
	net_buf_simple_add_u8(&msg, BT_MESH_DM_RESULT_STATUS_OP);
	net_buf_simple_add_u8(&msg, status);

	/* Omit last results field if status is not set to success*/
	if (status) {
		(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
		return;
	}

	result_status_populate(model, &msg, cnt);

	(void)bt_mesh_model_send(model, rx_ctx, &msg, NULL, NULL);
}

static void sync_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, uint8_t mode,
		      uint8_t timeout)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_DM_SRV_OP, BT_MESH_DM_SYNC_MSG_LEN);
	bt_mesh_model_msg_init(&msg, BT_MESH_DM_SRV_OP);
	net_buf_simple_add_u8(&msg, BT_MESH_DM_SYNC_OP);
	net_buf_simple_add_u8(&msg, mode);
	net_buf_simple_add_u8(&msg, timeout);
	(void)bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static int handle_cfg(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	struct bt_mesh_dm_srv *srv = model->user_data;

	if (!buf->len) {
		cfg_status_send(model, ctx, 0);
		return 0;
	}

	uint8_t ttl = net_buf_simple_pull_u8(buf);
	uint8_t timeout = net_buf_simple_pull_u8(buf);
	uint8_t delay = net_buf_simple_pull_u8(buf);

	if ((ttl == 1 || ttl > 127) || timeout < 10) {
		LOG_WRN("Invalid params for configuring DM");
		cfg_status_send(model, ctx, EBADMSG);
		return -EBADMSG;
	}

	srv->cfg.ttl = ttl;
	srv->cfg.timeout = timeout;
	srv->cfg.delay = delay;

	store(srv);

	cfg_status_send(model, ctx, 0);
	return 0;
}

static int handle_start(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	int err = 0;
	struct bt_mesh_dm_srv *srv = model->user_data;
	uint8_t timeout = srv->cfg.timeout;
	uint8_t delay = srv->cfg.delay;
	uint16_t temp = net_buf_simple_pull_le16(buf);
	uint8_t tid = net_buf_simple_pull_u8(buf);

	if (tid_check_and_update(&srv->prev_transaction, tid, ctx) != 0) {
		/* Ignore duplicate message.
		 * (Do not send a response, because this might arrive before
		 * the successful response which is sent after ranging and
		 * confuse clients)
		 */
		return 0;
	}

	if (srv->is_busy) {
		LOG_ERR("Failed to start DM. Measurement already in progress");
		err = EBUSY;
		goto rsp;
	}

	srv->target_addr = temp & 0x7fff;

	enum dm_ranging_mode dm_mode = (enum dm_ranging_mode)(temp >> 15);
	struct bt_mesh_msg_ctx tx_ctx = {
		.net_idx = ctx->net_idx,
		.addr = srv->target_addr,
		.app_idx = ctx->app_idx,
		.send_ttl = srv->cfg.ttl,
	};

	/* If optional cfg params is available, overwrite default ones */
	if (buf->len == 3) {
		tx_ctx.send_ttl = net_buf_simple_pull_u8(buf);
		timeout = net_buf_simple_pull_u8(buf);
		delay = net_buf_simple_pull_u8(buf);
	}

	if ((tx_ctx.send_ttl == 1 || tx_ctx.send_ttl > 127) || (timeout < 10) || !tx_ctx.addr) {
		LOG_WRN("Invalid params for starting DM");
		err = EBADMSG;
		goto rsp;
	}

	struct dm_request req = {
		.role = DM_ROLE_REFLECTOR,
		.ranging_mode = dm_mode,
		.rng_seed = ((bt_mesh_primary_addr() + srv->model->elem_idx) << 16) +
			srv->target_addr,
		.start_delay_us = delay,
		.extra_window_time_us = CONFIG_BT_MESH_DM_SRV_REFLECTOR_RANGING_WINDOW_US
	};

	LOG_INF("Starting %s DM for target node 0x%04x",
		dm_mode == DM_RANGING_MODE_RTT ? "RTT" : "MCPD", dm_srv->target_addr);

	srv->is_busy = true;
	k_sem_reset(&dm_srv->dm_ready_sem);
	err = dm_request_add(&req);
	if (err) {
		LOG_ERR("DM add request failed: %d", err);
		srv->is_busy = false;
		goto rsp;
	}

	sync_send(model, &tx_ctx, dm_mode, timeout);
	srv->rsp_ctx = *ctx;
	srv->current_role = DM_ROLE_REFLECTOR;
	k_work_reschedule(&srv->timeout, K_MSEC(timeout * 100));
	return 0;

rsp:
	result_status_send(model, ctx, 1, err);
	return -err;
}

static int handle_result_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			     struct net_buf_simple *buf)
{
	struct bt_mesh_dm_srv *srv = model->user_data;
	uint8_t cnt = net_buf_simple_pull_u8(buf);

	if (!cnt) {
		result_status_send(model, ctx, cnt, EBADMSG);
		return 0;
	}

	if (!srv->results.available_entries) {
		result_status_send(model, ctx, cnt, ENOENT);
		return 0;
	}

	result_status_send(model, ctx, cnt, 0);

	return 0;
}

static int handle_sync(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	int err;
	struct bt_mesh_dm_srv *srv = model->user_data;

	if (srv->is_busy) {
		return -EBUSY;
	}

	srv->target_addr = ctx->addr;

	enum dm_ranging_mode mode = net_buf_simple_pull_u8(buf);
	uint8_t timeout = net_buf_simple_pull_u8(buf);
	struct dm_request req = {
		.role = DM_ROLE_INITIATOR,
		.ranging_mode = mode,
		.rng_seed = (ctx->addr << 16) + ctx->recv_dst,
		.start_delay_us = 0,
		.extra_window_time_us = CONFIG_BT_MESH_DM_SRV_INITIATOR_RANGING_WINDOW_US
	};

	srv->is_busy = true;
	k_sem_reset(&srv->dm_ready_sem);
	err = dm_request_add(&req);
	if (err) {
		LOG_ERR("DM add request failed: %d", err);
		srv->is_busy = false;
		return err;
	}

	srv->current_role = DM_ROLE_INITIATOR;
	k_work_reschedule(&srv->timeout, K_MSEC(timeout * 100));

	return 0;
}

static int handle_msg(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	uint8_t opcode = net_buf_simple_pull_u8(buf);

	switch (opcode) {
	case BT_MESH_DM_CONFIG_OP:
		return handle_cfg(model, ctx, buf);
	case BT_MESH_DM_START_OP:
		return handle_start(model, ctx, buf);
	case BT_MESH_DM_RESULT_GET_OP:
		return handle_result_get(model, ctx, buf);
	case BT_MESH_DM_SYNC_OP:
		return handle_sync(model, ctx, buf);
	}

	return -EOPNOTSUPP;
}

const struct bt_mesh_model_op _bt_mesh_dm_srv_op[] = {
	{
		BT_MESH_DM_SRV_OP,
		BT_MESH_LEN_MIN(BT_MESH_DM_CONFIG_MSG_LEN_MIN),
		handle_msg,
	},
	BT_MESH_MODEL_OP_END,
};

static void data_ready(struct dm_result *result)
{
	if (!dm_srv->is_busy) {
		LOG_WRN("Unexpected DM result ready, discarding");
		return;
	}

	LOG_INF("New DM result ready for target node %04x", dm_srv->target_addr);

	new_entry_store(result);
	if (k_work_delayable_is_pending(&dm_srv->timeout)) {
		k_work_cancel_delayable(&dm_srv->timeout);
		if (dm_srv->current_role == DM_ROLE_REFLECTOR) {
			result_status_send(dm_srv->model, &dm_srv->rsp_ctx, 1, 0);
		}
	}

	dm_srv->is_busy = false;
}

static struct dm_cb dm_cb = {
	.data_ready = data_ready,
};

static void timeout_work(struct k_work *work)
{
	LOG_ERR("DM attempt timed out");

	struct dm_result result = {
		.quality = DM_QUALITY_DO_NOT_USE,
		.status = false
	};

	new_entry_store(&result);
	if (dm_srv->current_role == DM_ROLE_REFLECTOR) {
		result_status_send(dm_srv->model, &dm_srv->rsp_ctx, 1, ETIMEDOUT);
	}

	dm_srv->is_busy = false;
}

static int bt_mesh_dm_srv_init(const struct bt_mesh_model *model)
{
	int err;
	struct bt_mesh_dm_srv *srv = model->user_data;

	srv->model = model;
	dm_srv = srv;
	k_sem_init(&srv->dm_ready_sem, 0, 1);

	struct dm_init_param init_param = { .cb = &dm_cb};

	err = dm_init(&init_param);
	if (err) {
		LOG_ERR("Failed to initialize DM: %d", err);
		return err;
	}

	k_work_init_delayable(&srv->timeout, timeout_work);

	return 0;
}

static void bt_mesh_dm_srv_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_dm_srv *srv = model->user_data;

	srv->cfg.ttl = CONFIG_BT_MESH_DM_SRV_DEFAULT_TTL;
	srv->cfg.timeout = CONFIG_BT_MESH_DM_SRV_DEFAULT_TIMEOUT,
	srv->cfg.delay = CONFIG_BT_MESH_DM_SRV_REFLECTOR_DELAY,

#if CONFIG_BT_SETTINGS
	(void)bt_mesh_model_data_store(srv->model, true, NULL, NULL, 0);
#endif
}

static int bt_mesh_dm_srv_settings_set(const struct bt_mesh_model *model, const char *name,
						 size_t len_rd, settings_read_cb read_cb,
						 void *cb_arg)
{
	struct bt_mesh_dm_srv *srv = model->user_data;
	struct settings_data data;
	ssize_t len;

	len = read_cb(cb_arg, &data, sizeof(data));
	if (len < sizeof(data)) {
		return -EINVAL;
	}

	srv->cfg = data.cfg;
	return 0;
}

const struct bt_mesh_model_cb _bt_mesh_dm_srv_cb = {
	.init = bt_mesh_dm_srv_init,
	.reset = bt_mesh_dm_srv_reset,
	.settings_set = bt_mesh_dm_srv_settings_set,
#if CONFIG_BT_SETTINGS
	.pending_store = bt_mesh_dm_srv_pending_store,
#endif
};
