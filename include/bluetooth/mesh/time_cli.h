/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_time_cli Time Client model
 * @{
 * @brief API for the Time Client model.
 */

#ifndef BT_MESH_TIME_CLI_H__
#define BT_MESH_TIME_CLI_H__

#include <bluetooth/mesh/time.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_time_cli;

/** @def BT_MESH_TIME_CLI_INIT
 *
 * @brief Initialization parameters for a @ref bt_mesh_time_cli instance.
 *
 * @sa bt_mesh_time_cli_handlers
 *
 * @param[in] _handlers Optional message handler structure.
 */
#define BT_MESH_TIME_CLI_INIT(_handlers)                                       \
	{                                                                      \
		.handlers = _handlers,                                         \
		.pub = {.msg = NET_BUF_SIMPLE(BT_MESH_MODEL_BUF_LEN(           \
				BT_MESH_TIME_OP_TIME_SET,                      \
				BT_MESH_TIME_MSG_LEN_TIME_SET)) }              \
	}

/** @def BT_MESH_MODEL_TIME_CLI
 *
 * @brief Time Client model composition data entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_time_cli instance.
 */
#define BT_MESH_MODEL_TIME_CLI(_cli)                                           \
	BT_MESH_MODEL_CB(                                                      \
		BT_MESH_MODEL_ID_TIME_CLI, _bt_mesh_time_cli_op, &(_cli)->pub, \
		BT_MESH_MODEL_USER_DATA(struct bt_mesh_time_cli, _cli),        \
		&_bt_mesh_time_cli_cb)

/** Time Client handler functions. */
struct bt_mesh_time_cli_handlers {
	/** @brief Time status message handler.
	 *
	 * Called when the client receives a Time Status message, either as a
	 * result of calling @ref bt_mesh_time_cli_time_get,
	 * @ref bt_mesh_time_cli_time_set, or as an unsolicited message.
	 *
	 * @note This handler is optional.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the incoming message.
	 * @param[in] status Time Status contained in the message.
	 */
	void (*const time_status)(struct bt_mesh_time_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  const struct bt_mesh_time_status *status);

	/** @brief Time Zone status message handler.
	 *
	 * Called when the client receives a Time Zone Status message, either
	 * as a result of calling @ref bt_mesh_time_cli_zone_get,
	 * @ref bt_mesh_time_cli_zone_set, or as an unsolicited message.
	 *
	 * @note This handler is optional.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the incoming message.
	 * @param[in] status Time Zone Status contained in the message.
	 */
	void (*const time_zone_status)(
		struct bt_mesh_time_cli *cli, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_time_zone_status *status);

	/** @brief TAI-UTC Delta status message handler.
	 *
	 * Called when the client receives a TAI-UTC Delta Status message, either
	 * as a result of calling @ref bt_mesh_time_cli_tai_utc_delta_get,
	 * @ref bt_mesh_time_cli_tai_utc_delta_set, or as an unsolicited message.
	 *
	 * @note This handler is optional.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the incoming message.
	 * @param[in] status TAI-UTC Delta Status contained in the message.
	 */
	void (*const tai_utc_delta_status)(
		struct bt_mesh_time_cli *cli, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_time_tai_utc_delta_status *status);

	/** @brief Time Role status message handler.
	 *
	 * Called when the client receives a Time Role Status message, either
	 * as a result of calling @ref bt_mesh_time_cli_role_get,
	 * @ref bt_mesh_time_cli_role_set, or as an unsolicited message.
	 *
	 * @note This handler is optional.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the incoming message.
	 * @param[in] time_role Time Role of the server.
	 */
	void (*const time_role_status)(struct bt_mesh_time_cli *cli,
				       struct bt_mesh_msg_ctx *ctx,
				       enum bt_mesh_time_role time_role);
};

/**
 * Time Client instance. Should be initialized with
 * @ref BT_MESH_TIME_CLI_INIT.
 */
struct bt_mesh_time_cli {
	/** Model entry. */
	const struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/** Acknowledged message tracking. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
	/** Collection of handler callbacks.
	*
	* @note Must point to memory that remains valid.
	*/
	const struct bt_mesh_time_cli_handlers *handlers;
};

/** @brief Get the current Time Status of a Time Server.
 *
 * @param[in]  cli Client model to send on.
 * @param[in]  ctx Message context, or NULL to use the configured publish
 *                 parameters.
 * @param[out] rsp Status response buffer, returning the Server's current time
 *                 Status, or NULL to keep from blocking.
 *
 * @retval 0              Successfully sent the message and populated
 *                        the @p rsp buffer.
 * @retval -EALREADY      A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing
 *                        is not configured.
 * @retval -EAGAIN        The device has not been provisioned.
 * @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_time_cli_time_get(struct bt_mesh_time_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      struct bt_mesh_time_status *rsp);

/** @brief Set the Time Status of a Time Server.
 *
 * @param[in]  cli Client model to send on.
 * @param[in]  ctx Message context, or NULL to use the configured publish
 *                 parameters.
 * @param[in]  set Time Status parameter to set.
 * @param[out] rsp Status response buffer, returning the Server's current time
 *                 Status, or NULL to keep from blocking.
 *
 * @retval 0              Successfully sent the message and populated
 *                        the @p rsp buffer.
 * @retval -EALREADY      A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing
 *                        is not configured.
 * @retval -EAGAIN        The device has not been provisioned.
 * @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_time_cli_time_set(struct bt_mesh_time_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_time_status *set,
			      struct bt_mesh_time_status *rsp);

/** @brief Get the Time Zone status of a Time Server.
 *
 * @param[in]  cli Client model to send on.
 * @param[in]  ctx Message context, or NULL to use the configured publish
 *                 parameters.
 * @param[out] rsp Status response buffer, returning the Server's current
 *                 Time Zone Offset new state, or NULL to keep from blocking.
 *
 * @retval 0              Successfully sent the message and populated
 *                        the @p rsp buffer.
 * @retval -EALREADY      A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing
 *                        is not configured.
 * @retval -EAGAIN        The device has not been provisioned.
 * @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_time_cli_zone_get(struct bt_mesh_time_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      struct bt_mesh_time_zone_status *rsp);

/** @brief Schedule a Time Zone change for the Time Server.
 *
 * Used to set the point in TAI a Time Server is to update its
 * Time Zone Offset, and what this new offset should be
 *
 * @param[in]  cli Client model to send on.
 * @param[in]  ctx Message context, or NULL to use the configured publish
 *                 parameters.
 * @param[in]  set Time Zone Change parameters to set.
 * @param[out] rsp Status response buffer, returning the Server's current time
 *                 Status, or NULL to keep from blocking.
 *
 * @retval 0              Successfully sent the message and populated
 *                        the @p rsp buffer.
 * @retval -EALREADY      A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing
 *                        is not configured.
 * @retval -EAGAIN        The device has not been provisioned.
 * @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_time_cli_zone_set(struct bt_mesh_time_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      const struct bt_mesh_time_zone_change *set,
			      struct bt_mesh_time_zone_status *rsp);

/** @brief Get the UTC Delta status of a Time Server.
 *
 * @param[in]  cli Client model to send on.
 * @param[in]  ctx Message context, or NULL to use the configured publish
 *                 parameters.
 * @param[out] rsp Status response buffer, returning the Server's TAI-UTC
 *                 Delta new state, or NULL to keep from blocking.
 *
 * @retval 0              Successfully sent the message and populated
 *                        the @p rsp buffer.
 * @retval -EALREADY      A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing
 *                        is not configured.
 * @retval -EAGAIN        The device has not been provisioned.
 * @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_time_cli_tai_utc_delta_get(
	struct bt_mesh_time_cli *cli, struct bt_mesh_msg_ctx *ctx,
	struct bt_mesh_time_tai_utc_delta_status *rsp);

/** @brief Schedule a UTC Delta change for the Timer Server.
 *
 * Used to set the point in TAI a Time Server is to update its
 * TAI-UTC Delta, and what this new offset should be
 *
 * @param[in]  cli Client model to send on.
 * @param[in]  ctx Message context, or NULL to use the configured publish
 *                 parameters.
 * @param[in]  set TAI-UTC Delta Change parameters to set.
 * @param[out] rsp Status response buffer, returning the Server's current time
 *                 Status, or NULL to keep from blocking.
 *
 * @retval 0              Successfully sent the message and populated
 *                        the @p rsp buffer.
 * @retval -EALREADY      A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing
 *                        is not configured.
 * @retval -EAGAIN        The device has not been provisioned.
 * @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_time_cli_tai_utc_delta_set(
	struct bt_mesh_time_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_time_tai_utc_change *set,
	struct bt_mesh_time_tai_utc_delta_status *rsp);

/** @brief Get the Time Role state of a Time Server.
 *
 * @param[in]  cli Client model to send on.
 * @param[in]  ctx Message context, or NULL to use the configured publish
 *                 parameters.
 * @param[out] rsp Status response buffer, returning the Server's current Time
 *                 Role state, or NULL to keep from blocking.
 *
 * @retval 0              Successfully sent the message and populated
 *                        the @p rsp buffer.
 * @retval -EALREADY      A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing
 *                        is not configured.
 * @retval -EAGAIN        The device has not been provisioned.
 * @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_time_cli_role_get(struct bt_mesh_time_cli *cli,
			      struct bt_mesh_msg_ctx *ctx, uint8_t *rsp);

/** @brief Set the Time Role state of a Time Server.
 *
 * @param[in]  cli Client model to send on.
 * @param[in]  ctx Message context, or NULL to use the configured publish
 *                 parameters.
 * @param[in]  set Time Role to set.
 * @param[out] rsp Status response buffer, returning the Server's current time
 *                 Status, or NULL to keep from blocking.
 *
 * @retval 0              Successfully sent the message and populated
 *                        the @p rsp buffer.
 * @retval -EALREADY      A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing
 *                        is not configured.
 * @retval -EAGAIN        The device has not been provisioned.
 * @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_time_cli_role_set(struct bt_mesh_time_cli *cli,
			      struct bt_mesh_msg_ctx *ctx, const uint8_t *set,
			      uint8_t *rsp);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_time_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_time_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_TIME_CLI_H__ */

/** @} */
