/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <kernel_structs.h>
#include <stdio.h>
#include <string.h>
#include <gps.h>
#include <sensor.h>
#include <console.h>
#include <misc/reboot.h>
#include <logging/log_ctrl.h>
#if defined(CONFIG_BSD_LIBRARY)
#include <net/bsdlib.h>
#include <lte_lc.h>
#include <modem_info.h>
#endif /* CONFIG_BSD_LIBRARY */
#include <net/cloud.h>
#include <net/socket.h>
#include <nrf_cloud.h>

#include "cloud_codec.h"
#include "env_sensors.h"
#include "orientation_detector.h"
#include "ui.h"
#include "gps_controller.h"

#define CALIBRATION_PRESS_DURATION 	K_SECONDS(5)
#define CLOUD_CONNACK_WAIT_DURATION	K_SECONDS(CONFIG_CLOUD_WAIT_DURATION)

#ifdef CONFIG_ACCEL_USE_SIM
#define FLIP_INPUT			CONFIG_FLIP_INPUT
#define CALIBRATION_INPUT		-1
#else
#define FLIP_INPUT			-1
#ifdef CONFIG_ACCEL_CALIBRATE
#define CALIBRATION_INPUT		CONFIG_CALIBRATION_INPUT
#else
#define CALIBRATION_INPUT		-1
#endif /* CONFIG_ACCEL_CALIBRATE */
#endif /* CONFIG_ACCEL_USE_SIM */

#if defined(CONFIG_BSD_LIBRARY) && \
!defined(CONFIG_LTE_LINK_CONTROL)
#error "Missing CONFIG_LTE_LINK_CONTROL"
#endif

#if defined(CONFIG_BSD_LIBRARY) && \
defined(CONFIG_LTE_AUTO_INIT_AND_CONNECT) && \
defined(CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES)
#error "PROVISION_CERTIFICATES \
	requires CONFIG_LTE_AUTO_INIT_AND_CONNECT to be disabled!"
#endif

#define CLOUD_LED_ON_STR "{\"led\":\"on\"}"
#define CLOUD_LED_OFF_STR "{\"led\":\"off\"}"
#define CLOUD_LED_MSK UI_LED_1

struct rsrp_data {
	u16_t value;
	u16_t offset;
};

#if CONFIG_MODEM_INFO
static struct rsrp_data rsrp = {
	.value = 0,
	.offset = MODEM_INFO_RSRP_OFFSET_VAL,
};
#endif /* CONFIG_MODEM_INFO */


static struct cloud_backend *cloud_backend;

 /* Variables to keep track of nRF cloud user association. */
#if defined(CONFIG_USE_UI_MODULE)
static u8_t ua_pattern[6];
#endif
static int buttons_to_capture;
static int buttons_captured;
static atomic_t pattern_recording;
static bool recently_associated;
static bool association_with_pin;

/* Sensor data */
static struct gps_data gps_data;
static struct cloud_channel_data flip_cloud_data;
static struct cloud_channel_data gps_cloud_data;
static struct cloud_channel_data button_cloud_data;

#if CONFIG_MODEM_INFO
static struct modem_param_info modem_param;
static struct cloud_channel_data signal_strength_cloud_data;
static struct cloud_channel_data device_cloud_data;
#endif /* CONFIG_MODEM_INFO */
static atomic_val_t send_data_enable;

/* Flag used for flip detection */
static bool flip_mode_enabled = true;

/* Structures for work */
static struct k_work connect_work;
static struct k_work send_gps_data_work;
static struct k_work send_button_data_work;
static struct k_work send_flip_data_work;
static struct k_delayed_work send_env_data_work;
static struct k_delayed_work long_press_button_work;
static struct k_delayed_work cloud_reboot_work;
#if CONFIG_MODEM_INFO
static struct k_work device_status_work;
static struct k_work rsrp_work;
#endif /* CONFIG_MODEM_INFO */

enum error_type {
	ERROR_CLOUD,
	ERROR_BSD_RECOVERABLE,
	ERROR_BSD_IRRECOVERABLE,
	ERROR_LTE_LC,
	ERROR_SYSTEM_FAULT
};

/* Forward declaration of functions */
static void app_connect(struct k_work *work);
static void flip_send(struct k_work *work);
static void env_data_send(void);
static void sensors_init(void);
static void work_init(void);
static void sensor_data_send(struct cloud_channel_data *data);
#if CONFIG_MODEM_INFO
static void device_status_send(struct k_work *work);
#endif

/**@brief nRF Cloud error handler. */
void error_handler(enum error_type err_type, int err_code)
{
	if (err_type == ERROR_CLOUD) {
		if (gps_control_is_enabled()) {
			printk("Reboot\n");
			sys_reboot(0);
		}
#if defined(CONFIG_LTE_LINK_CONTROL)
		/* Turn off and shutdown modem */
		printk("LTE link disconnect\n");
		int err = lte_lc_power_off();
		if (err) {
			printk("lte_lc_power_off failed: %d\n", err);
		}
#endif /* CONFIG_LTE_LINK_CONTROL */
#if defined(CONFIG_BSD_LIBRARY)
		printk("Shutdown modem\n");
		bsdlib_shutdown();
#endif
	}

#if !defined(CONFIG_DEBUG) && defined(CONFIG_REBOOT)
	LOG_PANIC();
	sys_reboot(0);
#else
	switch (err_type) {
	case ERROR_CLOUD:
		/* Blinking all LEDs ON/OFF in pairs (1 and 4, 2 and 3)
		 * if there is an application error.
		 */
		ui_led_set_pattern(UI_LED_ERROR_CLOUD);
		printk("Error of type ERROR_CLOUD: %d\n", err_code);
	break;
	case ERROR_BSD_RECOVERABLE:
		/* Blinking all LEDs ON/OFF in pairs (1 and 3, 2 and 4)
		 * if there is a recoverable error.
		 */
		ui_led_set_pattern(UI_LED_ERROR_BSD_REC);
		printk("Error of type ERROR_BSD_RECOVERABLE: %d\n", err_code);
	break;
	case ERROR_BSD_IRRECOVERABLE:
		/* Blinking all LEDs ON/OFF if there is an
		 * irrecoverable error.
		 */
		ui_led_set_pattern(UI_LED_ERROR_BSD_IRREC);
		printk("Error of type ERROR_BSD_IRRECOVERABLE: %d\n", err_code);
	break;
	default:
		/* Blinking all LEDs ON/OFF in pairs (1 and 2, 3 and 4)
		 * undefined error.
		 */
		ui_led_set_pattern(UI_LED_ERROR_UNKNOWN);
		printk("Unknown error type: %d, code: %d\n",
			err_type, err_code);
	break;
	}

	while (true) {
		k_cpu_idle();
	}
#endif /* CONFIG_DEBUG */
}

void k_sys_fatal_error_handler(unsigned int reason,
			       const z_arch_esf_t *esf)
{
	ARG_UNUSED(esf);

	LOG_PANIC();
	z_fatal_print("Running main.c error handler");
	error_handler(ERROR_SYSTEM_FAULT, reason);
	CODE_UNREACHABLE;
}

void cloud_error_handler(int err)
{
	error_handler(ERROR_CLOUD, err);
}

/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	error_handler(ERROR_BSD_RECOVERABLE, (int)err);
}

/**@brief Irrecoverable BSD library error. */
void bsd_irrecoverable_error_handler(uint32_t err)
{
	error_handler(ERROR_BSD_IRRECOVERABLE, (int)err);
}

static void send_gps_data_work_fn(struct k_work *work)
{
	sensor_data_send(&gps_cloud_data);
}

static void send_env_data_work_fn(struct k_work *work)
{
	env_data_send();
}

static void send_button_data_work_fn(struct k_work *work)
{
	sensor_data_send(&button_cloud_data);
}

static void send_flip_data_work_fn(struct k_work *work)
{
	sensor_data_send(&flip_cloud_data);
}

/**@brief Callback for GPS trigger events */
static void gps_trigger_handler(struct device *dev, struct gps_trigger *trigger)
{
	static u32_t fix_count;

	ARG_UNUSED(trigger);

	if (ui_button_is_active(UI_SWITCH_2)
	   || !atomic_get(&send_data_enable)) {
		return;
	}

	if (++fix_count < CONFIG_GPS_CONTROL_FIX_COUNT) {
		return;
	}

	fix_count = 0;

	ui_led_set_pattern(UI_LED_GPS_FIX);

	gps_sample_fetch(dev);
	gps_channel_get(dev, GPS_CHAN_NMEA, &gps_data);
	gps_cloud_data.data.buf = gps_data.nmea.buf;
	gps_cloud_data.data.len = gps_data.nmea.len;
	gps_cloud_data.tag += 1;

	if (gps_cloud_data.tag == 0) {
		gps_cloud_data.tag = 0x1;
	}

	gps_control_stop(K_NO_WAIT);
	k_work_submit(&send_gps_data_work);
	k_delayed_work_submit(&send_env_data_work, K_NO_WAIT);
}

/**@brief Callback for sensor trigger events */
static void sensor_trigger_handler(struct device *dev,
			struct sensor_trigger *trigger)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trigger);

	flip_send(NULL);
}

#if defined(CONFIG_USE_UI_MODULE)
/**@brief Send button presses to cloud */
static void button_send(bool pressed)
{
	static char data[] = "1";

	if (!atomic_get(&send_data_enable)) {
		return;
	}

	if (pressed) {
		data[0] = '1';
	} else {
		data[0] = '0';
	}

	button_cloud_data.data.buf = data;
	button_cloud_data.data.len = strlen(data);
	button_cloud_data.tag += 1;

	if (button_cloud_data.tag == 0) {
		button_cloud_data.tag = 0x1;
	}

	k_work_submit(&send_button_data_work);
}
#endif

/**@brief Poll flip orientation and send to cloud if flip mode is enabled. */
static void flip_send(struct k_work *work)
{
	static enum orientation_state last_orientation_state =
		ORIENTATION_NOT_KNOWN;
	static struct orientation_detector_sensor_data sensor_data;

	if (!flip_mode_enabled || !atomic_get(&send_data_enable)) {
		return;
	}

	if (orientation_detector_poll(&sensor_data) == 0) {
		if (sensor_data.orientation == last_orientation_state) {
			return;
		}

		switch (sensor_data.orientation) {
		case ORIENTATION_NORMAL:
			flip_cloud_data.data.buf = "NORMAL";
			flip_cloud_data.data.len = sizeof("NORMAL") - 1;
			break;
		case ORIENTATION_UPSIDE_DOWN:
			flip_cloud_data.data.buf = "UPSIDE_DOWN";
			flip_cloud_data.data.len = sizeof("UPSIDE_DOWN") - 1;
			break;
		default:
			return;
		}

		last_orientation_state = sensor_data.orientation;

		k_work_submit(&send_flip_data_work);
	}
}

static void cloud_cmd_handler(struct cloud_command *cmd)
{
	/* Command handling goes here. */
	if (cmd->recipient == CLOUD_RCPT_MODEM_INFO) {
#if CONFIG_MODEM_INFO
		if (cmd->type == CLOUD_CMD_READ) {
			device_status_send(NULL);
		}
#endif
	} else if (cmd->recipient == CLOUD_RCPT_UI) {
		if (cmd->type == CLOUD_CMD_LED_RED) {
			ui_led_set_color(127, 0, 0);
		} else if (cmd->type == CLOUD_CMD_LED_GREEN) {
			ui_led_set_color(0, 127, 0);
		} else if (cmd->type == CLOUD_CMD_LED_BLUE) {
			ui_led_set_color(0, 0, 127);
		}
	}
}

#if CONFIG_MODEM_INFO
/**@brief Callback handler for LTE RSRP data. */
static void modem_rsrp_handler(char rsrp_value)
{
	rsrp.value = rsrp_value;

	k_work_submit(&rsrp_work);
}

/**@brief Publish RSRP data to the cloud. */
static void modem_rsrp_data_send(struct k_work *work)
{
	char buf[CONFIG_MODEM_INFO_BUFFER_SIZE] = {0};
	static u32_t timestamp_prev;
	size_t len;

	if (!atomic_get(&send_data_enable)) {
		return;
	}

	if (k_uptime_get_32() - timestamp_prev <
	    K_SECONDS(CONFIG_HOLD_TIME_RSRP)) {
		return;
	}

	len = snprintf(buf, CONFIG_MODEM_INFO_BUFFER_SIZE,
			"%d", rsrp.value - rsrp.offset);

	signal_strength_cloud_data.data.buf = buf;
	signal_strength_cloud_data.data.len = len;
	signal_strength_cloud_data.tag += 1;

	if (signal_strength_cloud_data.tag == 0) {
		signal_strength_cloud_data.tag = 0x1;
	}

	sensor_data_send(&signal_strength_cloud_data);
	timestamp_prev = k_uptime_get_32();
}

/**@brief Poll device info and send data to the cloud. */
static void device_status_send(struct k_work *work)
{
	int len;
	int ret;

	cJSON *root_obj = cJSON_CreateObject();

	if (root_obj == NULL) {
		printk("Unable to allocate JSON object\n");
		return;
	}

	if (!atomic_get(&send_data_enable)) {
		return;
	}

	ret = modem_info_params_get(&modem_param);

	if (ret < 0) {
		printk("Unable to obtain modem parameters: %d\n", ret);
		return;
	}

	len = modem_info_json_object_encode(&modem_param, root_obj);

	if (len < 0) {
		return;
	}

	device_cloud_data.data.buf = (char *)root_obj;
	device_cloud_data.data.len = len;
	device_cloud_data.tag += 1;

	if (device_cloud_data.tag == 0) {
		device_cloud_data.tag = 0x1;
	}

	/* Transmits the data to the cloud. Frees the JSON object. */
	sensor_data_send(&device_cloud_data);
}
#endif /* CONFIG_MODEM_INFO */

/**@brief Get environment data from sensors and send to cloud. */
static void env_data_send(void)
{
	int err;
	env_sensor_data_t env_data;
	struct cloud_msg msg = {
		.qos = CLOUD_QOS_AT_MOST_ONCE,
		.endpoint.type = CLOUD_EP_TOPIC_MSG
	};

	if (!atomic_get(&send_data_enable)) {
		return;
	}

	if (gps_control_is_active()) {
		k_delayed_work_submit(&send_env_data_work,
			K_SECONDS(CONFIG_ENVIRONMENT_DATA_BACKOFF_TIME));
		return;
	}

	if (env_sensors_get_temperature(&env_data) == 0) {
		if (cloud_encode_env_sensors_data(&env_data, &msg) == 0) {
			err = cloud_send(cloud_backend, &msg);
			cloud_release_data(&msg);
			if (err) {
				goto error;
			}
		}
	}

	if (env_sensors_get_humidity(&env_data) == 0) {
		if (cloud_encode_env_sensors_data(&env_data, &msg) == 0) {
			err = cloud_send(cloud_backend, &msg);
			cloud_release_data(&msg);
			if (err) {
				goto error;
			}
		}
	}

	if (env_sensors_get_pressure(&env_data) == 0) {
		if (cloud_encode_env_sensors_data(&env_data, &msg) == 0) {
			err = cloud_send(cloud_backend, &msg);
			cloud_release_data(&msg);
			if (err) {
				goto error;
			}
		}
	}

	if (env_sensors_get_air_quality(&env_data) == 0) {
		if (cloud_encode_env_sensors_data(&env_data, &msg) == 0) {
			err = cloud_send(cloud_backend, &msg);
			cloud_release_data(&msg);
			if (err) {
				goto error;
			}
		}
	}

	k_delayed_work_submit(&send_env_data_work,
	K_SECONDS(CONFIG_ENVIRONMENT_DATA_SEND_INTERVAL));

	return;
error:
	printk("sensor_data_send failed: %d\n", err);
	cloud_error_handler(err);
}

/**@brief Send sensor data to nRF Cloud. **/
static void sensor_data_send(struct cloud_channel_data *data)
{
	int err = 0;
	struct cloud_msg msg = {
			.qos = CLOUD_QOS_AT_MOST_ONCE,
			.endpoint.type = CLOUD_EP_TOPIC_MSG
		};

	if (data->type == CLOUD_CHANNEL_DEVICE_INFO) {
		msg.endpoint.type = CLOUD_EP_TOPIC_STATE;
	}

	if (!atomic_get(&send_data_enable) || gps_control_is_active()) {
		return;
	}

	if (data->type != CLOUD_CHANNEL_DEVICE_INFO) {
		err = cloud_encode_data(data, &msg);
	} else {
		err = cloud_encode_digital_twin_data(data, &msg);
	}

	if (err) {
		printk("Unable to encode cloud data: %d\n", err);
	}

	err = cloud_send(cloud_backend, &msg);

	cloud_release_data(&msg);

	if (err) {
		printk("sensor_data_send failed: %d\n", err);
		cloud_error_handler(err);
	}
}

/**@brief Reboot the device if CONNACK has not arrived. */
static void cloud_reboot_handler(struct k_work *work)
{
	error_handler(ERROR_CLOUD, -ETIMEDOUT);
}

/**@brief Callback for sensor attached event from nRF Cloud. */
void sensors_start(void)
{
	atomic_set(&send_data_enable, 1);
	sensors_init();
}

/**@brief nRF Cloud specific callback for cloud association event. */
static void on_user_pairing_req(const struct cloud_event *evt)
{
	if (evt->data.pair_info.type == CLOUD_PAIR_SEQUENCE) {
		if (!atomic_get(&pattern_recording)) {
			ui_led_set_pattern(UI_CLOUD_PAIRING);
			atomic_set(&pattern_recording, 1);
			buttons_captured = 0;
			buttons_to_capture = *evt->data.pair_info.buf;

			printk("Please enter the user association pattern ");
			printk("using the buttons and switches\n");
		}
	} else if (evt->data.pair_info.type == CLOUD_PAIR_PIN) {
		association_with_pin = true;
		ui_led_set_pattern(UI_CLOUD_PAIRING);
		printk("Waiting for cloud association with PIN\n");
	}
}

#if defined(CONFIG_USE_UI_MODULE)
/**@brief Send user association information to nRF Cloud. */
static void cloud_user_associate(void)
{
	int err;
	struct cloud_msg msg = {
		.buf = ua_pattern,
		.len = buttons_to_capture,
		.endpoint = {
			.type = CLOUD_EP_TOPIC_PAIR
		}
	};

	atomic_set(&pattern_recording, 0);

	err = cloud_send(cloud_backend, &msg);
	if (err) {
		printk("Could not send association message, error: %d\n", err);
		cloud_error_handler(err);
	}
}
#endif

/** @brief Handle procedures after successful association with nRF Cloud. */
void on_pairing_done(void)
{
	if (association_with_pin || (buttons_captured > 0)) {
		recently_associated = true;

		printk("Successful user association.\n");
		printk("The device will attempt to reconnect to ");
		printk("nRF Cloud. It may reset in the process.\n");
		printk("Manual reset may be required if connection ");
		printk("to nRF Cloud is not established within ");
		printk("20 - 30 seconds.\n");
	}

	if (!association_with_pin) {
		return;
	}

	int err;

	printk("Disconnecting from nRF cloud...\n");

	err = cloud_disconnect(cloud_backend);
	if (err == 0) {
		printk("Reconnecting to cloud...\n");
		err = cloud_connect(cloud_backend);
		if (err == 0) {
			return;
		}
		printk("Could not reconnect\n");
	} else {
		printk("Disconnection failed\n");
	}

	printk("Fallback to controlled reboot\n");
	printk("Shutting down LTE link...\n");

#if defined(CONFIG_BSD_LIBRARY)
	err = lte_lc_power_off();
	if (err) {
		printk("Could not shut down link\n");
	} else {
		printk("LTE link disconnected\n");
	}
#endif

#ifdef CONFIG_REBOOT
	printk("Rebooting...\n");
	LOG_PANIC();
	sys_reboot(SYS_REBOOT_COLD);
#endif
	printk("**** Manual reboot required ***\n");
}

void cloud_event_handler(const struct cloud_backend *const backend,
			 const struct cloud_event *const evt,
			 void *user_data)
{
	ARG_UNUSED(user_data);

	switch (evt->type) {
	case CLOUD_EVT_CONNECTED:
		printk("CLOUD_EVT_CONNECTED\n");
		k_delayed_work_cancel(&cloud_reboot_work);
		ui_led_set_pattern(UI_CLOUD_CONNECTED);
		break;
	case CLOUD_EVT_READY:
		printk("CLOUD_EVT_READY\n");
		ui_led_set_pattern(UI_CLOUD_CONNECTED);
		sensors_start();
		break;
	case CLOUD_EVT_DISCONNECTED:
		printk("CLOUD_EVT_DISCONNECTED\n");
		ui_led_set_pattern(UI_LTE_DISCONNECTED);
		break;
	case CLOUD_EVT_ERROR:
		printk("CLOUD_EVT_ERROR\n");
		break;
	case CLOUD_EVT_DATA_SENT:
		printk("CLOUD_EVT_DATA_SENT\n");
		break;
	case CLOUD_EVT_DATA_RECEIVED:
		printk("CLOUD_EVT_DATA_RECEIVED\n");
		cloud_decode_command(evt->data.msg.buf);
		break;
	case CLOUD_EVT_PAIR_REQUEST:
		printk("CLOUD_EVT_PAIR_REQUEST\n");
		on_user_pairing_req(evt);
		break;
	case CLOUD_EVT_PAIR_DONE:
		printk("CLOUD_EVT_PAIR_DONE\n");
		on_pairing_done();
		break;
	default:
		printk("Unknown cloud event type: %d\n", evt->type);
		break;
	}
}

/**@brief Connect to nRF Cloud, */
static void app_connect(struct k_work *work)
{
	int err;

	ui_led_set_pattern(UI_CLOUD_CONNECTING);
	err = cloud_connect(cloud_backend);

	if (err) {
		printk("cloud_connect failed: %d\n", err);
		cloud_error_handler(err);
	}
}

#if defined(CONFIG_USE_UI_MODULE)
/**@brief Function to keep track of user association input when using
 *	  buttons and switches to register the association pattern.
 *	  nRF Cloud specific.
 */
static void pairing_button_register(struct ui_evt *evt)
{
	if (buttons_captured < buttons_to_capture) {
		if (evt->button == UI_BUTTON_1 &&
		    evt->type == UI_EVT_BUTTON_ACTIVE) {
			ua_pattern[buttons_captured++] =
				NRF_CLOUD_UA_BUTTON_INPUT_3;
			printk("Button 1\n");
		} else if (evt->button == UI_BUTTON_2 &&
		    evt->type == UI_EVT_BUTTON_ACTIVE) {
			ua_pattern[buttons_captured++] =
				NRF_CLOUD_UA_BUTTON_INPUT_4;
			printk("Button 2\n");
		} else if (evt->button == UI_SWITCH_1) {
			ua_pattern[buttons_captured++] =
				NRF_CLOUD_UA_BUTTON_INPUT_1;
			printk("Switch 1\n");
		} else if (evt->button == UI_SWITCH_2) {
			ua_pattern[buttons_captured++] =
				NRF_CLOUD_UA_BUTTON_INPUT_2;
			printk("Switch 2\n");
		}
	}

	if (buttons_captured == buttons_to_capture) {
		cloud_user_associate();
	}
}
#endif

static void long_press_handler(struct k_work *work)
{
	if (!atomic_get(&send_data_enable)) {
		printk("Link not ready, long press disregarded\n");
		return;
	}

	if (gps_control_is_enabled()) {
		printk("Stopping GPS\n");
		gps_control_disable();
	} else {
		printk("Starting GPS\n");
		gps_control_enable();
		gps_control_start(K_SECONDS(1));
	}
}

/**@brief Initializes and submits delayed work. */
static void work_init(void)
{
	k_work_init(&connect_work, app_connect);
	k_work_init(&send_gps_data_work, send_gps_data_work_fn);
	k_work_init(&send_button_data_work, send_button_data_work_fn);
	k_work_init(&send_flip_data_work, send_flip_data_work_fn);
	k_delayed_work_init(&send_env_data_work, send_env_data_work_fn);
	k_delayed_work_init(&long_press_button_work, long_press_handler);
	k_delayed_work_init(&cloud_reboot_work, cloud_reboot_handler);
#if CONFIG_MODEM_INFO
	k_work_init(&device_status_work, device_status_send);
	k_work_init(&rsrp_work, modem_rsrp_data_send);
#endif /* CONFIG_MODEM_INFO */
}

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static void modem_configure(void)
{
#if defined(CONFIG_BSD_LIBRARY)
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already turned on
		 * and connected.
		 */
	} else {
		int err;

		printk("Connecting to LTE network. ");
		printk("This may take several minutes.\n");
		ui_led_set_pattern(UI_LTE_CONNECTING);

		err = lte_lc_init_and_connect();
		if (err) {
			printk("LTE link could not be established.\n");
			error_handler(ERROR_LTE_LC, err);
		}

		printk("Connected to LTE network\n");
		ui_led_set_pattern(UI_LTE_CONNECTED);
	}
#endif
}

/**@brief Initializes the accelerometer device and
 * configures trigger if set.
 */
static void accelerometer_init(void)
{
	if (IS_ENABLED(CONFIG_ACCEL_USE_EXTERNAL)) {

		struct device *accel_dev =
		device_get_binding(CONFIG_ACCEL_DEV_NAME);

		if (accel_dev == NULL) {
			printk("Could not get %s device\n",
				CONFIG_ACCEL_DEV_NAME);
			return;
		}

		struct sensor_trigger sensor_trig = {
			.type = SENSOR_TRIG_THRESHOLD,
		};

		printk("Setting trigger\n");
		int err = 0;

		err = sensor_trigger_set(accel_dev, &sensor_trig,
				sensor_trigger_handler);

		if (err) {
			printk("Unable to set trigger\n");
		}
	}
}

/**@brief Initializes flip detection using orientation detector module
 * and configured accelerometer device.
 */
static void flip_detection_init(void)
{
	int err;
	struct device *accel_dev =
		device_get_binding(CONFIG_ACCEL_DEV_NAME);

	if (accel_dev == NULL) {
		printk("Could not get %s device\n", CONFIG_ACCEL_DEV_NAME);
		return;
	}

	orientation_detector_init(accel_dev);

	if (!IS_ENABLED(CONFIG_ACCEL_CALIBRATE)) {
		return;
	}

	err = orientation_detector_calibrate();
	if (err) {
		printk("Could not calibrate accelerometer device: %d\n", err);
	}
}



static void button_sensor_init(void)
{
	button_cloud_data.type = CLOUD_CHANNEL_BUTTON;
	button_cloud_data.tag = 0x1;
}

#if CONFIG_MODEM_INFO
/**brief Initialize LTE status containers. */
static void modem_data_init(void)
{
	int err;
	err = modem_info_init();
	if (err) {
		printk("Modem info could not be established: %d\n", err);
		return;
	}

	modem_info_params_init(&modem_param);

	signal_strength_cloud_data.type = CLOUD_CHANNEL_LTE_LINK_RSRP;
	signal_strength_cloud_data.tag = 0x1;

	device_cloud_data.type = CLOUD_CHANNEL_DEVICE_INFO;
	device_cloud_data.tag = 0x1;

	k_work_submit(&device_status_work);

	modem_info_rsrp_register(modem_rsrp_handler);
}
#endif /* CONFIG_MODEM_INFO */

/**@brief Initializes the sensors that are used by the application. */
static void sensors_init(void)
{
	accelerometer_init();
	flip_detection_init();
	env_sensors_init_and_start();

#if CONFIG_MODEM_INFO
	modem_data_init();
#endif /* CONFIG_MODEM_INFO */
	if (IS_ENABLED(CONFIG_CLOUD_BUTTON)) {
		button_sensor_init();
	}

	gps_control_init(gps_trigger_handler);

	flip_cloud_data.type = CLOUD_CHANNEL_FLIP;

	/* Send sensor data after initialization, as it may be a long time until
	 * next time if the application is in power optimized mode.
	 */
	k_delayed_work_submit(&send_env_data_work, K_SECONDS(5));
}

#if defined(CONFIG_USE_UI_MODULE)
/**@brief User interface event handler. */
static void ui_evt_handler(struct ui_evt evt)
{
	if (pattern_recording) {
		pairing_button_register(&evt);
		return;
	}

	if (IS_ENABLED(CONFIG_CLOUD_BUTTON) &&
	   (evt.button == CONFIG_CLOUD_BUTTON_INPUT)) {
		button_send(evt.type == UI_EVT_BUTTON_ACTIVE ? 1 : 0);
	}

	if (IS_ENABLED(CONFIG_ACCEL_USE_SIM) && (evt.button == FLIP_INPUT)
	   && atomic_get(&send_data_enable)) {
		flip_send(NULL);
	}

	if (IS_ENABLED(CONFIG_GPS_CONTROL_ON_LONG_PRESS) &&
	   (evt.button == UI_BUTTON_1)) {
		if (evt.type == UI_EVT_BUTTON_ACTIVE) {
			k_delayed_work_submit(&long_press_button_work,
			K_SECONDS(5));
		} else {
			k_delayed_work_cancel(&long_press_button_work);
		}
	}

#if defined(CONFIG_LTE_LINK_CONTROL)
	if ((evt.button == UI_SWITCH_2) &&
	    IS_ENABLED(CONFIG_POWER_OPTIMIZATION_ENABLE)) {
		int err;
		if (evt.type == UI_EVT_BUTTON_ACTIVE) {
			err = lte_lc_edrx_req(false);
			if (err) {
				error_handler(ERROR_LTE_LC, err);
			}
			err = lte_lc_psm_req(true);
			if (err) {
				error_handler(ERROR_LTE_LC, err);
			}
		} else {
			err = lte_lc_psm_req(false);
			if (err) {
				error_handler(ERROR_LTE_LC, err);
			}
			err = lte_lc_edrx_req(true);
			if (err) {
				error_handler(ERROR_LTE_LC, err);
			}
		}
	}
#endif /* defined(CONFIG_LTE_LINK_CONTROL) */
}
#endif /* defined(CONFIG_USE_UI_MODULE) */

void main(void)
{
	int ret;

	printk("Asset tracker started\n");

	cloud_backend = cloud_get_binding("NRF_CLOUD");
	__ASSERT(cloud_backend != NULL, "nRF Cloud backend not found");

	ret = cloud_init(cloud_backend, cloud_event_handler);
	if (ret) {
		printk("Cloud backend could not be initialized, error: %d\n",
			ret);
		cloud_error_handler(ret);
	}

#if defined(CONFIG_USE_UI_MODULE)
	ui_init(ui_evt_handler);
#endif

	ret = cloud_decode_init(cloud_cmd_handler);
	if (ret) {
		printk("Cloud command decoder could not be initialized, error: %d\n", ret);
		cloud_error_handler(ret);
	}

	work_init();
	modem_configure();
connect:
	ret = cloud_connect(cloud_backend);
	if (ret) {
		printk("cloud_connect failed: %d\n", ret);
		cloud_error_handler(ret);
	} else {
		k_delayed_work_submit(&cloud_reboot_work,
				      CLOUD_CONNACK_WAIT_DURATION);
	}

	struct pollfd fds[] = {
		{
			.fd = cloud_backend->config->socket,
			.events = POLLIN
		}
	};

	while (true) {
		ret = poll(fds, ARRAY_SIZE(fds),
			K_SECONDS(CONFIG_MQTT_KEEPALIVE));

		if (ret < 0) {
			printk("poll() returned an error: %d\n", ret);
			error_handler(ERROR_CLOUD, ret);
			continue;
		}

		if (ret == 0) {
			cloud_ping(cloud_backend);
			continue;
		}

		if ((fds[0].revents & POLLIN) == POLLIN) {
			cloud_input(cloud_backend);
		}

		if ((fds[0].revents & POLLNVAL) == POLLNVAL) {
			printk("Socket error: POLLNVAL\n");
			error_handler(ERROR_CLOUD, -EIO);
			return;
		}

		if ((fds[0].revents & POLLHUP) == POLLHUP) {
			printk("Socket error: POLLHUP\n");
			error_handler(ERROR_CLOUD, -EIO);
			return;
		}

		if ((fds[0].revents & POLLERR) == POLLERR) {
			printk("Socket error: POLLERR\n");
			error_handler(ERROR_CLOUD, -EIO);
			return;
		}
	}

	cloud_disconnect(cloud_backend);
	goto connect;
}
