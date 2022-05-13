/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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
#include <bluetooth/mesh/gen_dtt_srv.h>

#if IS_ENABLED(CONFIG_EMDS)
/**
 * @brief Create emds model id to identify the model, where @p mod is the model.
 */
#define EMDS_MODEL_ID(mod) (((uint16_t)mod->elem_idx << 8) | mod->mod_idx)
#endif

/**
 * @brief Returns rounded division of @p A divided by @p B.
 * @note Arguments are evaluated twice.
 */
#define ROUNDED_DIV(A, B) (((A) + ((B) / 2)) / (B))

/** @brief Send a model message.
 *
 * Sends a model message with the given context. If the context is NULL, this
 * updates the publish message, and publishes with the configured parameters.
 *
 * @param model Model to send on.
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
int model_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
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
 * @param model Model to send the message on.
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
int model_ackd_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		    struct net_buf_simple *buf,
		    struct bt_mesh_msg_ack_ctx *ack, uint32_t rsp_op,
		    void *user_data);

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
int tid_check_and_update(struct bt_mesh_tid_ctx *prev_transaction, uint8_t tid,
			 const struct bt_mesh_msg_ctx *ctx);

uint8_t model_delay_encode(uint32_t delay);
int32_t model_delay_decode(uint8_t encoded_delay);
int32_t model_transition_decode(uint8_t encoded_transition);
uint8_t model_transition_encode(int32_t transition_time);

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

static inline struct bt_mesh_model_transition *
model_transition_get(struct bt_mesh_model *model,
		     struct bt_mesh_model_transition *transition,
		     struct net_buf_simple *buf)
{
	if (buf->len == 2) {
		model_transition_buf_pull(buf, transition);
		return transition;
	}

	if (bt_mesh_dtt_srv_transition_get(model, transition)) {
		return transition;
	}

	return NULL;
}

static inline bool
model_transition_is_invalid(const struct bt_mesh_model_transition *transition)
{
	return (transition != NULL &&
		(transition->time > BT_MESH_MODEL_TRANSITION_TIME_MAX_MS ||
		 transition->delay > BT_MESH_MODEL_DELAY_TIME_MAX_MS));
}

#endif /* MODEL_UTILS_H__ */

/** @} */
