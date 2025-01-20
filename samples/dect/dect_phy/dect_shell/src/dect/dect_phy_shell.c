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

#include "nrfx.h"
#include "desh_print.h"

#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>

#include <nrf_modem_dect_phy.h>

#include "dect_common_utils.h"
#include "dect_phy_api_scheduler.h"
#include "dect_common_settings.h"

#include "dect_phy_ctrl.h"
#include "dect_phy_rf_tool.h"
#include "dect_phy_scan.h"

static int dect_phy_dect_cmd_print_help(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 1;

	if (argc > 1) {
		shell_error(shell, "%s: subcommand not found", argv[1]);
		ret = -1;
	}

	shell_help(shell);

	return ret;
}

static int dect_phy_dect_cmd(const struct shell *shell, size_t argc, char **argv)
{
	return dect_phy_dect_cmd_print_help(shell, argc, argv);
}

static int dect_phy_scheduler_list_status_print_cmd(const struct shell *shell, size_t argc,
						    char **argv)
{
	dect_phy_api_scheduler_list_status_print();
	return 0;
}

static int dect_phy_scheduler_list_purge_cmd(const struct shell *shell, size_t argc, char **argv)
{
	dect_phy_api_scheduler_list_delete_all_items();
	return 0;
}

/**************************************************************************************************/

/* The following do not have short options:: */
enum {
	DECT_SHELL_PERF_CHANNEL = 2001,
	DECT_SHELL_PERF_SLOT_COUNT,
	DECT_SHELL_PERF_SUBSLOT_COUNT_HARQ_FEEDBACK_RX,
	DECT_SHELL_PERF_SUBSLOT_COUNT_HARQ_FEEDBACK_RX_DELAY_SLOT_COUNT,
	DECT_SHELL_PERF_SLOT_COUNT_HARQ_FEEDBACK_TX_DELAY_SUBSLOT_COUNT,
	DECT_SHELL_PERF_SLOT_COUNT_HARQ_FEEDBACK_TX_RX_DELAY_SUBSLOT_COUNT,
	DECT_SHELL_PERF_SLOT_GAP_COUNT,
	DECT_SHELL_PERF_SUBSLOT_GAP_COUNT,
	DECT_SHELL_PERF_TX_PWR,
	DECT_SHELL_PERF_TX_MCS,
	DECT_SHELL_PERF_DEST_SERVER_TX_ID,
	DECT_SHELL_PERF_HARQ_MDM_PROCESS_COUNT,
	DECT_SHELL_PERF_HARQ_MDM_EXPIRY_TIME,
	DECT_SHELL_PERF_DECT_HARQ_CLIENT_PROCESS_MAX_NBR,
};

static const char dect_phy_perf_cmd_usage_str[] =
	"Usage: dect perf  [stop] | [<server>|<client> [server_options]|[client_options]\n"
	"Note: make sure that accessed channel is free by using \"dect rssi_scan\" -command.\n"
	"\n"
	"Options:\n"
	"  -c, --client,                  Client role.\n"
	"  -s, --server,                  Server role.\n"
	"  -t, --duration <int>,          Duration time.\n"
	"                                 Default 5 seconds. For continuous server: -1.\n"
	"      --channel <int>,           Channel. Default 1665.\n"
	"  -e  --rx_exp_rssi_level <int>, Set expected RSSI level on RX (dBm).\n"
	"                                 Client side impact on RX for HARQ feedback\n"
	"                                 and results.\n"
	"                                 Default: from common rx settings.\n"
	"      --s_tx_id <id>,            Target tx id (default: 38).\n"
	"      --c_slots <int>,           Count of consecutive slots. Default: 2.\n"
	"      --c_gap_slots <int>,       Count of gap slots between operations. Default: 2.\n"
	"      --c_gap_subslots <int>,    Instead of slots, count of gap subslots between\n"
	"                                 operations.\n"
	"                                 Default: as slots (c_gap_slots).\n"
	"      --c_tx_pwr <int>,          TX power (dBm),\n"
	"                                 [-40,-30,-20,-16,-12,-8,-4,0,4,7,10,13,16,19,21,23].\n"
	"                                 See supported max for the used band by using\n"
	"                                 \"dect status\" -command output.\n"
	"                                 Default: from common tx settings.\n"
	"      --c_tx_mcs <int>,          Set client TX MCS. Default: from common tx settings.\n"
	"  -d, --debug,                   Print CRC errors. Note: might impact on actual\n"
	"                                 perf & timings.\n"
	"For HARQ only:\n"
	"  -a, --use_harq,                              Use HARQ.\n"
	"      --mdm_init_harq_process_count <int>,     Count of HARQ RX processes for\n"
	"                                               modem init [1,2,4,8].\n"
	"                                               Default from common settings.\n"
	"      --mdm_init_harq_expiry_time_us <int>,    HARQ RX buffer expiry time,\n"
	"                                               in microseconds.\n"
	"                                               Maximum supported value: 5000000.\n"
	"                                               Default from common settings.\n"
	"       Client side TX with HARQ iteration:\n"
	"	c_slots - c_harq_feedback_rx_delay_subslots - c_harq_feedback_rx_subslots -\n"
	"       c_gap_slots\n\n"
	"      --c_harq_feedback_rx_delay_subslots <int>, Client side.\n"
	"                                                 A gap between TX and starting RX\n"
	"                                                 for HARQ feedback.\n"
	"                                                 Default: from common settings.\n"
	"      --c_harq_feedback_rx_subslots <int>,       Client side.\n"
	"                                                 RX duration (in subslots) for\n"
	"                                                 receiving HARQ\n"
	"                                                 feedback for the client TX.\n"
	"                                                 Default: from common settings.\n"
	"      --c_harq_process_nbr_max <int>, HARQ process nbr range to TX headers [0-7].\n"
	"                                      Default: 3, i.e range of  0-3.\n"
	"                                      Note: can be bigger that\n"
	"                                      mdm_init_harq_process_count\n"
	"                                      but then all might not fit to server buffers.\n"
	"       Server side RX with HARQ iteration:\n"
	"	RX data - s_harq_feedback_tx_delay_subslots - HARQ feedback -\n"
	"       s_harq_feedback_tx_rx_delay_subslots - RX\n\n"
	"      --s_harq_feedback_tx_delay_subslots <int>,    Server side.\n"
	"                                                    A gap between RX and starting\n"
	"                                                    a TX for providing HARQ\n"
	"                                                    feedback if requested by client.\n"
	"                                                    Default: from common settings.\n"
	"      --s_harq_feedback_tx_rx_delay_subslots <int>, Server side.\n"
	"                                                    A gap between HARQ feedback TX\n"
	"                                                    end and re-starting a server RX.\n"
	"                                                    Default: 4.\n";

/* Specifying the expected options (both long and short): */
static struct option long_options_perf[] = {
	{"client", no_argument, 0, 'c'},
	{"server", no_argument, 0, 's'},
	{"use_harq", no_argument, 0, 'a'},
	{"debug", no_argument, 0, 'd'},
	{"duration", required_argument, 0, 't'},
	{"rx_exp_rssi_level", required_argument, 0, 'e'},
	{"s_tx_id", required_argument, 0, DECT_SHELL_PERF_DEST_SERVER_TX_ID},
	{"channel", required_argument, 0, DECT_SHELL_PERF_CHANNEL},
	{"c_slots", required_argument, 0, DECT_SHELL_PERF_SLOT_COUNT},
	{"c_gap_slots", required_argument, 0, DECT_SHELL_PERF_SLOT_GAP_COUNT},
	{"c_gap_subslots", required_argument, 0, DECT_SHELL_PERF_SUBSLOT_GAP_COUNT},
	{"c_harq_feedback_rx_subslots", required_argument, 0,
	 DECT_SHELL_PERF_SUBSLOT_COUNT_HARQ_FEEDBACK_RX},
	{"c_harq_feedback_rx_delay_subslots", required_argument, 0,
	 DECT_SHELL_PERF_SUBSLOT_COUNT_HARQ_FEEDBACK_RX_DELAY_SLOT_COUNT},
	{"c_harq_process_nbr_max", required_argument, 0,
	 DECT_SHELL_PERF_DECT_HARQ_CLIENT_PROCESS_MAX_NBR},
	{"s_harq_feedback_tx_delay_subslots", required_argument, 0,
	 DECT_SHELL_PERF_SLOT_COUNT_HARQ_FEEDBACK_TX_DELAY_SUBSLOT_COUNT},
	{"s_harq_feedback_tx_rx_delay_subslots", required_argument, 0,
	 DECT_SHELL_PERF_SLOT_COUNT_HARQ_FEEDBACK_TX_RX_DELAY_SUBSLOT_COUNT},
	{"mdm_init_harq_process_count", required_argument, 0,
	 DECT_SHELL_PERF_HARQ_MDM_PROCESS_COUNT},
	{"mdm_init_harq_expiry_time_us", required_argument, 0,
	 DECT_SHELL_PERF_HARQ_MDM_EXPIRY_TIME},
	{"c_tx_pwr", required_argument, 0, DECT_SHELL_PERF_TX_PWR},
	{"c_tx_mcs", required_argument, 0, DECT_SHELL_PERF_TX_MCS},
	{0, 0, 0, 0}};

static int dect_phy_perf_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct dect_phy_perf_params params;
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();
	int ret;
	int long_index = 0;
	int opt;
	int temp;

	if (argc < 2) {
		goto show_usage;
	}

	optreset = 1;
	optind = 1;

	if (argv[1] != NULL && !strcmp(argv[1], "stop")) {
		desh_print("Perf command stopping.");
		dect_phy_ctrl_perf_cmd_stop();
		return 0;
	}

	/* Set defaults */
	params.duration_secs = 5;
	params.tx_power_dbm = current_settings->tx.power_dbm;
	params.tx_mcs = current_settings->tx.mcs;
	params.role = DECT_PHY_COMMON_ROLE_NONE;
	params.channel = 1665;
	params.slot_count = 2;
	params.slot_gap_count = 2;
	params.subslot_gap_count = 0; /* As a default, in slots. */
	params.destination_transmitter_id = DECT_PHY_DEFAULT_TRANSMITTER_LONG_RD_ID;
	params.expected_rx_rssi_level = current_settings->rx.expected_rssi_level;
	params.debugs = false;
	params.use_harq = false;
	params.mdm_init_harq_process_count = current_settings->harq.mdm_init_harq_process_count;
	params.mdm_init_harq_expiry_time_us = current_settings->harq.mdm_init_harq_expiry_time_us;
	params.client_harq_feedback_rx_subslot_count =
		current_settings->harq.harq_feedback_rx_subslot_count;
	params.client_harq_feedback_rx_delay_subslot_count =
		current_settings->harq.harq_feedback_rx_delay_subslot_count;
	params.client_harq_process_nbr_max = 3;
	params.server_harq_feedback_tx_delay_subslot_count =
		current_settings->harq.harq_feedback_tx_delay_subslot_count;
	params.server_harq_feedback_tx_rx_delay_subslot_count = 4;

	while ((opt = getopt_long(argc, argv, "e:t:csadh", long_options_perf, &long_index)) != -1) {
		switch (opt) {
		case 'd': {
			params.debugs = true;
			break;
		}
		case 'c': {
			params.role = DECT_PHY_COMMON_ROLE_CLIENT;
			break;
		}
		case 's': {
			params.role = DECT_PHY_COMMON_ROLE_SERVER;
			break;
		}
		case 'a': {
			params.use_harq = true;
			break;
		}
		case 't': {
			params.duration_secs = atoi(optarg);
			break;
		}
		case 'e': {
			params.expected_rx_rssi_level = atoi(optarg);
			break;
		}
		case DECT_SHELL_PERF_DEST_SERVER_TX_ID: {
			params.destination_transmitter_id = atoi(optarg);
			break;
		}
		case DECT_SHELL_PERF_TX_PWR: {
			params.tx_power_dbm = atoi(optarg);
			break;
		}
		case DECT_SHELL_PERF_TX_MCS: {
			params.tx_mcs = atoi(optarg);
			break;
		}
		case DECT_SHELL_PERF_CHANNEL: {
			params.channel = atoi(optarg);
			break;
		}
		case DECT_SHELL_PERF_SLOT_COUNT: {
			ret = atoi(optarg);
			if (ret <= 0) {
				desh_error("Give decent value for the slot count (> 0)");
				goto show_usage;
			}
			params.slot_count = ret;
			break;
		}
		case DECT_SHELL_PERF_SLOT_GAP_COUNT: {
			params.slot_gap_count = atoi(optarg);
			break;
		}
		case DECT_SHELL_PERF_SUBSLOT_GAP_COUNT: {
			params.subslot_gap_count = atoi(optarg);
			break;
		}
		case DECT_SHELL_PERF_SUBSLOT_COUNT_HARQ_FEEDBACK_RX: {
			params.client_harq_feedback_rx_subslot_count = atoi(optarg);
			break;
		}
		case DECT_SHELL_PERF_SUBSLOT_COUNT_HARQ_FEEDBACK_RX_DELAY_SLOT_COUNT: {
			params.client_harq_feedback_rx_delay_subslot_count = atoi(optarg);
			break;
		}
		case DECT_SHELL_PERF_SLOT_COUNT_HARQ_FEEDBACK_TX_DELAY_SUBSLOT_COUNT: {
			params.server_harq_feedback_tx_delay_subslot_count = atoi(optarg);
			break;
		}
		case DECT_SHELL_PERF_SLOT_COUNT_HARQ_FEEDBACK_TX_RX_DELAY_SUBSLOT_COUNT: {
			params.server_harq_feedback_tx_rx_delay_subslot_count = atoi(optarg);
			break;
		}
		case DECT_SHELL_PERF_HARQ_MDM_PROCESS_COUNT: {
			temp = atoi(optarg);
			if (temp != 1 && temp != 2 && temp != 4 && temp != 8) {
				desh_error("Not valid HARQ process count.");
				goto show_usage;
			}
			params.mdm_init_harq_process_count = temp;
			break;
		}
		case DECT_SHELL_PERF_DECT_HARQ_CLIENT_PROCESS_MAX_NBR: {
			temp = atoi(optarg);
			if (temp < 0 || temp > 7) {
				desh_error("Not valid HARQ process nbr max.");
				goto show_usage;
			}
			params.client_harq_process_nbr_max = temp;
			break;
		}
		case DECT_SHELL_PERF_HARQ_MDM_EXPIRY_TIME: {
			temp = atoi(optarg);
			if (temp <= 0 || temp > 5000000) {
				desh_error("Not valid HARQ rx buffer expiration time given.");
				goto show_usage;
			}
			params.mdm_init_harq_expiry_time_us = temp;
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

	if (params.role == DECT_PHY_COMMON_ROLE_NONE) {
		desh_error("At least role need to be given. See usage:");
		goto show_usage;
	}

	if (params.subslot_gap_count) {
		params.slot_gap_count_in_mdm_ticks =
			DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS * params.subslot_gap_count;
	} else {
		params.slot_gap_count_in_mdm_ticks =
			DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS * params.slot_gap_count;
	}

	ret = dect_phy_ctrl_perf_cmd(&params);
	if (ret) {
		desh_error("Cannot start perf command, ret: %d", ret);
	} else {
		desh_print("perf command started.");
	}
	return 0;

show_usage:
	desh_print_no_format(dect_phy_perf_cmd_usage_str);
	return 0;
}

/**************************************************************************************************/

/* The following do not have short options:: */
enum {
	DECT_SHELL_CERT_FRAME_REPEAT_COUNT = 3001,
	DECT_SHELL_CERT_FRAME_REPEAT_COUNT_INTERVALS,
	DECT_SHELL_CERT_TEST_MODE_CONTINUOUS,
	DECT_SHELL_CERT_RF_MODE_PEER,
	DECT_SHELL_CERT_TX_MCS,
	DECT_SHELL_CERT_RX_FRAME_START_OFFSET_SUBSLOTS,
	DECT_SHELL_CERT_RX_SUBSLOT_COUNT,
	DECT_SHELL_CERT_RX_POST_IDLE_SUBSLOT_COUNT,
	DECT_SHELL_CERT_TX_FRAME_START_OFFSET_SUBSLOTS,
	DECT_SHELL_CERT_TX_SUBSLOT_COUNT,
	DECT_SHELL_CERT_TX_POST_IDLE_SUBSLOT_COUNT,
};

static const char dect_phy_rf_tool_cmd_usage_str[] =
	"Usage: dect rf_tool [stop] | [options]\n"
	"Note: make sure that accessed channel is free by using \"dect rssi_scan\" -command.\n"
	"\n"
	"Options:\n"
	"  -m, --rf_mode,                     RF operation mode, one of the following: \"rx\",\n"
	"                                    \"rx_cont\", \"tx\", \"rx_tx\".\n"
	"                                     Default: \"rx_cont\".\n"
	"                                     Note: As a default, RF mode \"rx_cont\" is\n"
	"                                     reporting the results only when\n"
	"                                     \"dect rf_tool stop\" is given.\n"
	"      --rf_mode_peer,                RF operation mode of the TX side peer,\n"
	"                                     can be one of the following: \"tx\", \"rx_tx\".\n"
	"                                     Only meaningful if rf_mode is set to \"rx_cont\".\n"
	"                                     If used, set frame config are having impact for\n"
	"                                     calculating peer frame length.\n"
	"                                     Defines also reporting interval based on set\n"
	"                                     framing parameters.\n"
	"                                     Note: will enable rx_find_sync automatically.\n"
	"                                     Default: None.\n"
	"  -t, --tx_id <id>,                  Target tx id (default: 38).\n"
	"  -c, --channel <int>,               Channel. Default 1665.\n"
	"                                     Ranges: band #1: 1657-1677 (only odd numbers as per\n"
	"                                     ETSI EN 301 406-2, ch 4.3.2.3),\n"
	"                                     band #2 1680-1700, band #4 524-552,\n"
	"                                     band #9 1703-1711, band #22 1691-1711.\n"
	"  -e  --rx_exp_rssi_level <int>,     Set expected RSSI level on RX (dBm).\n"
	"                                     Default: from common rx settings.\n"
	"  -p  --tx_pwr <int>,                TX power (dBm),\n"
	"                                     [-40,-30,-20,-16,-12,-8,-4,0,4,7,10,13,16,19,21,23]\n"
	"                                     See supported max for the used band by using\n"
	"                                     \"dect status\" -command output.\n"
	"                                     Default: from common tx settings.\n"
	"      --tx_mcs <int>,                Set TX MCS. Default: from common tx settings.\n"
	"Frame structure:\n"
	"rx_frame_start_offset + rx_subslot_count + rx_idle_subslot_count +\n"
	"tx_frame_start_offset + tx_subslot_count + tx_idle_subslot_count\n"
	"\n\n"
	"  -s, --rx_find_sync,                Continuous RX until sync found and then RX part\n"
	"                                     of the frame is started from received RX STF time.\n"
	"                                     Default: false.\n"
	"      --frame_repeat_count <int>,    The number of repeats for the configured TX/RX\n"
	"                                     frame.\n"
	"                                     This is also the count how many iterations of\n"
	"                                     configured frame are scheduled at once and\n"
	"                                     is also a reporting interval.\n"
	"                                     There is no delay between frames.\n"
	"                                     Default: 15.\n"
	"      --frame_repeat_count_intervals <int>, The number of intervals for\n"
	"                                            frame_repeat_count.\n"
	"                                     Intervals are scheduled ASAP after\n"
	"                                     the previous one.\n"
	"                                     Default: 5.\n"
	"      --continuous,                  Enable continuous mode, can be stopped by using:\n"
	"                                     \"dect rf_tool stop\".\n"
	"                                     Default: disabled.\n"
	"                                     i.e. only configured frame_repeat_count/interval.\n"
	"      --rx_frame_start_offset <int>, Subslot count before RX operation\n"
	"                                     calculated from a start of a frame.\n"
	"                                     Default: 0 (=starting from the start of a frame).\n"
	"      --rx_subslot_count <int>,      The duration of the RX operation in subslots.\n"
	"                                     Note: no impact if just \"tx\" mode in use.\n"
	"                                     Default: 5.\n"
	"      --rx_idle_subslot_count <int>, Idle duration after RX operation in subslots.\n"
	"                                     Default: 5.\n"
	"      --tx_frame_start_offset <int>, Subslot count before TX operation\n"
	"                                     calculated from the beginning of the TX/RX frame.\n"
	"                                     Defaults: with \"tx\" only mode: 0 (0 = 1st "
	"subslot),\n"
	"                                     with \"rx_tx\" mode:\n"
	"                                     rx_frame_start_offset + rx_subslot_count +\n"
	"                                     rx_idle_subslot_count.\n"
	"      --tx_subslot_count <int>,      The length of the TX operation in subslots.\n"
	"                                     Default: 4.\n"
	"                                     Note: Only MCS0 & MCS1 supports 8/16 consecutive TX\n"
	"                                     slots/subslots. MCS3 supports 5/10\n"
	"                                     TX slots/subslots and MCS4 supports max of 4/8\n"
	"                                     consecutive TX slots/subslots.\n"
	"      --tx_idle_subslot_count <int>, Idle duration after TX operation in subslots.\n"
	"                                     Default: 6.\n";

/* Specifying the expected options (both long and short): */
static struct option long_options_cert[] = {
	{"rf_mode", required_argument, 0, 'm'},
	{"rf_mode_peer", required_argument, 0, DECT_SHELL_CERT_RF_MODE_PEER},
	{"tx_id", required_argument, 0, 't'},
	{"channel", required_argument, 0, 'c'},
	{"rx_exp_rssi_level", required_argument, 0, 'e'},
	{"rx_find_sync", no_argument, 0, 's'},
	{"tx_pwr", required_argument, 0, 'p'},
	{"tx_mcs", required_argument, 0, DECT_SHELL_CERT_TX_MCS},
	{"frame_repeat_count", required_argument, 0, DECT_SHELL_CERT_FRAME_REPEAT_COUNT},
	{"frame_repeat_count_intervals", required_argument, 0,
	 DECT_SHELL_CERT_FRAME_REPEAT_COUNT_INTERVALS},
	{"continuous", no_argument, 0, DECT_SHELL_CERT_TEST_MODE_CONTINUOUS},
	{"rx_frame_start_offset", required_argument, 0,
	 DECT_SHELL_CERT_RX_FRAME_START_OFFSET_SUBSLOTS},
	{"rx_subslot_count", required_argument, 0, DECT_SHELL_CERT_RX_SUBSLOT_COUNT},
	{"rx_idle_subslot_count", required_argument, 0, DECT_SHELL_CERT_RX_POST_IDLE_SUBSLOT_COUNT},
	{"tx_frame_start_offset", required_argument, 0,
	 DECT_SHELL_CERT_TX_FRAME_START_OFFSET_SUBSLOTS},
	{"tx_subslot_count", required_argument, 0, DECT_SHELL_CERT_TX_SUBSLOT_COUNT},
	{"tx_idle_subslot_count", required_argument, 0, DECT_SHELL_CERT_TX_POST_IDLE_SUBSLOT_COUNT},
	{0, 0, 0, 0}};

static int dect_phy_rf_tool_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct dect_phy_rf_tool_params params;
	int ret;
	int long_index = 0;
	int opt;
	bool tx_starttime_set = false;
	bool peer_mode_set = false;

	if (argc == 0) {
		goto show_usage;
	}

	optreset = 1;
	optind = 1;

	if (argv[1] != NULL && !strcmp(argv[1], "stop")) {
		dect_phy_ctrl_rf_tool_cmd_stop();
		desh_print("rf_tool command stopping.");
		return 0;
	}
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

	/* Set defaults */
	params.mode = DECT_PHY_RF_TOOL_MODE_RX_CONTINUOUS;
	params.peer_mode = DECT_PHY_RF_TOOL_MODE_NONE;
	params.destination_transmitter_id = 38;
	params.channel = 1665;
	params.tx_power_dbm = current_settings->tx.power_dbm;
	params.tx_mcs = current_settings->tx.mcs;
	params.expected_rx_rssi_level = current_settings->rx.expected_rssi_level;
	params.find_rx_sync = false;
	params.continuous = false;
	params.frame_repeat_count = 15;
	params.frame_repeat_count_intervals = 5;

	params.rx_frame_start_offset = 0;
	params.rx_subslot_count = 5;
	params.rx_post_idle_subslot_count = 5;

	params.tx_frame_start_offset = params.rx_frame_start_offset + params.rx_subslot_count +
				       params.rx_post_idle_subslot_count;
	params.tx_subslot_count = 4;
	params.tx_post_idle_subslot_count = 6;

	while ((opt = getopt_long(argc, argv, "m:t:c:e:p:sh", long_options_cert, &long_index)) !=
	       -1) {
		switch (opt) {
		case 's': {
			params.find_rx_sync = true;
			break;
		}

		case 'm': {
			if (strcmp(optarg, "rx") == 0) {
				params.mode = DECT_PHY_RF_TOOL_MODE_RX;
			} else if (strcmp(optarg, "rx_cont") == 0) {
				params.mode = DECT_PHY_RF_TOOL_MODE_RX_CONTINUOUS;
			} else if (strcmp(optarg, "tx") == 0) {
				params.mode = DECT_PHY_RF_TOOL_MODE_TX;
			} else if (strcmp(optarg, "rx_tx") == 0) {
				params.mode = DECT_PHY_RF_TOOL_MODE_RX_TX;
			} else {
				desh_error("Unknown mode: %s", optarg);
				goto show_usage;
			}
			break;
		}
		case DECT_SHELL_CERT_RF_MODE_PEER: {
			if (strcmp(optarg, "tx") == 0) {
				params.peer_mode = DECT_PHY_RF_TOOL_MODE_TX;
			} else if (strcmp(optarg, "rx_tx") == 0) {
				params.peer_mode = DECT_PHY_RF_TOOL_MODE_RX_TX;
			} else {
				desh_error("Unknown peerf rf mode: %s", optarg);
				goto show_usage;
			}
			peer_mode_set = true;
			break;
		}
		case 'c': {
			params.channel = atoi(optarg);
			break;
		}
		case 'e': {
			params.expected_rx_rssi_level = atoi(optarg);
			break;
		}
		case 't': {
			params.destination_transmitter_id = atoi(optarg);
			break;
		}
		case 'p': {
			params.tx_power_dbm = atoi(optarg);
			break;
		}
		case DECT_SHELL_CERT_TEST_MODE_CONTINUOUS: {
			printk("Continuous mode enabled.\n");
			params.continuous = true;
			break;
		}
		case DECT_SHELL_CERT_TX_MCS: {
			params.tx_mcs = atoi(optarg);
			break;
		}
		case DECT_SHELL_CERT_FRAME_REPEAT_COUNT: {
			ret = atoi(optarg);
			if (ret <= 0) {
				desh_error("Give decent value (> 0)");
				goto show_usage;
			} else if (ret > 50) {
				desh_warn("Too high frame_repeat_count might cause issues.");
			}
			params.frame_repeat_count = ret;
			break;
		}
		case DECT_SHELL_CERT_FRAME_REPEAT_COUNT_INTERVALS: {
			ret = atoi(optarg);
			if (ret <= 0) {
				desh_error("Give decent value (> 0)");
				goto show_usage;
			}
			params.frame_repeat_count_intervals = ret;
			break;
		}
		case DECT_SHELL_CERT_RX_FRAME_START_OFFSET_SUBSLOTS: {
			ret = atoi(optarg);
			if (ret < 0) {
				desh_error("Give decent value (>= 0)");
				goto show_usage;
			}
			params.rx_frame_start_offset = ret;
			break;
		}
		case DECT_SHELL_CERT_RX_SUBSLOT_COUNT: {
			ret = atoi(optarg);
			if (ret <= 0) {
				desh_error("Give decent value (> 0)");
				goto show_usage;
			}
			params.rx_subslot_count = ret;
			break;
		}
		case DECT_SHELL_CERT_RX_POST_IDLE_SUBSLOT_COUNT: {
			ret = atoi(optarg);
			if (ret < 0) {
				desh_error("Give decent value (>= 0)");
				goto show_usage;
			}
			params.rx_post_idle_subslot_count = ret;
			break;
		}
		case DECT_SHELL_CERT_TX_FRAME_START_OFFSET_SUBSLOTS: {
			ret = atoi(optarg);
			if (ret < 0) {
				desh_error("Give decent value (>= 0)");
				goto show_usage;
			}
			params.tx_frame_start_offset = ret;
			tx_starttime_set = true;
			break;
		}
		case DECT_SHELL_CERT_TX_SUBSLOT_COUNT: {
			ret = atoi(optarg);
			if (ret < 0) {
				desh_error("Give decent value (>= 0)");
				goto show_usage;
			}
			params.tx_subslot_count = ret;
			break;
		}
		case DECT_SHELL_CERT_TX_POST_IDLE_SUBSLOT_COUNT: {
			ret = atoi(optarg);
			if (ret < 0) {
				desh_error("Give decent value (>= 0)");
				goto show_usage;
			}
			params.tx_post_idle_subslot_count = ret;
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
	if (!dect_common_utils_channel_is_supported(current_settings->common.band_nbr,
						    params.channel, true)) {
		desh_error("Channel %d is not supported at band #%d", params.channel,
			   current_settings->common.band_nbr);
		goto show_usage;
	}

	/* Set default for tx_starttime based on mode */
	if (tx_starttime_set == false) {
		if (params.mode == DECT_PHY_RF_TOOL_MODE_TX) {
			params.tx_frame_start_offset = 0;
		} else if (params.mode == DECT_PHY_RF_TOOL_MODE_RX_TX) {
			params.tx_frame_start_offset = params.rx_frame_start_offset +
						       params.rx_subslot_count +
						       params.rx_post_idle_subslot_count;
		} else if (params.mode == DECT_PHY_RF_TOOL_MODE_RX_CONTINUOUS) {
			if (params.peer_mode != DECT_PHY_RF_TOOL_MODE_NONE) {
				if (params.peer_mode == DECT_PHY_RF_TOOL_MODE_TX) {
					params.tx_frame_start_offset = 0;
				} else if (params.peer_mode == DECT_PHY_RF_TOOL_MODE_RX_TX) {
					params.tx_frame_start_offset =
						params.rx_frame_start_offset +
						params.rx_subslot_count +
						params.rx_post_idle_subslot_count;
				}
			}
		}
	}
	if (peer_mode_set) {
		params.find_rx_sync = true;
	}

	ret = dect_phy_ctrl_rf_tool_cmd(&params);
	if (ret) {
		desh_error("Cannot start rf_tool command, ret: %d", ret);
	} else {
		desh_print("rf_tool command started.");
	}
	return 0;

show_usage:
	desh_print_no_format(dect_phy_rf_tool_cmd_usage_str);
	return 0;
}

/**************************************************************************************************/

static const char dect_phy_ping_cmd_usage_str[] =
	"Usage: dect ping [stop] | [<server>|<client> [server_options]|[client_options]\n"
	"Note: make sure that accessed channel is free by using \"dect rssi_scan\" -command.\n"
	"\n"
	"Options:\n"
	"  -c, --client,              Client role.\n"
	"  -s, --server,              Server role.\n"
	"      --channel <int>,       Channel. Default 1665.\n"
	"  -t, --c_timeout <int>,     Ping timeout in milliseconds. Default: 1500 msecs.\n"
	"      --c_count <int>,       The number of times to send the ping request. Default: 5.\n"
	"  -l, --c_slots <int>,       Payload length (in slots) to be sent. Default: 1.\n"
	"  -i, --c_interval <int>,    Interval between successive packet transmissions\n"
	"                             in seconds. Default: 2 secs.\n"
	"      --c_tx_mcs <int>,      Set client TX MCS. Default: from common tx settings.\n"
	"      --c_tx_pwr <int>,      TX power (dBm),\n"
	"                             [-40,-30,-20,-16,-12,-8,-4,0,4,7,10,13,16,19,21,23].\n"
	"                             See supported max for the used band by using\n"
	"                             \"dect status\" -command output.\n"
	"                             Default: from common tx settings.\n"
	"      --c_tx_lbt_period <cnt>,  Listen Before Talk (LBT) period (symbol count).\n"
	"                                Zero value disables LBT (default).\n"
	"                                Valid range for symbol count: 2-110.\n"
	"      --c_tx_lbt_busy_th <dbm>, LBT busy RSSI threshold (dBm). Valid only when LBT\n"
	"                                is enabled. Default from RSSI busy threshold setting.\n"
	"      --s_tx_id <id>,        Target server transmitter id (default: 38).\n"
	"                             Note: lowest 16bits of transmitter id (long RD ID).\n"
	"  -e  --rx_exp_rssi_level <int>, Set expected RSSI level on RX (dBm).\n"
	"                                 Default: from common rx settings.\n"
	"  -m, --rssi_meas,           If set, RSSI measurements are enabled. Default: disabled.\n"
	"                             When enabled, RSSI measurements are printed after every 200\n"
	"                             measurements and after receiving PING_REQ/RESP\n"
	"                             (i.e. measurements are done before those).\n"
	"      --tx_pwr_ctrl_auto,    If given, sending side of the ping PDU tries to adjust TX\n"
	"                             power according to peer's expected RX RSSI level\n"
	"                             (informed in peer's PDU). Starting point is c_tx_pwr\n"
	"                             in a client side, and as received in server side.\n"
	"                             As a default, functionality is disabled.\n"
	"      --tx_pwr_ctrl_pdu_rx_exp_rssi_level <dbm>, Set expected RX RSSI level (dBm)\n"
	"                                                 announced in a ping PDU.\n"
	"                                                 Has impact only if tx_pwr_ctrl_auto\n"
	"                                                 is enabled.\n"
	"                                                 Default -60 dBm.\n"
	"For HARQ only:\n"
	"  -a, --use_harq,            Use HARQ. Client side requests HARQ feedback for sent ping\n"
	"                             requests and based on received HARQ ACK/NACK ping request\n"
	"                             is re-sent if possible within a timeout.\n";

enum {
	DECT_SHELL_PING_CHANNEL = 2001,
	DECT_SHELL_PING_COUNT,
	DECT_SHELL_PING_PAYLOAD_SLOT_COUNT,
	DECT_SHELL_PING_TX_PWR,
	DECT_SHELL_PING_TX_MCS,
	DECT_SHELL_PING_TX_LBT_PERIOD,
	DECT_SHELL_PING_TX_LBT_RSSI_BUSY_THRESHOLD,
	DECT_SHELL_PING_DEST_SERVER_TX_ID,
	DECT_SHELL_PING_TX_PWR_CTRL_AUTO,
	DECT_SHELL_PING_TX_PWR_CTRL_PDU_RX_EXPECTED_RSSI_LEVEL,
};

/* Specifying the expected options (both long and short): */
static struct option long_options_ping[] = {
	{"client", no_argument, 0, 'c'},
	{"server", no_argument, 0, 's'},
	{"rssi_meas", no_argument, 0, 'm'},
	{"use_harq", no_argument, 0, 'a'},
	{"channel", required_argument, 0, DECT_SHELL_PING_CHANNEL},
	{"c_timeout", required_argument, 0, 't'},
	{"c_count", required_argument, 0, DECT_SHELL_PING_COUNT},
	{"c_interval", required_argument, 0, 'i'},
	{"c_slots", required_argument, 0, 'l'},
	{"c_tx_pwr", required_argument, 0, DECT_SHELL_PING_TX_PWR},
	{"c_tx_mcs", required_argument, 0, DECT_SHELL_PING_TX_MCS},
	{"c_tx_lbt_period", required_argument, 0, DECT_SHELL_PING_TX_LBT_PERIOD },
	{"c_tx_lbt_busy_th", required_argument, 0, DECT_SHELL_PING_TX_LBT_RSSI_BUSY_THRESHOLD },
	{"rx_exp_rssi_level", required_argument, 0, 'e'},
	{"s_tx_id", required_argument, 0, DECT_SHELL_PING_DEST_SERVER_TX_ID},
	{"tx_pwr_ctrl_pdu_rx_exp_rssi_level", required_argument, 0,
	 DECT_SHELL_PING_TX_PWR_CTRL_PDU_RX_EXPECTED_RSSI_LEVEL},
	{"tx_pwr_ctrl_auto", no_argument, 0, DECT_SHELL_PING_TX_PWR_CTRL_AUTO},
	{0, 0, 0, 0}};

static int dect_phy_ping_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct dect_phy_ping_params params;
	int ret;
	int long_index = 0;
	int opt;
	int tmp_value;

	if (argc < 2) {
		goto show_usage;
	}
	optreset = 1;
	optind = 1;

	if (argv[1] != NULL && !strcmp(argv[1], "stop")) {
		desh_print("ping command stopping.");
		dect_phy_ctrl_ping_cmd_stop();
		return 0;
	}
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

	/* Set defaults */
	params.channel = 1665;
	params.timeout_msecs = 1500;
	params.interval_secs = 2;
	params.ping_count = 5;
	params.role = DECT_PHY_COMMON_ROLE_NONE;
	params.slot_count = 1;
	params.destination_transmitter_id = DECT_PHY_DEFAULT_TRANSMITTER_LONG_RD_ID;
	params.expected_rx_rssi_level = current_settings->rx.expected_rssi_level;
	params.tx_power_dbm = current_settings->tx.power_dbm;
	params.tx_mcs = current_settings->tx.mcs;
	params.tx_lbt_period_symbols = 0;
	params.tx_lbt_rssi_busy_threshold_dbm = current_settings->rssi_scan.busy_threshold;
	params.debugs = true;
	params.rssi_reporting_enabled = false;
	params.pwr_ctrl_pdu_expected_rx_rssi_level = -60;
	params.pwr_ctrl_automatic = false;
	params.use_harq = false;

	while ((opt = getopt_long(argc, argv, "i:e:t:l:csdmah", long_options_ping, &long_index)) !=
	       -1) {
		switch (opt) {
		case 'd': {
			params.debugs = true;
			break;
		}
		case 'c': {
			params.role = DECT_PHY_COMMON_ROLE_CLIENT;
			break;
		}
		case 'm': {
			params.rssi_reporting_enabled = true;
			break;
		}
		case 's': {
			params.role = DECT_PHY_COMMON_ROLE_SERVER;
			break;
		}
		case 't': {
			params.timeout_msecs = atoi(optarg);
			break;
		}
		case 'i': {
			params.interval_secs = atoi(optarg);
			break;
		}
		case 'e': {
			params.expected_rx_rssi_level = atoi(optarg);
			break;
		}
		case 'l': {
			ret = atoi(optarg);
			if (ret <= 0) {
				desh_error("Give decent value for the slot count (> 0)");
				goto show_usage;
			}
			params.slot_count = ret;
			break;
		}
		case 'a': {
			params.use_harq = true;
			break;
		}
		case DECT_SHELL_PING_DEST_SERVER_TX_ID: {
			params.destination_transmitter_id = atoi(optarg);
			break;
		}
		case DECT_SHELL_PING_TX_PWR: {
			params.tx_power_dbm = atoi(optarg);
			break;
		}
		case DECT_SHELL_PING_TX_MCS: {
			params.tx_mcs = atoi(optarg);
			break;
		}
		case DECT_SHELL_PING_TX_LBT_PERIOD: {
			tmp_value = atoi(optarg);
			if (tmp_value < DECT_PHY_LBT_PERIOD_MIN_SYM ||
				tmp_value > DECT_PHY_LBT_PERIOD_MAX_SYM) {
				desh_error("Invalid LBT period %d (range: [%d,%d])",
					   tmp_value,
					   DECT_PHY_LBT_PERIOD_MIN_SYM,
					   DECT_PHY_LBT_PERIOD_MAX_SYM);
				goto show_usage;
			}
			params.tx_lbt_period_symbols = tmp_value;
			break;
		}
		case DECT_SHELL_PING_TX_LBT_RSSI_BUSY_THRESHOLD: {
			tmp_value = atoi(optarg);
			if (tmp_value >= 0 ||
			    tmp_value < INT8_MIN) {
				desh_error("Invalid LBT RSSI busy threshold %d (range: [%d,-1])",
					   tmp_value,
					   INT8_MIN);
				goto show_usage;
			}
			params.tx_lbt_rssi_busy_threshold_dbm = tmp_value;
			break;
		}
		case DECT_SHELL_PING_CHANNEL: {
			params.channel = atoi(optarg);
			break;
		}
		case DECT_SHELL_PING_COUNT: {
			ret = atoi(optarg);
			if (ret <= 0) {
				desh_error("Give decent value for the ping count (> 0)");
				goto show_usage;
			}
			params.ping_count = ret;
			break;
		}
		case DECT_SHELL_PING_TX_PWR_CTRL_PDU_RX_EXPECTED_RSSI_LEVEL: {
			params.pwr_ctrl_pdu_expected_rx_rssi_level = atoi(optarg);
			break;
		}
		case DECT_SHELL_PING_TX_PWR_CTRL_AUTO: {
			params.pwr_ctrl_automatic = true;
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

	if (params.role == DECT_PHY_COMMON_ROLE_NONE) {
		desh_error("At least role need to be given. See usage:");
		goto show_usage;
	}

	if (params.timeout_msecs >= (params.interval_secs * 1000)) {
		desh_error("Timeout needs to be less than interval.");
		goto show_usage;
	}

	ret = dect_phy_ctrl_ping_cmd(&params);
	if (ret) {
		desh_error("Cannot start ping command, ret: %d", ret);
	} else {
		desh_print("ping command started.");
	}
	return 0;

show_usage:
	desh_print_no_format(dect_phy_ping_cmd_usage_str);
	return 0;
}

/**************************************************************************************************/

static void dect_phy_status_cmd(const struct shell *shell, size_t argc, char **argv)
{
	dect_phy_ctrl_status_get_n_print();
}

/**************************************************************************************************/

static const char dect_phy_activate_cmd_usage_str[] =
	"Usage: dect activate <mode>\n"
	"  Activate PHY software stack in given radio mode.\n"
	"Options:\n"
	"  -1, --low_latency,                Default. This mode has the lowest latency,\n"
	"                                    the best RX/TX switching performance,\n"
	"                                    and the highest power consumption.\n"
	"  -2, --low_latency_with_standby,   This mode has the same RX/TX switching\n"
	"                                    performance as the low latency mode,\n"
	"                                    but higher operation start-up latency.\n"
	"                                    Power consumption is thus lower compared to\n"
	"                                    the low latency mode.\n"
	"  -3, --non_lbt_with_standby,       This mode has the lowest power consumption,\n"
	"                                    due the to modem entering standby mode when possible\n"
	"                                    and not using LBT, at the cost\n"
	"                                    of higher start-up latency and worse RX/TX switching\n"
	"                                    performance compared to the other radio modes.\n";

/* Specifying the expected options (both long and short): */
static struct option long_options_radio_mode_conf[] = {
	{ "low_latency", no_argument, 0, '0' },
	{ "low_latency_with_standby", no_argument, 0, '1' },
	{ "non_lbt_with_standby", no_argument, 0, '2' },
	{ 0, 0, 0, 0 } };

static void dect_phy_activate_cmd(const struct shell *shell, size_t argc, char **argv)
{
	enum nrf_modem_dect_phy_radio_mode radio_mode = NRF_MODEM_DECT_PHY_RADIO_MODE_LOW_LATENCY;

	int long_index = 0;
	int opt, ret;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, "123h", long_options_radio_mode_conf, &long_index)) !=
	       -1) {
		switch (opt) {
		case '1':
			radio_mode = NRF_MODEM_DECT_PHY_RADIO_MODE_LOW_LATENCY;
			break;
		case '2':
			radio_mode = NRF_MODEM_DECT_PHY_RADIO_MODE_LOW_LATENCY_WITH_STANDBY;
			break;
		case '3':
			radio_mode = NRF_MODEM_DECT_PHY_RADIO_MODE_NON_LBT_WITH_STANDBY;
			break;
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

	ret = dect_phy_ctrl_activate_cmd(radio_mode);
	if (ret) {
		desh_error("Cannot start activate command, ret: %d", ret);
	}
	return;

show_usage:
	desh_print_no_format(dect_phy_activate_cmd_usage_str);
}

static void dect_phy_deactivate_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret = dect_phy_ctrl_deactivate_cmd();

	if (ret) {
		desh_error("Cannot start deactivate command, ret: %d", ret);
	} else {
		desh_print("Deactivation started.");
	}
}

/**************************************************************************************************/

static const char dect_phy_radio_mode_cmd_usage_str[] =
	"Usage: dect radio_mode <mode>\n"
	"  Configure radio mode.\n"
	"Options:\n"
	"  -1, --low_latency,                Default. This mode has the lowest latency,\n"
	"                                    the best RX/TX switching performance,\n"
	"                                    and the highest power consumption.\n"
	"  -2, --low_latency_with_standby,   This mode has the same RX/TX switching\n"
	"                                    performance as the low latency mode,\n"
	"                                    but higher operation start-up latency.\n"
	"                                    Power consumption is thus lower compared to\n"
	"                                    the low latency mode.\n"
	"  -3, --non_lbt_with_standby,       This mode has the lowest power consumption,\n"
	"                                    due the to modem entering standby mode when possible\n"
	"                                    and not using LBT, at the cost\n"
	"                                    of higher start-up latency and worse RX/TX switching\n"
	"                                    performance compared to the other radio modes.\n";

static void dect_phy_radio_mode_cmd(const struct shell *shell, size_t argc, char **argv)
{
	enum nrf_modem_dect_phy_radio_mode radio_mode = NRF_MODEM_DECT_PHY_RADIO_MODE_LOW_LATENCY;

	int long_index = 0;
	int opt, ret;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, "123h", long_options_radio_mode_conf, &long_index)) !=
	       -1) {
		switch (opt) {
		case '1':
			radio_mode = NRF_MODEM_DECT_PHY_RADIO_MODE_LOW_LATENCY;
			break;
		case '2':
			radio_mode = NRF_MODEM_DECT_PHY_RADIO_MODE_LOW_LATENCY_WITH_STANDBY;
			break;
		case '3':
			radio_mode = NRF_MODEM_DECT_PHY_RADIO_MODE_NON_LBT_WITH_STANDBY;
			break;
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

	ret = dect_phy_ctrl_radio_mode_cmd(radio_mode);
	if (ret) {
		desh_error("Cannot start activate command, ret: %d", ret);
	}
	return;

show_usage:
	desh_print_no_format(dect_phy_radio_mode_cmd_usage_str);
}

/**************************************************************************************************/

static void dect_phy_time_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret = dect_phy_ctrl_time_query();

	if (ret) {
		desh_error("Cannot query modem time: err %d", ret);
	}
}

/**************************************************************************************************/

static const char dect_rssi_scan_usage_str[] =
	"Usage: dect rssi_scan [list] | [stop] | [[-c <channel_nbr>]\n"
	"                                [-t <channel_scanning_time_ms>]\n"
	"                                [--busy_th <dbm_value>] [--free_th <dbm_value>]\n"
	"                                [-i <interval_secs>] [--force]]\n"
	"Options:\n"
	"  -c <ch_nbr>,         Channel to be scanned/measured. Zero value: all in a set band.\n"
	"                       Default: 1665.\n"
	"  -a, --only_allowed_channels,  Impacted only at band #1, scan only allowed channels per\n"
	"                                Harmonized standard requirements\n"
	"                                (ETSI EN 301 406-2, V3.0.1, ch 4.3.2.3\n"
	"                                Table 2 for 1,726MHz bandwidth).\n"
	"  --verdict_type_count,         Result verdicted based on SCAN_SUITABLE subslot count,\n"
	"                                as per MAC spec ch. 5.1.2.\n"
	"                                SCAN_SUITABLE can be adjusted from settings\n"
	"                                 (rssi_scan_suitable_percent).\n"
	"                                Default: Result verdicted based highest/lowest\n"
	"                                from on all measurements\n"
	"                                within a given ch_scanning_time_ms.\n"
	"  --verdict_type_count_details, Same as verdict_type_count but with printing subslot\n"
	"                                details for busy/possible subslots.\n"
	"  -i <interval_secs>,        Scanning interval in seconds.\n"
	"                             Default: no interval, i.e. one timer.\n"
	"  -t <ch_scanning_time_ms>,  Time that is used to scan per channel.\n"
	"                             Default from rssi_scan_time\n"
	"  --free_th <dbm_value>,     Channel access - considered as free:\n"
	"                             measured signal level <= <value>.\n"
	"                             Set a threshold for RSSI scan free threshold (dBm).\n"
	"                             Default from rssi_scan_busy_th\n"
	"                             Channel access - considered as possible:\n"
	"                             busy_th >= measured signal level > free_th\n"
	"  --busy_th <dbm_value>,     Channel access - considered as busy:\n"
	"                             measured signal level > <value>.\n"
	"                             Set a threshold for RSSI scan busy threshold (dBm).\n"
	"                             Default from rssi_scan_free_th\n"
	"  -f, --force,               Use special force to override current items in\n"
	"                             phy api scheduler so that RSSI scans are having priority "
	"over\n"
	"                             and re-init the modem phy api to cancel ongoing operations\n"
	"                             in modem. Note: when used, other running items are not\n"
	"                             scheduled to modem.\n";

/* The following do not have short options:: */
enum {
	DECT_SHELL_RSSI_SCAN_RESULT_VERDICT_TYPE_SUBSLOT_COUNT = 1001,
	DECT_SHELL_RSSI_SCAN_RESULT_VERDICT_TYPE_SUBSLOT_COUNT_DETAIL_PRINT,
	DECT_SHELL_RSSI_SCAN_FREE_TH,
	DECT_SHELL_RSSI_SCAN_BUSY_TH,
};

/* Specifying the expected options (both long and short): */
static struct option long_options_rssi_scan[] = {
	{"verdict_type_count", no_argument, 0,
		DECT_SHELL_RSSI_SCAN_RESULT_VERDICT_TYPE_SUBSLOT_COUNT},
	{"verdict_type_count_details", no_argument, 0,
		DECT_SHELL_RSSI_SCAN_RESULT_VERDICT_TYPE_SUBSLOT_COUNT_DETAIL_PRINT},
	{"free_th", required_argument, 0, DECT_SHELL_RSSI_SCAN_FREE_TH},
	{"busy_th", required_argument, 0, DECT_SHELL_RSSI_SCAN_BUSY_TH},
	{"force", no_argument, 0, 'f'},
	{"only_allowed_channels", no_argument, 0, 'a'},
	{0, 0, 0, 0}};

static int dect_phy_rssi_scan_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

	struct dect_phy_rssi_scan_params params;
	int ret;
	int long_index = 0;
	int opt;

	if (argc < 1) {
		goto show_usage;
	}

	optreset = 1;
	optind = 1;
	if (argv[1] != NULL && !strcmp(argv[1], "list")) {
		dect_phy_scan_rssi_latest_results_print();
	} else if (argv[1] != NULL && !strcmp(argv[1], "stop")) {
		dect_phy_ctrl_rssi_scan_stop();
	} else {
		/* Some defaults from settings */
		params.channel = 1665;
		params.result_verdict_type = DECT_PHY_RSSI_SCAN_RESULT_VERDICT_TYPE_ALL;
		params.scan_time_ms = current_settings->rssi_scan.time_per_channel_ms;
		params.type_subslots_params = current_settings->rssi_scan.type_subslots_params;
		params.type_subslots_params.detail_print = false;

		params.busy_rssi_limit = current_settings->rssi_scan.busy_threshold;
		params.free_rssi_limit = current_settings->rssi_scan.free_threshold;
		params.interval_secs = 0; /* One timer */
		params.stop_on_1st_free_channel = false;
		params.dont_stop_on_nbr_channels = false;
		params.only_allowed_channels = false;

		while ((opt = getopt_long(argc, argv, "c:i:t:fah", long_options_rssi_scan,
					  &long_index)) != -1) {
			switch (opt) {
			case 'c': {
				params.channel = atoi(optarg);
				break;
			}
			case 'i': {
				params.interval_secs = atoi(optarg);
				break;
			}
			case 't': {
				params.scan_time_ms = atoi(optarg);
				break;
			}
			case 'f': {
				params.suspend_scheduler = true;
				break;
			}
			case 'a': {
				params.only_allowed_channels = true;
				break;
			}
			case DECT_SHELL_RSSI_SCAN_RESULT_VERDICT_TYPE_SUBSLOT_COUNT_DETAIL_PRINT:
				params.type_subslots_params.detail_print = true;
			case DECT_SHELL_RSSI_SCAN_RESULT_VERDICT_TYPE_SUBSLOT_COUNT: {
				params.result_verdict_type =
					DECT_PHY_RSSI_SCAN_RESULT_VERDICT_TYPE_SUBSLOT_COUNT;
				break;
			}
			case DECT_SHELL_RSSI_SCAN_FREE_TH: {
				params.free_rssi_limit = atoi(optarg);
				break;
			}
			case DECT_SHELL_RSSI_SCAN_BUSY_TH: {
				params.busy_rssi_limit = atoi(optarg);
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

		if (params.channel != 0 && !dect_common_utils_channel_is_supported(
						   current_settings->common.band_nbr,
						   params.channel, params.only_allowed_channels)) {
			desh_error("Channel %d is not supported on set band #%d.\n", params.channel,
				   current_settings->common.band_nbr);
			goto show_usage;
		}

		ret = dect_phy_ctrl_rssi_scan_start(&params, NULL);
		if (ret) {
			desh_error("Cannot start RSSI scan, err %d", ret);
		} else {
			desh_print("RSSI scan started.");
		}
	}

	return 0;

show_usage:
	desh_print_no_format(dect_rssi_scan_usage_str);
	return 0;
}

/**************************************************************************************************/

static const char dect_rx_usage_str[] =
	"Usage: dect rx stop | start [-c <channel_nbr>] [-f] [-t <secs>]\n"
	"                            [-e <dbm>]\n"
	"                            [-i <secs] [--free_th <dbm_value>] [--busy_th <dbm_value>]\n"
	"Subcommands:\n"
	"  start,               Start RX.\n"
	"  stop,                Stop RX.\n"
	"Starting options:\n"
	"  -c <ch_nbr>,                 Channel number. Zero value: all channels in a set band.\n"
	"                               Default: 1665\n"
	"  -t, --c_scan_time <int>,     Scanning duration in seconds\n"
	"                               (default: 10 secs, max: 62 secs).\n"
	"  -e  --c_rx_exp_rssi_level <int>, Set client expected RSSI level on RX (dBm).\n"
	"                                   Default: 0 (fast AGC).\n"
	"      --use_filter,    Use filter for receiving packets destinated to this device,\n"
	"                       i.e. short network id and receiver identity\n"
	"                       (= tx id) from settings.\n"
	"                       Default: false, no filter.\n"
	"  -f, --force,         Use special force to override current items in phy api scheduler\n"
	"                       so that RX operation is having priority over others.\n"
	"                       Note: when used, other running items are not scheduled to modem.\n"
	"  -a, --use_all_channels,     Channel access override:\n"
	"                              this is overriding Harmonized standard\n"
	"                              requirements (ETSI EN 301 406-2, V3.0.1, ch 4.3.2.3,\n"
	"                              Table 2) to use only odd channel numbers at band #1.\n"
	"Following options only impacts on RSSI measurements during RX:\n"
	"  -i <interval_secs>,        RSSI measurement reporting interval in seconds.\n"
	"                             Default: 1 seconds, zero to disable.\n"
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

/* The following do not have short options:: */
enum {
	DECT_SHELL_RX_RSSI_SCAN_FREE_TH = 1001,
	DECT_SHELL_RX_RSSI_SCAN_BUSY_TH,
	DECT_SHELL_RX_USE_FILTER,
};

/* Specifying the expected options (both long and short): */
static struct option long_options_rx[] = {
	{"c_scan_time", required_argument, 0, 't'},
	{"c_rx_exp_rssi_level", required_argument, 0, 'e'},
	{"force", no_argument, 0, 'f'},
	{"use_all_channels", no_argument, 0, 'a'},
	{"free_th", required_argument, 0, DECT_SHELL_RX_RSSI_SCAN_FREE_TH},
	{"busy_th", required_argument, 0, DECT_SHELL_RX_RSSI_SCAN_BUSY_TH},
	{"use_filter", no_argument, 0, DECT_SHELL_RX_USE_FILTER},
	{0, 0, 0, 0}};

static int dect_phy_rx_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct dect_phy_settings *current_settings = dect_common_settings_ref_get();

	struct dect_phy_rx_cmd_params params;
	int ret;
	int long_index = 0;
	int opt;

	if (argc < 1) {
		goto show_usage;
	}

	optreset = 1;
	optind = 2;
	if (argv[1] != NULL && !strcmp(argv[1], "stop")) {
		dect_phy_ctrl_rx_stop();
	} else if (argv[1] != NULL && !strcmp(argv[1], "start")) {
		/* Defaults */
		params.handle = DECT_PHY_COMMON_RX_CMD_HANDLE;
		params.channel = 1665;
		params.suspend_scheduler = false;
		params.expected_rssi_level = 0;
		params.duration_secs = 10;
		params.busy_rssi_limit = current_settings->rssi_scan.busy_threshold;
		params.free_rssi_limit = current_settings->rssi_scan.free_threshold;
		params.rssi_interval_secs = 1;
		params.ch_acc_use_all_channels = false;

		/* No filters as a default */
		params.network_id = 0;
		params.filter.is_short_network_id_used = false;
		params.filter.receiver_identity = 0;
		params.filter.short_network_id = 0;

		while ((opt = getopt_long(argc, argv, "i:e:t:c:fha", long_options_rx,
					  &long_index)) != -1) {
			switch (opt) {
			case 'a': {
				params.ch_acc_use_all_channels = true;
				break;
			}
			case 'c': {
				params.channel = atoi(optarg);
				break;
			}
			case 'f': {
				params.suspend_scheduler = true;
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
			case DECT_SHELL_RX_RSSI_SCAN_FREE_TH: {
				params.free_rssi_limit = atoi(optarg);
				break;
			}
			case DECT_SHELL_RX_RSSI_SCAN_BUSY_TH: {
				params.busy_rssi_limit = atoi(optarg);
				break;
			}
			case DECT_SHELL_RX_USE_FILTER: {
				params.network_id = current_settings->common.network_id;
				params.filter.is_short_network_id_used = true;
				params.filter.receiver_identity =
					current_settings->common.transmitter_id;
				params.filter.short_network_id =
					(uint8_t)(current_settings->common.network_id & 0xFF);
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

		ret = dect_phy_ctrl_rx_start(&params, false);
		if (ret) {
			desh_error("Cannot start RX, err %d", ret);
		} else {
			desh_print("RX started.");
		}
	} else {
		goto show_usage;
	}

	return 0;

show_usage:
	desh_print_no_format(dect_rx_usage_str);
	return 0;
}

/**************************************************************************************************/

/* The following do not have short options:: */
enum {
	DECT_SHELL_SETT_COMMON_RSSI_SCAN_TIME_PER_CHANNEL,
	DECT_SHELL_SETT_COMMON_RSSI_SCAN_FREE_THRESHOLD,
	DECT_SHELL_SETT_COMMON_RSSI_SCAN_BUSY_THRESHOLD,
	DECT_SHELL_SETT_COMMON_RSSI_SCAN_SUITABLE_PERCENT,
	DECT_SHELL_SETT_COMMON_HARQ_MDM_PROCESS_COUNT,
	DECT_SHELL_SETT_COMMON_HARQ_MDM_EXPIRY_TIME,
	DECT_SHELL_SETT_COMMON_SUBSLOT_COUNT_HARQ_FEEDBACK_RX,
	DECT_SHELL_SETT_COMMON_SUBSLOT_COUNT_HARQ_FEEDBACK_RX_DELAY_SLOT_COUNT,
	DECT_SHELL_SETT_COMMON_SUBSLOT_COUNT_HARQ_FEEDBACK_TX_DELAY_SLOT_COUNT,
	DECT_SHELL_SETT_COMMON_RX_EXP_RSSI_LEVEL,
	DECT_SHELL_SETT_COMMON_TX_PWR,
	DECT_SHELL_SETT_COMMON_TX_MCS,
	DECT_SHELL_SETT_RESET_ALL,
};

static const char dect_phy_sett_cmd_usage_str[] =
	"Usage: dect sett <options>\n"
	"Options:\n"
	"  -r, --read,                Read current common settings\n"
	"      --reset,               Reset to default common settings\n"
	"  -n, --nw_id <#>,           Set network id (32bit)\n"
	"  -t, --tx_id <#>,           Set transmitter id (long RD ID)\n"
	"  -b, --band_nbr <#>,        Set used band.\n"
	"                             Impacted on when a channel is set as zero\n"
	"                             (e.g. in rssi_scan).\n"
	"                             Default: band #1. Other supported bands are:\n"
	"                             2, 4, 9 and 22.\n"
	"  -m, --radio_mode <#>,      Enable/Disable activation at start-up.\n"
	"                             With radio mode:\n"
	"                             1: Low latency (default).\n"
	"                             2: Low latency with standby.\n"
	"                             3: LBT (Listen Before Talk) disabled, with standby.\n"
	"                             4: Disabled and radio not activated at start-up.\n"
	"  -d, --sche_delay <usecs>,  Estimated scheduling delay (us).\n"
	"RSSI measurement settings:\n"
	"      --rssi_scan_time <msecs>,   Channel access: set the time (msec) that is used for\n"
	"                                  scanning per channel for RSSI measurements.\n"
	"                                  Default: 2 seconds + frame.\n"
	"      --rssi_scan_free_th <dbm>,  Channel access - considered as free:\n"
	"                                  measured signal level <= <value>.\n"
	"                                  Set a threshold for RSSI scan free threshold (dBm).\n"
	"      --rssi_scan_busy_th <dbm>,  Channel access - considered as busy:\n"
	"                                  measured signal level > <value>.\n"
	"                                  Set a threshold for RSSI scan busy threshold (dBm).\n"
	"                                  Channel access - considered as possible:\n"
	"                                  rssi_scan_busy_th >= measured signal level >\n"
	"                                  rssi_scan_free_th\n"
	"      --rssi_scan_suitable_percent <int>,  SCAN_SUITABLE as per MAC spec.\n"
	"                                           Impact only with subslot count based verdict.\n"
	"Common RX settings:\n"
	"      --rx_exp_rssi_level,        Set default expected RSSI level on RX (dBm).\n"
	"Common TX settings:\n"
	"      --tx_pwr <dbm>,             Set default TX power (dBm).\n"
	"                                  [-40,-30,-20,-16,-12,-8,-4,0,4,7,10,13,16,19,21,23]\n"
	"                                  See supported max for the used band by using\n"
	"                                 \"dect status\" -command output.\n"
	"      --tx_mcs <mcs>,             Set default MCS on TX (0-4).\n"
	"HARQ parameters in modem init:\n"
	"      --mdm_init_harq_process_count <int>,  Count of HARQ RX processes for modem init\n"
	"                                            [1,2,4,8]. Default: 4.\n"
	"                                            Note: this must match at both ends and\n"
	"                                            too big value will cause buffer overflows\n"
	"                                            (seen as PDC CRC errors) with higher\n"
	"                                            byte amounts (=mcs+slots),\n"
	"                                            i.e. the default 4 is too big with MCS4\n"
	"                                            in non-optimal environment.\n"
	"      --mdm_init_harq_expiry_time_us <int>, HARQ RX buffer expiry time, in microseconds.\n"
	"                                            Maximum supported value: 5000000.\n"
	"                                            Default: 2500000.\n"
	"Following impacted when HARQ feedback requested on a scheduler for TX operation:\n"
	"      --sche_harq_feedback_rx_delay_subslots <int>, Gap (in subslots) between our TX end\n"
	"                                                    and starting of a RX for HARQ\n"
	"                                                    feedback. Default: 4.\n"
	"      --sche_harq_feedback_rx_subslots <int>, RX duration (in subslots) for receiving\n"
	"                                              HARQ feedback for our TX.\n"
	"                                              Default: 18.\n"
	"Common settings for RX side for HARQ operation, e.g. sending HARQ feedback:\n"
	"      --sche_harq_feedback_tx_delay_subslots <int>, Gap between data RX and\n"
	"                                                    starting a TX for providing\n"
	"                                                    HARQ feedback if requested by\n"
	"                                                    client.\n"
	"                                                    Default: 4 (subslots).\n";

/* Specifying the expected options (both long and short): */
static struct option long_options_settings[] = {
	{"nw_id", required_argument, 0, 'n'},
	{"tx_id", required_argument, 0, 't'},
	{"band_nbr", required_argument, 0, 'b'},
	{"sche_delay", required_argument, 0, 'd'},
	{"radio_mode", required_argument, 0, 'm'},
	{"rx_exp_rssi_level", required_argument, 0, DECT_SHELL_SETT_COMMON_RX_EXP_RSSI_LEVEL},
	{"tx_pwr", required_argument, 0, DECT_SHELL_SETT_COMMON_TX_PWR},
	{"tx_mcs", required_argument, 0, DECT_SHELL_SETT_COMMON_TX_MCS},
	{"mdm_init_harq_process_count", required_argument, 0,
	 DECT_SHELL_SETT_COMMON_HARQ_MDM_PROCESS_COUNT},
	{"mdm_init_harq_expiry_time_us", required_argument, 0,
	 DECT_SHELL_SETT_COMMON_HARQ_MDM_EXPIRY_TIME},
	{"sche_harq_feedback_rx_delay_subslots", required_argument, 0,
	 DECT_SHELL_SETT_COMMON_SUBSLOT_COUNT_HARQ_FEEDBACK_RX_DELAY_SLOT_COUNT},
	{"sche_harq_feedback_rx_subslots", required_argument, 0,
	 DECT_SHELL_SETT_COMMON_SUBSLOT_COUNT_HARQ_FEEDBACK_RX},
	{"sche_harq_feedback_tx_delay_subslots", required_argument, 0,
	 DECT_SHELL_SETT_COMMON_SUBSLOT_COUNT_HARQ_FEEDBACK_TX_DELAY_SLOT_COUNT},
	{"rssi_scan_time", required_argument, 0, DECT_SHELL_SETT_COMMON_RSSI_SCAN_TIME_PER_CHANNEL},
	{"rssi_scan_free_th", required_argument, 0,
	 DECT_SHELL_SETT_COMMON_RSSI_SCAN_FREE_THRESHOLD},
	{"rssi_scan_busy_th", required_argument, 0,
	 DECT_SHELL_SETT_COMMON_RSSI_SCAN_BUSY_THRESHOLD},
	{"rssi_scan_suitable_percent", required_argument, 0,
	 DECT_SHELL_SETT_COMMON_RSSI_SCAN_SUITABLE_PERCENT},
	{"reset", no_argument, 0, DECT_SHELL_SETT_RESET_ALL},
	{"read", no_argument, 0, 'r'},
	{0, 0, 0, 0}};

static void dect_phy_sett_cmd_print(struct dect_phy_settings *dect_sett)
{
	char tmp_str[128] = {0};

	desh_print("Common settings:");
	desh_print("  network id (32bit).............................%u (0x%08x)",
		   dect_sett->common.network_id, dect_sett->common.network_id);
	desh_print("  transmitter id (long RD ID)....................%u (0x%08x)",
		   dect_sett->common.transmitter_id, dect_sett->common.transmitter_id);
	desh_print("  short RD ID....................................%u (0x%04x)",
		   dect_sett->common.short_rd_id, dect_sett->common.short_rd_id);
	desh_print("  band number....................................%d",
		   dect_sett->common.band_nbr);
	if (dect_sett->common.activate_at_startup) {
		desh_print("  Startup activation radio mode..................%s",
			   dect_common_utils_radio_mode_to_string(
				dect_sett->common.startup_radio_mode, tmp_str));
	} else {
		desh_print("  Startup activation.............................Disabled");
	}
	desh_print("Common RX settings:");
	desh_print("    expected RSSI level..........................%d",
		   dect_sett->rx.expected_rssi_level);
	desh_print("Common TX settings:");
	desh_print("    power (dBm)..................................%d", dect_sett->tx.power_dbm);
	desh_print("    MCS..........................................%d", dect_sett->tx.mcs);
	desh_print("Common RSSI scanning settings:");
	desh_print("  ch access: RSSI scan (msecs) per ch............%d",
		   dect_sett->rssi_scan.time_per_channel_ms);
	desh_print("  ch access: RSSI scan busy (dBm):...............signal level > %d",
		   dect_sett->rssi_scan.busy_threshold);
	desh_print("  ch access: RSSI scan possible (dBm):...........%d >= signal level > %d",
		   dect_sett->rssi_scan.busy_threshold, dect_sett->rssi_scan.free_threshold);
	desh_print("  ch access: RSSI scan free (dBm):...............signal level <= %d",
		   dect_sett->rssi_scan.free_threshold);
	desh_print("  ch access: SCAN_SUITABLE%%......................%d%%",
		   dect_sett->rssi_scan.type_subslots_params.scan_suitable_percent);
	desh_print("Scheduler common settings:");
	desh_print("  scheduling offset/delay (us).............................%d",
		   dect_sett->scheduler.scheduling_delay_us);
	desh_print("Common HARQ settings:");
	desh_print("  Modem init HARQ process count............................%d",
		   dect_sett->harq.mdm_init_harq_process_count);
	desh_print("  Modem init HARQ RX buffer expiry time (in microseconds)..%d",
		   dect_sett->harq.mdm_init_harq_expiry_time_us);

	desh_print("  HARQ feedback RX delay (subslots)........................%d",
		   dect_sett->harq.harq_feedback_rx_delay_subslot_count);
	desh_print("  HARQ feedback RX duration (subslots).....................%d",
		   dect_sett->harq.harq_feedback_rx_subslot_count);
	desh_print("  HARQ feedback TX delay (subslots)........................%d",
		   dect_sett->harq.harq_feedback_tx_delay_subslot_count);
}

static int dect_phy_sett_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct dect_phy_settings current_settings;
	struct dect_phy_settings newsettings;

	optreset = 1;
	optind = 1;

	int ret = 0;

	int long_index = 0;
	int opt, tmp_value;

	if (argc < 2) {
		goto show_usage;
	}
	dect_common_settings_read(&current_settings);
	newsettings = current_settings;

	while ((opt = getopt_long(
			argc, argv, "m:d:n:t:b:rh", long_options_settings, &long_index)) != -1) {
		switch (opt) {
		case 'r': {
			dect_common_settings_read(&current_settings);
			dect_phy_sett_cmd_print(&current_settings);
			return 0;
		}
		case 'n': {
			tmp_value = shell_strtoul(optarg, 10, &ret);
			if (ret || !dect_common_utils_32bit_network_id_validate(tmp_value)) {
				desh_error("%u (0x%08x) is not a valid network id.\n"
					   "The network ID shall be set to a value where neither\n"
					   "the 8 LSB bits are 0x00 nor the 24 MSB bits "
					   "are 0x000000.",
					   tmp_value, tmp_value);
				return -EINVAL;
			}
			newsettings.common.network_id = tmp_value;
			break;
		}
		case 'm': {
			tmp_value = atoi(optarg);
			newsettings.common.activate_at_startup = true;
			if (tmp_value == 1) {
				newsettings.common.startup_radio_mode =
					NRF_MODEM_DECT_PHY_RADIO_MODE_LOW_LATENCY;
			} else if (tmp_value == 2) {
				newsettings.common.startup_radio_mode =
					NRF_MODEM_DECT_PHY_RADIO_MODE_LOW_LATENCY_WITH_STANDBY;
			} else if (tmp_value == 3) {
				newsettings.common.startup_radio_mode =
					NRF_MODEM_DECT_PHY_RADIO_MODE_NON_LBT_WITH_STANDBY;
			} else if (tmp_value == 4) {
				newsettings.common.activate_at_startup = false;
			} else {
				desh_error("Give decent value for Radio Mode [1-3].", tmp_value);
				return -EINVAL;
			}
			break;
		}
		case 't': {
			tmp_value = shell_strtoul(optarg, 10, &ret);
			if (ret) {
				desh_error("Give decent tx id (> 0)");
				return -EINVAL;
			}
			newsettings.common.transmitter_id = tmp_value;
			break;
		}
		case 'b': {
			tmp_value = atoi(optarg);
			if (tmp_value == 1 || tmp_value == 2 || tmp_value == 4 || tmp_value == 9 ||
			    tmp_value == 22) {
				newsettings.common.band_nbr = tmp_value;
			} else {
				desh_error("Band #%d is not supported.", tmp_value);
				return -EINVAL;
			}
			if ((newsettings.common.band_nbr == 4 &&
			     current_settings.common.band_nbr != 4) ||
			    (current_settings.common.band_nbr == 4 &&
			     newsettings.common.band_nbr != 4)) {
				desh_warn("Note: Band change to/from 4 requires "
					  "a reboot or a reactivate.");
			}
			break;
		}
		case 'd': {
			newsettings.scheduler.scheduling_delay_us = atoi(optarg);
			break;
		}
		case DECT_SHELL_SETT_COMMON_RX_EXP_RSSI_LEVEL: {
			newsettings.rx.expected_rssi_level = atoi(optarg);
			break;
		}
		case DECT_SHELL_SETT_COMMON_TX_PWR: {
			newsettings.tx.power_dbm = atoi(optarg);
			break;
		}
		case DECT_SHELL_SETT_COMMON_TX_MCS: {
			newsettings.tx.mcs = atoi(optarg);
			break;
		}
		case DECT_SHELL_SETT_COMMON_HARQ_MDM_PROCESS_COUNT: {
			tmp_value = atoi(optarg);
			if (tmp_value != 1 && tmp_value != 2 && tmp_value != 4 && tmp_value != 8) {
				desh_error("Not valid HARQ process count.");
				goto show_usage;
			}
			newsettings.harq.mdm_init_harq_process_count = tmp_value;
			break;
		}
		case DECT_SHELL_SETT_COMMON_HARQ_MDM_EXPIRY_TIME: {
			tmp_value = atoi(optarg);
			if (tmp_value <= 0 || tmp_value > 5000000) {
				desh_error("Not valid HARQ rx buffer expiration time given.");
				goto show_usage;
			}
			newsettings.harq.mdm_init_harq_expiry_time_us = tmp_value;
			break;
		}

		case DECT_SHELL_SETT_COMMON_SUBSLOT_COUNT_HARQ_FEEDBACK_RX_DELAY_SLOT_COUNT: {
			newsettings.harq.harq_feedback_rx_delay_subslot_count = atoi(optarg);
			break;
		}
		case DECT_SHELL_SETT_COMMON_SUBSLOT_COUNT_HARQ_FEEDBACK_RX: {
			newsettings.harq.harq_feedback_rx_subslot_count = atoi(optarg);
			break;
		}
		case DECT_SHELL_SETT_COMMON_SUBSLOT_COUNT_HARQ_FEEDBACK_TX_DELAY_SLOT_COUNT: {
			newsettings.harq.harq_feedback_tx_delay_subslot_count = atoi(optarg);
			break;
		}
		case DECT_SHELL_SETT_COMMON_RSSI_SCAN_TIME_PER_CHANNEL: {
			newsettings.rssi_scan.time_per_channel_ms = atoi(optarg);
			break;
		}
		case DECT_SHELL_SETT_COMMON_RSSI_SCAN_FREE_THRESHOLD: {
			tmp_value = atoi(optarg);
			if (tmp_value >= 0) {
				desh_error("Give decent value (value < 0)");
				return -EINVAL;
			}
			newsettings.rssi_scan.free_threshold = tmp_value;
			break;
		}
		case DECT_SHELL_SETT_COMMON_RSSI_SCAN_BUSY_THRESHOLD: {
			tmp_value = atoi(optarg);
			if (tmp_value >= 0) {
				desh_error("Give decent value (value < 0)");
				return -EINVAL;
			}
			newsettings.rssi_scan.busy_threshold = tmp_value;
			break;
		}
		case DECT_SHELL_SETT_COMMON_RSSI_SCAN_SUITABLE_PERCENT: {
			tmp_value = atoi(optarg);
			if (tmp_value < 0 || tmp_value > 100) {
				desh_error("Give decent value (0-100)");
				return -EINVAL;
			}
			newsettings.rssi_scan.type_subslots_params.scan_suitable_percent =
				tmp_value;
			break;
		}
		case DECT_SHELL_SETT_RESET_ALL: {
			dect_common_settings_defaults_set();
			goto settings_updated;
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

	dect_common_settings_write(&newsettings);
settings_updated:
	dect_phy_ctrl_msgq_non_data_op_add(DECT_PHY_CTRL_OP_SETTINGS_UPDATED);
	return 0;

show_usage:
	desh_print_no_format(dect_phy_sett_cmd_usage_str);
	return 0;
}

/**************************************************************************************************/

/* A placeholder for all dect commands that are configured in the dect_*_shell.c files */
SHELL_SUBCMD_SET_CREATE(dect_cmds, (dect));

SHELL_CMD_REGISTER(dect, &dect_cmds, "Commands for MoSh DECT NR+ Phy.", dect_phy_dect_cmd);

SHELL_SUBCMD_ADD((dect), sche_list_status, NULL,
		 "Get dect desh scheduler status.\n"
		 " Usage: dect sche_list_status",
		 dect_phy_scheduler_list_status_print_cmd, 1, 0);
SHELL_SUBCMD_ADD((dect), sche_list_purge, NULL,
		 "Flush items in dect desh scheduler.\n"
		 " Usage: dect sche_list_purge",
		 dect_phy_scheduler_list_purge_cmd, 1, 0);
SHELL_SUBCMD_ADD((dect), status, NULL,
		 "Print desh dect status.\n"
		 " Usage: dect status",
		 dect_phy_status_cmd, 1, 0);
SHELL_SUBCMD_ADD((dect), time, NULL,
		 "Print dect time.\n"
		 " Usage: dect time",
		 dect_phy_time_cmd, 1, 0);
SHELL_SUBCMD_ADD((dect), sett, NULL,
		 "Set and read common dect settings.\n"
		 " Usage: dect sett -h",
		 dect_phy_sett_cmd, 1, 10);
SHELL_SUBCMD_ADD((dect), activate, NULL,
		 "Activate radio.\n"
		 " Usage: dect activate -h",
		 dect_phy_activate_cmd, 1, 10);
SHELL_SUBCMD_ADD((dect), deactivate, NULL,
		 "De-activate radio.\n"
		 " Usage: dect deactivate",
		 dect_phy_deactivate_cmd, 1, 0);
SHELL_SUBCMD_ADD((dect), radio_mode, NULL,
		 "Activate radio.\n"
		 " Usage: dect radio_mode -h",
		 dect_phy_radio_mode_cmd, 1, 10);
SHELL_SUBCMD_ADD((dect), rssi_scan, NULL,
		 "Execute RSSI measurement/scan.\n"
		 " Usage: dect rssi_scan [options: see: dect rssi_scan -h]",
		 dect_phy_rssi_scan_cmd, 1, 6);
SHELL_SUBCMD_ADD((dect), rx, NULL,
		 "RX command.\n"
		 " Usage: dect rx [options: see: dect rx -h]",
		 dect_phy_rx_cmd, 1, 6);
SHELL_SUBCMD_ADD((dect), perf, NULL,
		 "dect perf command.\n"
		 " Usage: dect perf [options: see: dect perf -h]",
		 dect_phy_perf_cmd, 1, 35);
SHELL_SUBCMD_ADD((dect), rf_tool, NULL,
		 "RX/TX tool for helping running of ETSI EN 301 406-2:\n"
		 " sections 4.3 (Conformance requirements for transmitter) and\n"
		 " 4.4 (Conformance requirements for receiver).\n"
		 " Usage: dect rf_tool [options: see: dect rf_tool -h]",
		 dect_phy_rf_tool_cmd, 1, 35);
SHELL_SUBCMD_ADD((dect), ping, NULL,
		 "dect ping command.\n"
		 " Usage: dect ping [options: see: dect ping -h]",
		 dect_phy_ping_cmd, 1, 35);
