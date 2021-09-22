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

extern const struct shell *shell_global;

enum location_shell_command {
	LOCATION_CMD_NONE = 0,
	LOCATION_CMD_GET,
	LOCATION_CMD_CANCEL
};

struct location_shell_cmd_args {
	enum location_shell_command command;
};

/******************************************************************************/
static const char location_usage_str[] =
	"Usage: location <subcommand> [options]\n"
	"\n"
	"Subcommands:\n"
	"  get:      Requests the current position or starts periodic position updates.\n"
	"  cancel:   Cancel/Stop on going request. No options\n";

static const char location_get_usage_str[] =
	"Usage: location get [--method <method>] [--interval <secs>]\n"
	"[--gnss_accuracy <acc>] [--gnss_num_fixes <number of fixes>]\n"
	"[--gnss_timeout <timeout in secs>] [--cellular_timeout <timeout in secs>]\n"
	"[--wlan_timeout <timeout in secs>] [--wlan_service <service_string>]\n"
	"Options:\n"
	"  --method,           Location method: 'gnss', 'cellular' or 'wlan'. Multiple\n"
	"                      '--method' parameters may be given to indicate list of\n"
	"                      methods in priority order.\n"
	"  --interval,         Position update interval in seconds (default: 0, = single position)\n"
	"  --gnss_accuracy,    Used GNSS accuracy: 'low' or 'normal' or 'high'\n"
	"  --gnss_num_fixes,   Number of consecutive fix attempts (if gnss_accuracy set to 'high',\n"
	"                      default: 2)\n"
	"  --gnss_timeout,     GNSS timeout in seconds\n"
	"  --cellular_timeout, Cellular timeout in seconds\n"
	"  --wlan_timeout,     WLAN timeout in seconds\n"
	"  --wlan_service,     Used WLAN positioning service: 'nrf' (default), 'skyhook' or 'here'\n";

/******************************************************************************/

/* Following are not having short options: */
enum {  LOCATION_SHELL_OPT_METHOD           = 1001,
	LOCATION_SHELL_OPT_INTERVAL         = 1002,
	LOCATION_SHELL_OPT_GNSS_ACCURACY    = 1003,
	LOCATION_SHELL_OPT_GNSS_TIMEOUT     = 1004,
	LOCATION_SHELL_OPT_GNSS_NUM_FIXES   = 1005,
	LOCATION_SHELL_OPT_CELLULAR_TIMEOUT = 1006,
	LOCATION_SHELL_OPT_WLAN_TIMEOUT     = 1007,
	LOCATION_SHELL_OPT_WLAN_SERVICE     = 1008,
};

/* Specifying the expected options: */
static struct option long_options[] = {
	{ "method", required_argument, 0, LOCATION_SHELL_OPT_METHOD },
	{ "interval", required_argument, 0, LOCATION_SHELL_OPT_INTERVAL },
	{ "gnss_accuracy", required_argument, 0, LOCATION_SHELL_OPT_GNSS_ACCURACY },
	{ "gnss_timeout", required_argument, 0, LOCATION_SHELL_OPT_GNSS_TIMEOUT },
	{ "gnss_num_fixes", required_argument, 0, LOCATION_SHELL_OPT_GNSS_NUM_FIXES },
	{ "cellular_timeout", required_argument, 0, LOCATION_SHELL_OPT_CELLULAR_TIMEOUT },
	{ "wlan_timeout", required_argument, 0, LOCATION_SHELL_OPT_WLAN_TIMEOUT },
	{ "wlan_service", required_argument, 0, LOCATION_SHELL_OPT_WLAN_SERVICE },
	{ 0, 0, 0, 0 }
};

/******************************************************************************/

static void location_shell_print_usage(const struct shell *shell,
				       struct location_shell_cmd_args *loc_cmd_args)
{
	switch (loc_cmd_args->command) {
	case LOCATION_CMD_GET:
		shell_print(shell, "%s", location_get_usage_str);
		break;

	default:
		shell_print(shell, "%s", location_usage_str);
		break;
	}
}

static const char *location_shell_method_to_string(int method, char *out_str_buff)
{
	switch (method) {
	case LOC_METHOD_CELLULAR:
		strcpy(out_str_buff, "Cellular");
		break;
	case LOC_METHOD_GNSS:
		strcpy(out_str_buff, "GNSS");
		break;
	case LOC_METHOD_WLAN:
		strcpy(out_str_buff, "WLAN");
		break;

	default:
		strcpy(out_str_buff, "Unknown");
	}

	return out_str_buff;
}

/******************************************************************************/

void location_ctrl_event_handler(const struct loc_event_data *event_data)
{
	char gmaps_str[512];
	char snum[64];

	switch (event_data->id) {
	case LOC_EVT_LOCATION:
		shell_print(shell_global, "Location:");
		shell_print(shell_global, "  used method: %s (%d)",
			    location_shell_method_to_string(event_data->method, snum),
			    event_data->method);
		if (event_data->method == LOC_METHOD_CELLULAR) {
			shell_print(
				shell_global,
				"  used service: %s",
				CONFIG_MULTICELL_LOCATION_HOSTNAME);
		}
		shell_print(shell_global, "  latitude: %.06f", event_data->location.latitude);
		shell_print(shell_global, "  longitude: %.06f", event_data->location.longitude);
		shell_print(shell_global, "  accuracy: %.01f m", event_data->location.accuracy);
		if (event_data->location.datetime.valid) {
			shell_print(shell_global, "  date: %04d-%02d-%02d",
				    event_data->location.datetime.year,
				    event_data->location.datetime.month,
				    event_data->location.datetime.day);
			shell_print(shell_global, "  time: %02d:%02d:%02d.%03d UTC",
				    event_data->location.datetime.hour,
				    event_data->location.datetime.minute,
				    event_data->location.datetime.second,
				    event_data->location.datetime.ms);
		}
		sprintf(gmaps_str, "  Google maps URL: https://maps.google.com/?q=%f,%f",
			event_data->location.latitude, event_data->location.longitude);
		shell_print(shell_global, "%s", gmaps_str);
		break;

	case LOC_EVT_TIMEOUT:
		shell_error(shell_global, "Timeout happened when getting a location");
		break;

	case LOC_EVT_ERROR:
		shell_error(shell_global, "An error happened during getting a location");
		break;
	}
}

void location_ctrl_init(void)
{
	int ret;

	ret = location_init(location_ctrl_event_handler);
	if (ret) {
		shell_error(shell_global, "Initializing the Location library failed, err: %d\n",
			    ret);
	}
}

/******************************************************************************/

int location_shell(const struct shell *shell, size_t argc, char **argv)
{
	struct location_shell_cmd_args loc_cmd_args = {
		.command = LOCATION_CMD_NONE
	};

	int interval = 0;
	bool interval_set = false;

	int gnss_timeout = 0;
	bool gnss_timeout_set = false;

	int gnss_num_fixes = 0;
	bool gnss_num_fixes_set = false;

	enum loc_accuracy gnss_accuracy = 0;
	bool gnss_accuracy_set = false;

	int cellular_timeout = 0;
	bool cellular_timeout_set = false;

	int wlan_timeout = 0;
	bool wlan_timeout_set = false;

	enum loc_method method_list[LOC_MAX_METHODS] = { 0 };
	int method_count = 0;

	enum loc_wlan_service wlan_service = LOC_WLAN_SERVICE_NRF_CLOUD;

	int opt;
	int ret = 0;
	int long_index = 0;

	if (argc < 2) {
		goto show_usage;
	}

	/* command = argv[0] = "location" */
	/* sub-command = argv[1]       */
	if (strcmp(argv[1], "get") == 0) {
		loc_cmd_args.command = LOCATION_CMD_GET;
	} else if (strcmp(argv[1], "cancel") == 0) {
		loc_cmd_args.command = LOCATION_CMD_CANCEL;
	} else {
		shell_error(shell, "Unsupported command=%s\n", argv[1]);
		ret = -EINVAL;
		goto show_usage;
	}

	/* Reset getopt due to possible previous failures */
	optreset = 1;

	/* We start from subcmd arguments */
	optind = 2;

	while ((opt = getopt_long(argc, argv, "", long_options, &long_index)) != -1) {
		switch (opt) {
		case LOCATION_SHELL_OPT_GNSS_TIMEOUT:
			gnss_timeout = atoi(optarg);
			if (gnss_timeout == 0) {
				shell_error(shell, "GNSS timeout (%d) must be positive integer. ",
					    gnss_timeout);
				return -EINVAL;
			}
			gnss_timeout_set = true;
			break;

		case LOCATION_SHELL_OPT_CELLULAR_TIMEOUT:
			cellular_timeout = atoi(optarg);
			if (cellular_timeout == 0) {
				shell_error(shell,
					    "Cellular timeout (%d) must be positive integer. ",
					    cellular_timeout);
				return -EINVAL;
			}
			cellular_timeout_set = true;
			break;

		case LOCATION_SHELL_OPT_WLAN_TIMEOUT:
			wlan_timeout = atoi(optarg);
			if (wlan_timeout == 0) {
				shell_error(shell,
					    "WLAN timeout (%d) must be positive integer. ",
					    wlan_timeout);
				return -EINVAL;
			}
			wlan_timeout_set = true;
			break;

		case LOCATION_SHELL_OPT_WLAN_SERVICE:
			if (strcmp(optarg, "nrf") == 0) {
				wlan_service = LOC_WLAN_SERVICE_NRF_CLOUD;
			} else if (strcmp(optarg, "here") == 0) {
				wlan_service = LOC_WLAN_SERVICE_HERE;
			} else if (strcmp(optarg, "skyhook") == 0) {
				wlan_service = LOC_WLAN_SERVICE_SKYHOOK;
			} else {
				shell_error(shell, "Unknown WLAN positioning service. See usage:");
				goto show_usage;
			}
			break;
		case LOCATION_SHELL_OPT_INTERVAL:
			interval = atoi(optarg);
			interval_set = true;
			break;

		case LOCATION_SHELL_OPT_GNSS_ACCURACY:
			if (strcmp(optarg, "low") == 0) {
				gnss_accuracy = LOC_ACCURACY_LOW;
			} else if (strcmp(optarg, "normal") == 0) {
				gnss_accuracy = LOC_ACCURACY_NORMAL;
			} else if (strcmp(optarg, "high") == 0) {
				gnss_accuracy = LOC_ACCURACY_HIGH;
			} else {
				shell_error(shell, "Unknown GNSS accuracy. See usage:");
				goto show_usage;
			}
			gnss_accuracy_set = true;
			break;
		case LOCATION_SHELL_OPT_GNSS_NUM_FIXES:
			gnss_num_fixes = atoi(optarg);
			gnss_num_fixes_set = true;
			break;
		case LOCATION_SHELL_OPT_METHOD:
			if (method_count >= LOC_MAX_METHODS) {
				shell_error(shell,
					    "Maximum location methods (%d) exceeded. "
					    "Location method (%s) still given.",
					    LOC_MAX_METHODS, optarg);
				return -EINVAL;
			}

			if (strcmp(optarg, "cellular") == 0) {
				method_list[method_count] = LOC_METHOD_CELLULAR;
			} else if (strcmp(optarg, "gnss") == 0) {
				method_list[method_count] = LOC_METHOD_GNSS;
			} else if (strcmp(optarg, "wlan") == 0) {
				method_list[method_count] = LOC_METHOD_WLAN;
			} else {
				shell_error(shell, "Unknown method (%s) given. See usage:", optarg);
				goto show_usage;
			}
			method_count++;
			break;
		case '?':
			goto show_usage;
		default:
			shell_error(shell, "Unknown option. See usage:");
			goto show_usage;
		}
	}

	/* Handle location subcommands */
	switch (loc_cmd_args.command) {
	case LOCATION_CMD_CANCEL:
		ret = location_request_cancel();
		if (ret) {
			shell_error(shell, "Cannot cancel location request, err: %d", ret);
			return -1;
		}
		shell_print(shell, "Getting of location cancelled");
		break;

	case LOCATION_CMD_GET: {
		struct loc_config config = { 0 };
		struct loc_method_config methods[LOC_MAX_METHODS] = { 0 };
		struct loc_config *real_config = &config;

		if (method_count == 0 && !interval_set) {
			/* No methods or top level config given. Use default config. */
			real_config = NULL;
		}

		loc_config_defaults_set(&config, method_count, methods);

		for (uint8_t i = 0; i < method_count; i++) {
			loc_config_method_defaults_set(&methods[i], method_list[i]);

			if (method_list[i] == LOC_METHOD_GNSS) {
				if (gnss_timeout_set) {
					methods[i].gnss.timeout = gnss_timeout;
				}
				if (gnss_accuracy_set) {
					methods[i].gnss.accuracy = gnss_accuracy;
				}
				if (gnss_num_fixes_set) {
					methods[i].gnss.num_consecutive_fixes = gnss_num_fixes;
				}
			} else if (methods[i].method == LOC_METHOD_CELLULAR) {
				if (cellular_timeout_set) {
					methods[i].cellular.timeout = cellular_timeout;
				}
			} else if (methods[i].method == LOC_METHOD_WLAN) {
				methods[i].wlan.service = wlan_service;
				if (wlan_timeout_set) {
					methods[i].wlan.timeout = wlan_timeout;
				}
			}
		}

		if (interval_set) {
			config.interval = interval;
		}

		ret = location_request(real_config);
		if (ret) {
			shell_error(shell, "Requesting location failed, err: %d", ret);
			return -1;
		}
		shell_print(shell, "Started to get current location...");
		break;
	}
	default:
		shell_error(shell, "Unknown command. See usage:");
		goto show_usage;
	}
	return ret;
show_usage:
	/* Reset getopt for another users */
	optreset = 1;

	location_shell_print_usage(shell, &loc_cmd_args);
	return ret;
}
