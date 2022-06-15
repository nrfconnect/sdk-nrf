/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <cloud_codec.h>
#include <zephyr/kernel.h>
#include <cJSON.h>
#include <cJSON_os.h>
#include <math.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cloud_codec_ringbuffer, CONFIG_CLOUD_CODEC_LOG_LEVEL);

/**
 * @brief Define cloud codec ringbuffer populate functions.
 *
 * Each cloud codec buffer needs to have one of those.
 * The function will check CONFIG_DATA_SOMETHING_BUFFER_STORE
 * whether that type of data should be stored at all
 * and if the supplied data is marked as valid using the "queued" flag.
 * The head of the buffer is moved to the next element
 * and the data is copied into the buffer.
 */
#define CLOUD_CODEC_RINGBUFFER_POPULATE_DEFINE(name, NAME, type) \
	void cloud_codec_populate_##name##_buffer( \
		type buffer, \
		type new_data, \
		int *head_buf, \
		size_t buffer_count) \
	{ \
	if (!IS_ENABLED(CONFIG_DATA_##NAME##_BUFFER_STORE)) { \
		return; \
	} \
	if (!new_data->queued) { \
		return; \
	} \
	*head_buf += 1; \
	/* Go to start of buffer if end is reached. */ \
	if (*head_buf == buffer_count) { \
		*head_buf = 0; \
	} \
	buffer[*head_buf] = *new_data; \
	LOG_DBG("Entry: %d of %d in %s buffer filled", \
		*head_buf, buffer_count - 1, #name); \
	}

CLOUD_CODEC_RINGBUFFER_POPULATE_DEFINE(sensor, SENSOR, struct cloud_data_sensors *)
CLOUD_CODEC_RINGBUFFER_POPULATE_DEFINE(ui, UI, struct cloud_data_ui *)
CLOUD_CODEC_RINGBUFFER_POPULATE_DEFINE(accel, ACCELEROMETER, struct cloud_data_accelerometer *)
CLOUD_CODEC_RINGBUFFER_POPULATE_DEFINE(bat, BATTERY, struct cloud_data_battery *)
CLOUD_CODEC_RINGBUFFER_POPULATE_DEFINE(gnss, GNSS, struct cloud_data_gnss *)
CLOUD_CODEC_RINGBUFFER_POPULATE_DEFINE(modem_dynamic, MODEM, struct cloud_data_modem_dynamic *)
