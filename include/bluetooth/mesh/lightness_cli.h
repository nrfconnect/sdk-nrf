/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_lightness_cli Light Lightness Client model
 * @{
 * @brief API for the Light Lightness Client model.
 */

#ifndef BT_MESH_LIGHTNESS_CLI_H__
#define BT_MESH_LIGHTNESS_CLI_H__

#include <bluetooth/mesh/lightness.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_lightness_cli;

/** @def BT_MESH_LIGHTNESS_CLI_INIT
 *
 * @brief Initialization parameters for the @ref bt_mesh_lightness_cli.
 *
 * @param[in] _handlers Optional message handler structure.
 *
 * @sa bt_mesh_lightness_cli_handlers
 */
#define BT_MESH_LIGHTNESS_CLI_INIT(_handlers)                                  \
	{                                                                      \
		.handlers = _handlers,                                         \
	}

/** @def BT_MESH_MODEL_LIGHTNESS_CLI
 *
 * @brief Light Lightness Client model composition data entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_lightness_cli instance.
 */
#define BT_MESH_MODEL_LIGHTNESS_CLI(_cli)                                      \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_CLI,                 \
			 _bt_mesh_lightness_cli_op, &(_cli)->pub,              \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_lightness_cli, \
						 _cli),                        \
			 &_bt_mesh_lightness_cli_cb)

/** Handler function for the Light Lightness Client. */
struct bt_mesh_lightness_cli_handlers {
	/** @brief Light Level status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] status Light Lightness status contained in the
	 * message.
	 */
	void (*const light_status)(
		struct bt_mesh_lightness_cli *cli, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_lightness_status *status);

	/** @brief Default Light status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] default_light Default Light Level of the server.
	 */
	void (*const default_status)(struct bt_mesh_lightness_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     uint16_t default_light);

	/** @brief Light Range status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] status Light Range state of the server.
	 */
	void (*const range_status)(
		struct bt_mesh_lightness_cli *cli, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_lightness_range_status *status);

	/** @brief Last non-zero Light Level status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] last Last non-zero Light Level of the server.
	 */
	void (*const last_light_status)(struct bt_mesh_lightness_cli *cli,
					struct bt_mesh_msg_ctx *ctx,
					uint16_t last);
};

/**
 * Light Lightness Client instance. Should be initialized with
 * @ref BT_MESH_LIGHTNESS_CLI_INIT.
 */
struct bt_mesh_lightness_cli {
	/** Model entry. */
	const struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_LIGHTNESS_OP_SET, BT_MESH_LIGHTNESS_MSG_MAXLEN_SET)];
	/** Acknowledged message tracking. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
	/** Current transaction ID. */
	uint8_t tid;
	/** Collection of handler callbacks */
	const struct bt_mesh_lightness_cli_handlers *const handlers;
};

/** @brief Get the Light Level of the bound server.
 *
 * By default, the ACTUAL representation will be used. LINEAR representation
 * can be configured by defining CONFIG_BT_MESH_LIGHTNESS_LINEAR.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_lightness_cli_handlers::light_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[out] rsp Status response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @c rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_lightness_cli_light_get(struct bt_mesh_lightness_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    struct bt_mesh_lightness_status *rsp);

/** @brief Set the Light Level of the server.
 *
 * By default, the ACTUAL representation will be used. LINEAR representation
 * can be configured by defining CONFIG_BT_MESH_LIGHTNESS_LINEAR.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_lightness_cli_handlers::light_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set New Light Level value to set. Set @c set::transition to NULL
 * to use the server's default transition parameters.
 * @param[out] rsp Response status buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @c rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_lightness_cli_light_set(struct bt_mesh_lightness_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    const struct bt_mesh_lightness_set *set,
				    struct bt_mesh_lightness_status *rsp);

/** @brief Set the Light Level of the server without requesting a response.
 *
 * By default, the ACTUAL representation will be used. LINEAR representation
 * can be configured by defining CONFIG_BT_MESH_LIGHTNESS_LINEAR.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set New Light Level value to set. Set @c set::transition to NULL
 * to use the server's default transition parameters.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_lightness_cli_light_set_unack(
	struct bt_mesh_lightness_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_lightness_set *set);

/** @brief Get the Light Range of the bound server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_lightness_cli_handlers::range_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[out] rsp Status response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @c rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_lightness_cli_range_get(struct bt_mesh_lightness_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    struct bt_mesh_lightness_range_status *rsp);

/** @brief Set the Light Range state in the server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_lightness_cli_handlers::range_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] range New Light Range value to set.
 * @param[out] rsp Response status buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @c rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_lightness_cli_range_set(struct bt_mesh_lightness_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    const struct bt_mesh_lightness_range *range,
				    struct bt_mesh_lightness_range_status *rsp);

/** @brief Set the Light Range state in the server without requesting a
 * response.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] range New Light Range value to set.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_lightness_cli_range_set_unack(
	struct bt_mesh_lightness_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_lightness_range *range);

/** @brief Get the Default Light of the bound server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_lightness_cli_handlers::default_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[out] rsp Status response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @c rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_lightness_cli_default_get(struct bt_mesh_lightness_cli *cli,
				      struct bt_mesh_msg_ctx *ctx, uint16_t *rsp);

/** @brief Set the Default Light state in the server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_lightness_cli_handlers::default_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] default_light New Default Light value to set.
 * @param[out] rsp Response status buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @c rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_lightness_cli_default_set(struct bt_mesh_lightness_cli *cli,
				      struct bt_mesh_msg_ctx *ctx,
				      uint16_t default_light, uint16_t *rsp);

/** @brief Set the Default Light state in the server without requesting a
 * response.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] default_light New Default Light value to set.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_lightness_cli_default_set_unack(struct bt_mesh_lightness_cli *cli,
					    struct bt_mesh_msg_ctx *ctx,
					    uint16_t default_light);

/** @brief Get the last non-zero Light Level of the bound server.
 *
 * The last non-zero Light Level is the Light Level that will be restored if
 * the Light Level changes from off to on and no Default Light is set.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_lightness_cli_handlers::last_light_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[out] rsp Status response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @c rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_lightness_cli_last_get(struct bt_mesh_lightness_cli *cli,
				   struct bt_mesh_msg_ctx *ctx, uint16_t *rsp);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_lightness_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_lightness_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHTNESS_CLI_H__ */

/** @} */
