/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @defgroup bt_mesh_light_xyl_cli Light xyL Client model
 * @{
 * @brief API for the Light xyL Client model.
 */

#ifndef BT_MESH_LIGHT_XYL_CLI_H__
#define BT_MESH_LIGHT_XYL_CLI_H__

#include <bluetooth/mesh/light_xyl.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_light_xyl_cli;

/** @def BT_MESH_MODEL_LIGHT_XYL_CLI
 *
 * @brief Light XYL Client model composition data entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_light_xyl_cli instance.
 */
#define BT_MESH_MODEL_LIGHT_XYL_CLI(_cli)                                      \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_LIGHT_XYL_CLI,                       \
			 _bt_mesh_light_xyl_cli_op, &(_cli)->pub,              \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_light_xyl_cli, \
						 _cli),                        \
			 &_bt_mesh_light_xyl_cli_cb)

/** Light xyL Client state access handlers. */
struct bt_mesh_light_xyl_cli_handlers {
	/** @brief xyL status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] status xyL status contained in the
	 * message.
	 */
	void (*const xyl_status)(struct bt_mesh_light_xyl_cli *cli,
				 struct bt_mesh_msg_ctx *ctx,
				 const struct bt_mesh_light_xyl_status *status);

	/** @brief xyL target status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] status Target xyL status contained in the
	 * message.
	 */
	void (*const target_status)(
		struct bt_mesh_light_xyl_cli *cli, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_light_xyl_status *status);

	/** @brief xyL Range status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] status xyL Range state of the server.
	 */
	void (*const range_status)(
		struct bt_mesh_light_xyl_cli *cli, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_light_xyl_range_status *status);

	/** @brief Default parameter status message handler.
	 *
	 * @param[in] cli Client that received the status message.
	 * @param[in] ctx Context of the message.
	 * @param[in] status Default xyL value of the server.
	 */
	void (*const default_status)(struct bt_mesh_light_xyl_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     const struct bt_mesh_light_xyl *status);
};

/** Light xyL Client instance. */
struct bt_mesh_light_xyl_cli {
	/** Model entry. */
	const struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication message */
	struct net_buf_simple pub_msg;
	/* Publication message buffer. */
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(BT_MESH_LIGHT_XYL_OP_SET,
					  BT_MESH_LIGHT_XYL_MSG_MAXLEN_SET)];

	/** Acknowledged message tracking. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
	/** Current transaction ID. */
	uint8_t tid;
	/** Handler function structure. */
	const struct bt_mesh_light_xyl_cli_handlers *handlers;
};

/** @brief Get the Light xyL state of the bound server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] rsp Status response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @c rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_xyl_get(struct bt_mesh_light_xyl_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_light_xyl_status *rsp);

/** @brief Set the xyL state of the server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set xyL state to set.
 * @param[in] rsp Response status buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @c rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_xyl_set(struct bt_mesh_light_xyl_cli *cli,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_light_xyl_set_params *set,
			  struct bt_mesh_light_xyl_status *rsp);

/** @brief Set the xyL state of the server without requesting a response.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set xyL state to set.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_light_xyl_set_unack(struct bt_mesh_light_xyl_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_light_xyl_set_params *set);

/** @brief Get the Light xyL target state of the bound server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] rsp Status response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @c rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_xyl_target_get(struct bt_mesh_light_xyl_cli *cli,
				 struct bt_mesh_msg_ctx *ctx,
				 struct bt_mesh_light_xyl_status *rsp);

/** @brief Get the Light xyL default state of the bound server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] rsp Status response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @c rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_xyl_default_get(struct bt_mesh_light_xyl_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  struct bt_mesh_light_xyl *rsp);

/** @brief Set the default xyL value of the server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set Default xyL value to set.
 * @param[in] rsp Response status buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @c rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_xyl_default_set(struct bt_mesh_light_xyl_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  const struct bt_mesh_light_xyl *set,
				  struct bt_mesh_light_xyl *rsp);

/** @brief Set the default xyL value of the server without requesting a
 * response.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set Default xyL value to set.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_light_xyl_default_set_unack(struct bt_mesh_light_xyl_cli *cli,
					struct bt_mesh_msg_ctx *ctx,
					const struct bt_mesh_light_xyl *set);

/** @brief Get the Light xyL state of the bound server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] rsp Status response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @c rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_light_xyl_range_get(struct bt_mesh_light_xyl_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				struct bt_mesh_light_xyl_range_status *rsp);

/** @brief Set the xyL Range of the server.
 *
 * This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 * function will return.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] set Range to set.
 * @param[in] rsp Response status buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @c rsp buffer.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 * @retval -EFAULT Invalid input range.
 */

int bt_mesh_light_xyl_range_set(struct bt_mesh_light_xyl_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_light_xy_range *set,
				struct bt_mesh_light_xyl_range_status *rsp);

/** @brief Set the xyL Range of the server without requesting a
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
 * @retval -EFAULT Invalid input range.
 */
int bt_mesh_light_xyl_range_set_unack(struct bt_mesh_light_xyl_cli *cli,
				      struct bt_mesh_msg_ctx *ctx,
				      const struct bt_mesh_light_xy_range *set);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_light_xyl_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_light_xyl_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_XYL_CLI_H__ */

/** @} */
