/* Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <helpers/nrfx_reset_reason.h>
#include <date_time.h>
#include <stdio.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_codec.h>
#if defined(CONFIG_NRF_CLOUD_COAP)
#include <net/nrf_cloud_coap.h>
#endif
#if defined(CONFIG_LOCATION_TRACKING)
#include <modem/location.h>
#include "location_tracking.h"
#endif
#include <dk_buttons_and_leds.h>
#include "application.h"
#include "temperature.h"
#include "cloud_connection.h"
#include "message_queue.h"
#include "led_control.h"
#include "shadow_config.h"

LOG_MODULE_REGISTER(application, CONFIG_WIFI_NRF_CLOUD_LOG_LEVEL);

/* Timer used to time the sensor sampling rate. */
static K_TIMER_DEFINE(sensor_sample_timer, NULL, NULL);

/* Temperature alert limits. */
#define TEMP_ALERT_LIMIT ((double)CONFIG_TEMP_ALERT_LIMIT)
#define TEMP_ALERT_HYSTERESIS 1.5
#define TEMP_ALERT_LOWER_LIMIT (TEMP_ALERT_LIMIT - TEMP_ALERT_HYSTERESIS)

/* State of the test counter. This can be changed using the configuration in the shadow */
static bool test_counter_enabled;

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

	MSG_OBJ_DEFINE(msg_obj);

	/* Create a timestamped message container object for the sensor sample. */
	ret = nrf_cloud_obj_msg_ts_init(&msg_obj, sensor,
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

#if defined(CONFIG_LOCATION_TRACKING)
/**
 * @brief Callback to receive periodic location updates from location_tracking.c and forward them
 * to nRF Cloud.
 *
 * @param location_data - The received location update.
 *
 */
static void on_location_update(const struct location_event_data * const location_data)
{
	LOG_INF("Location Updated: lat: %.06f, lon: %.06f, accuracy: %.01f m, Method: %s",
		location_data->location.latitude,
		location_data->location.longitude,
		(double)location_data->location.accuracy,
		location_method_str(location_data->method));
}
#endif /* CONFIG_LOCATION_TRACKING */

/** @brief Check whether temperature is acceptable.
 * If the device exceeds a temperature limit, log a warning.
 * Once the temperature falls below a lower limit, re-enable the temperature alert
 * so it will be logged if the limit is exceeded again.
 *
 * The difference between the two limits should be sufficient to prevent logging
 * a new alert if the temperature value oscillates between two nearby values.
 *
 * @param temp - The current device temperature.
 */
static void monitor_temperature(double temp)
{
	static bool temperature_alert_active;

	if ((temp > TEMP_ALERT_LIMIT) && !temperature_alert_active) {
		temperature_alert_active = true;
		LOG_INF("Temperature limit %f C exceeded: now %f C.",
			TEMP_ALERT_LIMIT, temp);
	} else if ((temp < TEMP_ALERT_LOWER_LIMIT) && temperature_alert_active) {
		temperature_alert_active = false;
		LOG_INF("Temperature now below limit: %f C.", temp);
	}
}

/** @brief Send an incrementing test counter message to nRF Cloud.
 * If CONFIG_TEST_COUNTER_MULTIPLIER is greater than 1, the counter will be incremented
 * that many times in a row, and a separate test counter message will be sent for each increment.
 */
static void test_counter_send(void)
{
	static int counter;

	for (int i = 0; i < CONFIG_TEST_COUNTER_MULTIPLIER; i++) {
		LOG_INF("Sent test counter = %d", counter);
		(void)send_sensor_sample("COUNT", counter++);
	}
}

#if defined(CONFIG_DK_LIBRARY)
static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & DK_BTN1_MSK) {
		if ((button_state & DK_BTN1_MSK) == DK_BTN1_MSK) {
			LOG_INF("Button pressed");
		}
	}
}
#endif

static void print_reset_reason(void)
{
	uint32_t reset_reason;

	reset_reason = nrfx_reset_reason_get();
	nrfx_reset_reason_clear(reset_reason);
	LOG_INF("Reset reason: 0x%x", reset_reason);
}

void main_application_thread_fn(void)
{
	print_reset_reason();

#if defined(CONFIG_DK_LIBRARY)
	dk_buttons_init(button_handler);
#endif

	/* Wait for first connection before starting the application. */
	(void)await_cloud_ready(K_FOREVER);

	/* Wait for the date and time to become known.
	 * This is needed both for location services and for sensor sample timestamping.
	 */
	LOG_INF("Waiting for device to determine current date and time");
	if (!await_date_time_known(K_SECONDS(CONFIG_DATE_TIME_ESTABLISHMENT_TIMEOUT_SECONDS))) {
		LOG_WRN("Failed to determine valid date time. Proceeding anyways");
	} else {
		LOG_INF("Current date and time determined");
	}

#if defined(CONFIG_LOCATION_TRACKING)
	/* Begin tracking location at the configured interval. */
	(void)start_location_tracking(on_location_update,
					CONFIG_LOCATION_TRACKING_SAMPLE_INTERVAL_SECONDS);
#endif

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

		if (test_counter_enable_get()) {
			test_counter_send();
		}

		/* Wait out any remaining time on the sample interval timer. */
		k_timer_status_sync(&sensor_sample_timer);

		/* If cloud stops being ready due to network trouble or device being deleted
		 * from the cloud, turn off sensors and wait, then restart sensors.
		 */
		if (!await_cloud_ready(K_NO_WAIT) && is_device_deleted()) {
#if defined(CONFIG_LOCATION_TRACKING)
			(void)location_request_cancel();
#endif
			LOG_INF("Cloud not ready. Pausing sensors.");
			(void)await_cloud_ready(K_FOREVER);
			LOG_INF("Cloud is ready. Enabling sensors.");
#if defined(CONFIG_LOCATION_TRACKING)
			/* Begin tracking location at the configured interval. */
			(void)start_location_tracking(on_location_update,
						  CONFIG_LOCATION_TRACKING_SAMPLE_INTERVAL_SECONDS);
#endif
		}
	}
}

void test_counter_enable_set(const bool enable)
{
	if (IS_ENABLED(CONFIG_TEST_COUNTER)) {
		LOG_DBG("CONFIG_TEST_COUNTER is enabled, ignoring state change request");
	} else {
		LOG_DBG("Test counter %s", enable ? "enabled" : "disabled");
		test_counter_enabled = enable;
	}
}

bool test_counter_enable_get(void)
{
	/* When CONFIG_TEST_COUNTER is enabled the test counter is always enabled */
	return (test_counter_enabled || IS_ENABLED(CONFIG_TEST_COUNTER));
}
