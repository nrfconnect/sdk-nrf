/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 *
 * @file
 * @defgroup bt_mesh_dtt_cli Generic Default Transition Time Client API
 * @{
 * @brief API for the Generic Default Transition Time (DTT) Client.
 */

#ifndef BT_MESH_GEN_DTT_CLI_H__
#define BT_MESH_GEN_DTT_CLI_H__

#include <bluetooth/mesh/gen_dtt.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_dtt_cli;

/** @def BT_MESH_DTT_CLI_INIT
 *
 * @brief Initialization parameters for the @ref bt_mesh_dtt_cli.
 *
 * @param[in] _status_handler Optional status message handler.
 * @sa bt_mesh_dtt_cli::status_handler
 */
#define BT_MESH_DTT_CLI_INIT(_status_handler)                                  \
	{                                                                      \
		.status_handler = _status_handler,                             \
	}

/** @def BT_MESH_MODEL_DTT_CLI
 *
 * @brief Generic DTT Client model composition data entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_dtt_cli instance.
 */
#define BT_MESH_MODEL_DTT_CLI(_cli)                                            \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_DEF_TRANS_TIME_CLI,              \
			 _bt_mesh_dtt_cli_op, &(_cli)->pub,                    \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_dtt_cli,       \
						 _cli),                        \
			 &_bt_mesh_dtt_cli_cb)

/**
 * Generic DTT client structure.
 *
 * Should be initialized using the @ref BT_MESH_DTT_CLI_INIT macro.
 */
struct bt_mesh_dtt_cli {
	/** @brief Default Transition Time status message handler.
	 *
	 * @param[in] cli Client that received the message.
	 * @param[in] ctx Message context.
	 * @param[in] transition_time Transition time presented in the message,
	 *                            in milliseconds.
	 */
	void (*const status_handler)(struct bt_mesh_dtt_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     int32_t transition_time);

	/** Response context for tracking acknowledged messages. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
	/** Model publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(BT_MESH_DTT_OP_SET,
					       BT_MESH_DTT_MSG_LEN_SET)];
	/** Composition data model entry pointer. */
	const struct bt_mesh_model *model;
};

/** @brief Get the Default Transition Time of the server.
 *
 * This call is blocking if the @p rsp_transition_time buffer is non-NULL.
 * Otherwise, this function will return, and the response will be passed to the
 * @ref bt_mesh_dtt_cli::status_handler callback.
 *
 * @param[in] cli Client making the request.
 * @param[in] ctx Message context to use for sending, or NULL to publish with
 * the configured parameters.
 * @param[out] rsp_transition_time Pointer to a response buffer, or NULL to
 * keep from blocking. The response denotes the configured transition time in
 * milliseconds. Can be @em SYS_FOREVER_MS if the current state is unknown or
 * too large to represent.
 *
 * @retval 0 Successfully retrieved the status of the bound srv.
 * @retval -EALREADY A blocking operation is already in progress in this model.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.

 */
int bt_mesh_dtt_get(struct bt_mesh_dtt_cli *cli, struct bt_mesh_msg_ctx *ctx,
		    int32_t *rsp_transition_time);

/** @brief Set the Default Transition Time of the server.
 *
 * This call is blocking if the @p rsp_transition_time buffer is non-NULL.
 * Otherwise, this function will return, and the response will be passed to the
 * @ref bt_mesh_dtt_cli::status_handler callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context to use for sending, or NULL to publish with
 * the configured parameters.
 * @param[in] transition_time Transition time to set (in milliseconds). Must be
 * less than @ref BT_MESH_MODEL_TRANSITION_TIME_MAX_MS.
 * @param[out] rsp_transition_time Pointer to a response buffer, or NULL to
 * keep from blocking. The response denotes the configured transition time in
 * milliseconds. Can be @em SYS_FOREVER_MS if the current state is unknown or
 * too large to represent.
 *
 * @retval 0 Successfully sent the message and populated the
 * @p rsp_transition_time buffer.
 * @retval -EINVAL The given transition time is invalid.
 * @retval -EALREADY A blocking operation is already in progress in this model.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_dtt_set(struct bt_mesh_dtt_cli *cli, struct bt_mesh_msg_ctx *ctx,
		    uint32_t transition_time, int32_t *rsp_transition_time);

/** @brief Set the Default Transition Time of the server without requesting a
 * response.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context to use for sending, or NULL to publish with
 * the configured parameters.
 * @param[in] transition_time Transition time to set (in milliseconds). Must be
 * less than @ref BT_MESH_MODEL_TRANSITION_TIME_MAX_MS.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EINVAL The given transition time is invalid.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_dtt_set_unack(struct bt_mesh_dtt_cli *cli,
			  struct bt_mesh_msg_ctx *ctx, uint32_t transition_time);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_dtt_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_dtt_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_DTT_CLI_H__ */

/** @} */
