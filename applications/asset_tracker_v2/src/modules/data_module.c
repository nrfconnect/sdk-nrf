/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <event_manager.h>
#include <settings/settings.h>
#include <date_time.h>

#include "cloud/cloud_codec/cloud_codec.h"

#define MODULE data_module

#include "modules_common.h"
#include "events/app_module_event.h"
#include "events/cloud_module_event.h"
#include "events/data_module_event.h"
#include "events/gps_module_event.h"
#include "events/modem_module_event.h"
#include "events/sensor_module_event.h"
#include "events/ui_module_event.h"
#include "events/util_module_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DATA_MODULE_LOG_LEVEL);

#define DEVICE_SETTINGS_KEY			"data_module"
#define DEVICE_SETTINGS_CONFIG_KEY		"config"

/* Value that is used to limit the maximum allowed device configuration value
 * for the accelerometer threshold. 100 m/s2 ~ 10.2g.
 */
#define ACCELEROMETER_S_M2_MAX 100

struct data_msg_data {
	union {
		struct modem_module_event modem;
		struct cloud_module_event cloud;
		struct gps_module_event gps;
		struct ui_module_event ui;
		struct sensor_module_event sensor;
		struct data_module_event data;
		struct app_module_event app;
		struct util_module_event util;
	} module;
};

/* Data module super states. */
static enum state_type {
	STATE_CLOUD_DISCONNECTED,
	STATE_CLOUD_CONNECTED,
	STATE_SHUTDOWN
} state;

/* Ringbuffers. All data received by the Data module are stored in ringbuffers.
 * Upon a LTE connection loss the device will keep sampling/storing data in
 * the buffers, and empty the buffers in batches upon a reconnect.
 */
static struct cloud_data_gps gps_buf[CONFIG_DATA_GPS_BUFFER_COUNT];
static struct cloud_data_sensors sensors_buf[CONFIG_DATA_SENSOR_BUFFER_COUNT];
static struct cloud_data_ui ui_buf[CONFIG_DATA_UI_BUFFER_COUNT];
static struct cloud_data_accelerometer accel_buf[CONFIG_DATA_ACCELEROMETER_BUFFER_COUNT];
static struct cloud_data_battery bat_buf[CONFIG_DATA_BATTERY_BUFFER_COUNT];
static struct cloud_data_modem_dynamic modem_dyn_buf[CONFIG_DATA_MODEM_DYNAMIC_BUFFER_COUNT];
static struct cloud_data_neighbor_cells neighbor_cells;

/* Static modem data does not change between firmware versions and does not
 * have to be buffered.
 */
static struct cloud_data_modem_static modem_stat;

/* Head of ringbuffers. */
static int head_gps_buf;
static int head_sensor_buf;
static int head_modem_dyn_buf;
static int head_ui_buf;
static int head_accel_buf;
static int head_bat_buf;

/* Default device configuration. */
static struct cloud_data_cfg current_cfg = {
	.gps_timeout			= CONFIG_DATA_GPS_TIMEOUT_SECONDS,
	.active_mode			= (IS_ENABLED(CONFIG_DATA_DEVICE_MODE) ? true : false),
	.active_wait_timeout		= CONFIG_DATA_ACTIVE_TIMEOUT_SECONDS,
	.movement_resolution		= CONFIG_DATA_MOVEMENT_RESOLUTION_SECONDS,
	.movement_timeout		= CONFIG_DATA_MOVEMENT_TIMEOUT_SECONDS,
	.accelerometer_threshold	= CONFIG_DATA_ACCELEROMETER_THRESHOLD,
	.no_data.gnss			= (IS_ENABLED(CONFIG_DATA_SAMPLE_GNSS_DEFAULT)
					   ? false : true),
	.no_data.neighbor_cell		= (IS_ENABLED(CONFIG_DATA_SAMPLE_NEIGHBOR_CELLS_DEFAULT)
					   ? false : true)
};

static struct k_work_delayable data_send_work;

/* List used to keep track of responses from other modules with data that is
 * requested to be sampled/published.
 */
static enum app_module_data_type req_type_list[APP_DATA_COUNT];

/* Total number of data types requested for a particular sample/publish
 * cycle.
 */
static int recv_req_data_count;

/* Counter of data types received from other modules. When this number
 * matches the affirmed_data_type variable all requested data has been
 * received by the Data module.
 */
static int req_data_count;

/* List of data types sent out by the data module. */
enum data_type {
	UNUSED,
	GENERIC,
	BATCH,
	UI,
	NEIGHBOR_CELLS,
	CONFIG
};

struct ack_data {
	enum data_type type;
	size_t len;
	void *ptr;
};

/* Data that has been attempted to be sent but failed. */
static struct ack_data failed_data[CONFIG_FAILED_DATA_COUNT];

/* Data that has been encoded and shipped on, but has not yet been ACKed. */
static struct ack_data pending_data[CONFIG_PENDING_DATA_COUNT];

/* Data module message queue. */
#define DATA_QUEUE_ENTRY_COUNT		10
#define DATA_QUEUE_BYTE_ALIGNMENT	4

K_MSGQ_DEFINE(msgq_data, sizeof(struct data_msg_data),
	      DATA_QUEUE_ENTRY_COUNT, DATA_QUEUE_BYTE_ALIGNMENT);

static struct module_data self = {
	.name = "data",
	.msg_q = &msgq_data,
	.supports_shutdown = true,
};

/* Forward declarations */
static void data_send_work_fn(struct k_work *work);
static int config_settings_handler(const char *key, size_t len,
				   settings_read_cb read_cb, void *cb_arg);

/* Static handlers */
SETTINGS_STATIC_HANDLER_DEFINE(MODULE, DEVICE_SETTINGS_KEY, NULL,
			       config_settings_handler, NULL, NULL);

/* Convenience functions used in internal state handling. */
static char *state2str(enum state_type new_state)
{
	switch (new_state) {
	case STATE_CLOUD_DISCONNECTED:
		return "STATE_CLOUD_DISCONNECTED";
	case STATE_CLOUD_CONNECTED:
		return "STATE_CLOUD_CONNECTED";
	case STATE_SHUTDOWN:
		return "STATE_SHUTDOWN";
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

/* Handlers */
static bool event_handler(const struct event_header *eh)
{
	struct data_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_modem_module_event(eh)) {
		struct modem_module_event *event = cast_modem_module_event(eh);

		msg.module.modem = *event;
		enqueue_msg = true;
	}

	if (is_cloud_module_event(eh)) {
		struct cloud_module_event *event = cast_cloud_module_event(eh);

		msg.module.cloud = *event;
		enqueue_msg = true;
	}

	if (is_gps_module_event(eh)) {
		struct gps_module_event *event = cast_gps_module_event(eh);

		msg.module.gps = *event;
		enqueue_msg = true;
	}

	if (is_sensor_module_event(eh)) {
		struct sensor_module_event *event =
				cast_sensor_module_event(eh);

		msg.module.sensor = *event;
		enqueue_msg = true;
	}

	if (is_ui_module_event(eh)) {
		struct ui_module_event *event = cast_ui_module_event(eh);

		msg.module.ui = *event;
		enqueue_msg = true;
	}

	if (is_app_module_event(eh)) {
		struct app_module_event *event = cast_app_module_event(eh);

		msg.module.app = *event;
		enqueue_msg = true;
	}

	if (is_data_module_event(eh)) {
		struct data_module_event *event = cast_data_module_event(eh);

		msg.module.data = *event;
		enqueue_msg = true;
	}

	if (is_util_module_event(eh)) {
		struct util_module_event *event = cast_util_module_event(eh);

		msg.module.util = *event;
		enqueue_msg = true;
	}

	if (enqueue_msg) {
		int err = module_enqueue_msg(&self, &msg);

		if (err) {
			LOG_ERR("Message could not be enqueued");
			SEND_ERROR(data, DATA_EVT_ERROR, err);
		}
	}

	return false;
}

static int config_settings_handler(const char *key, size_t len,
				   settings_read_cb read_cb, void *cb_arg)
{
	int err;

	if (strcmp(key, DEVICE_SETTINGS_CONFIG_KEY) == 0) {
		err = read_cb(cb_arg, &current_cfg, sizeof(current_cfg));
		if (err < 0) {
			LOG_ERR("Failed to load configuration, error: %d", err);
			return err;
		}
	}

	LOG_DBG("Device configuration loaded from flash");

	return 0;
}

static void date_time_event_handler(const struct date_time_evt *evt)
{
	switch (evt->type) {
	case DATE_TIME_OBTAINED_MODEM:
		/* Fall through. */
	case DATE_TIME_OBTAINED_NTP:
		/* Fall through. */
	case DATE_TIME_OBTAINED_EXT: {
		SEND_EVENT(data, DATA_EVT_DATE_TIME_OBTAINED);

		/* De-register handler. At this point the application will have
		 * date time to depend on indefinitely until a reboot occurs.
		 */
		date_time_register_handler(NULL);
		break;
	}
	case DATE_TIME_NOT_OBTAINED:
		break;
	default:
		break;
	}
}

/* Static module functions. */
static void data_list_clear_entry(struct ack_data *data)
{
	data->ptr = NULL,
	data->len = 0;
	data->type = UNUSED;
}

static void data_list_clear_and_free(struct ack_data *list, size_t list_count)
{
	for (size_t i = 0; i < list_count; i++) {
		if (list[i].ptr != NULL) {
			k_free(list[i].ptr);
			data_list_clear_entry(&list[i]);
		}
	}

	LOG_WRN("Data list cleared and freed");
}

static void data_list_add_failed(void *ptr, size_t len, enum data_type type)
{
	while (true) {
		for (size_t i = 0; i < ARRAY_SIZE(failed_data); i++) {
			if (failed_data[i].ptr == NULL) {
				failed_data[i].ptr = ptr;
				failed_data[i].len = len;
				failed_data[i].type = type;

				LOG_DBG("Failed data added: %p",
					failed_data[i].ptr);
				return;
			}
		}

		LOG_WRN("Could not add data to failed data list, list is full");
		LOG_WRN("Clearing failed data list and adding data");

		data_list_clear_and_free(failed_data, ARRAY_SIZE(failed_data));
	}
}

static void data_list_add_pending(void *ptr, size_t len, enum data_type type)
{
	for (size_t i = 0; i < ARRAY_SIZE(pending_data); i++) {
		if (pending_data[i].ptr == NULL) {
			pending_data[i].ptr = ptr;
			pending_data[i].len = len;
			pending_data[i].type = type;

			LOG_DBG("Pending data added: %p", pending_data[i].ptr);
			return;
		}
	}

	LOG_ERR("Could not add data to pending data list, list is full");
	SEND_ERROR(data, DATA_EVT_ERROR, -ENFILE);
}

static void data_resend(void)
{
	struct data_module_event *evt;

	for (size_t i = 0; i < ARRAY_SIZE(failed_data); i++) {
		if (failed_data[i].ptr != NULL) {

			evt = new_data_module_event();

			switch (failed_data[i].type) {
			case GENERIC:
				evt->type = DATA_EVT_DATA_SEND;
				break;
			case BATCH:
				evt->type = DATA_EVT_DATA_SEND_BATCH;
				break;
			case CONFIG:
				evt->type = DATA_EVT_CONFIG_SEND;
				break;
			case UI:
				evt->type = DATA_EVT_UI_DATA_SEND;
				break;
			case NEIGHBOR_CELLS:
				evt->type = DATA_EVT_NEIGHBOR_CELLS_DATA_SEND;
			default:
				LOG_WRN("Unknown associated data type");
				SEND_ERROR(data, DATA_EVT_ERROR, -ENODATA);
				return;
			}

			evt->data.buffer.buf = failed_data[i].ptr;
			evt->data.buffer.len = failed_data[i].len;
			LOG_WRN("Resending data: %.*s", failed_data[i].len,
				log_strdup(failed_data[i].ptr));
			EVENT_SUBMIT(evt);

			/* Move data from failed to pending data list after
			 * resend.
			 */

			/* Add entry to pending data list. */
			data_list_add_pending(failed_data[i].ptr,
					      failed_data[i].len,
					      failed_data[i].type);

			/* Remove entry from failed data list. */
			data_list_clear_entry(&failed_data[i]);
		}
	}
}

static void data_ack(void *ptr, bool sent)
{
	/* Move data from pending to failed data list if incoming data is
	 * flagged as not sent. If data is flagged as sent, free entry in
	 * pending data list.
	 */

	for (size_t i = 0; i < ARRAY_SIZE(pending_data); i++) {
		if (pending_data[i].ptr == ptr) {
			if (sent) {
				k_free(ptr);
				LOG_DBG("Pending data ACKed: %p",
					pending_data[i].ptr);
			} else {
				LOG_DBG("Moving %p data from pending to failed",
					pending_data[i].ptr);

				/* Add data to failed data list. */
				data_list_add_failed(pending_data[i].ptr,
						     pending_data[i].len,
						     pending_data[i].type);
			}
			data_list_clear_entry(&pending_data[i]);
			return;
		}
	}

	LOG_ERR("No matching pointer was found in pending data list");
	SEND_ERROR(data, DATA_EVT_ERROR, -ENOTDIR);
}

static int save_config(const void *buf, size_t buf_len)
{
	int err;

	err = settings_save_one(DEVICE_SETTINGS_KEY "/"
				DEVICE_SETTINGS_CONFIG_KEY,
				buf, buf_len);
	if (err) {
		LOG_WRN("settings_save_one, error: %d", err);
		return err;
	}

	LOG_DBG("Device configuration stored to flash");

	return 0;
}

static int setup(void)
{
	int err;

	cloud_codec_init();

	err = settings_subsys_init();
	if (err) {
		LOG_ERR("settings_subsys_init, error: %d", err);
		return err;
	}

	err = settings_load_subtree(DEVICE_SETTINGS_KEY);
	if (err) {
		LOG_ERR("settings_load_subtree, error: %d", err);
		return err;
	}

	return 0;
}

static void config_distribute(enum data_module_event_type type)
{
	struct data_module_event *data_module_event = new_data_module_event();

	data_module_event->type = type;
	data_module_event->data.cfg = current_cfg;

	EVENT_SUBMIT(data_module_event);
}

static void config_status_set_all(bool fresh)
{
	current_cfg.active_mode_fresh = fresh;
	current_cfg.gps_timeout_fresh = fresh;
	current_cfg.active_wait_timeout_fresh = fresh;
	current_cfg.movement_resolution_fresh = fresh;
	current_cfg.movement_timeout_fresh = fresh;
	current_cfg.accelerometer_threshold_fresh = fresh;
	current_cfg.nod_list_fresh = fresh;
}

static void data_send(enum data_module_event_type event,
		      enum data_type type,
		      struct cloud_codec_data *data)
{
	struct data_module_event *module_event = new_data_module_event();

	module_event->type = event;
	module_event->data.buffer.buf = data->buf;
	module_event->data.buffer.len = data->len;

	data_list_add_pending(data->buf, data->len, type);
	EVENT_SUBMIT(module_event);

	/* Reset buffer */
	data->buf = NULL;
	data->len = 0;
}

/* This function allocates buffer on the heap, which needs to be freed after use. */
static void data_encode(void)
{
	int err;
	struct cloud_codec_data codec = {0};

	if (!date_time_is_valid()) {
		/* Date time library does not have valid time to
		 * timestamp cloud data. Abort cloud publicaton. Data will
		 * be cached in it respective ringbuffer.
		 */
		return;
	}

	err = cloud_codec_encode_neighbor_cells(&codec, &neighbor_cells);
	switch (err) {
	case 0:
		LOG_DBG("Neighbor cell data encoded successfully");
		data_send(DATA_EVT_NEIGHBOR_CELLS_DATA_SEND, NEIGHBOR_CELLS, &codec);
		break;
	case -ENOTSUP:
		/* Neighbor cell data encoding not supported */
		break;
	case -ENODATA:
		LOG_DBG("No neighbor cells data to encode, error: %d", err);
		break;
	default:
		LOG_ERR("Error encoding neighbor cells data: %d", err);
		SEND_ERROR(data, DATA_EVT_ERROR, err);
		return;
	}

	err = cloud_codec_encode_data(
		&codec,
		&gps_buf[head_gps_buf],
		&sensors_buf[head_sensor_buf],
		&modem_stat,
		&modem_dyn_buf[head_modem_dyn_buf],
		&ui_buf[head_ui_buf],
		&accel_buf[head_accel_buf],
		&bat_buf[head_bat_buf]);
	switch (err) {
	case 0:
		LOG_DBG("Data encoded successfully");
		data_send(DATA_EVT_DATA_SEND, GENERIC, &codec);
		break;
	case -ENODATA:
		/* This error might occur when data has not been obtained prior
		 * to data encoding.
		 */
		LOG_DBG("No new data to encode");
		break;
	default:
		LOG_ERR("Error encoding message %d", err);
		SEND_ERROR(data, DATA_EVT_ERROR, err);
		return;
	}

	err = cloud_codec_encode_batch_data(&codec,
					gps_buf,
					sensors_buf,
					modem_dyn_buf,
					ui_buf,
					accel_buf,
					bat_buf,
					ARRAY_SIZE(gps_buf),
					ARRAY_SIZE(sensors_buf),
					ARRAY_SIZE(modem_dyn_buf),
					ARRAY_SIZE(ui_buf),
					ARRAY_SIZE(accel_buf),
					ARRAY_SIZE(bat_buf));
	switch (err) {
	case 0:
		LOG_DBG("Batch data encoded successfully");
		data_send(DATA_EVT_DATA_SEND_BATCH, BATCH, &codec);
		break;
	case -ENODATA:
		LOG_DBG("No batch data to encode, ringbuffers are empty");
		break;
	default:
		LOG_ERR("Error batch-enconding data: %d", err);
		SEND_ERROR(data, DATA_EVT_ERROR, err);
		return;
	}
}

static void config_get(void)
{
	SEND_EVENT(data, DATA_EVT_CONFIG_GET);
}

static void config_send(bool send_all)
{
	int err;
	struct cloud_codec_data codec;
	struct data_module_event *evt;

	if (send_all || IS_ENABLED(CONFIG_DATA_SEND_ALL_DEVICE_CONFIGURATIONS)) {
		config_status_set_all(true);
	}

	err = cloud_codec_encode_config(&codec, &current_cfg);
	if (err) {
		LOG_ERR("Error encoding configuration, error: %d", err);
		SEND_ERROR(data, DATA_EVT_ERROR, err);
		goto exit;
	}

	evt = new_data_module_event();
	evt->type = DATA_EVT_CONFIG_SEND;
	evt->data.buffer.buf = codec.buf;
	evt->data.buffer.len = codec.len;

	data_list_add_pending(codec.buf, codec.len, CONFIG);
	EVENT_SUBMIT(evt);

exit:
	/* Reset the status flag of all configuations after the data has been sent. */
	config_status_set_all(false);
}

static void data_ui_send(void)
{
	int err;
	struct data_module_event *evt;
	struct cloud_codec_data codec = {0};

	if (!date_time_is_valid()) {
		/* Date time library does not have valid time to
		 * timestamp cloud data. Abort cloud publicaton. Data will
		 * be cached in it respective ringbuffer.
		 */
		return;
	}

	err = cloud_codec_encode_ui_data(&codec, &ui_buf[head_ui_buf]);
	if (err == -ENODATA) {
		LOG_DBG("No new UI data to encode, error: %d", err);
		return;
	} else if (err) {
		LOG_ERR("Encoding button press, error: %d", err);
		SEND_ERROR(data, DATA_EVT_ERROR, err);
		return;
	}

	evt = new_data_module_event();
	evt->type = DATA_EVT_UI_DATA_SEND;
	evt->data.buffer.buf = codec.buf;
	evt->data.buffer.len = codec.len;

	data_list_add_pending(codec.buf, codec.len, UI);

	EVENT_SUBMIT(evt);
}

static void requested_data_clear(void)
{
	recv_req_data_count = 0;
	req_data_count = 0;
}

static void data_send_work_fn(struct k_work *work)
{
	SEND_EVENT(data, DATA_EVT_DATA_READY);

	requested_data_clear();
	k_work_cancel_delayable(&data_send_work);
}

static void requested_data_status_set(enum app_module_data_type data_type)
{
	for (size_t i = 0; i < recv_req_data_count; i++) {
		if (req_type_list[i] == data_type) {
			req_data_count++;
			break;
		}
	}

	if (req_data_count == recv_req_data_count) {
		data_send_work_fn(NULL);
	}
}

static void requested_data_list_set(enum app_module_data_type *data_list,
				    size_t count)
{
	if ((count == 0) || (count > APP_DATA_COUNT)) {
		LOG_ERR("Invalid data type list length");
		return;
	}

	requested_data_clear();

	for (size_t i = 0; i < count; i++) {
		req_type_list[i] = data_list[i];
	}

	recv_req_data_count = count;
}

static void new_config_handle(struct cloud_data_cfg *new_config)
{
	bool config_change = false;

	/* Guards making sure that only new configuration values are applied. */
	if (current_cfg.active_mode != new_config->active_mode) {
		current_cfg.active_mode = new_config->active_mode;

		if (current_cfg.active_mode) {
			LOG_WRN("New Device mode: Active");
		} else {
			LOG_WRN("New Device mode: Passive");
		}

		config_change = true;
		current_cfg.active_mode_fresh = true;
	}

	if (current_cfg.no_data.gnss != new_config->no_data.gnss) {
		current_cfg.no_data.gnss = new_config->no_data.gnss;

		if (!current_cfg.no_data.gnss) {
			LOG_WRN("Requesting of GNSS data is enabled");
		} else {
			LOG_WRN("Requesting of GNSS data is disabled");
		}

		config_change = true;
		current_cfg.nod_list_fresh = true;
	}

	if (current_cfg.no_data.neighbor_cell != new_config->no_data.neighbor_cell) {
		current_cfg.no_data.neighbor_cell = new_config->no_data.neighbor_cell;

		if (!current_cfg.no_data.neighbor_cell) {
			LOG_WRN("Requesting of neighbor cell data is enabled");
		} else {
			LOG_WRN("Requesting of neighbor cell data is disabled");
		}

		config_change = true;
		current_cfg.nod_list_fresh = true;
	}

	if (new_config->gps_timeout > 0) {
		if (current_cfg.gps_timeout != new_config->gps_timeout) {
			current_cfg.gps_timeout = new_config->gps_timeout;

			LOG_WRN("New GPS timeout: %d", current_cfg.gps_timeout);

			config_change = true;
			current_cfg.gps_timeout_fresh = true;
		}
	} else {
		LOG_ERR("New GPS timeout out of range: %d", new_config->gps_timeout);
	}

	if (new_config->active_wait_timeout > 0) {
		if (current_cfg.active_wait_timeout != new_config->active_wait_timeout) {
			current_cfg.active_wait_timeout = new_config->active_wait_timeout;

			LOG_WRN("New Active wait timeout: %d", current_cfg.active_wait_timeout);

			config_change = true;
			current_cfg.active_wait_timeout_fresh = true;
		}
	} else {
		LOG_ERR("New Active timeout out of range: %d", new_config->active_wait_timeout);
	}

	if (new_config->movement_resolution > 0) {
		if (current_cfg.movement_resolution != new_config->movement_resolution) {
			current_cfg.movement_resolution = new_config->movement_resolution;

			LOG_WRN("New Movement resolution: %d", current_cfg.movement_resolution);

			config_change = true;
			current_cfg.movement_resolution_fresh = true;
		}
	} else {
		LOG_ERR("New Movement resolution out of range: %d",
			new_config->movement_resolution);
	}

	if (new_config->movement_timeout > 0) {
		if (current_cfg.movement_timeout != new_config->movement_timeout) {
			current_cfg.movement_timeout = new_config->movement_timeout;

			LOG_WRN("New Movement timeout: %d", current_cfg.movement_timeout);

			config_change = true;
			current_cfg.movement_timeout_fresh = true;
		}
	} else {
		LOG_ERR("New Movement timeout out of range: %d", new_config->movement_timeout);
	}

	if ((new_config->accelerometer_threshold < ACCELEROMETER_S_M2_MAX) &&
	    (new_config->accelerometer_threshold > 0)) {
		if (current_cfg.accelerometer_threshold != new_config->accelerometer_threshold) {
			current_cfg.accelerometer_threshold = new_config->accelerometer_threshold;

			LOG_WRN("New Accelerometer threshold: %f",
				current_cfg.accelerometer_threshold);

			config_change = true;
			current_cfg.accelerometer_threshold_fresh = true;
		}
	} else {
		LOG_ERR("New Accelerometer threshold out of range: %f",
			new_config->accelerometer_threshold);
	}

	/* If there has been a change in the currently applied device configuration we want to store
	 * the configuration to flash, distribute it to other modules and acknowledge the change
	 * back to cloud.
	 */
	if (config_change) {
		int err = save_config(&current_cfg, sizeof(current_cfg));

		if (err) {
			LOG_WRN("Configuration not stored, error: %d", err);
		}

		config_distribute(DATA_EVT_CONFIG_READY);
		config_send(false);
		return;
	}

	LOG_DBG("No new values in incoming device configuration update message");
}

/* Message handler for STATE_CLOUD_DISCONNECTED. */
static void on_cloud_state_disconnected(struct data_msg_data *msg)
{
	if (IS_EVENT(msg, cloud, CLOUD_EVT_CONNECTED)) {
		date_time_update_async(date_time_event_handler);
		state_set(STATE_CLOUD_CONNECTED);
	}
}

/* Message handler for STATE_CLOUD_CONNECTED. */
static void on_cloud_state_connected(struct data_msg_data *msg)
{
	if (IS_EVENT(msg, data, DATA_EVT_DATA_READY)) {
		/* Resend data previously failed to be sent. */
		data_resend();
		data_encode();
		return;
	}

	if (IS_EVENT(msg, app, APP_EVT_CONFIG_GET)) {
		config_get();
		return;
	}

	if (IS_EVENT(msg, data, DATA_EVT_UI_DATA_READY)) {
		data_ui_send();
		return;
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_DISCONNECTED)) {
		state_set(STATE_CLOUD_DISCONNECTED);
		return;
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_CONFIG_EMPTY)) {
		config_send(true);
		return;
	}
}

/* Message handler for all states. */
static void on_all_states(struct data_msg_data *msg)
{
	/* Distribute new configuration received from cloud. */
	if (IS_EVENT(msg, cloud, CLOUD_EVT_CONFIG_RECEIVED)) {
		struct cloud_data_cfg new = {
			.active_mode =
				msg->module.cloud.data.config.active_mode,
			.active_wait_timeout =
				msg->module.cloud.data.config.active_wait_timeout,
			.movement_resolution =
				msg->module.cloud.data.config.movement_resolution,
			.movement_timeout =
				msg->module.cloud.data.config.movement_timeout,
			.gps_timeout =
				msg->module.cloud.data.config.gps_timeout,
			.accelerometer_threshold =
				msg->module.cloud.data.config.accelerometer_threshold,
			.no_data.gnss =
				msg->module.cloud.data.config.no_data.gnss,
			.no_data.neighbor_cell =
				msg->module.cloud.data.config.no_data.neighbor_cell
		};

		new_config_handle(&new);
		return;
	}

	if (IS_EVENT(msg, app, APP_EVT_START)) {
		config_distribute(DATA_EVT_CONFIG_INIT);
	}

	if (IS_EVENT(msg, util, UTIL_EVT_SHUTDOWN_REQUEST)) {
		/* The module doesn't have anything to shut down and can
		 * report back immediately.
		 */
		SEND_SHUTDOWN_ACK(data, DATA_EVT_SHUTDOWN_READY, self.id);
		state_set(STATE_SHUTDOWN);
	}

	if (IS_EVENT(msg, app, APP_EVT_DATA_GET)) {
		/* Store which data is requested by the app, later to be used
		 * to confirm data is reported to the data manger.
		 */
		requested_data_list_set(msg->module.app.data_list,
					msg->module.app.count);

		/* Start countdown until data must have been received by the
		 * Data module in order to be sent to cloud
		 */
		k_work_reschedule(&data_send_work,
				      K_SECONDS(msg->module.app.timeout));

		return;
	}

	if (IS_EVENT(msg, ui, UI_EVT_BUTTON_DATA_READY)) {
		struct cloud_data_ui new_ui_data = {
			.btn = msg->module.ui.data.ui.button_number,
			.btn_ts = msg->module.ui.data.ui.timestamp,
			.queued = true
		};

		cloud_codec_populate_ui_buffer(ui_buf, &new_ui_data,
					       &head_ui_buf,
					       ARRAY_SIZE(ui_buf));

		SEND_EVENT(data, DATA_EVT_UI_DATA_READY);
		return;
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_MODEM_STATIC_DATA_NOT_READY)) {
		requested_data_status_set(APP_DATA_MODEM_STATIC);
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_MODEM_STATIC_DATA_READY)) {
		modem_stat.nw_lte_m = msg->module.modem.data.modem_static.nw_mode_ltem;
		modem_stat.nw_nb_iot = msg->module.modem.data.modem_static.nw_mode_nbiot;
		modem_stat.nw_gps = msg->module.modem.data.modem_static.nw_mode_gps;
		modem_stat.bnd = msg->module.modem.data.modem_static.band;
		modem_stat.ts = msg->module.modem.data.modem_static.timestamp;
		modem_stat.queued = true;

		BUILD_ASSERT(sizeof(modem_stat.appv) >=
			     sizeof(msg->module.modem.data.modem_static.app_version));

		BUILD_ASSERT(sizeof(modem_stat.brdv) >=
			     sizeof(msg->module.modem.data.modem_static.board_version));

		BUILD_ASSERT(sizeof(modem_stat.fw) >=
			     sizeof(msg->module.modem.data.modem_static.modem_fw));

		BUILD_ASSERT(sizeof(modem_stat.iccid) >=
			     sizeof(msg->module.modem.data.modem_static.iccid));

		strcpy(modem_stat.appv, msg->module.modem.data.modem_static.app_version);
		strcpy(modem_stat.brdv, msg->module.modem.data.modem_static.board_version);
		strcpy(modem_stat.fw, msg->module.modem.data.modem_static.modem_fw);
		strcpy(modem_stat.iccid, msg->module.modem.data.modem_static.iccid);

		requested_data_status_set(APP_DATA_MODEM_STATIC);
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_MODEM_DYNAMIC_DATA_NOT_READY)) {
		requested_data_status_set(APP_DATA_MODEM_DYNAMIC);
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_MODEM_DYNAMIC_DATA_READY)) {
		struct cloud_data_modem_dynamic new_modem_data = {
			.area = msg->module.modem.data.modem_dynamic.area_code,
			.cell = msg->module.modem.data.modem_dynamic.cell_id,
			.rsrp = msg->module.modem.data.modem_dynamic.rsrp,
			.ts = msg->module.modem.data.modem_dynamic.timestamp,

			.area_code_fresh = msg->module.modem.data.modem_dynamic.area_code_fresh,
			.cell_id_fresh = msg->module.modem.data.modem_dynamic.cell_id_fresh,
			.rsrp_fresh = msg->module.modem.data.modem_dynamic.rsrp_fresh,
			.ip_address_fresh = msg->module.modem.data.modem_dynamic.ip_address_fresh,
			.mccmnc_fresh = msg->module.modem.data.modem_dynamic.mccmnc_fresh,
			.queued = true
		};

		BUILD_ASSERT(sizeof(new_modem_data.ip) >=
			     sizeof(msg->module.modem.data.modem_dynamic.ip_address));

		BUILD_ASSERT(sizeof(new_modem_data.mccmnc) >=
			     sizeof(msg->module.modem.data.modem_dynamic.mccmnc));

		strcpy(new_modem_data.ip, msg->module.modem.data.modem_dynamic.ip_address);
		strcpy(new_modem_data.mccmnc, msg->module.modem.data.modem_dynamic.mccmnc);

		cloud_codec_populate_modem_dynamic_buffer(
						modem_dyn_buf,
						&new_modem_data,
						&head_modem_dyn_buf,
						ARRAY_SIZE(modem_dyn_buf));

		requested_data_status_set(APP_DATA_MODEM_DYNAMIC);
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_BATTERY_DATA_NOT_READY)) {
		requested_data_status_set(APP_DATA_BATTERY);
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_BATTERY_DATA_READY)) {
		struct cloud_data_battery new_battery_data = {
			.bat = msg->module.modem.data.bat.battery_voltage,
			.bat_ts = msg->module.modem.data.bat.timestamp,
			.queued = true
		};

		cloud_codec_populate_bat_buffer(bat_buf, &new_battery_data,
						&head_bat_buf,
						ARRAY_SIZE(bat_buf));

		requested_data_status_set(APP_DATA_BATTERY);
	}

	if (IS_EVENT(msg, sensor, SENSOR_EVT_ENVIRONMENTAL_DATA_READY)) {
		struct cloud_data_sensors new_sensor_data = {
			.temp = msg->module.sensor.data.sensors.temperature,
			.hum = msg->module.sensor.data.sensors.humidity,
			.env_ts = msg->module.sensor.data.sensors.timestamp,
			.queued = true
		};

		cloud_codec_populate_sensor_buffer(sensors_buf,
						   &new_sensor_data,
						   &head_sensor_buf,
						   ARRAY_SIZE(sensors_buf));

		requested_data_status_set(APP_DATA_ENVIRONMENTAL);
	}

	if (IS_EVENT(msg, sensor, SENSOR_EVT_ENVIRONMENTAL_NOT_SUPPORTED)) {
		requested_data_status_set(APP_DATA_ENVIRONMENTAL);
	}

	if (IS_EVENT(msg, sensor, SENSOR_EVT_MOVEMENT_DATA_READY)) {
		struct cloud_data_accelerometer new_movement_data = {
			.values[0] = msg->module.sensor.data.accel.values[0],
			.values[1] = msg->module.sensor.data.accel.values[1],
			.values[2] = msg->module.sensor.data.accel.values[2],
			.ts = msg->module.sensor.data.accel.timestamp,
			.queued = true
		};

		cloud_codec_populate_accel_buffer(accel_buf, &new_movement_data,
						  &head_accel_buf,
						  ARRAY_SIZE(accel_buf));
	}

	if (IS_EVENT(msg, gps, GPS_EVT_DATA_READY)) {
		struct cloud_data_gps new_gps_data = {
			.gps_ts = msg->module.gps.data.gps.timestamp,
			.queued = true,
			.format = msg->module.gps.data.gps.format
		};

		switch (msg->module.gps.data.gps.format) {
		case GPS_MODULE_DATA_FORMAT_PVT: {
			/* Add PVT data */
			new_gps_data.pvt.acc = msg->module.gps.data.gps.pvt.accuracy;
			new_gps_data.pvt.alt = msg->module.gps.data.gps.pvt.altitude;
			new_gps_data.pvt.hdg = msg->module.gps.data.gps.pvt.heading;
			new_gps_data.pvt.lat = msg->module.gps.data.gps.pvt.latitude;
			new_gps_data.pvt.longi = msg->module.gps.data.gps.pvt.longitude;
			new_gps_data.pvt.spd = msg->module.gps.data.gps.pvt.speed;
			new_gps_data.pvt.spd = msg->module.gps.data.gps.pvt.speed;

		};
			break;
		case GPS_MODULE_DATA_FORMAT_NMEA: {
			/* Add NMEA data */
			BUILD_ASSERT(sizeof(new_gps_data.nmea) >=
				     sizeof(msg->module.gps.data.gps.nmea));

			strcpy(new_gps_data.nmea, msg->module.gps.data.gps.nmea);
		};
			break;
		case GPS_MODULE_DATA_FORMAT_INVALID:
			/* Fall through */
		default:
			LOG_WRN("Event does not carry valid GPS data");
			return;
		}

		cloud_codec_populate_gps_buffer(gps_buf, &new_gps_data,
						&head_gps_buf,
						ARRAY_SIZE(gps_buf));

		requested_data_status_set(APP_DATA_GNSS);
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_NEIGHBOR_CELLS_DATA_READY)) {
		BUILD_ASSERT(sizeof(neighbor_cells.cell_data) ==
			     sizeof(msg->module.modem.data.neighbor_cells.cell_data));

		BUILD_ASSERT(sizeof(neighbor_cells.neighbor_cells) ==
			     sizeof(msg->module.modem.data.neighbor_cells.neighbor_cells));

		memcpy(&neighbor_cells.cell_data, &msg->module.modem.data.neighbor_cells.cell_data,
		       sizeof(neighbor_cells.cell_data));

		memcpy(&neighbor_cells.neighbor_cells,
		       &msg->module.modem.data.neighbor_cells.neighbor_cells,
		       sizeof(neighbor_cells.neighbor_cells));

		neighbor_cells.ts = msg->module.modem.data.neighbor_cells.timestamp;
		neighbor_cells.queued = true;

		requested_data_status_set(APP_DATA_NEIGHBOR_CELLS);
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_NEIGHBOR_CELLS_DATA_NOT_READY)) {
		requested_data_status_set(APP_DATA_NEIGHBOR_CELLS);
	}

	if (IS_EVENT(msg, gps, GPS_EVT_TIMEOUT)) {
		requested_data_status_set(APP_DATA_GNSS);
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_DATA_ACK)) {
		data_ack(msg->module.cloud.data.ack.ptr,
			 msg->module.cloud.data.ack.sent);
	}
}

static void module_thread_fn(void)
{
	int err;
	struct data_msg_data msg;

	self.thread_id = k_current_get();

	err = module_start(&self);
	if (err) {
		LOG_ERR("Failed starting module, error: %d", err);
		SEND_ERROR(data, DATA_EVT_ERROR, err);
	}

	state_set(STATE_CLOUD_DISCONNECTED);

	k_work_init_delayable(&data_send_work, data_send_work_fn);

	err = setup();
	if (err) {
		LOG_ERR("setup, error: %d", err);
		SEND_ERROR(data, DATA_EVT_ERROR, err);
	}

	while (true) {
		module_get_next_msg(&self, &msg);

		switch (state) {
		case STATE_CLOUD_DISCONNECTED:
			on_cloud_state_disconnected(&msg);
			break;
		case STATE_CLOUD_CONNECTED:
			on_cloud_state_connected(&msg);
			break;
		case STATE_SHUTDOWN:
			/* The shutdown state has no transition. */
			break;
		default:
			LOG_WRN("Unknown sub state.");
			break;
		}

		on_all_states(&msg);
	}
}

K_THREAD_DEFINE(data_module_thread, CONFIG_DATA_THREAD_STACK_SIZE,
		module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, app_module_event);
EVENT_SUBSCRIBE(MODULE, util_module_event);
EVENT_SUBSCRIBE(MODULE, data_module_event);
EVENT_SUBSCRIBE_EARLY(MODULE, modem_module_event);
EVENT_SUBSCRIBE_EARLY(MODULE, cloud_module_event);
EVENT_SUBSCRIBE_EARLY(MODULE, gps_module_event);
EVENT_SUBSCRIBE_EARLY(MODULE, ui_module_event);
EVENT_SUBSCRIBE_EARLY(MODULE, sensor_module_event);
