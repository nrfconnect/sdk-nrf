/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <kernel_structs.h>
#include <stdio.h>
#include <string.h>
#include <drivers/gps.h>
#include <sensor.h>
#include <console.h>
#include <power/reboot.h>
#include <logging/log_ctrl.h>
#if defined(CONFIG_BSD_LIBRARY)
#include <net/bsdlib.h>
#include <bsd.h>
#include <lte_lc.h>
#include <modem_info.h>
#endif /* CONFIG_BSD_LIBRARY */
#include <net/cloud.h>
#include <net/socket.h>
#include <nrf_cloud.h>

#if defined(CONFIG_BOOTLOADER_MCUBOOT)
#include <dfu/mcuboot.h>
#endif

#include "cloud_codec.h"
#include "env_sensors.h"
#include "motion.h"
#include "ui.h"
#include "gps_controller.h"
#include "service_info.h"
#include "at_cmd.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(asset_tracker, CONFIG_ASSET_TRACKER_LOG_LEVEL);

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

/* Interval in milliseconds after which the device will reboot
 * if the disconnect event has not been handled.
 */
#define REBOOT_AFTER_DISCONNECT_WAIT_MS	K_SECONDS(15)

/* Interval in milliseconds after which the device will
 * disconnect and reconnect if association was not completed.
 */
#define CONN_CYCLE_AFTER_ASSOCIATION_REQ_MS	K_MINUTES(5)

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

/* Stack definition for application workqueue */
K_THREAD_STACK_DEFINE(application_stack_area,
		      CONFIG_APPLICATION_WORKQUEUE_STACK_SIZE);
static struct k_work_q application_work_q;
static struct cloud_backend *cloud_backend;

/* Sensor data */
static struct gps_data gps_data;
static struct cloud_channel_data gps_cloud_data;
static struct cloud_channel_data button_cloud_data;
static struct cloud_channel_data device_cloud_data = {
	.type = CLOUD_CHANNEL_DEVICE_INFO,
	.tag = 0x1
};

#if CONFIG_MODEM_INFO
static struct modem_param_info modem_param;
static struct cloud_channel_data signal_strength_cloud_data;
#endif /* CONFIG_MODEM_INFO */
static atomic_val_t send_data_enable;

/* Flag used for flip detection */
static bool flip_mode_enabled = true;

/* Variable to keep track of nRF cloud user association request. */
static atomic_val_t association_requested;
static atomic_val_t reconnect_to_cloud;

/* Structures for work */
static struct k_work connect_work;
static struct k_work send_gps_data_work;
static struct k_work send_button_data_work;
static struct k_work send_modem_at_cmd_work;
static struct k_delayed_work long_press_button_work;
static struct k_delayed_work cloud_reboot_work;
static struct k_delayed_work cycle_cloud_connection_work;
static struct k_work device_status_work;

#if defined(CONFIG_AT_CMD)
#define MODEM_AT_CMD_BUFFER_LEN (CONFIG_AT_CMD_RESPONSE_MAX_LEN + 1)
#else
#define MODEM_AT_CMD_NOT_ENABLED_STR "Error: AT Command driver is not enabled"
#define MODEM_AT_CMD_BUFFER_LEN (sizeof(MODEM_AT_CMD_NOT_ENABLED_STR))
#endif
#define MODEM_AT_CMD_RESP_TOO_BIG_STR "Error: AT Command response is too large to be sent"
#define MODEM_AT_CMD_MAX_RESPONSE_LEN (2000)
static char modem_at_cmd_buff[MODEM_AT_CMD_BUFFER_LEN];
static K_SEM_DEFINE(modem_at_cmd_sem, 1, 1);

#if CONFIG_MODEM_INFO
static struct k_work rsrp_work;
#endif /* CONFIG_MODEM_INFO */

enum error_type {
	ERROR_CLOUD,
	ERROR_BSD_RECOVERABLE,
	ERROR_LTE_LC,
	ERROR_SYSTEM_FAULT
};

/* Forward declaration of functions */
static void app_connect(struct k_work *work);
static void motion_handler(motion_data_t  motion_data);
static void env_data_send(void);
#if CONFIG_LIGHT_SENSOR
static void light_sensor_data_send(void);
#endif /* CONFIG_LIGHT_SENSOR */
static void sensors_init(void);
static void work_init(void);
static void sensor_data_send(struct cloud_channel_data *data);
static void device_status_send(struct k_work *work);
static void cycle_cloud_connection(struct k_work *work);
static void set_gps_enable(const bool enable);

/**@brief nRF Cloud error handler. */
void error_handler(enum error_type err_type, int err_code)
{
	if (err_type == ERROR_CLOUD) {
		if (gps_control_is_enabled()) {
			LOG_ERR("Reboot");
			sys_reboot(0);
		}
#if defined(CONFIG_LTE_LINK_CONTROL)
		/* Turn off and shutdown modem */
		LOG_ERR("LTE link disconnect");
		int err = lte_lc_power_off();
		if (err) {
			LOG_ERR("lte_lc_power_off failed: %d", err);
		}
#endif /* CONFIG_LTE_LINK_CONTROL */
#if defined(CONFIG_BSD_LIBRARY)
		LOG_ERR("Shutdown modem");
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
		LOG_ERR("Error of type ERROR_CLOUD: %d", err_code);
	break;
	case ERROR_BSD_RECOVERABLE:
		/* Blinking all LEDs ON/OFF in pairs (1 and 3, 2 and 4)
		 * if there is a recoverable error.
		 */
		ui_led_set_pattern(UI_LED_ERROR_BSD_REC);
		LOG_ERR("Error of type ERROR_BSD_RECOVERABLE: %d", err_code);
	break;
	default:
		/* Blinking all LEDs ON/OFF in pairs (1 and 2, 3 and 4)
		 * undefined error.
		 */
		ui_led_set_pattern(UI_LED_ERROR_UNKNOWN);
		LOG_ERR("Unknown error type: %d, code: %d",
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
	LOG_ERR("Running main.c error handler");
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

static void send_gps_data_work_fn(struct k_work *work)
{
	sensor_data_send(&gps_cloud_data);
}

static void send_button_data_work_fn(struct k_work *work)
{
	sensor_data_send(&button_cloud_data);
}

static void send_modem_at_cmd_work_fn(struct k_work *work)
{
	enum at_cmd_state state;
	int err;
	struct cloud_channel_data modem_data = {
		.type = CLOUD_CHANNEL_MODEM
	};
	struct cloud_msg msg = {
		.qos = CLOUD_QOS_AT_MOST_ONCE,
		.endpoint.type = CLOUD_EP_TOPIC_MSG
	};
	size_t len = strlen(modem_at_cmd_buff);

	if (len == 0) {
		err = -ENOBUFS;
		state = AT_CMD_ERROR;
	} else {
#if defined(CONFIG_AT_CMD)
		err = at_cmd_write(modem_at_cmd_buff, modem_at_cmd_buff,
				sizeof(modem_at_cmd_buff), &state);
#else
		/* Proper error msg has already been copied to buffer */
		err = 0;
#endif
	}

	/* Get response length; same buffer was used for the response. */
	len = strlen(modem_at_cmd_buff);

	if (err) {
		len = snprintf(modem_at_cmd_buff, sizeof(modem_at_cmd_buff),
				"AT CMD error: %d, state %d", err, state);
	} else if (len == 0) {
		len = snprintf(modem_at_cmd_buff, sizeof(modem_at_cmd_buff),
				"OK\r\n");
	} else if (len > MODEM_AT_CMD_MAX_RESPONSE_LEN) {
		len = snprintf(modem_at_cmd_buff, sizeof(modem_at_cmd_buff),
				MODEM_AT_CMD_RESP_TOO_BIG_STR);
	}

	modem_data.data.buf = modem_at_cmd_buff;
	modem_data.data.len = len;

	err = cloud_encode_data(&modem_data, CLOUD_CMD_GROUP_COMMAND, &msg);
	if (err) {
		LOG_ERR("[%s:%d] cloud_encode_data failed with error %d",
			__func__, __LINE__, err);
	} else {
		err = cloud_send(cloud_backend, &msg);
		cloud_release_data(&msg);
		if (err) {
			LOG_ERR("[%s:%d] cloud_send failed with error %d",
				__func__, __LINE__, err);
		}
	}

	k_sem_give(&modem_at_cmd_sem);
}

/**@brief Callback for GPS trigger events */
static void gps_trigger_handler(struct device *dev, struct gps_trigger *trigger)
{
	static u32_t fix_count;

	ARG_UNUSED(trigger);

	if (!atomic_get(&send_data_enable)) {
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
	k_work_submit_to_queue(&application_work_q, &send_gps_data_work);
	env_sensors_poll();
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

	k_work_submit_to_queue(&application_work_q, &send_button_data_work);
}
#endif

/**@brief Callback from the motion module. Sends motion data to cloud. */
static void motion_handler(motion_data_t  motion_data)
{
	static motion_orientation_state_t last_orientation_state =
		MOTION_ORIENTATION_NOT_KNOWN;

	if (motion_data.orientation == last_orientation_state) {
		return;
	}

	if (!flip_mode_enabled || !atomic_get(&send_data_enable)
		|| gps_control_is_active()) {
		return;
	}

	struct cloud_msg msg = {
		.qos = CLOUD_QOS_AT_MOST_ONCE,
		.endpoint.type = CLOUD_EP_TOPIC_MSG
	};

	int err;

	if (cloud_encode_motion_data(&motion_data, &msg) == 0) {
		err = cloud_send(cloud_backend, &msg);
		cloud_release_data(&msg);
		if (err) {
			LOG_ERR("Transmisison of motion data failed: %d", err);
			cloud_error_handler(err);
			return;
		}
	}

	last_orientation_state = motion_data.orientation;
}

static void cloud_cmd_handle_modem_at_cmd(const char * const at_cmd)
{
	if (!at_cmd) {
		return;
	}

	if (k_sem_take(&modem_at_cmd_sem, K_MSEC(20)) != 0) {
		LOG_ERR("[%s:%d] Modem AT cmd in progress.", __func__,
		       __LINE__);
		return;
	}

#if defined(CONFIG_AT_CMD)
	const size_t max_cmd_len = sizeof(modem_at_cmd_buff);

	if (strnlen(at_cmd, max_cmd_len) == max_cmd_len) {
		LOG_ERR("[%s:%d] AT cmd is too long, max length is %zu",
		       __func__, __LINE__, max_cmd_len - 1);
		/* Empty string will be handled as an error */
		modem_at_cmd_buff[0] = '\0';
	} else {
		strcpy(modem_at_cmd_buff, at_cmd);
	}
#else
	snprintf(modem_at_cmd_buff, sizeof(modem_at_cmd_buff),
			MODEM_AT_CMD_NOT_ENABLED_STR);
#endif

	k_work_submit_to_queue(&application_work_q, &send_modem_at_cmd_work);
}

static void cloud_cmd_handler(struct cloud_command *cmd)
{
	if ((cmd->channel == CLOUD_CHANNEL_GPS) &&
	    (cmd->group == CLOUD_CMD_GROUP_CFG_SET) &&
	    (cmd->type == CLOUD_CMD_ENABLE)) {
		set_gps_enable(cmd->data.sv.state == CLOUD_CMD_STATE_TRUE);
	} else if ((cmd->channel == CLOUD_CHANNEL_MODEM) &&
		   (cmd->group == CLOUD_CMD_GROUP_COMMAND) &&
		   (cmd->type == CLOUD_CMD_DATA_STRING)) {
		cloud_cmd_handle_modem_at_cmd(cmd->data.data_string);
	} else if ((cmd->channel == CLOUD_CHANNEL_RGB_LED) &&
		   (cmd->group == CLOUD_CMD_GROUP_CFG_SET) &&
		   (cmd->type == CLOUD_CMD_COLOR)) {
		ui_led_set_color(((u32_t)cmd->data.sv.value >> 16) & 0xFF,
				 ((u32_t)cmd->data.sv.value >> 8) & 0xFF,
				 ((u32_t)cmd->data.sv.value) & 0xFF);
	} else if ((cmd->channel == CLOUD_CHANNEL_DEVICE_INFO) &&
		   (cmd->group == CLOUD_CMD_GROUP_GET) &&
		   (cmd->type == CLOUD_CMD_EMPTY)) {
		k_work_submit_to_queue(&application_work_q,
				       &device_status_work);
	} else if ((cmd->channel == CLOUD_CHANNEL_LTE_LINK_RSRP) &&
		   (cmd->group == CLOUD_CMD_GROUP_GET) &&
		   (cmd->type == CLOUD_CMD_EMPTY)) {
#if CONFIG_MODEM_INFO
		k_work_submit_to_queue(&application_work_q, &rsrp_work);
#endif
	} else if ((cmd->group == CLOUD_CMD_GROUP_CFG_SET) &&
			   (cmd->type == CLOUD_CMD_INTERVAL)) {
		if (cmd->channel == CLOUD_CHANNEL_LIGHT_SENSOR) {
#if defined(CONFIG_LIGHT_SENSOR)
			light_sensor_set_send_interval(
				(u32_t)cmd->data.sv.value);
#endif
		} else if (cmd->channel == CLOUD_CHANNEL_ENVIRONMENT) {
			env_sensors_set_send_interval(
				(u32_t)cmd->data.sv.value);
		} else if (cmd->channel == CLOUD_CHANNEL_GPS) {
			/* TODO: update GPS controller to handle send */
			/* interval */
		} else {
			LOG_ERR("Interval command not valid for channel %d",
				cmd->channel);
		}
	}
}

#if CONFIG_MODEM_INFO
/**@brief Callback handler for LTE RSRP data. */
static void modem_rsrp_handler(char rsrp_value)
{
	rsrp.value = rsrp_value;

	/* If the RSRP value is 255, it's documented as 'not known or not
	 * detectable'. Therefore, we should not send those values.
	 */
	if (rsrp.value == 255) {
		return;
	}

	k_work_submit_to_queue(&application_work_q, &rsrp_work);
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
#endif /* CONFIG_MODEM_INFO */

/**@brief Poll device info and send data to the cloud. */
static void device_status_send(struct k_work *work)
{
	if (!atomic_get(&send_data_enable)) {
		return;
	}

	cJSON *root_obj = cJSON_CreateObject();

	if (root_obj == NULL) {
		LOG_ERR("Unable to allocate JSON object");
		return;
	}

	size_t item_cnt = 0;

#ifdef CONFIG_MODEM_INFO
	int ret = modem_info_params_get(&modem_param);

	if (ret < 0) {
		LOG_ERR("Unable to obtain modem parameters: %d", ret);
	} else {
		ret = modem_info_json_object_encode(&modem_param, root_obj);
		if (ret > 0) {
			item_cnt = (size_t)ret;
		}
	}
#endif /* CONFIG_MODEM_INFO */

	const char *const ui[] = {
		CLOUD_CHANNEL_STR_GPS,
		CLOUD_CHANNEL_STR_FLIP,
		CLOUD_CHANNEL_STR_TEMP,
		CLOUD_CHANNEL_STR_HUMID,
		CLOUD_CHANNEL_STR_AIR_PRESS,
#if IS_ENABLED(CONFIG_CLOUD_BUTTON)
		CLOUD_CHANNEL_STR_BUTTON,
#endif
#if IS_ENABLED(CONFIG_LIGHT_SENSOR)
		CLOUD_CHANNEL_STR_LIGHT_SENSOR,
#endif
	};

	const char *const fota[] = {
#if defined(CONFIG_CLOUD_FOTA_APP)
		SERVICE_INFO_FOTA_STR_APP,
#endif
#if defined(CONFIG_CLOUD_FOTA_MODEM)
		SERVICE_INFO_FOTA_STR_MODEM
#endif
	};

	if (service_info_json_object_encode(ui, ARRAY_SIZE(ui),
					    fota, ARRAY_SIZE(fota),
					    SERVICE_INFO_FOTA_VER_CURRENT,
					    root_obj) == 0) {
		++item_cnt;
	}

	if (item_cnt == 0) {
		cJSON_Delete(root_obj);
		return;
	}

	device_cloud_data.data.buf = (char *)root_obj;
	device_cloud_data.data.len = item_cnt;
	device_cloud_data.tag += 1;

	if (device_cloud_data.tag == 0) {
		device_cloud_data.tag = 0x1;
	}

	/* Transmits the data to the cloud. Frees the JSON object. */
	sensor_data_send(&device_cloud_data);
}

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
		env_sensors_set_backoff_enable(true);
		return;
	}

	env_sensors_set_backoff_enable(false);

	if (env_sensors_get_temperature(&env_data) == 0) {
		if (cloud_is_send_allowed(CLOUD_CHANNEL_TEMP, env_data.value) &&
		    cloud_encode_env_sensors_data(&env_data, &msg) == 0) {
			err = cloud_send(cloud_backend, &msg);
			cloud_release_data(&msg);
			if (err) {
				goto error;
			}
		}
	}

	if (env_sensors_get_humidity(&env_data) == 0) {
		if (cloud_is_send_allowed(CLOUD_CHANNEL_HUMID,
					  env_data.value) &&
		    cloud_encode_env_sensors_data(&env_data, &msg) == 0) {
			err = cloud_send(cloud_backend, &msg);
			cloud_release_data(&msg);
			if (err) {
				goto error;
			}
		}
	}

	if (env_sensors_get_pressure(&env_data) == 0) {
		if (cloud_is_send_allowed(CLOUD_CHANNEL_AIR_PRESS,
					  env_data.value) &&
		    cloud_encode_env_sensors_data(&env_data, &msg) == 0) {
			err = cloud_send(cloud_backend, &msg);
			cloud_release_data(&msg);
			if (err) {
				goto error;
			}
		}
	}

	if (env_sensors_get_air_quality(&env_data) == 0) {
		if (cloud_is_send_allowed(CLOUD_CHANNEL_AIR_QUAL,
					  env_data.value) &&
		    cloud_encode_env_sensors_data(&env_data, &msg) == 0) {
			err = cloud_send(cloud_backend, &msg);
			cloud_release_data(&msg);
			if (err) {
				goto error;
			}
		}
	}

	return;
error:
	LOG_ERR("sensor_data_send failed: %d", err);
	cloud_error_handler(err);
}

#if defined(CONFIG_LIGHT_SENSOR)
void light_sensor_data_send(void)
{
	int err;
	struct light_sensor_data light_data;
	struct cloud_msg msg = { .qos = CLOUD_QOS_AT_MOST_ONCE,
				 .endpoint.type = CLOUD_EP_TOPIC_MSG };

	if (!atomic_get(&send_data_enable) || gps_control_is_active()) {
		return;
	}

	err = light_sensor_get_data(&light_data);
	if (err) {
		LOG_ERR("Failed to get light sensor data, error %d", err);
		return;
	}

	if (!cloud_is_send_allowed(CLOUD_CHANNEL_LIGHT_RED, light_data.red) &&
	    !cloud_is_send_allowed(CLOUD_CHANNEL_LIGHT_GREEN,
				   light_data.green) &&
	    !cloud_is_send_allowed(CLOUD_CHANNEL_LIGHT_BLUE, light_data.blue) &&
	    !cloud_is_send_allowed(CLOUD_CHANNEL_LIGHT_IR, light_data.ir)) {
		LOG_WRN("Light values not sent due to config settings");
		return;
	}

	err = cloud_encode_light_sensor_data(&light_data, &msg);
	if (err) {
		LOG_ERR("Failed to encode light sensor data, error %d", err);
		return;
	}

	err = cloud_send(cloud_backend, &msg);
	cloud_release_data(&msg);

	if (err) {
		LOG_ERR("Failed to send light sensor data to cloud, error: %d",
		       err);
		cloud_error_handler(err);
	}
}
#endif /* CONFIG_LIGHT_SENSOR */

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
		err = cloud_encode_data(data, CLOUD_CMD_GROUP_DATA, &msg);
	} else {
		err = cloud_encode_digital_twin_data(data, &msg);
	}

	if (err) {
		LOG_ERR("Unable to encode cloud data: %d", err);
	}

	err = cloud_send(cloud_backend, &msg);

	cloud_release_data(&msg);

	if (err) {
		LOG_ERR("sensor_data_send failed: %d", err);
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
	if (atomic_get(&association_requested) == 0) {
		atomic_set(&association_requested, 1);
		ui_led_set_pattern(UI_CLOUD_PAIRING);
		LOG_INF("Add device to cloud account.");
		LOG_INF("Waiting for cloud association...");

		/* If the association is not done soon enough (< ~5 min?) */
		/* a connection cycle is needed... TBD why. */
		k_delayed_work_submit_to_queue(&application_work_q,
					       &cycle_cloud_connection_work,
					       CONN_CYCLE_AFTER_ASSOCIATION_REQ_MS);
	}
}

static void cycle_cloud_connection(struct k_work *work)
{
	int err;
	s32_t reboot_wait_ms = REBOOT_AFTER_DISCONNECT_WAIT_MS;

	LOG_INF("Disconnecting from cloud...");

	err = cloud_disconnect(cloud_backend);
	if (err == 0) {
		atomic_set(&reconnect_to_cloud, 1);
	} else {
		reboot_wait_ms = K_SECONDS(5);
		LOG_INF("Disconnect failed. Device will reboot in %d seconds",
			(reboot_wait_ms/MSEC_PER_SEC));
	}

	/* Reboot fail-safe on disconnect */
	k_delayed_work_submit_to_queue(&application_work_q, &cloud_reboot_work,
				       reboot_wait_ms);
}

/** @brief Handle procedures after successful association with nRF Cloud. */
void on_pairing_done(void)
{
	if (atomic_get(&association_requested)) {
		atomic_set(&association_requested, 0);
		k_delayed_work_cancel(&cycle_cloud_connection_work);

		/* After successful association, the device must */
		/* reconnect to the cloud. */
		LOG_INF("Device associated with cloud.");
		LOG_INF("Reconnecting for cloud policy to take effect.");
		cycle_cloud_connection(NULL);
	}
}

void cloud_event_handler(const struct cloud_backend *const backend,
			 const struct cloud_event *const evt,
			 void *user_data)
{
	ARG_UNUSED(user_data);

	switch (evt->type) {
	case CLOUD_EVT_CONNECTED:
		LOG_INF("CLOUD_EVT_CONNECTED");
		k_delayed_work_cancel(&cloud_reboot_work);
		ui_led_set_pattern(UI_CLOUD_CONNECTED);
		break;
	case CLOUD_EVT_READY:
		LOG_INF("CLOUD_EVT_READY");
		ui_led_set_pattern(UI_CLOUD_CONNECTED);

#if defined(CONFIG_BOOTLOADER_MCUBOOT)
		/* Mark image as good to avoid rolling back after update */
		boot_write_img_confirmed();
#endif

		sensors_start();
		break;
	case CLOUD_EVT_DISCONNECTED:
		LOG_INF("CLOUD_EVT_DISCONNECTED");
		ui_led_set_pattern(UI_LTE_DISCONNECTED);
		/* Expect an error event (POLLNVAL) on the cloud socket poll */
		/* Handle reconnect there if desired */
		break;
	case CLOUD_EVT_ERROR:
		LOG_INF("CLOUD_EVT_ERROR");
		break;
	case CLOUD_EVT_DATA_SENT:
		LOG_INF("CLOUD_EVT_DATA_SENT");
		break;
	case CLOUD_EVT_DATA_RECEIVED:
		LOG_INF("CLOUD_EVT_DATA_RECEIVED");
		cloud_decode_command(evt->data.msg.buf);
		break;
	case CLOUD_EVT_PAIR_REQUEST:
		LOG_INF("CLOUD_EVT_PAIR_REQUEST");
		on_user_pairing_req(evt);
		break;
	case CLOUD_EVT_PAIR_DONE:
		LOG_INF("CLOUD_EVT_PAIR_DONE");
		on_pairing_done();
		break;
	case CLOUD_EVT_FOTA_DONE:
		LOG_INF("CLOUD_EVT_FOTA_DONE");
		sys_reboot(SYS_REBOOT_COLD);
		break;
	default:
		LOG_WRN("Unknown cloud event type: %d", evt->type);
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
		LOG_ERR("cloud_connect failed: %d", err);
		cloud_error_handler(err);
	}
}

static void set_gps_enable(const bool enable)
{
	if (enable == gps_control_is_enabled()) {
		return;
	}

	if (enable) {
		LOG_INF("Starting GPS");
		gps_control_enable();
		gps_control_start(K_SECONDS(1));

	} else {
		LOG_INF("Stopping GPS");
		gps_control_disable();
	}
}

static void long_press_handler(struct k_work *work)
{
	if (!atomic_get(&send_data_enable)) {
		LOG_INF("Link not ready, long press disregarded");
		return;
	}

	/* Toggle GPS state */
	set_gps_enable(!gps_control_is_enabled());
}

/**@brief Initializes and submits delayed work. */
static void work_init(void)
{
	k_work_q_start(&application_work_q, application_stack_area,
		       K_THREAD_STACK_SIZEOF(application_stack_area),
		       CONFIG_APPLICATION_WORKQUEUE_PRIORITY);
	k_work_init(&connect_work, app_connect);
	k_work_init(&send_gps_data_work, send_gps_data_work_fn);
	k_work_init(&send_button_data_work, send_button_data_work_fn);
	k_work_init(&send_modem_at_cmd_work, send_modem_at_cmd_work_fn);
	k_delayed_work_init(&long_press_button_work, long_press_handler);
	k_delayed_work_init(&cloud_reboot_work, cloud_reboot_handler);
	k_delayed_work_init(&cycle_cloud_connection_work,
			    cycle_cloud_connection);
	k_work_init(&device_status_work, device_status_send);
#if CONFIG_MODEM_INFO
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

		LOG_INF("Connecting to LTE network. ");
		LOG_INF("This may take several minutes.");
		ui_led_set_pattern(UI_LTE_CONNECTING);

		err = lte_lc_init_and_connect();
		if (err) {
			LOG_ERR("LTE link could not be established.");
			error_handler(ERROR_LTE_LC, err);
		}

		LOG_INF("Connected to LTE network");
		ui_led_set_pattern(UI_LTE_CONNECTED);
	}
#endif
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
		LOG_ERR("Modem info could not be established: %d", err);
		return;
	}

	modem_info_params_init(&modem_param);

	signal_strength_cloud_data.type = CLOUD_CHANNEL_LTE_LINK_RSRP;
	signal_strength_cloud_data.tag = 0x1;

	modem_info_rsrp_register(modem_rsrp_handler);
}
#endif /* CONFIG_MODEM_INFO */

/**@brief Initializes the sensors that are used by the application. */
static void sensors_init(void)
{
	int err;

	err = motion_init_and_start(motion_handler);
	if (err) {
		LOG_ERR("motion module init failed, error: %d", err);
	}

	err = env_sensors_init_and_start(env_data_send);
	if (err) {
		LOG_ERR("Environmental sensors init failed, error: %d", err);
	}
#if CONFIG_LIGHT_SENSOR
	err = light_sensor_init_and_start(light_sensor_data_send);
	if (err) {
		LOG_ERR("Light sensor init failed, error: %d", err);
	}
#endif /* CONFIG_LIGHT_SENSOR */
#if CONFIG_MODEM_INFO
	modem_data_init();
#endif /* CONFIG_MODEM_INFO */

	k_work_submit_to_queue(&application_work_q, &device_status_work);

	if (IS_ENABLED(CONFIG_CLOUD_BUTTON)) {
		button_sensor_init();
	}

	gps_control_init(&application_work_q, gps_trigger_handler);
	if (IS_ENABLED(CONFIG_GPS_START_AFTER_CLOUD_EVT_READY)) {
		set_gps_enable(true);
	}
}

#if defined(CONFIG_USE_UI_MODULE)
/**@brief User interface event handler. */
static void ui_evt_handler(struct ui_evt evt)
{
	if (IS_ENABLED(CONFIG_CLOUD_BUTTON) &&
	   (evt.button == CONFIG_CLOUD_BUTTON_INPUT)) {
		button_send(evt.type == UI_EVT_BUTTON_ACTIVE ? 1 : 0);
	}

	if (IS_ENABLED(CONFIG_ACCEL_USE_SIM) && (evt.button == FLIP_INPUT)
	   && atomic_get(&send_data_enable)) {
		motion_simulate_trigger();
	}

	if (IS_ENABLED(CONFIG_GPS_CONTROL_ON_LONG_PRESS) &&
	   (evt.button == UI_BUTTON_1)) {
		if (evt.type == UI_EVT_BUTTON_ACTIVE) {
			k_delayed_work_submit_to_queue(&application_work_q,
						       &long_press_button_work,
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

void handle_bsdlib_init_ret(void)
{
	#if defined(CONFIG_BSD_LIBRARY)
	int ret = bsdlib_get_init_ret();

	/* Handle return values relating to modem firmware update */
	switch (ret) {
	case MODEM_DFU_RESULT_OK:
		LOG_INF("MODEM UPDATE OK. Will run new firmware");
		sys_reboot(SYS_REBOOT_COLD);
		break;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		LOG_ERR("MODEM UPDATE ERROR %d. Will run old firmware", ret);
		sys_reboot(SYS_REBOOT_COLD);
		break;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		LOG_ERR("MODEM UPDATE FATAL ERROR %d. Modem failiure", ret);
		sys_reboot(SYS_REBOOT_COLD);
		break;
	default:
		break;
	}
	#endif /* CONFIG_BSD_LIBRARY */
}

void main(void)
{
	int ret;

	LOG_INF("Asset tracker started");

	handle_bsdlib_init_ret();

	cloud_backend = cloud_get_binding("NRF_CLOUD");
	__ASSERT(cloud_backend != NULL, "nRF Cloud backend not found");

	ret = cloud_init(cloud_backend, cloud_event_handler);
	if (ret) {
		LOG_ERR("Cloud backend could not be initialized, error: %d",
			ret);
		cloud_error_handler(ret);
	}

#if defined(CONFIG_USE_UI_MODULE)
	ui_init(ui_evt_handler);
#endif

	ret = cloud_decode_init(cloud_cmd_handler);
	if (ret) {
		LOG_ERR("Cloud command decoder could not be initialized, error: %d", ret);
		cloud_error_handler(ret);
	}

	work_init();
	modem_configure();
connect:
	ret = cloud_connect(cloud_backend);
	if (ret) {
		LOG_ERR("cloud_connect failed: %d", ret);
		cloud_error_handler(ret);
	} else {
		atomic_set(&reconnect_to_cloud, 0);
		k_delayed_work_submit_to_queue(&application_work_q,
					       &cloud_reboot_work,
					       CLOUD_CONNACK_WAIT_DURATION);
	}

	struct pollfd fds[] = {
		{
			.fd = cloud_backend->config->socket,
			.events = POLLIN
		}
	};

	while (true) {
		/* The timeout is set to (keepalive / 3), so that the worst case
		 * time between two messages from device to broker is
		 * ((4 / 3) * keepalive + connection overhead), which is within
		 * MQTT specification of (1.5 * keepalive) before the broker
		 * must close the connection.
		 */
		ret = poll(fds, ARRAY_SIZE(fds),
			K_SECONDS(CONFIG_MQTT_KEEPALIVE / 3));

		if (ret < 0) {
			LOG_ERR("poll() returned an error: %d", ret);
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
			if (atomic_get(&reconnect_to_cloud)) {
				k_delayed_work_cancel(&cloud_reboot_work);
				LOG_INF("Attempting reconnect...");
				goto connect;
			}
			LOG_ERR("Socket error: POLLNVAL");
			LOG_ERR("The cloud socket was unexpectedly closed.");
			error_handler(ERROR_CLOUD, -EIO);
			return;
		}

		if ((fds[0].revents & POLLHUP) == POLLHUP) {
			LOG_ERR("Socket error: POLLHUP");
			LOG_ERR("Connection was closed by the cloud.");
			error_handler(ERROR_CLOUD, -EIO);
			return;
		}

		if ((fds[0].revents & POLLERR) == POLLERR) {
			LOG_ERR("Socket error: POLLERR");
			LOG_ERR("Cloud connection was unexpectedly closed.");
			error_handler(ERROR_CLOUD, -EIO);
			return;
		}
	}

	cloud_disconnect(cloud_backend);
	goto connect;
}
