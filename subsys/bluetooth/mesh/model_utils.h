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
#include "mesh/msg.h"

#if IS_ENABLED(CONFIG_EMDS)
/**
 * @brief Create emds model id to identify the model, where @p mod is the model.
 */
#define EMDS_MODEL_ID(mod) (((uint16_t)mod->rt->elem_idx << 8) | mod->rt->mod_idx)
#endif

/**
 * @brief Returns rounded division of @p A divided by @p B.
 * @note Arguments are evaluated twice.
 */
#define ROUNDED_DIV(A, B) (((A) + ((B) / 2)) / (B))

/** @brief Compare the TID of an incoming message with the previous
 * transaction, and update it, if it is new.
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
model_transition_get(const struct bt_mesh_model *model,
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

int32_t model_ackd_timeout_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx);

#endif /* MODEL_UTILS_H__ */

/** @} */
