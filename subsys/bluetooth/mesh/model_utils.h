/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**
 * @file
 * @defgroup bt_mesh_model_utils Model utility functions.
 * @{
 * @brief Internal utility functions for mesh model implementations.
 */
#ifndef MODEL_UTILS_H__
#define MODEL_UTILS_H__

#include <string.h>
#include <bluetooth/mesh/model_types.h>

/** @brief Send a model message.
 *
 * Sends a model message with the given context. If the context is NULL, this
 * updates the publish message, and publishes with the configured parameters.
 *
 * @param mod Model to send on.
 * @param ctx Context to send with, or NULL to publish on the configured
 * publish parameters.
 * @param buf Message to send.
 *
 * @retval 0 The message was sent successfully.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int model_send(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
	       struct net_buf_simple *buf);

/** @brief Send an acknowledged model message.
 *
 * If a message context is not provided, the message is published using the
 * model's configured publish parameters, if supported. If a response context
 * is provided, the call is blocking until the response context is either
 * released or the request times out.
 *
 * If a response context is provided, the call blocks for
 * 200 + TTL * 50 milliseconds, or until the acknowledgment is received.
 *
 * @param mod Model to send the message on.
 * @param ctx Message context, or NULL to send with the configured publish
 * parameters.
 * @param buf Message to send.
 * @param ack Message response context, or NULL if no response is expected.
 * @param rsp_op Expected response opcode.
 * @param user_data User defined parameter.
 *
 * @retval 0 The message was sent successfully.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int model_ackd_send(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf,
		    struct bt_mesh_model_ack_ctx *ack, u32_t rsp_op,
		    void *user_data);

static inline void model_ack_init(struct bt_mesh_model_ack_ctx *ack)
{
	k_sem_init(&ack->sem, 0, 1);
}

static inline int model_ack_ctx_prepare(struct bt_mesh_model_ack_ctx *ack,
					u32_t op, u16_t dst, void *user_data)
{
	if (ack->op != 0) {
		return -EALREADY;
	}
	ack->op = op;
	ack->user_data = user_data;
	ack->dst = dst;
	return 0;
}

static inline bool model_ack_match(const struct bt_mesh_model_ack_ctx *ack_ctx,
				   u32_t op,
				   const struct bt_mesh_msg_ctx *msg_ctx)
{
	return (ack_ctx->op == op &&
		(ack_ctx->dst == msg_ctx->addr || ack_ctx->dst == 0));
}

static inline int model_ack_wait(struct bt_mesh_model_ack_ctx *ack,
				 s32_t timeout)
{
	int status = k_sem_take(&ack->sem, K_MSEC(timeout));

	ack->op = 0;
	return status;
}

static inline bool model_ack_busy(struct bt_mesh_model_ack_ctx *ack)
{
	return (ack->op != 0);
}

static inline void model_ack_rx(struct bt_mesh_model_ack_ctx *ack)
{
	k_sem_give(&ack->sem);
}

static inline void model_ack_clear(struct bt_mesh_model_ack_ctx *ack)
{
	ack->op = 0;
}

/** @brief Compare the TID of an incoming message with the previous
 * transaction, and update it if it's new.
 *
 * @param prev_transaction Previous transaction.
 * @param tid Transaction ID of the incoming message.
 * @param ctx Message context of the incoming message.
 *
 * @retval 0 The TID is new, and the @p prev_transaction structure has been
 * updated.
 * @retval -EAGAIN The incoming message is of the same transaction.
 */
int tid_check_and_update(struct bt_mesh_tid_ctx *prev_transaction, u8_t tid,
			 const struct bt_mesh_msg_ctx *ctx);

u8_t model_delay_encode(u32_t delay);
s32_t model_delay_decode(u8_t encoded_delay);
s32_t model_transition_decode(u8_t encoded_transition);
u8_t model_transition_encode(s32_t transition_time);

static inline void
model_transition_buf_add(struct net_buf_simple *buf,
			 const struct bt_mesh_model_transition *transition)
{
	net_buf_simple_add_u8(buf, model_transition_encode(transition->time));
	net_buf_simple_add_u8(buf, model_delay_encode(transition->delay));
}

static inline void
model_transition_buf_pull(struct net_buf_simple *buf,
			  struct bt_mesh_model_transition *transition)
{
	transition->time = model_transition_decode(net_buf_simple_pull_u8(buf));
	transition->delay = model_delay_decode(net_buf_simple_pull_u8(buf));
}

static inline bool
model_transition_is_active(const struct bt_mesh_model_transition *transition)
{
	return (transition->time > 0 || transition->delay > 0);
}

#endif /* MODEL_UTILS_H__ */

/** @} */
