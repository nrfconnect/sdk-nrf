/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_ponoff_cli Generic Power OnOff Client
 * @{
 * @brief API for the Generic Power OnOff Client.
 */
#ifndef BT_MESH_GEN_PONOFF_CLI_H__
#define BT_MESH_GEN_PONOFF_CLI_H__

#include <bluetooth/mesh/gen_ponoff.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_ponoff_cli;

/** @def BT_MESH_PONOFF_CLI_INIT
 *
 * @brief Initialization parameters for @ref bt_mesh_ponoff_cli.
 *
 * @param[in] _power_onoff_status_handler OnPowerUp status handler.
 */
#define BT_MESH_PONOFF_CLI_INIT(_power_onoff_status_handler)                   \
	{                                                                      \
		.status_handler = _power_onoff_status_handler,                 \
	}

/** @def BT_MESH_MODEL_PONOFF_CLI
 *
 * @brief Generic Power OnOff Client model composition data entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_ponoff_cli instance.
 */
#define BT_MESH_MODEL_PONOFF_CLI(_cli)                                         \
		BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_POWER_ONOFF_CLI,         \
				 _bt_mesh_ponoff_cli_op, &(_cli)->pub,         \
				 BT_MESH_MODEL_USER_DATA(                      \
					 struct bt_mesh_ponoff_cli, _cli),     \
				 &_bt_mesh_ponoff_cli_cb)

/**
 * Generic Power OnOff client instance.
 *
 * Should be initialized with @ref BT_MESH_PONOFF_CLI_INIT.
 */
struct bt_mesh_ponoff_cli {
	/** Model entry pointer. */
	const struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(BT_MESH_PONOFF_OP_SET,
					       BT_MESH_PONOFF_MSG_LEN_SET)];
	/** Response context for tracking acknowledged messages. */
	struct bt_mesh_msg_ack_ctx ack_ctx;

	/** @brief OnPowerUp status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Message context the message was received with.
	 * @param[in] on_power_up The OnPowerUp state presented in the message.
	 */
	void (*const status_handler)(struct bt_mesh_ponoff_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     enum bt_mesh_on_power_up on_power_up);
};

/** @brief Get the OnPowerUp state of a server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_ponoff_cli::status_handler callback.
 *
 * @param[in] cli Power OnOff client to send the message on.
 * @param[in] ctx Context of the message, or NULL to send on the configured
 * publish parameters.
 * @param[out] rsp Response buffer to put the received response in, or NULL to
 * process the response in the status handler callback.
 *
 * @retval 0 Successfully sent a get message. If a response buffer is
 * provided, it has been populated.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_ponoff_cli_on_power_up_get(struct bt_mesh_ponoff_cli *cli,
				       struct bt_mesh_msg_ctx *ctx,
				       enum bt_mesh_on_power_up *rsp);

/** @brief Set the OnPowerUp state of a server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_ponoff_cli::status_handler callback.
 *
 * @param[in] cli Power OnOff client to send the message on.
 * @param[in] ctx Context of the message, or NULL to send with the configured
 * publish parameters.
 * @param[in] on_power_up New OnPowerUp state of the server.
 * @param[out] rsp Response buffer to put the received response in, or NULL to
 * keep from blocking.
 *
 * @retval 0 Successfully sent a set message. If a response buffer is
 * provided, it has been populated.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_ponoff_cli_on_power_up_set(struct bt_mesh_ponoff_cli *cli,
				       struct bt_mesh_msg_ctx *ctx,
				       enum bt_mesh_on_power_up on_power_up,
				       enum bt_mesh_on_power_up *rsp);

/** @brief Set the OnPowerUp state of a server without requesting a response.
 *
 * @param[in] cli Power OnOff client to send the message on.
 * @param[in] ctx Context of the message, or NULL to send with the configured
 * publish parameters.
 * @param[in] on_power_up New OnPowerUp state of the server.
 *
 * @retval 0 Successfully sent a set message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_ponoff_cli_on_power_up_set_unack(
	struct bt_mesh_ponoff_cli *cli, struct bt_mesh_msg_ctx *ctx,
	enum bt_mesh_on_power_up on_power_up);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_ponoff_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_ponoff_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_PONOFF_CLI_H__ */

/** @} */
