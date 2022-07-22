/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/location.h>
#include <net/nrf_cloud.h>
#include "nrf_cloud_codec.h"
#include <stdio.h>
#include <date_time.h>
#include <cJSON.h>
#include <math.h>

#include "application.h"
#include "temperature.h"
#include "connection.h"

#include "location_tracking.h"

LOG_MODULE_REGISTER(application, CONFIG_MQTT_MULTI_SERVICE_LOG_LEVEL);

/* Timer used to time the sensor sampling rate. */
static K_TIMER_DEFINE(sensor_sample_timer, NULL, NULL);

/**
 * @brief Construct a data device message cJSON object with automatically generated timestamp.
 *
 * @param appid
 * @return cJSON* - the timestamped data device message object if successful, NULL otherwise.
 */
static cJSON *create_timestamped_data_message_object(const char *const appid)
{
	cJSON *msg_obj = NULL;
	int64_t timestamp;

	if (date_time_now(&timestamp)) {
		LOG_ERR("Failed to create timestamp for data message "
			"with appid %s", log_strdup(appid));
		return NULL;
	}

	msg_obj = cJSON_CreateObject();
	if (msg_obj == NULL) {
		LOG_ERR("Failed to create container object for timestamped data message "
			"with appid %s", log_strdup(appid));
		return NULL;
	}

	/* This structure corresponds to the General Message Schema described in the
	 * application-protocols repo:
	 * https://github.com/nRFCloud/application-protocols
	 */

	if ((cJSON_AddStringToObject(msg_obj, NRF_CLOUD_JSON_APPID_KEY, appid)  == NULL) ||
	    (cJSON_AddStringToObject(msg_obj, NRF_CLOUD_JSON_MSG_TYPE_KEY,
					      NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA) == NULL) ||
	    (cJSON_AddNumberToObject(msg_obj, NRF_CLOUD_MSG_TIMESTAMP_KEY,
					      (double)timestamp)		== NULL)) {
		LOG_ERR("Failed to populate timestamped data message object "
			"with appid %s", log_strdup(appid));
		cJSON_Delete(msg_obj);
		return NULL;
	}

	return msg_obj;
}

/**
 * @brief Transmit a collected sensor sample to nRF Cloud.
 *
 * @param sensor - The name of the sensor which was sampled.
 * @param value - The sampled sensor value.
 * @return int - 0 on success, negative error code otherwise.
 */
static int send_sensor_sample(const char *const sensor, double value)
{
	int ret = 0;

	/* Create a timestamped message container object for the sensor sample. */
	cJSON *msg_obj = create_timestamped_data_message_object(sensor);

	if (msg_obj == NULL) {
		ret = -EINVAL;
		goto cleanup;
	}

	/* Populate the container object with the sensor value. */
	if (cJSON_AddNumberToObject(msg_obj, NRF_CLOUD_JSON_DATA_KEY, value) == NULL) {
		ret = -ENOMEM;
		LOG_ERR("Failed to append value to %s sample container object ",
			log_strdup(sensor));
		goto cleanup;
	}

	/* Send the sensor sample container object as a device message. */
	ret = send_device_message_cJSON(msg_obj);

cleanup:
	if (msg_obj) {
		cJSON_Delete(msg_obj);
	}
	return ret;
}

/**
 * @brief Transmit a collected GNSS sample to nRF Cloud.
 *
 * @param loc_gnss - GNSS location data.
 * @return int - 0 on success, negative error code otherwise.
 */
static int send_gnss(const struct location_data * const loc_gnss)
{
	if (!loc_gnss || (loc_gnss->method != LOCATION_METHOD_GNSS)) {
		return -EINVAL;
	}

	int ret = 0;
	struct nrf_cloud_gnss_data gnss_pvt = {
		.type = NRF_CLOUD_GNSS_TYPE_PVT,
		.ts_ms = NRF_CLOUD_NO_TIMESTAMP,
		.pvt = {
			.lon		= loc_gnss->longitude,
			.lat		= loc_gnss->latitude,
			.accuracy	= loc_gnss->accuracy,
			.has_alt	= 0,
			.has_speed	= 0,
			.has_heading	= 0
		}
	};

	cJSON *msg_obj = cJSON_CreateObject();

	/* Add the timestamp */
	(void)date_time_now(&gnss_pvt.ts_ms);

	/* Encode the location data into a device message */
	ret = nrf_cloud_gnss_msg_json_encode(&gnss_pvt, msg_obj);

	if (ret == 0) {
		/* Send the location message */
		ret = send_device_message_cJSON(msg_obj);
	}

	if (msg_obj) {
		cJSON_Delete(msg_obj);
	}

	return ret;
}

/**
 * @brief Callback to receive periodic location updates from location_tracking.c and forward them
 * to nRF Cloud.
 *
 * Note that cellular positioning (MCELL/Multi-Cell and SCELL/Single-Cell) is sent to nRF
 * Cloud automatically (since the Location library and nRF Cloud must work together to
 * determine them in the first place). GNSS positions, on the other hand, must be
 * sent manually, since they are determined entirely on-device.
 *
 * @param location_data - The received location update.
 *
 */
static void on_location_update(const struct location_data * const location_data)
{
	LOG_INF("Location Updated: %.06f N %.06f W, accuracy: %.01f m, Method: %s",
		location_data->latitude, location_data->longitude, location_data->accuracy,
		location_data->method == LOCATION_METHOD_CELLULAR	? "Cellular" :
		location_data->method == LOCATION_METHOD_GNSS		? "GNSS"     :
		location_data->method == LOCATION_METHOD_WIFI		? "WIFI"     :
									  "Invalid");

	/* If the position update was derived using GNSS, send it onward to nRF Cloud. */
	if (location_data->method == LOCATION_METHOD_GNSS) {
		LOG_INF("GNSS Position Update! Sending to nRF Cloud...");
		send_gnss(location_data);
	}
}

void main_application(void)
{
	/* Wait for the date and time to become known.
	 * This is needed both for location services and for sensor sample timestamping.
	 */
	LOG_INF("Waiting for modem to determine current date and time");
	if (!await_date_time_known(K_SECONDS(CONFIG_DATE_TIME_ESTABLISHMENT_TIMEOUT_SECONDS))) {
		LOG_WRN("Failed to determine valid date time. Proceeding anyways");
	} else {
		LOG_INF("Current date and time determined");
	}

	/* Begin tracking location at the configured interval. */
	(void)start_location_tracking(on_location_update,
					CONFIG_LOCATION_TRACKING_SAMPLE_INTERVAL_SECONDS);

	int counter = 0;

	/* Begin sampling sensors. */
	while (true) {
		/* Start the sensor sample interval timer.
		 * We use a timer here instead of merely sleeping the thread, because the
		 * application thread can be preempted by other threads performing long tasks
		 * (such as periodic location acquisition), and we want to account for these
		 * delays when metering the sample send rate.
		 */
		k_timer_start(&sensor_sample_timer,
			K_SECONDS(CONFIG_SENSOR_SAMPLE_INTERVAL_SECONDS), K_FOREVER);

		if (IS_ENABLED(CONFIG_TEMP_TRACKING)) {
			double temp = -1;

			if (get_temperature(&temp) == 0) {
				LOG_INF("Temperature is %d degrees C", (int)temp);
				(void)send_sensor_sample(NRF_CLOUD_JSON_APPID_VAL_TEMP, temp);
			}
		}

		if (IS_ENABLED(CONFIG_TEST_COUNTER)) {
			(void)send_sensor_sample("COUNT", counter++);
		}

		/* Wait out any remaining time on the sample interval timer. */
		k_timer_status_sync(&sensor_sample_timer);
	}
}
