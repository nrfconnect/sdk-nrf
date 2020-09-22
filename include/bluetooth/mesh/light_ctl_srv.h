/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @defgroup bt_mesh_light_ctl_srv Light CTL Server model
 * @{
 * @brief API for the Light CTL Server model.
 */

#ifndef BT_MESH_LIGHT_CTL_SRV_H__
#define BT_MESH_LIGHT_CTL_SRV_H__

#include <bluetooth/mesh/light_ctl.h>
#include <bluetooth/mesh/light_temp_srv.h>
#include <bluetooth/mesh/model_types.h>
#include <bluetooth/mesh/lightness_srv.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_light_ctl_srv;

/** @def BT_MESH_LIGHT_CTL_SRV_INIT
 *
 * @brief Initialization parameters for a @ref bt_mesh_light_ctl_srv instance.
 *
 * @param[in] _handlers Light CTL server callbacks.
 */
#define BT_MESH_LIGHT_CTL_SRV_INIT(_handlers)                                  \
	{                                                                      \
		.handlers = _handlers,                                         \
		.lightness_srv = BT_MESH_LIGHTNESS_SRV_INIT(                   \
			&_bt_mesh_light_ctl_lightness_handlers),               \
		.temp_srv = BT_MESH_LIGHT_TEMP_SRV_INIT(                       \
			&_bt_mesh_light_temp_handlers),                        \
		.pub = { .msg = NET_BUF_SIMPLE(BT_MESH_MODEL_BUF_LEN(          \
				 BT_MESH_LIGHT_CTL_STATUS,                     \
				 BT_MESH_LIGHT_CTL_MSG_MAXLEN_STATUS)) },      \
	}

/** @def BT_MESH_MODEL_LIGHT_CTL_SRV
 *
 * @brief Light CTL Server model composition data entry.
 *
 * @param[in] _srv Pointer to a @ref bt_mesh_light_ctl_srv instance.
 */
#define BT_MESH_MODEL_LIGHT_CTL_SRV(_srv)                                      \
	BT_MESH_MODEL_LIGHTNESS_SRV(&(_srv)->lightness_srv),                   \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_CTL_SRV,                       \
			 _bt_mesh_light_ctl_srv_op, &(_srv)->pub,              \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_light_ctl_srv, \
						 _srv),                        \
			 &_bt_mesh_light_ctl_srv_cb),                          \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_CTL_SETUP_SRV,                 \
			 _bt_mesh_light_ctl_setup_srv_op, &(_srv)->setup_pub,  \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_light_ctl_srv, \
						 _srv),                        \
			 NULL)

/** Generic status response structure for CTL messages. */
struct bt_mesh_light_ctl_rsp {
	/** Current CTL parameters */
	struct bt_mesh_light_ctl current;
	/** Target CTL parameters */
	struct bt_mesh_light_ctl target;
	/** Remaining time for the state change (ms). */
	int32_t remaining_time;
};

/** Generic set structure for CTL messages. */
struct bt_mesh_light_ctl_gen_cb_set {
	/**
	 * Pointer to new Light level to set.
	 * @note NULL if not applicable.
	 */
	uint16_t *light;
	/**
	 * Pointer to new Temperature level to set.
	 * @note NULL if not applicable.
	 */
	uint16_t *temp;
	/**
	 * Pointer to new Delta UV level to set.
	 * @note NULL if not applicable.
	 */
	int16_t *delta_uv;
	/** Transition time value in milliseconds. */
	uint32_t time;
	/** Message execution delay in milliseconds. */
	uint32_t delay;
};

/** Light CTL Server state access handlers. */
struct bt_mesh_light_ctl_srv_handlers {
	/** @brief Set the CTL state.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Server to set the CTL state of.
	 * @param[in] ctx Message context.
	 * @param[in] set Parameters of the state change.
	 * @param[out] rsp Response structure to be filled.
	 */
	void (*const set)(struct bt_mesh_light_ctl_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_light_ctl_gen_cb_set *set,
			  struct bt_mesh_light_ctl_rsp *rsp);

	/** @brief Get the CTL state.
	 *
	 * @note This handler is mandatory.
	 *
	 * @param[in] srv Server to get the CTL state of.
	 * @param[in] ctx Message context.
	 * @param[out] rsp Response structure to be filled.
	 */
	void (*const get)(struct bt_mesh_light_ctl_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_light_ctl_rsp *rsp);

	/** @brief The Temperature Range state has changed.
	 *
	 * @param[in] srv Server the Temperature Range state was changed on.
	 * @param[in] ctx Context of the set message that triggered the update.
	 * @param[in] set The new Temperature Range.
	 */
	void (*const temp_range_update)(
		struct bt_mesh_light_ctl_srv *srv, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_light_temp_range *set);

	/** @brief The Default Parameter state has changed.
	 *
	 * @param[in] srv Server the Default Parameter state was changed on.
	 * @param[in] ctx Context of the set message that triggered the update.
	 * @param[in] set The new Default Parameters.
	 */
	void (*const default_update)(struct bt_mesh_light_ctl_srv *srv,
				     struct bt_mesh_msg_ctx *ctx,
				     const struct bt_mesh_light_ctl *set);

	/** @brief The Light Range state has changed.
	 *
	 * @param[in] srv Server the Light Range state was changed on.
	 * @param[in] ctx Context of the set message that triggered the update,
	 * or NULL if it was not triggered by a message.
	 * @param[in] old_range The Light Range before the change.
	 * @param[in] new_range The Light Range after the change.
	 */
	void (*const lightness_range_update)(
		struct bt_mesh_light_ctl_srv *srv, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_lightness_range *old_range,
		const struct bt_mesh_lightness_range *new_range);
};

/**
 * Light CTL Server instance. Should be initialized with
 * @ref BT_MESH_LIGHT_CTL_SRV_INIT.
 */
struct bt_mesh_light_ctl_srv {
	/** Model entry. */
	struct bt_mesh_model *model;
	/** Light CTL Temp. Server instance. */
	struct bt_mesh_light_temp_srv temp_srv;
	/** Lightness Server instance. */
	struct bt_mesh_lightness_srv lightness_srv;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/** Transaction ID tracker for the set messages. */
	struct bt_mesh_tid_ctx prev_transaction;
	/** Setup model publish parameters */
	struct bt_mesh_model_pub setup_pub;
	/** Acknowledged message tracking. */
	struct bt_mesh_model_ack_ctx ack_ctx;
	/** Model state structure */
	struct bt_mesh_light_ctl default_params;
	/** Handler function structure. */
	const struct bt_mesh_light_ctl_srv_handlers *handlers;
	/** Scene entry */
	struct bt_mesh_scene_entry scene;
};

/** @brief Publish the current CTL status.
 *
 * Asynchronously publishes a CTL status message with the configured
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
 * @retval 0 Successfully published a Generic OnOff Status message.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int32_t bt_mesh_light_ctl_pub(struct bt_mesh_light_ctl_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      struct bt_mesh_light_ctl_status *status);

/** @brief Publish the current CTL Range status.
 *
 * Asynchronously publishes a CTL Range status message with the configured
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
 * @retval 0 Successfully published a Generic OnOff Status message.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int32_t bt_mesh_light_ctl_range_pub(struct bt_mesh_light_ctl_srv *srv,
				    struct bt_mesh_msg_ctx *ctx,
				    enum bt_mesh_model_status status);

/** @brief Publish the current CTL Default status.
 *
 * Asynchronously publishes a CTL Default status message with the configured
 * publish parameters.
 *
 * @note This API is only used publishing unprompted status messages. Response
 * messages for get and set messages are handled internally.
 *
 * @param[in] srv Server instance to publish on.
 * @param[in] ctx Message context to send with, or NULL to send with the
 * default publish parameters.
 *
 * @retval 0 Successfully published a Generic OnOff Status message.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int32_t bt_mesh_light_ctl_default_pub(struct bt_mesh_light_ctl_srv *srv,
				      struct bt_mesh_msg_ctx *ctx);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_light_ctl_srv_op[];
extern const struct bt_mesh_model_op _bt_mesh_light_ctl_setup_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_light_ctl_srv_cb;
extern const struct bt_mesh_light_temp_srv_handlers
	_bt_mesh_light_temp_handlers;
extern const struct bt_mesh_lightness_srv_handlers
	_bt_mesh_light_ctl_lightness_handlers;

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_CTL_SRV_H__ */

/* @} */
