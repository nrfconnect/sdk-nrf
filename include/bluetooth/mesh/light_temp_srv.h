/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
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
		.handlers = _handlers,                                         \
		.lvl = BT_MESH_LVL_SRV_INIT(                                   \
			&_bt_mesh_light_temp_srv_lvl_handlers),                \
		.pub = { .msg = NET_BUF_SIMPLE(BT_MESH_MODEL_BUF_LEN(          \
				 BT_MESH_LIGHT_TEMP_STATUS,                    \
				 BT_MESH_LIGHT_CTL_MSG_MAXLEN_TEMP_STATUS)) }, \
		.temp_range = {                                                \
			.min = BT_MESH_LIGHT_TEMP_RANGE_MIN,                   \
			.max = BT_MESH_LIGHT_TEMP_RANGE_MAX                    \
		}                                                              \
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

/** Set structure for CTL Temperature messages */
struct bt_mesh_light_temp_cb_set {
	/** Pointer to new Temperature level to set. */
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

/** Light CTL Temperature Server state access handlers. */
struct bt_mesh_light_temp_srv_handlers {
	/** @brief Set the CTL Temperature state.
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
			  const struct bt_mesh_light_temp_cb_set *set,
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
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/** Transaction ID tracker for the set messages. */
	struct bt_mesh_tid_ctx prev_transaction;
	/** Acknowledged message tracking. */
	struct bt_mesh_model_ack_ctx ack_ctx;
	/** Handler function structure. */
	const struct bt_mesh_light_temp_srv_handlers *handlers;
	/** The last known Temperature Level. */
	uint16_t temp_last;
	/** The last known Delta UV Level. */
	int16_t delta_uv_last;
	/** Current Temperature range. */
	struct bt_mesh_light_temp_range temp_range;
};

/** @brief Publish the current CTL Temperature status.
 *
 * Asynchronously publishes a CTL Temperature status message with the configured
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
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int32_t bt_mesh_light_temp_srv_pub(struct bt_mesh_light_temp_srv *srv,
				   struct bt_mesh_msg_ctx *ctx,
				   struct bt_mesh_light_temp_status *status);

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

/* @} */
