/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <cloud_codec.h>
#include <stdbool.h>
#include <sys/ring_buffer.h>
#include "buffer.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(buffer, CONFIG_CLOUD_CODEC_LOG_LEVEL);

/* Declare ringbuffers and allocate space depending on configured
 * max buffer sizes.
 */
RING_BUF_ITEM_DECLARE_SIZE(ui_buf, sizeof(struct cloud_data_ui) *
				  CONFIG_BUFFER_UI_MAX);

RING_BUF_ITEM_DECLARE_SIZE(modem_dyn_buf,
			   sizeof(struct cloud_data_modem_dynamic) *
				  CONFIG_BUFFER_MODEM_DYNAMIC_MAX);

RING_BUF_ITEM_DECLARE_SIZE(modem_stat_buf,
			   sizeof(struct cloud_data_modem_static) *
				  CONFIG_BUFFER_MODEM_STATIC_MAX);

RING_BUF_ITEM_DECLARE_SIZE(sensor_buf,
			   sizeof(struct cloud_data_sensors) *
				  CONFIG_BUFFER_SENSOR_MAX);

RING_BUF_ITEM_DECLARE_SIZE(accel_buf,
			   sizeof(struct cloud_data_accelerometer) *
				  CONFIG_BUFFER_ACCEL_MAX);

RING_BUF_ITEM_DECLARE_SIZE(battery_buf,
			   sizeof(struct cloud_data_battery) *
				  CONFIG_BUFFER_BATTERY_MAX);

RING_BUF_ITEM_DECLARE_SIZE(gps_buf,
			   sizeof(struct cloud_data_gps) *
				  CONFIG_BUFFER_GPS_MAX);

/* General information associated with a buffer. */
struct buffer_info {
	/* Pointer to ringbuffer. */
	struct ring_buf *buf;
	/* Length of data item in ringbuffer. */
	size_t len;
	/* Flag signifying if the last known item is valid. */
	bool last_known_valid;

	/* Last known item. Retains the last known data item that is added to
	 * the buffer.
	 */
	union {
		struct cloud_data_ui ui;
		struct cloud_data_modem_static modem_static;
		struct cloud_data_modem_dynamic modem_dynamic;
		struct cloud_data_sensors sensor;
		struct cloud_data_accelerometer accel;
		struct cloud_data_gps gps;
		struct cloud_data_battery battery;
	} last_known;

	/* Name of the buffer */
	char *name;
};

/* List containing static variables associated with a buffer. */
static struct buffer_info buf_list[BUFFER_COUNT] = {
	[BUFFER_UI].buf		    = &ui_buf,
	[BUFFER_UI].name	    = "BUFFER_UI",
	[BUFFER_UI].len		    = sizeof(struct cloud_data_ui),

	[BUFFER_MODEM_STATIC].buf   = &modem_stat_buf,
	[BUFFER_MODEM_STATIC].name  = "BUFFER_MODEM_STATIC",
	[BUFFER_MODEM_STATIC].len   = sizeof(struct cloud_data_modem_static),

	[BUFFER_MODEM_DYNAMIC].buf  = &modem_dyn_buf,
	[BUFFER_MODEM_DYNAMIC].name = "BUFFER_MODEM_DYNAMIC",
	[BUFFER_MODEM_DYNAMIC].len  = sizeof(struct cloud_data_modem_dynamic),

	[BUFFER_GPS].buf	    = &gps_buf,
	[BUFFER_GPS].name	    = "BUFFER_GPS",
	[BUFFER_GPS].len	    = sizeof(struct cloud_data_gps),

	[BUFFER_SENSOR].buf	    = &sensor_buf,
	[BUFFER_SENSOR].name	    = "BUFFER_SENSOR",
	[BUFFER_SENSOR].len	    = sizeof(struct cloud_data_sensors),

	[BUFFER_ACCELEROMETER].buf  = &accel_buf,
	[BUFFER_ACCELEROMETER].name = "BUFFER_ACCELEROMETER",
	[BUFFER_ACCELEROMETER].len  = sizeof(struct cloud_data_accelerometer),

	[BUFFER_BATTERY].buf	    = &battery_buf,
	[BUFFER_BATTERY].name	    = "BUFFER_BATTERY",
	[BUFFER_BATTERY].len	    = sizeof(struct cloud_data_battery),
};

static void buffer_last_known_clear_entry(enum buffer_type type)
{
	buf_list[type].last_known_valid = false;
}

static int buffer_last_known_set(enum buffer_type type, void *data, size_t len)
{
	if (data == NULL) {
		LOG_WRN("Pointer to data cannot be NULL");
		return -EINVAL;
	}

	if ((len <= 0) || (len != buffer_get_item_size(type))) {
		LOG_ERR("Passed in length not valid");
		return -EMSGSIZE;
	}

	switch (type) {
	case BUFFER_UI:
		memcpy(&buf_list[type].last_known.ui, data, len);
		buf_list[type].last_known_valid = true;
		break;
	case BUFFER_MODEM_STATIC:
		memcpy(&buf_list[type].last_known.modem_static, data, len);
		buf_list[type].last_known_valid = true;
		break;
	case BUFFER_MODEM_DYNAMIC:
		memcpy(&buf_list[type].last_known.modem_dynamic, data, len);
		buf_list[type].last_known_valid = true;
		break;
	case BUFFER_ACCELEROMETER:
		memcpy(&buf_list[type].last_known.accel, data, len);
		buf_list[type].last_known_valid = true;
		break;
	case BUFFER_GPS:
		memcpy(&buf_list[type].last_known.gps, data, len);
		buf_list[type].last_known_valid = true;
		break;
	case BUFFER_SENSOR:
		memcpy(&buf_list[type].last_known.sensor, data, len);
		buf_list[type].last_known_valid = true;
		break;
	case BUFFER_BATTERY:
		memcpy(&buf_list[type].last_known.battery, data, len);
		buf_list[type].last_known_valid = true;
		break;
	default:
		LOG_WRN("Unknown buffer");
		return -ENODATA;
	}

	return 0;
}

/* Public interface */
bool buffer_last_known_valid(enum buffer_type type)
{
	return buf_list[type].last_known_valid;
}

bool buffer_last_known_get(enum buffer_type type, void *data)
{
	if (data == NULL) {
		LOG_WRN("Pointer to data cannot be NULL");
		return -EINVAL;
	}

	if (!buffer_last_known_valid(type)) {
		return false;
	}

	switch (type) {
	case BUFFER_UI:
		*(struct cloud_data_ui **)data =
				&buf_list[type].last_known.ui;
		break;
	case BUFFER_MODEM_STATIC:
		*(struct cloud_data_modem_static **)data =
				&buf_list[type].last_known.modem_static;
		break;
	case BUFFER_MODEM_DYNAMIC:
		*(struct cloud_data_modem_dynamic **)data =
				&buf_list[type].last_known.modem_dynamic;
		break;
	case BUFFER_ACCELEROMETER:
		*(struct cloud_data_accelerometer **)data =
				&buf_list[type].last_known.accel;
		break;
	case BUFFER_GPS:
		*(struct cloud_data_gps **)data =
				&buf_list[type].last_known.gps;
		break;
	case BUFFER_SENSOR:
		*(struct cloud_data_sensors **)data =
				&buf_list[type].last_known.sensor;
		break;
	case BUFFER_BATTERY:
		*(struct cloud_data_battery **)data =
				&buf_list[type].last_known.battery;
		break;
	default:
		LOG_WRN("Unknown buffer");
		return false;
	}

	buffer_last_known_clear_entry(type);

	return true;
}

bool buffer_is_empty(enum buffer_type type)
{
	return ring_buf_is_empty(buf_list[type].buf);
}

int buffer_add_data(enum buffer_type type, void *data, size_t len)
{
	if (data == NULL) {
		LOG_WRN("Pointer to data cannot be NULL");
		return -EINVAL;
	}

	if ((len <= 0) || (len != buffer_get_item_size(type))) {
		LOG_ERR("Passed in length not valid");
		return -EMSGSIZE;
	}

	/* If the last known data item is valid place it in the ringbuffer and
	 * update the last known data item for the corresponding buffer with
	 * the incomding data.
	 */
	if (!buf_list[type].last_known_valid) {
		int err = buffer_last_known_set(type, data, len);

		if (err) {
			LOG_WRN("buffer_last_known_set, error: %d", err);
			return err;
		}
	} else {
		void *item = NULL;
		size_t rb_len;
		/* Check if buffer is full. If so replace oldest data item. */
		if (ring_buf_space_get(buf_list[type].buf) == 0) {
			int err = buffer_get_data_finished(type, buf_list[type].len);

			if (err) {
				LOG_ERR("buffer_get_data_finished, error: %d", err);
				return err;
			}

			LOG_DBG("Oldest entry in %s buffer removed",
				buf_list[type].name);
		}

		/* Place the last known item into the ringbuffer. */
		buffer_last_known_get(type, &item);

		rb_len = ring_buf_put(buf_list[type].buf,
				      item,
				      buf_list[type].len);
		if (rb_len != buf_list[type].len) {
			return -ENOMEM;
		}

		/* Update the last known item with the incoming item. */
		int err = buffer_last_known_set(type, data, len);

		if (err) {
			LOG_WRN("buffer_last_known_set, error: %d", err);
			return err;
		}
	}

	return 0;
}

int buffer_get_data(enum buffer_type type, void *data, size_t len)
{
	if (data == NULL) {
		LOG_WRN("Pointer to data cannot be NULL");
		return -EINVAL;
	}

	if ((len <= 0) || (len != buffer_get_item_size(type))) {
		LOG_ERR("Passed in length not valid");
		return -EMSGSIZE;
	}

	uint32_t rb_len = ring_buf_get_claim(buf_list[type].buf,
					     (uint8_t **)data,
					     len);

	if (rb_len != len) {
		return -ENOMEM;
	}

	return 0;
}

int buffer_get_data_finished(enum buffer_type type, size_t len)
{
	if ((len <= 0) || (len != buffer_get_item_size(type))) {
		LOG_ERR("Passed in length not valid");
		return -EMSGSIZE;
	}

	return ring_buf_get_finish(buf_list[type].buf, len);
}

uint32_t buffer_get_item_size(enum buffer_type type)
{
	return buf_list[type].len;
}
