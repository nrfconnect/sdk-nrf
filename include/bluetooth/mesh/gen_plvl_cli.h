/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @defgroup bt_mesh_plvl_cli Generic Power Level Client model
 * @{
 * @brief API for the Generic Power Level Client model.
 */

#ifndef BT_MESH_GEN_PLVL_CLI_H__
#define BT_MESH_GEN_PLVL_CLI_H__

#include <bluetooth/mesh/gen_plvl.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_plvl_cli;

/** @def BT_MESH_PLVL_CLI_INIT
 *
 * @brief Initialization parameters for the @ref bt_mesh_plvl_cli.
 *
 * @param[in] _handlers Optional message handler structure.
 * @sa bt_mesh_plvl_cli_handlers.
 */
#define BT_MESH_PLVL_CLI_INIT(_handlers)                                       \
	{                                                                      \
		.pub = { .msg = NET_BUF_SIMPLE(BT_MESH_MODEL_BUF_LEN(          \
				 BT_MESH_PLVL_OP_LEVEL_SET,                    \
				 BT_MESH_PLVL_MSG_MAXLEN_LEVEL_SET)) },        \
		.handlers = _handlers,                                         \
	}

/** @def BT_MESH_MODEL_PLVL_CLI
 *
 * @brief Generic Power Level Client model composition data entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_plvl_cli instance.
 */
#define BT_MESH_MODEL_PLVL_CLI(_cli)                                           \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_POWER_LEVEL_CLI,                 \
			 _bt_mesh_plvl_cli_op, &(_cli)->pub,                   \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_plvl_cli,      \
						 _cli),                        \
			 &_bt_mesh_plvl_cli_cb)

/** Handler function for the Power Level Client. */
struct bt_mesh_plvl_cli_handlers {
	/** @brief Power Level status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] status Generic Power Level status contained in the
	 * message.
	 */
	void (*const power_status)(struct bt_mesh_plvl_cli *cli,
				   struct bt_mesh_msg_ctx *ctx,
				   const struct bt_mesh_plvl_status *status);

	/** @brief Default Power status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] default_power Default Power Level of the server.
	 */
	void (*const default_status)(struct bt_mesh_plvl_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     u16_t default_power);

	/** @brief Power Range status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] status Power Range state of the server.
	 */
	void (*const range_status)(
		struct bt_mesh_plvl_cli *cli, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_plvl_range_status *status);

	/** @brief Last non-zero Power Level status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] last Last non-zero Power Level of the server.
	 */
	void (*const last_status)(struct bt_mesh_plvl_cli *cli,
				  struct bt_mesh_msg_ctx *ctx, u16_t last);
};

/**
 * Generic Power Level Client instance. Should be initialized with
 * @ref BT_MESH_PLVL_CLI_INIT.
 */
struct bt_mesh_plvl_cli {
	/** Model entry. */
	struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/** Acknowledged message tracking. */
	struct bt_mesh_model_ack_ctx ack_ctx;
	/** Current transaction ID. */
	u8_t tid;
	/** Collection of handler callbacks */
	const struct bt_mesh_plvl_cli_handlers *const handlers;
};

/** @brief Get the Power Level of the bound server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_plvl_cli_handlers::power_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] rsp Status response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @p rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_plvl_cli_power_get(struct bt_mesh_plvl_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       struct bt_mesh_plvl_status *rsp);

/** @brief Set the Power Level of the server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_plvl_cli_handlers::power_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set New Power Level value to set. Set @p set::transition to NULL
 * to use the server's default transition parameters.
 * @param[in] rsp Response status buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @p rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_plvl_cli_power_set(struct bt_mesh_plvl_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_plvl_set *set,
			       struct bt_mesh_plvl_status *rsp);

/** @brief Set the Power Level of the server without requesting a response.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set New Power Level value to set. Set @p set::transition to NULL
 * to use the server's default transition parameters.
 *
 * @retval 0 Successfully sent the message.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_plvl_cli_power_set_unack(struct bt_mesh_plvl_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     const struct bt_mesh_plvl_set *set);

/** @brief Get the Power Range of the bound server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_plvl_cli_handlers::range_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] rsp Status response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @p rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_plvl_cli_range_get(struct bt_mesh_plvl_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       struct bt_mesh_plvl_range_status *rsp);

/** @brief Set the Power Range state in the server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_plvl_cli_handlers::range_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] range New Power Range value to set.
 * @param[in] rsp Response status buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @p rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_plvl_cli_range_set(struct bt_mesh_plvl_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_plvl_range *range,
			       struct bt_mesh_plvl_range_status *rsp);

/** @brief Set the Power Range state in the server without requesting a
 * response.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] range New Power Range value to set.
 *
 * @retval 0 Successfully sent the message.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_plvl_cli_range_set_unack(struct bt_mesh_plvl_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     const struct bt_mesh_plvl_range *range);

/** @brief Get the Default Power of the bound server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_plvl_cli_handlers::default_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] rsp Status response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @p rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_plvl_cli_default_get(struct bt_mesh_plvl_cli *cli,
				 struct bt_mesh_msg_ctx *ctx, u16_t *rsp);

/** @brief Set the Default Power state in the server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_plvl_cli_handlers::default_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] default_power New Default Power value to set.
 * @param[in] rsp Response status buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @p rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_plvl_cli_default_set(struct bt_mesh_plvl_cli *cli,
				 struct bt_mesh_msg_ctx *ctx,
				 u16_t default_power, u16_t *rsp);

/** @brief Set the Default Power state in the server without requesting a
 * response.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] default_power New Default Power value to set.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_plvl_cli_default_set_unack(struct bt_mesh_plvl_cli *cli,
				 struct bt_mesh_msg_ctx *ctx,
				 u16_t default_power);

/** @brief Get the last non-zero Power Level of the bound server.
 *
 * The last non-zero Power Level is the Power Level that will be restored if
 * the Power Level changes from off to on and no Default Power is set.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_plvl_cli_handlers::last_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] rsp Status response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @p rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_plvl_cli_last_get(struct bt_mesh_plvl_cli *cli,
			      struct bt_mesh_msg_ctx *ctx, u16_t *rsp);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_plvl_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_plvl_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_PLVL_CLI_H__ */

/** @} */
