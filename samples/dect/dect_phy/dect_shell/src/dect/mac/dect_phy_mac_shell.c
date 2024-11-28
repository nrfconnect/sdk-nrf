/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <zephyr/kernel.h>

#include <zephyr/shell/shell.h>
#include <getopt.h>

#include "desh_print.h"

#include "dect_common.h"
#include "dect_common_utils.h"
#include "dect_common_settings.h"

#include "dect_phy_ctrl.h"

#include "dect_phy_mac_common.h"
#include "dect_phy_mac_cluster_beacon.h"
#include "dect_phy_mac_nbr.h"
#include "dect_phy_mac_client.h"
#include "dect_phy_mac_ctrl.h"

/**************************************************************************************************/

static int dect_phy_mac_cmd_print_help(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 1;

	if (argc > 1) {
		shell_error(shell, "%s: subcommand not found", argv[1]);
		ret = -1;
	}

	shell_help(shell);

	return ret;
}

static int dect_phy_mac_cmd(const struct shell *shell, size_t argc, char **argv)
{
	return dect_phy_mac_cmd_print_help(shell, argc, argv);
}

/**************************************************************************************************/

static const char dect_phy_mac_beacon_scan_usage_str[] =
	"Usage: dect mac beacon_scan [-c <channel>] [<other_options>]\n"
	"Options:\n"
	"  -c <integer>,                        Channel nbr to be scanned for a beacon.\n"
	"                                       Ranges: band #1: 1657-1677, band #2 1680-1700,\n"
	"                                       band #4 524-552, band #9 1703-1711,\n"
	"                                       band #22 1691-1711. Zero value: all in a set band.\n"
	"                                       Default: 1665.\n"
	"  -t, --scan_time <integer>,           Scanning duration in seconds (default: 4 "
	"seconds).\n"
	"  -e, --rx_exp_rssi_level <integer>,   Set expected RSSI level on a scan RX (dBm).\n"
	"                                       Default: 0 (AGC).\n"
	"  -D, --clear_nbr_cache,               Clear neighbor storage.\n"
	"  -f, --force,                         Use special force to override current items\n"
	"                                       in a scheduler so that beacon scans have priority.\n"
	"                                       Note: when used, other running items are\n"
	"                                       not scheduled to modem.\n"
	"Following options only impacts on RSSI measurements during RX:\n"
	"  -i <interval_secs>,        RSSI measurement reporting interval in seconds.\n"
	"                             Default: 1 seconds, use zero to disable.\n"
	"  --free_th <dbm_value>,     Channel access - considered as free:\n"
	"                             measured signal level <= <value>.\n"
	"                             Set a threshold for RSSI scan free threshold (dBm).\n"
	"                             Default from rssi_scan_busy_th\n"
	"                             Channel access - considered as possible:\n"
	"                             busy_th >= measured signal level > free_th\n"
	"  --busy_th <dbm_value>,     Channel access - considered as busy:\n"
	"                             measured signal level > <value>.\n"
	"                             Set a threshold for RSSI scan busy threshold (dBm).\n"
	"                             Default from rssi_scan_free_th\n";

/* The following do not have short options: */
enum {
	DECT_PHY_MAC_SHELL_BEACON_SCAN_RSSI_SCAN_FREE_TH = 1001,
	DECT_PHY_MAC_SHELL_BEACON_SCAN_RSSI_SCAN_BUSY_TH,
};

/* Specifying the expected options (both long and short): */
static struct option long_options_beacon_scan[] = {
	{"scan_time", required_argument, 0, 't'},
	{"rx_exp_rssi_level", required_argument, 0, 'e'},
	{"clear_nbr_cache", no_argument, 0, 'D'},
	{"force", no_argument, 0, 'f'},
	{"free_th", required_argument, 0, DECT_PHY_MAC_SHELL_BEACON_SCAN_RSSI_SCAN_FREE_TH},
	{"busy_th", required_argument, 0, DECT_PHY_MAC_SHELL_BEACON_SCAN_RSSI_SCAN_BUSY_TH},
	{0, 0, 0, 0}};

static void dect_phy_mac_beacon_scan_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct dect_phy_mac_beacon_scan_params params = {
		.clear_nbr_cache_before_scan = false,
		.expected_rssi_level = 0,
		.duration_secs = 4,
		.channel = 1665,
		.suspend_scheduler = false,
	};
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	int ret;
	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;

	params.busy_rssi_limit = current_settings->rssi_scan.busy_threshold;
	params.free_rssi_limit = current_settings->rssi_scan.free_threshold;
	params.rssi_interval_secs = 1;

	while ((opt = getopt_long(argc, argv, "c:t:e:i:Dfh", long_options_beacon_scan,
				  &long_index)) != -1) {
		switch (opt) {
		case 'c': {
			params.channel = atoi(optarg);
			break;
		}
		case 't': {
			params.duration_secs = atoi(optarg);
			break;
		}
		case 'e': {
			params.expected_rssi_level = atoi(optarg);
			break;
		}
		case 'D': {
			params.clear_nbr_cache_before_scan = true;
			break;
		}
		case 'f': {
			params.suspend_scheduler = true;
			break;
		}
		case DECT_PHY_MAC_SHELL_BEACON_SCAN_RSSI_SCAN_FREE_TH: {
			params.free_rssi_limit = atoi(optarg);
			break;
		}
		case DECT_PHY_MAC_SHELL_BEACON_SCAN_RSSI_SCAN_BUSY_TH: {
			params.busy_rssi_limit = atoi(optarg);
			break;
		}
		case 'i': {
			params.rssi_interval_secs = atoi(optarg);
			break;
		}
		case 'h':
			goto show_usage;
		case '?':
		default:
			desh_error("Unknown option (%s). See usage:", argv[optind - 1]);
			goto show_usage;
		}
	}
	if (optind < argc) {
		desh_error("Arguments without '-' not supported: %s", argv[argc - 1]);
		goto show_usage;
	}

	ret = dect_phy_mac_ctrl_beacon_scan_start(&params);
	if (ret) {
		desh_error("Cannot start beacon scan, err %d", ret);
	} else {
		desh_print("Beacon scan started.");
	}
	return;

show_usage:
	desh_print_no_format(dect_phy_mac_beacon_scan_usage_str);
}

/**************************************************************************************************/

static const char dect_phy_mac_beacon_start_cmd_usage_str[] =
	"Starts a cluster beacon with the configured settings and given parameters.\n"
	"\n"
	"Usage: dect mac beacon_start [<options>]\n"
	"Options:\n"
	"  -c <integer>,               Used channel for a beacon.\n"
	"                              Ranges: band #1: 1657-1677 (only odd numbers),\n"
	"                              band #2 1680-1700, band #4 524-552, band #9 1703-1711,\n"
	"                              band #22 1691-1711.\n"
	"                              Default: 0, i.e. automatic selection of\n"
	"                              free/possible channel on a set band.\n"
	"  -p, --tx_pwr <dbm>,         Set beacon broadcast power (dBm), default: -16.\n"
	"                              [-40,-30,-20,-16,-12,-8,-4,0,4,7,10,13,16,19,21,23]\n";

/* Specifying the expected options (both long and short): */
static struct option long_options_beacon_start[] = {{"tx_pwr", required_argument, 0, 'p'},
						    {0, 0, 0, 0}};

static void dect_phy_mac_beacon_start_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct dect_phy_mac_beacon_start_params params = {
		.tx_power_dbm = 0,
		.beacon_channel = 0,
	};
	struct dect_phy_settings *current_settings;
	int ret;
	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;

	while ((opt = getopt_long(argc, argv, "p:c:h", long_options_beacon_start,
				  &long_index)) != -1) {
		switch (opt) {
		case 'c': {
			params.beacon_channel = atoi(optarg);
			break;
		}
		case 'p': {
			params.tx_power_dbm = atoi(optarg);
			break;
		}
		case 'h':
			goto show_usage;
		case '?':
		default:
			desh_error("Unknown option (%s). See usage:", argv[optind - 1]);
			goto show_usage;
		}
	}
	if (optind < argc) {
		desh_error("Arguments without '-' not supported: %s", argv[argc - 1]);
		goto show_usage;
	}

	current_settings = dect_common_settings_ref_get();
	if (params.beacon_channel != 0 &&
	    !dect_common_utils_channel_is_supported(current_settings->common.band_nbr,
						    params.beacon_channel, true)) {
		desh_error("Channel %d is not supported at band #%d", params.beacon_channel,
			   current_settings->common.band_nbr);
		goto show_usage;
	}

	ret = dect_phy_mac_ctrl_cluster_beacon_start(&params);
	if (ret) {
		desh_error("Cannot start beacon, err %d", ret);
	} else {
		desh_print("Beacon starting");
	}

	return;

show_usage:
	desh_print_no_format(dect_phy_mac_beacon_start_cmd_usage_str);
}

static void dect_phy_mac_beacon_stop_cmd(const struct shell *shell, size_t argc, char **argv)
{
	desh_print("Stopping beacon.");
	dect_phy_mac_ctrl_cluster_beacon_stop(DECT_PHY_MAC_CTRL_BEACON_STOP_CAUSE_USER_INITIATED);
}

/**************************************************************************************************/

static const char dect_phy_mac_associate_cmd_usage_str[] =
	"Usage: dect mac associate [<options>]\n"
	"Sends Association Request.\n"
	"Options:\n"
	"  -t, --long_rd_id <id>,  Target long rd id. Default: 38.\n"
	"  -p, --tx_pwr <integer>, TX power (dBm). Default: 0 dBm.\n"
	"  -m, --tx_mcs <integer>, TX MCS (integer). Default: 0.\n"
	"Note: LBT (Listen Before Talk) is enabled as a default for a min period,\n"
	"      but the LBT max RSSI threshold can be configured in settings\n"
	"      (dect sett --rssi_scan_busy_th <dbm>).\n";

/* Specifying the expected options (both long and short): */
static struct option long_options_associate[] = {{"tx_pwr", required_argument, 0, 'p'},
						 {"tx_mcs", required_argument, 0, 'm'},
						 {"long_rd_id", required_argument, 0, 't'},
						 {0, 0, 0, 0}};

static int dect_phy_mac_associate_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct dect_phy_mac_associate_params params;
	int ret = 0;
	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;

	params.tx_power_dbm = 0;
	params.mcs = 0;
	params.target_long_rd_id = 38;

	while ((opt = getopt_long(argc, argv, "p:m:t:h", long_options_associate,
				  &long_index)) != -1) {
		switch (opt) {
		case 't': {
			params.target_long_rd_id = shell_strtoul(optarg, 10, &ret);
			if (ret) {
				desh_error("Give decent tx id (> 0)");
				return -EINVAL;
			}
			break;
		}
		case 'p': {
			params.tx_power_dbm = atoi(optarg);
			break;
		}
		case 'm': {
			params.mcs = atoi(optarg);
			break;
		}
		case 'h':
			goto show_usage;
		case '?':
		default:
			desh_error("Unknown option (%s). See usage:", argv[optind - 1]);
			goto show_usage;
		}
	}
	if (optind < argc) {
		desh_error("Arguments without '-' not supported: %s", argv[argc - 1]);
		goto show_usage;
	}

	desh_print("Sending association_req to FT %u's random access resource",
		params.target_long_rd_id);

	ret = dect_phy_mac_ctrl_associate(&params);
	if (ret) {
		desh_error("Cannot send association request to PT %u "
			   "a random access resource, err %d",
				params.target_long_rd_id, ret);
	} else {
		desh_print("Association request TX started.");
	}
	return 0;

show_usage:
	desh_print_no_format(dect_phy_mac_associate_cmd_usage_str);
	return 0;
}

/**************************************************************************************************/

static const char dect_phy_mac_dissociate_cmd_usage_str[] =
	"Usage: dect mac dissociate [<options>]\n"
	"Sends Association Release message.\n"
	"Options:\n"
	"  -t, --long_rd_id <id>,  Target long rd id. Default: 38.\n"
	"  -p, --tx_pwr <integer>, TX power (dBm). Default 0 dBm.\n"
	"  -m, --tx_mcs <integer>, TX MCS (integer). Default: 0.\n"
	"Note: LBT (Listen Before Talk) is enabled as a default for a min period,\n"
	"      but the LBT max RSSI threshold can be configured in settings\n"
	"      (dect sett --rssi_scan_busy_th <dbm>).\n";

/* Specifying the expected options (both long and short): */
static struct option long_options_dissociate[] = {
	{"tx_pwr", required_argument, 0, 'p'},
	{"tx_mcs", required_argument, 0, 'm'},
	{"long_rd_id", required_argument, 0, 't'},
	{0, 0, 0, 0}};

static int dect_phy_mac_dissociate_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct dect_phy_mac_associate_params params;
	int ret = 0;
	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;

	params.tx_power_dbm = 0;
	params.mcs = 0;
	params.target_long_rd_id = 38;

	while ((opt = getopt_long(argc, argv, "p:m:t:h", long_options_dissociate,
				  &long_index)) != -1) {
		switch (opt) {
		case 't': {
			params.target_long_rd_id = shell_strtoul(optarg, 10, &ret);
			if (ret) {
				desh_error("Give decent tx id (> 0)");
				return -EINVAL;
			}
			break;
		}
		case 'p': {
			params.tx_power_dbm = atoi(optarg);
			break;
		}
		case 'm': {
			params.mcs = atoi(optarg);
			break;
		}
		case 'h':
			goto show_usage;
		case '?':
		default:
			desh_error("Unknown option (%s). See usage:", argv[optind - 1]);
			goto show_usage;
		}
	}
	if (optind < argc) {
		desh_error("Arguments without '-' not supported: %s", argv[argc - 1]);
		goto show_usage;
	}

	desh_print("Sending association release to FT %u's random access resource",
		   params.target_long_rd_id);

	ret = dect_phy_mac_ctrl_dissociate(&params);
	if (ret) {
		desh_error("Cannot send association release to FT %u's random access resource, "
			   "err %d",
				params.target_long_rd_id, ret);
	} else {
		desh_print("Association Release TX started.");
	}
	return 0;

show_usage:
	desh_print_no_format(dect_phy_mac_dissociate_cmd_usage_str);
	return 0;
}

/**************************************************************************************************/

static const char dect_phy_mac_rach_tx_cmd_usage_str[] =
	"Usage: dect mac rach_tx [stop] | -d <data> [<options>]\n"
	"Options:\n"
	"  -d, --data <data_str>,  Data to be sent.\n"
	"  -p, --tx_pwr <integer>, TX power (dBm). Default: 0 dBm.\n"
	"  -m, --tx_mcs <integer>, TX MCS (integer). Default: 0.\n"
	"  -t, --long_rd_id <id>,  Target long rd id. Default: 38.\n"
	"  -i, --interval <interval_secs>, Data sending interval in seconds.\n"
	"                                  Default: 0, data sent only once.\n"
	"  -j, --get_mdm_temp,             Include modem temperature in the payload. The payload\n"
	"                                  is encoded in JSON.\n"
	"Note: LBT (Listen Before Talk) is enabled as a default for a min period,\n"
	"      but the LBT max RSSI threshold can be configured in settings\n"
	"      (dect sett --rssi_scan_busy_th <dbm>).\n";

#define DECT_PHY_MAC_RACH_TX_DATA_JSON_OVERHEAD 30

/* Specifying the expected options (both long and short): */
static struct option long_options_rach_tx[] = { {"data", required_argument, 0, 'd'},
						{"tx_pwr", required_argument, 0, 'p'},
						{"tx_mcs", required_argument, 0, 'm'},
						{"long_rd_id", required_argument, 0, 't'},
						{"interval", required_argument, 0, 'i'},
						{"get_mdm_temp", no_argument, 0, 'j'},
						{0, 0, 0, 0}};

static int dect_phy_mac_rach_tx_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct dect_phy_mac_rach_tx_params params;
	int ret = 0;
	int long_index = 0;
	int opt;

	if (argc < 2) {
		goto show_usage;
	}
	if (argv[1] != NULL && !strcmp(argv[1], "stop")) {
		dect_phy_mac_ctrl_rach_tx_stop();
		desh_print("rach_tx stopped.");
		return 0;
	}

	optreset = 1;
	optind = 1;

	params.tx_power_dbm = 0;
	params.mcs = 0;
	params.target_long_rd_id = 38;
	params.interval_secs = 0;
	params.get_mdm_temp = false;

	while ((opt = getopt_long(argc, argv, "d:p:m:t:i:jh", long_options_rach_tx,
				  &long_index)) != -1) {
		switch (opt) {
		case 't': {
			params.target_long_rd_id = shell_strtoul(optarg, 10, &ret);
			if (ret) {
				desh_error("Give decent tx id (> 0)");
				return -EINVAL;
			}
			break;
		}
		case 'd': {
			if (strlen(optarg) >= (DECT_DATA_MAX_LEN - 1)) {
				desh_error("RACH TX data (%s) too long.", optarg);
				return -EINVAL;
			}
			strcpy(params.tx_data_str, optarg);
			break;
		}
		case 'p': {
			params.tx_power_dbm = atoi(optarg);
			break;
		}
		case 'm': {
			params.mcs = atoi(optarg);
			break;
		}
		case 'i': {
			params.interval_secs = atoi(optarg);
			if (params.interval_secs < 0) {
				desh_error("The interval must be positive.");
				return -EINVAL;
			}
			break;
		}
		case 'j': {
			params.get_mdm_temp = true;
			break;
		}

		case 'h':
			goto show_usage;
		case '?':
		default:
			desh_error("Unknown option (%s). See usage:", argv[optind - 1]);
			goto show_usage;
		}
	}
	if (optind < argc) {
		desh_error("Arguments without '-' not supported: %s", argv[argc - 1]);
		goto show_usage;
	}
	if (params.get_mdm_temp &&
	    ((strlen(params.tx_data_str) + DECT_PHY_MAC_RACH_TX_DATA_JSON_OVERHEAD) >=
	     (DECT_DATA_MAX_LEN - 1))) {
		desh_error(
			"The given data is too long to be encoded into JSON with modem temperature.");
		goto show_usage;
	}

	desh_print("Sending data %s to FT %u's random access resource", params.tx_data_str,
		   params.target_long_rd_id);

	ret = dect_phy_mac_ctrl_rach_tx_start(&params);
	if (ret) {
		desh_error("Cannot send data %s to FT %u's random access resource, err %d",
			   params.tx_data_str, params.target_long_rd_id, ret);
	} else {
		desh_print("Client TX to RACH started.");
	}
	return 0;

show_usage:
	desh_print_no_format(dect_phy_mac_rach_tx_cmd_usage_str);
	return 0;
}

/**************************************************************************************************/

static void dect_phy_mac_status_cmd(const struct shell *shell, size_t argc, char **argv)
{
	desh_print("dect-phy-mac status:");

	dect_phy_mac_cluster_beacon_status_print();
	dect_phy_mac_client_status_print();
	dect_phy_mac_nbr_status_print();
}

/**************************************************************************************************/

SHELL_STATIC_SUBCMD_SET_CREATE(
	mac_shell_cmd_sub,
	SHELL_CMD_ARG(status, NULL, "Usage options: dect mac status", dect_phy_mac_status_cmd, 1,
		      0),
	SHELL_CMD_ARG(beacon_start, NULL, "Usage options: dect mac beacon_start -h",
		      dect_phy_mac_beacon_start_cmd, 1, 6),
	SHELL_CMD_ARG(beacon_stop, NULL, "Usage: dect mac beacon_stop",
		      dect_phy_mac_beacon_stop_cmd, 1, 0),
	SHELL_CMD_ARG(beacon_scan, NULL, "Usage options: dect mac beacon_scan -h",
		      dect_phy_mac_beacon_scan_cmd, 1, 12),
	SHELL_CMD_ARG(associate, NULL, "Usage: dect mac associate -h",
		      dect_phy_mac_associate_cmd, 1, 11),
	SHELL_CMD_ARG(dissociate, NULL, "Usage: dect mac dissociate -h",
		      dect_phy_mac_dissociate_cmd, 1, 6),
	SHELL_CMD_ARG(rach_tx, NULL, "Usage options: dect mac rach_tx -h",
		      dect_phy_mac_rach_tx_cmd, 1, 11),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((dect), mac, &mac_shell_cmd_sub,
		 "Commands for using sample MAC commands on top of libmodem PHY API.",
		 dect_phy_mac_cmd, 1, 0);
