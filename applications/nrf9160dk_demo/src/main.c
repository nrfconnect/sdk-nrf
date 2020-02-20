/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <kernel_structs.h>
#include <stdio.h>
#include <string.h>
#include <console/console.h>
#include <power/reboot.h>
#include <logging/log_ctrl.h>
#include <modem/bsdlib.h>
#include <bsd.h>
#include <modem/lte_lc.h>
#include <net/cloud.h>
#include <net/socket.h>
#include <net/nrf_cloud.h>

#if defined(CONFIG_BOOTLOADER_MCUBOOT)
#include <dfu/mcuboot.h>
#endif

#include "cloud_codec.h"
#include "ui.h"
#include "service_info.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(nrf9160dk_demo, CONFIG_NRF9160DK_DEMO_LOG_LEVEL);

/* How long the device should wait for a CONNACK event from the cloud
 * when connecting.  Device will reboot if an ack is not received
 * within the duration.
 */
#define CLOUD_CONNACK_WAIT_DURATION	K_SECONDS(CONFIG_CLOUD_WAIT_DURATION)

/* Interval in milliseconds after which the device will reboot
 * if the disconnect event has not been handled.
 */
#define REBOOT_AFTER_DISCONNECT_WAIT_MS	(15000)

/* Interval in milliseconds after a disconnect failure before rebooting. */
#define REBOOT_AFTER_DISCONNECT_FAILED_WAIT_MS	(15000)

/* Interval in milliseconds after which the device will
 * disconnect and reconnect if association was not completed.
 */
#define CONN_CYCLE_AFTER_ASSOCIATION_REQ_MS	K_MINUTES(5)

/* Stack definition for application workqueue */
K_THREAD_STACK_DEFINE(application_stack_area,
		      CONFIG_APPLICATION_WORKQUEUE_STACK_SIZE);

/* Define variable to hold work queue kernel queue and thread data. */
static struct k_work_q application_work_q;

/* Define variable to hold pointer to current backend for cloud operations. */
static struct cloud_backend *cloud_backend;

/* Define variable to hold outgoing button press data. */
static struct cloud_channel_data button_cloud_data;

/* Define variable to hold outgoing human-readable message data. */
static struct cloud_channel_data msg_cloud_data = {
	.type = CLOUD_CHANNEL_MSG,
	.tag = 0x1
};

/* Variable to keep track of nRF cloud user association request. */
static atomic_t carrier_requested_disconnect;
static atomic_t cloud_connect_attempts;

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
static struct k_work send_button_data_work;
static struct k_work send_msg_data_work;
static struct k_delayed_work cloud_reboot_work;
static struct k_delayed_work cycle_cloud_connection_work;
static struct k_delayed_work cloud_connect_work;
static struct k_work device_status_work;

static K_SEM_DEFINE(cloud_disconnected, 0, 1);
enum error_type {
	ERROR_CLOUD,
	ERROR_BSD_RECOVERABLE,
	ERROR_LTE_LC,
	ERROR_SYSTEM_FAULT
};

/* Forward declaration of functions */
static void work_init(void);
static void sensor_data_send(struct cloud_channel_data *data);
static void device_status_send(struct k_work *work);
static void cycle_cloud_connection(struct k_work *work);
static void send_signon_message(void);
static bool data_send_enabled(void);
static void connection_evt_handler(const struct cloud_event *const evt);

static void shutdown_modem(void)
{
	/* Turn off and shutdown modem */
	LOG_ERR("LTE link disconnect");
	int err = lte_lc_power_off();

	if (err) {
		LOG_ERR("lte_lc_power_off failed: %d", err);
	}
	LOG_ERR("Shutdown modem");
	bsdlib_shutdown();
}

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

/**@brief Reboot the device if CONNACK has not arrived. */
static void cloud_reboot_handler(struct k_work *wbuilork)
{
	error_handler(ERROR_CLOUD, -ETIMEDOUT);
}

/**@brief Send button presses to cloud */
static void button_send(u8_t button_num, bool pressed)
{
	static char data[3];

	if (!data_send_enabled()) {
		return;
	}

	data[0] = button_num + '0';
	if (pressed) {
		data[1] = '1';
	} else {
		data[1] = '0';
	}
	data[2] = '\0';

	button_cloud_data.data.buf = data;
	button_cloud_data.data.len = strlen(data);
	button_cloud_data.tag += 1;

	if (button_cloud_data.tag == 0) {
		button_cloud_data.tag = 0x1;
	}

	LOG_INF("Sending button event for button %d=%s", button_num,
		pressed ? "pressed" : "released");

	/* Submit this to the work queue, so we can return to the caller
	 * quickly.  A separate thread will process this using the callback
	 * @fn send_button_data_work_fn associated with send_button_data_work.
	 */
	k_work_submit_to_queue(&application_work_q, &send_button_data_work);

	if ((button_num == 4) && pressed) {
		send_signon_message();
	}
}

/**@brief Called by the application work queue, this function will actually
 * actually the button press data to the cloud.
 */
static void send_button_data_work_fn(struct k_work *work)
{
	sensor_data_send(&button_cloud_data);
}

/**@brief Send a text message to the cloud; work function
 * will need to free the duplicated string; duplication is done
 * in case message is in a temporary buffer
 */
static void msg_send(const char *message)
{
	if (!data_send_enabled()) {
		return;
	}

	msg_cloud_data.data.buf = strdup(message);
	msg_cloud_data.data.len = strlen(message);
	msg_cloud_data.tag += 1;

	if (msg_cloud_data.tag == 0) {
		msg_cloud_data.tag = 0x1;
	}

	LOG_INF("Sending message: %s", message);
	k_work_submit_to_queue(&application_work_q, &send_msg_data_work);
}

static void send_msg_data_work_fn(struct k_work *work)
{
	sensor_data_send(&msg_cloud_data);
}

/** @brief Send helpful text to 9160DK user in nRFCloud
 *         Terminal card.
 */
static void send_signon_message(void)
{
	msg_send(
	     "**Welcome to the Nordic Semiconductor nrf9160 Development Kit**\n"
	     "  1. Change buttons or switches to receive messages below.\n"
	     "  2. Type commands in the Send a message box:\n"
	     "       Use the state array [LED1,LED2] to turn on(1) or off(0).\n"
	     "       For example, to set LED1 on and LED2 off:\n"
	     "          {\"appId\":\"LED\",\n"
	     "           \"messageType\":\"CFG_SET\",\n"
	     "           \"data\":{\"state\":[1,0]}}\n"
	     "  3. Use the FOTA update service to try out other examples.\n"
	     "  Getting started can be found here: https://bit.ly/37NMvuo");
}


/**@brief Summarize device info and send data to the cloud. */
static void device_status_send(struct k_work *work)
{
	int ret;

	if (!data_send_enabled()) {
		return;
	}

	const char *const ui[] = {
		CLOUD_CHANNEL_STR_BUTTON,
		CLOUD_CHANNEL_STR_MSG,
	};

	const char *const fota[] = {
		SERVICE_INFO_FOTA_STR_APP,
		SERVICE_INFO_FOTA_STR_MODEM
	};

	struct cloud_msg msg = {
		.qos = CLOUD_QOS_AT_MOST_ONCE,
		.endpoint.type = CLOUD_EP_TOPIC_STATE
	};

	ret = cloud_encode_device_status_data(NULL,
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

/**@brief Send sensor data to nRF Cloud. */
static void sensor_data_send(struct cloud_channel_data *data)
{
	int err = 0;
	struct cloud_msg msg = {
			.qos = CLOUD_QOS_AT_MOST_ONCE,
			.endpoint.type = CLOUD_EP_TOPIC_MSG
		};

	if (!data_send_enabled()) {
		return;
	}

	err = cloud_encode_data(data, CLOUD_CMD_GROUP_DATA, &msg);
	if (err) {
		LOG_ERR("Unable to encode cloud data: %d", err);
	} else {
		err = cloud_send(cloud_backend, &msg);
		cloud_release_data(&msg);
		if (err) {
			LOG_ERR("%s failed: %d", __func__, err);
			cloud_error_handler(err);
		}
	}
}

/**@brief Initializes the only sensor used by the application. */
static void button_sensor_init(void)
{
	button_cloud_data.type = CLOUD_CHANNEL_BUTTON;
	button_cloud_data.tag = 0x1;
}

/**@brief User interface event handler. */
static void ui_evt_handler(struct ui_evt evt)
{
	/* Send information to cloud about which button generated the event,
	 * as well as whether it was activated or deactivated.
	 */
	button_send(evt.button, evt.type == UI_EVT_BUTTON_ACTIVE ? 1 : 0);
}

static bool data_send_enabled(void)
{
	return (atomic_get(&cloud_association) ==
		   CLOUD_ASSOCIATION_STATE_READY);
}

static void connect_to_cloud(const s32_t connect_delay_s)
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
			CLOUD_CONNACK_WAIT_DURATION);

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
		       CONFIG_CLOUD_WAIT_DURATION);
		k_delayed_work_submit_to_queue(&application_work_q,
					       &cloud_reboot_work,
					       CLOUD_CONNACK_WAIT_DURATION);
	}
}

static void cloud_cmd_handler(struct cloud_command *cmd)
{
	if ((cmd->channel == CLOUD_CHANNEL_LED) &&
		   (cmd->group == CLOUD_CMD_GROUP_CFG_SET) &&
		   (cmd->type == CLOUD_CMD_STATE)) {
		u8_t led1 = (((u32_t)cmd->data.sv.value) & 0x00FF) != 0;
		u8_t led2 = (((u32_t)cmd->data.sv.value) & 0xFF00) != 0;

		LOG_INF("Received LED STATE cmd from cloud: [%u, %u]",
			led1, led2);
		ui_led_set_state(UI_LED_1, led1);
		ui_led_set_state(UI_LED_2, led2);
	} else if ((cmd->channel == CLOUD_CHANNEL_DEVICE_INFO) &&
		   (cmd->group == CLOUD_CMD_GROUP_GET) &&
		   (cmd->type == CLOUD_CMD_EMPTY)) {
		k_work_submit_to_queue(&application_work_q,
				       &device_status_work);
	} else if ((cmd->group == CLOUD_CMD_GROUP_CFG_SET) &&
			   (cmd->type == CLOUD_CMD_INTERVAL)) {
		LOG_ERR("Interval command not valid for channel %d",
			cmd->channel);
	} else {
		LOG_ERR("bad command: channel %d:%s, group %d:%s, type %d:%s",
			cmd->channel, cloud_get_channel_name(cmd->channel),
			cmd->group, cloud_get_group_name(cmd->group),
			cmd->type, cloud_get_type_name(cmd->type));
	}
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
	s32_t reboot_wait_ms = REBOOT_AFTER_DISCONNECT_WAIT_MS;

	LOG_INF("Disconnecting from cloud...");

	if (cloud_disconnect(cloud_backend) != 0) {
		reboot_wait_ms = REBOOT_AFTER_DISCONNECT_FAILED_WAIT_MS;
		LOG_INF("Disconnect failed. Device will reboot in %d seconds",
			(reboot_wait_ms / MSEC_PER_SEC));
	}

	/* Reboot fail-safe on disconnect */
	k_delayed_work_submit_to_queue(&application_work_q, &cloud_reboot_work,
				       K_MSEC(reboot_wait_ms));
}

/** @brief Handle procedures after successful association with nRF Cloud. */
static void on_pairing_done(void)
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
		/* Report device information to cloud. */
		k_work_submit_to_queue(&application_work_q,
			&device_status_work);
		/* Set up the sensor. */
		button_sensor_init();
		/* Send help text to cloud. */
		send_signon_message();
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

		if (err == -ENOTSUP) {
			LOG_ERR("Unsupported command");
		}

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
		lte_lc_power_off();
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
		ui_led_set_pattern(UI_CLOUD_CONNECTED);
		LOG_INF("Persistent Sessions = %u",
			evt->data.persistent_session);
	} else if (evt->type == CLOUD_EVT_DISCONNECTED) {
		s32_t connect_wait_s = CONFIG_CLOUD_CONNECT_RETRY_DELAY;

		LOG_INF("CLOUD_EVT_DISCONNECTED: %d", evt->data.err);
		ui_led_set_pattern(UI_LTE_CONNECTED);

		switch (evt->data.err) {
		case CLOUD_DISCONNECT_INVALID_REQUEST:
			LOG_INF("Cloud connection closed.");
			if ((atomic_get(&cloud_connect_attempts) == 1) &&
			    (atomic_get(&cloud_association) ==
			     CLOUD_ASSOCIATION_STATE_INIT)) {
				LOG_INF("This can occur during initial "
					"nRF Cloud provisioning.");

				connect_wait_s = 10;
			} else {
				LOG_INF("This can occur if the device has the "
					"wrong nRF Cloud certificates.");
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

/**@brief Initializes and submits delayed work. */
static void work_init(void)
{
	k_work_init(&send_button_data_work, send_button_data_work_fn);
	k_work_init(&send_msg_data_work, send_msg_data_work_fn);
	k_delayed_work_init(&cloud_reboot_work, cloud_reboot_handler);
	k_delayed_work_init(&cycle_cloud_connection_work,
			    cycle_cloud_connection);
	k_delayed_work_init(&cloud_connect_work, cloud_connect_work_fn);
	k_work_init(&device_status_work, device_status_send);
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
		LOG_ERR("Cloud command decoder could not be initialized, "
			"error: %d", ret);
		cloud_error_handler(ret);
	}
}

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static int modem_configure(void)
{
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already turned on */
		/* and connected */
		goto connected;
	}

	ui_led_set_pattern(UI_LTE_CONNECTING);
	LOG_INF("Connecting to LTE network.");
	LOG_INF("This may take several minutes.");

	int err = lte_lc_init_and_connect();
	if (err) {
		LOG_ERR("LTE link could not be established.");
		return err;
	}

connected:
	LOG_INF("Connected to LTE network.");
	ui_led_set_pattern(UI_LTE_CONNECTED);

	return 0;
}

void handle_bsdlib_init_ret(void)
{
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
}

void main(void)
{
	LOG_INF("nRF9160DK Demo Started");

	/* Start a work queue, which is a thread plus a queue, to take care
	 * of low priority tasks. This allows high priority things like
	 * interrupt handlers to finish quickly, while low priority things
	 * will be done as soon as possible.
	 */
	k_work_q_start(&application_work_q, application_stack_area,
		       K_THREAD_STACK_SIZEOF(application_stack_area),
		       CONFIG_APPLICATION_WORKQUEUE_PRIORITY);

	/* bsdlib provides a vital connection between applications running
	 * on the application core and the modem running on the modem core.
	 * The modem provides network facilities for communicating with the
	 * cloud; bsdlib provides a convenient interface to that. Here we
	 * check that initialization of bsdlib at system boot time was
	 * successful.
	 */
	handle_bsdlib_init_ret();

	/* Prepare communications with nRFCloud servers. */
	cloud_api_init();

	/* Set up the 9160DK board's user interface (LEDs, buttons, switches) */
	ui_init(ui_evt_handler);

	/* Set up structures for scheduling various work items in the
	 * application work queue.
	 */
	work_init();

	/* Set up the modem. */
	while (modem_configure() != 0) {
		LOG_WRN("Failed to establish LTE connection.");
		LOG_WRN("Will retry in %d seconds.",
				CONFIG_CLOUD_CONNECT_RETRY_DELAY);
		k_sleep(K_SECONDS(CONFIG_CLOUD_CONNECT_RETRY_DELAY));
	}

	/* Connect to cloud, then use another thread (connection_poll_thread)
	 * to process events automatically.  Event processing is in
	 * subsys/net/nrf_cloud/nrf_cloud.c: nrf_cloud_run().
	 */
	connect_to_cloud(0);
}
