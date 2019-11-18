/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**
 * @file
 * @defgroup bt_mesh_prop_cli Generic Property Client model
 * @{
 * @brief API for the Generic Property Client model.
 */
#ifndef BT_MESH_GEN_PROP_CLI_H__
#define BT_MESH_GEN_PROP_CLI_H__

#include <bluetooth/mesh/gen_prop.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_prop_cli;

/** @def BT_MESH_PROP_CLI_INIT
 *
 * @brief Initialization parameters for the @ref bt_mesh_prop_cli.
 *
 * @param[in] _prop_list_handler Optional status message handler.
 * @sa bt_mesh_prop_cli::prop_list
 * @param[in] _prop_status_handler Optional status message handler.
 * @sa bt_mesh_prop_cli::prop_status
 */
#define BT_MESH_PROP_CLI_INIT(_prop_list_handler, _prop_status_handler)        \
	{                                                                      \
		.pub = { .msg = NET_BUF_SIMPLE(BT_MESH_MODEL_BUF_LEN(          \
				 BT_MESH_PROP_OP_ADMIN_PROP_SET,               \
				 BT_MESH_PROP_MSG_MAXLEN_ADMIN_PROP_SET)) },   \
		.prop_list = _prop_list_handler,                               \
		.prop_status = _prop_status_handler,                           \
	}

/** @def BT_MESH_MODEL_PROP_CLI
 *
 * @brief Generic Property Client model composition data entry.
 *
 * @param[in] _cli Pointer to a @ref bt_mesh_prop_cli instance.
 */
#define BT_MESH_MODEL_PROP_CLI(_cli)                                           \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_GEN_PROP_CLI, _bt_mesh_prop_cli_op,  \
			 &(_cli)->pub,                                         \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_prop_cli,      \
						 _cli),                        \
			 &_bt_mesh_prop_cli_cb)

/** List of property IDs. */
struct bt_mesh_prop_list {
	u8_t count; /**< Number of IDs in the list. */
	u16_t *ids; /**< Available Property IDs. */
};

/**
 * Generic Property Client instance.
 * Should be initialized with @ref BT_MESH_PROP_CLI_INIT.
 */
struct bt_mesh_prop_cli {
	/** Model entry. */
	struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/** Acknowledged message tracking. */
	struct bt_mesh_model_ack_ctx ack_ctx;

	/** @brief Property list message handler.
	 *
	 * @param[in] cli Client that received the message.
	 * @param[in] ctx Context of the message.
	 * @param[in] kind Kind of Property Server that sent the message.
	 * @param[in] list List of properties supported by the server.
	 */
	void (*const prop_list)(struct bt_mesh_prop_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				enum bt_mesh_prop_srv_kind kind,
				const struct bt_mesh_prop_list *list);

	/** @brief Property status message handler.
	 *
	 * @param[in] cli Client that received the message.
	 * @param[in] ctx Context of the message.
	 * @param[in] kind Kind of Property Server that sent the message.
	 * @param[in] prop Property value.
	 */
	void (*const prop_status)(struct bt_mesh_prop_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  enum bt_mesh_prop_srv_kind kind,
				  const struct bt_mesh_prop_val *prop);
};

/** @brief Get the list of properties of the bound server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_prop_cli::prop_list callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] kind Kind of Property Server to query.
 * @param[out] rsp Response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @p rsp buffer.
 * @retval -EINVAL The @p rsp::ids list was NULL.
 * @retval -ENOBUFS The client received a response, but the supplied response
 * buffer was too small to hold all the properties. All property IDs that could
 * fit in the response buffers were copied into it, and the @p rsp::count
 * field was left unchanged.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_prop_cli_props_get(struct bt_mesh_prop_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       enum bt_mesh_prop_srv_kind kind,
			       struct bt_mesh_prop_list *rsp);

/** @brief Get the value of a property in a server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_prop_cli::prop_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] kind Kind of Property Server to query.
 * @param[in] id ID of the property to get.
 * @param[out] rsp Response buffer, or NULL to keep from blocking.
 *
 * @retval 0 Successfully sent the message and populated the @p rsp buffer.
 * @retval -EINVAL The @p rsp::ids list was NULL.
 * @retval -ENOBUFS The client received a response, but the supplied response
 * buffer was too small to hold all the properties. All property IDs that could
 * fit in the response buffers were copied into it, and the @p rsp::count
 * field was left unchanged.
 * @retval -EALREADY A blocking request is already in progress.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 * @retval -ETIMEDOUT The request timed out without a response.
 */
int bt_mesh_prop_cli_prop_get(struct bt_mesh_prop_cli *cli,
			      struct bt_mesh_msg_ctx *ctx,
			      enum bt_mesh_prop_srv_kind kind, u16_t id,
			      struct bt_mesh_prop_val *rsp);

/** @brief Set a property value in a User Property Server.
 *
 * @copydetails bt_mesH_prop_cli_user_prop_set_unack
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_prop_cli::prop_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] val New property value to set. Note that the user_access mode
 * will be ignored.
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
int bt_mesh_prop_cli_user_prop_set(struct bt_mesh_prop_cli *cli,
				   struct bt_mesh_msg_ctx *ctx,
				   const struct bt_mesh_prop_val *val,
				   struct bt_mesh_prop_val *rsp);

/** @brief Set a property value in a User Property Server without requesting a
 * response.
 *
 * The User Property may only be set if the server enabled user write access to
 * it. If this is not the case, the server will only respond with the set user
 * access mode for the given property.
 *
 * @note The @p val::meta::user_access level will be ignored.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] val New property value to set. Note that the user_access mode
 * will be ignored.
 *
 * @retval 0 Successfully sent the message.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_prop_cli_user_prop_set_unack(struct bt_mesh_prop_cli *cli,
					 struct bt_mesh_msg_ctx *ctx,
					 const struct bt_mesh_prop_val *val);

/** @brief Set a property value in an Admin Property server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_prop_cli::prop_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] val New property value to set.
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
int bt_mesh_prop_cli_admin_prop_set(struct bt_mesh_prop_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    const struct bt_mesh_prop_val *val,
				    struct bt_mesh_prop_val *rsp);

/** @brief Set a property value in an Admin Property server without requesting
 * a response.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] val New property value to set.
 *
 * @retval 0 Successfully sent the message.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_prop_cli_admin_prop_set_unack(struct bt_mesh_prop_cli *cli,
					  struct bt_mesh_msg_ctx *ctx,
					  const struct bt_mesh_prop_val *val);

/** @brief Set the user access of a property in a Manufacturer Property server.
 *
 * This call is blocking if the @p rsp buffer is non-NULL. Otherwise, this
 * function will return, and the response will be passed to the
 * @ref bt_mesh_prop_cli::prop_status callback.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] val New property value to set.
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
int bt_mesh_prop_cli_mfr_prop_set(struct bt_mesh_prop_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  const struct bt_mesh_prop *prop,
				  struct bt_mesh_prop_val *rsp);

/** @brief Set the user access of a property in a Manufacturer Property server
 * without requesting a response.
 *
 * @param[in] cli Client model to send on.
 * @param[in] ctx Message context, or NULL to use the configured publish
 * parameters.
 * @param[in] val New property value to set.
 *
 * @retval 0 Successfully sent the message.
 * @retval -ENOTSUP A message context was not provided and publishing is not
 * supported.
 * @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 * not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int bt_mesh_prop_cli_mfr_prop_set_unack(struct bt_mesh_prop_cli *cli,
					struct bt_mesh_msg_ctx *ctx,
					const struct bt_mesh_prop *prop);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_prop_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_prop_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_GEN_PROP_CLI_H__ */

/** @} */
