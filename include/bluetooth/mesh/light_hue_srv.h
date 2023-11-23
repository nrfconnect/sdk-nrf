/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_light_hue_srv Light Hue Server model
 * @{
 * @brief API for the Light Hue Server model.
 */

#ifndef BT_MESH_LIGHT_HUE_SRV_H__
#define BT_MESH_LIGHT_HUE_SRV_H__

#include <bluetooth/mesh/light_hsl.h>
#include <bluetooth/mesh/model_types.h>
#include <bluetooth/mesh/gen_lvl_srv.h>
#if IS_ENABLED(CONFIG_EMDS) && IS_ENABLED(CONFIG_BT_SETTINGS)
#include "emds/emds.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_light_hue_srv;

/** @def BT_MESH_LIGHT_HUE_SRV_INIT
 *
 * @brief Initialization parameters for a @ref bt_mesh_light_hue_srv
 * instance.
 *
 * @param[in] _handlers HSL Hue Server callbacks.
 */
#define BT_MESH_LIGHT_HUE_SRV_INIT(_handlers)                                  \
	{                                                                      \
		.handlers = _handlers,                                         \
		.lvl = BT_MESH_LVL_SRV_INIT(                                   \
			&_bt_mesh_light_hue_srv_lvl_handlers),                 \
		.range = BT_MESH_LIGHT_HSL_OP_RANGE_DEFAULT,                   \
	}

/** @def BT_MESH_MODEL_LIGHT_HUE_SRV
 *
 * @brief HSL Hue Server model composition data entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_light_hue_srv instance.
 */
#define BT_MESH_MODEL_LIGHT_HUE_SRV(_srv)                                      \
	BT_MESH_MODEL_LVL_SRV(&(_srv)->lvl),                                   \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_HSL_HUE_SRV,                   \
			 _bt_mesh_light_hue_srv_op, &(_srv)->pub,              \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_light_hue_srv, \
						 _srv),                        \
			 &_bt_mesh_light_hue_srv_cb)

/** HSL Hue Server state access handlers. */
struct bt_mesh_light_hue_srv_handlers {
	/** @brief Set the Hue state.
	 *
	 * When a set message is received, the model publishes a status message, with the response
	 * set to @c rsp. When an acknowledged set message is received, the model also sends a
	 * response back to a client. If a state change is non-instantaneous, for example when
	 * @ref bt_mesh_model_transition_time returns a nonzero value, the application is
	 * responsible for publishing a value of the Hue state at the end of the transition.
	 *
	 * When a state change is not-instantaneous, the @ref bt_mesh_light_hue::direction field
	 * represents a direction in which the value of the Hue state should change. When the target
	 * value of the Hue state is less than the current value and the direction field is
	 * positive, or when the target value is greater than the current value and the direction
	 * field is negative, the application is responsible for wrapping the value of the Hue
	 * state around when the value reaches the end of the type range.
	 *
	 *  @note This handler is mandatory.
	 *
	 *  @param[in] srv Server to set the Hue state of.
	 *  @param[in] ctx Message context.
	 *  @param[in] set Parameters of the state change.
	 *  @param[out] rsp Response structure to be filled.
	 */
	void (*const set)(struct bt_mesh_light_hue_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_light_hue *set,
			  struct bt_mesh_light_hue_status *rsp);

	/** @brief Move the hue continuously at a certain rate.
	 *
	 *  The hue state should move @c move_set::delta units for every
	 *  @c move_set::transition::time milliseconds. For instance, if
	 *  delta is 5 and the transition time is 100ms, the state should move
	 *  at a rate of 50 per second.
	 *
	 *  When reaching the border values for the state, the value should wrap
	 *  around. While the server is executing a move command, it should
	 *  report its @c target value as @ref BT_MESH_LIGHT_HSL_MIN or
	 *  @ref BT_MESH_LIGHT_HSL_MAX, depending on whether it's moving up or
	 *  down.
	 *
	 *  @note This handler is mandatory.
	 *
	 *  @param[in]  srv  Server to set the Hue state of.
	 *  @param[in]  ctx  Message context.
	 *  @param[in]  move Parameters of the state change.
	 *  @param[out] rsp  Response structure to be filled.
	 */
	void (*const move_set)(struct bt_mesh_light_hue_srv *srv,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_light_hue_move *move,
			       struct bt_mesh_light_hue_status *rsp);

	/** @brief Get the Hue state.
	 *
	 *  @note This handler is mandatory.
	 *
	 *  @param[in] srv Server to get the CTL state of.
	 *  @param[in] ctx Message context.
	 *  @param[out] rsp Response structure to be filled.
	 */
	void (*const get)(struct bt_mesh_light_hue_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_light_hue_status *rsp);

	/** @brief The default hue has been updated.
	 *
	 *  @param[in] srv         Light Hue server.
	 *  @param[in] ctx         Message context, or NULL if the callback was
	 *                         not triggered by a mesh message
	 *  @param[in] old_default Default value before the update.
	 *  @param[in] new_default Default value after the update.
	 */
	void (*const default_update)(struct bt_mesh_light_hue_srv *srv,
				     struct bt_mesh_msg_ctx *ctx,
				     uint16_t old_default,
				     uint16_t new_default);

	/** @brief The valid hue range has been updated.
	 *
	 *  @param[in] srv       Light Hue server.
	 *  @param[in] ctx       Message context, or NULL if the callback was
	 *                       not triggered by a mesh message
	 *  @param[in] old_range Range before the update.
	 *  @param[in] new_range Range after the update.
	 */
	void (*const range_update)(
		struct bt_mesh_light_hue_srv *srv,
		struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_light_hsl_range *old_range,
		const struct bt_mesh_light_hsl_range *new_range);
};

/**
 * HSL Hue Server instance. Should be initialized with
 * @ref BT_MESH_LIGHT_HUE_SRV_INIT.
 */
struct bt_mesh_light_hue_srv {
	/** Light Level Server instance. */
	struct bt_mesh_lvl_srv lvl;
	/** Handler function structure. */
	const struct bt_mesh_light_hue_srv_handlers *handlers;

	/** Model entry. */
	const struct bt_mesh_model *model;
	/** Pointer to the corresponding HSL server, if it has one.
	 *  Is set automatically by the HSL server.
	 */
	const struct bt_mesh_light_hsl_srv *hsl;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/** Publication buffer */
	struct net_buf_simple buf;
	/** Publication buffer data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_LIGHT_HUE_OP_STATUS,
		BT_MESH_LIGHT_HSL_MSG_MAXLEN_HUE_STATUS)];
	/** Transaction ID tracker for the set messages. */
	struct bt_mesh_tid_ctx prev_transaction;

	/** Hue range */
	struct bt_mesh_light_hsl_range range;
	/** Default Hue level */
	uint16_t dflt;
	struct __packed {
		/** Last known Hue level */
		uint16_t last;
	} transient;

#if IS_ENABLED(CONFIG_EMDS) && IS_ENABLED(CONFIG_BT_SETTINGS)
	/** Dynamic entry to be stored with EMDS */
	struct emds_dynamic_entry emds_entry;
#endif
};

/** @brief Publish the current HSL Hue status.
 *
 * Asynchronously publishes a HSL Hue status message with the configured
 * publish parameters.
 *
 * @note This API is only used publishing unprompted status messages. Response
 * messages for get and set messages are handled internally.
 *
 * @param[in] srv Server instance to publish on.
 * @param[in] ctx Message context to send with, or NULL to send with the
 * default publish parameters.
 * @param[in] status Status parameters.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_light_hue_srv_pub(struct bt_mesh_light_hue_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_light_hue_status *status);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_light_hue_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_light_hue_srv_cb;
extern const struct bt_mesh_lvl_srv_handlers
	_bt_mesh_light_hue_srv_lvl_handlers;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_HUE_SRV_H__ */

/** @} */
