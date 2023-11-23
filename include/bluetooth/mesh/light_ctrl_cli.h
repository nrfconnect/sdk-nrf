/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @defgroup bt_mesh_light_ctrl_cli Light Lightness Control Client
 *  @ingroup bt_mesh_light_ctrl
 *  @{
 *  @brief Light Lightness Control Client model API
 */

#ifndef BT_MESH_LIGHT_CTRL_CLI_H__
#define BT_MESH_LIGHT_CTRL_CLI_H__

#include <bluetooth/mesh/light_ctrl.h>
#include <bluetooth/mesh/model_types.h>
#include <bluetooth/mesh/gen_onoff.h>
#include <bluetooth/mesh/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_light_ctrl_cli;

/** @def BT_MESH_LIGHT_CTRL_CLI_INIT
 *
 *  @brief Initialization parameters for the @ref bt_mesh_light_ctrl_cli.
 *
 *  @param[in] _handlers Optional message handler structure.
 *
 *  @sa bt_mesh_light_ctrl_cli_handlers
 */
#define BT_MESH_LIGHT_CTRL_CLI_INIT(_handlers)                                 \
	{                                                                      \
		.handlers = _handlers,                                         \
	}

/** @def BT_MESH_MODEL_LIGHT_CTRL_CLI
 *
 *  @brief Light Lightness Control Client model composition data entry.
 *
 *  @param[in] _cli Pointer to a @ref bt_mesh_light_ctrl_cli instance.
 */
#define BT_MESH_MODEL_LIGHT_CTRL_CLI(_cli)                                    \
	BT_MESH_MODEL_CB(                                                     \
		BT_MESH_MODEL_ID_LIGHT_LC_CLI, _bt_mesh_light_ctrl_cli_op,    \
		&(_cli)->pub,                                                 \
		BT_MESH_MODEL_USER_DATA(struct bt_mesh_light_ctrl_cli, _cli), \
		&_bt_mesh_light_ctrl_cli_cb)

/** Light Control Client handlers */
struct bt_mesh_light_ctrl_cli_handlers {
	/** @brief Light LC Mode status handler.
	 *
	 *  The Light Lightness Control Server will only control its
	 *  Lightness Server when in enabled mode.
	 *
	 *  @param[in] cli     Client that received the status message.
	 *  @param[in] ctx     Context of the message.
	 *  @param[in] enabled Whether the reporting server is enabled.
	 */
	void (*mode)(struct bt_mesh_light_ctrl_cli *cli,
		     struct bt_mesh_msg_ctx *ctx, bool enabled);

	/** @brief Light LC Occupancy Mode status handler
	 *
	 *  The Occupancy Mode state controls whether the Light Lightness
	 *  Control accepts occupancy sensor messages for controlling it.
	 *
	 *  @param[in] cli     Client that received the message.
	 *  @param[in] ctx     Context of the message.
	 *  @param[in] enabled Whether the reporting server accepts occupancy
	 *                     sensor messages.
	 */
	void (*occupancy_mode)(struct bt_mesh_light_ctrl_cli *cli,
			       struct bt_mesh_msg_ctx *ctx, bool enabled);

	/** @brief Light LC OnOff status handler.
	 *
	 *  The Light OnOff state indicates whether the Light Lightness Control
	 *  Server is enabled and has turned the Lightness Server on.
	 *
	 *  @param[in] cli    Client that received the message.
	 *  @param[in] ctx    Context of the message.
	 *  @param[in] status Current OnOff state of the reporting server. The
	 *                    remaining time indicates the remaining fading
	 *                    time.
	 */
	void (*light_onoff)(struct bt_mesh_light_ctrl_cli *cli,
			    struct bt_mesh_msg_ctx *ctx,
			    const struct bt_mesh_onoff_status *status);

	/** @brief Light LC Property status handler.
	 *
	 *  The Light Lightness Control Server's properties are configuration
	 *  parameters for its behavior. All properties are represented as a
	 *  single sensor value channel.
	 *
	 *  @param[in] cli   Client that received the message.
	 *  @param[in] ctx   Context of the message.
	 *  @param[in] id    ID of the property.
	 *  @param[in] value Value of the property.
	 */
	void (*prop)(struct bt_mesh_light_ctrl_cli *cli,
		     struct bt_mesh_msg_ctx *ctx,
		     enum bt_mesh_light_ctrl_prop id,
		     const struct sensor_value *value);

	/** @brief Light LC Regulator Coefficient status handler.
	 *
	 *  The Light Lightness Control Server's Regulator Coefficients control
	 *  its Illuminance Regulator's response.
	 *
	 *  @param[in] cli   Client that received the message.
	 *  @param[in] ctx   Context of the message.
	 *  @param[in] id    ID of the property.
	 *  @param[in] value Value of the property.
	 */
	void (*coeff)(struct bt_mesh_light_ctrl_cli *cli,
		     struct bt_mesh_msg_ctx *ctx,
		     enum bt_mesh_light_ctrl_coeff id,
		     float value);
};

/** @brief Light Lightness Control Client instance.
 *
 *  Should be initialized with @ref BT_MESH_LIGHT_CTRL_CLI_INIT.
 */
struct bt_mesh_light_ctrl_cli {
	/** Composition data model entry pointer. */
	const struct bt_mesh_model *model;
	/** Publication parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[BT_MESH_MODEL_BUF_LEN(
		BT_MESH_LIGHT_CTRL_OP_PROP_SET,
		2 + CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX)];
	/** Acknowledged message tracking. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
	/** Collection of handler callbacks. */
	const struct bt_mesh_light_ctrl_cli_handlers *handlers;
	/** Current transaction ID. */
	uint8_t tid;
};

/** @brief Get a Light Lightness Control Server's current Mode.
 *
 *  The Mode determines whether the Server is actively controlling its Lightness
 *  Server's state.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  @ref bt_mesh_light_ctrl_cli_handlers::mode callback.
 *
 *  @param[in]  cli Client model to send on.
 *  @param[in]  ctx Message context, or NULL to use the configured publish
 *                  parameters.
 *  @param[out] rsp Response status buffer, returning the Server's current mode,
 *                  or NULL to keep from blocking.
 *
 *  @retval 0              Successfully sent the message and populated the @c
 *                         rsp buffer.
 *  @retval -EALREADY      A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_light_ctrl_cli_mode_get(struct bt_mesh_light_ctrl_cli *cli,
				    struct bt_mesh_msg_ctx *ctx, bool *rsp);

/** @brief Set a Light Lightness Control Server's current Mode.
 *
 *  The Mode determines whether the Server is actively controlling its Lightness
 *  Server's state.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  @ref bt_mesh_light_ctrl_cli_handlers::mode callback.
 *
 *  @param[in]  cli     Client model to send on.
 *  @param[in]  ctx     Message context, or NULL to use the configured publish
 *                      parameters.
 *  @param[in]  enabled The new Mode to set.
 *  @param[out] rsp     Response status buffer, returning the Server's current
 *                      mode, or NULL to keep from blocking.
 *
 *  @retval 0              Successfully sent the message and populated the @c
 *                         rsp buffer.
 *  @retval -EALREADY      A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_light_ctrl_cli_mode_set(struct bt_mesh_light_ctrl_cli *cli,
				    struct bt_mesh_msg_ctx *ctx, bool enabled,
				    bool *rsp);

/** @brief Set a Light Lightness Control Server's current Mode without
 *         requesting a response.
 *
 *  The Mode determines whether the Server is actively controlling its Lightness
 *  Server's state.
 *
 *  @param[in] cli     Client model to send on.
 *  @param[in] ctx     Message context, or NULL to use the configured publish
 *                     parameters.
 *  @param[in] enabled The new Mode to set.
 *
 *  @retval 0              Successfully sent the message.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 */
int bt_mesh_light_ctrl_cli_mode_set_unack(struct bt_mesh_light_ctrl_cli *cli,
					  struct bt_mesh_msg_ctx *ctx,
					  bool enabled);

/** @brief Get a Light Lightness Control Server's current Occupancy Mode.
 *
 *  The Occupancy Mode determines whether the Server accepts messages from
 *  Occupancy Servers.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  @ref bt_mesh_light_ctrl_cli_handlers::occupancy_mode callback.
 *
 *  @param[in]  cli Client model to send on.
 *  @param[in]  ctx Message context, or NULL to use the configured publish
 *                  parameters.
 *  @param[out] rsp Response status buffer, returning whether the Server's
 *                  occupancy mode is currently enabled, or NULL to keep from
 *                  blocking.
 *
 *  @retval 0              Successfully sent the message and populated the @c
 *                         rsp buffer.
 *  @retval -EALREADY      A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_light_ctrl_cli_occupancy_enabled_get(
	struct bt_mesh_light_ctrl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	bool *rsp);

/** @brief Set a Light Lightness Control Server's current Occupancy Mode.
 *
 *  The Occupancy Mode determines whether the Server accepts messages from
 *  Occupancy Servers.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  @ref bt_mesh_light_ctrl_cli_handlers::occupancy_mode callback.
 *
 *  @param[in]  cli     Client model to send on.
 *  @param[in]  ctx     Message context, or NULL to use the configured publish
 *                      parameters.
 *  @param[in]  enabled The new Occupancy Mode to set.
 *  @param[out] rsp     Response status buffer, returning whether the Server's
 *                      occupancy mode is currently enabled, or NULL to keep
 *                      from blocking.
 *
 *  @retval 0              Successfully sent the message and populated the @c
 *                         rsp buffer.
 *  @retval -EALREADY      A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_light_ctrl_cli_occupancy_enabled_set(
	struct bt_mesh_light_ctrl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	bool enabled, bool *rsp);

/** @brief Set a Light Lightness Control Server's current Occupancy Mode without
 *         requesting a response.
 *
 *  The Occupancy Mode determines whether the Server accepts messages from
 *  Occupancy Servers.
 *
 *  @param[in] cli     Client model to send on.
 *  @param[in] ctx     Message context, or NULL to use the configured publish
 *                     parameters.
 *  @param[in] enabled The new Occupancy Mode to set.
 *
 *  @retval 0              Successfully sent the message.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 */
int bt_mesh_light_ctrl_cli_occupancy_enabled_set_unack(
	struct bt_mesh_light_ctrl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	bool enabled);

/** @brief Get a Light Lightness Control Server's current OnOff state.
 *
 *  The OnOff state determines whether the Server is currently keeping the
 *  light of its Lightness Server on.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  @ref bt_mesh_light_ctrl_cli_handlers::light_onoff callback.
 *
 *  @param[in]  cli Client model to send on.
 *  @param[in]  ctx Message context, or NULL to use the configured publish
 *                  parameters.
 *  @param[out] rsp Response status buffer, returning the Server's current OnOff
 *                  state, or NULL to keep from blocking.
 *
 *  @retval 0              Successfully sent the message and populated the @c
 *                         rsp buffer.
 *  @retval -EALREADY      A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_light_ctrl_cli_light_onoff_get(struct bt_mesh_light_ctrl_cli *cli,
					   struct bt_mesh_msg_ctx *ctx,
					   struct bt_mesh_onoff_status *rsp);

/** @brief Tell a Light Lightness Control Server to turn the light on or off.
 *
 *  The OnOff state determines whether the Server is currently keeping the
 *  light of its Lightness Server on.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  @ref bt_mesh_light_ctrl_cli_handlers::light_onoff callback.
 *
 *  @param[in]  cli Client model to send on.
 *  @param[in]  ctx Message context, or NULL to use the configured publish
 *                  parameters.
 *  @param[in]  set Set parameters. The @c transition::time parameter may be set
 *                  to override the Server's default fade time, or @c transition
 *                  may be set to NULL.
 *  @param[out] rsp Response status buffer, returning the Server's current OnOff
 *                  state, or NULL to keep from blocking.
 *
 *  @retval 0              Successfully sent the message and populated the @c
 *                         rsp buffer.
 *  @retval -EALREADY      A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_light_ctrl_cli_light_onoff_set(struct bt_mesh_light_ctrl_cli *cli,
					   struct bt_mesh_msg_ctx *ctx,
					   const struct bt_mesh_onoff_set *set,
					   struct bt_mesh_onoff_status *rsp);

/** @brief Tell a Light Lightness Control Server to turn the light on or off
 *         without requesting a response.
 *
 *  The OnOff state determines whether the Server is currently keeping the
 *  light of its Lightness Server on.
 *
 *  @param[in]  cli Client model to send on.
 *  @param[in]  ctx Message context, or NULL to use the configured publish
 *                  parameters.
 *  @param[in]  set Set parameters. The @c transition::time parameter may be set
 *                  to override the Server's default fade time, or @c transition
 *                  may be set to NULL.
 *
 *  @retval 0              Successfully sent the message.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 */
int bt_mesh_light_ctrl_cli_light_onoff_set_unack(
	struct bt_mesh_light_ctrl_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_onoff_set *set);

/** @brief Get a Light Lightness Control Server property value.
 *
 *  Properties are the configuration parameters for the Light Lightness Control
 *  Server. Each property value is represented as a single sensor channel.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  @ref bt_mesh_light_ctrl_cli_handlers::prop callback.
 *
 *  @param[in]  cli Client model to send on.
 *  @param[in]  ctx Message context, or NULL to use the configured publish
 *                  parameters.
 *  @param[in]  id  Light Lightness Control Server property to get.
 *  @param[out] rsp Property value response buffer, or NULL to keep from
 *                  blocking.
 *
 *  @retval 0              Successfully sent the message and populated the @c
 *                         rsp buffer.
 *  @retval -EALREADY      A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_light_ctrl_cli_prop_get(struct bt_mesh_light_ctrl_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    enum bt_mesh_light_ctrl_prop id,
				    struct sensor_value *rsp);

/** @brief Set a Light Lightness Control Server property value.
 *
 *  Properties are the configuration parameters for the Light Lightness Control
 *  Server. Each property value is represented as a single sensor channel.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  @ref bt_mesh_light_ctrl_cli_handlers::prop callback.
 *
 *  @param[in]  cli Client model to send on.
 *  @param[in]  ctx Message context, or NULL to use the configured publish
 *                  parameters.
 *  @param[in]  id  Light Lightness Control Server property to set.
 *  @param[in]  val New property value.
 *  @param[out] rsp Property value response buffer, or NULL to keep from
 *                  blocking.
 *
 *  @retval 0              Successfully sent the message and populated the @c
 *                         rsp buffer.
 *  @retval -EALREADY      A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_light_ctrl_cli_prop_set(struct bt_mesh_light_ctrl_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    enum bt_mesh_light_ctrl_prop id,
				    const struct sensor_value *val,
				    struct sensor_value *rsp);

/** @brief Set a Light Lightness Control Server property value without
 *         requesting a response.
 *
 *  Properties are the configuration parameters for the Light Lightness Control
 *  Server. Each property value is represented as a single sensor channel.
 *
 *  @param[in]  cli Client model to send on.
 *  @param[in]  ctx Message context, or NULL to use the configured publish
 *                  parameters.
 *  @param[in]  id  Light Lightness Control Server property to set.
 *  @param[in]  val New property value.
 *
 *  @retval 0              Successfully sent the message.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 */
int bt_mesh_light_ctrl_cli_prop_set_unack(struct bt_mesh_light_ctrl_cli *cli,
					  struct bt_mesh_msg_ctx *ctx,
					  enum bt_mesh_light_ctrl_prop id,
					  const struct sensor_value *val);

/** @brief Get a Light Lightness Control Server Regulator Coefficient value.
 *
 *  Regulator coefficients are the configuration parameters for the Light
 *  Lightness Control Server Illuminance Regulator.
 *
 *  @param[in]  cli Client model to send on.
 *  @param[in]  ctx Message context, or NULL to use the configured publish
 *                  parameters.
 *  @param[in]  id  Light Lightness Control Server coefficient to get.
 *  @param[in]  rsp Coefficient value response buffer, or NULL to keep from
 *                  blocking.
 *
 *  @retval 0              Successfully sent the message and populated the @c
 *                         rsp buffer.
 *  @retval -EALREADY      A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_light_ctrl_cli_coeff_get(struct bt_mesh_light_ctrl_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     enum bt_mesh_light_ctrl_coeff id,
				     float *rsp);

/** @brief Set a Light Lightness Control Server Regulator Coefficient value.
 *
 *  Regulator coefficients are the configuration parameters for the Light
 *  Lightness Control Server Illuminance Regulator.
 *
 *  @param[in]  cli Client model to send on.
 *  @param[in]  ctx Message context, or NULL to use the configured publish
 *                  parameters.
 *  @param[in]  id  Light Lightness Control Server coefficient to set.
 *  @param[in]  val New coefficient value.
 *  @param[in]  rsp Coefficient value response buffer, or NULL to keep from
 *                  blocking.
 *
 *  @retval 0              Successfully sent the message and populated the @c
 *                         rsp buffer.
 *  @retval -EALREADY      A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_light_ctrl_cli_coeff_set(struct bt_mesh_light_ctrl_cli *cli,
				     struct bt_mesh_msg_ctx *ctx,
				     enum bt_mesh_light_ctrl_coeff id,
				     float val, float *rsp);

/** @brief Set a Light Lightness Control Server Regulator Coefficient value
 *         without requesting a response.
 *
 *  Regulator coefficients are the configuration parameters for the Light
 *  Lightness Control Server Illuminance Regulator.
 *
 *  @param[in]  cli Client model to send on.
 *  @param[in]  ctx Message context, or NULL to use the configured publish
 *                  parameters.
 *  @param[in]  id  Light Lightness Control Server coefficient to set.
 *  @param[in]  val New coefficient value.
 *
 *  @retval 0              Successfully sent the message.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 */
int bt_mesh_light_ctrl_cli_coeff_set_unack(struct bt_mesh_light_ctrl_cli *cli,
					   struct bt_mesh_msg_ctx *ctx,
					   enum bt_mesh_light_ctrl_coeff id,
					   float val);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_light_ctrl_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_light_ctrl_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_LIGHT_CTRL_CLI_H__ */

/** @} */
