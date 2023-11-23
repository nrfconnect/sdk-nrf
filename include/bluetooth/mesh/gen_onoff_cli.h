/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_onoff_cli Generic OnOff Client model
 * @{
 * @brief API for the Generic OnOff Client model.
 */

#ifndef BT_MESH_GEN_ONOFF_CLI_H__
#define BT_MESH_GEN_ONOFF_CLI_H__

#include <bluetooth/mesh/gen_onoff.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_onoff_cli;

/** @def BT_MESH_ONOFF_CLI_INIT
 *
 * @brief Initialization parameters for a @ref bt_mesh_onoff_cli instance.
 *
 * @param[in] _status_handler Optional status message handler.
 */
#define BT_MESH_ONOFF_CLI_INIT(_status_handler)                                \
	{                                                                      \
		.status_handler = _status_handler,                             \
	}

/** @def BT_MESH_MODEL_ONOFF_CLI
 *
 * @brief Generic OnOff Client model composition data entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_onoff_cli instance.
 */
#define BT_MESH_MODEL_ONOFF_CLI(_cli)                                          \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_ONOFF_CLI,                       \
			 _bt_mesh_onoff_cli_op, &(_cli)->pub,                  \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_onoff_cli,     \
						 _cli),                        \
			 &_bt_mesh_onoff_cli_cb)

/**
 * Generic OnOff Client structure.
 *
 * Should be initialized with the @ref BT_MESH_ONOFF_CLI_INIT macro.
 */
struct bt_mesh_onoff_cli {
	/** @brief OnOff status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the incoming message.
	 * @param[in] status OnOff Status of the Generic OnOff Server that
	 * published the message.
	 */
	void (*const status_handler)(struct bt_mesh_onoff_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     const struct bt_mesh_onoff_status *status);
	/** Current Transaction ID. */
	uint8_t tid;
	/** Response context for tracking acknowledged messages. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(BT_MESH_ONOFF_OP_SET,
					       BT_MESH_ONOFF_MSG_MAXLEN_SET)];
	/** Access model pointer. */
	const struct bt_mesh_model *model;
};

/** @brief Get the status of the bound srv.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_onoff_cli::status_handler callback.
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
int bt_mesh_onoff_cli_get(struct bt_mesh_onoff_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_onoff_status *rsp);

/** @brief Set the OnOff state in the srv.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_onoff_cli::status_handler callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set New OnOff parameters to set. @p set::transition can either
 * point to a transition structure, or be left to NULL to use the default
 * transition parameters on the server.
 * @param[out] rsp Status response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @p rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_onoff_cli_set(struct bt_mesh_onoff_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_onoff_set *set,
			  struct bt_mesh_onoff_status *rsp);

/** @brief Set the OnOff state in the srv without requesting a response.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set New OnOff parameters to set. @p set::transition can either
 * point to a transition structure, or be left to NULL to use the default
 * transition parameters on the server.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_onoff_cli_set_unack(struct bt_mesh_onoff_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_onoff_set *set);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_onoff_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_onoff_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_ONOFF_CLI_H__ */

/** @} */
