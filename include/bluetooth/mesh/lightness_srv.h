/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_lightness_srv Light Lightness Server model
 * @{
 * @brief API for the Light Lightness Server.
 */

#ifndef BT_MESH_LIGHTNESS_SRV_H__
#define BT_MESH_LIGHTNESS_SRV_H__

#include <zephyr/bluetooth/mesh.h>

#include <bluetooth/mesh/lightness.h>
#include <bluetooth/mesh/gen_lvl_srv.h>
#include <bluetooth/mesh/gen_ponoff_srv.h>
#include <bluetooth/mesh/model_types.h>
#if IS_ENABLED(CONFIG_EMDS) && IS_ENABLED(CONFIG_BT_SETTINGS)
#include "emds/emds.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_lightness_srv;

/** @def BT_MESH_LIGHTNESS_SRV_INIT
 *
 * @brief Initialization parameters for @ref bt_mesh_lightness_srv.
 *
 * @param[in] _handlers Handler callback structure.
 */
#define BT_MESH_LIGHTNESS_SRV_INIT(_handlers)                                  \
	{                                                                      \
		.lvl = BT_MESH_LVL_SRV_INIT(                                   \
			&_bt_mesh_lightness_srv_lvl_handlers),                 \
		.ponoff = BT_MESH_PONOFF_SRV_INIT(                             \
			&_bt_mesh_lightness_srv_onoff_handlers, NULL, NULL),   \
		.handlers = _handlers,                                         \
	}

/** @def BT_MESH_MODEL_LIGHTNESS_SRV
 *
 * @brief Light Lightness model entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_lightness_srv instance.
 */
#define BT_MESH_MODEL_LIGHTNESS_SRV(_srv)                                      \
	BT_MESH_MODEL_LVL_SRV(&(_srv)->lvl),                                   \
	BT_MESH_MODEL_PONOFF_SRV(&(_srv)->ponoff),                             \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SRV,                 \
			 _bt_mesh_lightness_srv_op, &(_srv)->pub,              \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_lightness_srv, \
						 _srv),                        \
			 &_bt_mesh_lightness_srv_cb),                          \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SETUP_SRV,           \
			 _bt_mesh_lightness_setup_srv_op, NULL,                \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_lightness_srv, \
						 _srv),                        \
			 &_bt_mesh_lightness_setup_srv_cb)

/** Collection of handler callbacks for the Light Lightness Server. */
struct bt_mesh_lightness_srv_handlers {
	/** @brief Set the Light state.
	 *
	 * When a set message is received, the model publishes a status message, with the response
	 * set to @c rsp. When an acknowledged set message is received, the model also sends a
	 * response back to a client. If a state change is non-instantaneous, for example when
	 * @ref bt_mesh_model_transition_time returns a nonzero value, the application is
	 * responsible for publishing a value of the Light state at the end of the transition.
	 *
	 * When performing a non-instantaneous state change, the Light state at any intermediate
	 * point must be clamped to the current lightness range of the server using
	 * @ref bt_mesh_lightness_clamp.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Server to set the Light state of.
	 * @param[in] ctx Message context, or NULL if the state change was not
	 * a result of a message.
	 * @param[in] set Parameters of the state change.
	 * @param[out] rsp Response structure to be filled.
	 */
	void (*const light_set)(struct bt_mesh_lightness_srv *srv,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_lightness_set *set,
				struct bt_mesh_lightness_status *rsp);

	/** @brief Get the current Light state.
	 *
	 * When in the middle of a non-instantaneous state change, the Light value filled in
	 * @c rsp must be clamped to the current lightness range of the server using
	 * @ref bt_mesh_lightness_clamp.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Server to get the Light state of.
	 * @param[in] ctx Context of the get message that triggered the query,
	 * or NULL if it was not triggered by a message.
	 * @param[out] rsp Response structure to be filled.
	 */
	void (*const light_get)(struct bt_mesh_lightness_srv *srv,
				struct bt_mesh_msg_ctx *ctx,
				struct bt_mesh_lightness_status *rsp);

	/** @brief The Default Light state has changed.
	 *
	 * The user may implement this handler to subscribe to changes to the
	 * Default Light state.
	 *
	 * @param[in] srv Server the Default Light state was changed on.
	 * @param[in] ctx Context of the set message that triggered the update,
	 * or NULL if it was not triggered by a message.
	 * @param[in] old_default The Default Light before the change.
	 * @param[in] new_default The Default Light after the change.
	 */
	void (*const default_update)(struct bt_mesh_lightness_srv *srv,
				     struct bt_mesh_msg_ctx *ctx,
				     uint16_t old_default, uint16_t new_default);

	/** @brief The Light Range state has changed.
	 *
	 * The user may implement this handler to subscribe to change to the
	 * Light Range state. If the change in range causes the current Light
	 * state to be out of range, the Light state should be changed to the
	 * nearest value inside the range. It's recommended to call
	 * @ref bt_mesh_lightness_srv_pub to notify the mesh if the Light state
	 * changes.
	 *
	 * @param[in] srv Server the Light Range state was changed on.
	 * @param[in] ctx Context of the set message that triggered the update,
	 * or NULL if it was not triggered by a message.
	 * @param[in] old_range The Light Range before the change.
	 * @param[in] new_range The Light Range after the change.
	 */
	void (*const range_update)(
		struct bt_mesh_lightness_srv *srv, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_lightness_range *old_range,
		const struct bt_mesh_lightness_range *new_range);
};

/**
 * Light Lightness Server instance.
 *
 * Should be initialized with @ref BT_MESH_LIGHTNESS_SRV_INIT.
 */
struct bt_mesh_lightness_srv {
	/** Light Level Server instance. */
	struct bt_mesh_lvl_srv lvl;
	/** Light Light OnOff server instance. */
	struct bt_mesh_ponoff_srv ponoff;
	/** Internal flag state. */
	atomic_t flags;
	/** Pointer to the model entry in the composition data. */
	const struct bt_mesh_model *lightness_model;
	/** Pointer to the Setup Server model entry in the composition data. */
	const struct bt_mesh_model *lightness_setup_model;
	/** Model publication parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_LIGHTNESS_OP_STATUS,
		BT_MESH_LIGHTNESS_MSG_MAXLEN_STATUS)];
	/** Transaction ID tracker for the set messages. */
	struct bt_mesh_tid_ctx tid;
	/** User handler functions. */
	const struct bt_mesh_lightness_srv_handlers *const handlers;

	/** Current Light Level Range. */
	struct bt_mesh_lightness_range range;
	/** Current Default Light Level. */
	uint16_t default_light;
	/** The delta start Light Level */
	uint16_t delta_start;
	struct __packed {
		/** The last known Light Level. */
		uint16_t last;
		/** Whether the Light is on. */
		bool is_on;
	} transient;

#if IS_ENABLED(CONFIG_EMDS) && IS_ENABLED(CONFIG_BT_SETTINGS)
	/** Dynamic entry to be stored with EMDS */
	struct emds_dynamic_entry emds_entry;
#endif

#if defined(CONFIG_BT_MESH_LIGHT_CTRL_SRV)
	/** Acting controller, if enabled. */
	struct bt_mesh_light_ctrl_srv *ctrl;
#endif
};

/** @brief Clamp lightness to lightness range.
 *
 *  @return @c lightness clamped to the lightness range set on the
 *  @ref bt_mesh_lightness_srv pointed to by @c srv if @c lightness > 0, zero
 *  otherwise.
 */
static inline uint16_t bt_mesh_lightness_clamp(const struct bt_mesh_lightness_srv *srv,
					       uint16_t lightness)
{
	return lightness == 0 ? 0 : CLAMP(lightness, srv->range.min, srv->range.max);
}

/** @brief Publish the current Light state.
 *
 * Publishes a Light Lightness status message with the configured publish
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
 * @return 0 Successfully published the current Light state.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_lightness_srv_pub(struct bt_mesh_lightness_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_lightness_status *status);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_cb _bt_mesh_lightness_srv_cb;
extern const struct bt_mesh_model_cb _bt_mesh_lightness_setup_srv_cb;
extern const struct bt_mesh_model_op _bt_mesh_lightness_srv_op[];
extern const struct bt_mesh_model_op _bt_mesh_lightness_setup_srv_op[];
extern const struct bt_mesh_lvl_srv_handlers
	_bt_mesh_lightness_srv_lvl_handlers;
extern const struct bt_mesh_onoff_srv_handlers
	_bt_mesh_lightness_srv_onoff_handlers;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHTNESS_SRV_H__ */

/** @} */
