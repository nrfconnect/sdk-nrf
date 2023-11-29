/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_lvl_srv Bluetooth Mesh Generic Level Server model
 * @ingroup bt_mesh_lvl
 * @{
 * @brief API for the Generic Level Server model.
 */

#ifndef BT_MESH_GEN_LVL_SRV_H__
#define BT_MESH_GEN_LVL_SRV_H__

#include <bluetooth/mesh/gen_lvl.h>
#include <bluetooth/mesh/scene_srv.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_lvl_srv;

/** @def BT_MESH_LVL_SRV_INIT
 *
 * @brief Initialization parameters for a @ref bt_mesh_lvl_srv instance.
 *
 * @param[in] _handlers Pointer to a handler function structure.
 */
#define BT_MESH_LVL_SRV_INIT(_handlers)                                        \
	{                                                                      \
		.handlers = _handlers,                                         \
	}

/** @def BT_MESH_MODEL_LVL_SRV
 *
 * @brief Generic Level Server model composition data entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_lvl_srv instance.
 */
#define BT_MESH_MODEL_LVL_SRV(_srv)                                            \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_LEVEL_SRV, _bt_mesh_lvl_srv_op,  \
			 &(_srv)->pub,                                         \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_lvl_srv,       \
						 _srv),                        \
			 &_bt_mesh_lvl_srv_cb)

/** Handler functions for the Generic Level Server. */
struct bt_mesh_lvl_srv_handlers {
	/** @brief Get the current Level state.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Level server to get the state of.
	 * @param[in] ctx Message context for the message that triggered the
	 * request, or NULL if the request is not coming from a message.
	 * @param[out] rsp Response structure to be filled.
	 */
	void (*const get)(struct bt_mesh_lvl_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_lvl_status *rsp);

	/** @brief Set the Level state.
	 *
	 * When a set message is received, the model publishes a status message, with the response
	 * set to @c rsp. When an acknowledged set message is received, the model also sends a
	 * response back to a client. If a state change is non-instantaneous, for example when
	 * @ref bt_mesh_model_transition_time returns a nonzero value, the application is
	 * responsible for publishing a value of the Level state at the end of the transition.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Level server to set the state of.
	 * @param[in] ctx Message context for the message that triggered the
	 * change, or NULL if the change is not coming from a message.
	 * @param[in] set Parameters of the state change. Note that the
	 * transition will always be non-NULL.
	 * @param[out] rsp Response structure to be filled.
	 */
	void (*const set)(struct bt_mesh_lvl_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_lvl_set *set,
			  struct bt_mesh_lvl_status *rsp);

	/**
	 * @brief Change the Level state relative to its current value.
	 *
	 * If @c delta_set::new_transaction is false, the state transition
	 * should use the same start point as the previous delta_set message,
	 * effectively overriding the previous message. If it's true, the level
	 * transition should start from the current level, stopping any
	 * ongoing transitions.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Level server to change the state of.
	 * @param[in] ctx Message context for the message that triggered the
	 * change, or NULL if the change is not coming from a message.
	 * @param[in] delta_set Parameters of the state change. Note that the
	 * transition will always be non-NULL.
	 * @param[out] rsp Response structure to be filled.
	 */
	void (*const delta_set)(struct bt_mesh_lvl_srv *srv,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_lvl_delta_set *delta_set,
				struct bt_mesh_lvl_status *rsp);

	/** @brief Move the Level state continuously at a given rate.
	 *
	 * The Level state should move @c move_set::delta units for every
	 * @c move_set::transition::time milliseconds. For instance, if
	 * delta is 5 and the transition time is 100ms, the state should move
	 * at a rate of 50 per second.
	 *
	 * When reaching the border values for the state, the wraparound
	 * behavior is application specific. While the server is executing a
	 * move command, it should report its @c target value as
	 * @ref BT_MESH_LVL_MIN or @ref BT_MESH_LVL_MAX, depending on whether
	 * it's moving up or down.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Level server to change the state of.
	 * @param[in] ctx Message context for the message that triggered the
	 * change, or NULL if the change is not coming from a message.
	 * @param[in] move_set Parameters of the state change. Note that the
	 * transition will always be non-NULL.
	 * @param[out] rsp Response structure to be filled.
	 */
	void (*const move_set)(struct bt_mesh_lvl_srv *srv,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_lvl_move_set *move_set,
			       struct bt_mesh_lvl_status *rsp);
};

/**
 * Generic Level Server instance. Should be initialized with the
 * @ref BT_MESH_LVL_SRV_INIT macro.
 */
struct bt_mesh_lvl_srv {
	/** Application handler functions. */
	const struct bt_mesh_lvl_srv_handlers *const handlers;
	/** Pointer to the mesh model entry. */
	struct bt_mesh_model *model;
	/** Model publication parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(BT_MESH_LVL_OP_STATUS,
					       BT_MESH_LVL_MSG_MAXLEN_STATUS)];
	/** Transaction ID tracking. */
	struct bt_mesh_tid_ctx tid;
};

/** @brief Publish the current Level state.
 *
 * @param[in] srv Server whose Level state should be published.
 * @param[in] ctx Message context to publish with, or NULL to publish on the
 * configured publish parameters.
 * @param[in] status Current status.
 *
 * @retval 0 Successfully published a Generic Level Status message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_lvl_srv_pub(struct bt_mesh_lvl_srv *srv,
			struct bt_mesh_msg_ctx *ctx,
			const struct bt_mesh_lvl_status *status);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_lvl_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_lvl_srv_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_LVL_SRV_H__ */

/** @} */
