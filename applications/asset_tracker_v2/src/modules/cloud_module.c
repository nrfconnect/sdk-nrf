/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <stdio.h>
#include <zephyr/dfu/mcuboot.h>
#include <math.h>
#include <app_event_manager.h>
#include <qos.h>

#if defined(CONFIG_NRF_CLOUD_AGNSS)
#include <modem/modem_info.h>
#include <net/nrf_cloud_agnss.h>
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_pgps.h>
#endif

#include "cloud_wrapper.h"
#include "cloud/cloud_codec/cloud_codec.h"

#define MODULE cloud_module

#include "modules_common.h"
#include "events/cloud_module_event.h"
#include "events/app_module_event.h"
#include "events/data_module_event.h"
#include "events/util_module_event.h"
#include "events/modem_module_event.h"
#include "events/location_module_event.h"
#include "events/debug_module_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CLOUD_MODULE_LOG_LEVEL);

BUILD_ASSERT(CONFIG_CLOUD_CONNECT_RETRIES < 14,
	     "Cloud connect retries too large");

BUILD_ASSERT(IS_ENABLED(CONFIG_NRF_CLOUD_MQTT) ||
	     IS_ENABLED(CONFIG_AWS_IOT)	       ||
	     IS_ENABLED(CONFIG_AZURE_IOT_HUB)  ||
	     IS_ENABLED(CONFIG_LWM2M_INTEGRATION),
	     "A cloud transport service must be enabled");

#if defined(CONFIG_BOARD_QEMU_X86) || defined(CONFIG_BOARD_NATIVE_POSIX)
BUILD_ASSERT(IS_ENABLED(CONFIG_CLOUD_CLIENT_ID_USE_CUSTOM),
	     "Passing IMEI as cloud client ID is not supported when building for PC builds. "
	     "This is because IMEI is retrieved from the modem and not available when running "
	     "Zephyr's native TCP/IP stack.");
#endif

struct cloud_msg_data {
	union {
		struct app_module_event app;
		struct data_module_event data;
		struct modem_module_event modem;
		struct cloud_module_event cloud;
		struct util_module_event util;
		struct location_module_event location;
		struct debug_module_event debug;
	} module;
};

/* Cloud module super states. */
static enum state_type {
	STATE_LTE_INIT,
	STATE_LTE_DISCONNECTED,
	STATE_LTE_CONNECTED,
	STATE_SHUTDOWN
} state;

/* Cloud module sub states. */
static enum sub_state_type {
	SUB_STATE_CLOUD_DISCONNECTED,
	SUB_STATE_CLOUD_CONNECTED
} sub_state;

static struct k_work_delayable connect_check_work;

struct cloud_backoff_delay_lookup {
	int delay;
};

/* Lookup table for backoff reconnection to cloud. Binary scaling. */
static struct cloud_backoff_delay_lookup backoff_delay[] = {
	{ 32 }, { 64 }, { 128 }, { 256 }, { 512 }, { 1024 },
	{ 2048 }, { 4096 }, { 8192 }, { 16384 }, { 32768 },
	{ 65536 }, { 131072 }, { 262144 }, { 524288 }, { 1048576 }
};

/* Variable that keeps track of how many times a reconnection to cloud
 * has been tried without success.
 */
static int connect_retries;

/* Local copy of the device configuration. */
static struct cloud_data_cfg copy_cfg;
const k_tid_t cloud_module_thread;

/* Message IDs that are used with the QoS library. */
enum {
	GENERIC = 0,
	BATCH,
	UI,
	CLOUD_LOCATION,
	AGNSS_REQUEST,
	PGPS_REQUEST,
	CONFIG,
	MEMFAULT,
};

#if defined(CONFIG_NRF_CLOUD_AGNSS)
/* Whether `agnss_request_buffer` has A-GNSS request buffered for sending when connection to
 * cloud has been re-established.
 */
static bool agnss_request_buffered;

/* Buffered A-GNSS request. */
static struct nrf_modem_gnss_agnss_data_frame agnss_request_buffer;
#endif /* CONFIG_NRF_CLOUD_AGNSS */

/* Cloud module message queue. */
#define CLOUD_QUEUE_ENTRY_COUNT		20
#define CLOUD_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_cloud, sizeof(struct cloud_msg_data),
	      CLOUD_QUEUE_ENTRY_COUNT, CLOUD_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
	.name = "cloud",
	.msg_q = &msgq_cloud,
	.supports_shutdown = true
};

/* Forward declarations. */
static void connect_check_work_fn(struct k_work *work);
static void send_config_received(void);
static void add_qos_message(uint8_t *ptr, size_t len, uint8_t type,
			    uint32_t flags, bool heap_allocated);

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type state)
{
	switch (state) {
	case STATE_LTE_INIT:
		return "STATE_LTE_INIT";
	case STATE_LTE_DISCONNECTED:
		return "STATE_LTE_DISCONNECTED";
	case STATE_LTE_CONNECTED:
		return "STATE_LTE_CONNECTED";
	case STATE_SHUTDOWN:
		return "STATE_SHUTDOWN";
	default:
		return "Unknown";
	}
}

static char *sub_state2str(enum sub_state_type new_state)
{
	switch (new_state) {
	case SUB_STATE_CLOUD_DISCONNECTED:
		return "SUB_STATE_CLOUD_DISCONNECTED";
	case SUB_STATE_CLOUD_CONNECTED:
		return "SUB_STATE_CLOUD_CONNECTED";
	default:
		return "Unknown";
	}
}

static void state_set(enum state_type new_state)
{
	if (new_state == state) {
		LOG_DBG("State: %s", state2str(state));
		return;
	}

	LOG_DBG("State transition %s --> %s",
		state2str(state),
		state2str(new_state));

	state = new_state;
}

static void sub_state_set(enum sub_state_type new_state)
{
	if (new_state == sub_state) {
		LOG_DBG("Sub state: %s", sub_state2str(sub_state));
		return;
	}

	LOG_DBG("Sub state transition %s --> %s",
		sub_state2str(sub_state),
		sub_state2str(new_state));

	sub_state = new_state;
}

#if defined(CONFIG_NRF_CLOUD_AGNSS) && !defined(CONFIG_NRF_CLOUD_MQTT)
static int get_modem_info(struct modem_param_info *const modem_info)
{
	__ASSERT_NO_MSG(modem_info != NULL);

	int err = modem_info_init();

	if (err) {
		LOG_ERR("Could not initialize modem info module, error: %d", err);
		return err;
	}

	err = modem_info_params_init(modem_info);
	if (err) {
		LOG_ERR("Could not initialize modem info parameters, error: %d", err);
		return err;
	}

	err = modem_info_params_get(modem_info);
	if (err) {
		LOG_ERR("Could not obtain cell information, error: %d", err);
		return err;
	}

	return 0;
}

static int agnss_request_encode(struct nrf_modem_gnss_agnss_data_frame *incoming_request)
{
	int err;
	struct cloud_codec_data output = {0};
	static struct modem_param_info modem_info = {0};
	static struct cloud_data_agnss_request cloud_agnss_request = {0};

	err = get_modem_info(&modem_info);
	if (err) {
		return err;
	}

	if (incoming_request == NULL) {
		const uint32_t mask = IS_ENABLED(CONFIG_NRF_CLOUD_PGPS) ? 0u : 0xFFFFFFFFu;

		LOG_DBG("Requesting all A-GNSS elements");
		cloud_agnss_request.request.data_flags =
					NRF_MODEM_GNSS_AGNSS_GPS_UTC_REQUEST |
					NRF_MODEM_GNSS_AGNSS_KLOBUCHAR_REQUEST |
					NRF_MODEM_GNSS_AGNSS_NEQUICK_REQUEST |
					NRF_MODEM_GNSS_AGNSS_GPS_SYS_TIME_AND_SV_TOW_REQUEST |
					NRF_MODEM_GNSS_AGNSS_POSITION_REQUEST |
					NRF_MODEM_GNSS_AGNSS_INTEGRITY_REQUEST;
		cloud_agnss_request.request.system_count = 1;
		cloud_agnss_request.request.system[0].sv_mask_ephe = mask;
		cloud_agnss_request.request.system[0].sv_mask_alm = mask;
	} else {
		cloud_agnss_request.request = *incoming_request;
	}

	cloud_agnss_request.mcc = modem_info.network.mcc.value;
	cloud_agnss_request.mnc = modem_info.network.mnc.value;
	cloud_agnss_request.cell = modem_info.network.cellid_dec;
	cloud_agnss_request.area = modem_info.network.area_code.value;
	cloud_agnss_request.queued = true;

	err = cloud_codec_encode_agnss_request(&output, &cloud_agnss_request);
	switch (err) {
	case 0:
		LOG_DBG("A-GNSS request encoded successfully");

		if (IS_ENABLED(CONFIG_LWM2M_INTEGRATION)) {
			err = cloud_wrap_agnss_request_send(NULL, 0, true, 0);
			if (err) {
				LOG_ERR("cloud_wrap_agnss_request_send, err: %d", err);
			}

			return err;
		}

		add_qos_message(output.buf,
				output.len,
				AGNSS_REQUEST,
				QOS_FLAG_RELIABILITY_ACK_REQUIRED,
				true);
		break;
	case -ENOTSUP:
		LOG_ERR("Encoding of A-GNSS requests are not supported by the configured codec");
		break;
	case -ENODATA:
		LOG_DBG("No A-GNSS request data to encode, error: %d", err);
		break;
	default:
		LOG_ERR("Error encoding A-GNSS request: %d", err);
		SEND_ERROR(data, CLOUD_EVT_ERROR, err);
		break;
	}

	return err;
}
#endif /* CONFIG_NRF_CLOUD_AGNSS && !CONFIG_NRF_CLOUD_MQTT */

#if defined(CONFIG_NRF_CLOUD_PGPS) && !defined(CONFIG_NRF_CLOUD_MQTT)
static int pgps_request_encode(struct gps_pgps_request *incoming_request)
{
	/* Encode and send P-GPS request to cloud. */
	int err;
	struct cloud_codec_data output = {0};
	struct cloud_data_pgps_request request = {
		.count = incoming_request->prediction_count,
		.interval = incoming_request->prediction_period_min,
		.day = incoming_request->gps_day,
		.time = incoming_request->gps_time_of_day,
		.queued = true,
	};

	err = cloud_codec_encode_pgps_request(&output, &request);
	switch (err) {
	case 0:
		LOG_DBG("P-GPS request encoded successfully");

		if (IS_ENABLED(CONFIG_LWM2M_INTEGRATION)) {
			err = cloud_wrap_pgps_request_send(NULL, 0, true, 0);
			if (err) {
				LOG_ERR("cloud_wrap_pgps_request_send, err: %d", err);
			}

			return err;
		}

		add_qos_message(output.buf,
				output.len,
				PGPS_REQUEST,
				QOS_FLAG_RELIABILITY_ACK_REQUIRED,
				true);
		break;
	case -ENOTSUP:
		LOG_DBG("P-GPS request encoding is not supported, error: %d", err);
		break;
	case -ENODATA:
		LOG_DBG("No P-GPS data to encode, error: %d", err);
		break;
	default:
		LOG_ERR("Error encoding P-GPS request: %d", err);
		SEND_ERROR(data, CLOUD_EVT_ERROR, err);
	}

	return err;
}
#endif /* CONFIG_NRF_CLOUD_PGPS && !CONFIG_NRF_CLOUD_MQTT */

/* Handlers */
static bool app_event_handler(const struct app_event_header *aeh)
{
	struct cloud_msg_data msg = {0};
	bool enqueue_msg = false, consume = false;

	if (is_app_module_event(aeh)) {
		struct app_module_event *evt = cast_app_module_event(aeh);

		msg.module.app = *evt;
		enqueue_msg = true;
	}

	if (is_data_module_event(aeh)) {
		struct data_module_event *evt = cast_data_module_event(aeh);

		msg.module.data = *evt;
		enqueue_msg = true;
	}

	if (is_modem_module_event(aeh)) {
		struct modem_module_event *evt = cast_modem_module_event(aeh);

		msg.module.modem = *evt;
		enqueue_msg = true;
	}

	if (is_cloud_module_event(aeh)) {
		struct cloud_module_event *evt = cast_cloud_module_event(aeh);

		msg.module.cloud = *evt;
		enqueue_msg = true;

		/* If the event is intended to only be used by the cloud module,
		 * the event is consumed after it has been added to the internal message queue.
		 * This is to prevent other modules that subscribe to cloud module events from
		 * processing redundant events. Cloud module events are subscribed to first using
		 * the APP_EVENT_SUBSCRIBE_FIRST macro.
		 */
		if (msg.module.cloud.type == CLOUD_EVT_DATA_SEND_QOS) {
			consume = true;
		}
	}

	if (is_util_module_event(aeh)) {
		struct util_module_event *evt = cast_util_module_event(aeh);

		msg.module.util = *evt;
		enqueue_msg = true;
	}

	if (is_location_module_event(aeh)) {
		struct location_module_event *evt = cast_location_module_event(aeh);

		msg.module.location = *evt;
		enqueue_msg = true;
	}

	if (is_debug_module_event(aeh)) {
		struct debug_module_event *evt = cast_debug_module_event(aeh);

		msg.module.debug = *evt;
		enqueue_msg = true;
	}

	if (enqueue_msg) {
		int err = module_enqueue_msg(&self, &msg);

		if (err) {
			LOG_ERR("Message could not be enqueued");
			SEND_ERROR(cloud, CLOUD_EVT_ERROR, err);
		}
	}

	return consume;
}

static void config_data_handle(uint8_t *buf, const size_t len)
{
	int err;

	/* Use the config copy when populating the config variable
	 * before it is sent to the Data module. This way we avoid
	 * sending uninitialized variables to the Data module.
	 */
	err = cloud_codec_decode_config(buf, len, &copy_cfg);
	if (err == 0) {
		LOG_DBG("Device configuration encoded");
		send_config_received();
	} else if (err == -ENODATA) {
		LOG_WRN("Device configuration empty!");
		SEND_EVENT(cloud, CLOUD_EVT_CONFIG_EMPTY);
	} else if (err == -ECANCELED) {
		/* The incoming message has already been handled, ignored. */
	} else if (err == -ENOENT) {
		/* Encoding of incoming message is not supported. */
	} else {
		LOG_ERR("Decoding of device configuration, error: %d", err);
		SEND_ERROR(cloud, CLOUD_EVT_ERROR, err);
	}
}

#if defined(CONFIG_NRF_CLOUD_AGNSS)
/**
 * @brief Requests A-GNSS data upon receiving a request from the location module.
 *
 * @param[in] incoming_request Pointer to a structure containing A-GNSS data types that has been
 *                             requested by the modem.
 */
static void agnss_request_handle(struct nrf_modem_gnss_agnss_data_frame *incoming_request)
{
	int err;

#if defined(CONFIG_NRF_CLOUD_MQTT)
	/* If CONFIG_NRF_CLOUD_MQTT is enabled, the nRF Cloud MQTT transport is used to send
	 * the request.
	 */
	err = nrf_cloud_agnss_request(incoming_request);
	if (err) {
		LOG_WRN("Failed to request A-GNSS data, error: %d", err);
		LOG_DBG("This is expected to fail if we are not in a connected state");
	} else {
		if (nrf_cloud_agnss_request_in_progress()) {
			LOG_DBG("A-GNSS request sent");
		} else {
			LOG_DBG("No A-GNSS data requested");
		}
	}
#else /* !CONFIG_NRF_CLOUD_MQTT */
	/* If the nRF Cloud MQTT transport is not enabled, encode the A-GNSS request and send it
	 * to the cloud.
	 */
	err = agnss_request_encode(incoming_request);
	if (err) {
		LOG_WRN("Failed to request A-GNSS data, error: %d", err);
	} else {
		LOG_DBG("A-GNSS request sent");
	}
#endif /* !CONFIG_NRF_CLOUD_MQTT */

	(void)err;
}
#endif /* CONFIG_NRF_CLOUD_AGNSS */

#if defined(CONFIG_NRF_CLOUD_PGPS)
/**
 * @brief Requests P-GPS data upon receiving a request from the location module.
 *
 * When nRF Cloud MQTT transport is used, the nRF Cloud P-GPS library handles the P-GPS data
 * download.
 *
 * @param[in] incoming_request Pointer to a structure containing the P-GPS data request.
 */
static void pgps_request_handle(struct gps_pgps_request *incoming_request)
{
#if !defined(CONFIG_NRF_CLOUD_MQTT)
	int err;

	/* nRF Cloud MQTT transport is not enabled, encode the P-GPS request and send it
	 * to the cloud.
	 */
	err = pgps_request_encode(incoming_request);
	if (err) {
		LOG_WRN("Failed to request P-GPS data, error: %d", err);
	} else {
		LOG_DBG("P-GPS request sent");
	}
#endif /* !CONFIG_NRF_CLOUD_MQTT */
}
#endif /* CONFIG_NRF_CLOUD_PGPS */

static void agnss_data_handle(const uint8_t *buf, const size_t len)
{
#if defined(CONFIG_NRF_CLOUD_AGNSS)
	(void)location_agnss_data_process(buf, len);
#endif
}

static void pgps_data_handle(const uint8_t *buf, const size_t len)
{
#if defined(CONFIG_NRF_CLOUD_PGPS) && !defined(CONFIG_NRF_CLOUD_PGPS_DOWNLOAD_TRANSPORT_CUSTOM)
	(void)location_pgps_data_process(buf, len);
#endif
}

#if defined(CONFIG_LOCATION)
void submit_cloud_module_event(
	enum cloud_module_event_type type,
	struct location_data *location,
	int error)
{
	struct cloud_module_event *cloud_module_event = new_cloud_module_event();

	__ASSERT(cloud_module_event, "Not enough heap left to allocate event");

	cloud_module_event->type = type;
	if (type == CLOUD_EVT_CLOUD_LOCATION_RECEIVED) {
		cloud_module_event->data.cloud_location = *location;
	} else if (type == CLOUD_EVT_ERROR) {
		cloud_module_event->data.err = error;
	}

	APP_EVENT_SUBMIT(cloud_module_event);
}
#endif

static void cloud_location_data_handle(uint8_t *buf, const size_t len)
{
#if defined(CONFIG_LOCATION)
	int err;
	struct location_data location;

	err = cloud_codec_decode_cloud_location(buf, len, &location);
	if (err == 0) {
		submit_cloud_module_event(CLOUD_EVT_CLOUD_LOCATION_RECEIVED, &location, 0);
	} else if (err == -EFAULT) {
		submit_cloud_module_event(CLOUD_EVT_CLOUD_LOCATION_ERROR, NULL, err);
	} else if (err == -ENOTSUP) {
		submit_cloud_module_event(CLOUD_EVT_CLOUD_LOCATION_UNKNOWN, NULL, 0);
	} else {
		LOG_ERR("Decoding of cloud location response, error: %d", err);
		SEND_ERROR(cloud, CLOUD_EVT_ERROR, err);
	}
#endif
}

static void cloud_wrap_event_handler(const struct cloud_wrap_event *const evt)
{
	switch (evt->type) {
	case CLOUD_WRAP_EVT_CONNECTING: {
		LOG_DBG("CLOUD_WRAP_EVT_CONNECTING");
		SEND_EVENT(cloud, CLOUD_EVT_CONNECTING);
		break;
	}
	case CLOUD_WRAP_EVT_CONNECTED: {
		LOG_DBG("CLOUD_WRAP_EVT_CONNECTED");
		SEND_EVENT(cloud, CLOUD_EVT_CONNECTED);
		break;
	}
	case CLOUD_WRAP_EVT_DISCONNECTED: {
		LOG_DBG("CLOUD_WRAP_EVT_DISCONNECTED");
		SEND_EVENT(cloud, CLOUD_EVT_DISCONNECTED);
		break;
	}
	case CLOUD_WRAP_EVT_DATA_RECEIVED:
		LOG_DBG("CLOUD_WRAP_EVT_DATA_RECEIVED");
		config_data_handle(evt->data.buf, evt->data.len);
		break;
	case CLOUD_WRAP_EVT_PGPS_DATA_RECEIVED:
		LOG_DBG("CLOUD_WRAP_EVT_PGPS_DATA_RECEIVED");
		pgps_data_handle(evt->data.buf, evt->data.len);
		break;
	case CLOUD_WRAP_EVT_AGNSS_DATA_RECEIVED:
		LOG_DBG("CLOUD_WRAP_EVT_AGNSS_DATA_RECEIVED");
		agnss_data_handle(evt->data.buf, evt->data.len);
		break;
	case CLOUD_WRAP_EVT_CLOUD_LOCATION_RESULT_RECEIVED:
		LOG_DBG("CLOUD_WRAP_EVT_CLOUD_LOCATION_RESULT_RECEIVED");
		cloud_location_data_handle(evt->data.buf, evt->data.len);
		break;
	case CLOUD_WRAP_EVT_USER_ASSOCIATION_REQUEST: {
		LOG_DBG("CLOUD_WRAP_EVT_USER_ASSOCIATION_REQUEST");

		/* Cancel the reconnection routine upon a user association request. Device is
		 * awaiting registration to an nRF Cloud and does not need to reconnect
		 * until this happens.
		 */
		k_work_cancel_delayable(&connect_check_work);
		connect_retries = 0;

		SEND_EVENT(cloud, CLOUD_EVT_USER_ASSOCIATION_REQUEST);
		break;
	}
	case CLOUD_WRAP_EVT_USER_ASSOCIATED: {
		LOG_DBG("CLOUD_WRAP_EVT_USER_ASSOCIATED");

		/* After user association, the device is disconnected. Reconnect immediately
		 * to complete the process.
		 */
		if (!k_work_delayable_is_pending(&connect_check_work)) {
			k_work_reschedule(&connect_check_work, K_SECONDS(5));
		}

		SEND_EVENT(cloud, CLOUD_EVT_USER_ASSOCIATED);
		break;
	}
	case CLOUD_WRAP_EVT_REBOOT_REQUEST: {
		SEND_EVENT(cloud, CLOUD_EVT_REBOOT_REQUEST);
		break;
	}
	case CLOUD_WRAP_EVT_LTE_DISCONNECT_REQUEST: {
		SEND_EVENT(cloud, CLOUD_EVT_LTE_DISCONNECT);
		break;
	}
	case CLOUD_WRAP_EVT_LTE_CONNECT_REQUEST: {
		SEND_EVENT(cloud, CLOUD_EVT_LTE_CONNECT);
		break;
	}
	case CLOUD_WRAP_EVT_FOTA_DONE: {
		LOG_DBG("CLOUD_WRAP_EVT_FOTA_DONE");
		SEND_EVENT(cloud, CLOUD_EVT_FOTA_DONE);
		break;
	}
	case CLOUD_WRAP_EVT_FOTA_START: {
		LOG_DBG("CLOUD_WRAP_EVT_FOTA_START");
		SEND_EVENT(cloud, CLOUD_EVT_FOTA_START);
		break;
	}
	case CLOUD_WRAP_EVT_FOTA_ERASE_PENDING:
		LOG_DBG("CLOUD_WRAP_EVT_FOTA_ERASE_PENDING");
		break;
	case CLOUD_WRAP_EVT_FOTA_ERASE_DONE:
		LOG_DBG("CLOUD_WRAP_EVT_FOTA_ERASE_DONE");
		break;
	case CLOUD_WRAP_EVT_FOTA_ERROR: {
		LOG_DBG("CLOUD_WRAP_EVT_FOTA_ERROR");
		SEND_EVENT(cloud, CLOUD_EVT_FOTA_ERROR);
		break;
	}
	case CLOUD_WRAP_EVT_DATA_ACK: {
		LOG_DBG("CLOUD_WRAP_EVT_DATA_ACK: %d", evt->message_id);

		int err = qos_message_remove(evt->message_id);

		if (err == -ENODATA) {
			LOG_DBG("Message Acknowledgment not in pending QoS list, ID: %d",
				evt->message_id);
		} else if (err) {
			LOG_ERR("qos_message_remove, error: %d", err);
			SEND_ERROR(cloud, CLOUD_EVT_ERROR, err);
		}

		break;
	}
	case CLOUD_WRAP_EVT_PING_ACK: {
		LOG_DBG("CLOUD_WRAP_EVT_PING_ACK");

		/* Notify all messages upon a PING ACK. This means that there most likely is an
		 * established RCC connection and we can try to send all available messages.
		 */
		qos_message_notify_all();
		break;
	}
	case CLOUD_WRAP_EVT_ERROR: {
		LOG_DBG("CLOUD_WRAP_EVT_ERROR");
		SEND_ERROR(cloud, CLOUD_EVT_ERROR, evt->err);
		break;
	}
	default:
		break;

	}
}

/* Static module functions. */
static void send_config_received(void)
{
	struct cloud_module_event *cloud_module_event = new_cloud_module_event();

	__ASSERT(cloud_module_event, "Not enough heap left to allocate event");

	cloud_module_event->type = CLOUD_EVT_CONFIG_RECEIVED;
	cloud_module_event->data.config = copy_cfg;

	APP_EVENT_SUBMIT(cloud_module_event);
}

static void connect_cloud(void)
{
	int err;
	int backoff_sec = backoff_delay[connect_retries].delay;

	LOG_DBG("Connecting to cloud");

	if (connect_retries > CONFIG_CLOUD_CONNECT_RETRIES) {
		LOG_WRN("Too many failed cloud connection attempts");
		SEND_ERROR(cloud, CLOUD_EVT_ERROR, -ENETUNREACH);
		return;
	}

	/* The cloud will return error if cloud_wrap_connect() is called while
	 * the socket is polled on in the internal cloud thread or the
	 * cloud backend is the wrong state. We cannot treat this as an error as
	 * it is rather common that cloud_connect can be called under these
	 * conditions.
	 */
	err = cloud_wrap_connect();
	if (err) {
		LOG_DBG("cloud_connect failed, error: %d", err);
	}

	connect_retries++;

	LOG_DBG("Cloud connection establishment in progress");
	LOG_DBG("New connection attempt in %d seconds if not successful",
		backoff_sec);

	/* Start timer to check connection status after backoff */
	k_work_reschedule(&connect_check_work, K_SECONDS(backoff_sec));
}

static void disconnect_cloud(void)
{
	cloud_wrap_disconnect();

	connect_retries = 0;
	qos_timer_reset();

	k_work_cancel_delayable(&connect_check_work);
}

/* Convenience function used to add messages to the QoS library. */
static void add_qos_message(uint8_t *ptr, size_t len, uint8_t type,
			    uint32_t flags, bool heap_allocated)
{
	int err;
	struct qos_data message = {
		.heap_allocated = heap_allocated,
		.data.buf = ptr,
		.data.len = len,
		.id = qos_message_id_get_next(),
		.type = type,
		.flags = flags
	};

	err = qos_message_add(&message);
	if (err == -ENOMEM) {
		LOG_WRN("Cannot add message, internal pending list is full");
	} else if (err) {
		LOG_ERR("qos_message_add, error: %d", err);
		SEND_ERROR(cloud, CLOUD_EVT_ERROR, err);
	}
}

static void qos_event_handler(const struct qos_evt *evt)
{
	switch (evt->type) {
	case QOS_EVT_MESSAGE_NEW: {
		LOG_DBG("QOS_EVT_MESSAGE_NEW");
		struct cloud_module_event *cloud_module_event = new_cloud_module_event();

		__ASSERT(cloud_module_event, "Not enough heap left to allocate event");

		cloud_module_event->type = CLOUD_EVT_DATA_SEND_QOS;
		cloud_module_event->data.message = evt->message;

		APP_EVENT_SUBMIT(cloud_module_event);
	}
		break;
	case QOS_EVT_MESSAGE_TIMER_EXPIRED: {
		LOG_DBG("QOS_EVT_MESSAGE_TIMER_EXPIRED");

		struct cloud_module_event *cloud_module_event = new_cloud_module_event();

		__ASSERT(cloud_module_event, "Not enough heap left to allocate event");

		cloud_module_event->type = CLOUD_EVT_DATA_SEND_QOS;
		cloud_module_event->data.message = evt->message;

		APP_EVENT_SUBMIT(cloud_module_event);
	}
		break;
	case QOS_EVT_MESSAGE_REMOVED_FROM_LIST:
		LOG_DBG("QOS_EVT_MESSAGE_REMOVED_FROM_LIST");

		if (evt->message.heap_allocated) {
			LOG_DBG("Freeing pointer: %p", (void *)evt->message.data.buf);
			k_free(evt->message.data.buf);
		}
		break;
	default:
		LOG_DBG("Unknown QoS handler event");
		break;
	}
}

/* If this work is executed, it means that the connection attempt was not
 * successful before the backoff timer expired. A timeout message is then
 * added to the message queue to signal the timeout.
 */
static void connect_check_work_fn(struct k_work *work)
{
	if ((state == STATE_LTE_CONNECTED && sub_state == SUB_STATE_CLOUD_CONNECTED) ||
	    (state == STATE_LTE_DISCONNECTED)) {
		return;
	}

	LOG_DBG("Cloud connection timeout occurred");

	SEND_EVENT(cloud, CLOUD_EVT_CONNECTION_TIMEOUT);
}

static int setup(void)
{
	int err;

	err = cloud_wrap_init(cloud_wrap_event_handler);
	if (err) {
		LOG_ERR("cloud_wrap_init, error: %d", err);
		return err;
	}

	err = qos_init(qos_event_handler);
	if (err) {
		LOG_ERR("qos_init, error: %d", err);
		return err;
	}

#if defined(CONFIG_MCUBOOT_IMG_MANAGER)
	/* After a successful initializaton, tell the bootloader that the
	 * current image is confirmed to be working.
	 */
	boot_write_img_confirmed();
#endif /* CONFIG_MCUBOOT_IMG_MANAGER */

	return 0;
}

/* Message handler for STATE_LTE_INIT. */
static void on_state_init(struct cloud_msg_data *msg)
{
	if ((IS_EVENT(msg, modem, MODEM_EVT_INITIALIZED)) ||
	    (IS_EVENT(msg, debug, DEBUG_EVT_EMULATOR_INITIALIZED))) {
		int err;

		state_set(STATE_LTE_DISCONNECTED);

		err = setup();
		__ASSERT(err == 0, "setp() failed");
	}
}

/* Message handler for STATE_LTE_CONNECTED. */
static void on_state_lte_connected(struct cloud_msg_data *msg)
{
	if (IS_EVENT(msg, modem, MODEM_EVT_LTE_DISCONNECTED)) {
		sub_state_set(SUB_STATE_CLOUD_DISCONNECTED);
		state_set(STATE_LTE_DISCONNECTED);

		/* Explicitly disconnect cloud when you receive an LTE disconnected event.
		 * This is to clear up the cloud library state.
		 */
		disconnect_cloud();
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_CARRIER_FOTA_PENDING)) {
		sub_state_set(SUB_STATE_CLOUD_DISCONNECTED);
		disconnect_cloud();
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_CARRIER_FOTA_STOPPED)) {
		connect_cloud();
	}
}

/* Message handler for STATE_LTE_DISCONNECTED. */
static void on_state_lte_disconnected(struct cloud_msg_data *msg)
{
	if ((IS_EVENT(msg, modem, MODEM_EVT_LTE_CONNECTED)) ||
	    (IS_EVENT(msg, debug, DEBUG_EVT_EMULATOR_NETWORK_CONNECTED))) {
		state_set(STATE_LTE_CONNECTED);

		/* LTE is now connected, cloud connection can be attempted */
		connect_cloud();
	}

#if defined(CONFIG_NRF_CLOUD_AGNSS)
	if (IS_EVENT(msg, location, LOCATION_MODULE_EVT_AGNSS_NEEDED)) {
		LOG_DBG("A-GNSS request buffered");
		agnss_request_buffered = true;
		agnss_request_buffer = msg->module.location.data.agnss_request;
		return;
	}
#endif
}

/* Message handler for SUB_STATE_CLOUD_CONNECTED. */
static void on_sub_state_cloud_connected(struct cloud_msg_data *msg)
{
	int err = 0;

	if (IS_EVENT(msg, cloud, CLOUD_EVT_DISCONNECTED)) {
		sub_state_set(SUB_STATE_CLOUD_DISCONNECTED);

		k_work_reschedule(&connect_check_work, K_SECONDS(1));

		/* Reset QoS timer. Will be restarted upon a successful call to qos_message_add() */
		qos_timer_reset();
		return;
	}

	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_GET)) {
		/* The device will get its configuration if it has changed, for every update.
		 * Due to this we don't use the QoS library.
		 */
		err = cloud_wrap_state_get(false, 0);
		if (err == -ENOTSUP) {
			LOG_DBG("Requesting of device configuration is not supported");
		} else if (err) {
			LOG_ERR("cloud_wrap_state_get, err: %d", err);
		} else {
			LOG_DBG("Device configuration requested");
		}
	}

	if (IS_EVENT(msg, debug, DEBUG_EVT_MEMFAULT_DATA_READY)) {
		add_qos_message(msg->module.debug.data.memfault.buf,
				msg->module.debug.data.memfault.len,
				MEMFAULT,
				QOS_FLAG_RELIABILITY_ACK_REQUIRED,
				true);
	}

	if (IS_EVENT(msg, data, DATA_EVT_DATA_SEND)) {

		if (IS_ENABLED(CONFIG_LWM2M_INTEGRATION)) {

			struct lwm2m_obj_path paths[CONFIG_CLOUD_CODEC_LWM2M_PATH_LIST_ENTRIES_MAX];

			__ASSERT(ARRAY_SIZE(paths) ==
				 ARRAY_SIZE(msg->module.data.data.buffer.paths),
				 "Path object list not the same size");

			for (int i = 0; i < ARRAY_SIZE(paths); i++) {
				paths[i] = msg->module.data.data.buffer.paths[i];
			}

			err = cloud_wrap_data_send(NULL,
						   msg->module.data.data.buffer.valid_object_paths,
						   true,
						   0,
						   paths);
			if (err) {
				LOG_ERR("cloud_wrap_data_send, err: %d", err);
			}

			return;
		}

		add_qos_message(msg->module.data.data.buffer.buf,
				msg->module.data.data.buffer.len,
				GENERIC,
				QOS_FLAG_RELIABILITY_ACK_DISABLED,
				true);
	}

	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_SEND)) {
		add_qos_message(msg->module.data.data.buffer.buf,
				msg->module.data.data.buffer.len,
				CONFIG,
				QOS_FLAG_RELIABILITY_ACK_REQUIRED,
				true);
	}

	if (IS_EVENT(msg, data, DATA_EVT_DATA_SEND_BATCH)) {
		add_qos_message(msg->module.data.data.buffer.buf,
				msg->module.data.data.buffer.len,
				BATCH,
				QOS_FLAG_RELIABILITY_ACK_REQUIRED,
				true);
	}

	if ((IS_EVENT(msg, data, DATA_EVT_UI_DATA_SEND)) ||
	    (IS_EVENT(msg, data, DATA_EVT_IMPACT_DATA_SEND))) {

		if (IS_ENABLED(CONFIG_LWM2M_INTEGRATION)) {

			struct lwm2m_obj_path paths[CONFIG_CLOUD_CODEC_LWM2M_PATH_LIST_ENTRIES_MAX];

			__ASSERT(ARRAY_SIZE(paths) ==
				 ARRAY_SIZE(msg->module.data.data.buffer.paths),
				 "Path object list not the same size");

			for (int i = 0; i < ARRAY_SIZE(paths); i++) {
				paths[i] = msg->module.data.data.buffer.paths[i];
			}

			err = cloud_wrap_ui_send(NULL,
						 msg->module.data.data.buffer.valid_object_paths,
						 true,
						 0,
						 paths);
			if (err) {
				LOG_ERR("cloud_wrap_ui_send, err: %d", err);
			}

			return;
		}

		add_qos_message(msg->module.data.data.buffer.buf,
				msg->module.data.data.buffer.len,
				UI,
				QOS_FLAG_RELIABILITY_ACK_REQUIRED,
				true);
	}

	if (IS_EVENT(msg, data, DATA_EVT_CLOUD_LOCATION_DATA_SEND)) {

		if (IS_ENABLED(CONFIG_LWM2M_INTEGRATION)) {
			err = cloud_wrap_cloud_location_send(NULL, 0, true, 0);
			if (err) {
				LOG_ERR("cloud_wrap_cloud_location_send, err: %d", err);
			}

			return;
		}

		add_qos_message(msg->module.data.data.buffer.buf,
				msg->module.data.data.buffer.len,
				CLOUD_LOCATION,
				QOS_FLAG_RELIABILITY_ACK_REQUIRED,
				true);

		/* Check if the configured cloud service will return the resolved location back
		 * to the device. If it does not, indicate that location result is unknown.
		 */
		if (!cloud_wrap_cloud_location_response_wait()) {
			struct cloud_module_event *cloud_module_event = new_cloud_module_event();

			__ASSERT(cloud_module_event, "Not enough heap left to allocate event");
			cloud_module_event->type = CLOUD_EVT_CLOUD_LOCATION_UNKNOWN;
			APP_EVENT_SUBMIT(cloud_module_event);
		}
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_DATA_SEND_QOS)) {

		if (IS_ENABLED(CONFIG_LWM2M_INTEGRATION)) {
			return;
		}

		bool ack = qos_message_has_flag(&msg->module.cloud.data.message,
						QOS_FLAG_RELIABILITY_ACK_REQUIRED);

		qos_message_print(&msg->module.cloud.data.message);

		struct qos_payload *message = &msg->module.cloud.data.message.data;

		switch (msg->module.cloud.data.message.type) {
		case GENERIC:
			err = cloud_wrap_data_send(message->buf,
						   message->len,
						   ack,
						   msg->module.cloud.data.message.id,
						   NULL);
			if (err) {
				LOG_WRN("cloud_wrap_data_send, err: %d", err);
			}
			break;
		case BATCH:
			err = cloud_wrap_batch_send(message->buf,
						    message->len,
						    ack,
						    msg->module.cloud.data.message.id);
			if (err) {
				LOG_WRN("cloud_wrap_batch_send, err: %d", err);
			}
			break;
		case UI:
			err = cloud_wrap_ui_send(message->buf,
						 message->len,
						 ack,
						 msg->module.cloud.data.message.id,
						 NULL);
			if (err) {
				LOG_WRN("cloud_wrap_ui_send, err: %d", err);
			}
			break;
		case CLOUD_LOCATION:
			err = cloud_wrap_cloud_location_send(message->buf,
							message->len,
							ack,
							msg->module.cloud.data.message.id);
			if (err) {
				LOG_WRN("cloud_wrap_cloud_location_send, err: %d", err);
			}
			break;
		case AGNSS_REQUEST:
			err = cloud_wrap_agnss_request_send(message->buf,
							    message->len,
							    ack,
							    msg->module.cloud.data.message.id);
			if (err) {
				LOG_WRN("cloud_wrap_agnss_request_send, err: %d", err);
			}
			break;
		case PGPS_REQUEST:
			err = cloud_wrap_pgps_request_send(message->buf,
							   message->len,
							   ack,
							   msg->module.cloud.data.message.id);
			if (err) {
				LOG_WRN("cloud_wrap_pgps_request_send, err: %d", err);
			}
			break;
		case CONFIG:
			err = cloud_wrap_state_send(message->buf,
						    message->len,
						    ack,
						    msg->module.cloud.data.message.id);
			if (err) {
				LOG_WRN("cloud_wrap_state_send, err: %d", err);
			}
			break;
		case MEMFAULT:
			err = cloud_wrap_memfault_data_send(message->buf,
							    message->len,
							    ack,
							    msg->module.cloud.data.message.id);
			if (err) {
				LOG_WRN("cloud_wrap_memfault_data_send, err: %d", err);
			}
			break;
		default:
			LOG_ERR("Unknown data type");
			break;
		}
	}

#if defined(CONFIG_NRF_CLOUD_AGNSS)
	if (IS_EVENT(msg, location, LOCATION_MODULE_EVT_AGNSS_NEEDED)) {
		agnss_request_handle(&msg->module.location.data.agnss_request);
		return;
	}
#endif

#if defined(CONFIG_NRF_CLOUD_PGPS)
	if (IS_EVENT(msg, location, LOCATION_MODULE_EVT_PGPS_NEEDED)) {
		pgps_request_handle(&msg->module.location.data.pgps_request);
		return;
	}
#endif
}

/* Message handler for SUB_STATE_CLOUD_DISCONNECTED. */
static void on_sub_state_cloud_disconnected(struct cloud_msg_data *msg)
{
	if (IS_EVENT(msg, cloud, CLOUD_EVT_CONNECTED)) {
		sub_state_set(SUB_STATE_CLOUD_CONNECTED);

		connect_retries = 0;
		k_work_cancel_delayable(&connect_check_work);

#if defined(CONFIG_NRF_CLOUD_AGNSS)
		if (agnss_request_buffered) {
			LOG_DBG("Handle buffered A-GNSS request");
			agnss_request_handle(&agnss_request_buffer);
			agnss_request_buffered = false;
		}
#endif
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_CONNECTION_TIMEOUT)) {
		connect_cloud();
	}

	/* The initial device configuration acknowledgment typically occurs before
	 * the nRF Cloud library has notified that it has established a full connection to
	 * the cloud service. Due to this we allow data to be sent
	 * in the SUB_STATE_CLOUD_DISCONNECTED state.
	 */
	if (IS_EVENT(msg, data, DATA_EVT_CONFIG_SEND) &&
	    IS_ENABLED(CONFIG_NRF_CLOUD_MQTT)) {
		add_qos_message(msg->module.data.data.buffer.buf,
				msg->module.data.data.buffer.len,
				CONFIG,
				QOS_FLAG_RELIABILITY_ACK_REQUIRED,
				true);
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_DATA_SEND_QOS) &&
	    IS_ENABLED(CONFIG_NRF_CLOUD_MQTT)) {
		bool ack = qos_message_has_flag(&msg->module.cloud.data.message,
						QOS_FLAG_RELIABILITY_ACK_REQUIRED);

		qos_message_print(&msg->module.cloud.data.message);

		struct qos_payload *message = &msg->module.cloud.data.message.data;

		switch (msg->module.cloud.data.message.type) {
		case CONFIG: {
			int err = cloud_wrap_state_send(message->buf, message->len, ack,
							msg->module.cloud.data.message.id);

			if (err) {
				LOG_WRN("cloud_wrap_state_send, err: %d", err);
			}
		}
			break;
		default:
			LOG_ERR("Unknown data type");
			break;
		}
	}

#if defined(CONFIG_NRF_CLOUD_AGNSS)
	if (IS_EVENT(msg, location, LOCATION_MODULE_EVT_AGNSS_NEEDED)) {
		LOG_DBG("A-GNSS request buffered");
		agnss_request_buffered = true;
		agnss_request_buffer = msg->module.location.data.agnss_request;
		return;
	}
#endif
}

/* Message handler for all states. */
static void on_all_states(struct cloud_msg_data *msg)
{
	if (IS_EVENT(msg, util, UTIL_EVT_SHUTDOWN_REQUEST)) {
		/* The module doesn't have anything to shut down and can
		 * report back immediately.
		 */
		SEND_SHUTDOWN_ACK(cloud, CLOUD_EVT_SHUTDOWN_READY, self.id);
		state_set(STATE_SHUTDOWN);
	}

	if (is_data_module_event(&msg->module.data.header)) {
		switch (msg->module.data.type) {
		case DATA_EVT_CONFIG_INIT:
			/* Fall through. */
		case DATA_EVT_CONFIG_READY:
			copy_cfg = msg->module.data.data.cfg;
			break;
		default:
			break;
		}
	}
}

static void module_thread_fn(void)
{
	int err;
	struct cloud_msg_data msg = { 0 };

	self.thread_id = k_current_get();

	err = module_start(&self);
	if (err) {
		LOG_ERR("Failed starting module, error: %d", err);
		SEND_ERROR(cloud, CLOUD_EVT_ERROR, err);
	}

	state_set(STATE_LTE_INIT);
	sub_state_set(SUB_STATE_CLOUD_DISCONNECTED);

	k_work_init_delayable(&connect_check_work, connect_check_work_fn);

	while (true) {
		module_get_next_msg(&self, &msg);

		switch (state) {
		case STATE_LTE_INIT:
			on_state_init(&msg);
			break;
		case STATE_LTE_CONNECTED:
			switch (sub_state) {
			case SUB_STATE_CLOUD_CONNECTED:
				on_sub_state_cloud_connected(&msg);
				break;
			case SUB_STATE_CLOUD_DISCONNECTED:
				on_sub_state_cloud_disconnected(&msg);
				break;
			default:
				LOG_ERR("Unknown sub state");
				break;
			}

			on_state_lte_connected(&msg);
			break;
		case STATE_LTE_DISCONNECTED:
			on_state_lte_disconnected(&msg);
			break;
		case STATE_SHUTDOWN:
			/* The shutdown state has no transition. */
			break;
		default:
			LOG_ERR("Unknown state.");
			break;
		}

		on_all_states(&msg);
	}
}

K_THREAD_DEFINE(cloud_module_thread, CONFIG_CLOUD_THREAD_STACK_SIZE,
		module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, data_module_event);
APP_EVENT_SUBSCRIBE(MODULE, app_module_event);
APP_EVENT_SUBSCRIBE(MODULE, modem_module_event);
APP_EVENT_SUBSCRIBE_FIRST(MODULE, cloud_module_event);
APP_EVENT_SUBSCRIBE(MODULE, location_module_event);
APP_EVENT_SUBSCRIBE(MODULE, debug_module_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, util_module_event);
