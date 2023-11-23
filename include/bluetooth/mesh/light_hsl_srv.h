/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_light_hsl_srv Light HSL Server model
 * @{
 * @brief API for the Light HSL Server model.
 */

#ifndef BT_MESH_LIGHT_HSL_SRV_H__
#define BT_MESH_LIGHT_HSL_SRV_H__

#include <bluetooth/mesh/light_hsl.h>
#include <bluetooth/mesh/light_sat_srv.h>
#include <bluetooth/mesh/light_hue_srv.h>
#include <bluetooth/mesh/lightness_srv.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_light_hsl_srv;

/** @def BT_MESH_LIGHT_HSL_SRV_INIT
 *
 *  @brief Initialization parameters for a @ref bt_mesh_light_hsl_srv instance.
 *
 *  @param[in] _lightness_srv Pointer to the Lightness Server instance.
 *  @param[in] _hue_handlers Hue Server model handlers.
 *  @param[in] _sat_handlers Saturation Server model handlers.
 */
#define BT_MESH_LIGHT_HSL_SRV_INIT(_lightness_srv, _hue_handlers, _sat_handlers)                 \
	{                                                                                          \
		.lightness = _lightness_srv,                          \
		.hue = BT_MESH_LIGHT_HUE_SRV_INIT(_hue_handlers),                                  \
		.sat = BT_MESH_LIGHT_SAT_SRV_INIT(_sat_handlers),                                  \
	}

/** @def BT_MESH_MODEL_LIGHT_HSL_SRV
 *
 *  @brief Light HSL Server model composition data entry.
 *
 *  @param[in] _srv Pointer to a @ref bt_mesh_light_hsl_srv instance.
 */
#define BT_MESH_MODEL_LIGHT_HSL_SRV(_srv)                                      \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_HSL_SRV,                       \
			 _bt_mesh_light_hsl_srv_op, &(_srv)->pub,              \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_light_hsl_srv, \
						 _srv),                        \
			 &_bt_mesh_light_hsl_srv_cb),                          \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_HSL_SETUP_SRV,                 \
			 _bt_mesh_light_hsl_setup_srv_op, NULL,                \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_light_hsl_srv, \
						 _srv),                        \
			 &_bt_mesh_light_hsl_setup_srv_cb)

/**
 * Light HSL Server instance.
 */
struct bt_mesh_light_hsl_srv {
	/** Light Hue Server instance. */
	struct bt_mesh_light_hue_srv hue;
	/** Light Saturation Server instance. */
	struct bt_mesh_light_sat_srv sat;
	/** Pointer to Lightness Server instance. */
	struct bt_mesh_lightness_srv *lightness;

	/** Model entry. */
	const struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/** Publish message buffer */
	struct net_buf_simple buf;
	/** Publish message buffer data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_LIGHT_HSL_OP_STATUS,
		BT_MESH_LIGHT_HSL_MSG_MAXLEN_STATUS)];
	/** A publication is pending, and any calls to
	 *  @ref bt_mesh_light_hsl_srv_pub will return successfully without
	 *  sending a message.
	 */
	bool pub_pending;
	/** Transaction ID tracker for the set messages. */
	struct bt_mesh_tid_ctx prev_transaction;
};

/** @brief Publish the current HSL status.
 *
 *  Asynchronously publishes a HSL status message with the configured
 *  publish parameters.
 *
 *  @note This API is only used publishing unprompted status messages. Response
 *  messages for get and set messages are handled internally.
 *
 *  @param[in] srv    Server instance to publish on.
 *  @param[in] ctx    Message context to send with, or NULL to send with the
 *                    configured publish parameters.
 *  @param[in] status Status parameters.
 *
 *  @retval 0 Successfully sent the message.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *  not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_light_hsl_srv_pub(struct bt_mesh_light_hsl_srv *srv,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_light_hsl_status *status);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_light_hsl_srv_op[];
extern const struct bt_mesh_model_op _bt_mesh_light_hsl_setup_srv_op[];
extern const struct bt_mesh_model_cb _bt_mesh_light_hsl_srv_cb;
extern const struct bt_mesh_model_cb _bt_mesh_light_hsl_setup_srv_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_HSL_SRV_H__ */

/** @} */
