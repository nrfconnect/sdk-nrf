/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_light_temp_srv Light Temperature Server model
 * @{
 * @brief API for the Light CTL Temperature Server model.
 */

#ifndef BT_MESH_LIGHT_TEMP_SRV_H__
#define BT_MESH_LIGHT_TEMP_SRV_H__

#include <bluetooth/mesh/light_ctl.h>
#include <bluetooth/mesh/model_types.h>
#include <bluetooth/mesh/gen_lvl_srv.h>
#if IS_ENABLED(CONFIG_EMDS) && IS_ENABLED(CONFIG_BT_SETTINGS)
#include "emds/emds.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_light_temp_srv;

/** @def BT_MESH_LIGHT_TEMP_SRV_INIT
 *
 * @brief Initialization parameters for a @ref bt_mesh_light_temp_srv
 * instance.
 *
 * @param[in] _handlers Light CTL Temperature Server callbacks.
 */
#define BT_MESH_LIGHT_TEMP_SRV_INIT(_handlers)                                 \
	{                                                                      \
		.lvl = BT_MESH_LVL_SRV_INIT(                                   \
			&_bt_mesh_light_temp_srv_lvl_handlers),                \
		.pub = { .msg = NET_BUF_SIMPLE(BT_MESH_MODEL_BUF_LEN(          \
				 BT_MESH_LIGHT_TEMP_STATUS,                    \
				 BT_MESH_LIGHT_CTL_MSG_MAXLEN_TEMP_STATUS)) }, \
		.handlers = _handlers,                                         \
	}

/** @def BT_MESH_MODEL_LIGHT_TEMP_SRV
 *
 * @brief Light CTL Temperature Server model composition data entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_light_temp_srv instance.
 */
#define BT_MESH_MODEL_LIGHT_TEMP_SRV(_srv)                                     \
	BT_MESH_MODEL_LVL_SRV(&(_srv)->lvl),                                   \
	BT_MESH_MODEL_CB(                                                      \
		BT_MESH_MODEL_ID_LIGHT_CTL_TEMP_SRV,                           \
		_bt_mesh_light_temp_srv_op, &(_srv)->pub,                      \
		BT_MESH_MODEL_USER_DATA(struct bt_mesh_light_temp_srv, _srv),  \
		&_bt_mesh_light_temp_srv_cb)

/** Light CTL Temperature Server state access handlers. */
struct bt_mesh_light_temp_srv_handlers {
	/** @brief Set the Light Temperature state.
	 *
	 * When a set message is received, the model publishes a status message, with the response
	 * set to @c rsp. When an acknowledged set message is received, the model also sends a
	 * response back to a client. If a state change is non-instantaneous, for example when
	 * @ref bt_mesh_model_transition_time returns a nonzero value, the application is
	 * responsible for publishing a value of the Light Temperature state at the end of the
	 * transition.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Server to set the CTL Temperature state of.
	 * @param[in] ctx Message context.
	 * @param[in] set Parameters of the state change.
	 * @param[out] rsp Response structure to be filled.
	 */
	void (*const set)(struct bt_mesh_light_temp_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_light_temp_set *set,
			  struct bt_mesh_light_temp_status *rsp);

	/** @brief Get the CTL Temperature state.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Server to get the CTL state of.
	 * @param[in] ctx Message context.
	 * @param[out] rsp Response structure to be filled.
	 */
	void (*const get)(struct bt_mesh_light_temp_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_light_temp_status *rsp);

	/** @brief The Temperature Range state has changed.
	 *
	 *  @param[in] srv Server the Temperature Range state was changed on.
	 *  @param[in] ctx Context of the set message that triggered the update.
	 *  @param[in] old_range The old Temperature Range.
	 *  @param[in] new_range The new Temperature Range.
	 */
	void (*const range_update)(
		struct bt_mesh_light_temp_srv *srv, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_light_temp_range *old_range,
		const struct bt_mesh_light_temp_range *new_range);

	/** @brief The Default Light Temperature has changed.
	 *
	 *  @param[in] srv Server the Default Light Temperature state was
	 *                 changed on.
	 *  @param[in] ctx Context of the set message that triggered the update.
	 *  @param[in] old_default The old Default Light Temperature.
	 *  @param[in] new_default The new Default Light Temperature.
	 */
	void (*const default_update)(
		struct bt_mesh_light_temp_srv *srv, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_light_temp *old_default,
		const struct bt_mesh_light_temp *new_default);
};

/**
 * Light CTL Temperature Server instance. Should be initialized with
 * @ref BT_MESH_LIGHT_TEMP_SRV_INIT.
 */
struct bt_mesh_light_temp_srv {
	/** Light Level Server instance. */
	struct bt_mesh_lvl_srv lvl;
	/** Model entry. */
	struct bt_mesh_model *model;
	/** Pointer to the corresponding CTL server, if it has one.
	 *  Is set automatically by the CTL server.
	 */
	const struct bt_mesh_light_ctl_srv *ctl;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/** Transaction ID tracker for the set messages. */
	struct bt_mesh_tid_ctx prev_transaction;
	/** Handler function structure. */
	const struct bt_mesh_light_temp_srv_handlers *handlers;

	/** Default light temperature and delta UV */
	struct bt_mesh_light_temp dflt;
	/** Current Temperature range. */
	struct bt_mesh_light_temp_range range;
	/** Corrective delta used by the generic level server. */
	uint16_t corrective_delta;
	struct __packed {
		/** The last known color temperature. */
		struct bt_mesh_light_temp last;
	} transient;

#if IS_ENABLED(CONFIG_EMDS) && IS_ENABLED(CONFIG_BT_SETTINGS)
	/** Dynamic entry to be stored with EMDS */
	struct emds_dynamic_entry emds_entry;
#endif
};

/** @brief Publish the current CTL Temperature status.
 *
 * Asynchronously publishes a CTL Temperature status message with the configured
 * publish parameters.
 *
 * @note This API is only used for publishing unprompted status messages.
 * Response messages for get and set messages are handled internally.
 *
 * @param[in] srv Server instance to publish on.
 * @param[in] ctx Message context to send with, or NULL to send with the
 * default publish parameters.
 * @param[in] status Current status.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_light_temp_srv_pub(struct bt_mesh_light_temp_srv *srv,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_light_temp_status *status);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_light_temp_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_light_temp_srv_cb;
extern const struct bt_mesh_lvl_srv_handlers
	_bt_mesh_light_temp_srv_lvl_handlers;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_TEMP_SRV_H__ */

/** @} */
