/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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
 * @param[in] _lightness_handlers Lightness server callbacks.
 * @param[in] _light_temp_handlers Light temperature server callbacks.
 */
#define BT_MESH_LIGHT_CTL_SRV_INIT(_lightness_handlers, _light_temp_handlers)  \
	{                                                                      \
		.lightness_srv =                                               \
			BT_MESH_LIGHTNESS_SRV_INIT(_lightness_handlers),       \
		.temp_srv = BT_MESH_LIGHT_TEMP_SRV_INIT(_light_temp_handlers), \
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
			 _bt_mesh_light_ctl_setup_srv_op, NULL,                \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_light_ctl_srv, \
						 _srv),                        \
			 &_bt_mesh_light_ctl_setup_srv_cb)

/**
 * Light CTL Server instance. Should be initialized with
 * @ref BT_MESH_LIGHT_CTL_SRV_INIT.
 */
struct bt_mesh_light_ctl_srv {
	/** Light CTL Temp. Server instance. */
	struct bt_mesh_light_temp_srv temp_srv;
	/** Lightness Server instance. */
	struct bt_mesh_lightness_srv lightness_srv;
	/** Model entry. */
	const struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_LIGHT_CTL_STATUS, BT_MESH_LIGHT_CTL_MSG_MAXLEN_STATUS)];
	/** Transaction ID tracker for the set messages. */
	struct bt_mesh_tid_ctx prev_transaction;
};

/** @brief Publish the current CTL status.
 *
 * Asynchronously publishes a CTL status message with the configured
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
int bt_mesh_light_ctl_pub(struct bt_mesh_light_ctl_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_light_ctl_status *status);

/** @brief Publish the current CTL Range status.
 *
 * Asynchronously publishes a CTL Range status message with the configured
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
int bt_mesh_light_ctl_range_pub(struct bt_mesh_light_ctl_srv *srv,
				struct bt_mesh_msg_ctx *ctx,
				enum bt_mesh_model_status status);

/** @brief Publish the current CTL Default status.
 *
 * Asynchronously publishes a CTL Default status message with the configured
 * publish parameters.
 *
 * @note This API is only used for publishing unprompted status messages.
 * Response messages for get and set messages are handled internally.
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
int bt_mesh_light_ctl_default_pub(struct bt_mesh_light_ctl_srv *srv,
				  struct bt_mesh_msg_ctx *ctx);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_light_ctl_srv_op[];
extern const struct bt_mesh_model_op _bt_mesh_light_ctl_setup_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_light_ctl_srv_cb;
extern const struct bt_mesh_model_cb _bt_mesh_light_ctl_setup_srv_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_CTL_SRV_H__ */

/** @} */
