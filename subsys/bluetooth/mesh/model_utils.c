/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <bluetooth/mesh/models.h>
#include "model_utils.h"
#include "mesh/mesh.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)
#define LOG_MODULE_NAME bt_mesh_mod
#include "common/log.h"

/** Unknown encoded transition time value */
#define TRANSITION_TIME_UNKNOWN (0x3F)

/** Delay field step factor in milliseconds */
#define DELAY_TIME_STEP_MS (5)

int tid_check_and_update(struct bt_mesh_tid_ctx *prev_transaction, uint8_t tid,
			 const struct bt_mesh_msg_ctx *ctx)
{
	/* This updates the timestamp in the progress: */
	int64_t uptime_delta = k_uptime_delta(&prev_transaction->timestamp);

	if (prev_transaction->src == ctx->addr &&
	    prev_transaction->dst == ctx->recv_dst &&
	    prev_transaction->tid == tid && uptime_delta < 6000) {
		return -EALREADY;
	}

	prev_transaction->src = ctx->addr;
	prev_transaction->dst = ctx->recv_dst;
	prev_transaction->tid = tid;

	return 0;
}

static const uint32_t model_transition_res[] = {
	100,
	1 * MSEC_PER_SEC,
	10 * MSEC_PER_SEC,
	60 * 10 * MSEC_PER_SEC,
};

int32_t model_transition_decode(uint8_t encoded_transition)
{
	uint8_t steps = (encoded_transition & 0x3f);
	uint8_t res = (encoded_transition >> 6);

	return (steps == TRANSITION_TIME_UNKNOWN) ?
		       SYS_FOREVER_MS :
		       (model_transition_res[res] * steps);
}

uint8_t model_transition_encode(int32_t transition_time)
{
	if (transition_time == 0) {
		return 0;
	}
	if (transition_time == SYS_FOREVER_MS) {
		return TRANSITION_TIME_UNKNOWN;
	}

	/* Even though we're only able to encode values up to 6200 ms in
	 * 100 ms step resolution, values between 6200 and 6600 ms are closer
	 * to 6200 ms than 7000 ms, which would be the nearest 1-second step
	 * representation. Similar cases exist for the other step resolutions
	 * too, so we'll use these custom limits instead of checking against
	 * the max value of each step resolution. The formula for the limit is
	 * ((0x3e * res[i]) + (((0x3e * res[i]) / res[i+1]) + 1) * res[i+1])/2.
	 */
	const int32_t limits[] = {
		6600,
		66 * MSEC_PER_SEC,
		910 * MSEC_PER_SEC,
		60 * 620 * MSEC_PER_SEC,
	};

	for (uint8_t i = 0; i < ARRAY_SIZE(model_transition_res); ++i) {
		if (transition_time > limits[i]) {
			continue;
		}

		uint8_t steps = ((transition_time + model_transition_res[i] / 2) /
			      model_transition_res[i]);
		steps = MAX(1, steps);
		steps = MIN(0x3e, steps);
		return (i << 6) | steps;
	}

	return TRANSITION_TIME_UNKNOWN;
}

uint8_t model_delay_encode(uint32_t delay)
{
	return delay / DELAY_TIME_STEP_MS;
}

int32_t model_delay_decode(uint8_t encoded_delay)
{
	return encoded_delay * DELAY_TIME_STEP_MS;
}

int model_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
	       struct net_buf_simple *buf)
{
	if (!ctx && !model->pub) {
		return -ENOTSUP;
	}

	if (ctx) {
		return bt_mesh_model_send(model, ctx, buf, NULL, 0);
	}

	net_buf_simple_reset(model->pub->msg);
	net_buf_simple_add_mem(model->pub->msg, buf->data, buf->len);

	return bt_mesh_model_publish(model);
}

int model_ackd_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf,
		    struct bt_mesh_model_ack_ctx *ack, uint32_t rsp_op,
		    void *user_data)
{
	if (ack &&
	    model_ack_ctx_prepare(ack, rsp_op, ctx ? ctx->addr : model->pub->addr,
				  user_data) != 0) {
		return -EALREADY;
	}

	int retval = model_send(model, ctx, buf);

	if (ack) {
		if (retval == 0) {
			uint8_t ttl = (ctx ? ctx->send_ttl : model->pub->ttl);
			int32_t time = (CONFIG_BT_MESH_MOD_ACKD_TIMEOUT_BASE +
				ttl * CONFIG_BT_MESH_MOD_ACKD_TIMEOUT_PER_HOP);
			return model_ack_wait(ack, time);
		}

		model_ack_clear(ack);
	}
	return retval;
}

bool bt_mesh_model_pub_is_unicast(const struct bt_mesh_model *model)
{
	return model->pub && BT_MESH_ADDR_IS_UNICAST(model->pub->addr);
}
