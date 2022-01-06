/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <net/azure_iot_hub.h>
#include <net/azure_iot_hub_dps.h>
#include <dk_buttons_and_leds.h>
#include <cJSON.h>
#include <cJSON_os.h>

#if IS_ENABLED(CONFIG_LTE_LINK_CONTROL)
#include <modem/lte_lc.h>
#endif

/* Interval [s] between sending events to the IoT hub. The value can be changed
 * by setting a new desired value for property 'telemetryInterval' in the
 * device twin document.
 */
#define EVENT_INTERVAL		20
#define RECV_BUF_SIZE		1024
#define APP_WORK_Q_STACK_SIZE	KB(2)

struct method_data {
	struct k_work work;
	char request_id[8];
	char name[32];
	char payload[200];
} method_data;
struct k_work twin_report_work;
struct k_work_delayable send_event_work;

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
		printk("Could not parse properties object\n");
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
		printk("No 'telemetryInterval' object in the device twin\n");
		goto clean_exit;
	}

	if (cJSON_IsString(event_interval_obj)) {
		new_interval = atoi(event_interval_obj->valuestring);
	} else if (cJSON_IsNumber(event_interval_obj)) {
		new_interval = event_interval_obj->valueint;
	} else {
		printk("Invalid telemetry interval format received\n");
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
	k_work_reschedule_for_queue(&application_work_q, &send_event_work,
				    K_NO_WAIT);
}

static void on_evt_twin_desired(char *buf, size_t len)
{
	if (k_sem_take(&recv_buf_sem, K_NO_WAIT) == 0) {
		if (len > sizeof(recv_buf) - 1) {
			printk("Incoming data too big for buffer");
			return;
		}

		memcpy(recv_buf, buf, len);
		recv_buf[len] = '\0';
		k_work_submit_to_queue(&application_work_q, &twin_report_work);
	} else {
		printk("Recv buffer is busy, data was not copied\n");
	}
}

static void on_evt_direct_method(struct azure_iot_hub_method *method)
{
	size_t request_id_len = MIN(sizeof(method_data.request_id) - 1, method->request_id.size);
	size_t name_len = MIN(sizeof(method_data.name) - 1, method->name.size);

	printk("Method name: %.*s\n", method->name.size, method->name.ptr);
	printk("Payload: %.*s\n", method->payload.size, method->payload.ptr);

	memcpy(method_data.request_id, method->request_id.ptr, request_id_len);

	method_data.request_id[request_id_len] = '\0';

	memcpy(method_data.name, method->name.ptr, name_len);
	method_data.name[name_len] = '\0';

	snprintk(method_data.payload, sizeof(method_data.payload),
		 "%.*s", method->payload.size, method->payload.ptr);

	k_work_submit_to_queue(&application_work_q, &method_data.work);
}

static void azure_event_handler(struct azure_iot_hub_evt *const evt)
{
	switch (evt->type) {
	case AZURE_IOT_HUB_EVT_CONNECTING:
		printk("AZURE_IOT_HUB_EVT_CONNECTING\n");
		break;
	case AZURE_IOT_HUB_EVT_CONNECTED:
		printk("AZURE_IOT_HUB_EVT_CONNECTED\n");
		break;
	case AZURE_IOT_HUB_EVT_CONNECTION_FAILED:
		printk("AZURE_IOT_HUB_EVT_CONNECTION_FAILED\n");
		printk("Error code received from IoT Hub: %d\n",
		       evt->data.err);
		break;
	case AZURE_IOT_HUB_EVT_DISCONNECTED:
		printk("AZURE_IOT_HUB_EVT_DISCONNECTED\n");
		break;
	case AZURE_IOT_HUB_EVT_READY:
		printk("AZURE_IOT_HUB_EVT_READY\n");

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
		printk("AZURE_IOT_HUB_EVT_DATA_RECEIVED\n");
		printk("Received payload: %.*s\n",
			evt->data.msg.payload.size, evt->data.msg.payload.ptr);
		break;
	case AZURE_IOT_HUB_EVT_TWIN_RECEIVED:
		printk("AZURE_IOT_HUB_EVT_TWIN_RECEIVED\n");
		event_interval_apply(event_interval_get(evt->data.msg.payload.ptr));
		break;
	case AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED:
		printk("AZURE_IOT_HUB_EVT_TWIN_DESIRED_RECEIVED\n");
		on_evt_twin_desired(evt->data.msg.payload.ptr, evt->data.msg.payload.size);
		break;
	case AZURE_IOT_HUB_EVT_DIRECT_METHOD:
		printk("AZURE_IOT_HUB_EVT_DIRECT_METHOD\n");
		on_evt_direct_method(&evt->data.method);
		break;
	case AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS:
		printk("AZURE_IOT_HUB_EVT_TWIN_RESULT_SUCCESS, ID: %.*s\n",
		       evt->data.result.request_id.size, evt->data.result.request_id.ptr);
		break;
	case AZURE_IOT_HUB_EVT_TWIN_RESULT_FAIL:
		printk("AZURE_IOT_HUB_EVT_TWIN_RESULT_FAIL, ID %.*s, status %d\n",
			evt->data.result.request_id.size,
			evt->data.result.request_id.ptr,
			evt->data.result.status);
		break;
	case AZURE_IOT_HUB_EVT_PUBACK:
		printk("AZURE_IOT_HUB_EVT_PUBACK\n");
		break;
	case AZURE_IOT_HUB_EVT_ERROR:
		printk("AZURE_IOT_HUB_EVT_ERROR\n");
		break;
	default:
		printk("Unknown Azure IoT Hub event type: %d\n", evt->type);
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
		printk("Failed to populate event buffer\n");
		goto exit;
	}

	msg.payload.size = len;

	printk("Sending event:\n%s\n", buf);

	err = azure_iot_hub_send(&msg);
	if (err) {
		printk("Failed to send event\n");
		goto exit;
	}

	printk("Event was successfully sent\n");
exit:
	if (atomic_get(&event_interval) <= 0) {
		printk("The event reporting stops, interval is set to %ld\n",
		       atomic_get(&event_interval));
		return;
	}

	printk("Next event will be sent in %ld seconds\n", event_interval);
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
		printk("Unknown direct method\n");
		return;
	}

#if defined(CONFIG_DK_LIBRARY)
	err = dk_set_led(DK_LED1, led_state);
	if (err) {
		printk("Failed to set LED, error: %d\n", err);
		return;
	}
#else
	printk("No hardware interface to set LED state to %d\n", led_state);
#endif

	err = azure_iot_hub_method_respond(&result);
	if (err) {
		printk("Failed to send direct method response\n");
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
		printk("Failed to create twin report\n");
		return;
	}

	data.payload.size = len;

	err = azure_iot_hub_send(&data);
	if (err) {
		printk("Failed to send twin report");
		return;
	}

	/* Note that the new interval value is first applied here, because that
	 * will make the "reported" value in the device twin be in sync with
	 * the reality on the device. Other applications may decide
	 * to apply the desired properties regardless of whether the value is
	 * successfully reported or not.
	 */
	event_interval_apply(new_interval);
	printk("New telemetry interval has been applied: %d\n",  new_interval);
}

#if IS_ENABLED(CONFIG_LTE_LINK_CONTROL)

static void lte_handler(const struct lte_lc_evt *const evt)
{
	static bool connected;

	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		    (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			if (!connected) {
				break;
			}

			connected = false;

			printk("LTE network is disconnected.\n");
			printk("Subsequent sending of data may block or fail.\n");
			break;
		}

		printk("Network registration status: %s\n",
		       evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
		       "Connected - home network" : "Connected - roaming");
		k_sem_give(&network_connected_sem);

		connected = true;
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		printk("PSM parameter update: TAU: %d, Active time: %d\n",
		       evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintk(log_buf, sizeof(log_buf),
			       "eDRX parameter update: eDRX: %f, PTW: %f",
			       evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			printk("%s\n", log_buf);
		}
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		printk("RRC mode: %s\n",
		       evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
		       "Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		printk("LTE cell changed: Cell ID: %d, Tracking area: %d\n",
		       evt->cell.id, evt->cell.tac);
		break;
	default:
		break;
	}
}

static void modem_configure(void)
{
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already configured and LTE connected. */
	} else {
		int err = lte_lc_init_and_connect_async(lte_handler);

		if (err) {
			printk("Modem could not be configured, error: %d\n",
				err);
			return;
		}
	}
}
#endif

static void work_init(void)
{
	k_work_init(&method_data.work, direct_method_handler);
	k_work_init(&twin_report_work, twin_report_work_fn);
	k_work_init_delayable(&send_event_work, send_event);
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
		printk("DPS registration status: AZURE_IOT_HUB_DPS_REG_STATUS_NOT_STARTED\n");
		break;
	case AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNING:
		printk("DPS registration status: AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNING\n");
		break;
	case AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNED:
		printk("DPS registration status: AZURE_IOT_HUB_DPS_REG_STATUS_ASSIGNED\n");

		dps_was_successful = true;

		k_sem_give(&dps_done_sem);
		break;
	case AZURE_IOT_HUB_DPS_REG_STATUS_FAILED:
		printk("DPS registration status: AZURE_IOT_HUB_DPS_REG_STATUS_FAILED\n");

		dps_was_successful = false;

		k_sem_give(&dps_done_sem);
		break;
	default:
		printk("Unhandled DPS status: %d\n", status);
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

	printk("Starting DPS\n");

	err = azure_iot_hub_dps_init(&dps_cfg);
	if (err) {
		printk("azure_iot_hub_dps_init failed, error: %d\n", err);
		return err;
	}

	err = azure_iot_hub_dps_start();
	if (err == 0) {
		printk("The DPS process has started, timeout is set to %d seconds\n",
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
		printk("Already assigned to an IoT hub, skipping DPS\n");
	} else {
		printk("DPS failed to start, error: %d\n", err);
		return err;
	}

	err = azure_iot_hub_dps_hostname_get(hostname);
	if (err) {
		printk("Failed to get hostname, error: %d", err);
		return err;
	}

	printk("Device ID \"%.*s\" assigned to IoT hub with hostname \"%.*s\"\n",
		device_id->size, device_id->ptr,
		hostname->size, hostname->ptr);

	return 0;
}
#endif /* CONFIG_AZURE_IOT_HUB_DPS && !CONFIG_AZURE_IOT_HUB_DPS_AUTO */

void main(void)
{
	int err;
	char device_id[128] = CONFIG_AZURE_IOT_HUB_DEVICE_ID;
	char hostname[128] = CONFIG_AZURE_IOT_HUB_HOSTNAME;
	struct azure_iot_hub_config cfg = {
		.device_id = {
			.ptr = device_id,
			.size = sizeof(CONFIG_AZURE_IOT_HUB_DEVICE_ID) - 1,
		},
		.hostname = {
			.ptr = hostname,
			.size = sizeof(CONFIG_AZURE_IOT_HUB_HOSTNAME) - 1,
		},
		.use_dps = true,
	};

	printk("Azure IoT Hub sample started\n");

#if IS_ENABLED(CONFIG_DK_LIBRARY)
	dk_leds_init();
#endif

	work_init();
	cJSON_Init();

#if IS_ENABLED(CONFIG_LTE_LINK_CONTROL)
	printk("Connecting to LTE network\n");
	modem_configure();
	k_sem_take(&network_connected_sem, K_FOREVER);
	printk("Connected to LTE network\n");
#endif

#if IS_ENABLED(CONFIG_AZURE_IOT_HUB_DPS)
	/* Using the device ID as DPS registration ID. */
	err = dps_run(&cfg.hostname, &cfg.device_id);
	if (err) {
		printk("Failed to run DPS, error: %d\n, terminating connection attempt", err);
		return;
	}
#endif

	err = azure_iot_hub_init(azure_event_handler);
	if (err) {
		printk("Azure IoT Hub could not be initialized, error: %d\n", err);
		return;
	}

	printk("Azure IoT Hub library initialized\n");

	err = azure_iot_hub_connect(&cfg);
	if (err < 0) {
		printk("azure_iot_hub_connect failed: %d\n", err);
		return;
	}

	printk("Connection request sent to IoT Hub\n");

	/* After the connection to the IoT hub has been established, the
	 * Azure IoT Hub library will generate events when data is received.
	 * See azure_event_handler() for which actions will be taken on the
	 * various events.
	 */
}
