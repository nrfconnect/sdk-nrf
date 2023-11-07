/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @defgroup bt_mesh_sensor Bluetooth mesh Sensors
 *  @{
 *  @brief API for Bluetooth mesh Sensors.
 */

#ifndef BT_MESH_SENSOR_H__
#define BT_MESH_SENSOR_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>
#include <zephyr/drivers/sensor.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Largest period divisor value allowed. */
#define BT_MESH_SENSOR_PERIOD_DIV_MAX 15
/** Largest sensor interval allowed. The value is represented as
 *  2 to the power of N milliseconds.
 */
#define BT_MESH_SENSOR_INTERVAL_MAX 26

/** String length for representing a single sensor channel. */
#define BT_MESH_SENSOR_CH_STR_LEN 19

/** Sensor sampling type.
 *
 *  Represents the sampling function used to produce the presented sensor value.
 */
enum bt_mesh_sensor_sampling {
	/** The sampling function is unspecified */
	BT_MESH_SENSOR_SAMPLING_UNSPECIFIED,
	/** The presented value is an instantaneous sample. */
	BT_MESH_SENSOR_SAMPLING_INSTANTANEOUS,
	/** The presented value is the arithmetic mean of multiple samples. */
	BT_MESH_SENSOR_SAMPLING_ARITHMETIC_MEAN,
	/** The presented value is the root mean square of multiple samples. */
	BT_MESH_SENSOR_SAMPLING_RMS,
	/** The presented value is the maximum of multiple samples. */
	BT_MESH_SENSOR_SAMPLING_MAXIMUM,
	/** The presented value is the minimum of multiple samples. */
	BT_MESH_SENSOR_SAMPLING_MINIMUM,
	/** The presented value is the accumulated moving average value of the
	 *  samples. The updating frequency of the moving average should be
	 *  indicated in @em bt_mesh_descriptor::update_interval. The total
	 *  measurement period should be indicated in @em
	 *  bt_mesh_descriptor::period.
	 */
	BT_MESH_SENSOR_SAMPLING_ACCUMULATED,
	/** The presented value is a count of events in a specific measurement
	 *  period. @em bt_mesh_descriptor::period should denote the
	 *  measurement period, or left to 0 to indicate that the sample is a
	 *  lifetime value.
	 */
	BT_MESH_SENSOR_SAMPLING_COUNT,
};

/** Sensor descriptor representing various static metadata for the sensor. */
struct bt_mesh_sensor_descriptor {
	/** Sensor measurement tolerance specification. */
	struct {
		/** Maximum positive measurement error (in percent).
		 *  A tolerance of 0 should be interpreted as "unspecified".
		 */
		struct sensor_value positive;
		/** Maximum negative measurement error (in percent).
		 *  A tolerance of 0 should be interpreted as "unspecified".
		 */
		struct sensor_value negative;
	} tolerance;
	/** Sampling type for the sensor data. */
	enum bt_mesh_sensor_sampling sampling_type;
	/** Measurement period for the samples, if applicable. */
	uint64_t period;
	/** Update interval for the samples, if applicable. */
	uint64_t update_interval;
};

/** Delta threshold type. */
enum bt_mesh_sensor_delta {
	/** Value based delta threshold.
	 *  The delta threshold values are represented as absolute value
	 *  changes.
	 */
	BT_MESH_SENSOR_DELTA_VALUE,
	/** Percent based delta threshold.
	 *  The delta threshold values are represented as percentages of their
	 *  old value (resolution: 0.01 %).
	 */
	BT_MESH_SENSOR_DELTA_PERCENT,
};

/** Sensor sampling cadence */
enum bt_mesh_sensor_cadence {
	/** Normal sensor publish interval. */
	BT_MESH_SENSOR_CADENCE_NORMAL,
	/** Fast sensor publish interval. */
	BT_MESH_SENSOR_CADENCE_FAST,
};

/** Sensor thresholds for publishing. */
struct bt_mesh_sensor_threshold {
	/** Delta threshold values.
	 *
	 *  Denotes the minimal sensor value change that should cause the sensor
	 *  to publish its value.
	 */
	struct {
		/** Type of delta threshold */
		enum bt_mesh_sensor_delta type;
		/** Minimal delta for a positive change. */
		struct sensor_value up;
		/** Minimal delta for a negative change. */
		struct sensor_value down;
	} delta;
	/** Range based threshold values.
	 *
	 *  Denotes the value range in which the sensor should be in fast
	 *  cadence mode.
	 */
	struct {
		/** Cadence when the sensor value is inside the range.
		 *
		 *  If the cadence is fast when the value is inside the range,
		 *  it's normal when it's outside the range. If the cadence is
		 *  normal when the value is inside the range, it's fast outside
		 *  the range.
		 */
		enum bt_mesh_sensor_cadence cadence;
		/** Lower boundary for the range based sensor cadence threshold.
		 */
		struct sensor_value low;
		/** Upper boundary for the range based sensor cadence threshold.
		 */
		struct sensor_value high;
	} range;
};

/** Unit for single sensor channel values. */
struct bt_mesh_sensor_unit {
	/** English name of the unit, e.g. "Decibel". */
	const char *name;
	/** Symbol of the unit, e.g. "dB". */
	const char *symbol;
};

/** Sensor channel value format. */
struct bt_mesh_sensor_format {
	/** @brief Sensor channel value encode function.
	 *
	 *  @param[in]  format Pointer to the format structure.
	 *  @param[in]  val    Sensor channel value to encode.
	 *  @param[out] buf    Buffer to encode the value into.
	 *
	 *  @return 0 on success, or (negative) error code otherwise.
	 */
	int (*const encode)(const struct bt_mesh_sensor_format *format,
			    const struct sensor_value *val,
			    struct net_buf_simple *buf);

	/** @brief Sensor channel value decode function.
	 *
	 *  @param[in]  format Pointer to the format structure.
	 *  @param[in]  buf    Buffer to decode the value from.
	 *  @param[out] val    Resulting sensor channel value.
	 *
	 *  @return 0 on success, or (negative) error code otherwise.
	 */
	int (*const decode)(const struct bt_mesh_sensor_format *format,
			    struct net_buf_simple *buf,
			    struct sensor_value *val);

	/** User data pointer. Used internally by the sensor types. */
	void *user_data;
	/** Size of the encoded data in bytes. */
	size_t size;

#ifdef CONFIG_BT_MESH_SENSOR_LABELS
	/** Pointer to the unit associated with this format. */
	const struct bt_mesh_sensor_unit *unit;
#endif
};

/** Signle sensor channel */
struct bt_mesh_sensor_channel {
	/** Format for this sensor channel. */
	const struct bt_mesh_sensor_format *format;
#ifdef CONFIG_BT_MESH_SENSOR_LABELS
	/** Name of this sensor channel. */
	const char *name;
#endif
};

/** Flag indicating this sensor type has a series representation. */
#define BT_MESH_SENSOR_TYPE_FLAG_SERIES BIT(0)

/** Sensor type. Should only be instantiated in sensor_types.c.
 *  See sensor_types.h for a list of all defined sensor types.
 */
struct bt_mesh_sensor_type {
	/** Device Property ID. */
	uint16_t id;
	/** Flags, @see BT_MESH_SENSOR_TYPE_FLAG_SERIES */
	uint8_t flags;
	/** The number of channels supported by this type. */
	uint8_t channel_count;
	/** Array of channel descriptors.
	 *
	 *  All channels are mandatory and immutable.
	 */
	const struct bt_mesh_sensor_channel *channels;
};

struct bt_mesh_sensor;
struct bt_mesh_sensor_srv;

/** Single sensor setting. */
struct bt_mesh_sensor_setting {
	/** Sensor type of this setting. */
	const struct bt_mesh_sensor_type *type;

	/** @brief Getter for this sensor setting.
	 *
	 *  @note This handler is mandatory.
	 *
	 *  @param[in]  srv     Sensor server instance associated with this
	 *                      setting.
	 *  @param[in]  sensor  Sensor this setting belongs to.
	 *  @param[in]  setting Pointer to this setting structure.
	 *  @param[in]  ctx     Context parameters for the packet this call
	 *                      originated from, or NULL if this call wasn't
	 *                      triggered by a packet.
	 *  @param[out] rsp     Response buffer for the setting value. Points to
	 *                      an array with the number of channels specified
	 *                      by the setting sensor type. All channels must be
	 *                      filled.
	 */
	void (*get)(struct bt_mesh_sensor_srv *srv,
		    struct bt_mesh_sensor *sensor,
		    const struct bt_mesh_sensor_setting *setting,
		    struct bt_mesh_msg_ctx *ctx,
		    struct sensor_value *rsp);

	/** @brief Setter for this sensor setting.
	 *
	 *  Should only be specified for writable sensor settings.
	 *
	 *  @param[in] srv     Sensor server instance associated with this
	 *                     setting.
	 *  @param[in] sensor  Sensor this setting belongs to.
	 *  @param[in] setting Pointer to this setting structure.
	 *  @param[in] ctx     Context parameters for the packet this call
	 *                     originated from, or NULL if this call wasn't
	 *                     triggered by a packet.
	 *  @param[in] value   New setting value. Contains the number of
	 *                     channels specified by the setting sensor type.
	 *
	 *  @return 0 on success, or (negative) error code otherwise.
	 */
	int (*set)(struct bt_mesh_sensor_srv *srv,
		struct bt_mesh_sensor *sensor,
		const struct bt_mesh_sensor_setting *setting,
		struct bt_mesh_msg_ctx *ctx,
		const struct sensor_value *value);
};

/** Single sensor series data column.
 *
 *  The series data columns represent a range for specific measurement values,
 *  inside which a set of sensor measurements were made. The range is
 *  interpreted as a half-open interval (i.e. start <= value < end).
 *
 *  @note Contrary to the Bluetooth mesh specification, the column has an end
 *        value instead of a width, to match the conventional property format.
 *        This reduces implementation complexity for sensor series values that
 *        include the start and end (or min and max) of the measurement range,
 *        as the value of the column can be copied directly into the
 *        corresponding channels.
 */
struct bt_mesh_sensor_column {
	/** Start of the column (inclusive). */
	struct sensor_value start;
	/** End of the column (exclusive). */
	struct sensor_value end;
};

/** Sensor series specification. */
struct bt_mesh_sensor_series {
	/** Pointer to the list of columns.
	 *
	 *  The columns may overlap, but the start value of each column must be
	 *  unique. The list of columns do not have to cover the entire valid
	 *  range, and values that don't fit in any of the columns should be
	 *  ignored. If columns overlap, samples must be present in all columns
	 *  they fall into. The columns may come in any order.
	 *
	 *  This list is not used for sensor types with one or two channels.
	 */
	const struct bt_mesh_sensor_column *columns;

	/** Number of columns. */
	uint32_t column_count;

	/** @brief Getter for the series values.
	 *
	 *  Should return the historical data for the latest sensor readings in
	 *  the given column.
	 *
	 *  @param[in]  srv          Sensor server associated with sensor instance.
	 *  @param[in]  sensor       Sensor pointer.
	 *  @param[in]  ctx          Message context pointer, or NULL if this call
	 *                           didn't originate from a mesh message.
	 *  @param[in]  column_index The index of the requested sensor column.
	 *                           Index into the @c columns array for sensors
	 *                           with more than two channels.
	 *  @param[out] value        Sensor value response buffer. Holds the number
	 *                           of channels indicated by the sensor type. All
	 *                           channels must be filled.
	 *
	 *  @return 0 on success, or (negative) error code otherwise.
	 */
	int (*get)(struct bt_mesh_sensor_srv *srv,
		struct bt_mesh_sensor *sensor,
		struct bt_mesh_msg_ctx *ctx,
		uint32_t column_index,
		struct sensor_value *value);
};

/** Sensor instance. */
struct bt_mesh_sensor {
	/** Sensor type.
	 *
	 *  Must be one of the specification defined types listed in @ref
	 *  bt_mesh_sensor_types.
	 */
	const struct bt_mesh_sensor_type *type;
	/** Optional sensor descriptor. */
	const struct bt_mesh_sensor_descriptor *descriptor;
	/** Sensor settings access specification. */
	const struct {
		/** Static array of sensor settings */
		const struct bt_mesh_sensor_setting *list;
		/** Number of sensor settings. */
		size_t count;
	} settings;

	/** Sensor series specification.
	 *
	 *  Only sensors who have a non-zero column-count and a defined
	 *  series getter will accept series messages. Sensors with more than
	 *  two channels also require a non-empty list of columns.
	 */
	const struct bt_mesh_sensor_series series;

	/** @brief Getter function for the sensor value.
	 *
	 *  @param[in]  srv    Sensor server associated with sensor instance.
	 *  @param[in]  sensor Sensor instance.
	 *  @param[in]  ctx    Message context, or NULL if the call wasn't
	 *                     triggered by a mesh message.
	 *  @param[out] rsp    Value response buffer. Fits the number of
	 *                     channels specified by the sensor type. All
	 *                     channels must be filled.
	 *
	 *  @return 0 on success, or (negative) error code otherwise.
	 */
	int (*const get)(struct bt_mesh_sensor_srv *srv,
			 struct bt_mesh_sensor *sensor,
			 struct bt_mesh_msg_ctx *ctx,
			 struct sensor_value *rsp);

	/* Internal state, overwritten on init. Should only be written to by
	 * internal modules.
	 */
	struct {
		/** Sensor threshold specification. */
		struct bt_mesh_sensor_threshold threshold;

		/** Linked list node. */
		sys_snode_t node;

		/** The previously published sensor value. */
		struct sensor_value prev;

		/** Sequence number of the previous publication. */
		uint16_t seq;

		/** Minimum possible interval for fast cadence value publishing.
		 *  The value is represented as 2 to the power of N milliseconds.
		 *
		 *  @see BT_MESH_SENSOR_INTERVAL_MAX
		 */
		uint8_t min_int;

		/** Fast period divisor used when publishing with fast cadence.
		 */
		uint8_t pub_div : 4;

		/** Flag indicating whether the sensor is in fast cadence mode.
		 */
		uint8_t fast_pub : 1;

		/** Flag indicating whether the sensor cadence state has been configured. */
		uint8_t configured : 1;
	} state;
};

/** @brief Check if a value change breaks the delta threshold.
 *
 *  Sensors should publish their value if the measured sample is outside the
 *  delta threshold compared to the previously published value. This function
 *  checks the threshold and the previously published value for this sensor,
 *  and returns whether the sensor should publish its value.
 *
 *  @note Only single-channel sensors support cadence. Multi-channel sensors are
 *        always considered out of their threshold range, and will always return
 *        true from this function. Single-channel sensors that haven't been
 *        assigned a threshold will return true if the value is different.
 *
 *  @param[in] sensor    The sensor instance.
 *  @param[in] value     Sensor value.
 *
 *  @return true if the difference between the measurements exceeds the delta
 *          threshold, false otherwise.
 */
bool bt_mesh_sensor_delta_threshold(const struct bt_mesh_sensor *sensor,
				    const struct sensor_value *value);

/** @brief Get the sensor type associated with the given Device Property ID.
 *
 *  Only known sensor types from @ref bt_mesh_sensor_types will be available.
 *  Sensor types can be made known to the sensor module by enabling
 *  @kconfig{CONFIG_BT_MESH_SENSOR_ALL_TYPES} or by referencing them in the
 *  application.
 *
 *  @param[in] id A Device Property ID.
 *
 *  @return The associated sensor type, or NULL if the ID is unknown.
 */
const struct bt_mesh_sensor_type *bt_mesh_sensor_type_get(uint16_t id);

/** @brief Check whether a single channel sensor value lies within a column.
 *
 *  @param[in] value Value to check. Only the first channel is considered.
 *  @param[in] col   Sensor column.
 *
 *  @return true if the value belongs in the column, false otherwise.
 */
bool bt_mesh_sensor_value_in_column(const struct sensor_value *value,
				    const struct bt_mesh_sensor_column *col);

/** @brief Get the format of the sensor column data.
 *
 *  @param[in] type Sensor type.
 *
 *  @return The sensor type's sensor column format if series access is
 *          supported. Otherwise NULL.
 */
const struct bt_mesh_sensor_format *
bt_mesh_sensor_column_format_get(const struct bt_mesh_sensor_type *type);

/** @brief Get a human readable representation of a single sensor channel.
 *
 *  @param[in]  ch  Sensor channel to represent.
 *  @param[out] str String buffer to fill. Should be @ref
 *                  BT_MESH_SENSOR_CH_STR_LEN bytes long.
 *  @param[in]  len Length of @c str buffer.
 *
 *  @return Number of bytes that should have been written if @c str is
 *          sufficiently large.
 */
static inline int bt_mesh_sensor_ch_to_str(const struct sensor_value *ch,
					   char *str, size_t len)
{
	return snprintk(str, len, "%s%u.%06u",
			((ch->val1 < 0 || ch->val2 < 0) ? "-" : ""),
			abs(ch->val1), abs(ch->val2));
}

/** @brief Get a human readable representation of a single sensor channel.
 *
 *  @note This function is not thread safe.
 *
 *  @param[in] ch Sensor channel to represent.
 *
 *  @return A string representing the sensor channel.
 */
const char *bt_mesh_sensor_ch_str(const struct sensor_value *ch);

/** @cond INTERNAL_HIDDEN */

#define BT_MESH_SENSOR_OP_DESCRIPTOR_GET BT_MESH_MODEL_OP_2(0x82, 0x30)
#define BT_MESH_SENSOR_OP_DESCRIPTOR_STATUS BT_MESH_MODEL_OP_1(0x51)
#define BT_MESH_SENSOR_OP_GET BT_MESH_MODEL_OP_2(0x82, 0x31)
#define BT_MESH_SENSOR_OP_STATUS BT_MESH_MODEL_OP_1(0x52)
#define BT_MESH_SENSOR_OP_COLUMN_GET BT_MESH_MODEL_OP_2(0x82, 0x32)
#define BT_MESH_SENSOR_OP_COLUMN_STATUS BT_MESH_MODEL_OP_1(0x53)
#define BT_MESH_SENSOR_OP_SERIES_GET BT_MESH_MODEL_OP_2(0x82, 0x33)
#define BT_MESH_SENSOR_OP_SERIES_STATUS BT_MESH_MODEL_OP_1(0x54)
#define BT_MESH_SENSOR_OP_CADENCE_GET BT_MESH_MODEL_OP_2(0x82, 0x34)
#define BT_MESH_SENSOR_OP_CADENCE_SET BT_MESH_MODEL_OP_1(0x55)
#define BT_MESH_SENSOR_OP_CADENCE_SET_UNACKNOWLEDGED BT_MESH_MODEL_OP_1(0x56)
#define BT_MESH_SENSOR_OP_CADENCE_STATUS BT_MESH_MODEL_OP_1(0x57)
#define BT_MESH_SENSOR_OP_SETTINGS_GET BT_MESH_MODEL_OP_2(0x82, 0x35)
#define BT_MESH_SENSOR_OP_SETTINGS_STATUS BT_MESH_MODEL_OP_1(0x58)
#define BT_MESH_SENSOR_OP_SETTING_GET BT_MESH_MODEL_OP_2(0x82, 0x36)
#define BT_MESH_SENSOR_OP_SETTING_SET BT_MESH_MODEL_OP_1(0x59)
#define BT_MESH_SENSOR_OP_SETTING_SET_UNACKNOWLEDGED BT_MESH_MODEL_OP_1(0x5A)
#define BT_MESH_SENSOR_OP_SETTING_STATUS BT_MESH_MODEL_OP_1(0x5B)

#ifndef CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX
#define CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX 0
#endif

#ifndef CONFIG_BT_MESH_SENSOR_CHANNELS_MAX
#define CONFIG_BT_MESH_SENSOR_CHANNELS_MAX 0
#endif

#ifndef CONFIG_BT_MESH_SENSOR_SRV_SENSORS_MAX
#define CONFIG_BT_MESH_SENSOR_SRV_SENSORS_MAX 0
#endif

#define BT_MESH_SENSOR_ENCODED_VALUE_MAXLEN                                    \
	(CONFIG_BT_MESH_SENSOR_CHANNELS_MAX *                                  \
	 CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX)

#define BT_MESH_SENSOR_STATUS_MAXLEN (3 + BT_MESH_SENSOR_ENCODED_VALUE_MAXLEN)

#define BT_MESH_SENSOR_MSG_MINLEN_DESCRIPTOR_GET 0
#define BT_MESH_SENSOR_MSG_MAXLEN_DESCRIPTOR_GET 2
#define BT_MESH_SENSOR_MSG_MINLEN_DESCRIPTOR_STATUS 2
#define BT_MESH_SENSOR_MSG_MAXLEN_DESCRIPTOR_STATUS 8
#define BT_MESH_SENSOR_MSG_MINLEN_GET 0
#define BT_MESH_SENSOR_MSG_MAXLEN_GET 2
#define BT_MESH_SENSOR_MSG_MINLEN_STATUS 0
#define BT_MESH_SENSOR_MSG_MINLEN_COLUMN_GET 2
#define BT_MESH_SENSOR_MSG_MAXLEN_COLUMN_GET                                   \
	(2 + CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX)
#define BT_MESH_SENSOR_MSG_MINLEN_COLUMN_STATUS 2
#define BT_MESH_SENSOR_MSG_MAXLEN_COLUMN_STATUS                                \
	(2 + CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX * 3)
#define BT_MESH_SENSOR_MSG_MINLEN_SERIES_GET 2
#define BT_MESH_SENSOR_MSG_MAXLEN_SERIES_GET                                   \
	(2 + 2 * CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX)
#define BT_MESH_SENSOR_MSG_MINLEN_SERIES_STATUS 2
#define BT_MESH_SENSOR_MSG_LEN_CADENCE_GET 2
#define BT_MESH_SENSOR_MSG_MINLEN_CADENCE_SET 8
#define BT_MESH_SENSOR_MSG_MAXLEN_CADENCE_SET                                  \
	(4 + CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX * 4)
#define BT_MESH_SENSOR_MSG_MINLEN_CADENCE_STATUS 2
#define BT_MESH_SENSOR_MSG_MAXLEN_CADENCE_STATUS                               \
	(4 + 4 * CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX)
#define BT_MESH_SENSOR_MSG_LEN_SETTINGS_GET 2
#define BT_MESH_SENSOR_MSG_MINLEN_SETTINGS_STATUS 2
#define BT_MESH_SENSOR_MSG_LEN_SETTING_GET 4
#define BT_MESH_SENSOR_MSG_MINLEN_SETTING_SET 4
#define BT_MESH_SENSOR_MSG_MAXLEN_SETTING_SET                                  \
	(4 + BT_MESH_SENSOR_ENCODED_VALUE_MAXLEN)
#define BT_MESH_SENSOR_MSG_MINLEN_SETTING_STATUS 4
#define BT_MESH_SENSOR_MSG_MAXLEN_SETTING_STATUS                               \
	(4 + BT_MESH_SENSOR_ENCODED_VALUE_MAXLEN)

#define BT_MESH_SENSOR_SRV_PUB_MAXLEN(_count)                                  \
	BT_MESH_MODEL_BUF_LEN(BT_MESH_SENSOR_OP_STATUS,                        \
			      MAX(CONFIG_BT_MESH_SENSOR_SRV_SENSORS_MAX,       \
				  _count) *                                    \
				      BT_MESH_SENSOR_STATUS_MAXLEN)

#define BT_MESH_SENSOR_SETUP_SRV_PUB_MAXLEN                                    \
	MAX(BT_MESH_MODEL_BUF_LEN(BT_MESH_SENSOR_OP_SETTING_STATUS,            \
				  BT_MESH_SENSOR_MSG_MAXLEN_SETTING_STATUS),   \
	    BT_MESH_MODEL_BUF_LEN(BT_MESH_SENSOR_OP_CADENCE_STATUS,            \
				  BT_MESH_SENSOR_MSG_MAXLEN_CADENCE_STATUS))
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_SENSOR_H__ */

/** @} */
