/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_lvl_cli Generic Level Client model
 * @{
 * @brief API for the Generic Level Client model.
 */
#ifndef BT_MESH_GEN_LVL_CLI_H__
#define BT_MESH_GEN_LVL_CLI_H__

#include <bluetooth/mesh/gen_lvl.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_lvl_cli;

/** @def BT_MESH_LVL_CLI_INIT
 *
 * @brief Initialization parameters for the @ref bt_mesh_lvl_cli.
 *
 * @param[in] _status_handler Optional status message handler.
 * @sa bt_mesh_lvl_cli::status_handler
 */
#define BT_MESH_LVL_CLI_INIT(_status_handler)                                  \
	{                                                                      \
		.status_handler = _status_handler,                             \
	}

/** @def BT_MESH_MODEL_LVL_CLI
 *
 * @brief Generic Level Client model composition data entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_lvl_cli instance.
 */
#define BT_MESH_MODEL_LVL_CLI(_cli)                                            \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_LEVEL_CLI, _bt_mesh_lvl_cli_op,  \
			 &(_cli)->pub,                                         \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_lvl_cli,       \
						 _cli),                        \
			 &_bt_mesh_lvl_cli_cb)

/**
 * Generic Level Client instance.
 *
 * Should be initialized with @ref BT_MESH_LVL_CLI_INIT.
 */
struct bt_mesh_lvl_cli {
	/** Model entry. */
	const struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_LVL_OP_DELTA_SET, BT_MESH_LVL_MSG_MAXLEN_DELTA_SET)];
	/** Acknowledged message tracking. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
	/** Current transaction ID. */
	uint8_t tid;

	/** @brief Level status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] status Generic level status contained in the message.
	 */
	void (*const status_handler)(struct bt_mesh_lvl_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     const struct bt_mesh_lvl_status *status);
};

/** @brief Get the status of the bound server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_lvl_cli::status_handler callback.
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
int bt_mesh_lvl_cli_get(struct bt_mesh_lvl_cli *cli,
			struct bt_mesh_msg_ctx *ctx,
			struct bt_mesh_lvl_status *rsp);

/** @brief Set the level state in the server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_lvl_cli::status_handler callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set New level value to set. Set @p set::transition to NULL to use
 * the server's default transition parameters.
 * @param[out] rsp Response status buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @p rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_lvl_cli_set(struct bt_mesh_lvl_cli *cli,
			struct bt_mesh_msg_ctx *ctx,
			const struct bt_mesh_lvl_set *set,
			struct bt_mesh_lvl_status *rsp);

/** @brief Set the level state in the server without requesting a response.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set New level value to set. Set @p set::transition to NULL to use
 * the server's default transition parameters.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_lvl_cli_set_unack(struct bt_mesh_lvl_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_lvl_set *set);

/** @brief Trigger a differential level state change in the server.
 *
 * Makes the server move its level state by some delta value. If multiple
 * delta_set messages are sent in a row (with less than 6 seconds interval),
 * and @p delta_set::new_transaction is set to false, the server will continue
 * using the same base value for its delta as in the first message, unless
 * some other client made changes to the server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_lvl_cli::status_handler callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] delta_set State change to make. Set @p set::transition to NULL to
 * use the server's default transition parameters.
 * @param[out] rsp Response status buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @p rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_lvl_cli_delta_set(struct bt_mesh_lvl_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_lvl_delta_set *delta_set,
			      struct bt_mesh_lvl_status *rsp);

/** @brief Trigger a differential level state change in the server without
 * requesting a response.
 *
 * Makes the server move its level state by some delta value. If multiple
 * delta_set messages are sent in a row (with less than 6 seconds interval),
 * and @p delta_set::new_transaction is set to false, the server will continue
 * using the same base value for its delta as in the first message, unless
 * some other client made changes to the server.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] delta_set State change to make. Set @p set::transition to NULL to
 * use the server's default transition parameters.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_lvl_cli_delta_set_unack(
	struct bt_mesh_lvl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_lvl_delta_set *delta_set);

/** @brief Trigger a continuous level change in the server.
 *
 * Makes the server continuously move its level state by the set rate:
 *
 * @code
 * rate_of_change = move_set->delta / move_set->transition->time
 * @endcode
 *
 * The server will continue moving its level until it is told to stop, or until
 * it reaches some application specific boundary value. The server may choose
 * to wrap around the level value, depending on its usage. The move can be
 * stopped by sending a new move message with a delta value of 0.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_lvl_cli::status_handler callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] move_set State change to make. Set @p set::transition to NULL to
 * use the server's default transition parameters.
 * @param[out] rsp Response status buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @p rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_lvl_cli_move_set(struct bt_mesh_lvl_cli *cli,
			     struct bt_mesh_msg_ctx *ctx,
			     const struct bt_mesh_lvl_move_set *move_set,
			     struct bt_mesh_lvl_status *rsp);

/** @brief Trigger a continuous level change in the server without requesting
 * a response.
 *
 * Makes the server continuously move its level state by the set rate:
 *
 * @code
 * rate_of_change = move_set->delta / move_set->transition->time
 * @endcode
 *
 * The server will continue moving its level until it is told to stop, or until
 * it reaches some application specific boundary value. The server may choose
 * to wrap around the level value, depending on its usage. The move can be
 * stopped by sending a new move message with a delta value of 0.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] move_set State change to make. Set @p set::transition to NULL to
 * use the server's default transition parameters.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_lvl_cli_move_set_unack(struct bt_mesh_lvl_cli *cli,
				   struct bt_mesh_msg_ctx *ctx,
				   const struct bt_mesh_lvl_move_set *move_set);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_lvl_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_lvl_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_LVL_CLI_H__ */

/** @} */
