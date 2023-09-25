/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <app_event_manager.h>
#include <zephyr/settings/settings.h>
#include <date_time.h>
#if defined(CONFIG_DATA_GRANT_SEND_ON_CONNECTION_QUALITY)
#include <modem/lte_lc.h>
#endif
#include <modem/modem_info.h>
#if defined(CONFIG_NRF_CLOUD_AGPS)
#include <net/nrf_cloud_agps.h>
#endif

#if defined(CONFIG_NRF_CLOUD_PGPS)
#include <net/nrf_cloud_pgps.h>
#endif

#include "cloud/cloud_codec/cloud_codec.h"

#define MODULE data_module

#include "modules_common.h"
#include "events/app_module_event.h"
#include "events/cloud_module_event.h"
#include "events/data_module_event.h"
#include "events/location_module_event.h"
#include "events/modem_module_event.h"
#include "events/sensor_module_event.h"
#include "events/ui_module_event.h"
#include "events/util_module_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DATA_MODULE_LOG_LEVEL);

#define DEVICE_SETTINGS_KEY			"data_module"
#define DEVICE_SETTINGS_CONFIG_KEY		"config"

struct data_msg_data {
	union {
		struct modem_module_event modem;
		struct cloud_module_event cloud;
		struct location_module_event location;
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
static struct cloud_data_gnss gnss_buf[CONFIG_DATA_GNSS_BUFFER_COUNT];
static struct cloud_data_sensors sensors_buf[CONFIG_DATA_SENSOR_BUFFER_COUNT];
static struct cloud_data_ui ui_buf[CONFIG_DATA_UI_BUFFER_COUNT];
static struct cloud_data_impact impact_buf[CONFIG_DATA_IMPACT_BUFFER_COUNT];
static struct cloud_data_battery bat_buf[CONFIG_DATA_BATTERY_BUFFER_COUNT];
static struct cloud_data_modem_dynamic modem_dyn_buf[CONFIG_DATA_MODEM_DYNAMIC_BUFFER_COUNT];
static struct cloud_data_cloud_location cloud_location;

/* Static modem data does not change between firmware versions and does not
 * have to be buffered.
 */
static struct cloud_data_modem_static modem_stat;
/* Size of the static modem (modem_stat) data structure.
 * Used to provide an array size when encoding batch data.
 */
#define MODEM_STATIC_ARRAY_SIZE 1

/* Head of ringbuffers. */
static int head_gnss_buf;
static int head_sensor_buf;
static int head_modem_dyn_buf;
static int head_ui_buf;
static int head_impact_buf;
static int head_bat_buf;

static K_SEM_DEFINE(config_load_sem, 0, 1);

/* Default device configuration. */
static struct cloud_data_cfg current_cfg = {
	.location_timeout	 = CONFIG_DATA_LOCATION_TIMEOUT_SECONDS,
	.active_mode		 = IS_ENABLED(CONFIG_DATA_DEVICE_MODE_ACTIVE),
	.active_wait_timeout	 = CONFIG_DATA_ACTIVE_TIMEOUT_SECONDS,
	.movement_resolution	 = CONFIG_DATA_MOVEMENT_RESOLUTION_SECONDS,
	.movement_timeout	 = CONFIG_DATA_MOVEMENT_TIMEOUT_SECONDS,
	.accelerometer_activity_threshold	= CONFIG_DATA_ACCELEROMETER_ACT_THRESHOLD,
	.accelerometer_inactivity_threshold	= CONFIG_DATA_ACCELEROMETER_INACT_THRESHOLD,
	.accelerometer_inactivity_timeout	= CONFIG_DATA_ACCELEROMETER_INACT_TIMEOUT_SECONDS,
	.no_data.gnss		 = !IS_ENABLED(CONFIG_DATA_SAMPLE_GNSS_DEFAULT),
	.no_data.neighbor_cell	 = !IS_ENABLED(CONFIG_DATA_SAMPLE_NEIGHBOR_CELLS_DEFAULT)
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

/* List of data types that are supported to be sent based on LTE connection evaluation. */
enum coneval_supported_data_type {
	UNUSED,
	GENERIC,
	BATCH,
	CLOUD_LOCATION,
	COUNT,
};

/* Whether `agps_request_buffer` has A-GPS request buffered for sending when connection to
 * cloud has been re-established.
 */
bool agps_request_buffered;

/* Buffered A-GPS request. */
struct nrf_modem_gnss_agnss_data_frame agps_request_buffer;

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
static void new_config_handle(struct cloud_data_cfg *new_config);

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
static bool app_event_handler(const struct app_event_header *aeh)
{
	struct data_msg_data msg = {0};
	bool enqueue_msg = false;

	if (is_modem_module_event(aeh)) {
		struct modem_module_event *event = cast_modem_module_event(aeh);

		msg.module.modem = *event;
		enqueue_msg = true;
	}

	if (is_cloud_module_event(aeh)) {
		struct cloud_module_event *event = cast_cloud_module_event(aeh);

		msg.module.cloud = *event;
		enqueue_msg = true;
	}

	if (is_location_module_event(aeh)) {
		struct location_module_event *event = cast_location_module_event(aeh);

		msg.module.location = *event;
		enqueue_msg = true;
	}

	if (is_sensor_module_event(aeh)) {
		struct sensor_module_event *event =
				cast_sensor_module_event(aeh);

		msg.module.sensor = *event;
		enqueue_msg = true;
	}

	if (is_ui_module_event(aeh)) {
		struct ui_module_event *event = cast_ui_module_event(aeh);

		msg.module.ui = *event;
		enqueue_msg = true;
	}

	if (is_app_module_event(aeh)) {
		struct app_module_event *event = cast_app_module_event(aeh);

		msg.module.app = *event;
		enqueue_msg = true;
	}

	if (is_data_module_event(aeh)) {
		struct data_module_event *event = cast_data_module_event(aeh);

		msg.module.data = *event;
		enqueue_msg = true;
	}

	if (is_util_module_event(aeh)) {
		struct util_module_event *event = cast_util_module_event(aeh);

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

static bool grant_send(enum coneval_supported_data_type type,
		       struct lte_lc_conn_eval_params *coneval,
		       bool override)
{
#if defined(CONFIG_DATA_GRANT_SEND_ON_CONNECTION_QUALITY)
	/* List used to keep track of how many times a data type has been denied a send.
	 * Indexed by coneval_supported_data_type.
	 */
	static uint8_t send_denied_count[COUNT];

	if (override) {
		/* The override flag is set, grant send. */
		return true;
	}

	if (send_denied_count[type] >= CONFIG_DATA_SEND_ATTEMPTS_COUNT_MAX) {
		/* Grant send if a message has been attempted too many times. */
		LOG_WRN("Too many attempts, granting send");
		goto grant;
	}

	LOG_DBG("Current LTE energy estimate: %d", coneval->energy_estimate);

	switch (type) {
	case GENERIC:
		if (IS_ENABLED(CONFIG_DATA_GENERIC_UPDATES_ENERGY_THRESHOLD_EXCESSIVE) &&
		    coneval->energy_estimate >= LTE_LC_ENERGY_CONSUMPTION_EXCESSIVE) {
			goto grant;
		} else if (IS_ENABLED(CONFIG_DATA_GENERIC_UPDATES_ENERGY_THRESHOLD_INCREASED) &&
		    coneval->energy_estimate >= LTE_LC_ENERGY_CONSUMPTION_INCREASED) {
			goto grant;
		} else if (IS_ENABLED(CONFIG_DATA_GENERIC_UPDATES_ENERGY_THRESHOLD_NORMAL) &&
		    coneval->energy_estimate >= LTE_LC_ENERGY_CONSUMPTION_NORMAL) {
			goto grant;
		} else if (IS_ENABLED(CONFIG_DATA_GENERIC_UPDATES_ENERGY_THRESHOLD_REDUCED) &&
		    coneval->energy_estimate >= LTE_LC_ENERGY_CONSUMPTION_REDUCED) {
			goto grant;
		} else if (IS_ENABLED(CONFIG_DATA_GENERIC_UPDATES_ENERGY_THRESHOLD_EFFICIENT) &&
		    coneval->energy_estimate >= LTE_LC_ENERGY_CONSUMPTION_EFFICIENT) {
			goto grant;
		}
		break;
	case NEIGHBOR_CELLS:
		if (IS_ENABLED(CONFIG_DATA_NEIGHBOR_CELL_UPDATES_ENERGY_THRESHOLD_EXCESSIVE) &&
		    coneval->energy_estimate >= LTE_LC_ENERGY_CONSUMPTION_EXCESSIVE) {
			goto grant;
		} else if (IS_ENABLED(CONFIG_DATA_NEIGHBOR_CELL_UPDATES_ENERGY_THRESHOLD_INCREASED)
			   && coneval->energy_estimate >= LTE_LC_ENERGY_CONSUMPTION_INCREASED) {
			goto grant;
		} else if (IS_ENABLED(CONFIG_DATA_NEIGHBOR_CELL_UPDATES_ENERGY_THRESHOLD_NORMAL) &&
		    coneval->energy_estimate >= LTE_LC_ENERGY_CONSUMPTION_NORMAL) {
			goto grant;
		} else if (IS_ENABLED(CONFIG_DATA_NEIGHBOR_CELL_UPDATES_ENERGY_THRESHOLD_REDUCED) &&
		    coneval->energy_estimate >= LTE_LC_ENERGY_CONSUMPTION_REDUCED) {
			goto grant;
		} else if (IS_ENABLED(CONFIG_DATA_NEIGHBOR_CELL_UPDATES_ENERGY_THRESHOLD_EFFICIENT)
			   && coneval->energy_estimate >= LTE_LC_ENERGY_CONSUMPTION_EFFICIENT) {
			goto grant;
		}
		break;
	case BATCH:
		if (IS_ENABLED(CONFIG_DATA_BATCH_UPDATES_ENERGY_THRESHOLD_EXCESSIVE) &&
		    coneval->energy_estimate >= LTE_LC_ENERGY_CONSUMPTION_EXCESSIVE) {
			goto grant;
		} else if (IS_ENABLED(CONFIG_DATA_BATCH_UPDATES_ENERGY_THRESHOLD_INCREASED) &&
		    coneval->energy_estimate >= LTE_LC_ENERGY_CONSUMPTION_INCREASED) {
			goto grant;
		} else if (IS_ENABLED(CONFIG_DATA_BATCH_UPDATES_ENERGY_THRESHOLD_NORMAL) &&
		    coneval->energy_estimate >= LTE_LC_ENERGY_CONSUMPTION_NORMAL) {
			goto grant;
		} else if (IS_ENABLED(CONFIG_DATA_BATCH_UPDATES_ENERGY_THRESHOLD_REDUCED) &&
		    coneval->energy_estimate >= LTE_LC_ENERGY_CONSUMPTION_REDUCED) {
			goto grant;
		} else if (IS_ENABLED(CONFIG_DATA_BATCH_UPDATES_ENERGY_THRESHOLD_EFFICIENT) &&
		    coneval->energy_estimate >= LTE_LC_ENERGY_CONSUMPTION_EFFICIENT) {
			goto grant;
		}
		break;
	default:
		LOG_WRN("Invalid/unsupported data type: %d", type);
		return false;
	}

	LOG_DBG("Send NOT granted, type: %d, energy estimate: %d, attempt: %d", type,
		coneval->energy_estimate, send_denied_count[type]);
	send_denied_count[type]++;
	return false;

grant:
	LOG_DBG("Send granted, type: %d, energy estimate: %d, attempt: %d",
		type, coneval->energy_estimate, send_denied_count[type]);
	send_denied_count[type] = 0;
#endif /* CONFIG_DATA_GRANT_SEND_ON_CONNECTION_QUALITY */
	return true;
}

static int config_settings_handler(const char *key, size_t len,
				   settings_read_cb read_cb, void *cb_arg)
{
	int err = 0;

	if (strcmp(key, DEVICE_SETTINGS_CONFIG_KEY) == 0) {
		err = read_cb(cb_arg, &current_cfg, sizeof(current_cfg));
		if (err < 0) {
			LOG_ERR("Failed to load configuration, error: %d", err);
		} else {
			LOG_DBG("Device configuration loaded from flash");
			err = 0;
		}
	}

	k_sem_give(&config_load_sem);
	return err;
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

static void cloud_codec_event_handler(const struct cloud_codec_evt *evt)
{
	if (evt->type == CLOUD_CODEC_EVT_CONFIG_UPDATE) {
		new_config_handle((struct cloud_data_cfg *)&evt->config_update);
	} else {
		LOG_ERR("Unknown event.");
	}
}

static int setup(void)
{
	int err;

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

	/* Wait up to 1 seconds for the settings API to load the device configuration stored
	 * to flash, if any.
	 */
	if (k_sem_take(&config_load_sem, K_SECONDS(1)) != 0) {
		LOG_DBG("Failed retrieveing the device configuration from flash in time");
	}

	err = cloud_codec_init(&current_cfg, cloud_codec_event_handler);
	if (err) {
		LOG_ERR("cloud_codec_init, error: %d", err);
		return err;
	}

	date_time_register_handler(date_time_event_handler);
	return 0;
}

static void config_print_all(void)
{
	if (current_cfg.active_mode) {
		LOG_DBG("Device mode: Active");
	} else {
		LOG_DBG("Device mode: Passive");
	}

	LOG_DBG("Active wait timeout: %d", current_cfg.active_wait_timeout);
	LOG_DBG("Movement resolution: %d", current_cfg.movement_resolution);
	LOG_DBG("Movement timeout: %d", current_cfg.movement_timeout);
	LOG_DBG("Location timeout: %d", current_cfg.location_timeout);
	LOG_DBG("Accelerometer act threshold: %.2f",
		 current_cfg.accelerometer_activity_threshold);
	LOG_DBG("Accelerometer inact threshold: %.2f",
		 current_cfg.accelerometer_inactivity_threshold);
	LOG_DBG("Accelerometer inact timeout: %.2f",
		 current_cfg.accelerometer_inactivity_timeout);

	if (!current_cfg.no_data.neighbor_cell) {
		LOG_DBG("Requesting of neighbor cell data is enabled");
	} else {
		LOG_DBG("Requesting of neighbor cell data is disabled");
	}

	if (!current_cfg.no_data.gnss) {
		LOG_DBG("Requesting of GNSS data is enabled");
	} else {
		LOG_DBG("Requesting of GNSS data is disabled");
	}

	if (!current_cfg.no_data.wifi) {
		LOG_DBG("Requesting of Wi-Fi data is enabled");
	} else {
		LOG_DBG("Requesting of Wi-Fi data is disabled");
	}
}

static void config_distribute(enum data_module_event_type type)
{
	struct data_module_event *data_module_event = new_data_module_event();

	__ASSERT(data_module_event, "Not enough heap left to allocate event");

	data_module_event->type = type;
	data_module_event->data.cfg = current_cfg;

	APP_EVENT_SUBMIT(data_module_event);
}

static void data_send(enum data_module_event_type event,
		      struct cloud_codec_data *data)
{
	struct data_module_event *module_event = new_data_module_event();

	__ASSERT(module_event, "Not enough heap left to allocate event");

	module_event->type = event;

	BUILD_ASSERT((sizeof(data->paths) == sizeof(module_event->data.buffer.paths)),
			"Size of the object path list does not match");
	BUILD_ASSERT((sizeof(data->paths[0]) == sizeof(module_event->data.buffer.paths[0])),
			"Size of an entry in the object path list does not match");

	if (IS_ENABLED(CONFIG_CLOUD_CODEC_LWM2M)) {
		memcpy(module_event->data.buffer.paths, data->paths, sizeof(data->paths));
		module_event->data.buffer.valid_object_paths = data->valid_object_paths;
	} else {
		module_event->data.buffer.buf = data->buf;
		module_event->data.buffer.len = data->len;
	}

	APP_EVENT_SUBMIT(module_event);

	/* Reset buffer */
	memset(data, 0, sizeof(struct cloud_codec_data));
}

/* This function allocates buffer on the heap, which needs to be freed after use. */
static void data_encode(void)
{
	int err;
	struct cloud_codec_data codec = { 0 };
	struct lte_lc_conn_eval_params coneval = { 0 };

	/* Variable used to override connection evaluation calculations in case connection
	 * evalution fails for some non-critical reason.
	 */
	bool override = false;

	if (!date_time_is_valid()) {
		/* Date time library does not have valid time to
		 * timestamp cloud data. Abort cloud publicaton. Data will
		 * be cached in it respective ringbuffer.
		 */
		return;
	}

#if defined(CONFIG_DATA_GRANT_SEND_ON_CONNECTION_QUALITY)
	/* Perform connection evaluation to determine how expensive it is to send data. */
	err = lte_lc_conn_eval_params_get(&coneval);
	if (err < 0) {
		LOG_ERR("lte_lc_conn_eval_params_get, error: %d", err);
		SEND_ERROR(cloud, CLOUD_EVT_ERROR, err);
		return;
	} else if (err > 0) {
		LOG_WRN("Connection evaluation failed, error: %d", err);

		/* Positive error codes returned from lte_lc_conn_eval_params_get() indicates
		 * that the connection evaluation failed due to a non-critical reason.
		 * We don't treat this as an irrecoverable error because it can occur
		 * occasionally. Since we don't have any connection evaluation parameters we
		 * grant encoding and sending of data.
		 */
		override = true;
	}
#endif

	if (grant_send(CLOUD_LOCATION, &coneval, override)) {
		err = cloud_codec_encode_cloud_location(&codec, &cloud_location);
		switch (err) {
		case 0:
			LOG_DBG("Cloud location data encoded successfully");
			data_send(DATA_EVT_CLOUD_LOCATION_DATA_SEND, &codec);
			break;
		case -ENOTSUP:
			/* Cloud location data encoding not supported */
			break;
		case -ENODATA:
			LOG_DBG("No cloud location data to encode, error: %d", err);
			break;
		default:
			LOG_ERR("Error encoding cloud location data: %d", err);
			SEND_ERROR(data, DATA_EVT_ERROR, err);
			return;
		}
	}

	if (grant_send(GENERIC, &coneval, override)) {
		err = cloud_codec_encode_data(&codec,
					      &gnss_buf[head_gnss_buf],
					      &sensors_buf[head_sensor_buf],
					      &modem_stat,
					      &modem_dyn_buf[head_modem_dyn_buf],
					      &ui_buf[head_ui_buf],
					      &impact_buf[head_impact_buf],
					      &bat_buf[head_bat_buf]);
		switch (err) {
		case 0:
			LOG_DBG("Data encoded successfully");
			data_send(DATA_EVT_DATA_SEND, &codec);
			break;
		case -ENODATA:
			/* This error might occur when data has not been obtained prior
			 * to data encoding.
			 */
			LOG_DBG("No new data to encode");
			break;
		case -ENOTSUP:
			LOG_DBG("Regular data updates are not supported");
			break;
		default:
			LOG_ERR("Error encoding message %d", err);
			SEND_ERROR(data, DATA_EVT_ERROR, err);
			return;
		}
	}

	if (grant_send(BATCH, &coneval, override)) {
		err = cloud_codec_encode_batch_data(&codec,
						    gnss_buf,
						    sensors_buf,
						    &modem_stat,
						    modem_dyn_buf,
						    ui_buf,
						    impact_buf,
						    bat_buf,
						    ARRAY_SIZE(gnss_buf),
						    ARRAY_SIZE(sensors_buf),
						    MODEM_STATIC_ARRAY_SIZE,
						    ARRAY_SIZE(modem_dyn_buf),
						    ARRAY_SIZE(ui_buf),
						    ARRAY_SIZE(impact_buf),
						    ARRAY_SIZE(bat_buf));
		switch (err) {
		case 0:
			LOG_DBG("Batch data encoded successfully");
			data_send(DATA_EVT_DATA_SEND_BATCH, &codec);
			break;
		case -ENODATA:
			LOG_DBG("No batch data to encode, ringbuffers are empty");
			break;
		case -ENOTSUP:
			LOG_DBG("Encoding of batch data not supported");
			break;
		default:
			LOG_ERR("Error batch-enconding data: %d", err);
			SEND_ERROR(data, DATA_EVT_ERROR, err);
			return;
		}
	}
}

#if defined(CONFIG_NRF_CLOUD_AGPS) && !defined(CONFIG_NRF_CLOUD_MQTT)
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

/**
 * @brief Combine and encode modem network parameters together with the incoming A-GPS data request
 *	  types to form the A-GPS request.
 *
 * @param[in] incoming_request Pointer to a structure containing A-GPS data types that has been
 *			       requested by the modem. If incoming_request is NULL, all A-GPS data
 *			       types are requested.
 *
 * @return 0 on success, otherwise a negative error code indicating reason of failure.
 */
static int agps_request_encode(struct nrf_modem_gnss_agnss_data_frame *incoming_request)
{
	int err;
	struct cloud_codec_data codec = {0};
	static struct modem_param_info modem_info = {0};
	static struct cloud_data_agps_request cloud_agps_request = {0};

	err = get_modem_info(&modem_info);
	if (err) {
		return err;
	}

	if (incoming_request == NULL) {
		const uint32_t mask = IS_ENABLED(CONFIG_NRF_CLOUD_PGPS) ? 0u : 0xFFFFFFFFu;

		LOG_DBG("Requesting all A-GPS elements");
		cloud_agps_request.request.data_flags =
					NRF_MODEM_GNSS_AGNSS_GPS_UTC_REQUEST |
					NRF_MODEM_GNSS_AGNSS_KLOBUCHAR_REQUEST |
					NRF_MODEM_GNSS_AGNSS_NEQUICK_REQUEST |
					NRF_MODEM_GNSS_AGNSS_GPS_SYS_TIME_AND_SV_TOW_REQUEST |
					NRF_MODEM_GNSS_AGNSS_POSITION_REQUEST |
					NRF_MODEM_GNSS_AGNSS_INTEGRITY_REQUEST;
		cloud_agps_request.request.system_count = 1;
		cloud_agps_request.request.system[0].sv_mask_ephe = mask;
		cloud_agps_request.request.system[0].sv_mask_alm = mask;
	} else {
		cloud_agps_request.request = *incoming_request;
	}

	cloud_agps_request.mcc = modem_info.network.mcc.value;
	cloud_agps_request.mnc = modem_info.network.mnc.value;
	cloud_agps_request.cell = modem_info.network.cellid_dec;
	cloud_agps_request.area = modem_info.network.area_code.value;
	cloud_agps_request.queued = true;

	err = cloud_codec_encode_agps_request(&codec, &cloud_agps_request);
	switch (err) {
	case 0:
		LOG_DBG("A-GPS request encoded successfully");
		data_send(DATA_EVT_AGPS_REQUEST_DATA_SEND, &codec);
		break;
	case -ENOTSUP:
		LOG_ERR("Encoding of A-GPS requests are not supported by the configured codec");
		break;
	case -ENODATA:
		LOG_DBG("No A-GPS request data to encode, error: %d", err);
		break;
	default:
		LOG_ERR("Error encoding A-GPS request: %d", err);
		SEND_ERROR(data, DATA_EVT_ERROR, err);
		break;
	}

	return err;
}
#endif /* CONFIG_NRF_CLOUD_AGPS && !CONFIG_NRF_CLOUD_MQTT */

static void config_get(void)
{
	SEND_EVENT(data, DATA_EVT_CONFIG_GET);
}

static void config_send(void)
{
	int err;
	struct cloud_codec_data codec = { 0 };

	err = cloud_codec_encode_config(&codec, &current_cfg);
	if (err == -ENOTSUP) {
		LOG_WRN("Encoding of device configuration is not supported");
		return;
	} else if (err) {
		LOG_ERR("Error encoding configuration, error: %d", err);
		SEND_ERROR(data, DATA_EVT_ERROR, err);
		return;
	}

	data_send(DATA_EVT_CONFIG_SEND, &codec);
}

static void data_ui_send(void)
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

	err = cloud_codec_encode_ui_data(&codec, &ui_buf[head_ui_buf]);
	if (err == -ENODATA) {
		LOG_DBG("No new UI data to encode, error: %d", err);
		return;
	} else if (err == -ENOTSUP) {
		LOG_ERR("Encoding of UI data is not supported, error: %d", err);
		return;
	} else if (err) {
		LOG_ERR("Encoding button press, error: %d", err);
		SEND_ERROR(data, DATA_EVT_ERROR, err);
		return;
	}

	data_send(DATA_EVT_UI_DATA_SEND, &codec);
}

static void data_impact_send(void)
{
	int err;
	struct cloud_codec_data codec = {0};

	if (!date_time_is_valid()) {
		return;
	}

	err = cloud_codec_encode_impact_data(&codec, &impact_buf[head_impact_buf]);
	if (err == -ENODATA) {
		LOG_DBG("No new impact data to encode, error: %d", err);
		return;
	} else if (err == -ENOTSUP) {
		LOG_WRN("Encoding of impact data is not supported, error: %d", err);
		return;
	} else if (err) {
		LOG_ERR("Encoding impact data failed, error: %d", err);
		SEND_ERROR(data, DATA_EVT_ERROR, err);
		return;
	}

	data_send(DATA_EVT_IMPACT_DATA_SEND, &codec);
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
	if (!k_work_delayable_is_pending(&data_send_work)) {
		if (data_type == APP_DATA_LOCATION) {
			/* We may get location related data when a fallback to next method occurs */
			data_send_work_fn(NULL);
		} else {
			/* If the data_send_work is not pending it means that the module has already
			 * triggered an data encode/send.
			 */
			LOG_DBG("Data already encoded and sent, abort.");
		}
		return;
	}

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
			LOG_DBG("New Device mode: Active");
		} else {
			LOG_DBG("New Device mode: Passive");
		}

		config_change = true;
	}

	if (current_cfg.no_data.gnss != new_config->no_data.gnss) {
		current_cfg.no_data.gnss = new_config->no_data.gnss;

		if (!current_cfg.no_data.gnss) {
			LOG_DBG("Requesting of GNSS data is enabled");
		} else {
			LOG_DBG("Requesting of GNSS data is disabled");
		}

		config_change = true;
	}

	if (current_cfg.no_data.neighbor_cell != new_config->no_data.neighbor_cell) {
		current_cfg.no_data.neighbor_cell = new_config->no_data.neighbor_cell;

		if (!current_cfg.no_data.neighbor_cell) {
			LOG_DBG("Requesting of neighbor cell data is enabled");
		} else {
			LOG_DBG("Requesting of neighbor cell data is disabled");
		}

		config_change = true;
	}

	if (current_cfg.no_data.wifi != new_config->no_data.wifi) {
		current_cfg.no_data.wifi = new_config->no_data.wifi;

		if (!current_cfg.no_data.wifi) {
			LOG_DBG("Requesting of Wi-Fi data is enabled");
		} else {
			LOG_DBG("Requesting of Wi-Fi data is disabled");
		}

		config_change = true;
	}

	if (new_config->location_timeout > 0) {
		if (current_cfg.location_timeout != new_config->location_timeout) {
			current_cfg.location_timeout = new_config->location_timeout;

			LOG_DBG("New location timeout: %d", current_cfg.location_timeout);

			config_change = true;
		}
	} else {
		LOG_WRN("New location timeout out of range: %d", new_config->location_timeout);
	}

	if (new_config->active_wait_timeout > 0) {
		if (current_cfg.active_wait_timeout != new_config->active_wait_timeout) {
			current_cfg.active_wait_timeout = new_config->active_wait_timeout;

			LOG_DBG("New Active wait timeout: %d", current_cfg.active_wait_timeout);

			config_change = true;
		}
	} else {
		LOG_WRN("New Active timeout out of range: %d", new_config->active_wait_timeout);
	}

	if (new_config->movement_resolution > 0) {
		if (current_cfg.movement_resolution != new_config->movement_resolution) {
			current_cfg.movement_resolution = new_config->movement_resolution;

			LOG_DBG("New Movement resolution: %d", current_cfg.movement_resolution);

			config_change = true;
		}
	} else {
		LOG_WRN("New Movement resolution out of range: %d",
			new_config->movement_resolution);
	}

	if (new_config->movement_timeout > 0) {
		if (current_cfg.movement_timeout != new_config->movement_timeout) {
			current_cfg.movement_timeout = new_config->movement_timeout;

			LOG_DBG("New Movement timeout: %d", current_cfg.movement_timeout);

			config_change = true;
		}
	} else {
		LOG_WRN("New Movement timeout out of range: %d", new_config->movement_timeout);
	}

	if (current_cfg.accelerometer_activity_threshold !=
	    new_config->accelerometer_activity_threshold) {
		current_cfg.accelerometer_activity_threshold =
		new_config->accelerometer_activity_threshold;
		LOG_DBG("New Accelerometer act threshold: %.2f",
			current_cfg.accelerometer_activity_threshold);
		config_change = true;
	}
	if (current_cfg.accelerometer_inactivity_threshold !=
	    new_config->accelerometer_inactivity_threshold) {
		current_cfg.accelerometer_inactivity_threshold =
		new_config->accelerometer_inactivity_threshold;
		LOG_DBG("New Accelerometer inact threshold: %.2f",
			current_cfg.accelerometer_inactivity_threshold);
		config_change = true;
	}
	if (current_cfg.accelerometer_inactivity_timeout !=
	    new_config->accelerometer_inactivity_timeout) {
		current_cfg.accelerometer_inactivity_timeout =
		new_config->accelerometer_inactivity_timeout;
		LOG_DBG("New Accelerometer inact timeout: %.2f",
			current_cfg.accelerometer_inactivity_timeout);
		config_change = true;
	}

	/* If there has been a change in the currently applied device configuration we want to store
	 * the configuration to flash and distribute it to other modules.
	 */
	if (config_change) {
		int err = save_config(&current_cfg, sizeof(current_cfg));

		if (err) {
			LOG_ERR("Configuration not stored, error: %d", err);
		}

		config_distribute(DATA_EVT_CONFIG_READY);
	} else {
		LOG_DBG("No new values in incoming device configuration update message");
	}

	/* Always acknowledge all configurations back to cloud to avoid a potential mismatch
	 * between reported parameters in the cloud-side state and parameters reported by the
	 * device.
	 */

	/* LwM2M doesn't require reporting of the current configuration back to cloud. */
	if (IS_ENABLED(CONFIG_LWM2M_INTEGRATION)) {
		return;
	}

	LOG_DBG("Acknowledge currently applied configuration back to cloud");
	config_send();
}

/**
 * @brief Function that requests A-GPS and P-GPS data upon receiving a request from the
 *        location module.
 *
 * @param[in] incoming_request Pointer to a structure containing A-GPS data types that has been
 *			       requested by the modem. If incoming_request is NULL, all A-GPS data
 *			       types are requested.
 */
static void agps_request_handle(struct nrf_modem_gnss_agnss_data_frame *incoming_request)
{
	int err;

#if defined(CONFIG_NRF_CLOUD_AGPS)
#if defined(CONFIG_NRF_CLOUD_MQTT)
	/* If CONFIG_NRF_CLOUD_MQTT is enabled, the nRF Cloud MQTT transport library will be used
	 * to send the request.
	 */
	err = (incoming_request == NULL) ? nrf_cloud_agps_request_all() :
					   nrf_cloud_agps_request(incoming_request);
	if (err) {
		LOG_WRN("Failed to request A-GPS data, error: %d", err);
		LOG_DBG("This is expected to fail if we are not in a connected state");
	} else {
		if (nrf_cloud_agps_request_in_progress()) {
			LOG_DBG("A-GPS request sent");
			return;
		}
		LOG_DBG("No A-GPS data requested");
		/* Continue so P-GPS, if enabled, can be requested. */
	}
#else
	/* If the nRF Cloud MQTT transport library is not enabled, we will have to create an
	 * A-GPS request and send out an event containing the request for the cloud module to pick
	 * up and send to the cloud that is currently used.
	 */
	err = (incoming_request == NULL) ? agps_request_encode(NULL) :
					   agps_request_encode(incoming_request);
	if (err) {
		LOG_WRN("Failed to request A-GPS data, error: %d", err);
	} else {
		LOG_DBG("A-GPS request sent");
		return;
	}
#endif
#endif

#if defined(CONFIG_NRF_CLOUD_PGPS)
	/* A-GPS data is not expected to be received. Proceed to schedule a callback when
	 * P-GPS data for current time is available.
	 */
	err = nrf_cloud_pgps_notify_prediction();
	if (err) {
		LOG_ERR("Requesting notification of prediction availability, error: %d", err);
	}
#endif

	(void)err;
}

/* Message handler for STATE_CLOUD_DISCONNECTED. */
static void on_cloud_state_disconnected(struct data_msg_data *msg)
{
	if (IS_EVENT(msg, cloud, CLOUD_EVT_CONNECTED)) {
		state_set(STATE_CLOUD_CONNECTED);
		if (agps_request_buffered) {
			LOG_DBG("Handle buffered A-GPS request");
			agps_request_handle(&agps_request_buffer);
			agps_request_buffered = false;
		}
		return;
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_CONFIG_EMPTY) &&
	    IS_ENABLED(CONFIG_NRF_CLOUD_MQTT)) {
		config_send();
	}

	if (IS_EVENT(msg, location, LOCATION_MODULE_EVT_AGPS_NEEDED)) {
		LOG_DBG("A-GPS request buffered");
		agps_request_buffered = true;
		agps_request_buffer = msg->module.location.data.agps_request;
		return;
	}
}

/* Message handler for STATE_CLOUD_CONNECTED. */
static void on_cloud_state_connected(struct data_msg_data *msg)
{
	if (IS_EVENT(msg, data, DATA_EVT_DATA_READY)) {
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

	if (IS_EVENT(msg, data, DATA_EVT_IMPACT_DATA_READY)) {
		data_impact_send();
		return;
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_DISCONNECTED)) {
		state_set(STATE_CLOUD_DISCONNECTED);
		return;
	}

	if (IS_EVENT(msg, cloud, CLOUD_EVT_CONFIG_EMPTY)) {
		config_send();
		return;
	}

	if (IS_EVENT(msg, location, LOCATION_MODULE_EVT_AGPS_NEEDED)) {
		agps_request_handle(&msg->module.location.data.agps_request);
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
			.location_timeout =
				msg->module.cloud.data.config.location_timeout,
			.accelerometer_activity_threshold =
				msg->module.cloud.data.config.accelerometer_activity_threshold,
			.accelerometer_inactivity_threshold =
				msg->module.cloud.data.config.accelerometer_inactivity_threshold,
			.accelerometer_inactivity_timeout =
				msg->module.cloud.data.config.accelerometer_inactivity_timeout,
			.no_data.gnss =
				msg->module.cloud.data.config.no_data.gnss,
			.no_data.neighbor_cell =
				msg->module.cloud.data.config.no_data.neighbor_cell,
			.no_data.wifi =
				msg->module.cloud.data.config.no_data.wifi
		};

		new_config_handle(&new);
		return;
	}

	if (IS_EVENT(msg, app, APP_EVT_START)) {
		config_print_all();
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
		 * to confirm data is reported to the data manager.
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

		BUILD_ASSERT(sizeof(modem_stat.imei) >=
			     sizeof(msg->module.modem.data.modem_static.imei));

		strcpy(modem_stat.appv, msg->module.modem.data.modem_static.app_version);
		strcpy(modem_stat.brdv, msg->module.modem.data.modem_static.board_version);
		strcpy(modem_stat.fw, msg->module.modem.data.modem_static.modem_fw);
		strcpy(modem_stat.iccid, msg->module.modem.data.modem_static.iccid);
		strcpy(modem_stat.imei, msg->module.modem.data.modem_static.imei);

		requested_data_status_set(APP_DATA_MODEM_STATIC);
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_MODEM_DYNAMIC_DATA_NOT_READY)) {
		requested_data_status_set(APP_DATA_MODEM_DYNAMIC);
	}

	if (IS_EVENT(msg, modem, MODEM_EVT_MODEM_DYNAMIC_DATA_READY)) {
		struct cloud_data_modem_dynamic new_modem_data = {
			.area = msg->module.modem.data.modem_dynamic.area_code,
			.nw_mode = msg->module.modem.data.modem_dynamic.nw_mode,
			.band = msg->module.modem.data.modem_dynamic.band,
			.cell = msg->module.modem.data.modem_dynamic.cell_id,
			.rsrp = msg->module.modem.data.modem_dynamic.rsrp,
			.mcc = msg->module.modem.data.modem_dynamic.mcc,
			.mnc = msg->module.modem.data.modem_dynamic.mnc,
			.ts = msg->module.modem.data.modem_dynamic.timestamp,
			.queued = true
		};

		BUILD_ASSERT(sizeof(new_modem_data.ip) >=
			     sizeof(msg->module.modem.data.modem_dynamic.ip_address));

		BUILD_ASSERT(sizeof(new_modem_data.apn) >=
			     sizeof(msg->module.modem.data.modem_dynamic.apn));

		BUILD_ASSERT(sizeof(new_modem_data.apn) >=
			     sizeof(msg->module.modem.data.modem_dynamic.apn));

		strcpy(new_modem_data.ip, msg->module.modem.data.modem_dynamic.ip_address);
		strcpy(new_modem_data.apn, msg->module.modem.data.modem_dynamic.apn);
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

	if (IS_EVENT(msg, sensor, SENSOR_EVT_FUEL_GAUGE_READY)) {
		struct cloud_data_battery new_battery_data = {
			.bat = msg->module.sensor.data.bat.battery_level,
			.bat_ts = msg->module.sensor.data.bat.timestamp,
			.queued = true
		};

		cloud_codec_populate_bat_buffer(bat_buf, &new_battery_data,
						&head_bat_buf,
						ARRAY_SIZE(bat_buf));

		requested_data_status_set(APP_DATA_BATTERY);
	}

	if (IS_EVENT(msg, sensor, SENSOR_EVT_FUEL_GAUGE_NOT_SUPPORTED)) {
		requested_data_status_set(APP_DATA_BATTERY);
	}

	if (IS_EVENT(msg, sensor, SENSOR_EVT_ENVIRONMENTAL_DATA_READY)) {
		struct cloud_data_sensors new_sensor_data = {
			.temperature = msg->module.sensor.data.sensors.temperature,
			.humidity = msg->module.sensor.data.sensors.humidity,
			.pressure = msg->module.sensor.data.sensors.pressure,
			.bsec_air_quality = msg->module.sensor.data.sensors.bsec_air_quality,
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

	if (IS_EVENT(msg, sensor, SENSOR_EVT_MOVEMENT_IMPACT_DETECTED)) {
		struct cloud_data_impact new_impact_data = {
			.magnitude = msg->module.sensor.data.impact.magnitude,
			.ts = msg->module.sensor.data.impact.timestamp,
			.queued = true
		};

		cloud_codec_populate_impact_buffer(impact_buf, &new_impact_data,
						   &head_impact_buf,
						   ARRAY_SIZE(impact_buf));
		SEND_EVENT(data, DATA_EVT_IMPACT_DATA_READY);
		return;
	}

	if (IS_EVENT(msg, location, LOCATION_MODULE_EVT_GNSS_DATA_READY)) {
		struct cloud_data_gnss new_location_data = {
			.gnss_ts = msg->module.location.data.location.timestamp,
			.queued = true
		};

		new_location_data.pvt.acc = msg->module.location.data.location.pvt.accuracy;
		new_location_data.pvt.alt = msg->module.location.data.location.pvt.altitude;
		new_location_data.pvt.hdg = msg->module.location.data.location.pvt.heading;
		new_location_data.pvt.lat = msg->module.location.data.location.pvt.latitude;
		new_location_data.pvt.longi = msg->module.location.data.location.pvt.longitude;
		new_location_data.pvt.spd = msg->module.location.data.location.pvt.speed;

		cloud_codec_populate_gnss_buffer(gnss_buf, &new_location_data,
						&head_gnss_buf,
						ARRAY_SIZE(gnss_buf));

		requested_data_status_set(APP_DATA_LOCATION);
	}

	if (IS_EVENT(msg, location, LOCATION_MODULE_EVT_DATA_NOT_READY)) {
		requested_data_status_set(APP_DATA_LOCATION);
	}

	if (IS_EVENT(msg, location, LOCATION_MODULE_EVT_CLOUD_LOCATION_DATA_READY)) {
		cloud_location.neighbor_cells_valid = false;
		cloud_location.neighbor_cells.queued = false;
		if (msg->module.location.data.cloud_location.neighbor_cells_valid) {
			BUILD_ASSERT(sizeof(cloud_location.neighbor_cells.cell_data) ==
				     sizeof(msg->module.location.data.cloud_location
					.neighbor_cells.cell_data));

			BUILD_ASSERT(sizeof(cloud_location.neighbor_cells.neighbor_cells) ==
				     sizeof(msg->module.location.data.cloud_location
					.neighbor_cells.neighbor_cells));

			cloud_location.neighbor_cells_valid = true;
			cloud_location.neighbor_cells.ts =
				msg->module.location.data.cloud_location.timestamp;
			cloud_location.neighbor_cells.queued = true;

			memcpy(&cloud_location.neighbor_cells.cell_data,
			       &msg->module.location.data.cloud_location.neighbor_cells.cell_data,
			       sizeof(cloud_location.neighbor_cells.cell_data));

			memcpy(&cloud_location.neighbor_cells.neighbor_cells,
			       &msg->module.location.data.cloud_location
					.neighbor_cells.neighbor_cells,
			       sizeof(cloud_location.neighbor_cells.neighbor_cells));
		}
#if defined(CONFIG_LOCATION_METHOD_WIFI)
		cloud_location.wifi_access_points_valid = false;
		cloud_location.wifi_access_points.queued = false;

		if (msg->module.location.data.cloud_location.wifi_access_points_valid) {
			BUILD_ASSERT(sizeof(cloud_location.wifi_access_points.ap_info) ==
				     sizeof(msg->module.location.data.cloud_location
					.wifi_access_points.ap_info));

			cloud_location.wifi_access_points_valid = true;
			cloud_location.wifi_access_points.ts =
				msg->module.location.data.cloud_location.timestamp;
			cloud_location.wifi_access_points.queued = true;

			memcpy(&cloud_location.wifi_access_points.ap_info,
			       &msg->module.location.data.cloud_location
					.wifi_access_points.ap_info,
			       sizeof(cloud_location.wifi_access_points.ap_info));

			cloud_location.wifi_access_points.cnt =
				msg->module.location.data.cloud_location.wifi_access_points.cnt;
		}
#endif
		cloud_location.ts = msg->module.location.data.cloud_location.timestamp;
		cloud_location.queued = true;

		requested_data_status_set(APP_DATA_LOCATION);
	}

	if (IS_EVENT(msg, location, LOCATION_MODULE_EVT_TIMEOUT)) {
		requested_data_status_set(APP_DATA_LOCATION);
	}
}

static void module_thread_fn(void)
{
	int err;
	struct data_msg_data msg = { 0 };

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
			LOG_ERR("Unknown sub state.");
			break;
		}

		on_all_states(&msg);
	}
}

K_THREAD_DEFINE(data_module_thread, CONFIG_DATA_THREAD_STACK_SIZE,
		module_thread_fn, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, app_module_event);
APP_EVENT_SUBSCRIBE(MODULE, util_module_event);
APP_EVENT_SUBSCRIBE(MODULE, data_module_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, modem_module_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, cloud_module_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, location_module_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, ui_module_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, sensor_module_event);
