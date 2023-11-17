/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @defgroup bt_mesh_sensor_cli Sensor Client model
 *  @{
 *  @brief API for the Sensor Client.
 */

#ifndef BT_MESH_SENSOR_CLI_H__
#define BT_MESH_SENSOR_CLI_H__

#include <bluetooth/mesh/sensor.h>
#include <bluetooth/mesh/model_types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_sensor_cli_handlers;

/** @def BT_MESH_SENSOR_CLI_INIT
 *
 *  @brief Initialization parameters for @ref bt_mesh_sensor_cli.
 *
 *  @sa bt_mesh_sensor_cli_handlers
 *
 *  @param[in] _handlers Optional message handler structure.
 */
#define BT_MESH_SENSOR_CLI_INIT(_handlers)                                     \
	{                                                                      \
		.cb = _handlers,                                               \
	}

/** @def BT_MESH_MODEL_SENSOR_CLI
 *
 *  @brief Sensor Client model composition data entry.
 *
 *  @param[in] _cli Pointer to a @ref bt_mesh_sensor_cli instance.
 */
#define BT_MESH_MODEL_SENSOR_CLI(_cli)                                         \
	BT_MESH_MODEL_CB(BT_MESH_MODEL_ID_SENSOR_CLI, _bt_mesh_sensor_cli_op,  \
			 &(_cli)->pub,                                         \
			 BT_MESH_MODEL_USER_DATA(struct bt_mesh_sensor_cli,    \
						 _cli),                        \
			 &_bt_mesh_sensor_cli_cb)

/** Sensor client instance.
 *
 *  Should be initialized with @ref BT_MESH_SENSOR_CLI_INIT.
 */
struct bt_mesh_sensor_cli {
	/** Composition data model instance. */
	struct bt_mesh_model *model;
	/** Model publication parameters. */
	struct bt_mesh_model_pub pub;
	/* Publication buffer */
	struct net_buf_simple pub_buf;
	/* Publication data */
	uint8_t pub_data[MAX(
		BT_MESH_MODEL_BUF_LEN(BT_MESH_SENSOR_OP_CADENCE_SET,
				      BT_MESH_SENSOR_MSG_MAXLEN_CADENCE_SET),
		BT_MESH_MODEL_BUF_LEN(BT_MESH_SENSOR_OP_SETTING_SET,
				      BT_MESH_SENSOR_MSG_MAXLEN_SETTING_SET))];
	/** Response context for acknowledged messages. */
	struct bt_mesh_msg_ack_ctx ack_ctx;
	/** Client callback functions. */
	const struct bt_mesh_sensor_cli_handlers *cb;
};

/** Sensor cadence status parameters */
struct bt_mesh_sensor_cadence_status {
	/** Logarithmic fast publish period divisor value.
	 *
	 *  The sensor's fast period interval is
	 *  publish_period / (1 << fast_period_div).
	 */
	uint8_t fast_period_div;
	/** Logarithmic minimum publish period.
	 *
	 *  The sensor's fast period interval is never lower than
	 *  (1 << min_int).
	 */
	uint8_t min_int;
	/** Sensor threshold values. */
	struct bt_mesh_sensor_threshold threshold;
};

#if !defined(CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE) || defined(__DOXYGEN__)
/** Sensor settings parameters. */
struct bt_mesh_sensor_setting_status {
	/** Setting type */
	const struct bt_mesh_sensor_type *type;
	/** Setting value. */
	struct bt_mesh_sensor_value value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];
	/** Whether the setting can be written to. */
	bool writable;
};

/** Sensor series entry **/
struct bt_mesh_sensor_series_entry {
	/** Sensor column descriptor. */
	struct bt_mesh_sensor_column column;
	/** Sensor column value. */
	struct bt_mesh_sensor_value value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];
};

/** Sensor info structure. */
struct bt_mesh_sensor_info {
	/** Sensor Device Property ID */
	uint16_t id;
	/** Sensor descriptor. */
	struct bt_mesh_sensor_descriptor descriptor;
};

/** Sensor data structure. */
struct bt_mesh_sensor_data {
	/** Sensor type. */
	const struct bt_mesh_sensor_type *type;
	/** Sensor value. */
	struct bt_mesh_sensor_value value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];
};

/** @brief Sensor column indexing value
 *
 *  @c sensor_value is used for sensor types with three channels. @c index is
 *  used for sensor types with one or two channels.
 */
union bt_mesh_sensor_column_key {
	struct bt_mesh_sensor_value sensor_value;
	uint16_t index;
};

/** Sensor client handler functions. */
struct bt_mesh_sensor_cli_handlers {
	/** @brief Sensor data callback.
	 *
	 *  Called when the client receives sensor sample data, either as a
	 *  result of calling @ref bt_mesh_sensor_cli_get, or as an unsolicited
	 *  message.
	 *
	 *  @param[in] cli    Sensor client receiving the message.
	 *  @param[in] ctx    Message context.
	 *  @param[in] sensor Sensor instance.
	 *  @param[in] value  The received sensor data value as an array of
	 *                    channels. The length of the array matches the
	 *                    sensor channel count.
	 */
	void (*data)(struct bt_mesh_sensor_cli *cli,
		     struct bt_mesh_msg_ctx *ctx,
		     const struct bt_mesh_sensor_type *sensor,
		     const struct bt_mesh_sensor_value *value);

	/** @brief Sensor description callback.
	 *
	 *  Called when the client receives sensor descriptors, either as a
	 *  result of calling @ref bt_mesh_sensor_cli_all_get or @ref
	 *  bt_mesh_sensor_cli_desc_get, or as an unsolicited message.
	 *
	 *  The sensor description does not reference the sensor type directly,
	 *  to allow discovery of sensor types unknown to the client. To get the
	 *  sensor type of a known sensor, call @ref bt_mesh_sensor_type_get.
	 *
	 *  @param[in] cli    Sensor client receiving the message.
	 *  @param[in] ctx    Message context.
	 *  @param[in] sensor Sensor information for a single sensor.
	 */
	void (*sensor)(struct bt_mesh_sensor_cli *cli,
		       struct bt_mesh_msg_ctx *ctx,
		       const struct bt_mesh_sensor_info *sensor);

	/** @brief Sensor cadence callback.
	 *
	 *  Called when the client receives the cadence of a sensor, either as a
	 *  result of calling one of @ref bt_mesh_sensor_cli_cadence_get,
	 *  @ref bt_mesh_sensor_cli_cadence_set or
	 *  @ref bt_mesh_sensor_cli_cadence_set_unack, or as an unsolicited
	 *  message.
	 *
	 *  @param[in] cli     Sensor client receiving the message.
	 *  @param[in] ctx     Message context.
	 *  @param[in] sensor  Sensor instance.
	 *  @param[in] cadence Sensor cadence information.
	 */
	void (*cadence)(struct bt_mesh_sensor_cli *cli,
			struct bt_mesh_msg_ctx *ctx,
			const struct bt_mesh_sensor_type *sensor,
			const struct bt_mesh_sensor_cadence_status *cadence);

	/** @brief Sensor settings list callback.
	 *
	 *  Called when the client receives the full list of sensor settings, as
	 *  a result of calling @ref bt_mesh_sensor_cli_settings_get or as an
	 *  unsolicited message.
	 *
	 *  @param[in] cli    Sensor client receiving the message.
	 *  @param[in] ctx    Message context.
	 *  @param[in] sensor Sensor instance.
	 *  @param[in] ids    Available sensor setting IDs.
	 *  @param[in] count  The number of sensor setting IDs.
	 */
	void (*settings)(struct bt_mesh_sensor_cli *cli,
			 struct bt_mesh_msg_ctx *ctx,
			 const struct bt_mesh_sensor_type *sensor,
			 const uint16_t *ids, uint32_t count);

	/** @brief Sensor setting status callback.
	 *
	 *  Called when the client receives a sensor setting status, either as
	 *  result of calling @ref bt_mesh_sensor_cli_setting_get, @ref
	 *  bt_mesh_sensor_cli_setting_set, @ref
	 *  bt_mesh_sensor_cli_setting_set_unack, or as an unsolicited message.
	 *
	 *  @param[in] cli     Sensor client receiving the message.
	 *  @param[in] ctx     Message context.
	 *  @param[in] sensor  Sensor instance.
	 *  @param[in] setting Sensor setting information.
	 */
	void (*setting_status)(
		struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_sensor_type *sensor,
		const struct bt_mesh_sensor_setting_status *setting);

	/** @brief Series entry callback.
	 *
	 *  Called when the client receives series entries, either as result of
	 *  calling one of @ref bt_mesh_sensor_cli_series_entry_get or @ref
	 *  bt_mesh_sensor_cli_series_entries_get, or as a result of an
	 *  unsolicited message.
	 *
	 *  If the received series entry message contains several entries, this
	 *  callback is called once per entry, with the @c index and @c count
	 *  parameters indicating the progress.
	 *
	 *  @note The @c index and @c count parameters does not necessarily
	 *        match the total number of series entries of the sensor, as the
	 *        callback may be the result of a filtered query.
	 *
	 *  @param[in] cli    Sensor client receiving the message.
	 *  @param[in] ctx    Message context.
	 *  @param[in] sensor Sensor instance.
	 *  @param[in] index  Index of this entry in the list of entries
	 *                    received.
	 *  @param[in] count  Total number of entries received.
	 *  @param[in] entry  Single sensor series entry.
	 */
	void (*series_entry)(struct bt_mesh_sensor_cli *cli,
			     struct bt_mesh_msg_ctx *ctx,
			     const struct bt_mesh_sensor_type *sensor,
			     uint8_t index, uint8_t count,
			     const struct bt_mesh_sensor_series_entry *entry);

	/** @brief Unknown type callback.
	 *
	 *  Called when the client receives a message with a sensor type it
	 *  doesn't have type information for.
	 *
	 *  @param[in] cli    Sensor client receiving the message.
	 *  @param[in] ctx    Message context.
	 *  @param[in] id     Sensor type ID.
	 *  @param[in] opcode The opcode of the message containing the unknown
	 *                    sensor type.
	 */
	void (*unknown_type)(struct bt_mesh_sensor_cli *cli,
			     struct bt_mesh_msg_ctx *ctx, uint16_t id,
			     uint32_t opcode);
};

/** @brief Set a setting value for a sensor.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  bt_mesh_sensor_cli_handlers::setting_status callback.
 *
 *  @param[in]  cli     Sensor client instance.
 *  @param[in]  ctx     Message context parameters, or NULL to use the
 *                      configured publish parameters.
 *  @param[in]  sensor  Sensor instance present on the targeted sensor server.
 *  @param[in]  setting Setting to change.
 *  @param[in]  value   New setting value. Must contain values for all channels
 *                      described by @c setting.
 *  @param[out] rsp     Sensor setting value response buffer, or NULL to keep
 *                      from blocking.
 *
 *  @retval 0              Successfully changed the setting. The @c rsp
 *                         buffer has been filled.
 *  @retval -ENOENT        The sensor doesn't have the given setting.
 *  @retval -EACCES        The setting can't be written to.
 *  @retval -EALREADY      A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_sensor_cli_setting_set(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_sensor_type *sensor,
	const struct bt_mesh_sensor_type *setting,
	const struct bt_mesh_sensor_value *value,
	struct bt_mesh_sensor_setting_status *rsp);

/** @brief Set a setting value for a sensor without requesting a response.
 *
 *  @param[in] cli     Sensor client instance.
 *  @param[in] ctx     Message context parameters, or NULL to use the configured
 *                     publish parameters.
 *  @param[in] sensor  Sensor instance present on the targeted sensor server.
 *  @param[in] setting Setting to change.
 *  @param[in] value   New setting value. Must contain values for all channels
 *                     described by @c setting.
 *
 *  @retval 0              Successfully changed the setting.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 */
int bt_mesh_sensor_cli_setting_set_unack(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_sensor_type *sensor,
	const struct bt_mesh_sensor_type *setting,
	const struct bt_mesh_sensor_value *value);

/** @brief Read sensor data from a sensor instance.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  bt_mesh_sensor_cli_handlers::data callback.
 *
 *  @param[in]  cli    Sensor client instance.
 *  @param[in]  ctx    Message context parameters, or NULL to use the configured
 *                     publish parameters.
 *  @param[in]  sensor Sensor instance present on the targeted sensor server.
 *  @param[out] rsp    Response value buffer, or NULL to keep from blocking.
 *                     Must be able to fit all channels described by the sensor
 *                     type.
 *
 *  @retval 0              Successfully received the sensor data.
 *  @retval -ENODEV        The sensor server doesn't have the given sensor.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_sensor_cli_get(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_sensor_type *sensor,
	struct bt_mesh_sensor_value *rsp);

/** @brief Read a single sensor series data entry.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  bt_mesh_sensor_cli_handlers::series_entry callback as a list of
 *  sensor descriptors.
 *
 *  @param[in]  cli    Sensor client instance.
 *  @param[in]  ctx    Message context parameters, or NULL to use the configured
 *                     publish parameters.
 *  @param[in]  sensor Sensor instance present on the targeted sensor server.
 *  @param[in]  column Column to read. For sensor with three channels, the
 *                     @c sensor_value field must match the start value of a
 *                     series column on the sensor. For sensors with only one or
 *                     two channels, the @c index field is used as the index of
 *                     the column to get.
 *  @param[out] rsp    Response value buffer, or NULL to keep from blocking.
 *                     Must be able to fit all channels described by the sensor
 *                     type.
 *
 *  @retval 0              Successfully received the sensor data.
 *  @retval -ENODEV        The sensor server doesn't have the given sensor.
 *  @retval -ENOENT        The sensor doesn't have the given column.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_sensor_cli_series_entry_get(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_sensor_type *sensor,
	const union bt_mesh_sensor_column_key *column,
	struct bt_mesh_sensor_series_entry *rsp);

/** @brief Get multiple sensor series data entries.
 *
 *  Retrieves all data series columns starting within the given range
 *  (inclusive), or all data series entries if @c range_start or @c range_end
 *  is NULL. For instance, requesting range [1, 5] from a sensor with columns
 *  [0, 2], [1, 4], [4, 5] and [5, 8] will return all columns except [0, 2].
 *  For sensors with only one or two channels, the range values are indices
 *  into the column series, inclusive.
 *
 *  If a @c rsp array is provided and the client received a response, the array
 *  will be filled with as many of the response columns as it can fit, even if
 *  the buffer isn't big enough. If the call fails in a way that results in no
 *  response, @c count is set to 0.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  bt_mesh_sensor_cli_handlers::series_entry callback as a list of
 *  sensor descriptors.
 *
 *  @param[in]     cli         Sensor client instance.
 *  @param[in]     ctx         Message context parameters, or NULL to use the
 *                             configured publish parameters.
 *  @param[in]     sensor      Sensor instance present on the targeted sensor
 *                             server.
 *  @param[in]     range_start Start of column range to get, or NULL to get all
 *                             columns.
 *  @param[in]     range_end   End of column range to get, or NULL to get all
 *                             columns.
 *  @param[out]    rsp         Array of entries to copy the response into, or
 *                             NULL to keep from blocking.
 *  @param[in,out] count       The number of entries in @c rsp. Is changed to
 *                             reflect the number of entries in the response.
 *
 *  @retval 0              Successfully received the full list of sensor series
 *                         columns. The @c rsp array and @c count has been
 *                         changed to reflect the response contents.
 *  @retval -ENODEV        The sensor server doesn't have the given sensor.
 *  @retval -E2BIG         The list of sensor columns in the response was too
 *                         big to fit in the @c rsp array. The @c rsp array has
 *                         been filled up to the original @c count, and @c count
 *                         has been changed to the number of columns in the
 *                         response.
 *  @retval -ENOTSUP       The sensor doesn't support series data.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_sensor_cli_series_entries_get(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_sensor_type *sensor,
	const union bt_mesh_sensor_column_key *range_start,
	const union bt_mesh_sensor_column_key *range_end,
	struct bt_mesh_sensor_series_entry *rsp, uint32_t *count);
#else /* defined(CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE) */
/** Sensor settings parameters. */
struct bt_mesh_sensor_setting_status {
	/** Setting type */
	const struct bt_mesh_sensor_type *type;
	/** Setting value. */
	struct sensor_value value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];
	/** Whether the setting can be written to. */
	bool writable;
};

/** Sensor series entry **/
struct bt_mesh_sensor_series_entry {
	/** Sensor column descriptor. */
	struct bt_mesh_sensor_column column;
	/** Sensor column value. */
	struct sensor_value value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];
};

/** Sensor info structure. */
struct bt_mesh_sensor_info {
	/** Sensor Device Property ID */
	uint16_t id;
	/** Sensor descriptor. */
	struct bt_mesh_sensor_descriptor descriptor;
};

/** Sensor data structure. */
struct bt_mesh_sensor_data {
	/** Sensor type. */
	const struct bt_mesh_sensor_type *type;
	/** Sensor value. */
	struct sensor_value value[CONFIG_BT_MESH_SENSOR_CHANNELS_MAX];
};

/** Sensor client handler functions. */
struct bt_mesh_sensor_cli_handlers {
	/** @brief Sensor data callback.
	 *
	 *  Called when the client receives sensor sample data, either as a
	 *  result of calling @ref bt_mesh_sensor_cli_get, or as an unsolicited
	 *  message.
	 *
	 *  @param[in] cli    Sensor client receiving the message.
	 *  @param[in] ctx    Message context.
	 *  @param[in] sensor Sensor instance.
	 *  @param[in] value  The interpreted sensor data value as an array of
	 *                    channels. The length of the array matches the
	 *                    sensor channel count.
	 */
	void (*data)(struct bt_mesh_sensor_cli *cli,
		     struct bt_mesh_msg_ctx *ctx,
		     const struct bt_mesh_sensor_type *sensor,
		     const struct sensor_value *value);

	/** @brief Sensor description callback.
	 *
	 *  Called when the client receives sensor descriptors, either as a
	 *  result of calling @ref bt_mesh_sensor_cli_all_get or @ref
	 *  bt_mesh_sensor_cli_desc_get, or as an unsolicited message.
	 *
	 *  The sensor description does not reference the sensor type directly,
	 *  to allow discovery of sensor types unknown to the client. To get the
	 *  sensor type of a known sensor, call @ref bt_mesh_sensor_type_get.
	 *
	 *  @param[in] cli    Sensor client receiving the message.
	 *  @param[in] ctx    Message context.
	 *  @param[in] sensor Sensor information for a single sensor.
	 */
	void (*sensor)(struct bt_mesh_sensor_cli *cli,
		       struct bt_mesh_msg_ctx *ctx,
		       const struct bt_mesh_sensor_info *sensor);

	/** @brief Sensor cadence callback.
	 *
	 *  Called when the client receives the cadence of a sensor, either as a
	 *  result of calling one of @ref bt_mesh_sensor_cli_cadence_get,
	 *  @ref bt_mesh_sensor_cli_cadence_set or
	 *  @ref bt_mesh_sensor_cli_cadence_set_unack, or as an unsolicited
	 *  message.
	 *
	 *  @param[in] cli     Sensor client receiving the message.
	 *  @param[in] ctx     Message context.
	 *  @param[in] sensor  Sensor instance.
	 *  @param[in] cadence Sensor cadence information.
	 */
	void (*cadence)(struct bt_mesh_sensor_cli *cli,
			struct bt_mesh_msg_ctx *ctx,
			const struct bt_mesh_sensor_type *sensor,
			const struct bt_mesh_sensor_cadence_status *cadence);

	/** @brief Sensor settings list callback.
	 *
	 *  Called when the client receives the full list of sensor settings, as
	 *  a result of calling @ref bt_mesh_sensor_cli_settings_get or as an
	 *  unsolicited message.
	 *
	 *  @param[in] cli    Sensor client receiving the message.
	 *  @param[in] ctx    Message context.
	 *  @param[in] sensor Sensor instance.
	 *  @param[in] ids    Available sensor setting IDs.
	 *  @param[in] count  The number of sensor setting IDs.
	 */
	void (*settings)(struct bt_mesh_sensor_cli *cli,
			 struct bt_mesh_msg_ctx *ctx,
			 const struct bt_mesh_sensor_type *sensor,
			 const uint16_t *ids, uint32_t count);

	/** @brief Sensor setting status callback.
	 *
	 *  Called when the client receives a sensor setting status, either as
	 *  result of calling @ref bt_mesh_sensor_cli_setting_get, @ref
	 *  bt_mesh_sensor_cli_setting_set, @ref
	 *  bt_mesh_sensor_cli_setting_set_unack, or as an unsolicited message.
	 *
	 *  @param[in] cli     Sensor client receiving the message.
	 *  @param[in] ctx     Message context.
	 *  @param[in] sensor  Sensor instance.
	 *  @param[in] setting Sensor setting information.
	 */
	void (*setting_status)(
		struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_sensor_type *sensor,
		const struct bt_mesh_sensor_setting_status *setting);

	/** @brief Series entry callback.
	 *
	 *  Called when the client receives series entries, either as result of
	 *  calling one of @ref bt_mesh_sensor_cli_series_entry_get or @ref
	 *  bt_mesh_sensor_cli_series_entries_get, or as a result of an
	 *  unsolicited message.
	 *
	 *  If the received series entry message contains several entries, this
	 *  callback is called once per entry, with the @c index and @c count
	 *  parameters indicating the progress.
	 *
	 *  @note The @c index and @c count parameters does not necessarily
	 *        match the total number of series entries of the sensor, as the
	 *        callback may be the result of a filtered query.
	 *
	 *  @param[in] cli    Sensor client receiving the message.
	 *  @param[in] ctx    Message context.
	 *  @param[in] sensor Sensor instance.
	 *  @param[in] index  Index of this entry in the list of entries
	 *                    received.
	 *  @param[in] count  Total number of entries received.
	 *  @param[in] entry  Single sensor series entry.
	 */
	void (*series_entry)(struct bt_mesh_sensor_cli *cli,
			     struct bt_mesh_msg_ctx *ctx,
			     const struct bt_mesh_sensor_type *sensor,
			     uint8_t index, uint8_t count,
			     const struct bt_mesh_sensor_series_entry *entry);

	/** @brief Unknown type callback.
	 *
	 *  Called when the client receives a message with a sensor type it
	 *  doesn't have type information for.
	 *
	 *  @param[in] cli    Sensor client receiving the message.
	 *  @param[in] ctx    Message context.
	 *  @param[in] id     Sensor type ID.
	 *  @param[in] opcode The opcode of the message containing the unknown
	 *                    sensor type.
	 */
	void (*unknown_type)(struct bt_mesh_sensor_cli *cli,
			     struct bt_mesh_msg_ctx *ctx, uint16_t id,
			     uint32_t opcode);
};

/** @brief Set a setting value for a sensor.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  bt_mesh_sensor_cli_handlers::setting_status callback.
 *
 *  @param[in]  cli     Sensor client instance.
 *  @param[in]  ctx     Message context parameters, or NULL to use the
 *                      configured publish parameters.
 *  @param[in]  sensor  Sensor instance present on the targeted sensor server.
 *  @param[in]  setting Setting to change.
 *  @param[in]  value   New setting value. Must contain values for all channels
 *                      described by @c setting.
 *  @param[out] rsp     Sensor setting value response buffer, or NULL to keep
 *                      from blocking.
 *
 *  @retval 0              Successfully changed the setting. The @c rsp
 *                         buffer has been filled.
 *  @retval -ENOENT        The sensor doesn't have the given setting.
 *  @retval -EACCES        The setting can't be written to.
 *  @retval -EALREADY      A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_sensor_cli_setting_set(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_sensor_type *sensor,
	const struct bt_mesh_sensor_type *setting,
	const struct sensor_value *value,
	struct bt_mesh_sensor_setting_status *rsp);

/** @brief Set a setting value for a sensor without requesting a response.
 *
 *  @param[in] cli     Sensor client instance.
 *  @param[in] ctx     Message context parameters, or NULL to use the configured
 *                     publish parameters.
 *  @param[in] sensor  Sensor instance present on the targeted sensor server.
 *  @param[in] setting Setting to change.
 *  @param[in] value   New setting value. Must contain values for all channels
 *                     described by @c setting.
 *
 *  @retval 0              Successfully changed the setting.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 */
int bt_mesh_sensor_cli_setting_set_unack(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_sensor_type *sensor,
	const struct bt_mesh_sensor_type *setting,
	const struct sensor_value *value);

/** @brief Read sensor data from a sensor instance.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  bt_mesh_sensor_cli_handlers::data callback.
 *
 *  @param[in]  cli    Sensor client instance.
 *  @param[in]  ctx    Message context parameters, or NULL to use the configured
 *                     publish parameters.
 *  @param[in]  sensor Sensor instance present on the targeted sensor server.
 *  @param[out] rsp    Response value buffer, or NULL to keep from blocking.
 *                     Must be able to fit all channels described by the sensor
 *                     type.
 *
 *  @retval 0              Successfully received the sensor data.
 *  @retval -ENODEV        The sensor server doesn't have the given sensor.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_sensor_cli_get(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_sensor_type *sensor,
	struct sensor_value *rsp);

/** @brief Read a single sensor series data entry.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  bt_mesh_sensor_cli_handlers::series_entry callback as a list of
 *  sensor descriptors.
 *
 *  @param[in]  cli    Sensor client instance.
 *  @param[in]  ctx    Message context parameters, or NULL to use the configured
 *                     publish parameters.
 *  @param[in]  sensor Sensor instance present on the targeted sensor server.
 *  @param[in]  column Column to read. The start value must match the start
 *                     value of a series column on the sensor. The end value is
 *                     ignored. For sensors with only one or two channels, this
 *                     sensor value represents the index of the column to get.
 *  @param[out] rsp    Response value buffer, or NULL to keep from blocking.
 *                     Must be able to fit all channels described by the sensor
 *                     type.
 *
 *  @retval 0              Successfully received the sensor data.
 *  @retval -ENODEV        The sensor server doesn't have the given sensor.
 *  @retval -ENOENT        The sensor doesn't have the given column.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_sensor_cli_series_entry_get(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_sensor_type *sensor,
	const struct bt_mesh_sensor_column *column,
	struct bt_mesh_sensor_series_entry *rsp);

/** @brief Get multiple sensor series data entries.
 *
 *  Retrieves all data series columns starting within the given range
 *  (inclusive), or all data series entries if @c range is NULL. For instance,
 *  requesting range [1, 5] from a sensor with columns
 *  [0, 2], [1, 4], [4, 5] and [5, 8] will return all columns except [0, 2].
 *  For sensors with only one or two channels, the range values are indices
 *  into the column series, inclusive.
 *
 *  If a @c rsp array is provided and the client received a response, the array
 *  will be filled with as many of the response columns as it can fit, even if
 *  the buffer isn't big enough. If the call fails in a way that results in no
 *  response, @c count is set to 0.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  bt_mesh_sensor_cli_handlers::series_entry callback as a list of
 *  sensor descriptors.
 *
 *  @param[in]     cli    Sensor client instance.
 *  @param[in]     ctx    Message context parameters, or NULL to use the
 *                        configured publish parameters.
 *  @param[in]     sensor Sensor instance present on the targeted sensor server.
 *  @param[in]     range  Range of columns to get, or NULL to get all columns.
 *  @param[out]    rsp    Array of entries to copy the response into, or NULL
 *                        to keep from blocking.
 *  @param[in,out] count  The number of entries in @c rsp. Is changed to reflect
 *                        the number of entries in the response.
 *
 *  @retval 0              Successfully received the full list of sensor series
 *                         columns. The @c rsp array and @c count has been
 *                         changed to reflect the response contents.
 *  @retval -ENODEV        The sensor server doesn't have the given sensor.
 *  @retval -E2BIG         The list of sensor columns in the response was too
 *                         big to fit in the @c rsp array. The @c rsp array has
 *                         been filled up to the original @c count, and @c count
 *                         has been changed to the number of columns in the
 *                         response.
 *  @retval -ENOTSUP       The sensor doesn't support series data.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_sensor_cli_series_entries_get(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_sensor_type *sensor,
	const struct bt_mesh_sensor_column *range,
	struct bt_mesh_sensor_series_entry *rsp, uint32_t *count);
#endif /* !defined(CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE) */

/** @brief Retrieve all sensor descriptors in a sensor server.
 *
 *  This call is blocking if the @c sensors buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  bt_mesh_sensor_cli_handlers::sensor callback as a list of sensor
 *  descriptors.
 *
 *  If a @c sensors array is provided and the client received a response, the
 *  array will be filled with as many of the response sensors as it can fit,
 *  even if the buffer isn't big enough. If the call fails in a way that results
 *  in no response, @c count is set to 0.
 *
 *  @param[in]     cli     Sensor client instance.
 *  @param[in]     ctx     Message context parameters, or NULL to use the
 *                         configured publish parameters.
 *  @param[out]    sensors Array of sensors to fill with the response, or NULL
 *                         to keep from blocking.
 *  @param[in,out] count   The number of elements in the @c sensors array. Will
 *                         be changed to reflect the resulting number of
 *                         sensors.
 *
 *  @retval 0              Successfully received the full list of sensors.
 *                         The @c sensors array and @c count has been changed to
 *                         reflect the response contents.
 *  @retval -EALREADY      A blocking request is already in progress.
 *  @retval -E2BIG         The list of sensors in the response was too big to
 *                         fit in the @c sensors array. The @c sensors array has
 *                         been filled up to the original @c count, and @c count
 *                         has been changed to the number of sensors in the
 *                         response.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_sensor_cli_desc_all_get(struct bt_mesh_sensor_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    struct bt_mesh_sensor_info *sensors,
				    uint32_t *count);

/** @brief Get the descriptor for the given sensor.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  bt_mesh_sensor_cli_handlers::sensor callback.
 *
 *  @param[in]  cli    Sensor client instance.
 *  @param[in]  ctx    Message context parameters, or NULL to use the configured
 *                     publish parameters.
 *  @param[in]  sensor Sensor instance present on the targeted sensor server.
 *  @param[out] rsp    Sensor descriptor response buffer, or NULL to keep from
 *                     blocking.
 *
 *  @retval 0              Successfully received the descriptor. The @c rsp
 *                         buffer has been filled.
 *  @retval -ENODEV        The sensor server doesn't have the given sensor.
 *  @retval -EALREADY      A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_sensor_cli_desc_get(struct bt_mesh_sensor_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_sensor_type *sensor,
				struct bt_mesh_sensor_descriptor *rsp);

/** @brief Get the cadence state.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  bt_mesh_sensor_cli_handlers::cadence callback.
 *
 *  @param[in]  cli    Sensor client instance.
 *  @param[in]  ctx    Message context parameters, or NULL to use the configured
 *                     publish parameters.
 *  @param[in]  sensor Sensor to get the cadence of.
 *  @param[out] rsp    Sensor cadence response buffer, or NULL to keep from
 *                     blocking.
 *
 *  @retval 0              Successfully received the cadence. The @c rsp
 *                         buffer has been filled.
 *  @retval -ENOTSUP       The sensor doesn't support cadence settings.
 *  @retval -EALREADY      A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_sensor_cli_cadence_get(struct bt_mesh_sensor_cli *cli,
				   struct bt_mesh_msg_ctx *ctx,
				   const struct bt_mesh_sensor_type *sensor,
				   struct bt_mesh_sensor_cadence_status *rsp);

/** @brief Set the cadence state for the given sensor.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  bt_mesh_sensor_cli_handlers::cadence callback.
 *
 *  @note Only single-channel sensors support cadence.
 *
 *  @param[in]  cli     Sensor client instance.
 *  @param[in]  ctx     Message context parameters, or NULL to use the
 *                      configured publish parameters.
 *  @param[in]  sensor  Sensor instance present on the targeted sensor server.
 *  @param[in]  cadence New sensor cadence for the sensor.
 *  @param[out] rsp     Sensor cadence response buffer, or NULL to keep from
 *                      blocking.
 *
 *  @retval 0              Successfully received the cadence. The @c rsp
 *                         buffer has been filled.
 *  @retval -ENOTSUP       The sensor doesn't support cadence settings.
 *  @retval -EALREADY      A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_sensor_cli_cadence_set(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_sensor_type *sensor,
	const struct bt_mesh_sensor_cadence_status *cadence,
	struct bt_mesh_sensor_cadence_status *rsp);

/** @brief Set the cadence state for the given sensor without requesting a
 *         response.
 *
 *  @note Only single-channel sensors support cadence.
 *
 *  @param[in] cli     Sensor client instance.
 *  @param[in] ctx     Message context parameters, or NULL to use the configured
 *                     publish parameters.
 *  @param[in] sensor  Sensor instance present on the targeted sensor server.
 *  @param[in] cadence New sensor cadence for the sensor.
 *
 *  @retval 0              Successfully received the cadence.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 */
int bt_mesh_sensor_cli_cadence_set_unack(
	struct bt_mesh_sensor_cli *cli, struct bt_mesh_msg_ctx *ctx,
	const struct bt_mesh_sensor_type *sensor,
	const struct bt_mesh_sensor_cadence_status *cadence);

/** @brief Get the list of settings for the given sensor.
 *
 *  This call is blocking if the @c ids buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  bt_mesh_sensor_cli_handlers::settings callback.
 *
 *  If an @c ids array is provided and the client received a response, the array
 *  will be filled with as many of the response ids as it can fit, even if the
 *  buffer isn't big enough. If the call fails in a way that results in no
 *  response, @c count is set to 0.
 *
 *  @param[in]     cli    Sensor client instance.
 *  @param[in]     ctx    Message context parameters, or NULL to use the
 *                        configured publish parameters.
 *  @param[in]     sensor Sensor instance present on the targeted sensor server.
 *  @param[out]    ids    Array of sensor setting IDs to fill with the response,
 *                        or NULL to keep from blocking.
 *  @param[in,out] count  The number of elements in the @c ids array. Will be
 *                        changed to reflect the resulting number of IDs.
 *
 *  @retval 0              Successfully received the full list of sensor setting
 *                         IDs. The @c ids array and @c count has been changed
 *                         to reflect the response contents.
 *  @retval -ENODEV        The sensor server doesn't have the given sensor.
 *  @retval -E2BIG         The list of IDs in the response was too big to fit in
 *                         the @c ids array. The @c ids array has been filled
 *                         up to the original @c count, and @c count has been
 *                         changed to the number of IDs in the response.
 *  @retval -EALREADY      A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_sensor_cli_settings_get(struct bt_mesh_sensor_cli *cli,
				    struct bt_mesh_msg_ctx *ctx,
				    const struct bt_mesh_sensor_type *sensor,
				    uint16_t *ids, uint32_t *count);

/** @brief Get a setting value for a sensor.
 *
 *  This call is blocking if the @c rsp buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  bt_mesh_sensor_cli_handlers::setting_status callback.
 *
 *  @param[in]  cli     Sensor client instance.
 *  @param[in]  ctx     Message context parameters, or NULL to use the
 *                      configured publish parameters.
 *  @param[in]  sensor  Sensor instance present on the targeted sensor server.
 *  @param[in]  setting Setting to read.
 *  @param[out] rsp     Sensor setting value response buffer, or NULL to keep
 *                      from blocking.
 *
 *  @retval 0              Successfully received the setting value. The @c rsp
 *                         buffer has been filled.
 *  @retval -ENOENT        The sensor doesn't have the given setting.
 *  @retval -EALREADY      A blocking request is already in progress.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_sensor_cli_setting_get(struct bt_mesh_sensor_cli *cli,
				   struct bt_mesh_msg_ctx *ctx,
				   const struct bt_mesh_sensor_type *sensor,
				   const struct bt_mesh_sensor_type *setting,
				   struct bt_mesh_sensor_setting_status *rsp);

/** @brief Read sensor data from all sensors on a server.
 *
 *  This call is blocking if the @c sensors buffer is non-NULL. Otherwise, this
 *  function will return, and the response will be passed to the
 *  bt_mesh_sensor_cli_handlers::data callback.
 *
 *  @param[in]  cli       Sensor client instance.
 *  @param[in]  ctx       Message context parameters, or NULL to use the
 *                        configured publish parameters.
 *  @param[out] sensors   Array of the sensors data to fill with the response, or NULL
 *                        to keep from blocking.
 *  @param[in,out] count  The number of elements in the @c sensors array.
 *                        Will be changed to reflect the resulting number
 *                        of elements in a list.
 *
 *  @retval 0              Successfully received the sensor data.
 *  @retval -ENODEV        The sensor server doesn't have the given sensor.
 *  @retval -EADDRNOTAVAIL A message context was not provided and publishing is
 *                         not configured.
 *  @retval -EAGAIN        The device has not been provisioned.
 *  @retval -ETIMEDOUT     The request timed out without a response.
 */
int bt_mesh_sensor_cli_all_get(struct bt_mesh_sensor_cli *cli,
			       struct bt_mesh_msg_ctx *ctx,
			       struct bt_mesh_sensor_data *sensors,
			       uint32_t *count);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op _bt_mesh_sensor_cli_op[];
extern const struct bt_mesh_model_cb _bt_mesh_sensor_cli_cb;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_SENSOR_CLI_H__ */

/** @} */
