/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/location.h>
#include <nrf_modem_at.h>
#include <nrf_errno.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_alerts.h>
#include <date_time.h>
#include <cJSON.h>
#include <stdio.h>

#include "nrf_cloud_codec_internal.h"
#include "application.h"
#include "temperature.h"
#include "connection.h"

#include "location_tracking.h"
#include "led_control.h"

LOG_MODULE_REGISTER(application, CONFIG_MQTT_MULTI_SERVICE_LOG_LEVEL);

/* Timer used to time the sensor sampling rate. */
static K_TIMER_DEFINE(sensor_sample_timer, NULL, NULL);

/* AT command request error handling */
#define AT_CMD_REQUEST_ERR_FORMAT "Error while processing AT command request: %d"
#define AT_CMD_REQUEST_ERR_MAX_LEN (sizeof(AT_CMD_REQUEST_ERR_FORMAT) + 20)
BUILD_ASSERT(CONFIG_AT_CMD_REQUEST_RESPONSE_BUFFER_LENGTH >= AT_CMD_REQUEST_ERR_MAX_LEN,
	     "Not enough AT command response buffer for printing error events.");

/* Temperature alert limits. */
#define TEMP_ALERT_LIMIT CONFIG_TEMP_ALERT_LIMIT
#define TEMP_ALERT_HYSTERESIS 2
#define TEMP_ALERT_LOWER_LIMIT (TEMP_ALERT_LIMIT + TEMP_ALERT_HYSTERESIS)

/* Buffer to contain modem responses when performing AT command requests */
static char at_req_resp_buf[CONFIG_AT_CMD_REQUEST_RESPONSE_BUFFER_LENGTH];

/**
 * @brief Construct a device message cJSON object with automatically generated timestamp
 *
 * The resultant cJSON object will be conformal to the General Message Schema described in the
 * application-protocols repo:
 *
 * https://github.com/nRFCloud/application-protocols
 *
 * @param appid - The appId for the device message
 * @param msg_type - The messageType for the device message
 * @return cJSON* - the timestamped data device message object if successful, NULL otherwise.
 */
static cJSON *create_timestamped_device_message(const char *const appid, const char *const msg_type)
{
	cJSON *msg_obj = NULL;
	int64_t timestamp;

	/* Acquire timestamp */
	if (date_time_now(&timestamp)) {
		LOG_ERR("Failed to create timestamp for data message "
			"with appid %s", appid);
		return NULL;
	}

	/* Create container object */
	msg_obj = json_create_req_obj(appid, msg_type);
	if (msg_obj == NULL) {
		LOG_ERR("Failed to create container object for timestamped data message "
			"with appid %s and message type %s", appid, msg_type);
		return NULL;
	}

	/* Add timestamp to container object */
	if (!cJSON_AddNumberToObject(msg_obj, NRF_CLOUD_MSG_TIMESTAMP_KEY, (double)timestamp)) {
		LOG_ERR("Failed to add timestamp to data message with appid %s and message type %s",
			appid, msg_type);
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
	cJSON *msg_obj = create_timestamped_device_message(
		sensor, NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA
	);

	if (msg_obj == NULL) {
		ret = -EINVAL;
		goto cleanup;
	}

	/* Populate the container object with the sensor value. */
	if (cJSON_AddNumberToObject(msg_obj, NRF_CLOUD_JSON_DATA_KEY, value) == NULL) {
		ret = -ENOMEM;
		LOG_ERR("Failed to append value to %s sample container object ",
			sensor);
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
static int send_gnss(const struct location_event_data * const loc_gnss)
{
	if (!loc_gnss || (loc_gnss->method != LOCATION_METHOD_GNSS)) {
		return -EINVAL;
	}

	int ret = 0;
	struct nrf_cloud_gnss_data gnss_pvt = {
		.type = NRF_CLOUD_GNSS_TYPE_PVT,
		.ts_ms = NRF_CLOUD_NO_TIMESTAMP,
		.pvt = {
			.lon		= loc_gnss->location.longitude,
			.lat		= loc_gnss->location.latitude,
			.accuracy	= loc_gnss->location.accuracy,
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
static void on_location_update(const struct location_event_data * const location_data)
{
	LOG_INF("Location Updated: %.06f N %.06f W, accuracy: %.01f m, Method: %s",
		location_data->location.latitude,
		location_data->location.longitude,
		location_data->location.accuracy,
		location_method_str(location_data->method));

	/* If the position update was derived using GNSS, send it onward to nRF Cloud. */
	if (location_data->method == LOCATION_METHOD_GNSS) {
		LOG_INF("GNSS Position Update! Sending to nRF Cloud...");
		send_gnss(location_data);
	}
}

/**
 * @brief Receives general device messages from nRF Cloud, checks if they are AT command requests,
 * and performs them if so, transmitting the modem response back to nRF Cloud.
 *
 * Try sending {"appId":"MODEM", "messageType":"CMD", "data":"AT+CGMR"}
 * in the nRF Cloud Portal Terminal card.
 *
 * @param msg - The device message to check.
 */
static void handle_at_cmd_requests(const char *const msg)
{
	/* Attempt to parse the message as if it is JSON */
	struct cJSON *msg_obj = cJSON_Parse(msg);

	if (!msg_obj) {
		/* The message isn't JSON or otherwise couldn't be parsed. */
		LOG_DBG("A general topic device message of length %d could not be parsed.",
			msg ? strlen(msg) : 0);
		return;
	}

	/* Check that we are actually dealing with an AT command request */
	char *msg_appid =
		cJSON_GetStringValue(cJSON_GetObjectItem(msg_obj, NRF_CLOUD_JSON_APPID_KEY));
	char *msg_type =
		cJSON_GetStringValue(cJSON_GetObjectItem(msg_obj, NRF_CLOUD_JSON_MSG_TYPE_KEY));
	if (!msg_appid || !msg_type ||
	    (strcmp(msg_appid, NRF_CLOUD_JSON_APPID_VAL_MODEM)  != 0) ||
	    (strcmp(msg_type,  NRF_CLOUD_JSON_MSG_TYPE_VAL_CMD) != 0)) {
		goto cleanup;
	}

	/* If it is, extract the command string */
	char *cmd =
		cJSON_GetStringValue(cJSON_GetObjectItem(msg_obj, NRF_CLOUD_JSON_DATA_KEY));

	if (!cmd) {
		/* Missing or invalid command value will be treated as a blank command */
		cmd = "";
	}

	/* Execute the command and receive the result */
	LOG_DBG("Modem AT command requested: %s", cmd);
	memset(at_req_resp_buf, 0, sizeof(at_req_resp_buf));

	/* We must pass the command in using a format specifier it might contain special characters
	 * such as %.
	 *
	 * We subtract 1 from the passed-in response buffer length to ensure that the response is
	 * always null-terminated, even when the response is longer than the response buffer size.
	 */
	int err = nrf_modem_at_cmd(at_req_resp_buf, sizeof(at_req_resp_buf) - 1, "%s", cmd);

	LOG_DBG("Modem AT command response (%d, %d): %s",
		nrf_modem_at_err_type(err), nrf_modem_at_err(err), at_req_resp_buf);

	/* Trim \r\n from modem response for better readability in the portal. */
	at_req_resp_buf[MAX(0, strlen(at_req_resp_buf) - 2)] = '\0';

	/* If an error occurred with the request, report it */
	if (err < 0) {
		/* Negative error codes indicate an error with the modem lib itself, so the
		 * response buffer will be empty (or filled with junk). Thus, we can print the
		 * error message directly into it.
		 */
		snprintf(at_req_resp_buf, sizeof(at_req_resp_buf), AT_CMD_REQUEST_ERR_FORMAT, err);
		LOG_ERR("%s", at_req_resp_buf);
	}

	/* Free the old container object and create a new one to contain our response */
	cJSON_Delete(msg_obj);
	msg_obj = create_timestamped_device_message(
		NRF_CLOUD_JSON_APPID_VAL_MODEM,
		NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA
	);

	if (!msg_obj) {
		return;
	}

	/* Populate response with command result */
	if (!cJSON_AddStringToObject(msg_obj, NRF_CLOUD_JSON_DATA_KEY, at_req_resp_buf)) {
		LOG_ERR("Failed to populate AT CMD response with modem response data");
		goto cleanup;
	}

	/* Send the response */
	err = send_device_message_cJSON(msg_obj);

	if (err) {
		LOG_ERR("Failed to send AT CMD request response, error: %d", err);
	}

cleanup:
	cJSON_Delete(msg_obj);
}

/** @brief Check whether temperature is acceptable.
 * If the device exceeds a temperature limit, send the temperature alert one time.
 * Once the temperature falls below a lower limit, re-enable the temperature alert
 * so it will be sent if limit is exceeded again.
 *
 * The difference between the two limits should be sufficient to prevent sending
 * new alerts if the temperature value oscillates between two nearby values.
 *
 * @param temp - The current device temperature.
 */
static void monitor_temperature(double temp)
{
	static bool temperature_alert_active;

	if ((temp > TEMP_ALERT_LIMIT) && !temperature_alert_active) {
		temperature_alert_active = true;
		(void)nrf_cloud_alert_send(ALERT_TYPE_TEMPERATURE, (float)temp,
					   "Temperature over limit!");
	} else if (temp < TEMP_ALERT_LOWER_LIMIT) {
		temperature_alert_active = false;
	}
}

void main_application_thread_fn(void)
{
	if (IS_ENABLED(CONFIG_AT_CMD_REQUESTS)) {
		/* Register with connection.c to receive general device messages and check them for
		 * AT command requests.
		 */
		register_general_dev_msg_handler(handle_at_cmd_requests);
	}

	/* Wait for first connection before starting the application. */
	(void)await_connection(K_FOREVER);

	(void)nrf_cloud_alert_send(ALERT_TYPE_DEVICE_NOW_ONLINE, 0, NULL);

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

				monitor_temperature(temp);
			}
		}

		if (IS_ENABLED(CONFIG_TEST_COUNTER)) {
			(void)send_sensor_sample("COUNT", counter++);
		}

		/* Wait out any remaining time on the sample interval timer. */
		k_timer_status_sync(&sensor_sample_timer);
	}
}
