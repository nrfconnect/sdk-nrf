/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_battery_cli Generic Battery Client model
 * @{
 * @brief API for the Generic Battery Client model.
 */

#ifndef BT_MESH_GEN_BATTERY_CLI_H__
#define BT_MESH_GEN_BATTERY_CLI_H__

#include <bluetooth/mesh/gen_battery.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_battery_cli;

/** @def BT_MESH_BATTERY_CLI_INIT
 *
 * @brief Initialization parameters for a @ref bt_mesh_battery_cli instance.
 *
 * @param[in] _status_handler Optional status message handler.
 */
#define BT_MESH_BATTERY_CLI_INIT(_status_handler)                              \
	{                                                                      \
		.status_handler = _status_handler,                             \
	}

/** @def BT_MESH_MODEL_BATTERY_CLI
 *
 * @brief Generic Battery Client model composition data entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_battery_cli instance.
 */
#define BT_MESH_MODEL_BATTERY_CLI(_cli)                                        \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_BATTERY_CLI,                     \
			 _bt_mesh_battery_cli_op, &(_cli)->pub,                \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_battery_cli,   \
						 _cli),                        \
			 &_bt_mesh_battery_cli_cb)

/**
 * Generic Battery Client structure.
 *
 * Should be initialized with the @ref BT_MESH_BATTERY_CLI_INIT macro.
 */
struct bt_mesh_battery_cli {
	/** @brief Battery status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the incoming message.
	 * @param[in] status Battery Status of the Generic Battery Server that
	 * published the message.
	 */
	void (*const status_handler)(
		struct bt_mesh_battery_cli *cli, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_battery_status *status);

	/** Response context for tracking acknowledged messages. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(BT_MESH_BATTERY_OP_GET,
					       BT_MESH_BATTERY_MSG_LEN_GET)];
	/** Composition data model entry pointer. */
	const struct bt_mesh_model *model;
};

/** @brief Get the status of the bound srv.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_battery_cli::status_handler callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[out] rsp Status response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @p rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_battery_cli_get(struct bt_mesh_battery_cli *cli,
			    struct bt_mesh_msg_ctx *ctx,
			    struct bt_mesh_battery_status *rsp);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_battery_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_battery_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_BATTERY_CLI_H__ */

/** @} */
