/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @defgroup bt_mesh_loc_cli Generic Location Client model
 * @{
 * @brief API for the Generic Location Client model.
 */

#ifndef BT_MESH_GEN_LOC_CLI_H__
#define BT_MESH_GEN_LOC_CLI_H__

#include <bluetooth/mesh/gen_loc.h>
#include <bluetooth/mesh/model_types.h>
#include <sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_loc_cli;

/** @def BT_MESH_LOC_CLI_INIT
 *
 * @brief Initialization parameters for a @ref bt_mesh_loc_cli instance.
 *
 * @param[in] _handlers Pointer to a @ref bt_mesh_loc_cli_handlers instance.
 */
#define BT_MESH_LOC_CLI_INIT(_handlers)                                        \
	{                                                                      \
		.handlers = _handlers,                                         \
		.pub = {.msg = NET_BUF_SIMPLE(MAX(                             \
				BT_MESH_MODEL_BUF_LEN(                         \
					BT_MESH_LOC_OP_LOCAL_SET,              \
					BT_MESH_LOC_MSG_LEN_LOCAL_SET),        \
				BT_MESH_MODEL_BUF_LEN(                         \
					BT_MESH_LOC_OP_GLOBAL_SET,             \
					BT_MESH_LOC_MSG_LEN_GLOBAL_SET))) }    \
	}

/** @def BT_MESH_MODEL_LOC_CLI
 *
 * @brief Generic Location Client model composition data entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_loc_cli instance.
 */
#define BT_MESH_MODEL_LOC_CLI(_cli)                                            \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_LOCATION_CLI,                    \
			 _bt_mesh_loc_cli_op, &(_cli)->pub,                    \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_loc_cli,       \
						 _cli),                        \
			 &_bt_mesh_loc_cli_cb)

/** Location handler functions */
struct bt_mesh_loc_cli_handlers {
	/** @brief Global Location status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the incoming message.
	 * @param[in] status Global Location parameters of the Generic Location
	 * Server that published the message.
	 */
	void (*const global_status)(struct bt_mesh_loc_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    const struct bt_mesh_loc_global *status);

	/** @brief Local Location status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the incoming message.
	 * @param[in] status Local Location parameters of the Generic Location
	 * Server that published the message.
	 */
	void (*const local_status)(struct bt_mesh_loc_cli *cli,
				   struct bt_mesh_msg_ctx *ctx,
				   const struct bt_mesh_loc_local *status);
};

/**
 * Generic Location Client structure.
 *
 * Should be initialized with the @ref BT_MESH_LOC_CLI_INIT macro.
 */
struct bt_mesh_loc_cli {
	/** Handler function structure */
	const struct bt_mesh_loc_cli_handlers *const handlers;
	/** Response context for tracking acknowledged messages. */
	struct bt_mesh_model_ack_ctx ack_ctx;
	struct bt_mesh_model_pub pub; /**< Publish parameters. */
	struct bt_mesh_model *model; /**< Access model pointer. */
};

/** @brief Get the global location of the bound srv.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_loc_cli_handlers::global_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[out] rsp Global Location response buffer, or NULL to keep from
 * blocking.
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
int bt_mesh_loc_cli_global_get(struct bt_mesh_loc_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       struct bt_mesh_loc_global *rsp);

/** @brief Set the Global Location in the server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_loc_cli_handlers::global_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] loc New Global Location value to set.
 * @param[out] rsp Response status buffer, or NULL to keep from blocking.
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
int bt_mesh_loc_cli_global_set(struct bt_mesh_loc_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       const struct bt_mesh_loc_global *loc,
			       struct bt_mesh_loc_global *rsp);

/** @brief Set the Global Location in the server without requesting a response.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] loc New Global Location value to set.
 *
 * @retval 0 Successfully sent the message.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_loc_cli_global_set_unack(struct bt_mesh_loc_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     const struct bt_mesh_loc_global *loc);

/** @brief Get the local location of the bound srv.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_loc_cli_handlers::local_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[out] rsp Local Location response buffer, or NULL to keep from
 * blocking.
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
int bt_mesh_loc_cli_local_get(struct bt_mesh_loc_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      struct bt_mesh_loc_local *rsp);

/** @brief Set the Local Location in the server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_loc_cli_handlers::local_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] loc New Local Location value to set.
 * @param[out] rsp Response status buffer, or NULL to keep from blocking.
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
int bt_mesh_loc_cli_local_set(struct bt_mesh_loc_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_loc_local *loc,
			      struct bt_mesh_loc_local *rsp);

/** @brief Set the Local Location in the server without requesting a response.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] loc New Local Location value to set.
 *
 * @retval 0 Successfully sent the message.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_loc_cli_local_set_unack(struct bt_mesh_loc_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    const struct bt_mesh_loc_local *loc);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_loc_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_loc_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_LOC_CLI_H__ */

/* @} */
