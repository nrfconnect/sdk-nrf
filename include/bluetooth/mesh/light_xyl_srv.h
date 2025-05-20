/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_light_xyl_srv Light xyL Server model
 * @{
 * @brief API for the Light xyL Server model.
 */

#ifndef BT_MESH_LIGHT_XYL_SRV_H__
#define BT_MESH_LIGHT_XYL_SRV_H__

#include <bluetooth/mesh/light_xyl.h>
#include <bluetooth/mesh/model_types.h>
#include <bluetooth/mesh/lightness_srv.h>
#if IS_ENABLED(CONFIG_EMDS) && IS_ENABLED(CONFIG_BT_SETTINGS)
#include "emds/emds.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_light_xyl_srv;

/** @def BT_MESH_LIGHT_XYL_SRV_INIT
 *
 * @brief initialization parameters for a @ref bt_mesh_light_xyl_srv instance.
 *
 * @param[in] _lightness_srv Pointer to Lightness server instance.
 * @param[in] _light_xyl_handlers Light xyL server callbacks.
 */
#define BT_MESH_LIGHT_XYL_SRV_INIT(_lightness_srv, _light_xyl_handlers)   \
	{                                                                      \
		.handlers = _light_xyl_handlers,                               \
		.lightness_srv = _lightness_srv,				\
		.range = {                                                     \
			.min = { .x = 0, .y = 0 },                             \
			.max = { .x = UINT16_MAX, .y = UINT16_MAX }            \
		}                                                              \
	}

/** @def BT_MESH_MODEL_LIGHT_XYL_SRV
 *
 * @brief Light XYL Server model composition data entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_light_xyl_srv instance.
 */
#define BT_MESH_MODEL_LIGHT_XYL_SRV(_srv)                                      \
		BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_XYL_SRV,               \
				 _bt_mesh_light_xyl_srv_op, &(_srv)->pub,      \
				 BT_MESH_MODEL_USER_DATA(                      \
					 struct bt_mesh_light_xyl_srv, _srv),  \
				 &_bt_mesh_light_xyl_srv_cb),                  \
		BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_XYL_SETUP_SRV,         \
				 _bt_mesh_light_xyl_setup_srv_op, NULL,        \
				 BT_MESH_MODEL_USER_DATA(                      \
					 struct bt_mesh_light_xyl_srv, _srv),  \
				 &_bt_mesh_light_xyl_setup_srv_cb)

/** Light xy set parameters. */
struct bt_mesh_light_xy_set {
	/** xy set parameters */
	struct bt_mesh_light_xy params;
	/**
	 * Transition time parameters for the state change, or NULL.
	 *
	 * When sending, setting the transition to NULL makes the receiver use
	 * its default transition time parameters, or 0 if no default transition
	 * time is set.
	 */
	struct bt_mesh_model_transition *transition;
};

/** Light xy status response parameters. */
struct bt_mesh_light_xy_status {
	/** Current xy parameters */
	struct bt_mesh_light_xy current;
	/** Target xy parameters */
	struct bt_mesh_light_xy target;
	/** Remaining time for the state change (ms). */
	int32_t remaining_time;
};

/** Light xyL Server state access handlers. */
struct bt_mesh_light_xyl_srv_handlers {
	/** @brief Set the xy state.
	 *
	 * When a set message is received, the model publishes a status message, with the response
	 * set to @c rsp. When an acknowledged set message is received, the model also sends a
	 * response back to a client. If a state change is non-instantaneous, for example when
	 * @ref bt_mesh_model_transition_time returns a nonzero value, the application is
	 * responsible for publishing a value of the xy state at the end of the transition.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Server to set the xy state of.
	 * @param[in] ctx Message context.
	 * @param[in] set Parameters of the state change.
	 * @param[out] rsp Response structure to be filled.
	 */
	void (*const xy_set)(struct bt_mesh_light_xyl_srv *srv,
			     struct bt_mesh_msg_ctx *ctx,
			     const struct bt_mesh_light_xy_set *set,
			     struct bt_mesh_light_xy_status *rsp);

	/** @brief Get the xy state.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Server to get the xy state of.
	 * @param[in] ctx Message context.
	 * @param[out] rsp Response structure to be filled.
	 */
	void (*const xy_get)(struct bt_mesh_light_xyl_srv *srv,
			     struct bt_mesh_msg_ctx *ctx,
			     struct bt_mesh_light_xy_status *rsp);

	/** @brief The Range state has changed.
	 *
	 * @param[in] srv Server the Range state was changed on.
	 * @param[in] ctx Context of the set message that triggered the update.
	 * @param[in] old_range The old Range.
	 * @param[in] new_range The new Range.
	 */
	void (*const range_update)(struct bt_mesh_light_xyl_srv *srv,
				   struct bt_mesh_msg_ctx *ctx,
				   const struct bt_mesh_light_xy_range *old_range,
				   const struct bt_mesh_light_xy_range *new_range);

	/** @brief The Default Parameter state has changed.
	 *
	 * @param[in] srv Server the Default Parameter state was changed on.
	 * @param[in] ctx Context of the set message that triggered the update.
	 * @param[in] old_default The old Default Parameters.
	 * @param[in] new_default The new Default Parameters.
	 */
	void (*const default_update)(struct bt_mesh_light_xyl_srv *srv,
				     struct bt_mesh_msg_ctx *ctx,
				     const struct bt_mesh_light_xy *old_default,
				     const struct bt_mesh_light_xy *new_default);
};

/**
 * Light xyL Server instance. Should be initialized with
 * @ref BT_MESH_LIGHT_XYL_SRV_INIT.
 */
struct bt_mesh_light_xyl_srv {
	/** Model entry. */
	const struct bt_mesh_model *model;
	/** Pointer to Lightness Server instance. */
	struct bt_mesh_lightness_srv *lightness_srv;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_LIGHT_XYL_OP_RANGE_STATUS,
		BT_MESH_LIGHT_XYL_MSG_LEN_RANGE_STATUS)];
	/** Transaction ID tracker for the set messages. */
	struct bt_mesh_tid_ctx prev_transaction;

	/** Current range parameters */
	struct bt_mesh_light_xy_range range;
	/** Handler function structure. */
	const struct bt_mesh_light_xyl_srv_handlers *handlers;
	/** The default xy Level. */
	struct bt_mesh_light_xy xy_default;
	struct __packed {
		/** The last known xy Level. */
		struct bt_mesh_light_xy xy_last;
	} transient;

#if IS_ENABLED(CONFIG_EMDS) && IS_ENABLED(CONFIG_BT_SETTINGS)
	/** Dynamic entry to be stored with EMDS */
	struct emds_dynamic_entry emds_entry;
#endif
};

/** @brief Publish the current xyL status.
 *
 * Asynchronously publishes a xyL status message with the configured
 * publish parameters.
 *
 * @note This API is only used publishing unprompted status messages. Response
 * messages for get and set messages are handled internally.
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
int bt_mesh_light_xyl_srv_pub(struct bt_mesh_light_xyl_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      struct bt_mesh_light_xyl_status *status);

/** @brief Publish the current xyL target status.
 *
 * Asynchronously publishes a xyL target status message with the configured
 * publish parameters.
 *
 * @note This API is only used publishing unprompted status messages. Response
 * messages for get and set messages are handled internally.
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
int bt_mesh_light_xyl_srv_target_pub(struct bt_mesh_light_xyl_srv *srv,
				     struct bt_mesh_msg_ctx *ctx,
				     struct bt_mesh_light_xyl_status *status);

/** @brief Publish the current xyL Range status.
 *
 * Asynchronously publishes a xyL Range status message with the configured
 * publish parameters.
 *
 * @note This API is only used publishing unprompted status messages. Response
 * messages for get and set messages are handled internally.
 *
 * @param[in] srv Server instance to publish on.
 * @param[in] ctx Message context to send with, or NULL to send with the
 * default publish parameters.
 * @param[in] status_code The status code of the response.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_light_xyl_srv_range_pub(struct bt_mesh_light_xyl_srv *srv,
				    struct bt_mesh_msg_ctx *ctx,
				    enum bt_mesh_model_status status_code);

/** @brief Publish the current xyL Default parameter status.
 *
 * Asynchronously publishes a xyL Default parameter status message with the
 * configured publish parameters.
 *
 * @note This API is only used publishing unprompted status messages. Response
 * messages for get and set messages are handled internally.
 *
 * @param[in] srv Server instance to publish on.
 * @param[in] ctx Message context to send with, or NULL to send with the
 * default publish parameters.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_light_xyl_srv_default_pub(struct bt_mesh_light_xyl_srv *srv,
				      struct bt_mesh_msg_ctx *ctx);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_light_xyl_srv_op[];
extern const struct bt_mesh_model_op _bt_mesh_light_xyl_setup_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_light_xyl_srv_cb;
extern const struct bt_mesh_model_cb _bt_mesh_light_xyl_setup_srv_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_XYL_SRV_H__ */

/** @} */
