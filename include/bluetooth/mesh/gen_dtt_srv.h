/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @defgroup bt_mesh_dtt_srv Generic Default Transition Time Server
 * model
 * @{
 * @brief API for the Generic Default Transition Time (DTT) Server model.
 */

#ifndef BT_MESH_GEN_DTT_SRV_H__
#define BT_MESH_GEN_DTT_SRV_H__

#include <bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>
#include <bluetooth/mesh/gen_dtt.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_dtt_srv;

/** @def BT_MESH_DTT_SRV_INIT
 *
 * @brief Initialization parameters for @ref bt_mesh_dtt_srv.
 *
 * @param[in] _update Update handler called on every change to the transition
 * time in this server.
 */
#define BT_MESH_DTT_SRV_INIT(_update)                                          \
	{                                                                      \
		.update = _update,                                             \
		.pub = {.update = _bt_mesh_dtt_srv_update_handler,             \
			.msg = NET_BUF_SIMPLE(BT_MESH_MODEL_BUF_LEN(           \
				BT_MESH_DTT_OP_STATUS,                         \
				BT_MESH_DTT_MSG_LEN_STATUS)) }                 \
	}

/** @def BT_MESH_MODEL_DTT_SRV
 *
 * @brief Generic Default Transition Time Server model composition data entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_dtt_srv instance.
 */
#define BT_MESH_MODEL_DTT_SRV(_srv)                                            \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_SRV,              \
			 _bt_mesh_dtt_srv_op, &(_srv)->pub,                    \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_dtt_srv,       \
						 _srv),                        \
			 &_bt_mesh_dtt_srv_cb)

/** Generic Default Transition Time server instance. */
struct bt_mesh_dtt_srv {
	/** Current transition time in milliseconds */
	u32_t transition_time;

	/** @brief Update handler for the transition time state.
	 *
	 * Called every time the transition time is updated.
	 *
	 * @param[in] srv Server instance that was updated.
	 * @param[in] ctx Context of the set message that caused the update, or
	 * NULL if the update was not a result of a set message.
	 * @param[in] old_transition_time The transition time prior to the
	 * update.
	 * @param[in] new_transition_time The new transition time after the
	 * update.
	 */
	void (*const update)(struct bt_mesh_dtt_srv *srv,
			     struct bt_mesh_msg_ctx *ctx,
			     u32_t old_transition_time,
			     u32_t new_transition_time);
	/** Composition data Model entry pointer. */
	struct bt_mesh_model *model;
	/** Model publish parameters. */
	struct bt_mesh_model_pub pub;
};

/** @brief Set the Default Transition Time of the DTT server.
 *
 * If an update handler is set, it'll be called with the updated transition
 * time. If publication is configured, the change will cause the server to
 * publish.
 *
 * @param[in] srv Server to set the DTT of.
 * @param[in] transition_time New Default Transition Time.
 */
void bt_mesh_dtt_srv_set(struct bt_mesh_dtt_srv *srv, u32_t transition_time);

/** @brief Publish the current transition time.
 *
 * @param[in] srv Server instance to publish with.
 * @param[in] ctx Message context to publish with, or NULL to publish with the
 * configured parameters.
 *
 * @retval 0 Successfully published the current transition time.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_dtt_srv_pub(struct bt_mesh_dtt_srv *srv,
			struct bt_mesh_msg_ctx *ctx);

/** @brief Find the Generic DTT server in a given element.
 *
 * @param[in] elem Element to find the DTT server in.
 *
 * @return A pointer to the DTT server instance, or NULL if no instance is
 * found.
 */
static inline struct bt_mesh_dtt_srv *
bt_mesh_dtt_srv_get(const struct bt_mesh_elem *elem)
{
	struct bt_mesh_model *mod = bt_mesh_model_find(
		elem, BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_SRV);

	return (struct bt_mesh_dtt_srv *)(mod ? mod->user_data : NULL);
}

/** @brief Get the default transition parameters for the given model.
 *
 * @param[in] mod Model to get the DTT for.
 * @param[out] transition Transition buffer.
 *
 * @return Whether the transition was set.
 */
static inline bool
bt_mesh_dtt_srv_transition_get(struct bt_mesh_model *mod,
			       struct bt_mesh_model_transition *transition)
{
	struct bt_mesh_dtt_srv *srv =
		bt_mesh_dtt_srv_get(bt_mesh_model_elem(mod));

	transition->time = srv ? srv->transition_time : 0;
	transition->delay = 0;
	return (transition->time != 0);
}

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_dtt_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_dtt_srv_cb;
int _bt_mesh_dtt_srv_update_handler(struct bt_mesh_model *model);
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_DTT_SRV_H__ */

/** @} */
