/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/shell/shell.h>
#ifdef CONFIG_GETOPT
#include <zephyr/posix/unistd.h>
#endif
#include <getopt.h>
#include <net/nrf_cloud.h>
#if defined(CONFIG_NRF_CLOUD_REST)
#include <net/nrf_cloud_rest.h>
#endif
#if defined(CONFIG_NRF_CLOUD_COAP)
#include <net/nrf_cloud_coap.h>
#endif
#include <modem/location.h>
#include <dk_buttons_and_leds.h>
#include <date_time.h>
#if defined(CONFIG_MOSH_CLOUD_LWM2M)
#include <net/lwm2m_client_utils_location.h>
#include "cloud_lwm2m.h"
#endif

#include "mosh_print.h"
#include "str_utils.h"
#include "location_srv_ext.h"
#include "location_cmd_utils.h"
#if defined(CONFIG_LOCATION_DATA_DETAILS)
#include "location_details.h"
#include "str_utils.h"
#endif
#include "mosh_defines.h"

extern struct k_work_q mosh_common_work_q;

#define MOSH_LOC_SERVICE_NONE 0xFF

/* Whether cloud location (cellular and Wi-Fi positioning) response is requested from the cloud.
 * Or whether it is not requested and MoSh indicates to Location library that positioning
 * result is unknown.
 */
static bool arg_cloud_resp_enabled;
/* Whether GNSS location is sent to cloud. */
static bool arg_cloud_gnss;
/* Format of the GNSS location to be sent to cloud. */
static enum nrf_cloud_gnss_type arg_cloud_gnss_format;
/* Whether location details are sent to cloud. */
static bool arg_cloud_details;

#if defined(CONFIG_DK_LIBRARY)
static struct k_work_delayable location_evt_led_off_work;
#endif

#define LOCATION_DETAILS_CMD_STR_MAX_LEN 255

/* Work for location details to nRF Cloud */
struct location_cloud_work_data {
	struct k_work work;

	enum nrf_cloud_gnss_type format;
	int64_t timestamp_ms;
	struct location_event_data loc_evt_data;
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	char mosh_cmd[LOCATION_DETAILS_CMD_STR_MAX_LEN + 1];
#endif
	/* Whether GNSS location should be sent to cloud. */
	bool send_cloud_gnss;
	/* Whether location details should be sent to cloud. */
	bool send_cloud_details;
};

static struct location_cloud_work_data location_cloud_work_data;

static const char location_get_usage_str[] =
	"Usage: location get [--mode <mode>] [--method <method>]\n"
	"       [--timeout <secs>] [--interval <secs>]\n"
	"       [--gnss_accuracy <acc>] [--gnss_num_fixes <number of fixes>]\n"
	"       [--gnss_timeout <timeout in secs>] [--gnss_visibility]\n"
	"       [--gnss_priority] [--gnss_cloud_nmea] [--gnss_cloud_pvt]\n"
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	"       [--cloud_details]\n"
#endif
	"       [--cellular_timeout <timeout in secs>] [--cellular_service <service_string>]\n"
	"       [--cellular_cell_count <cell count>]\n"
	"       [--wifi_timeout <timeout in secs>] [--wifi_service <service_string>]\n"
	"       [--cloud_resp_disabled]\n"
	"\n"
	"Options:\n"
	"  -m, --method, [str]         Location method: 'gnss', 'cellular' or 'wifi'. Multiple\n"
	"                              '--method' parameters may be given to indicate list of\n"
	"                              methods in priority order.\n"
	"  --mode, [str]               Location request mode: 'fallback' (default) or 'all'.\n"
	"  --interval, [int]           Position update interval in seconds\n"
	"                              (default: 0 = single position)\n"
	"  -t, --timeout, [float]      Timeout for the entire location request in seconds.\n"
	"                              Zero means timeout is disabled.\n"
	"  --gnss_accuracy, [str]      Used GNSS accuracy: 'low', 'normal' or 'high'\n"
	"  --gnss_num_fixes, [int]     Number of consecutive fix attempts (if gnss_accuracy\n"
	"                              set to 'high', default: 3)\n"
	"  --gnss_timeout, [float]     GNSS timeout in seconds. Zero means timeout is disabled.\n"
	"  --gnss_visibility,          Enables GNSS obstructed visibility detection\n"
	"  --gnss_priority,            Enables GNSS priority mode\n"
	"  --gnss_cloud_nmea,          Send acquired GNSS location to nRF Cloud formatted as NMEA\n"
	"  --gnss_cloud_pvt,           Send acquired GNSS location to nRF Cloud formatted as PVT\n"
	"  --cloud_details,            Send detailed data to cloud.\n"
	"                              Valid if CONFIG_LOCATION_DATA_DETAILS is set.\n"
	"  --cellular_timeout, [float] Cellular timeout in seconds.\n"
	"                              Zero means timeout is disabled.\n"
	"  --cellular_service, [str]   Used cellular positioning service:\n"
	"                              'any' (default), 'nrf' or 'here'\n"
	"  --cellular_cell_count, [int]\n"
	"                              Requested number of cells\n"
	"  --wifi_timeout, [float]     Wi-Fi timeout in seconds. Zero means timeout is disabled.\n"
	"  --wifi_service, [str]       Used Wi-Fi positioning service:\n"
	"                              'any' (default), 'nrf' or 'here'\n"
	"  --cloud_resp_disabled,      Do not wait for location response from cloud.\n"
	"                              Valid if CONFIG_LOCATION_SERVICE_EXTERNAL is set.\n"
	"  -h, --help,                 Shows this help information";

/* The following do not have short options */
enum {
	LOCATION_SHELL_OPT_INTERVAL         = 1001,
	LOCATION_SHELL_OPT_MODE,
	LOCATION_SHELL_OPT_GNSS_ACCURACY,
	LOCATION_SHELL_OPT_GNSS_TIMEOUT,
	LOCATION_SHELL_OPT_GNSS_NUM_FIXES,
	LOCATION_SHELL_OPT_GNSS_VISIBILITY,
	LOCATION_SHELL_OPT_GNSS_PRIORITY_MODE,
	LOCATION_SHELL_OPT_GNSS_LOC_CLOUD_NMEA,
	LOCATION_SHELL_OPT_GNSS_LOC_CLOUD_PVT,
	LOCATION_SHELL_OPT_GNSS_LOC_CLOUD_DETAILS,
	LOCATION_SHELL_OPT_CELLULAR_TIMEOUT,
	LOCATION_SHELL_OPT_CELLULAR_SERVICE,
	LOCATION_SHELL_OPT_CELLULAR_CELL_COUNT,
	LOCATION_SHELL_OPT_CLOUD_RESP_DISABLED,
	LOCATION_SHELL_OPT_WIFI_TIMEOUT,
	LOCATION_SHELL_OPT_WIFI_SERVICE,
};

/* Specifying the expected options */
static struct option long_options[] = {
	{ "method", required_argument, 0, 'm' },
	{ "mode", required_argument, 0, LOCATION_SHELL_OPT_MODE },
	{ "interval", required_argument, 0, LOCATION_SHELL_OPT_INTERVAL },
	{ "timeout", required_argument, 0, 't' },
	{ "gnss_accuracy", required_argument, 0, LOCATION_SHELL_OPT_GNSS_ACCURACY },
	{ "gnss_timeout", required_argument, 0, LOCATION_SHELL_OPT_GNSS_TIMEOUT },
	{ "gnss_num_fixes", required_argument, 0, LOCATION_SHELL_OPT_GNSS_NUM_FIXES },
	{ "gnss_visibility", no_argument, 0, LOCATION_SHELL_OPT_GNSS_VISIBILITY },
	{ "gnss_priority", no_argument, 0, LOCATION_SHELL_OPT_GNSS_PRIORITY_MODE },
	{ "gnss_cloud_nmea", no_argument, 0, LOCATION_SHELL_OPT_GNSS_LOC_CLOUD_NMEA },
	{ "gnss_cloud_pvt", no_argument, 0, LOCATION_SHELL_OPT_GNSS_LOC_CLOUD_PVT },
	{ "cloud_details", no_argument, 0, LOCATION_SHELL_OPT_GNSS_LOC_CLOUD_DETAILS },
	{ "cellular_timeout", required_argument, 0, LOCATION_SHELL_OPT_CELLULAR_TIMEOUT },
	{ "cellular_service", required_argument, 0, LOCATION_SHELL_OPT_CELLULAR_SERVICE },
	{ "cellular_cell_count", required_argument, 0, LOCATION_SHELL_OPT_CELLULAR_CELL_COUNT },
	{ "cloud_resp_disabled", no_argument, 0, LOCATION_SHELL_OPT_CLOUD_RESP_DISABLED },
	{ "wifi_timeout", required_argument, 0, LOCATION_SHELL_OPT_WIFI_TIMEOUT },
	{ "wifi_service", required_argument, 0, LOCATION_SHELL_OPT_WIFI_SERVICE },
	{ "help", no_argument, 0, 'h' },
	{ 0, 0, 0, 0 }
};

static enum location_service location_shell_string_to_service(const char *service_str)
{
	enum location_service service = MOSH_LOC_SERVICE_NONE;

	if (strcmp(service_str, "any") == 0) {
		service = LOCATION_SERVICE_ANY;
	} else if (strcmp(service_str, "nrf") == 0) {
		service = LOCATION_SERVICE_NRF_CLOUD;
	} else if (strcmp(service_str, "here") == 0) {
		service = LOCATION_SERVICE_HERE;
	}

	return service;
}

#if defined(CONFIG_DK_LIBRARY)
static void location_evt_led_off_work_fn(struct k_work *work_item)
{
	dk_set_led_off(LOCATION_STATUS_LED);
}
#endif

static int location_cloud_send(char *body)
{
	int ret;

#if defined(CONFIG_NRF_CLOUD_MQTT)
	struct nrf_cloud_tx_data mqtt_msg = {
		.data.ptr = body,
		.data.len = strlen(body),
		.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		.topic_type = NRF_CLOUD_TOPIC_MESSAGE,
	};

	ret = nrf_cloud_send(&mqtt_msg);
	if (ret) {
		mosh_error("MQTT: location data sending failed: %d", ret);
	}
#elif defined(CONFIG_NRF_CLOUD_COAP)
	ret = nrf_cloud_coap_json_message_send(body, false, true);
	if (ret) {
		mosh_error("CoAP: location data sending failed");
	}
#elif defined(CONFIG_NRF_CLOUD_REST)
#define REST_DETAILS_RX_BUF_SZ 300 /* No payload in a response, "just" headers */
	static char rx_buf[REST_DETAILS_RX_BUF_SZ];
	static char device_id[NRF_CLOUD_CLIENT_ID_MAX_LEN + 1];
	static struct nrf_cloud_rest_context rest_ctx = {
		.connect_socket = -1,
		.keep_alive = false,
		.rx_buf = rx_buf,
		.rx_buf_len = sizeof(rx_buf),
		.fragment_size = 0
	};

	ret = nrf_cloud_client_id_get(device_id, sizeof(device_id));
	if (ret == 0) {
		ret = nrf_cloud_rest_send_device_message(&rest_ctx, device_id, body, false, NULL);
		if (ret) {
			mosh_error("REST: location data sending failed: %d", ret);
		}
	} else {
		mosh_error("Failed to get device ID, error: %d", ret);
	}
#endif
	return ret;
}

static void location_cloud_work_fn(struct k_work *work_item)
{
	struct location_cloud_work_data *data =
		CONTAINER_OF(work_item, struct location_cloud_work_data, work);
	int ret;
	char *body_gnss = NULL;

	if (data->send_cloud_gnss) {
		/* Encode json payload in requested format, either NMEA or in PVT */
		ret = location_cmd_utils_gnss_loc_to_cloud_payload_json_encode(
			data->format,
			&data->loc_evt_data.location,
			data->timestamp_ms,
			&body_gnss);
		if (ret) {
			mosh_error("Failed to generate nRF Cloud NMEA or PVT, err: %d", ret);
		} else {
			mosh_print(
				"Sending acquired GNSS location to nRF Cloud, body: %s",
				body_gnss);
			location_cloud_send(body_gnss);
		}
		if (body_gnss) {
			cJSON_free(body_gnss);
		}
	}

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	char *body_details = NULL;

	if (data->send_cloud_details) {
		ret = location_details_json_payload_encode(
			&data->loc_evt_data,
			data->timestamp_ms,
			data->mosh_cmd,
			&body_details);
		if (ret) {
			mosh_error("location details json encoding failed: %d", ret);
		} else {
			mosh_print(
				"Sending location related details to nRF Cloud, body: %s",
				body_details);
			location_cloud_send(body_details);
		}
		if (body_details) {
			cJSON_free(body_details);
		}
	}
#endif
}

#if defined(CONFIG_LOCATION_DATA_DETAILS)
static void location_print_data_details(
	enum location_method method,
	const struct location_data_details *details)
{
	mosh_print("  elapsed method time: %d ms", details->elapsed_time_method);
#if defined(CONFIG_LOCATION_METHOD_GNSS)
	if (method == LOCATION_METHOD_GNSS) {
		mosh_print("  satellites tracked: %d", details->gnss.satellites_tracked);
		mosh_print("  satellites used: %d", details->gnss.satellites_used);
		mosh_print("  elapsed GNSS time: %d ms", details->gnss.elapsed_time_gnss);
		mosh_print("  GNSS execution time: %d ms", details->gnss.pvt_data.execution_time);
	}
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
	if (method == LOCATION_METHOD_CELLULAR || method == LOCATION_METHOD_WIFI_CELLULAR) {
		mosh_print("  neighbor cells: %d", details->cellular.ncells_count);
		mosh_print("  GCI cells: %d", details->cellular.gci_cells_count);
	}
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI)
	if (method == LOCATION_METHOD_WIFI || method == LOCATION_METHOD_WIFI_CELLULAR) {
		mosh_print("  Wi-Fi APs: %d", details->wifi.ap_count);
	}
#endif
}
#endif

void location_ctrl_event_handler(const struct location_event_data *event_data)
{
	int64_t ts_ms = NRF_CLOUD_NO_TIMESTAMP;

	location_cloud_work_data.send_cloud_details = false;
	location_cloud_work_data.send_cloud_gnss = false;

#if defined(CONFIG_DATE_TIME)
	date_time_now(&ts_ms);
#endif

	switch (event_data->id) {
	case LOCATION_EVT_LOCATION:
		location_cloud_work_data.send_cloud_details = arg_cloud_details;
		mosh_print("Location:");
		mosh_print(
			"  used method: %s (%d)",
			location_method_str(event_data->method),
			event_data->method);
		mosh_print("  latitude: %.06f", event_data->location.latitude);
		mosh_print("  longitude: %.06f", event_data->location.longitude);
		mosh_print("  accuracy: %.01f m", (double)event_data->location.accuracy);
		if (event_data->location.datetime.valid) {
			mosh_print(
				"  date: %04d-%02d-%02d",
				event_data->location.datetime.year,
				event_data->location.datetime.month,
				event_data->location.datetime.day);
			mosh_print(
				"  time: %02d:%02d:%02d.%03d UTC",
				event_data->location.datetime.hour,
				event_data->location.datetime.minute,
				event_data->location.datetime.second,
				event_data->location.datetime.ms);
		}
		mosh_print(
			"  Google maps URL: https://maps.google.com/?q=%f,%f",
			event_data->location.latitude, event_data->location.longitude);

#if defined(CONFIG_LOCATION_DATA_DETAILS)
		location_print_data_details(event_data->method, &event_data->location.details);
#endif
		if (event_data->method == LOCATION_METHOD_GNSS) {
			location_cloud_work_data.send_cloud_gnss = arg_cloud_gnss;
		}

#if defined(CONFIG_DK_LIBRARY)
		dk_set_led_on(LOCATION_STATUS_LED);
		k_work_reschedule(&location_evt_led_off_work, K_SECONDS(5));
#endif
		break;

	case LOCATION_EVT_TIMEOUT:
		location_cloud_work_data.send_cloud_details = arg_cloud_details;
		mosh_error("Location request timed out:");
		mosh_print(
			"  used method: %s (%d)",
			location_method_str(event_data->method),
			event_data->method);
#if defined(CONFIG_LOCATION_DATA_DETAILS)
		location_print_data_details(event_data->method, &event_data->error.details);
#endif
		break;

	case LOCATION_EVT_ERROR:
		location_cloud_work_data.send_cloud_details = arg_cloud_details;
		mosh_error("Location request failed:");
		mosh_print(
			"  used method: %s (%d)",
			location_method_str(event_data->method),
			event_data->method);
#if defined(CONFIG_LOCATION_DATA_DETAILS)
		location_print_data_details(event_data->method, &event_data->error.details);
#endif
		break;
#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
#if defined(CONFIG_NRF_CLOUD_AGNSS)
	case LOCATION_EVT_GNSS_ASSISTANCE_REQUEST: {
		char flags_string[48];

		agnss_data_flags_str_get(flags_string, event_data->agnss_request.data_flags);
		mosh_print("A-GNSS request from Location library: flags: %s", flags_string);
		for (int i = 0; i < event_data->agnss_request.system_count; i++) {
			mosh_print(
				"A-GNSS request from Location library: "
				"%s sv_mask_ephe: 0x%llx, sv_mask_alm: 0x%llx",
				gnss_system_str_get(event_data->agnss_request.system[i].system_id),
				event_data->agnss_request.system[i].sv_mask_ephe,
				event_data->agnss_request.system[i].sv_mask_alm);
		}
		location_srv_ext_agnss_handle(&event_data->agnss_request);
		break;
	}
#endif
#if defined(CONFIG_NRF_CLOUD_PGPS)
	case LOCATION_EVT_GNSS_PREDICTION_REQUEST:
		mosh_print(
			"P-GPS request from Location library "
			"(prediction count: %d validity time: %d gps day: %d time of day: %d)",
			event_data->pgps_request.prediction_count,
			event_data->pgps_request.prediction_period_min,
			event_data->pgps_request.gps_day,
			event_data->pgps_request.gps_time_of_day);
		location_srv_ext_pgps_handle(&event_data->pgps_request);
		break;
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR) || defined(CONFIG_LOCATION_METHOD_WIFI)
	case LOCATION_EVT_CLOUD_LOCATION_EXT_REQUEST:
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
		if (event_data->cloud_location_request.cell_data != NULL) {
			mosh_print(
				"Cloud positioning request from Location library "
				"(neighbor cells: %d GCI cells: %d)",
				event_data->cloud_location_request.cell_data->ncells_count,
				event_data->cloud_location_request.cell_data->gci_cells_count);
		}
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI)
		if (event_data->cloud_location_request.wifi_data != NULL) {
			mosh_print(
				"Cloud positioning request from Location library (access points: %d)",
				event_data->cloud_location_request.wifi_data->cnt);
		}
#endif
		location_srv_ext_cloud_location_handle(
			&event_data->cloud_location_request, arg_cloud_resp_enabled);
		break;
#endif
#endif /* defined(CONFIG_LOCATION_SERVICE_EXTERNAL) */
	case LOCATION_EVT_RESULT_UNKNOWN:
		location_cloud_work_data.send_cloud_details = arg_cloud_details;
		mosh_print("Location request completed, but the result is not known:");
		mosh_print(
			"  used method: %s (%d)",
			location_method_str(event_data->method),
			event_data->method);
#if defined(CONFIG_LOCATION_DATA_DETAILS)
		location_print_data_details(event_data->method, &event_data->error.details);
#endif
		break;
#if defined(CONFIG_LOCATION_DATA_DETAILS)
	case LOCATION_EVT_STARTED:
		mosh_print("Location request has been started:");
		mosh_print(
			"  used method: %s (%d)",
			location_method_str(event_data->method),
			event_data->method);
		break;
	case LOCATION_EVT_FALLBACK:
		location_cloud_work_data.send_cloud_details = arg_cloud_details;
		mosh_print("Location request fallback has occurred:");
		mosh_print(
			"  failed method: %s (%d)",
			location_method_str(event_data->method),
			event_data->method);
		mosh_print(
			"  new method: %s (%d)",
			location_method_str(event_data->fallback.next_method),
			event_data->fallback.next_method);
		mosh_print(
			"  cause: %s",
			(event_data->fallback.cause == LOCATION_EVT_TIMEOUT) ? "timeout" :
			(event_data->fallback.cause == LOCATION_EVT_ERROR) ? "error" :
			"unknown");
		location_print_data_details(event_data->method, &event_data->fallback.details);
		break;
#endif
	default:
		mosh_warn("Unknown event from location library, id %d", event_data->id);
		break;
	}

	if (location_cloud_work_data.send_cloud_gnss ||
	    location_cloud_work_data.send_cloud_details) {

		if (k_work_busy_get(&location_cloud_work_data.work) == 0) {
			location_cloud_work_data.loc_evt_data = *event_data;
			location_cloud_work_data.timestamp_ms = ts_ms;
			location_cloud_work_data.format = arg_cloud_gnss_format;
			/* location_cloud_work_data.mosh_cmd set when location command is issued */

			k_work_submit_to_queue(&mosh_common_work_q, &location_cloud_work_data.work);
		} else {
			mosh_warn(
				"Sending previous location data to cloud still ongoing. "
				"New location data ignored.");
		}
	}
}

void location_ctrl_init(void)
{
	int ret;

	k_work_init(&location_cloud_work_data.work, location_cloud_work_fn);

#if defined(CONFIG_DK_LIBRARY)
	k_work_init_delayable(&location_evt_led_off_work, location_evt_led_off_work_fn);
#endif

	ret = location_init(location_ctrl_event_handler);
	if (ret) {
		mosh_error("Initializing the Location library failed, err: %d\n", ret);
	}
}

static int cmd_location_get(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;

	enum location_method method_list[CONFIG_LOCATION_METHODS_LIST_SIZE] = { 0 };
	int method_count = 0;
	int interval = 0;
	bool interval_set = false;
	float timeout = 0;
	bool timeout_set = false;
	enum location_req_mode req_mode = LOCATION_REQ_MODE_FALLBACK;

	float gnss_timeout = 0;
	bool gnss_timeout_set = false;
	enum location_accuracy gnss_accuracy = 0;
	bool gnss_accuracy_set = false;
	int gnss_num_fixes = 0;
	bool gnss_num_fixes_set = false;
	bool gnss_visibility = false;
	bool gnss_priority_mode = false;

	float cellular_timeout = 0;
	bool cellular_timeout_set = false;
	enum location_service cellular_service = LOCATION_SERVICE_ANY;
	int cellular_cell_count = 0;
	bool cellular_cell_count_set = false;

	float wifi_timeout = 0;
	bool wifi_timeout_set = false;
	enum location_service wifi_service = LOCATION_SERVICE_ANY;

	arg_cloud_gnss_format = NRF_CLOUD_GNSS_TYPE_PVT;
	arg_cloud_gnss = false;
	arg_cloud_details = false;
	arg_cloud_resp_enabled = true;

	optreset = 1;
	optind = 1;
	int opt;

	while ((opt = getopt_long(argc, argv, "m:t:h", long_options, NULL)) != -1) {
		switch (opt) {
		case LOCATION_SHELL_OPT_GNSS_TIMEOUT:
			gnss_timeout = atof(optarg);
			gnss_timeout_set = true;
			break;

		case LOCATION_SHELL_OPT_GNSS_NUM_FIXES:
			gnss_num_fixes = atoi(optarg);
			gnss_num_fixes_set = true;
			break;
		case LOCATION_SHELL_OPT_GNSS_LOC_CLOUD_NMEA:
			arg_cloud_gnss_format = NRF_CLOUD_GNSS_TYPE_NMEA;
		/* flow-through */
		case LOCATION_SHELL_OPT_GNSS_LOC_CLOUD_PVT:
			arg_cloud_gnss = true;
			break;
		case LOCATION_SHELL_OPT_GNSS_LOC_CLOUD_DETAILS:
#if defined(CONFIG_LOCATION_DATA_DETAILS)
			arg_cloud_details = true;
#else
			mosh_error(
				"Option --cloud_details is not supported "
				"when CONFIG_LOCATION_DATA_DETAILS is disabled");
			goto show_usage;
#endif
			break;

		case LOCATION_SHELL_OPT_CELLULAR_TIMEOUT:
			cellular_timeout = atof(optarg);
			cellular_timeout_set = true;
			break;
		case LOCATION_SHELL_OPT_CELLULAR_SERVICE:
			cellular_service = location_shell_string_to_service(optarg);
			if (cellular_service == MOSH_LOC_SERVICE_NONE) {
				mosh_error("Unknown cellular positioning service. See usage:");
				goto show_usage;
			}
			break;
		case LOCATION_SHELL_OPT_CELLULAR_CELL_COUNT:
			cellular_cell_count = atoi(optarg);
			cellular_cell_count_set = true;
			break;
		case LOCATION_SHELL_OPT_CLOUD_RESP_DISABLED:
#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
			arg_cloud_resp_enabled = false;
#else
			mosh_error(
				"Option --cloud_resp_disabled is not supported "
				"when CONFIG_LOCATION_SERVICE_EXTERNAL is disabled");
			goto show_usage;
#endif
			break;

		case LOCATION_SHELL_OPT_WIFI_TIMEOUT:
			wifi_timeout = atof(optarg);
			wifi_timeout_set = true;
			break;

		case LOCATION_SHELL_OPT_WIFI_SERVICE:
			wifi_service = location_shell_string_to_service(optarg);
			if (wifi_service == MOSH_LOC_SERVICE_NONE) {
				mosh_error("Unknown Wi-Fi positioning service. See usage:");
				goto show_usage;
			}
			break;

		case LOCATION_SHELL_OPT_INTERVAL:
			interval = atoi(optarg);
			interval_set = true;
			break;
		case 't':
			timeout = atof(optarg);
			timeout_set = true;
			break;

		case LOCATION_SHELL_OPT_GNSS_ACCURACY:
			if (strcmp(optarg, "low") == 0) {
				gnss_accuracy = LOCATION_ACCURACY_LOW;
			} else if (strcmp(optarg, "normal") == 0) {
				gnss_accuracy = LOCATION_ACCURACY_NORMAL;
			} else if (strcmp(optarg, "high") == 0) {
				gnss_accuracy = LOCATION_ACCURACY_HIGH;
			} else {
				mosh_error("Unknown GNSS accuracy. See usage:");
				goto show_usage;
			}
			gnss_accuracy_set = true;
			break;

		case LOCATION_SHELL_OPT_GNSS_VISIBILITY:
			gnss_visibility = true;
			break;

		case LOCATION_SHELL_OPT_GNSS_PRIORITY_MODE:
			gnss_priority_mode = true;
			break;

		case LOCATION_SHELL_OPT_MODE:
			if (strcmp(optarg, "fallback") == 0) {
				req_mode = LOCATION_REQ_MODE_FALLBACK;
			} else if (strcmp(optarg, "all") == 0) {
				req_mode = LOCATION_REQ_MODE_ALL;
			} else {
				mosh_error(
					"Unknown location request mode (%s) was given. See usage:",
						optarg);
				goto show_usage;
			}
			break;

		case 'm':
			if (method_count >= CONFIG_LOCATION_METHODS_LIST_SIZE) {
				mosh_error(
					"Maximum number of location methods (%d) exceeded. "
					"Location method (%s) still given.",
					CONFIG_LOCATION_METHODS_LIST_SIZE, optarg);
				return -EINVAL;
			}

			if (strcmp(optarg, "cellular") == 0) {
				method_list[method_count] = LOCATION_METHOD_CELLULAR;
			} else if (strcmp(optarg, "gnss") == 0) {
				method_list[method_count] = LOCATION_METHOD_GNSS;
			} else if (strcmp(optarg, "wifi") == 0) {
				method_list[method_count] = LOCATION_METHOD_WIFI;
			} else {
				mosh_error("Unknown method (%s) given. See usage:", optarg);
				goto show_usage;
			}
			method_count++;
			break;

		case 'h':
			goto show_usage;
		case '?':
		default:
			mosh_error("Unknown option (%s). See usage:", argv[optind - 1]);
			goto show_usage;
		}
	}

	if (optind < argc) {
		mosh_error("Arguments without '-' not supported: %s", argv[argc - 1]);
		goto show_usage;
	}

	struct location_config config = { 0 };
	struct location_config *real_config = &config;

	if (method_count == 0 && !interval_set && !timeout_set &&
		req_mode == LOCATION_REQ_MODE_FALLBACK) {
		/* No methods or top level config given. Use default config. */
		real_config = NULL;
	}

	location_config_defaults_set(&config, method_count, method_list);

	for (uint8_t i = 0; i < method_count; i++) {
		if (config.methods[i].method == LOCATION_METHOD_GNSS) {
			if (gnss_timeout_set) {
				config.methods[i].gnss.timeout = (gnss_timeout == 0) ?
					SYS_FOREVER_MS : gnss_timeout * MSEC_PER_SEC;
			}
			if (gnss_accuracy_set) {
				config.methods[i].gnss.accuracy = gnss_accuracy;
			}
			if (gnss_num_fixes_set) {
				config.methods[i].gnss.num_consecutive_fixes =
					gnss_num_fixes;
			}
			config.methods[i].gnss.visibility_detection = gnss_visibility;
			config.methods[i].gnss.priority_mode = gnss_priority_mode;
		} else if (config.methods[i].method == LOCATION_METHOD_CELLULAR) {
			config.methods[i].cellular.service = cellular_service;
			if (cellular_timeout_set) {
				config.methods[i].cellular.timeout =
					(cellular_timeout == 0) ?
					SYS_FOREVER_MS : cellular_timeout * MSEC_PER_SEC;
			}
			if (cellular_cell_count_set) {
				config.methods[i].cellular.cell_count = cellular_cell_count;
			}
		} else if (config.methods[i].method == LOCATION_METHOD_WIFI) {
			config.methods[i].wifi.service = wifi_service;
			if (wifi_timeout_set) {
				config.methods[i].wifi.timeout = (wifi_timeout == 0) ?
					SYS_FOREVER_MS : wifi_timeout * MSEC_PER_SEC;
			}
		}
	}

	if (interval_set) {
		config.interval = interval;
	}
	if (timeout_set) {
		config.timeout = (timeout == 0) ?
			SYS_FOREVER_MS : timeout * MSEC_PER_SEC;
	}
	config.mode = req_mode;

	ret = location_request(real_config);
	if (ret) {
		mosh_error("Requesting location failed, err: %d", ret);
		return -1;
	}

#if defined(CONFIG_LOCATION_DATA_DETAILS)
	/* Store current command */
	shell_command_str_from_argv(
		argc,
		argv,
		"location ",
		location_cloud_work_data.mosh_cmd,
		LOCATION_DETAILS_CMD_STR_MAX_LEN + 1);
#endif

	mosh_print("Started to get current location...");
	return ret;

show_usage:
	mosh_print_no_format(location_get_usage_str);
	return ret;
}

static int cmd_location_cancel(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	ret = location_request_cancel();
	if (ret) {
		mosh_error("Canceling location request failed, err: %d", ret);
	} else {
		mosh_print("Location request cancelled");
	}

	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_location,
	SHELL_CMD_ARG(
		get, NULL,
		"Requests the current position or starts periodic position updates.",
		cmd_location_get, 0, 20),
	SHELL_CMD_ARG(
		cancel, NULL,
		"Cancel/stop on going request. No options.",
		cmd_location_cancel, 1, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(
	location,
	&sub_location,
	"Commands for using the Location library.",
	mosh_print_help_shell);
