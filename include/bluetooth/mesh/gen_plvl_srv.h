/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_plvl_srv Generic Power Level Server model
 * @{
 * @brief API for the Generic Power Level Server.
 */

#ifndef BT_MESH_GEN_PLVL_SRV_H__
#define BT_MESH_GEN_PLVL_SRV_H__

#include <zephyr/bluetooth/mesh.h>

#include <bluetooth/mesh/gen_plvl.h>
#include <bluetooth/mesh/gen_lvl_srv.h>
#include <bluetooth/mesh/gen_ponoff_srv.h>
#include <bluetooth/mesh/model_types.h>
#if IS_ENABLED(CONFIG_EMDS) && IS_ENABLED(CONFIG_BT_SETTINGS)
#include "emds/emds.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_plvl_srv;

/** @def BT_MESH_PLVL_SRV_INIT
 *
 * @brief Initialization parameters for @ref bt_mesh_plvl_srv.
 *
 * @param[in] _handlers Handler callback structure.
 */
#define BT_MESH_PLVL_SRV_INIT(_handlers)                                       \
	{                                                                      \
		.lvl = BT_MESH_LVL_SRV_INIT(&bt_mesh_plvl_srv_lvl_handlers),   \
		.ponoff = BT_MESH_PONOFF_SRV_INIT(                             \
			&bt_mesh_plvl_srv_onoff_handlers, NULL, NULL),         \
		.handlers = _handlers,                                         \
	}

/** @def BT_MESH_MODEL_PLVL_SRV
 *
 * @brief Generic Power Level model entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_plvl_srv instance.
 */
#define BT_MESH_MODEL_PLVL_SRV(_srv)                                           \
	BT_MESH_MODEL_LVL_SRV(&(_srv)->lvl),                                   \
		BT_MESH_MODEL_PONOFF_SRV(&(_srv)->ponoff),                     \
		BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_SRV,         \
				 _bt_mesh_plvl_srv_op, &(_srv)->pub,           \
				 BT_MESH_MODEL_USER_DATA(                      \
					 struct bt_mesh_plvl_srv, _srv),       \
				 &_bt_mesh_plvl_srv_cb),                       \
		BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_SETUP_SRV,   \
				 _bt_mesh_plvl_setup_srv_op, NULL,             \
				 BT_MESH_MODEL_USER_DATA(                      \
					 struct bt_mesh_plvl_srv, _srv),       \
				 &_bt_mesh_plvl_setup_srv_cb)

/** Collection of handler callbacks for the Generic Power Level Server. */
struct bt_mesh_plvl_srv_handlers {
	/** @brief Set the Power state.
	 *
	 * When a set message is received, the model publishes a status message, with the response
	 * set to @c rsp. When an acknowledged set message is received, the model also sends a
	 * response back to a client. If a state change is non-instantaneous, for example when
	 * @ref bt_mesh_model_transition_time returns a nonzero value, the application is
	 * responsible for publishing a value of the Power state at the end of the transition.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Server to set the Power state of.
	 * @param[in] ctx Message context, or NULL if the state change was not
	 * a result of a message.
	 * @param[in] set Parameters of the state change.
	 * @param[out] rsp Response structure to be filled.
	 */
	void (*const power_set)(struct bt_mesh_plvl_srv *srv,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_plvl_set *set,
				struct bt_mesh_plvl_status *rsp);

	/** @brief Get the current Power state.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Server to get the Power state of.
	 * @param[in] ctx Context of the get message that triggered the query,
	 * or NULL if it was not triggered by a message.
	 * @param[out] rsp Response structure to be filled.
	 */
	void (*const power_get)(struct bt_mesh_plvl_srv *srv,
				struct bt_mesh_msg_ctx *ctx,
				struct bt_mesh_plvl_status *rsp);

	/** @brief The Default Power state has changed.
	 *
	 * The user may implement this handler to subscribe to changes to the
	 * Default Power state.
	 *
	 * @param[in] srv Server the Default Power state was changed on.
	 * @param[in] ctx Context of the set message that triggered the update,
	 * or NULL if it was not triggered by a message.
	 * @param[in] old_default The Default Power before the change.
	 * @param[in] new_default The Default Power after the change.
	 */
	void (*const default_update)(struct bt_mesh_plvl_srv *srv,
				     struct bt_mesh_msg_ctx *ctx,
				     uint16_t old_default, uint16_t new_default);

	/** @brief The Power Range state has changed.
	 *
	 * The user may implement this handler to subscribe to change to the
	 * Power Range state. If the change in range causes the current Power
	 * state to be out of range, the Power state should be changed to the
	 * nearest value inside the range. It is recommended to call
	 * @ref bt_mesh_plvl_srv_pub to notify the mesh if the Power state
	 * changes.
	 *
	 * @param[in] srv Server the Power Range state was changed on.
	 * @param[in] ctx Context of the set message that triggered the update,
	 * or NULL if it was not triggered by a message.
	 * @param[in] old_range The Power Range before the change.
	 * @param[in] new_range The Power Range after the change.
	 */
	void (*const range_update)(struct bt_mesh_plvl_srv *srv,
				   struct bt_mesh_msg_ctx *ctx,
				   const struct bt_mesh_plvl_range *old_range,
				   const struct bt_mesh_plvl_range *new_range);
};

/**
 * Generic Power Level Server instance.
 *
 * Should be initialized with @ref BT_MESH_PLVL_SRV_INIT.
 */
struct bt_mesh_plvl_srv {
	/** Generic Level Server instance. */
	struct bt_mesh_lvl_srv lvl;
	/** Generic Power OnOff server instance. */
	struct bt_mesh_ponoff_srv ponoff;
	/** Pointer to the model entry in the composition data. */
	const struct bt_mesh_model *plvl_model;
	/** Pointer to the model entry of the Setup Server. */
	const struct bt_mesh_model *plvl_setup_model;
	/** Model publication parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_PLVL_OP_LEVEL_STATUS,
		BT_MESH_PLVL_MSG_MAXLEN_LEVEL_STATUS)];
	/** Transaction ID tracker for the set messages. */
	struct bt_mesh_tid_ctx tid;
	/** User handler functions. */
	const struct bt_mesh_plvl_srv_handlers *const handlers;

	/** Current Power Range. */
	struct bt_mesh_plvl_range range;
	/** Current Default Power. */
	uint16_t default_power;
	struct __packed {
		/** The last known Power Level. */
		uint16_t last;
		/** Whether the Power is on. */
		bool is_on;
	} transient;

#if IS_ENABLED(CONFIG_EMDS) && IS_ENABLED(CONFIG_BT_SETTINGS)
	/** Dynamic entry to be stored with EMDS */
	struct emds_dynamic_entry emds_entry;
#endif
};

/** @brief Publish the current Power state.
 *
 * Publishes a Generic Power Level status message with the configured publish
 * parameters, or using the given message context.
 *
 * @note This API is only used for publishing unprompted status messages.
 * Response messages for get and set messages are handled internally.
 *
 * @param[in] srv Server instance to publish with.
 * @param[in] ctx Message context, or NULL to publish with the configured
 * parameters.
 * @param[in] status Status to publish.
 *
 * @return 0 Successfully published the current Power state.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_plvl_srv_pub(struct bt_mesh_plvl_srv *srv,
			 struct bt_mesh_msg_ctx *ctx,
			 const struct bt_mesh_plvl_status *status);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_cb _bt_mesh_plvl_srv_cb;
extern const struct bt_mesh_model_cb _bt_mesh_plvl_setup_srv_cb;
extern const struct bt_mesh_model_op _bt_mesh_plvl_srv_op[];
extern const struct bt_mesh_model_op _bt_mesh_plvl_setup_srv_op[];
extern const struct bt_mesh_lvl_srv_handlers bt_mesh_plvl_srv_lvl_handlers;
extern const struct bt_mesh_onoff_srv_handlers bt_mesh_plvl_srv_onoff_handlers;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_PLVL_SRV_H__ */

/** @} */
