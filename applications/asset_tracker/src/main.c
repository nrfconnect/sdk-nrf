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
#include <drivers/sensor.h>
#include <console/console.h>
#include <power/reboot.h>
#include <logging/log_ctrl.h>
#if defined(CONFIG_BSD_LIBRARY)
#include <modem/bsdlib.h>
#include <bsd.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#endif /* CONFIG_BSD_LIBRARY */
#include <net/cloud.h>
#include <net/socket.h>
#include <net/nrf_cloud.h>
#if defined(CONFIG_NRF_CLOUD_AGPS)
#include <net/nrf_cloud_agps.h>
#endif

#if defined(CONFIG_LWM2M_CARRIER)
#include <lwm2m_carrier.h>
#endif

#if defined(CONFIG_BOOTLOADER_MCUBOOT)
#include <dfu/mcuboot.h>
#endif

#include "cloud_codec.h"
#include "env_sensors.h"
#include "motion.h"
#include "ui.h"
#include "service_info.h"
#include <modem/at_cmd.h>
#include "watchdog.h"
#include "gps_controller.h"
#include <date_time.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(asset_tracker, CONFIG_ASSET_TRACKER_LOG_LEVEL);

#define CALIBRATION_PRESS_DURATION  K_SECONDS(5)
#define CLOUD_CONNACK_WAIT_DURATION (CONFIG_CLOUD_WAIT_DURATION * MSEC_PER_SEC)

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

/* Timeout in seconds in which the application will wait for an initial event
 * from the date time library.
 */
#define DATE_TIME_TIMEOUT_S 15

/* Interval in milliseconds after which the device will reboot
 * if the disconnect event has not been handled.
 */
#define REBOOT_AFTER_DISCONNECT_WAIT_MS     (15 * MSEC_PER_SEC)

/* Interval in milliseconds after which the device will
 * disconnect and reconnect if association was not completed.
 */
#define CONN_CYCLE_AFTER_ASSOCIATION_REQ_MS K_MINUTES(5)

struct rsrp_data {
	uint16_t value;
	uint16_t offset;
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
static struct gps_nmea gps_data;
static struct cloud_channel_data gps_cloud_data = {
	.type = CLOUD_CHANNEL_GPS,
	.tag = 0x1
};
static struct cloud_channel_data button_cloud_data;

#if CONFIG_MODEM_INFO
static struct modem_param_info modem_param;
static struct cloud_channel_data signal_strength_cloud_data;
#endif /* CONFIG_MODEM_INFO */

static struct gps_agps_request agps_request;

static int64_t gps_last_active_time;
static int64_t gps_last_search_start_time;
static atomic_t carrier_requested_disconnect;
static atomic_t cloud_connect_attempts;

/* Flag used for flip detection */
static bool flip_mode_enabled = true;
static motion_data_t last_motion_data = {
	.orientation = MOTION_ORIENTATION_NOT_KNOWN,
};

#if IS_ENABLED(CONFIG_GPS_START_ON_MOTION)
/* Current state of activity monitor */
static motion_activity_state_t last_activity_state = MOTION_ACTIVITY_NOT_KNOWN;
#endif

/* Variable to keep track of nRF cloud association state. */
enum cloud_association_state {
	CLOUD_ASSOCIATION_STATE_INIT,
	CLOUD_ASSOCIATION_STATE_REQUESTED,
	CLOUD_ASSOCIATION_STATE_PAIRED,
	CLOUD_ASSOCIATION_STATE_RECONNECT,
	CLOUD_ASSOCIATION_STATE_READY,
};
static atomic_val_t cloud_association =
	ATOMIC_INIT(CLOUD_ASSOCIATION_STATE_INIT);

/* Structures for work */
static struct k_work sensors_start_work;
static struct k_work send_gps_data_work;
static struct k_work send_button_data_work;
static struct k_work send_modem_at_cmd_work;
static struct k_delayed_work long_press_button_work;
static struct k_delayed_work cloud_reboot_work;
static struct k_delayed_work cycle_cloud_connection_work;
static struct k_delayed_work device_config_work;
static struct k_delayed_work cloud_connect_work;
static struct k_work device_status_work;
static struct k_delayed_work send_agps_request_work;
static struct k_work motion_data_send_work;
static struct k_work no_sim_go_offline_work;

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
static K_SEM_DEFINE(cloud_disconnected, 0, 1);
#if defined(CONFIG_LWM2M_CARRIER)
static void app_disconnect(void);
static K_SEM_DEFINE(bsdlib_initialized, 0, 1);
static K_SEM_DEFINE(lte_connected, 0, 1);
static K_SEM_DEFINE(cloud_ready_to_connect, 0, 1);
#endif
static K_SEM_DEFINE(date_time_obtained, 0, 1);

#if CONFIG_MODEM_INFO
static struct k_delayed_work rsrp_work;
#endif /* CONFIG_MODEM_INFO */

enum error_type {
	ERROR_CLOUD,
	ERROR_BSD_RECOVERABLE,
	ERROR_LTE_LC,
	ERROR_SYSTEM_FAULT
};

/* Forward declaration of functions */
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
static bool data_send_enabled(void);
static void connection_evt_handler(const struct cloud_event *const evt);
static void no_sim_go_offline(struct k_work *work);

static void shutdown_modem(void)
{
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

#if defined(CONFIG_LWM2M_CARRIER)
int lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
{
	switch (event->type) {
	case LWM2M_CARRIER_EVENT_BSDLIB_INIT:
		LOG_INF("LWM2M_CARRIER_EVENT_BSDLIB_INIT");
		k_sem_give(&bsdlib_initialized);
		break;
	case LWM2M_CARRIER_EVENT_CONNECTING:
		LOG_INF("LWM2M_CARRIER_EVENT_CONNECTING\n");
		break;
	case LWM2M_CARRIER_EVENT_CONNECTED:
		LOG_INF("LWM2M_CARRIER_EVENT_CONNECTED");
		k_sem_give(&lte_connected);
		break;
	case LWM2M_CARRIER_EVENT_DISCONNECTING:
		LOG_INF("LWM2M_CARRIER_EVENT_DISCONNECTING");
		break;
	case LWM2M_CARRIER_EVENT_BOOTSTRAPPED:
		LOG_INF("LWM2M_CARRIER_EVENT_BOOTSTRAPPED");
		break;
	case LWM2M_CARRIER_EVENT_DISCONNECTED:
		LOG_INF("LWM2M_CARRIER_EVENT_DISCONNECTED");
		break;
	case LWM2M_CARRIER_EVENT_READY:
		LOG_INF("LWM2M_CARRIER_EVENT_READY");
		k_sem_give(&cloud_ready_to_connect);
		break;
	case LWM2M_CARRIER_EVENT_FOTA_START:
		LOG_INF("LWM2M_CARRIER_EVENT_FOTA_START");
		/* Due to limitations in the number of secure sockets,
		 * the cloud socket has to be closed when the carrier
		 * library initiates firmware upgrade download.
		 */
		atomic_set(&carrier_requested_disconnect, 1);
		app_disconnect();
		break;
	case LWM2M_CARRIER_EVENT_REBOOT:
		LOG_INF("LWM2M_CARRIER_EVENT_REBOOT");
		break;
	case LWM2M_CARRIER_EVENT_ERROR:
		LOG_ERR("LWM2M_CARRIER_EVENT_ERROR: code %d, value %d",
			((lwm2m_carrier_event_error_t *)event->data)->code,
			((lwm2m_carrier_event_error_t *)event->data)->value);
		break;
	}

	return 0;
}

/**@brief Disconnects from cloud. First it tries using the cloud backend's
 *        disconnect() implementation. If that fails, it falls back to close the
 *        socket directly, using close().
 */
static void app_disconnect(void)
{
	int err;

	atomic_set(&cloud_association, CLOUD_ASSOCIATION_STATE_INIT);
	LOG_INF("Disconnecting from cloud.");

	err = cloud_disconnect(cloud_backend);
	if (err == 0) {
		/* Ensure that the socket is indeed closed before returning. */
		if (k_sem_take(&cloud_disconnected, K_MINUTES(1)) == 0) {
			LOG_INF("Disconnected from cloud.");
			return;
		}
	} else if (err == -ENOTCONN) {
		LOG_INF("Cloud connection was not established.");
		return;
	} else {
		LOG_ERR("Could not disconnect from cloud, err: %d", err);
	}

	LOG_INF("Closing the cloud socket directly.");

	err = close(cloud_backend->config->socket);
	if (err) {
		LOG_ERR("Failed to close socket, error: %d", err);
		return;
	}

	LOG_INF("Socket was closed successfully.");
	return;
}
#endif /* defined(CONFIG_LWM2M_CARRIER) */

/**@brief nRF Cloud error handler. */
void error_handler(enum error_type err_type, int err_code)
{
	atomic_set(&cloud_association, CLOUD_ASSOCIATION_STATE_INIT);

	if (err_type == ERROR_CLOUD) {
		shutdown_modem();
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

static void send_agps_request(struct k_work *work)
{
	ARG_UNUSED(work);

#if defined(CONFIG_AGPS)
	int err;
	static int64_t last_request_timestamp;

/* Request A-GPS data no more often than every hour (time in milliseconds). */
#define AGPS_UPDATE_PERIOD (60 * 60 * 1000)

	if ((last_request_timestamp != 0) &&
	    (k_uptime_get() - last_request_timestamp) < AGPS_UPDATE_PERIOD) {
		LOG_WRN("A-GPS request was sent less than 1 hour ago");
		return;
	}

	LOG_INF("Sending A-GPS request");

	err = gps_agps_request(agps_request, GPS_SOCKET_NOT_PROVIDED);
	if (err) {
		LOG_ERR("Failed to request A-GPS data, error: %d", err);
		return;
	}

	last_request_timestamp = k_uptime_get();

	LOG_INF("A-GPS request sent");
#endif /* defined(CONFIG_AGPS) */
}

void cloud_connect_error_handler(enum cloud_connect_result err)
{
	bool reboot = true;
	char *backend_name = "invalid";

	if (err == CLOUD_CONNECT_RES_SUCCESS) {
		return;
	}

	LOG_ERR("Failed to connect to cloud, error %d", err);

	switch (err) {
	case CLOUD_CONNECT_RES_ERR_NOT_INITD: {
		LOG_ERR("Cloud back-end has not been initialized");
		/* no need to reboot, program error */
		reboot = false;
		break;
	}
	case CLOUD_CONNECT_RES_ERR_NETWORK: {
		LOG_ERR("Network error, check cloud configuration");
		break;
	}
	case CLOUD_CONNECT_RES_ERR_BACKEND: {
		if (cloud_backend && cloud_backend->config &&
		    cloud_backend->config->name) {
			backend_name = cloud_backend->config->name;
		}
		LOG_ERR("An error occurred specific to the cloud back-end: %s",
			backend_name);
		break;
	}
	case CLOUD_CONNECT_RES_ERR_PRV_KEY: {
		LOG_ERR("Ensure device has a valid private key");
		break;
	}
	case CLOUD_CONNECT_RES_ERR_CERT: {
		LOG_ERR("Ensure device has a valid CA and client certificate");
		break;
	}
	case CLOUD_CONNECT_RES_ERR_CERT_MISC: {
		LOG_ERR("A certificate/authorization error has occurred");
		break;
	}
	case CLOUD_CONNECT_RES_ERR_TIMEOUT_NO_DATA: {
		LOG_ERR("Connect timeout. SIM card may be out of data");
		break;
	}
	case CLOUD_CONNECT_RES_ERR_ALREADY_CONNECTED: {
		LOG_ERR("Connection already exists.");
		break;
	}
	case CLOUD_CONNECT_RES_ERR_MISC: {
		break;
	}
	default: {
		LOG_ERR("Unhandled connect error");
		break;
	}
	}

	if (reboot) {
		LOG_ERR("Device will reboot in %d seconds",
				CONFIG_CLOUD_CONNECT_ERR_REBOOT_S);
		k_delayed_work_submit_to_queue(
			&application_work_q, &cloud_reboot_work,
			K_SECONDS(CONFIG_CLOUD_CONNECT_ERR_REBOOT_S));
	}

	ui_led_set_pattern(UI_LED_ERROR_CLOUD);
	shutdown_modem();
	k_thread_suspend(k_current_get());
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

void connect_to_cloud(const int32_t connect_delay_s)
{
	static bool initial_connect = true;

	/* Ensure no data can be sent to cloud before connection is established.
	 */
	atomic_set(&cloud_association, CLOUD_ASSOCIATION_STATE_INIT);

	if (atomic_get(&carrier_requested_disconnect)) {
		/* A disconnect was requested to free up the TLS socket
		 * used by the cloud.  If enabled, the carrier library
		 * (CONFIG_LWM2M_CARRIER) will perform FOTA updates in
		 * the background and reboot the device when complete.
		 */
		return;
	}

	atomic_inc(&cloud_connect_attempts);

	/* Check if max cloud connect retry count is exceeded. */
	if (atomic_get(&cloud_connect_attempts) >
	    CONFIG_CLOUD_CONNECT_COUNT_MAX) {
		LOG_ERR("The max cloud connection attempt count exceeded.");
		cloud_error_handler(-ETIMEDOUT);
	}

	if (!initial_connect) {
		LOG_INF("Attempting reconnect in %d seconds...",
			connect_delay_s);
		k_delayed_work_cancel(&cloud_reboot_work);
	} else {
		initial_connect = false;
	}

	k_delayed_work_submit_to_queue(&application_work_q,
				       &cloud_connect_work,
				       K_SECONDS(connect_delay_s));
}

static void cloud_connect_work_fn(struct k_work *work)
{
	int ret;

	LOG_INF("Connecting to cloud, attempt %d of %d",
	       atomic_get(&cloud_connect_attempts),
		   CONFIG_CLOUD_CONNECT_COUNT_MAX);

	k_delayed_work_submit_to_queue(&application_work_q,
			&cloud_reboot_work,
			K_MSEC(CLOUD_CONNACK_WAIT_DURATION));

	ui_led_set_pattern(UI_CLOUD_CONNECTING);

	/* Attempt cloud connection */
	ret = cloud_connect(cloud_backend);
	if (ret != CLOUD_CONNECT_RES_SUCCESS) {
		k_delayed_work_cancel(&cloud_reboot_work);
		/* Will not return from this function.
		 * If the connect fails here, it is likely
		 * that user intervention is required.
		 */
		cloud_connect_error_handler(ret);
	} else {
		LOG_INF("Cloud connection request sent.");
		LOG_INF("Connection response timeout is set to %d seconds.",
		       CLOUD_CONNACK_WAIT_DURATION / MSEC_PER_SEC);
		k_delayed_work_submit_to_queue(&application_work_q,
					&cloud_reboot_work,
					K_MSEC(CLOUD_CONNACK_WAIT_DURATION));
	}
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

static void gps_time_set(struct gps_pvt *gps_data)
{
	/* Change datetime.year and datetime.month to accommodate the
	 * correct input format.
	 */
	struct tm gps_time = {
		.tm_year = gps_data->datetime.year - 1900,
		.tm_mon = gps_data->datetime.month - 1,
		.tm_mday = gps_data->datetime.day,
		.tm_hour = gps_data->datetime.hour,
		.tm_min = gps_data->datetime.minute,
		.tm_sec = gps_data->datetime.seconds,
	};

	date_time_set(&gps_time);
}

static void gps_handler(const struct device *dev, struct gps_event *evt)
{
	gps_last_active_time = k_uptime_get();

	switch (evt->type) {
	case GPS_EVT_SEARCH_STARTED:
		LOG_INF("GPS_EVT_SEARCH_STARTED");
		gps_control_set_active(true);
		ui_led_set_pattern(UI_LED_GPS_SEARCHING);
		gps_last_search_start_time = k_uptime_get();
		break;
	case GPS_EVT_SEARCH_STOPPED:
		LOG_INF("GPS_EVT_SEARCH_STOPPED");
		gps_control_set_active(false);
		ui_led_set_pattern(UI_CLOUD_CONNECTED);
		break;
	case GPS_EVT_SEARCH_TIMEOUT:
		LOG_INF("GPS_EVT_SEARCH_TIMEOUT");
		gps_control_set_active(false);
		LOG_INF("GPS will be attempted again in %d seconds",
			gps_control_get_gps_reporting_interval());
		break;
	case GPS_EVT_PVT:
		/* Don't spam logs */
		break;
	case GPS_EVT_PVT_FIX:
		LOG_INF("GPS_EVT_PVT_FIX");
		gps_time_set(&evt->pvt);
		break;
	case GPS_EVT_NMEA:
		/* Don't spam logs */
		break;
	case GPS_EVT_NMEA_FIX:
		LOG_INF("Position fix with NMEA data");

		memcpy(gps_data.buf, evt->nmea.buf, evt->nmea.len);
		gps_data.len = evt->nmea.len;
		gps_cloud_data.data.buf = gps_data.buf;
		gps_cloud_data.data.len = gps_data.len;
		gps_cloud_data.ts = k_uptime_get();
		gps_cloud_data.tag += 1;

		if (gps_cloud_data.tag == 0) {
			gps_cloud_data.tag = 0x1;
		}

		int64_t gps_time_from_start_to_fix_seconds = (k_uptime_get() -
				gps_last_search_start_time) / 1000;
		ui_led_set_pattern(UI_LED_GPS_FIX);
		gps_control_set_active(false);
		LOG_INF("GPS will be started in %lld seconds",
			CONFIG_GPS_CONTROL_FIX_TRY_TIME -
			gps_time_from_start_to_fix_seconds +
			gps_control_get_gps_reporting_interval());

		k_work_submit_to_queue(&application_work_q,
				       &send_gps_data_work);
		env_sensors_poll();
		break;
	case GPS_EVT_OPERATION_BLOCKED:
		LOG_INF("GPS_EVT_OPERATION_BLOCKED");
		ui_led_set_pattern(UI_LED_GPS_BLOCKED);
		break;
	case GPS_EVT_OPERATION_UNBLOCKED:
		LOG_INF("GPS_EVT_OPERATION_UNBLOCKED");
		ui_led_set_pattern(UI_LED_GPS_SEARCHING);
		break;
	case GPS_EVT_AGPS_DATA_NEEDED:
		LOG_INF("GPS_EVT_AGPS_DATA_NEEDED");
		/* Send A-GPS request with short delay to avoid LTE network-
		 * dependent corner-case where the request would not be sent.
		 */
		memcpy(&agps_request, &evt->agps_request, sizeof(agps_request));
		k_delayed_work_submit_to_queue(&application_work_q,
					       &send_agps_request_work,
					       K_SECONDS(1));
		break;
	case GPS_EVT_ERROR:
		LOG_INF("GPS_EVT_ERROR\n");
		break;
	default:
		break;
	}
}

#if defined(CONFIG_USE_UI_MODULE)
/**@brief Send button presses to cloud */
static void button_send(bool pressed)
{
	static char data[] = "1";

	if (!data_send_enabled()) {
		return;
	}

	if (pressed) {
		data[0] = '1';
	} else {
		data[0] = '0';
	}

	button_cloud_data.data.buf = data;
	button_cloud_data.data.len = strlen(data);
	button_cloud_data.ts = k_uptime_get();
	button_cloud_data.tag += 1;

	if (button_cloud_data.tag == 0) {
		button_cloud_data.tag = 0x1;
	}

	k_work_submit_to_queue(&application_work_q, &send_button_data_work);
}
#endif

#if IS_ENABLED(CONFIG_GPS_START_ON_MOTION)
static bool motion_activity_is_active(void)
{
	return (last_activity_state == MOTION_ACTIVITY_ACTIVE);
}

static void motion_trigger_gps(motion_data_t  motion_data)
{
	static bool initial_run = true;

	/* The handler is triggered once on startup, regardless of motion,
	 * in order to get into a known state. This should be ignored.
	 */
	if (initial_run) {
		initial_run = false;
		return;
	}

	if (motion_activity_is_active() && gps_control_is_enabled()) {
		static int64_t next_active_time;
		int64_t last_active_time = gps_last_active_time / 1000;
		int64_t now = k_uptime_get() / 1000;
		int64_t time_since_fix_attempt = now - last_active_time;
		int64_t time_until_next_attempt = next_active_time - now;

		LOG_DBG("Last at %lld s, now %lld s, next %lld s; "
			"%lld secs since last, %lld secs until next",
			last_active_time, now, next_active_time,
			time_since_fix_attempt, time_until_next_attempt);

		if (time_until_next_attempt >= 0) {
			LOG_DBG("keeping original schedule.");
			return;
		}

		time_since_fix_attempt = MAX(0, (MIN(time_since_fix_attempt,
			CONFIG_GPS_CONTROL_FIX_CHECK_INTERVAL)));
		int64_t time_to_start_next_fix = 1 +
			CONFIG_GPS_CONTROL_FIX_CHECK_INTERVAL -
			time_since_fix_attempt;

		next_active_time = now + time_to_start_next_fix;

		char buf[100];

		/* due to a known design issue in Zephyr, we need to use
		 * snprintf to output floats; see:
		 * https://github.com/zephyrproject-rtos/zephyr/issues/18351
		 * https://github.com/zephyrproject-rtos/zephyr/pull/18921
		 */
		snprintf(buf, sizeof(buf),
			"Motion triggering GPS; accel, %.1f, %.1f, %.1f",
			motion_data.acceleration.x,
			motion_data.acceleration.y,
			motion_data.acceleration.z);
		LOG_INF("%s", log_strdup(buf));

		LOG_INF("starting GPS in %lld seconds", time_to_start_next_fix);
		gps_control_start(time_to_start_next_fix * MSEC_PER_SEC);
	}
}
#endif

/**@brief Callback from the motion module. Sends motion data to cloud. */
static void motion_handler(motion_data_t  motion_data)
{

#if IS_ENABLED(CONFIG_GPS_START_ON_MOTION)
	/* toggle state since the accelerometer does not yet report
	 * which state occurred
	 */
	last_activity_state = (last_activity_state != MOTION_ACTIVITY_ACTIVE) ?
			      MOTION_ACTIVITY_ACTIVE : MOTION_ACTIVITY_INACTIVE;
#endif

	if (motion_data.orientation != last_motion_data.orientation) {
		last_motion_data = motion_data;
		k_work_submit_to_queue(&application_work_q,
				       &motion_data_send_work);
	}

#if IS_ENABLED(CONFIG_GPS_START_ON_MOTION)
	motion_trigger_gps(motion_data);
#endif
}

static void motion_data_send(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!flip_mode_enabled || !data_send_enabled() ||
	    gps_control_is_active()) {
		return;
	}

	struct cloud_msg msg = {
		.qos = CLOUD_QOS_AT_MOST_ONCE,
		.endpoint.type = CLOUD_EP_TOPIC_MSG
	};

	int err = 0;

	if (cloud_encode_motion_data(&last_motion_data, &msg) == 0) {
		err = cloud_send(cloud_backend, &msg);
		cloud_release_data(&msg);
		if (err) {
			LOG_ERR("Transmisison of motion data failed: %d", err);
			cloud_error_handler(err);
		}
	}
}

static void cloud_cmd_handle_modem_at_cmd(const char *const at_cmd)
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
		ui_led_set_color(((uint32_t)cmd->data.sv.value >> 16) & 0xFF,
				 ((uint32_t)cmd->data.sv.value >> 8) & 0xFF,
				 ((uint32_t)cmd->data.sv.value) & 0xFF);
	} else if ((cmd->group == CLOUD_CMD_GROUP_CFG_SET) &&
			   (cmd->type == CLOUD_CMD_INTERVAL)) {
		if (cmd->channel == CLOUD_CHANNEL_LIGHT_SENSOR) {
#if defined(CONFIG_LIGHT_SENSOR)
			light_sensor_set_send_interval(
				(uint32_t)cmd->data.sv.value);
#endif
		} else if (cmd->channel == CLOUD_CHANNEL_ENVIRONMENT) {
			env_sensors_set_send_interval(
				(uint32_t)cmd->data.sv.value);
		} else if (cmd->channel == CLOUD_CHANNEL_GPS) {
			/* TODO: update GPS controller to handle send */
			/* interval */
		} else {
			LOG_ERR("Interval command not valid for channel %d",
				cmd->channel);
		}
	} else if ((cmd->group == CLOUD_CMD_GROUP_GET) &&
		   (cmd->type == CLOUD_CMD_EMPTY)) {
		if (cmd->channel == CLOUD_CHANNEL_FLIP) {
			k_work_submit_to_queue(&application_work_q,
					       &motion_data_send_work);
		} else if (cmd->channel == CLOUD_CHANNEL_DEVICE_INFO) {
			k_work_submit_to_queue(&application_work_q,
					       &device_status_work);
		} else if (cmd->channel == CLOUD_CHANNEL_LTE_LINK_RSRP) {
#if CONFIG_MODEM_INFO
			k_delayed_work_submit_to_queue(&application_work_q,
						       &rsrp_work,
						       K_NO_WAIT);
#endif
		} else if (cmd->channel == CLOUD_CHANNEL_ENVIRONMENT) {
			env_sensors_poll();
		} else if (cmd->channel == CLOUD_CHANNEL_LIGHT_SENSOR) {
#if IS_ENABLED(CONFIG_LIGHT_SENSOR)
			light_sensor_poll();
#endif
		}
	}
}

#if CONFIG_MODEM_INFO
/**@brief Callback handler for LTE RSRP data. */
static void modem_rsrp_handler(char rsrp_value)
{
	/* RSRP raw values that represent actual signal strength are
	 * 0 through 97 (per "nRF91 AT Commands" v1.1). If the received value
	 * falls outside this range, we should not send the value.
	 */
	if (rsrp_value > 97) {
		return;
	}

	rsrp.value = rsrp_value;

	/* Only send the RSRP if transmission is not already scheduled.
	 * Checking CONFIG_HOLD_TIME_RSRP gives the compiler a shortcut.
	 */
	if (CONFIG_HOLD_TIME_RSRP == 0 ||
	    k_delayed_work_remaining_get(&rsrp_work) == 0) {
		k_delayed_work_submit_to_queue(&application_work_q, &rsrp_work,
					       K_NO_WAIT);
	}
}

/**@brief Publish RSRP data to the cloud. */
static void modem_rsrp_data_send(struct k_work *work)
{
	char buf[CONFIG_MODEM_INFO_BUFFER_SIZE] = {0};
	static int32_t rsrp_prev; /* RSRP value last sent to cloud */
	int32_t rsrp_current;
	size_t len;

	if (!data_send_enabled()) {
		return;
	}

	/* The RSRP value is copied locally to avoid any race */
	rsrp_current = rsrp.value - rsrp.offset;

	if (rsrp_current == rsrp_prev) {
		return;
	}

	len = snprintf(buf, CONFIG_MODEM_INFO_BUFFER_SIZE,
		       "%d", rsrp_current);

	signal_strength_cloud_data.data.buf = buf;
	signal_strength_cloud_data.data.len = len;
	signal_strength_cloud_data.tag += 1;

	if (signal_strength_cloud_data.tag == 0) {
		signal_strength_cloud_data.tag = 0x1;
	}

	sensor_data_send(&signal_strength_cloud_data);
	rsrp_prev = rsrp_current;

	if (CONFIG_HOLD_TIME_RSRP > 0) {
		k_delayed_work_submit_to_queue(&application_work_q, &rsrp_work,
					      K_SECONDS(CONFIG_HOLD_TIME_RSRP));
	}
}
#endif /* CONFIG_MODEM_INFO */

/**@brief Poll device info and send data to the cloud. */
static void device_status_send(struct k_work *work)
{
	struct modem_param_info *modem_ptr = NULL;
	int ret;

	if (!data_send_enabled() || gps_control_is_active()) {
		return;
	}

#ifdef CONFIG_MODEM_INFO
	ret = modem_info_params_get(&modem_param);
	if (ret < 0) {
		LOG_ERR("Unable to obtain modem parameters: %d", ret);
	} else {
		modem_ptr = &modem_param;
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
#if IS_ENABLED(CONFIG_MODEM_INFO)
		CLOUD_CHANNEL_STR_LTE_LINK_RSRP,
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

	struct cloud_msg msg = {
		.qos = CLOUD_QOS_AT_MOST_ONCE,
		.endpoint.type = CLOUD_EP_TOPIC_STATE
	};

	ret = cloud_encode_device_status_data(modem_ptr,
					      ui, ARRAY_SIZE(ui),
					      fota, ARRAY_SIZE(fota),
					      SERVICE_INFO_FOTA_VER_CURRENT,
					      &msg);
	if (ret) {
		LOG_ERR("Unable to encode cloud data: %d", ret);
	} else {
		/* Transmits the data to the cloud. */
		ret = cloud_send(cloud_backend, &msg);
		cloud_release_data(&msg);
		if (ret) {
			LOG_ERR("sensor_data_send failed: %d", ret);
			cloud_error_handler(ret);
		}
	}
}

/**@brief Send device config to the cloud. */
static void device_config_send(struct k_work *work)
{
	int ret;
	struct cloud_msg msg = {
		.qos = CLOUD_QOS_AT_MOST_ONCE,
		.endpoint.type = CLOUD_EP_TOPIC_STATE
	};
	enum cloud_cmd_state gps_cfg_state =
		cloud_get_channel_enable_state(CLOUD_CHANNEL_GPS);

	if (gps_cfg_state == CLOUD_CMD_STATE_UNDEFINED) {
		return;
	}

	if (gps_control_is_active() && gps_cfg_state == CLOUD_CMD_STATE_FALSE) {
		/* GPS hasn't been stopped yet, reschedule this work */
		k_delayed_work_submit_to_queue(&application_work_q,
			&device_config_work, K_SECONDS(5));
		return;
	}

	ret = cloud_encode_config_data(&msg);
	if (ret) {
		LOG_ERR("Unable to encode cloud data: %d", ret);
	} else if (msg.len && msg.buf) {
		ret = cloud_send(cloud_backend, &msg);
		cloud_release_data(&msg);

		if (ret) {
			LOG_ERR("%s failed: %d", __func__, ret);
			cloud_error_handler(ret);
		}
	}

	if (gps_cfg_state == CLOUD_CMD_STATE_TRUE) {
		gps_control_start(0);
	}
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

	if (!data_send_enabled()) {
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

	if (!data_send_enabled() || gps_control_is_active()) {
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

	if (!data_send_enabled() || gps_control_is_active()) {
		return;
	}

	err = cloud_encode_data(data, CLOUD_CMD_GROUP_DATA, &msg);
	if (err) {
		LOG_ERR("Unable to encode cloud data: %d", err);
	} else {
		err = cloud_send(cloud_backend, &msg);
		cloud_release_data(&msg);
		if (err) {
			LOG_ERR("%s failed, data was not sent: %d", __func__,
			err);
		}
	}
}

/**@brief Reboot the device if CONNACK has not arrived. */
static void cloud_reboot_handler(struct k_work *work)
{
	error_handler(ERROR_CLOUD, -ETIMEDOUT);
}

static bool data_send_enabled(void)
{
	return (atomic_get(&cloud_association) ==
		   CLOUD_ASSOCIATION_STATE_READY);
}

/**@brief Callback for sensor attached event from nRF Cloud. */
void sensors_start(void)
{
	static bool started;
	bool start_gps = IS_ENABLED(CONFIG_GPS_START_AFTER_CLOUD_EVT_READY);

	if (!started) {
		sensors_init();

		/* Value from cloud has precedence */
		switch (cloud_get_channel_enable_state(CLOUD_CHANNEL_GPS)) {
		case CLOUD_CMD_STATE_FALSE:
			start_gps = false;
			break;
		case CLOUD_CMD_STATE_TRUE:
			start_gps = true;
			break;
		case CLOUD_CMD_STATE_UNDEFINED:
		default:
			break;
		}
		set_gps_enable(start_gps);
		started = true;
	}
}

static void sensors_start_work_fn(struct k_work *work)
{
	sensors_start();
}

/**@brief nRF Cloud specific callback for cloud association event. */
static void on_user_pairing_req(const struct cloud_event *evt)
{
	if (atomic_get(&cloud_association) !=
		CLOUD_ASSOCIATION_STATE_REQUESTED) {
		atomic_set(&cloud_association,
				   CLOUD_ASSOCIATION_STATE_REQUESTED);
		ui_led_set_pattern(UI_CLOUD_PAIRING);
		LOG_INF("Add device to cloud account.");
		LOG_INF("Waiting for cloud association...");

		/* If the association is not done soon enough (< ~5 min?)
		 * a connection cycle is needed... TBD why.
		 */
		k_delayed_work_submit_to_queue(&application_work_q,
					       &cycle_cloud_connection_work,
					       CONN_CYCLE_AFTER_ASSOCIATION_REQ_MS);
	}
}

static void cycle_cloud_connection(struct k_work *work)
{
	int32_t reboot_wait_ms = REBOOT_AFTER_DISCONNECT_WAIT_MS;

	LOG_INF("Disconnecting from cloud...");

	if (cloud_disconnect(cloud_backend) != 0) {
		reboot_wait_ms = 5 * MSEC_PER_SEC;
		LOG_INF("Disconnect failed. Device will reboot in %d seconds",
			(reboot_wait_ms / MSEC_PER_SEC));
	}

	/* Reboot fail-safe on disconnect */
	k_delayed_work_submit_to_queue(&application_work_q, &cloud_reboot_work,
				       K_MSEC(reboot_wait_ms));
}

/** @brief Handle procedures after successful association with nRF Cloud. */
void on_pairing_done(void)
{
	if (atomic_get(&cloud_association) ==
			CLOUD_ASSOCIATION_STATE_REQUESTED) {
		k_delayed_work_cancel(&cycle_cloud_connection_work);

		/* After successful association, the device must
		 * reconnect to the cloud.
		 */
		LOG_INF("Device associated with cloud.");
		LOG_INF("Reconnecting for cloud policy to take effect.");
		atomic_set(&cloud_association,
				   CLOUD_ASSOCIATION_STATE_RECONNECT);
		k_delayed_work_submit_to_queue(&application_work_q,
					       &cycle_cloud_connection_work,
					       K_NO_WAIT);
	} else {
		atomic_set(&cloud_association, CLOUD_ASSOCIATION_STATE_PAIRED);
	}
}

void cloud_event_handler(const struct cloud_backend *const backend,
			 const struct cloud_event *const evt,
			 void *user_data)
{
	ARG_UNUSED(user_data);

	switch (evt->type) {
	case CLOUD_EVT_CONNECTED:
	case CLOUD_EVT_CONNECTING:
	case CLOUD_EVT_DISCONNECTED:
		connection_evt_handler(evt);
		break;
	case CLOUD_EVT_READY:
		LOG_INF("CLOUD_EVT_READY");
		ui_led_set_pattern(UI_CLOUD_CONNECTED);

#if defined(CONFIG_BOOTLOADER_MCUBOOT)
		/* Mark image as good to avoid rolling back after update */
		boot_write_img_confirmed();
#endif
		atomic_set(&cloud_association, CLOUD_ASSOCIATION_STATE_READY);
		k_work_submit_to_queue(&application_work_q, &sensors_start_work);
		break;
	case CLOUD_EVT_ERROR:
		LOG_INF("CLOUD_EVT_ERROR");
		break;
	case CLOUD_EVT_DATA_SENT:
		LOG_INF("CLOUD_EVT_DATA_SENT");
		break;
	case CLOUD_EVT_DATA_RECEIVED: {
		int err;

		LOG_INF("CLOUD_EVT_DATA_RECEIVED");
		err = cloud_decode_command(evt->data.msg.buf);
		if (err == 0) {
			/* Cloud decoder has handled the data */
			return;
		}

#if defined(CONFIG_AGPS)
		err = gps_process_agps_data(evt->data.msg.buf,
					    evt->data.msg.len);
		if (err) {
			LOG_WRN("Data was not valid A-GPS data, err: %d", err);
			break;
		}

		LOG_INF("A-GPS data processed");
#endif /* defined(CONFIG_AGPS) */
		break;
	}
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
#if defined(CONFIG_LTE_LINK_CONTROL)
		lte_lc_power_off();
#endif
		sys_reboot(SYS_REBOOT_COLD);
		break;
	default:
		LOG_WRN("Unknown cloud event type: %d", evt->type);
		break;
	}
}

void connection_evt_handler(const struct cloud_event *const evt)
{
	if (evt->type == CLOUD_EVT_CONNECTING) {
		LOG_INF("CLOUD_EVT_CONNECTING");
		ui_led_set_pattern(UI_CLOUD_CONNECTING);
		k_delayed_work_cancel(&cloud_reboot_work);

		if (evt->data.err != CLOUD_CONNECT_RES_SUCCESS) {
			cloud_connect_error_handler(evt->data.err);
		}
		return;
	} else if (evt->type == CLOUD_EVT_CONNECTED) {
		LOG_INF("CLOUD_EVT_CONNECTED");
		k_delayed_work_cancel(&cloud_reboot_work);
		k_sem_take(&cloud_disconnected, K_NO_WAIT);
		atomic_set(&cloud_connect_attempts, 0);
#if !IS_ENABLED(CONFIG_MQTT_CLEAN_SESSION)
		LOG_INF("Persistent Sessions = %u",
			evt->data.persistent_session);
#endif
	} else if (evt->type == CLOUD_EVT_DISCONNECTED) {
		int32_t connect_wait_s = CONFIG_CLOUD_CONNECT_RETRY_DELAY;

		LOG_INF("CLOUD_EVT_DISCONNECTED: %d", evt->data.err);
		ui_led_set_pattern(UI_LTE_CONNECTED);

		switch (evt->data.err) {
		case CLOUD_DISCONNECT_INVALID_REQUEST:
			LOG_INF("Cloud connection closed.");
			if ((atomic_get(&cloud_connect_attempts) == 1) &&
			    (atomic_get(&cloud_association) ==
			     CLOUD_ASSOCIATION_STATE_INIT)) {
				LOG_INF("This can occur during initial nRF Cloud provisioning.");
#if defined(CONFIG_LWM2M_CARRIER)
#if !defined(CONFIG_DEBUG) && defined(CONFIG_REBOOT)
				/* Reconnect does not work with carrier library */
				LOG_INF("Rebooting in 10 seconds...");
				k_sleep(K_SECONDS(10));
#endif
				error_handler(ERROR_CLOUD, -EIO);
				break;
#endif
				connect_wait_s = 10;
			} else {
				LOG_INF("This can occur if the device has the wrong nRF Cloud certificates.");
			}
			break;
		case CLOUD_DISCONNECT_USER_REQUEST:
			if (atomic_get(&cloud_association) ==
			    CLOUD_ASSOCIATION_STATE_RECONNECT ||
			    atomic_get(&cloud_association) ==
			    CLOUD_ASSOCIATION_STATE_REQUESTED ||
			    (atomic_get(&carrier_requested_disconnect))) {
				connect_wait_s = 10;
			}
			break;
		case CLOUD_DISCONNECT_CLOSED_BY_REMOTE:
			LOG_INF("Disconnected by the cloud.");
			break;
		case CLOUD_DISCONNECT_MISC:
		default:
			break;
		}
		k_sem_give(&cloud_disconnected);
		connect_to_cloud(connect_wait_s);
	}
}

static void set_gps_enable(const bool enable)
{
	int32_t delay_ms = 0;
	bool changing = (enable != gps_control_is_enabled());

	/* Exit early if the link is not ready or if the cloud
	 * state is defined and the local state is not changing.
	 */
	if (!data_send_enabled() ||
	    ((cloud_get_channel_enable_state(CLOUD_CHANNEL_GPS) !=
	    CLOUD_CMD_STATE_UNDEFINED) && !changing)) {
		return;
	}

	cloud_set_channel_enable_state(CLOUD_CHANNEL_GPS,
		enable ? CLOUD_CMD_STATE_TRUE : CLOUD_CMD_STATE_FALSE);

	if (changing) {
		if (enable) {
			LOG_INF("Starting GPS");
			/* GPS will be started from the device config work
			 * handler AFTER the config has been sent to the cloud
			 */
		} else {
			LOG_INF("Stopping GPS");
			gps_control_stop(0);
			/* Allow time for the gps to be stopped before
			 * attemping to send the config update
			 */
			delay_ms = 5 * MSEC_PER_SEC;
		}
	}

	/* Update config state in cloud */
	k_delayed_work_submit_to_queue(&application_work_q,
			&device_config_work, K_MSEC(delay_ms));
}

static void long_press_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!data_send_enabled()) {
		LOG_INF("Link not ready, long press disregarded");
		return;
	}

	set_gps_enable(!gps_control_is_enabled());
}

/**@brief Initializes and submits delayed work. */
static void work_init(void)
{
	k_work_init(&sensors_start_work, sensors_start_work_fn);
	k_work_init(&send_gps_data_work, send_gps_data_work_fn);
	k_work_init(&send_button_data_work, send_button_data_work_fn);
	k_work_init(&send_modem_at_cmd_work, send_modem_at_cmd_work_fn);
	k_delayed_work_init(&send_agps_request_work, send_agps_request);
	k_delayed_work_init(&long_press_button_work, long_press_handler);
	k_delayed_work_init(&cloud_reboot_work, cloud_reboot_handler);
	k_delayed_work_init(&cycle_cloud_connection_work,
			    cycle_cloud_connection);
	k_delayed_work_init(&device_config_work, device_config_send);
	k_delayed_work_init(&cloud_connect_work, cloud_connect_work_fn);
	k_work_init(&device_status_work, device_status_send);
	k_work_init(&motion_data_send_work, motion_data_send);
	k_work_init(&no_sim_go_offline_work, no_sim_go_offline);
#if CONFIG_MODEM_INFO
	k_delayed_work_init(&rsrp_work, modem_rsrp_data_send);
#endif /* CONFIG_MODEM_INFO */
}

static void cloud_api_init(void)
{
	int ret;

	cloud_backend = cloud_get_binding("NRF_CLOUD");
	__ASSERT(cloud_backend != NULL, "nRF Cloud backend not found");

	ret = cloud_init(cloud_backend, cloud_event_handler);
	if (ret) {
		LOG_ERR("Cloud backend could not be initialized, error: %d",
			ret);
		cloud_error_handler(ret);
	}

	ret = cloud_decode_init(cloud_cmd_handler);
	if (ret) {
		LOG_ERR("Cloud command decoder could not be initialized, error: %d",
			ret);
		cloud_error_handler(ret);
	}
}

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static int modem_configure(void)
{
#if defined(CONFIG_BSD_LIBRARY)
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already turned on */
		/* and connected */
		goto connected;
	}

	ui_led_set_pattern(UI_LTE_CONNECTING);
	LOG_INF("Connecting to LTE network.");
	LOG_INF("This may take several minutes.");

#if defined(CONFIG_LWM2M_CARRIER)
	/* Wait for the LWM2M carrier library to configure the */
	/* modem and set up the LTE connection. */
	k_sem_take(&lte_connected, K_FOREVER);
#else /* defined(CONFIG_LWM2M_CARRIER) */
	int err = lte_lc_init_and_connect();
	if (err) {
		LOG_ERR("LTE link could not be established.");
		return err;
	}
#endif /* defined(CONFIG_LWM2M_CARRIER) */

connected:
	LOG_INF("Connected to LTE network.");
	ui_led_set_pattern(UI_LTE_CONNECTED);

#endif /* defined(CONFIG_BSD_LIBRARY) */
	return 0;
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

	err = motion_init_and_start(&application_work_q, motion_handler);
	if (err) {
		LOG_ERR("motion module init failed, error: %d", err);
	}

	err = env_sensors_init_and_start(&application_work_q, env_data_send);
	if (err) {
		LOG_ERR("Environmental sensors init failed, error: %d", err);
	}
#if CONFIG_LIGHT_SENSOR
	err = light_sensor_init_and_start(&application_work_q,
					  light_sensor_data_send);
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

	err = gps_control_init(&application_work_q, gps_handler);
	if (err) {
		LOG_ERR("GPS could not be initialized");
		return;
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

	if (IS_ENABLED(CONFIG_ACCEL_USE_SIM) && (evt.button == FLIP_INPUT) &&
	    data_send_enabled()) {
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

static void no_sim_go_offline(struct k_work *work)
{
#if defined(CONFIG_BSD_LIBRARY)
	lte_lc_offline();
	/* Wait for lte_lc events to be processed before printing info message */
	k_sleep(K_MSEC(100));
	LOG_INF("No SIM card detected.");
	LOG_INF("Insert SIM and reset device to run the asset tracker.");
	ui_led_set_pattern(UI_LED_ERROR_LTE_LC);
#endif /* CONFIG_BSD_LIBRARY */
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
#if defined(CONFIG_BSD_LIBRARY)
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:

		if (evt->nw_reg_status == LTE_LC_NW_REG_UICC_FAIL) {
			k_work_submit_to_queue(&application_work_q,
				       &no_sim_go_offline_work);

		} else if ((evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
			(evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			LOG_INF("Network registration status: %s",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");
		}
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		LOG_INF("PSM parameter update: TAU: %d, Active time: %d",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintf(log_buf, sizeof(log_buf),
					"eDRX parameter update: eDRX: %0.2f, PTW: %0.2f",
					evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if ((len > 0) && (len < sizeof(log_buf))) {
			LOG_INF("%s", log_strdup(log_buf));
		}
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_INF("RRC mode: %s",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		LOG_INF("LTE cell changed: Cell ID: %d, Tracking area: %d",
			evt->cell.id, evt->cell.tac);
		break;
	default:
		break;
	}
#endif /* CONFIG_BSD_LIBRARY */
}

static void date_time_event_handler(const struct date_time_evt *evt)
{
	switch (evt->type) {
	case DATE_TIME_OBTAINED_MODEM:
		LOG_INF("DATE_TIME_OBTAINED_MODEM");
		break;
	case DATE_TIME_OBTAINED_NTP:
		LOG_INF("DATE_TIME_OBTAINED_NTP");
		break;
	case DATE_TIME_OBTAINED_EXT:
		LOG_INF("DATE_TIME_OBTAINED_EXT");
		break;
	case DATE_TIME_NOT_OBTAINED:
		LOG_INF("DATE_TIME_NOT_OBTAINED");
		break;
	default:
		break;
	}

	/* Do not depend on obtained time, continue upon any event from the
	 * date time library.
	 */
	k_sem_give(&date_time_obtained);
}

void main(void)
{
	int ret;

	LOG_INF("Asset tracker started");
	k_work_q_start(&application_work_q, application_stack_area,
		       K_THREAD_STACK_SIZEOF(application_stack_area),
		       CONFIG_APPLICATION_WORKQUEUE_PRIORITY);
	if (IS_ENABLED(CONFIG_WATCHDOG)) {
		watchdog_init_and_start(&application_work_q);
	}

#if defined(CONFIG_LWM2M_CARRIER)
	k_sem_take(&bsdlib_initialized, K_FOREVER);
#else
	handle_bsdlib_init_ret();
#endif

	cloud_api_init();

#if defined(CONFIG_USE_UI_MODULE)
	ui_init(ui_evt_handler);
#endif
	work_init();
#if defined(CONFIG_BSD_LIBRARY)
	lte_lc_register_handler(lte_handler);
#endif /* CONFIG_BSD_LIBRARY */
	while (modem_configure() != 0) {
		LOG_WRN("Failed to establish LTE connection.");
		LOG_WRN("Will retry in %d seconds.",
				CONFIG_CLOUD_CONNECT_RETRY_DELAY);
		k_sleep(K_SECONDS(CONFIG_CLOUD_CONNECT_RETRY_DELAY));
	}

#if defined(CONFIG_LWM2M_CARRIER)
	LOG_INF("Waiting for LWM2M carrier to complete initialization...");
	k_sem_take(&cloud_ready_to_connect, K_FOREVER);
#endif

	date_time_update_async(date_time_event_handler);

	ret = k_sem_take(&date_time_obtained, K_SECONDS(DATE_TIME_TIMEOUT_S));
	if (ret) {
		LOG_WRN("Date time, no callback event within %d seconds",
			DATE_TIME_TIMEOUT_S);
	}

	connect_to_cloud(0);
}
