/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <shell/shell.h>

#include <getopt.h>

#include <modem/location.h>

#include "mosh_print.h"

enum location_shell_command {
	LOCATION_CMD_NONE = 0,
	LOCATION_CMD_GET,
	LOCATION_CMD_CANCEL
};

/******************************************************************************/
static const char location_usage_str[] =
	"Usage: location <subcommand> [options]\n"
	"\n"
	"Subcommands:\n"
	"  get:      Requests the current position or starts periodic position updates.\n"
	"  cancel:   Cancel/stop on going request. No options\n";

static const char location_get_usage_str[] =
	"Usage: location get [--method <method>] [--interval <secs>]\n"
	"[--gnss_accuracy <acc>] [--gnss_timeout <timeout in secs>]\n"
	"[--cellular_timeout <timeout in secs>] [--cellular_service <service_string>]\n"
	"[--wifi_timeout <timeout in secs>] [--wifi_service <service_string>]\n"
	"Options:\n"
	"  --method,           Location method: 'gnss', 'cellular' or 'wifi'. Multiple\n"
	"                      '--method' parameters may be given to indicate list of\n"
	"                      methods in priority order.\n"
	"  --interval,         Position update interval in seconds\n"
	"                      (default: 0 = single position)\n"
	"  --gnss_accuracy,    Used GNSS accuracy: 'low' or 'normal'\n"
	"  --gnss_timeout,     GNSS timeout in seconds\n"
	"  --cellular_timeout, Cellular timeout in seconds\n"
	"  --cellular_service, Used cellular positioning service:\n"
	"                      'any' (default), 'nrf', 'skyhook', 'here' or 'polte'\n"
	"  --wifi_timeout,     Wi-Fi timeout in seconds\n"
	"  --wifi_service,     Used Wi-Fi positioning service:\n"
	"                      'any' (default), 'nrf', 'skyhook' or 'here'\n";

/******************************************************************************/

/* Following are not having short options */
enum {
	LOCATION_SHELL_OPT_INTERVAL         = 1001,
	LOCATION_SHELL_OPT_GNSS_ACCURACY    = 1002,
	LOCATION_SHELL_OPT_GNSS_TIMEOUT     = 1003,
	LOCATION_SHELL_OPT_CELLULAR_TIMEOUT = 1004,
	LOCATION_SHELL_OPT_CELLULAR_SERVICE = 1005,
	LOCATION_SHELL_OPT_WIFI_TIMEOUT     = 1006,
	LOCATION_SHELL_OPT_WIFI_SERVICE     = 1007,
};

/* Specifying the expected options */
static struct option long_options[] = {
	{ "method", required_argument, 0, 'm' },
	{ "interval", required_argument, 0, LOCATION_SHELL_OPT_INTERVAL },
	{ "gnss_accuracy", required_argument, 0, LOCATION_SHELL_OPT_GNSS_ACCURACY },
	{ "gnss_timeout", required_argument, 0, LOCATION_SHELL_OPT_GNSS_TIMEOUT },
	{ "cellular_timeout", required_argument, 0, LOCATION_SHELL_OPT_CELLULAR_TIMEOUT },
	{ "cellular_service", required_argument, 0, LOCATION_SHELL_OPT_CELLULAR_SERVICE },
	{ "wifi_timeout", required_argument, 0, LOCATION_SHELL_OPT_WIFI_TIMEOUT },
	{ "wifi_service", required_argument, 0, LOCATION_SHELL_OPT_WIFI_SERVICE },
	{ 0, 0, 0, 0 }
};

/******************************************************************************/

static void location_shell_print_usage(const struct shell *shell,
				       enum location_shell_command command)
{
	switch (command) {
	case LOCATION_CMD_GET:
		mosh_print_no_format(location_get_usage_str);
		break;

	default:
		mosh_print_no_format(location_usage_str);
		break;
	}
}

#define MOSH_LOC_SERVICE_NONE 0xFF

static enum location_service location_shell_string_to_service(const char *service_str)
{
	enum location_service service = MOSH_LOC_SERVICE_NONE;

	if (strcmp(service_str, "any") == 0) {
		service = LOCATION_SERVICE_ANY;
	} else if (strcmp(service_str, "nrf") == 0) {
		service = LOCATION_SERVICE_NRF_CLOUD;
	} else if (strcmp(service_str, "here") == 0) {
		service = LOCATION_SERVICE_HERE;
	} else if (strcmp(service_str, "skyhook") == 0) {
		service = LOCATION_SERVICE_SKYHOOK;
	} else if (strcmp(service_str, "polte") == 0) {
		service = LOCATION_SERVICE_POLTE;
	}

	return service;
}

/******************************************************************************/

void location_ctrl_event_handler(const struct location_event_data *event_data)
{
	switch (event_data->id) {
	case LOCATION_EVT_LOCATION:
		mosh_print("Location:");
		mosh_print(
			"  used method: %s (%d)",
			location_method_str(event_data->location.method),
			event_data->location.method);
		mosh_print("  latitude: %.06f", event_data->location.latitude);
		mosh_print("  longitude: %.06f", event_data->location.longitude);
		mosh_print("  accuracy: %.01f m", event_data->location.accuracy);
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
		break;

	case LOCATION_EVT_TIMEOUT:
		mosh_error("Location request timed out");
		break;

	case LOCATION_EVT_ERROR:
		mosh_error("Location request failed");
		break;
	case LOCATION_EVT_GNSS_ASSISTANCE_REQUEST:
#if defined(CONFIG_LOCATION_METHOD_GNSS_AGPS_EXTERNAL)
		mosh_print(
			"MoSh: A-GPS request from Location library "
			"(ephe: 0x%08x alm: 0x%08x flags: 0x%02x)",
			event_data->request.sv_mask_ephe,
			event_data->request.sv_mask_alm,
			event_data->request.data_flags);
#endif
		break;
	}
}

void location_ctrl_init(void)
{
	int ret;

	ret = location_init(location_ctrl_event_handler);
	if (ret) {
		mosh_error("Initializing the Location library failed, err: %d\n", ret);
	}
}

/******************************************************************************/

int location_shell(const struct shell *shell, size_t argc, char **argv)
{
	enum location_shell_command command = LOCATION_CMD_NONE;

	int interval = 0;
	bool interval_set = false;

	int gnss_timeout = 0;
	bool gnss_timeout_set = false;

	enum location_accuracy gnss_accuracy = 0;
	bool gnss_accuracy_set = false;

	int cellular_timeout = 0;
	bool cellular_timeout_set = false;
	enum location_service cellular_service = LOCATION_SERVICE_ANY;

	int wifi_timeout = 0;
	bool wifi_timeout_set = false;
	enum location_service wifi_service = LOCATION_SERVICE_ANY;

	enum location_method method_list[CONFIG_LOCATION_METHODS_LIST_SIZE] = { 0 };
	int method_count = 0;

	int opt;
	int ret = 0;
	int long_index = 0;

	if (argc < 2) {
		goto show_usage;
	}

	/* command = argv[0] = "location" */
	/* sub-command = argv[1] */
	if (strcmp(argv[1], "get") == 0) {
		command = LOCATION_CMD_GET;
	} else if (strcmp(argv[1], "cancel") == 0) {
		command = LOCATION_CMD_CANCEL;
	} else {
		mosh_error("Unsupported command=%s\n", argv[1]);
		ret = -EINVAL;
		goto show_usage;
	}

	/* Reset getopt due to possible previous failures */
	optreset = 1;

	/* We start from subcmd arguments */
	optind = 2;

	while ((opt = getopt_long(argc, argv, "m:", long_options, &long_index)) != -1) {
		switch (opt) {
		case LOCATION_SHELL_OPT_GNSS_TIMEOUT:
			gnss_timeout = atoi(optarg);
			gnss_timeout_set = true;
			break;

		case LOCATION_SHELL_OPT_CELLULAR_TIMEOUT:
			cellular_timeout = atoi(optarg);
			cellular_timeout_set = true;
			break;

		case LOCATION_SHELL_OPT_CELLULAR_SERVICE:
			cellular_service = location_shell_string_to_service(optarg);
			if (cellular_service == MOSH_LOC_SERVICE_NONE) {
				mosh_error("Unknown cellular positioning service. See usage:");
				goto show_usage;
			}
			break;

		case LOCATION_SHELL_OPT_WIFI_TIMEOUT:
			wifi_timeout = atoi(optarg);
			wifi_timeout_set = true;
			break;

		case LOCATION_SHELL_OPT_WIFI_SERVICE:
			wifi_service = location_shell_string_to_service(optarg);
			if (wifi_service == MOSH_LOC_SERVICE_NONE ||
			    wifi_service == LOCATION_SERVICE_POLTE) {
				mosh_error("Unknown Wi-Fi positioning service. See usage:");
				goto show_usage;
			}
			break;

		case LOCATION_SHELL_OPT_INTERVAL:
			interval = atoi(optarg);
			interval_set = true;
			break;

		case LOCATION_SHELL_OPT_GNSS_ACCURACY:
			if (strcmp(optarg, "low") == 0) {
				gnss_accuracy = LOCATION_ACCURACY_LOW;
			} else if (strcmp(optarg, "normal") == 0) {
				gnss_accuracy = LOCATION_ACCURACY_NORMAL;
			} else {
				mosh_error("Unknown GNSS accuracy. See usage:");
				goto show_usage;
			}
			gnss_accuracy_set = true;
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

		case '?':
			goto show_usage;

		default:
			mosh_error("Unknown option. See usage:");
			goto show_usage;
		}
	}

	/* Handle location subcommands */
	switch (command) {
	case LOCATION_CMD_CANCEL:
		ret = location_request_cancel();
		if (ret) {
			mosh_error("Canceling location request failed, err: %d", ret);
			return -1;
		}
		mosh_print("Location request cancelled");
		break;

	case LOCATION_CMD_GET: {
		struct location_config config = { 0 };
		struct location_config *real_config = &config;

		if (method_count == 0 && !interval_set) {
			/* No methods or top level config given. Use default config. */
			real_config = NULL;
		}

		location_config_defaults_set(&config, method_count, method_list);

		for (uint8_t i = 0; i < method_count; i++) {
			if (config.methods[i].method == LOCATION_METHOD_GNSS) {
				if (gnss_timeout_set) {
					config.methods[i].gnss.timeout = gnss_timeout;
				}
				if (gnss_accuracy_set) {
					config.methods[i].gnss.accuracy = gnss_accuracy;
				}
			} else if (config.methods[i].method == LOCATION_METHOD_CELLULAR) {
				config.methods[i].cellular.service = cellular_service;
				if (cellular_timeout_set) {
					config.methods[i].cellular.timeout = cellular_timeout;
				}
			} else if (config.methods[i].method == LOCATION_METHOD_WIFI) {
				config.methods[i].wifi.service = wifi_service;
				if (wifi_timeout_set) {
					config.methods[i].wifi.timeout = wifi_timeout;
				}
			}
		}

		if (interval_set) {
			config.interval = interval;
		}

		ret = location_request(real_config);
		if (ret) {
			mosh_error("Requesting location failed, err: %d", ret);
			return -1;
		}
		mosh_print("Started to get current location...");
		break;
	}
	default:
		mosh_error("Unknown command. See usage:");
		goto show_usage;
	}
	return ret;
show_usage:
	/* Reset getopt for another users */
	optreset = 1;

	location_shell_print_usage(shell, command);
	return ret;
}
