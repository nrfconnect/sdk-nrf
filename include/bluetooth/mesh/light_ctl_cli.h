/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_light_ctl_cli Light CTL Client model
 * @{
 * @brief API for the Light CTL Client model.
 */

#ifndef BT_MESH_LIGHT_CTL_CLI_H__
#define BT_MESH_LIGHT_CTL_CLI_H__

#include <bluetooth/mesh/light_ctl.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_light_ctl_cli;

/** @def BT_MESH_LIGHT_CTL_CLI_INIT
 *
 * @brief Initialization parameters for a @ref bt_mesh_light_ctl_cli instance.
 */
#define BT_MESH_LIGHT_CTL_CLI_INIT(_handlers)                                  \
	{                                                                      \
		.handlers = _handlers,                                         \
	}

/** @def BT_MESH_MODEL_LIGHT_CTL_CLI
 *
 * @brief Light CTL Client model composition data entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_light_ctl_cli instance.
 */
#define BT_MESH_MODEL_LIGHT_CTL_CLI(_cli)                                      \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_CTL_CLI,                       \
			 _bt_mesh_light_ctl_cli_op, &(_cli)->pub,              \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_light_ctl_cli, \
						 _cli),                        \
			 &_bt_mesh_light_ctl_cli_cb)

/** Light CTL Client state access handlers. */
struct bt_mesh_light_ctl_cli_handlers {
	/** @brief CTL status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] status CTL status contained in the
	 * message.
	 */
	void (*const ctl_status)(struct bt_mesh_light_ctl_cli *cli,
				 struct bt_mesh_msg_ctx *ctx,
				 const struct bt_mesh_light_ctl_status *status);

	/** @brief CTL Temperature status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] status CTL Temperature status contained in the
	 * message.
	 */
	void (*const temp_status)(
		struct bt_mesh_light_ctl_cli *cli, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_light_temp_status *status);

	/** @brief Default parameter status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] status Default CTL value of the server.
	 */
	void (*const default_status)(
		struct bt_mesh_light_ctl_cli *cli, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_light_ctl *status);

	/** @brief CTL Range status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] status CTL Range state of the server.
	 */
	void (*const temp_range_status)(
		struct bt_mesh_light_ctl_cli *cli, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_light_temp_range_status *status);
};

/**
 * Light CTL Client instance. Should be initialized with
 * @ref BT_MESH_LIGHT_CTL_CLI_INIT.
 */
struct bt_mesh_light_ctl_cli {
	/** Model entry. */
	const struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_LIGHT_CTL_SET, BT_MESH_LIGHT_CTL_MSG_MAXLEN_SET)];
	/** Acknowledged message tracking. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
	/** Current transaction ID. */
	uint8_t tid;
	/** Handler function structure. */
	const struct bt_mesh_light_ctl_cli_handlers *handlers;
};

/** @brief Get the Light CTL state of the bound server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return.
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
int bt_mesh_light_ctl_get(struct bt_mesh_light_ctl_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_light_ctl_status *rsp);

/** @brief Set the CTL state of the server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set CTL state to set.
 * @param[out] rsp Response status buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @c rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_ctl_set(struct bt_mesh_light_ctl_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_light_ctl_set *set,
			  struct bt_mesh_light_ctl_status *rsp);

/** @brief Set the CTL state of the server without requesting a response.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set CTL state to set.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_light_ctl_set_unack(struct bt_mesh_light_ctl_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_light_ctl_set *set);

/** @brief Get the Light CTL Temperature state of the bound server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return.
 *
 * @note In order to use this call, the user must ensure that the client
 * instance is set to publish messages to the Light Temperature associated
 * with the target Light CTL server. This server will be located in another
 * element than the Light CTL server.
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
int bt_mesh_light_temp_get(struct bt_mesh_light_ctl_cli *cli,
			   struct bt_mesh_msg_ctx *ctx,
			   struct bt_mesh_light_temp_status *rsp);

/** @brief Set the CTL Temperature state of the server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return.
 *
 * @note In order to use this call, the user must ensure that the client
 * instance is set to publish messages to the Light Temperature associated
 * with the target Light CTL server. This server will be located in another
 * element than the Light CTL server.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set CTL Temperature state to set.
 * @param[out] rsp Response status buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @c rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_temp_set(struct bt_mesh_light_ctl_cli *cli,
			   struct bt_mesh_msg_ctx *ctx,
			   const struct bt_mesh_light_temp_set *set,
			   struct bt_mesh_light_temp_status *rsp);

/** @brief Set the CTL Temperature state of the server without requesting a
 * response.
 *
 * @note In order to use this call, the user must ensure that the client
 * instance is set to publish messages to the Light Temperature associated
 * with the target Light CTL server. This server will be located in another
 * element than the Light CTL server.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set CTL Temperature state to set.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_light_temp_set_unack(
	struct bt_mesh_light_ctl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_light_temp_set *set);

/** @brief Get the default CTL value of the bound server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return.
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
int bt_mesh_light_ctl_default_get(struct bt_mesh_light_ctl_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  struct bt_mesh_light_ctl *rsp);

/** @brief Set the default CTL value of the server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set Default CTL value to set.
 * @param[out] rsp Response status buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @c rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_ctl_default_set(struct bt_mesh_light_ctl_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  const struct bt_mesh_light_ctl *set,
				  struct bt_mesh_light_ctl *rsp);

/** @brief Set the default CTL value of the server without requesting a
 * response.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set Default CTL value to set.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_light_ctl_default_set_unack(
	struct bt_mesh_light_ctl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_light_ctl *set);

/** @brief Get the Light CTL Range state of the bound server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return.
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
int bt_mesh_light_temp_range_get(
	struct bt_mesh_light_ctl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	struct bt_mesh_light_temp_range_status *rsp);

/** @brief Set the CTL Range of the server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set Range to set.
 * @param[out] rsp Response status buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @c rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_temp_range_set(
	struct bt_mesh_light_ctl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_light_temp_range *set,
	struct bt_mesh_light_temp_range_status *rsp);

/** @brief Set the CTL Range of the server without requesting a
 * response.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set Range state to set.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_light_temp_range_set_unack(
	struct bt_mesh_light_ctl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_light_temp_range *set);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_light_ctl_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_light_ctl_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_CTL_CLI_H__ */

/** @} */
