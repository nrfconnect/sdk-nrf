/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <nrf_modem_at.h>
#include <nrf_modem_gnss.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/console/console.h>
#include <net/nrf_cloud.h>
#include <net/nrf_cloud_agnss.h>
#include <dk_buttons_and_leds.h>
#include <modem/lte_lc.h>
#include <zephyr/sys/reboot.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/logging/log.h>

#include "aggregator.h"
#include "ble.h"
#include "alarm.h"

LOG_MODULE_REGISTER(lte_ble_gw, CONFIG_LTE_BLE_GW_LOG_LEVEL);

/* Interval in milliseconds between each time status LEDs are updated. */
#define LEDS_UPDATE_INTERVAL K_MSEC(500)

/* Interval in microseconds between each time LEDs are updated when indicating
 * that an error has occurred.
 */
#define LEDS_ERROR_UPDATE_INTERVAL 250000

#define BUTTON_1 BIT(0)
#define BUTTON_2 BIT(1)
#define SWITCH_1 BIT(2)
#define SWITCH_2 BIT(3)

#define LED_ON(x)			(x)
#define LED_BLINK(x)		((x) << 8)
#define LED_GET_ON(x)		((x) & 0xFF)
#define LED_GET_BLINK(x)	(((x) >> 8) & 0xFF)

/* Interval in milliseconds after the device will retry cloud connection
 * if the event NRF_CLOUD_EVT_TRANSPORT_CONNECTED is not received.
 */
#define RETRY_CONNECT_WAIT K_MSEC(90000)

enum {
	LEDS_INITIALIZING       = LED_ON(0),
	LEDS_LTE_CONNECTING     = LED_BLINK(DK_LED3_MSK),
	LEDS_LTE_CONNECTED      = LED_ON(DK_LED3_MSK),
	LEDS_CLOUD_CONNECTING   = LED_BLINK(DK_LED4_MSK),
	LEDS_CLOUD_PAIRING_WAIT = LED_BLINK(DK_LED3_MSK | DK_LED4_MSK),
	LEDS_CLOUD_CONNECTED    = LED_ON(DK_LED4_MSK),
	LEDS_ERROR              = LED_ON(DK_ALL_LEDS_MSK)
} display_state;

/* Variable to keep track of nRF cloud user association request. */
static atomic_val_t association_requested;

/* Sensor data */
static struct nrf_modem_gnss_nmea_data_frame gnss_nmea_data;
static uint32_t gnss_cloud_data_tag = 0x1;
static atomic_val_t send_data_enable;

/* Structures for work */
static struct k_work_delayable leds_update_work;
static struct k_work_delayable connect_work;
struct k_work_delayable aggregated_work;
static struct k_work agnss_request_work;

static struct nrf_modem_gnss_pvt_data_frame last_pvt;
static bool nrf_modem_gnss_fix;

static bool cloud_connected;

enum error_type {
	ERROR_NRF_CLOUD,
	ERROR_MODEM_RECOVERABLE,
};

/* Forward declaration of functions. */
static void cloud_connect(struct k_work *work);
static void sensors_init(void);
static void work_init(void);

void sensor_data_send(struct nrf_cloud_sensor_data *data);

/**@brief nRF Cloud error handler. */
void error_handler(enum error_type err_type, int err)
{
	if (err_type == ERROR_NRF_CLOUD) {
		int err;

		/* Turn off and shutdown modem */
		k_sched_lock();
		err = lte_lc_power_off();
		__ASSERT(err == 0, "lte_lc_power_off failed: %d", err);
		nrf_modem_lib_shutdown();
	}

#if !defined(CONFIG_DEBUG)
	sys_reboot(SYS_REBOOT_COLD);
#else
	uint8_t led_pattern;

	switch (err_type) {
	case ERROR_NRF_CLOUD:
		/* Blinking all LEDs ON/OFF in pairs (1 and 4, 2 and 3)
		 * if there is an application error.
		 */
		led_pattern = DK_LED1_MSK | DK_LED4_MSK;
		LOG_ERR("Error of type ERROR_NRF_CLOUD: %d", err);
		break;
	case ERROR_MODEM_RECOVERABLE:
		/* Blinking all LEDs ON/OFF in pairs (1 and 3, 2 and 4)
		 * if there is a recoverable error.
		 */
		led_pattern = DK_LED1_MSK | DK_LED3_MSK;
		LOG_ERR("Error of type ERROR_MODEM_RECOVERABLE: %d", err);
		break;
	default:
		/* Blinking all LEDs ON/OFF in pairs (1 and 2, 3 and 4)
		 * undefined error.
		 */
		led_pattern = DK_LED1_MSK | DK_LED2_MSK;
		break;
	}

	while (true) {
		dk_set_leds(led_pattern & 0x0f);
		k_busy_wait(LEDS_ERROR_UPDATE_INTERVAL);
		dk_set_leds(~led_pattern & 0x0f);
		k_busy_wait(LEDS_ERROR_UPDATE_INTERVAL);
	}
#endif /* CONFIG_DEBUG */
}

void nrf_cloud_error_handler(int err)
{
	switch (err) {
	case NRF_CLOUD_ERR_STATUS_MQTT_CONN_FAIL:
	case NRF_CLOUD_ERR_STATUS_MQTT_CONN_BAD_PROT_VER:
	case NRF_CLOUD_ERR_STATUS_MQTT_CONN_ID_REJECTED:
	case NRF_CLOUD_ERR_STATUS_MQTT_CONN_SERVER_UNAVAIL:
	case NRF_CLOUD_ERR_STATUS_MQTT_CONN_BAD_USR_PWD:
	case NRF_CLOUD_ERR_STATUS_MQTT_CONN_NOT_AUTH:
	case NRF_CLOUD_ERR_STATUS_MQTT_SUB_FAIL:
		error_handler(ERROR_NRF_CLOUD, err);
	default:
		return;
	}
}

/**@brief Modem fault handler. */
void nrf_modem_fault_handler(struct nrf_modem_fault_info *fault_info)
{
	error_handler(ERROR_MODEM_RECOVERABLE, (int)fault_info->reason);
}

/**@brief Request assisted GNSS data from nRF Cloud. */
static void agnss_request(struct k_work *work)
{
	int retval;
	struct nrf_modem_gnss_agnss_data_frame agnss_data;

	if (!nrf_cloud_agnss_request_in_progress()) {
		retval = nrf_modem_gnss_read(&agnss_data, sizeof(agnss_data),
					     NRF_MODEM_GNSS_DATA_AGNSS_REQ);
		if (retval) {
			LOG_ERR("Failed to read A-GNSS data request, err %d", retval);
			return;
		}

		retval = nrf_cloud_agnss_request(&agnss_data);
		if (retval) {
			LOG_ERR("Failed to request A-GNSS data, err %d", retval);
			return;
		}
	}
}

/**@brief Callback for GNSS events */
static void gnss_event_handler(int event)
{
	int retval;

	uint32_t button_state, has_changed;
	struct sensor_data in_data = {
		.type = GNSS_POSITION,
	};

	switch (event) {
	case NRF_MODEM_GNSS_EVT_PVT:
		retval = nrf_modem_gnss_read(&last_pvt, sizeof(last_pvt), NRF_MODEM_GNSS_DATA_PVT);
		if (retval) {
			LOG_ERR("nrf_modem_gnss_read failed, err %d", retval);
			return;
		}
		if (last_pvt.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
			nrf_modem_gnss_fix = true;
		} else {
			nrf_modem_gnss_fix = false;
		}
		return;
	case NRF_MODEM_GNSS_EVT_FIX:
		LOG_DBG("NRF_MODEM_GNSS_EVT_FIX");
		return;
	case NRF_MODEM_GNSS_EVT_NMEA:
		if (nrf_modem_gnss_fix == true) {
			retval = nrf_modem_gnss_read(&gnss_nmea_data,
						     sizeof(struct nrf_modem_gnss_nmea_data_frame),
						     NRF_MODEM_GNSS_DATA_NMEA);
			if (retval) {
				LOG_ERR("nrf_modem_gnss_read failed, err: %d", retval);
				return;
			}
			LOG_INF("GNSS NMEA data ready");
			break;
		}
		return;

	case NRF_MODEM_GNSS_EVT_AGNSS_REQ:
		LOG_INF("Requesting A-GNSS Data");
		k_work_submit(&agnss_request_work);
		return;
	case NRF_MODEM_GNSS_EVT_BLOCKED:
		LOG_DBG("NRF_MODEM_GNSS_EVT_BLOCKED");
		return;
	case NRF_MODEM_GNSS_EVT_UNBLOCKED:
		LOG_DBG("NRF_MODEM_GNSS_EVT_UNBLOCKED");
		return;
	case NRF_MODEM_GNSS_EVT_PERIODIC_WAKEUP:
		LOG_DBG("NRF_MODEM_GNSS_EVT_PERIODIC_WAKEUP");
		return;
	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT:
		LOG_DBG("NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT");
		return;
	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_FIX:
		LOG_DBG("NRF_MODEM_GNSS_EVT_SLEEP_AFTER_FIX");
		return;
	default:
		LOG_WRN("Unknown GNSS event %d", event);
		return;
	}

	dk_read_buttons(&button_state, &has_changed);

	if ((button_state & SWITCH_2) || !atomic_get(&send_data_enable)) {
		return;
	}

	gnss_cloud_data_tag++;

	memcpy(in_data.data, &gnss_cloud_data_tag, sizeof(gnss_cloud_data_tag));

	in_data.length = sizeof(gnss_cloud_data_tag) + sizeof(gnss_nmea_data.nmea_str);

	memcpy(&in_data.data[sizeof(gnss_cloud_data_tag)], gnss_nmea_data.nmea_str,
	       sizeof(gnss_nmea_data.nmea_str));

	retval = aggregator_put(in_data);
	if (retval) {
		LOG_ERR("Failed to store GNSS data.");
	}
}

/**@brief Update LEDs state. */
static void leds_update(struct k_work *work)
{
	static bool led_on;
	static uint8_t current_led_on_mask;
	uint8_t led_on_mask = current_led_on_mask;

	ARG_UNUSED(work);

	/* Reset LED3 and LED4. */
	led_on_mask &= ~(DK_LED3_MSK | DK_LED4_MSK);

	/* Set LED3 and LED4 to match current state. */
	led_on_mask |= LED_GET_ON(display_state);

	led_on = !led_on;
	if (led_on) {
		led_on_mask |= LED_GET_BLINK(display_state);
	} else {
		led_on_mask &= ~LED_GET_BLINK(display_state);
	}

	if (led_on_mask != current_led_on_mask) {
		dk_set_leds(led_on_mask);
		current_led_on_mask = led_on_mask;
	}

	k_work_schedule(&leds_update_work, LEDS_UPDATE_INTERVAL);
}

/**@brief Send sensor data to nRF Cloud. **/
void sensor_data_send(struct nrf_cloud_sensor_data *data)
{
	int err;

	if (!atomic_get(&send_data_enable)) {
		return;
	}

	if (data->type == NRF_CLOUD_SENSOR_FLIP) {
		err = nrf_cloud_sensor_data_stream(data);
	} else {
		err = nrf_cloud_sensor_data_send(data);
	}

	if (err) {
		LOG_ERR("Failed to send data to cloud: %d", err);
		nrf_cloud_error_handler(err);
	}
}

static void on_cloud_evt_user_association_request(void)
{
	if (!atomic_get(&association_requested)) {
		atomic_set(&association_requested, 1);
		LOG_INF("Add device to cloud account");
		LOG_INF("Waiting for cloud association...");
	}
}

static void on_cloud_evt_user_associated(void)
{
	int err;

	if (atomic_get(&association_requested)) {
		atomic_set(&association_requested, 0);

		/* after successful association, the device must
		 * reconnect to aws.
		 */
		LOG_INF("Device associated with cloud");
		LOG_INF("Reconnecting for settings to take effect");
		LOG_INF("Disconnecting from cloud...");

		err = nrf_cloud_disconnect();

		if (err == 0) {
			return;
		} else {
			LOG_ERR("Disconnect failed, rebooting...");
			nrf_cloud_error_handler(err);
		}
	}
}

/**@brief Callback for nRF Cloud events. */
static void cloud_event_handler(const struct nrf_cloud_evt *evt)
{
	switch (evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTED");
		cloud_connected = true;
		/* This may fail if the work item is already being processed,
		 * but in such case, the next time the work handler is executed,
		 * it will exit after checking the above flag and the work will
		 * not be scheduled again.
		 */
		(void)k_work_cancel_delayable(&connect_work);
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTING:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECTING");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_CONNECT_ERROR, status: %d", evt->status);
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST:
		LOG_DBG("NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST");
		display_state = LEDS_CLOUD_PAIRING_WAIT;
		on_cloud_evt_user_association_request();
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATED:
		LOG_DBG("NRF_CLOUD_EVT_USER_ASSOCIATED");
		on_cloud_evt_user_associated();
		break;
	case NRF_CLOUD_EVT_READY:
		LOG_DBG("NRF_CLOUD_EVT_READY");
		display_state = LEDS_CLOUD_CONNECTED;
		sensors_init();
		atomic_set(&send_data_enable, 1);
		k_work_schedule(&aggregated_work, K_MSEC(100));
		break;
	case NRF_CLOUD_EVT_RX_DATA_GENERAL:
		LOG_DBG("NRF_CLOUD_EVT_RX_DATA_GENERAL");
		break;
	case NRF_CLOUD_EVT_RX_DATA_SHADOW:
		LOG_DBG("NRF_CLOUD_EVT_RX_DATA_SHADOW");
		break;
	case NRF_CLOUD_EVT_RX_DATA_LOCATION:
		LOG_DBG("NRF_CLOUD_EVT_RX_DATA_LOCATION");
		break;
	case NRF_CLOUD_EVT_SENSOR_DATA_ACK:
		LOG_DBG("NRF_CLOUD_EVT_SENSOR_DATA_ACK");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		LOG_DBG("NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED");
		atomic_set(&send_data_enable, 0);
		k_work_cancel_delayable(&aggregated_work);
		display_state = LEDS_INITIALIZING;

		cloud_connected = false;
		/* Reconnect to nRF Cloud. */
		k_work_schedule(&connect_work, K_NO_WAIT);
		break;
	case NRF_CLOUD_EVT_ERROR:
		LOG_DBG("NRF_CLOUD_EVT_ERROR, status: %d", evt->status);
		atomic_set(&send_data_enable, 0);
		display_state = LEDS_ERROR;
		nrf_cloud_error_handler(evt->status);
		break;
	default:
		LOG_WRN("Received unknown event %d", evt->type);
		break;
	}
}

/**@brief Initialize nRF CLoud library. */
static void cloud_init(void)
{
	const struct nrf_cloud_init_param param = {
		.event_handler = cloud_event_handler
	};

	int err = nrf_cloud_init(&param);

	__ASSERT(err == 0, "nRF Cloud library could not be initialized.");
}

/**@brief Connect to nRF Cloud, */
static void cloud_connect(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	if (cloud_connected) {
		return;
	}

	err = nrf_cloud_connect();
	if (err) {
		LOG_ERR("nrf_cloud_connect failed: %d", err);
		nrf_cloud_error_handler(err);
	}

	display_state = LEDS_CLOUD_CONNECTING;
	k_work_schedule(&connect_work, RETRY_CONNECT_WAIT);
}

/**@brief Callback for button events from the DK buttons and LEDs library. */
static void button_handler(uint32_t buttons, uint32_t has_changed)
{
	LOG_INF("button_handler: button 1: %u, button 2: %u "
		"switch 1: %u, switch 2: %u",
		(bool)(buttons & BUTTON_1), (bool)(buttons & BUTTON_2), (bool)(buttons & SWITCH_1),
		(bool)(buttons & SWITCH_2));
}

/**@brief Initializes and submits delayed work. */
static void work_init(void)
{
	k_work_init(&agnss_request_work, agnss_request);
	k_work_init_delayable(&leds_update_work, leds_update);
	k_work_init_delayable(&connect_work, cloud_connect);
	k_work_init_delayable(&aggregated_work, send_aggregated_data);
	k_work_schedule(&leds_update_work, LEDS_UPDATE_INTERVAL);
}

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static void modem_configure(void)
{
	int err;

	err = nrf_modem_lib_init();
	__ASSERT(err == 0, "Modem library could not be initialized, err %d.", err);

	display_state = LEDS_LTE_CONNECTING;

	LOG_INF("Establishing LTE link (this may take some time) ...");
	err = lte_lc_connect();
	__ASSERT(err == 0, "LTE link could not be established.");
	display_state = LEDS_LTE_CONNECTED;
}

/**@brief Initializes the sensors that are used by the application. */
static void sensors_init(void)
{
	int err;

	LOG_INF("Initializing GNSS");

	nrf_modem_gnss_fix = false;

	/* Configure GNSS. */
	err = nrf_modem_gnss_event_handler_set(gnss_event_handler);
	if (err) {
		LOG_ERR("Failed to set GNSS event handler");
		return;
	}

	/* Enable position NMEA GGA messages only. */
	uint16_t nmea_mask = NRF_MODEM_GNSS_NMEA_GGA_MASK;

	err = nrf_modem_gnss_nmea_mask_set(nmea_mask);
	if (err) {
		LOG_ERR("Failed to set GNSS NMEA mask");
		return;
	}

	/* This use case flag should always be set. */
	uint8_t use_case = NRF_MODEM_GNSS_USE_CASE_MULTIPLE_HOT_START;

	err = nrf_modem_gnss_use_case_set(use_case);
	if (err) {
		LOG_WRN("Failed to set GNSS use case");
	}

	err = nrf_modem_gnss_fix_retry_set(CONFIG_GNSS_SEARCH_TIMEOUT);
	if (err) {
		LOG_ERR("Failed to set GNSS fix retry");
		return;
	}

	err = nrf_modem_gnss_fix_interval_set(CONFIG_GNSS_SEARCH_INTERVAL);
	if (err) {
		LOG_ERR("Failed to set GNSS fix interval");
		return;
	}

	LOG_INF("GNSS initialized");

	err = nrf_modem_gnss_start();
	if (err) {
		LOG_ERR("Failed to start GNSS, err: %d", err);
		return;
	}

	LOG_INF("GNSS started with interval %d seconds, and timeout %d seconds",
		CONFIG_GNSS_SEARCH_INTERVAL, CONFIG_GNSS_SEARCH_TIMEOUT);
}

/**@brief Initializes buttons and LEDs, using the DK buttons and LEDs
 * library.
 */
static void buttons_leds_init(void)
{
	int err;

	err = dk_buttons_init(button_handler);
	if (err) {
		LOG_ERR("Could not initialize buttons, err code: %d", err);
	}

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Could not initialize leds, err code: %d", err);
	}

	err = dk_set_leds_state(0x00, DK_ALL_LEDS_MSK);
	if (err) {
		LOG_ERR("Could not set leds state, err code: %d", err);
	}
}

int main(void)
{
	LOG_INF("LTE Sensor Gateway sample started");

	buttons_leds_init();
	ble_init();

	work_init();
	modem_configure();
	cloud_init();
	cloud_connect(NULL);

	return 0;
}
