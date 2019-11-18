/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <bluetooth/mesh/models.h>
#include "model_utils.h"
#include "mesh/mesh.h"

/** Unknown encoded transition time value */
#define TRANSITION_TIME_UNKNOWN (0x3F)

/** Delay field step factor in milliseconds */
#define DELAY_TIME_STEP_MS (5)
/** Maximum encoded value of the delay field */
#define DELAY_TIME_STEP_MAX (0xFF)
/** Maximum permisible delay time in milliseconds */
#define DELAY_TIME_MAX_MS (DELAY_TIME_STEP_MAX * DELAY_TIME_STEP_FACTOR_MS)

#define MOD_ACKD_TIMEOUT_BASE 200
#define MOD_ACKD_TIMEOUT_PER_HOP 50

int tid_check_and_update(struct bt_mesh_tid_ctx *prev_transaction, u8_t tid,
			 const struct bt_mesh_msg_ctx *ctx)
{
	/* This updates the timestamp in the progress: */
	s64_t uptime_delta = k_uptime_delta(&prev_transaction->timestamp);

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

static const u32_t model_transition_res[] = {
	K_MSEC(100),
	K_SECONDS(1),
	K_SECONDS(10),
	K_MINUTES(10),
};

s32_t model_transition_decode(u8_t encoded_transition)
{
	u8_t steps = (encoded_transition & 0x3f);
	u8_t res = (encoded_transition >> 6);

	return (steps == TRANSITION_TIME_UNKNOWN) ?
		       K_FOREVER :
		       (model_transition_res[res] * steps);
}

u8_t model_transition_encode(s32_t transition_time)
{
	if (transition_time == 0) {
		return 0;
	}
	if (transition_time == K_FOREVER) {
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
	const s32_t limits[] = {
		K_MSEC(6600),
		K_SECONDS(66),
		K_SECONDS(910),
		K_MINUTES(620),
	};

	for (u8_t i = 0; i < ARRAY_SIZE(model_transition_res); ++i) {
		if (transition_time > limits[i]) {
			continue;
		}

		u8_t steps = ((transition_time + model_transition_res[i] / 2) /
			      model_transition_res[i]);
		steps = MAX(1, steps);
		steps = MIN(0x3e, steps);
		return (i << 6) | steps;
	}

	return TRANSITION_TIME_UNKNOWN;
}

u8_t model_delay_encode(u32_t delay)
{
	return delay / DELAY_TIME_STEP_MS;
}

s32_t model_delay_decode(u8_t encoded_delay)
{
	return encoded_delay * 5;
}

int model_send(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
	       struct net_buf_simple *buf)
{
	if (!ctx && !mod->pub) {
		return -ENOTSUP;
	}

	if (ctx) {
		return bt_mesh_model_send(mod, ctx, buf, NULL, 0);
	}

	net_buf_simple_reset(mod->pub->msg);
	memcpy(net_buf_simple_add(mod->pub->msg, buf->len), buf->data,
	       buf->len);
	return bt_mesh_model_publish(mod);
}

int model_ackd_send(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf,
		    struct bt_mesh_model_ack_ctx *ack, u32_t rsp_op,
		    void *user_data)
{
	if (ack &&
	    model_ack_ctx_prepare(ack, rsp_op, ctx ? ctx->addr : mod->pub->addr,
				  user_data) != 0) {
		return -EALREADY;
	}

	int retval = model_send(mod, ctx, buf);

	if (ack) {
		if (retval == 0) {
			u8_t ttl = (ctx ? ctx->send_ttl : mod->pub->ttl);
			s32_t time = (MOD_ACKD_TIMEOUT_BASE +
				      ttl * MOD_ACKD_TIMEOUT_PER_HOP);
			return model_ack_wait(ack, time);
		}

		model_ack_clear(ack);
	}
	return retval;
}

bool bt_mesh_model_pub_is_unicast(const struct bt_mesh_model *mod)
{
	return mod->pub && BT_MESH_ADDR_IS_UNICAST(mod->pub->addr);
}
