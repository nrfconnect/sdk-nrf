/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 *  @file
 *  @defgroup bt_mesh_light_hsl_cli Light HSL Client model
 *  @{
 *  @brief API for the Light HSL Client model.
 */

#ifndef BT_MESH_LIGHT_HSL_CLI_H__
#define BT_MESH_LIGHT_HSL_CLI_H__

#include <bluetooth/mesh/light_hsl.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_light_hsl_cli;

/** @def BT_MESH_MODEL_LIGHT_HSL_CLI
 *
 *  @brief Light HSL Client model composition data entry.
 *
 *  @param[in] _cli Pointer to a @ref bt_mesh_light_hsl_cli instance.
 */
#define BT_MESH_MODEL_LIGHT_HSL_CLI(_cli)                                      \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_HSL_CLI,                       \
			 _bt_mesh_light_hsl_cli_op, &(_cli)->pub,              \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_light_hsl_cli, \
						 _cli),                        \
			 &_bt_mesh_light_hsl_cli_cb)

/** Light HSL Client state access handlers. */
struct bt_mesh_light_hsl_cli_handlers {
	/** @brief HSL status message handler.
	 *
	 *  @param[in] cli    Client that received the status message.
	 *  @param[in] ctx    Context of the message.
	 *  @param[in] status HSL status contained in the
	 *                    message.
	 */
	void (*const status)(struct bt_mesh_light_hsl_cli *cli,
			     struct bt_mesh_msg_ctx *ctx,
			     const struct bt_mesh_light_hsl_status *status);

	/** @brief HSL target status message handler.
	 *
	 *  @param[in] cli    Client that received the status message.
	 *  @param[in] ctx    Context of the message.
	 *  @param[in] status HSL target status contained in the
	 *                    message.
	 */
	void (*const target_status)(
		struct bt_mesh_light_hsl_cli *cli, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_light_hsl_status *status);

	/** @brief Default parameter status message handler.
	 *
	 *  @param[in] cli    Client that received the status message.
	 *  @param[in] ctx    Context of the message.
	 *  @param[in] status Default HSL value of the server.
	 */
	void (*const default_status)(
		struct bt_mesh_light_hsl_cli *cli, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_light_hsl *status);

	/** @brief Range status message handler.
	 *
	 *  @param[in] cli    Client that received the status message.
	 *  @param[in] ctx    Context of the message.
	 *  @param[in] status HSL Range state of the server.
	 */
	void (*const range_status)(
		struct bt_mesh_light_hsl_cli *cli, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_light_hsl_range_status *status);

	/** @brief Hue status message handler.
	 *
	 *  @param[in] cli    Client that received the status message.
	 *  @param[in] ctx    Context of the message.
	 *  @param[in] status Hue status contained in the
	 *                    message.
	 */
	void (*const hue_status)(
		struct bt_mesh_light_hsl_cli *cli, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_light_hue_status *status);

	/** @brief Saturation status message handler.
	 *
	 *  @param[in] cli    Client that received the status message.
	 *  @param[in] ctx    Context of the message.
	 *  @param[in] status Saturation status contained in the
	 *                    message.
	 */
	void (*const saturation_status)(
		struct bt_mesh_light_hsl_cli *cli, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_light_sat_status *status);
};

/**
 * Light HSL Client instance.
 */
struct bt_mesh_light_hsl_cli {
	/** Handler function structure. */
	const struct bt_mesh_light_hsl_cli_handlers *handlers;

	/** Model entry. */
	const struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/** Publication buffer */
	struct net_buf_simple buf;
	/** Publication buffer data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_LIGHT_HSL_OP_SET, BT_MESH_LIGHT_HSL_MSG_MAXLEN_SET)];
	/** Acknowledged message tracking. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
	/** Current transaction ID. */
	uint8_t tid;
};

/** @brief Get the Light HSL state of the bound server.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return.
 *
 *  @param[in] cli Client model to send on.
 *  @param[in] ctx Message context, or NULL to use the configured publish
 *                 parameters.
 *  @param[in] rsp Status response buffer, or NULL to keep from blocking.
 *
 *  @retval 0 Successfully sent the message and populated the @c rsp buffer.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_hsl_get(struct bt_mesh_light_hsl_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_light_hsl_status *rsp);

/** @brief Set the Light HSL state of the server.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return.
 *
 *  @param[in] cli Client model to send on.
 *  @param[in] ctx Message context, or NULL to use the configured publish
 *                 parameters.
 *  @param[in] set HSL state to set.
 *  @param[in] rsp Response status buffer, or NULL to keep from blocking.
 *
 *  @retval 0 Successfully sent the message and populated the @c rsp buffer.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_hsl_set(struct bt_mesh_light_hsl_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_light_hsl_params *set,
			  struct bt_mesh_light_hsl_status *rsp);

/** @brief Set the Light HSL state of the server without requesting a response.
 *
 *  @param[in] cli Client model to send on.
 *  @param[in] ctx Message context, or NULL to use the configured publish
 *                 parameters.
 *  @param[in] set HSL state to set.
 *
 *  @retval 0 Successfully sent the message.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_light_hsl_set_unack(struct bt_mesh_light_hsl_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_light_hsl_params *set);

/** @brief Get the Light HSL target state of the bound server.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return.
 *
 *  @param[in] cli Client model to send on.
 *  @param[in] ctx Message context, or NULL to use the configured publish
 *                 parameters.
 *  @param[in] rsp Status response buffer, or NULL to keep from blocking.
 *
 *  @retval 0 Successfully sent the message and populated the @c rsp buffer.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_hsl_target_get(struct bt_mesh_light_hsl_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_light_hsl_status *rsp);

/** @brief Get the default HSL value of the bound server.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return.
 *
 *  @param[in] cli Client model to send on.
 *  @param[in] ctx Message context, or NULL to use the configured publish
 *                 parameters.
 *  @param[in] rsp Status response buffer, or NULL to keep from blocking.
 *
 *  @retval 0 Successfully sent the message and populated the @c rsp buffer.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_hsl_default_get(struct bt_mesh_light_hsl_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  struct bt_mesh_light_hsl *rsp);

/** @brief Set the default HSL value of the server.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return.
 *
 *  @param[in] cli Client model to send on.
 *  @param[in] ctx Message context, or NULL to use the configured publish
 *                 parameters.
 *  @param[in] set Default HSL value to set.
 *  @param[in] rsp Response status buffer, or NULL to keep from blocking.
 *
 *  @retval 0 Successfully sent the message and populated the @c rsp buffer.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_hsl_default_set(struct bt_mesh_light_hsl_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  const struct bt_mesh_light_hsl *set,
				  struct bt_mesh_light_hsl *rsp);

/** @brief Set the default HSL value of the server without requesting a
 * response.
 *
 *  @param[in] cli Client model to send on.
 *  @param[in] ctx Message context, or NULL to use the configured publish
 *                 parameters.
 *  @param[in] set Default HSL value to set.
 *
 *  @retval 0 Successfully sent the message.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_light_hsl_default_set_unack(
	struct bt_mesh_light_hsl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_light_hsl *set);

/** @brief Get the Light HSL Range state of the bound server.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return.
 *
 *  @param[in] cli Client model to send on.
 *  @param[in] ctx Message context, or NULL to use the configured publish
 *                 parameters.
 *  @param[in] rsp Status response buffer, or NULL to keep from blocking.
 *
 *  @retval 0 Successfully sent the message and populated the @c rsp buffer.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_hsl_range_get(
	struct bt_mesh_light_hsl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	struct bt_mesh_light_hsl_range_status *rsp);

/** @brief Set the Light HSL Range of the server.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return.
 *
 *  @param[in] cli Client model to send on.
 *  @param[in] ctx Message context, or NULL to use the configured publish
 *                 parameters.
 *  @param[in] set Range to set.
 *  @param[in] rsp Response status buffer, or NULL to keep from blocking.
 *
 *  @retval 0 Successfully sent the message and populated the @c rsp buffer.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_hsl_range_set(
	struct bt_mesh_light_hsl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_light_hue_sat_range *set,
	struct bt_mesh_light_hsl_range_status *rsp);

/** @brief Set the Light HSL Range of the server without requesting a
 * response.
 *
 *  @param[in] cli Client model to send on.
 *  @param[in] ctx Message context, or NULL to use the configured publish
 *                 parameters.
 *  @param[in] set Range state to set.
 *
 *  @retval 0 Successfully sent the message.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_light_hsl_range_set_unack(
	struct bt_mesh_light_hsl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_light_hue_sat_range *set);

/** @brief Get the Light Hue state of the bound server.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return.
 *
 *  @param[in] cli Client model to send on.
 *  @param[in] ctx Message context, or NULL to use the configured publish
 *                 parameters.
 *  @param[in] rsp Status response buffer, or NULL to keep from blocking.
 *
 *  @retval 0 Successfully sent the message and populated the @c rsp buffer.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_hue_get(struct bt_mesh_light_hsl_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_light_hue_status *rsp);

/** @brief Set the Light Hue state of the server.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return.
 *
 *  @param[in] cli Client model to send on.
 *  @param[in] ctx Message context, or NULL to use the configured publish
 *                 parameters.
 *  @param[in] set Light Hue state to set.
 *  @param[in] rsp Response status buffer, or NULL to keep from blocking.
 *
 *  @retval 0 Successfully sent the message and populated the @c rsp buffer.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_hue_set(struct bt_mesh_light_hsl_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_light_hue *set,
			  struct bt_mesh_light_hue_status *rsp);

/** @brief Set the Light Hue state of the server without requesting a response.
 *
 *  @param[in] cli Client model to send on.
 *  @param[in] ctx Message context, or NULL to use the configured publish
 *                 parameters.
 *  @param[in] set Light Hue state to set.
 *
 *  @retval 0 Successfully sent the message.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_light_hue_set_unack(struct bt_mesh_light_hsl_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_light_hue *set);

/** @brief Get the Light Saturation state of the bound server.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return.
 *
 *  @param[in] cli Client model to send on.
 *  @param[in] ctx Message context, or NULL to use the configured publish
 *                 parameters.
 *  @param[in] rsp Status response buffer, or NULL to keep from blocking.
 *
 *  @retval 0 Successfully sent the message and populated the @c rsp buffer.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_saturation_get(struct bt_mesh_light_hsl_cli *cli,
				 struct bt_mesh_msg_ctx *ctx,
				 struct bt_mesh_light_sat_status *rsp);

/** @brief Set the Light Saturation state of the server.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return.
 *
 *  @param[in] cli Client model to send on.
 *  @param[in] ctx Message context, or NULL to use the configured publish
 *                 parameters.
 *  @param[in] set Light Saturation state to set.
 *  @param[in] rsp Response status buffer, or NULL to keep from blocking.
 *
 *  @retval 0 Successfully sent the message and populated the @c rsp buffer.
 *  @retval -EALREADY A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 *  @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_saturation_set(struct bt_mesh_light_hsl_cli *cli,
				 struct bt_mesh_msg_ctx *ctx,
				 const struct bt_mesh_light_sat *set,
				 struct bt_mesh_light_sat_status *rsp);

/** @brief Set the Light Saturation state of the server without requesting a
 *         response.
 *
 *  @param[in] cli Client model to send on.
 *  @param[in] ctx Message context, or NULL to use the configured publish
 *                 parameters.
 *  @param[in] set Light Saturation state to set.
 *
 *  @retval 0 Successfully sent the message.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_light_saturation_set_unack(struct bt_mesh_light_hsl_cli *cli,
				       struct bt_mesh_msg_ctx *ctx,
				       const struct bt_mesh_light_sat *set);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_light_hsl_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_light_hsl_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_HSL_CLI_H__ */

/** @} */
