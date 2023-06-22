/* Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/location.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_codec.h>
#include <net/nrf_cloud_log.h>
#include <net/nrf_cloud_alert.h>
#include <date_time.h>
#include <stdio.h>

#include "application.h"
#include "temperature.h"
#include "connection.h"

#include "location_tracking.h"
#include "led_control.h"
#include "at_commands.h"

LOG_MODULE_REGISTER(application, CONFIG_MQTT_MULTI_SERVICE_LOG_LEVEL);

/* Timer used to time the sensor sampling rate. */
static K_TIMER_DEFINE(sensor_sample_timer, NULL, NULL);

/* AT command request error handling */
#define AT_CMD_REQUEST_ERR_FORMAT "Error while processing AT command request: %d"
#define AT_CMD_REQUEST_ERR_MAX_LEN (sizeof(AT_CMD_REQUEST_ERR_FORMAT) + 20)
BUILD_ASSERT(CONFIG_AT_CMD_REQUEST_RESPONSE_BUFFER_LENGTH >= AT_CMD_REQUEST_ERR_MAX_LEN,
	     "Not enough AT command response buffer for printing error events.");

/* Temperature alert limits. */
#define TEMP_ALERT_LIMIT ((float)CONFIG_TEMP_ALERT_LIMIT)
#define TEMP_ALERT_HYSTERESIS 1.5f
#define TEMP_ALERT_LOWER_LIMIT (TEMP_ALERT_LIMIT - TEMP_ALERT_HYSTERESIS)

/**
 * @brief Construct a device message object with automatically generated timestamp
 *
 * The resultant JSON object will be conformal to the General Message Schema described in the
 * application-protocols repo:
 *
 * https://github.com/nRFCloud/application-protocols
 *
 * @param msg - The object to contain the message
 * @param appid - The appId for the device message
 * @param msg_type - The messageType for the device message
 * @return int - 0 on success, negative error code otherwise.
 */
static int create_timestamped_device_message(struct nrf_cloud_obj *const msg,
					     const char *const appid,
					     const char *const msg_type)
{
	int err;
	int64_t timestamp;

	/* Acquire timestamp */
	err = date_time_now(&timestamp);
	if (err) {
		LOG_ERR("Failed to obtain current time, error %d", err);
		return -ETIME;
	}

	/* Create message object */
	err = nrf_cloud_obj_msg_init(msg, appid, msg_type);
	if (err) {
		LOG_ERR("Failed to initialize message with appid %s and msg type %s",
			appid, msg_type);
		return err;
	}

	/* Add timestamp to message object */
	err = nrf_cloud_obj_ts_add(msg, timestamp);
	if (err) {
		LOG_ERR("Failed to add timestamp to data message with appid %s and msg type %s",
			appid, msg_type);
		nrf_cloud_obj_free(msg);
		return err;
	}

	return 0;
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
	int ret;

	NRF_CLOUD_OBJ_JSON_DEFINE(msg_obj);

	/* Create a timestamped message container object for the sensor sample. */
	ret = create_timestamped_device_message(&msg_obj, sensor,
						NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA);
	if (ret) {
		return -EINVAL;
	}

	/* Populate the container object with the sensor value. */
	ret = nrf_cloud_obj_num_add(&msg_obj, NRF_CLOUD_JSON_DATA_KEY, value, false);
	if (ret) {
		LOG_ERR("Failed to append value to %s sample container object ",
			sensor);
		nrf_cloud_obj_free(&msg_obj);
		return -ENOMEM;
	}

	/* Send the sensor sample container object as a device message. */
	return send_device_message(&msg_obj);
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
	NRF_CLOUD_OBJ_JSON_DEFINE(msg_obj);

	/* Add the timestamp */
	(void)date_time_now(&gnss_pvt.ts_ms);

	/* Encode the location data into a device message */
	ret = nrf_cloud_obj_gnss_msg_create(&msg_obj, &gnss_pvt);

	if (ret == 0) {
		/* Send the location message */
		ret = send_device_message(&msg_obj);
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
static void handle_at_cmd_requests(const struct nrf_cloud_data *const dev_msg)
{
	char *cmd;
	struct nrf_cloud_obj msg_obj;
	int err = nrf_cloud_obj_input_decode(&msg_obj, dev_msg);

	if (err) {
		/* The message isn't JSON or otherwise couldn't be parsed. */
		LOG_DBG("A general topic device message of length %d could not be parsed.",
			dev_msg->len);
		return;
	}

	/* Confirm app ID and message type */
	err = nrf_cloud_obj_msg_check(&msg_obj, NRF_CLOUD_JSON_APPID_VAL_MODEM,
				      NRF_CLOUD_JSON_MSG_TYPE_VAL_CMD);
	if (err) {
		goto cleanup;
	}

	/* Get the command string */
	err = nrf_cloud_obj_str_get(&msg_obj, NRF_CLOUD_JSON_DATA_KEY, &cmd);
	if (err) {
		/* Missing or invalid command value will be treated as a blank command */
		cmd = "";
	}

	/* Execute the command and receive the result */
	char *response = execute_at_cmd_request(cmd);

	/* To re-use msg_obj for the response message we must first free its memory and
	 * reset its state.
	 * The cmd string will no longer be valid after msg_obj is freed.
	 */
	cmd = NULL;
	/* Free the object's allocated memory */
	err = nrf_cloud_obj_free(&msg_obj);
	if (err) {
		LOG_ERR("Failed to free AT CMD request");
		return;
	}

	/* Reset the object's state */
	err = nrf_cloud_obj_reset(&msg_obj);
	if (err) {
		LOG_ERR("Failed to reset AT CMD request message object for reuse");
		return;
	}

	err = create_timestamped_device_message(&msg_obj, NRF_CLOUD_JSON_APPID_VAL_MODEM,
						NRF_CLOUD_JSON_MSG_TYPE_VAL_DATA);
	if (err) {
		return;
	}

	/* Populate response with command result */
	err = nrf_cloud_obj_str_add(&msg_obj, NRF_CLOUD_JSON_DATA_KEY, response, false);
	if (err) {
		LOG_ERR("Failed to populate AT CMD response with modem response data");
		goto cleanup;
	}

	/* Send the response */
	err = send_device_message(&msg_obj);
	if (err) {
		LOG_ERR("Failed to send AT CMD request response, error: %d", err);
	}

	return;

cleanup:
	(void)nrf_cloud_obj_free(&msg_obj);
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
		LOG_INF("Temperature limit %f C exceeded: now %f C.",
			TEMP_ALERT_LIMIT, temp);
	} else if ((temp < TEMP_ALERT_LOWER_LIMIT) && temperature_alert_active) {
		temperature_alert_active = false;
		LOG_INF("Temperature now below limit: %f C.", temp);
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

	nrf_cloud_log_init();
	/* Send a direct log to the nRF Cloud web portal indicating the sample has started up. */
	nrf_cloud_log_send(LOG_LEVEL_INF, "nRF Cloud MQTT multi-service sample v%s",
			   CONFIG_APP_VERSION);

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
			LOG_INF("Sent test counter = %d", counter);
			(void)send_sensor_sample("COUNT", counter++);
		}

		/* Wait out any remaining time on the sample interval timer. */
		k_timer_status_sync(&sensor_sample_timer);
	}
}
