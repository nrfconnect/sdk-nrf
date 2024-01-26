/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @defgroup bt_mesh_sensor Bluetooth Mesh Sensors
 *  @{
 *  @brief API for Bluetooth Mesh Sensors.
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

#ifndef CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX
#define CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX 0
#endif

/** Largest period divisor value allowed. */
#define BT_MESH_SENSOR_PERIOD_DIV_MAX 15
/** Largest sensor interval allowed. The value is represented as
 *  2 to the power of N milliseconds.
 */
#define BT_MESH_SENSOR_INTERVAL_MAX 26

/** String length for representing a single sensor channel. */
#define BT_MESH_SENSOR_CH_STR_LEN 23

#if !defined(CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE) || defined(__DOXYGEN__)
/** @def BT_MESH_SENSOR_VALUE_IN_RANGE
 *
 *  @brief Returns whether or not encoded sensor value _value is in the range
 *         [_start, _end], inclusive.
 *
 *  @param[in] _value The value to check.
 *  @param[in] _start Start point of the range to check, inclusive.
 *  @param[in] _end End point of the range to check, inclusive.
 */
#define BT_MESH_SENSOR_VALUE_IN_RANGE(_value, _start, _end) (                  \
		(_value)->format->cb->compare((_value), (_start)) >= 0 &&      \
		(_value)->format->cb->compare((_end), (_value)) >= 0)
#else
#define BT_MESH_SENSOR_VALUE_IN_RANGE(_value, _start, _end) (                  \
		((_value)->val1 > (_start)->val1 ||                            \
		 ((_value)->val1 == (_start)->val1 &&                          \
		  (_value)->val2 >= (_start)->val2)) &&                        \
		((_value)->val1 < (_end)->val1 ||                              \
		 ((_value)->val1 == (_end)->val1 &&                            \
		  (_value)->val2 <= (_end)->val2)))
#endif

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

/** Sensor sampling cadence */
enum bt_mesh_sensor_cadence {
	/** Normal sensor publish interval. */
	BT_MESH_SENSOR_CADENCE_NORMAL,
	/** Fast sensor publish interval. */
	BT_MESH_SENSOR_CADENCE_FAST,
};

/** Unit for single sensor channel values. */
struct bt_mesh_sensor_unit {
	/** English name of the unit, e.g. "Decibel". */
	const char *name;
	/** Symbol of the unit, e.g. "dB". */
	const char *symbol;
};

struct bt_mesh_sensor_format;

/** Single sensor channel */
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

#if !defined(CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE) || defined(__DOXYGEN__)

/** Status of conversion from @ref bt_mesh_sensor_value. */
enum bt_mesh_sensor_value_status {
	/** The encoded sensor value represents a number. */
	BT_MESH_SENSOR_VALUE_NUMBER = 0,
	/** An error ocurred during conversion from the encoded sensor value. */
	BT_MESH_SENSOR_VALUE_CONVERSION_ERROR,
	/** The encoded sensor value represents an unknown value. */
	BT_MESH_SENSOR_VALUE_UNKNOWN,
	/** The encoded sensor value represents an invalid value. */
	BT_MESH_SENSOR_VALUE_INVALID,
	/** The encoded sensor value represents a value greater than or equal to
	 *  the format maximum.
	 */
	BT_MESH_SENSOR_VALUE_MAX_OR_GREATER,
	/** The encoded sensor value represents a value less than or equal to
	 *  the format minimum.
	 */
	BT_MESH_SENSOR_VALUE_MIN_OR_LESS,
	/** The encoded sensor value represents the total lifetime of the device. */
	BT_MESH_SENSOR_VALUE_TOTAL_DEVICE_LIFE,
};

/** Sensor value type representing the value and format of a single channel of
 *  sensor data.
 */
struct bt_mesh_sensor_value {
	/** The format the sensor value is encoded in. */
	const struct bt_mesh_sensor_format *format;
	/** Raw encoded sensor value, in little endian order. */
	uint8_t raw[CONFIG_BT_MESH_SENSOR_CHANNEL_ENCODED_SIZE_MAX];
};

/** @def BT_MESH_SENSOR_TOLERANCE_ENCODE
 *
 *  @brief Encode a sensor tolerance percentage.
 *
 *  @param[in] _percent The sensor tolerance to encode, in percent.
 */
#define BT_MESH_SENSOR_TOLERANCE_ENCODE(_percent) ((_percent) * 4095) / 100

/** Sensor descriptor representing various static metadata for the sensor. */
struct bt_mesh_sensor_descriptor {
	/** Sensor measurement tolerance specification. */
	struct {
		/** @brief Encoded maximum positive measurement error.
		 *
		 *  Represents the magnitude of the maximum possible positive
		 *  measurement error, in percent.
		 *
		 *  The value is encoded using the following formula:
		 *
		 *  encoded_value = (max_err_in_percent / 100) * 4095
		 *
		 *  A tolerance of 0 should be interpreted as "unspecified".
		 */
		uint16_t positive:12;
		/** @brief Encoded maximum negative measurement error.
		 *
		 *  Represents the magnitude of the maximum possible negative
		 *  measurement error, in percent.
		 *
		 *  The value is encoded using the following formula:
		 *
		 *  encoded_value = (max_err_in_percent / 100) * 4095
		 *
		 *  A tolerance of 0 should be interpreted as "unspecified".
		 */
		uint16_t negative:12;
	} tolerance;
	/** Sampling type for the sensor data. */
	enum bt_mesh_sensor_sampling sampling_type;
	/** Measurement period for the samples, if applicable. */
	uint64_t period;
	/** Update interval for the samples, if applicable. */
	uint64_t update_interval;
};

/* Sensor delta triggering values. */
struct bt_mesh_sensor_deltas {
	/** Minimal delta for a positive change. */
	struct bt_mesh_sensor_value up;
	/** Minimal delta for a negative change. */
	struct bt_mesh_sensor_value down;
};

/** Sensor thresholds for publishing. */
struct bt_mesh_sensor_threshold {
	/** Delta threshold values.
	 *
	 *  Denotes the minimal sensor value change that should cause the sensor
	 *  to publish its value.
	 */
	struct bt_mesh_sensor_deltas deltas;

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
		struct bt_mesh_sensor_value low;
		/** Upper boundary for the range based sensor cadence threshold.
		 */
		struct bt_mesh_sensor_value high;
	} range;
};

/** Single sensor series data column.
 *
 *  The series data columns represent a range for specific measurement values,
 *  inside which a set of sensor measurements were made. The range is
 *  interpreted as a half-open interval (i.e. start <= value < start + width).
 *
 */
struct bt_mesh_sensor_column {
	/** Start of the column (inclusive). */
	struct bt_mesh_sensor_value start;
	/** Width of the column. */
	struct bt_mesh_sensor_value width;
};

/** Sensor format callbacks. */
struct bt_mesh_sensor_format_cb {
	/** @brief Perform a delta check between two @ref bt_mesh_sensor_value
	 *         instances.
	 *
	 *  @c current and @c previous must have the same format.
	 *
	 *  @param[in] current  The current value.
	 *  @param[in] previous The previous sensor value to compare against.
	 *  @param[in] delta    The delta to use when checking.
	 *
	 *  @return @c true if the difference between @c current and @c previous
	 *          is bigger than the relevant delta specified in @c delta, @c
	 *          false otherwise.
	 */
	bool (*const delta_check)(const struct bt_mesh_sensor_value *current,
				  const struct bt_mesh_sensor_value *previous,
				  const struct bt_mesh_sensor_deltas *delta);

	/** @brief Compare two @ref bt_mesh_sensor_value instances.
	 *
	 *  @c op1 and @c op1 must have the same format.
	 *
	 *  @param[in] op1 The first value to compare.
	 *  @param[in] op2 The second value to compare.
	 *
	 *  @return 0 if @c op1 == @c op2, 1 if @c op1 > @c op2, -1 otherwise
	 *          (including if @c op1 and @c op2 are not comparable).
	 */
	int (*const compare)(const struct bt_mesh_sensor_value *op1,
			     const struct bt_mesh_sensor_value *op2);

	/** @brief Convert a @ref bt_mesh_sensor_value instance to an
	 *         integer in micro units.
	 *
	 *  If this function returns a status for which
	 *  @ref bt_mesh_sensor_value_status_is_numeric returns false, @c val is
	 *  not modified.
	 *
	 *  @param[in]  sensor_val The @ref bt_mesh_sensor_value to convert.
	 *  @param[out] val        The resulting integer.
	 *
	 *  @return The status of the conversion.
	 */
	enum bt_mesh_sensor_value_status (*const to_micro)(
		const struct bt_mesh_sensor_value *sensor_val, int64_t *val);

	/** @brief Convert an integer in micro units to a
	 *         @ref bt_mesh_sensor_value.
	 *
	 *  If @c val has a value that cannot be represented by the format,
	 *  @c sensor_val will be set to the value clamped to the range
	 *  supported by the format, and this function will return -ERANGE. This
	 *  will clamp to "Greater than or equal to the maximum value" and "Less
	 *  than or equal to the minimum value" if these are supported by the
	 *  format.
	 *
	 *  If this function returns an error code other than -ERANGE,
	 *  @c sensor_val is not modified.
	 *
	 *  @param[in]  format     Format to use when encoding the sensor value.
	 *  @param[in]  val        The integer to convert.
	 *  @param[out] sensor_val The resulting @ref bt_mesh_sensor_value.
	 *
	 *  @return 0 on success, (negative) error code otherwise.
	 */
	int (*const from_micro)(
		const struct bt_mesh_sensor_format *format, int64_t val,
		struct bt_mesh_sensor_value *sensor_val);

	/** @brief Convert a @ref bt_mesh_sensor_value to a @c float.
	 *
	 *  If this function returns a status for which
	 *  @ref bt_mesh_sensor_value_status_is_numeric returns false, @c val is
	 *  not modified.
	 *
	 *  @param[in]  sensor_val The @ref bt_mesh_sensor_value to convert.
	 *  @param[out] val        The resulting @c float.
	 *
	 *  @return The status of the conversion.
	 */
	enum bt_mesh_sensor_value_status (*const to_float)(
		const struct bt_mesh_sensor_value *sensor_val, float *val);

	/** @brief Convert a @c float to a @ref bt_mesh_sensor_value.
	 *
	 *  If @c val has a value that cannot be represented by the format,
	 *  @c sensor_val will be set to the value clamped to the range
	 *  supported by the format, and this function will return -ERANGE. This
	 *  will clamp to "Greater than or equal to the maximum value" and "Less
	 *  than or equal to the minimum value" if these are supported by the
	 *  format.
	 *
	 *  If this function returns an error code other than -ERANGE,
	 *  @c sensor_val is not modified.
	 *
	 *  @param[in]  format     Format to use when encoding the sensor value.
	 *  @param[in]  val        The @c float to convert.
	 *  @param[out] sensor_val The resulting @ref bt_mesh_sensor_value.
	 *
	 *  @return 0 on success, (negative) error code otherwise.
	 */
	int (*const from_float)(const struct bt_mesh_sensor_format *format,
				float val,
				struct bt_mesh_sensor_value *sensor_val);

	/** @brief Convert a special @ref bt_mesh_sensor_value_status value to a
	 *         @ref bt_mesh_sensor_value.
	 *
	 *  @param[in]  format     Format to use when encoding the sensor value.
	 *  @param[in]  status     The @ref bt_mesh_sensor_value_status
	 *                         to convert.
	 *  @param[out] sensor_val The resulting @ref bt_mesh_sensor_value on
	 *                         success. Undefined otherwise.
	 *
	 *  @return 0 on success, (negative) error code otherwise.
	 */
	int (*const from_special_status)(
		const struct bt_mesh_sensor_format *format,
		enum bt_mesh_sensor_value_status status,
		struct bt_mesh_sensor_value *sensor_val);

	/** @brief Get a human readable representation of a
	 *         @ref bt_mesh_sensor_value.
	 *
	 *  @param[in]  sensor_val Sensor value to represent.
	 *  @param[out] str        String buffer to fill. Should be
	 *                         @ref BT_MESH_SENSOR_CH_STR_LEN bytes long.
	 *  @param[in]  len        Length of @c str buffer.
	 *
	 *  @return The number of characters that would have been writen if
	 *          @c len had been big enough, or (negative) error code on
	 *          error.
	 */
	int (*const to_string)(const struct bt_mesh_sensor_value *sensor_val,
			       char *str, size_t len);

	/** @brief Check if a @ref bt_mesh_sensor_value lies within a
	 *         @ref bt_mesh_sensor_column.
	 *
	 *  @c sensor_val, @c col->start and @c col->width must all have the
	 *  same format.
	 *
	 *  If @c sensor_val, @c col->start or @c col->width represent a
	 *  non-numeric value, this will return @c false.
	 *
	 *  A value is considered to be in a column if
	 *
	 *  @b start <= @b value <= @b start + @b width
	 *
	 *  where @b start is the value represented by @c col->start, @b value
	 *  is the value represented by @c sensor_val, and @b width is the value
	 *  represented by @c col->width.
	 *
	 *  @param[in] sensor_val The @ref bt_mesh_sensor_value to check.
	 *  @param[in] col        The @ref bt_mesh_sensor_column to check
	 *                        against.
	 *
	 *  @return @c true if @c sensor_val, @c col->start and @c col->width
	 *          represent numeric values and @c sensor_val is inside the
	 *          range specified by @c col, inclusive. @c false otherwise.
	 */
	bool (*const value_in_column)(
		const struct bt_mesh_sensor_value *sensor_val,
		const struct bt_mesh_sensor_column *col);
};

/** Sensor channel value format. */
struct bt_mesh_sensor_format {
	/** Callbacks used for this format. */
	struct bt_mesh_sensor_format_cb *cb;
	/** User data pointer. Used internally by the sensor types. */
	void *user_data;
	/** Size of the encoded data in bytes. */
	size_t size;

#ifdef CONFIG_BT_MESH_SENSOR_LABELS
	/** Pointer to the unit associated with this format. */
	const struct bt_mesh_sensor_unit *unit;
#endif
};

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
		    struct bt_mesh_sensor_value *rsp);

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
		const struct bt_mesh_sensor_value *value);
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
		struct bt_mesh_sensor_value *value);
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
			 struct bt_mesh_sensor_value *rsp);

	/* Internal state, overwritten on init. Should only be written to by
	 * internal modules.
	 */
	struct {
		/** Sensor threshold specification. */
		struct bt_mesh_sensor_threshold threshold;

		/** Linked list node. */
		sys_snode_t node;

		/** The previously published sensor value. */
		struct bt_mesh_sensor_value prev;

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

/** @brief Compare two @ref bt_mesh_sensor_value instances.
 *
 *  @param[in] a The first value to compare.
 *  @param[in] b The second value to compare.
 *
 *  @return 0 if @c a == @c b, 1 if @c a > @c b, -1 otherwise (including if
 *          @c a and @c b are not comparable).
 */
int bt_mesh_sensor_value_compare(const struct bt_mesh_sensor_value *a,
				 const struct bt_mesh_sensor_value *b);

/** @brief Returns true if @c status is a value which can be represented by a
 *         number, meaning one of @c BT_MESH_SENSOR_VALUE_NUMBER,
 *         @c BT_MESH_SENSOR_VALUE_MIN_OR_LESS and
 *         @c BT_MESH_SENSOR_VALUE_MAX_OR_GREATER.
 *
 *  @param[in] status The value to check.
 *
 *  @return @c true if @c status is numeric, @c false otherwise.
 */
static inline bool
bt_mesh_sensor_value_status_is_numeric(enum bt_mesh_sensor_value_status status)
{
	return (status == BT_MESH_SENSOR_VALUE_NUMBER ||
		status == BT_MESH_SENSOR_VALUE_MIN_OR_LESS ||
		status == BT_MESH_SENSOR_VALUE_MAX_OR_GREATER);
}

/** @brief Convert a @ref bt_mesh_sensor_value to a @c float.
 *
 *  If this function returns a status for which
 *  @ref bt_mesh_sensor_value_status_is_numeric returns false, @c val is not
 *  modified.
 *
 *  @param[in]  sensor_val The @ref bt_mesh_sensor_value to convert.
 *  @param[out] val        The resulting @c float.
 *
 *  @return The status of the conversion.
 */
enum bt_mesh_sensor_value_status
bt_mesh_sensor_value_to_float(const struct bt_mesh_sensor_value *sensor_val,
			      float *val);

/** @brief Convert a @c float to a @ref bt_mesh_sensor_value.
 *
 *  If @c val has a value that cannot be represented by the format,
 *  @c sensor_val will be set to the value clamped to the range supported by
 *  the format, and this function will return -ERANGE. This will clamp to
 *  "Greater than or equal to the maximum value" and "Less than or equal to the
 *  minimum value" if these are supported by the format.
 *
 *  If this function returns an error code other than -ERANGE, @c sensor_val is
 *  not modified.
 *
 *  @param[in]  format     Format to use when encoding the sensor value.
 *  @param[in]  val        The @c float to convert.
 *  @param[out] sensor_val The resulting @ref bt_mesh_sensor_value.
 *
 *  @return 0 on success, (negative) error code otherwise.
 */
int bt_mesh_sensor_value_from_float(const struct bt_mesh_sensor_format *format,
				    float val,
				    struct bt_mesh_sensor_value *sensor_val);

/** @brief Convert a @ref bt_mesh_sensor_value instance to an integer in micro
 *         units.
 *
 *  If this function returns a status for which
 *  @ref bt_mesh_sensor_value_status_is_numeric returns false, @c val is not
 *  modified.
 *
 *  @param[in]  sensor_val The @ref bt_mesh_sensor_value to convert.
 *  @param[out] val        The resulting integer.
 *
 *  @return The status of the conversion.
 */
enum bt_mesh_sensor_value_status
bt_mesh_sensor_value_to_micro(
	const struct bt_mesh_sensor_value *sensor_val,
	int64_t *val);

/** @brief Convert an integer in micro units to a @ref bt_mesh_sensor_value.
 *
 *  If @c val has a value that cannot be represented by the format,
 *  @c sensor_val will be set to the value clamped to the range supported by
 *  the format, and this function will return -ERANGE. This will clamp to
 *  "Greater than or equal to the maximum value" and "Less than or equal to the
 *  minimum value" if these are supported by the format.
 *
 *  If this function returns an error code other than -ERANGE, @c sensor_val is
 *  not modified.
 *
 *  @param[in]  format     Format to use when encoding the sensor value.
 *  @param[in]  val        The integer to convert.
 *  @param[out] sensor_val The resulting @ref bt_mesh_sensor_value.
 *
 *  @return 0 on success, (negative) error code otherwise.
 */
int bt_mesh_sensor_value_from_micro(
	const struct bt_mesh_sensor_format *format,
	int64_t val,
	struct bt_mesh_sensor_value *sensor_val);

/** @brief Convert a @ref bt_mesh_sensor_value instance to a
 *         @c sensor_value (include/zephyr/drivers/sensor.h).
 *
 *  If this function returns a status for which
 *  @ref bt_mesh_sensor_value_status_is_numeric returns false, @c val is not
 *  modified.
 *
 *  @param[in]  sensor_val The @ref bt_mesh_sensor_value to convert.
 *  @param[out] val        The resulting @c sensor_value.
 *
 *  @return The status of the conversion.
 */
enum bt_mesh_sensor_value_status
bt_mesh_sensor_value_to_sensor_value(
	const struct bt_mesh_sensor_value *sensor_val,
	struct sensor_value *val);

/** @brief Convert a @c sensor_value (include/zephyr/drivers/sensor.h) instance
 *         to a @ref bt_mesh_sensor_value.
 *
 *  If @c val has a value that cannot be represented by the format,
 *  @c sensor_val will be set to the value clamped to the range supported by
 *  the format, and this function will return -ERANGE. This will clamp to
 *  "Greater than or equal to the maximum value" and "Less than or equal to the
 *  minimum value" if these are supported by the format.
 *
 *  If this function returns an error code other than -ERANGE, @c sensor_val is
 *  not modified.
 *
 *  @param[in]  format     Format to use when encoding the sensor value.
 *  @param[in]  val        The @c sensor_value to convert.
 *  @param[out] sensor_val The resulting @ref bt_mesh_sensor_value.
 *
 *  @return 0 on success, (negative) error code otherwise.
 */
int bt_mesh_sensor_value_from_sensor_value(
	const struct bt_mesh_sensor_format *format,
	const struct sensor_value *val,
	struct bt_mesh_sensor_value *sensor_val);

/** @brief Return a @ref bt_mesh_sensor_value_status describing the value in a
 *         @ref bt_mesh_sensor_value.
 *
 *  @param[in] sensor_val The value to return the status for.
 *
 *  @return The status describing the value.
 */
enum bt_mesh_sensor_value_status
bt_mesh_sensor_value_get_status(const struct bt_mesh_sensor_value *sensor_val);

/** @brief Convert a @ref bt_mesh_sensor_value_status value to a
 *         @ref bt_mesh_sensor_value.
 *
 *  This is useful for creating a @ref bt_mesh_sensor_value representing a
 *  special status value like @c BT_MESH_SENSOR_VALUE_UNKNOWN or
 *  @c BT_MESH_SENSOR_VALUE_TOTAL_DEVICE_LIFE.
 *
 *  This cannot be used to create a value representing
 *  @c BT_MESH_SENSOR_VALUE_NUMBER. Use one of
 *  @ref bt_mesh_sensor_value_from_sensor_value,
 *  @ref bt_mesh_sensor_value_from_micro or @ref bt_mesh_sensor_value_from_float
 *  instead.
 *
 *  Not all formats can represent all special status values. In the case where
 *  the supplied status value cannot be represented by the format, this function
 *  will return a (negative) error code.
 *
 *  @param[in]  format     Format to use when encoding the sensor value.
 *  @param[in]  status     The @ref bt_mesh_sensor_value_status value to
 *                         convert.
 *  @param[out] sensor_val The resulting @ref bt_mesh_sensor_value on success.
 *                         Unchanged otherwise.
 *
 *  @return 0 on success, (negative) error code otherwise.
 */
int bt_mesh_sensor_value_from_special_status(
	const struct bt_mesh_sensor_format *format,
	enum bt_mesh_sensor_value_status status,
	struct bt_mesh_sensor_value *sensor_val);

/** @brief Check whether a single channel sensor value lies within a column.
 *
 *  @param[in] value Value to check. Only the first channel is considered.
 *  @param[in] col   Sensor column.
 *
 *  @return true if the value belongs in the column, false otherwise.
 */
bool bt_mesh_sensor_value_in_column(const struct bt_mesh_sensor_value *value,
				    const struct bt_mesh_sensor_column *col);

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
int bt_mesh_sensor_ch_to_str(const struct bt_mesh_sensor_value *ch, char *str,
			     size_t len);

/** @brief Get a human readable representation of a single sensor channel.
 *
 *  @note This function is not thread safe.
 *
 *  @param[in] ch Sensor channel to represent.
 *
 *  @return A string representing the sensor channel.
 */
const char *bt_mesh_sensor_ch_str(const struct bt_mesh_sensor_value *ch);

#else /* defined(CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE) */
/** Sensor descriptor representing various static metadata for the sensor.
 */
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
 *  @note Contrary to the Bluetooth Mesh specification, the column has an end
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

/** @brief Check whether a single channel sensor value lies within a column.
 *
 *  @param[in] value Value to check. Only the first channel is considered.
 *  @param[in] col   Sensor column.
 *
 *  @return true if the value belongs in the column, false otherwise.
 */
bool bt_mesh_sensor_value_in_column(const struct sensor_value *value,
				    const struct bt_mesh_sensor_column *col);

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
#endif /* !defined(CONFIG_BT_MESH_SENSOR_USE_LEGACY_SENSOR_VALUE) */

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

/** @brief Get the format of the sensor column data.
 *
 *  @param[in] type Sensor type.
 *
 *  @return The sensor type's sensor column format if series access is
 *          supported. Otherwise NULL.
 */
const struct bt_mesh_sensor_format *
bt_mesh_sensor_column_format_get(const struct bt_mesh_sensor_type *type);

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
