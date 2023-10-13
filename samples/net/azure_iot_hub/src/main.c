/*
 * Copyright (c) 2020-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */


#include <stdio.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/dfu/mcuboot.h>
#include <net/azure_iot_hub.h>
#include <net/azure_iot_hub_dps.h>
#include <zephyr/logging/log.h>

#include <dk_buttons_and_leds.h>
#include <cJSON.h>
#include <cJSON_os.h>

#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_SAMPLE_DEVICE_ID_USE_HW_ID)
#include <hw_id.h>
#endif

LOG_MODULE_REGISTER(azure_iot_hub_sample, CONFIG_AZURE_IOT_HUB_SAMPLE_LOG_LEVEL);

/* Macros used to subscribe to specific Zephyr net management events. */
#define L4_EVENT_MASK (NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED)
#define CONN_LAYER_EVENT_MASK (NET_EVENT_CONN_IF_FATAL_ERROR)

/* Interval [s] between sending events to the IoT hub. The value can be changed
 * by setting a new desired value for property 'telemetryInterval' in the
 * device twin document.
 */
#define EVENT_INTERVAL		20
#define RECV_BUF_SIZE		1024
#define APP_WORK_Q_STACK_SIZE	KB(8)

/* Zephyr net management event callback structures. */
static struct net_mgmt_event_callback l4_cb;
static struct net_mgmt_event_callback conn_cb;

static struct method_data {
	struct k_work work;
	char request_id[8];
	char name[32];
	char payload[200];
} method_data;
static struct k_work twin_report_work;
static struct k_work_delayable send_event_work;
static struct k_work_delayable reboot_work;

static char recv_buf[RECV_BUF_SIZE];
static void direct_method_handler(struct k_work *work);
static K_SEM_DEFINE(network_connected_sem, 0, 1);
static K_SEM_DEFINE(recv_buf_sem, 1, 1);
static atomic_t event_interval = EVENT_INTERVAL;

#ifdef CONFIG_AZURE_IOT_HUB_DPS
static bool dps_was_successful;
#endif

/* A work queue is created to execute potentially blocking calls from.
 * This is done to avoid blocking for example the system work queue for extended
 * periods of time.
 */
static K_THREAD_STACK_DEFINE(application_stack_area, APP_WORK_Q_STACK_SIZE);
static struct k_work_q application_work_q;

/* Returns a positive integer if the new interval can be parsed, otherwise -1 */
static int event_interval_get(char *buf)
{
	struct cJSON *root_obj, *desired_obj, *event_interval_obj;
	int new_interval = -1;

	root_obj = cJSON_Parse(buf);
	if (root_obj == NULL) {
		LOG_ERR("Could not parse properties object");
		return -1;
	}

	/* If the incoming buffer is a notification from the cloud about changes
	 * made to the device twin's "desired" properties, the root object will
	 * only contain the newly changed properties, and can be treated as if
	 * it is the "desired" object.
	 * If the incoming is the response to a request to receive the device
	 * twin, it will contain a "desired" object and a "reported" object,
	 * and we need to access that object instead of the root.
	 */
	desired_obj = cJSON_GetObjectItem(root_obj, "desired");
	if (desired_obj == NULL) {
		desired_obj = root_obj;
	}

	/* Update only recognized properties. */
	event_interval_obj = cJSON_GetObjectItem(desired_obj,
						 "telemetryInterval");
	if (event_interval_obj == NULL) {
		LOG_INF("No 'telemetryInterval' object in the device twin");
		goto clean_exit;
	}

	if (cJSON_IsString(event_interval_obj)) {
		new_interval = atoi(event_interval_obj->valuestring);
	} else if (cJSON_IsNumber(event_interval_obj)) {
		new_interval = event_interval_obj->valueint;
	} else {
		LOG_WRN("Invalid telemetry interval format received");
		goto clean_exit;
	}

clean_exit:
	cJSON_Delete(root_obj);
	k_sem_give(&recv_buf_sem);

	return new_interval;
}

static void event_interval_apply(int interval)
{
	if (interval <= 0) {
		return;
	}

	atomic_set(&event_interval, interval);
	k_work_reschedule_for_queue(&application_work_q, &send_event_work, K_NO_WAIT);
}

static void on_evt_twin_desired(char *buf, size_t len)
{
	if (k_sem_take(&recv_buf_sem, K_NO_WAIT) == 0) {
		if (len > sizeof(recv_buf) - 1) {
			LOG_ERR("Incoming data too big for buffer");
			return;
		}

		memcpy(recv_buf, buf, len);
		recv_buf[len] = '\0';
		k_work_submit_to_queue(&application_work_q, &twin_report_work);
	} else {
		LOG_WRN("Recv buffer is busy, data was not copied");
	}
}

static void on_evt_direct_method(struct azure_iot_hub_method *method)
{
	size_t request_id_len = MIN(sizeof(method_data.request_id) - 1, method->request_id.size);
	size_t name_len = MIN(sizeof(method_data.name) - 1, method->name.size);

	LOG_INF("Method name: %.*s", method->name.size, method->name.ptr);
	LOG_INF("Payload: %.*s", method->payload.size, method->payload.ptr);

	memcpy(method_data.request_id, method->request_id.ptr, request_id_len);

	method_data.request_id[request_id_len] = '\0';

	memcpy(method_data.name, method->name.ptr, name_len);
	method_data.name[name_len] = '\0';

	snprintk(method_data.payload, sizeof(method_data.payload),
		 "%.*s", method->payload.size, method->payload.ptr);

	k_work_submit_to_queue(&application_work_q, &method_data.work);
}

static void reboot_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	sys_reboot(SYS_REBOOT_COLD);
}

static void azure_event_handler(struct azure_iot_hub_evt *const evt)
{
	switch (evt->type) {
	case AZURE_IOT_HUB_EVT_CONNECTING:
		LOG_INF("AZURE_IOT_HUB_EVT_CONNECTING");
		break;
	case AZURE_IOT_HUB_EVT_CONNECTED:
		LOG_INF("AZURE_IOT_HUB_EVT_CONNECTED");
		break;
	case AZURE_IOT_HUB_EVT_CONNECTION_FAILED:
		LOG_INF("AZURE_IOT_HUB_EVT_CONNECTION_FAILED");
		LOG_INF("Error code received from IoT Hub: %d",
			evt->data.err);
		break;
	case AZURE_IOT_HUB_EVT_DISCONNECTED:
		LOG_INF("AZURE_IOT_HUB_EVT_DISCONNECTED");
		break;
	case AZURE_IOT_HUB_EVT_READY:
		LOG_INF("AZURE_IOT_HUB_EVT_READY");

		/* All initializations and cloud connection were successful, now mark
		 * image as working so that we will not revert upon reboot.
		 */
#if defined(CONFIG_BOOTLOADER_MCUBOOT)
		boot_write_img_confirmed();
#endif

		/* The AZURE_IOT_HUB_EVT_READY event indicates that the
		 * IoT hub connection is established and interaction with the
		 * cloud can begin.
		 *
		 * The below work submission will cause send_event() to be
		 * call after 3 seconds.
		 */
		k_work_reschedule_for_queue(&application_work_q,
					    &send_event_work, K_NO_WAIT);
		break;
	case AZURE_IOT_HUB_EVT_DATA_RECEIVED:
		LOG_INF("AZURE_IOT_HUB_EVT_DATA_RECEIVED");
		LOG_INF("Received payload: %.*s",
			evt->data.msg.payload.size, evt->data.msg.payload.ptr);
		break;
	case AZURE_IOT_HUB_EVT_TWIN_RECEIVED:
		LOG_INF("AZURE_IOT_HUB_EVT_TWIN_RECEIVED");
		event_interval_apply(event_interval_get(evt->data.msg.payload.ptr));
		break;
	case AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED:
		LOG_INF("AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED");
		on_evt_twin_desired(evt->data.msg.payload.ptr, evt->data.msg.payload.size);
		break;
	case AZURE_IOT_HUB_EVT_DIRECT_METHOD:
		LOG_INF("AZURE_IOT_HUB_EVT_DIRECT_METHOD");
		on_evt_direct_method(&evt->data.method);
		break;
	case AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS:
		LOG_INF("AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS, ID: %.*s",
			evt->data.result.request_id.size, evt->data.result.request_id.ptr);
		break;
	case AZURE_IOT_HUB_EVT_TWIN_RESULT_FAIL:
		LOG_INF("AZURE_IOT_HUB_EVT_TWIN_RESULT_FAIL, ID %.*s, status %d",
			evt->data.result.request_id.size,
			evt->data.result.request_id.ptr,
			evt->data.result.status);
		break;
	case AZURE_IOT_HUB_EVT_PUBACK:
		LOG_INF("AZURE_IOT_HUB_EVT_PUBACK");
		break;
	case AZURE_IOT_HUB_EVT_FOTA_START:
		LOG_INF("AZURE_IOT_HUB_EVT_FOTA_START");
		break;
	case AZURE_IOT_HUB_EVT_FOTA_DONE:
		LOG_INF("AZURE_IOT_HUB_EVT_FOTA_DONE");
		LOG_INF("The device will reboot in 5 seconds to apply update");
		k_work_schedule(&reboot_work, K_SECONDS(5));
		break;
	case AZURE_IOT_HUB_EVT_FOTA_ERASE_PENDING:
		LOG_INF("AZURE_IOT_HUB_EVT_FOTA_ERASE_PENDING");
		break;
	case AZURE_IOT_HUB_EVT_FOTA_ERASE_DONE:
		LOG_INF("AZURE_IOT_HUB_EVT_FOTA_ERASE_DONE");
		break;
	case AZURE_IOT_HUB_EVT_FOTA_ERROR:
		LOG_ERR("AZURE_IOT_HUB_EVT_FOTA_ERROR: FOTA failed");
		break;
	case AZURE_IOT_HUB_EVT_ERROR:
		LOG_INF("AZURE_IOT_HUB_EVT_ERROR");
		break;
	default:
		LOG_ERR("Unknown Azure IoT Hub event type: %d", evt->type);
		break;
	}
}

static void send_event(struct k_work *work)
{
	int err;
	static char buf[60];
	ssize_t len;
	struct azure_iot_hub_msg msg = {
		.topic.type = AZURE_IOT_HUB_TOPIC_EVENT,
		.payload.ptr = buf,
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
	};

	len = snprintk(buf, sizeof(buf),
		       "{\"temperature\":%d.%d,\"timestamp\":%d}",
		       25, k_uptime_get_32() % 10, k_uptime_get_32());
	if ((len < 0) || (len > sizeof(buf))) {
		LOG_ERR("Failed to populate event buffer");
		goto exit;
	}

	msg.payload.size = len;

	LOG_INF("Sending event:%s", buf);

	err = azure_iot_hub_send(&msg);
	if (err) {
		LOG_ERR("Failed to send event");
		goto exit;
	}

	LOG_INF("Event was successfully sent");
exit:
	if (atomic_get(&event_interval) <= 0) {
		LOG_ERR("The event reporting stops, interval is set to %ld",
		       atomic_get(&event_interval));
		return;
	}

	LOG_INF("Next event will be sent in %ld seconds", event_interval);
	k_work_reschedule_for_queue(&application_work_q, &send_event_work,
				    K_SECONDS(event_interval));
}

static void direct_method_handler(struct k_work *work)
{
	int err;
	static char *response = "{\"it\":\"worked\"}";

	/* Status code 200 indicates successful execution of direct method. */
	struct azure_iot_hub_result result = {
		.request_id = {
			.ptr = method_data.request_id,
			.size = strlen(method_data.request_id),
		},
		.status = 200,
		.payload.ptr = response,
		.payload.size = sizeof(response) - 1,
	};
	bool led_state = strncmp(method_data.payload, "0", 1) ? 1 : 0;

	if (strcmp(method_data.name, "led") != 0) {
		LOG_INF("Unknown direct method");
		return;
	}

#if defined(CONFIG_DK_LIBRARY)
	err = dk_set_led(DK_LED1, led_state);
	if (err) {
		LOG_ERR("Failed to set LED, error: %d", err);
		return;
	}
#else
	LOG_INF("No hardware interface to set LED state to %d", led_state);
#endif

	err = azure_iot_hub_method_respond(&result);
	if (err) {
		LOG_ERR("Failed to send direct method response");
	}
}

static void twin_report_work_fn(struct k_work *work)
{
	int err;
	char buf[100];
	ssize_t len;
	struct azure_iot_hub_msg data = {
		.topic.type = AZURE_IOT_HUB_TOPIC_TWIN_REPORTED,
		.payload.ptr = buf,
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
	};
	int new_interval;

	new_interval = event_interval_get(recv_buf);
	if (new_interval < 0) {
		return;
	}

	len = snprintk(buf, sizeof(buf),
		       "{\"telemetryInterval\":%d}", new_interval);
	if (len <= 0) {
		LOG_ERR("Failed to create twin report");
		return;
	}

	data.payload.size = len;

	err = azure_iot_hub_send(&data);
	if (err) {
		LOG_ERR("Failed to send twin report");
		return;
	}

	/* Note that the new interval value is first applied here, because that
	 * will make the "reported" value in the device twin be in sync with
	 * the reality on the device. Other applications may decide
	 * to apply the desired properties regardless of whether the value is
	 * successfully reported or not.
	 */
	event_interval_apply(new_interval);
	LOG_INF("New telemetry interval has been applied: %d",  new_interval);
}

static void on_net_event_l4_connected(void)
{
	k_sem_give(&network_connected_sem);
}

static void on_net_event_l4_disconnected(void)
{
	(void)azure_iot_hub_disconnect();
}



static void l4_event_handler(struct net_mgmt_event_callback *cb,
			     uint32_t event,
			     struct net_if *iface)
{
	switch (event) {
	case NET_EVENT_L4_CONNECTED:
		LOG_INF("Network connectivity established and IP address assigned");
		on_net_event_l4_connected();
		break;
	case NET_EVENT_L4_DISCONNECTED:
		LOG_INF("Network connectivity lost, no IP address assigned");
		on_net_event_l4_disconnected();
		break;
	default:
		return;
	}
}

static void connectivity_event_handler(struct net_mgmt_event_callback *cb,
				       uint32_t event,
				       struct net_if *iface)
{
	if (event == NET_EVENT_CONN_IF_FATAL_ERROR) {
		LOG_ERR("Fatal error received from the connectivity layer, rebooting");
		sys_reboot(SYS_REBOOT_COLD);
		CODE_UNREACHABLE;
	}
}

static void work_init(void)
{
	k_work_init(&method_data.work, direct_method_handler);
	k_work_init(&twin_report_work, twin_report_work_fn);
	k_work_init_delayable(&send_event_work, send_event);
	k_work_init_delayable(&reboot_work, reboot_work_fn);
	k_work_queue_start(&application_work_q, application_stack_area,
		       K_THREAD_STACK_SIZEOF(application_stack_area),
		       K_HIGHEST_APPLICATION_THREAD_PRIO, NULL);
}

#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)
static K_SEM_DEFINE(dps_done_sem, 0, 1);

static void dps_handler(enum azure_iot_hub_dps_reg_status status)
{
	switch (status) {
	case AZURE_IOT_HUB_DPS_REG_STATUS_NOT_STARTED:
		LOG_INF("DPS registration status: AZURE_IOT_HUB_DPS_REG_STATUS_NOT_STARTED");
		break;
	case AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNING:
		LOG_INF("DPS registration status: AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNING");
		break;
	case AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNED:
		LOG_INF("DPS registration status: AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNED");

		dps_was_successful = true;

		k_sem_give(&dps_done_sem);
		break;
	case AZURE_IOT_HUB_DPS_REG_STATUS_FAILED:
		LOG_INF("DPS registration status: AZURE_IOT_HUB_DPS_REG_STATUS_FAILED");

		dps_was_successful = false;

		k_sem_give(&dps_done_sem);
		break;
	default:
		LOG_WRN("Unhandled DPS status: %d", status);
		break;
	}
}

/* Run DPS using the provided device_id as registration ID. Upon success, the function will return
 * 0 and populate the hostname and device_id buffers with the assigned values.
 * The return value will be a negative integer in case of failure.
 */
static int dps_run(struct azure_iot_hub_buf *hostname, struct azure_iot_hub_buf *device_id)
{
	int err;
	struct azure_iot_hub_dps_config dps_cfg = {
		.handler = dps_handler,
		.reg_id = {
			.ptr = device_id->ptr,
			.size = strlen(device_id->ptr),
		},
		/* Default ID scope provided by Kconfig is used, so we will not change that. */
	};

	LOG_INF("Starting DPS");

	err = azure_iot_hub_dps_init(&dps_cfg);
	if (err) {
		LOG_ERR("azure_iot_hub_dps_init failed, error: %d", err);
		return err;
	}

	err = azure_iot_hub_dps_start();
	if (err == 0) {
		LOG_INF("The DPS process has started, timeout is set to %d seconds",
			CONFIG_AZURE_IOT_HUB_DPS_TIMEOUT_SEC);

		/* If DPS was started successfully, we wait for the semaphore that is given when the
		 * provisioning completes.
		 */

		(void)k_sem_take(&dps_done_sem, K_FOREVER);

		if (!dps_was_successful) {
			return -EFAULT;
		}

		/* The device was assigned, continue to retrieve the hostname. */
	} else if (err == -EALREADY) {
		LOG_INF("Already assigned to an IoT hub, skipping DPS");
	} else {
		LOG_ERR("DPS failed to start, error: %d", err);
		return err;
	}

	err = azure_iot_hub_dps_hostname_get(hostname);
	if (err) {
		LOG_ERR("Failed to get hostname, error: %d", err);
		return err;
	}

	err = azure_iot_hub_dps_device_id_get(device_id);
	if (err) {
		LOG_ERR("Failed to get device ID, error: %d", err);
		return err;
	}

	LOG_INF("Device ID \"%.*s\" assigned to IoT hub with hostname \"%.*s\"",
		device_id->size, device_id->ptr,
		hostname->size, hostname->ptr);

	return 0;
}
#endif /* CONFIG_AZURE_IOT_HUB_DPS */

int main(void)
{
	int err;
	char hostname[128] = CONFIG_AZURE_IOT_HUB_HOSTNAME;
	char device_id[128] = CONFIG_AZURE_IOT_HUB_DEVICE_ID;
	struct azure_iot_hub_config cfg = {
		.device_id = {
			.ptr = device_id,
			.size = strlen(device_id),
		},
		.hostname = {
			.ptr = hostname,
			.size = strlen(hostname),
		},
		.use_dps = true,
	};

	LOG_INF("Azure IoT Hub sample started");

	/* Setup handler for Zephyr NET Connection Manager events. */
	net_mgmt_init_event_callback(&l4_cb, l4_event_handler, L4_EVENT_MASK);
	net_mgmt_add_event_callback(&l4_cb);

	/* Setup handler for Zephyr NET Connection Manager Connectivity layer. */
	net_mgmt_init_event_callback(&conn_cb, connectivity_event_handler, CONN_LAYER_EVENT_MASK);
	net_mgmt_add_event_callback(&conn_cb);

	LOG_INF("Bringing network interface up and connecting to the network");

	/* Connecting to the configured connectivity layer.
	 * Wi-Fi or LTE depending on the board that the sample was built for.
	 */
	err = conn_mgr_all_if_up(true);
	if (err) {
		LOG_ERR("conn_mgr_if_connect, error: %d", err);
		return err;
	}

#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_SAMPLE_DEVICE_ID_USE_HW_ID)
	err = hw_id_get(device_id, ARRAY_SIZE(device_id));
	if (err) {
		LOG_ERR("Failed to retrieve device ID");
		return 0;
	}
	cfg.device_id.size = strlen(device_id);
#endif

	LOG_INF("Device ID: %s", device_id);

	if (!IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)) {
		LOG_INF("Host name: %s", hostname);
	}

#if IS_ENABLED(CONFIG_DK_LIBRARY)
	dk_leds_init();
#endif

	work_init();
	cJSON_Init();

	/* Resend connection status if the sample is built for QEMU x86.
	 * This is necessary because the network interface is automatically brought up
	 * at SYS_INIT() before main() is called.
	 * This means that NET_EVENT_L4_CONNECTED fires before the
	 * appropriate handler l4_event_handler() is registered.
	 */
	if (IS_ENABLED(CONFIG_BOARD_QEMU_X86)) {
		conn_mgr_mon_resend_status();
	}

	k_sem_take(&network_connected_sem, K_FOREVER);

	LOG_INF("Connected to network");

#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)
	/* Using the device ID as DPS registration ID. */
	err = dps_run(&cfg.hostname, &cfg.device_id);
	if (err) {
		LOG_ERR("Failed to run DPS, error: %d, terminating connection attempt", err);
		return 0;
	}
#endif

	err = azure_iot_hub_init(azure_event_handler);
	if (err) {
		LOG_ERR("Azure IoT Hub could not be initialized, error: %d", err);
		return 0;
	}

	LOG_INF("Azure IoT Hub library initialized");

	err = azure_iot_hub_connect(&cfg);
	if (err < 0) {
		LOG_ERR("azure_iot_hub_connect failed: %d", err);
		return 0;
	}

	LOG_INF("Connection request sent to IoT Hub");

	/* After the connection to the IoT hub has been established, the
	 * Azure IoT Hub library will generate events when data is received.
	 * See azure_event_handler() for which actions will be taken on the
	 * various events.
	 */
	return 0;
}
