/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#include <zephyr/net/socket.h>

#if defined(CONFIG_MODEM_CELLULAR)
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/cellular.h>
#endif
#include <zephyr/net/ethernet.h> /* just for ETH_P_ALL */

#include <zephyr/sys/printk.h>
#include <zephyr/init.h>

#include <zephyr/sys/sys_getopt.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>

#include <net/dect/dect_utils.h>

#if defined(CONFIG_NET_CONNECTION_MANAGER)
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#endif
#include <net/dect/dect_net_l2.h>
#include <net/dect/dect_net_l2_mgmt.h>
#include <net/dect/dect_net_l2_shell_util.h>

#define DECT_L2_SHELL_STR_BUFF_SIZE 128

#define ANSI_RESET_ALL	  "\x1b[0m"
#define ANSI_COLOR_RED	  "\x1b[31m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_GREEN  "\x1b[32m"
#define ANSI_COLOR_BLUE	  "\x1b[34m"

static struct {
	struct net_if *iface;
	const struct shell *shell;
	struct dect_net_l2_shell_print_fns print_fns;
	bool custom_print_enabled;
} context = {
	.iface = NULL,
	.shell = NULL,
	.print_fns = {0},
	.custom_print_enabled = false,
};
static struct net_mgmt_event_callback dect_shell_mgmt_cb;

/* RSSI scan command */

/* The following do not have short options: */
enum {
	DECT_SHELL_RSSI_SCAN_CMD_FRAMES,
};

static const char dect_shell_rssi_scan_usage_str[] =
	"Usage: dect rssi_scan <options>\n\n"
	"Options:\n"
	"      --frames <nbr_of_frames>,  Number of frames to be scanned on a channel.\n"
	"  -c  --channels <int,int,..>    Channel list (comma separated). Max 20 channels.\n"
	"  -b  --band <int>               Band number. If given, given channels ignored and\n"
	"                                 scanning whole band).\n";

/* Specifying the expected options (both long and short): */
static struct sys_getopt_option long_options_rssi_scan[] = {
	{"channels", sys_getopt_required_argument, 0, 'c'},
	{"band", sys_getopt_required_argument, 0, 'b'},
	{"frames", sys_getopt_required_argument, 0, DECT_SHELL_RSSI_SCAN_CMD_FRAMES},
	{0, 0, 0, 0}};

	static const char dect_shell_scan_usage_str[] =
	"Usage: dect scan <options>\n\n"
	"Options:\n"
	"  -t  --scan_time <int>          Time to wait in a channel for beacons [1, 60000] ms.\n"
	"                                 Default: 3000.\n"
	"  -c  --channels <int,int,..>    Channel list (comma separated), MAX 4 channels.\n"
	"  -b  --band <int>               Band number. If given, given channels ignored and "
	"scanning\n"
	"                                 whole band).\n";

/* Network scan command */

/* Specifying the expected options (both long and short): */
static struct sys_getopt_option long_options_nw_scan[] = {
	{"scan_time", sys_getopt_required_argument, 0, 't'},
	{"channels", sys_getopt_required_argument, 0, 'c'},
	{"band", sys_getopt_required_argument, 0, 'b'},
	{0, 0, 0, 0}
};

/* Settings command */

static const char dect_shell_sett_common_usage_str[] =
	"Usage: dect sett <options>\n"
	"Options:\n"
	"  -r, --read,                  Read current common settings.\n"
	"      --reset,                 Reset to driver default settings.\n"
	"  -n, --nw_id <#>,             Set network id (32bit).\n"
	"  -t, --tx_id <#>,             Set transmitter id (long RD ID).\n"
	"      --region <eu/us/global>, Set region or variant. Impacts e.g. in channel access.\n"
	"  -b, --band_nbr <#>,          Set used band.\n"
	"      --max_tx_pwr <dbm>,      Set max TX power (dBm). Note: max depends on supported\n"
	"                               capabilities, i.e. power class.\n"
	"                               [-40,-30,-20,-16,-12,-8,-4,0,4,7,10,13,16,19,21,23]\n"
	"      --max_mcs <uint>,        Set max used MCS.\n"
	"      --power_save <on/off>,  \"on\" to enable power save on modem, \"off\" to disable.\n"
	"                               Note: PT device only.\n"
	"      --dev_type <dev_type>    Set device type. dev_type: \"FT\" or \"PT\".\n";
static const char dect_shell_sett_auto_start_usage_str[] =
	"      --auto_activate <on/off>, \"on\" to enable activation at bootup, \"off\" to "
	"disable.\n"
	"                                Default: on.\n"
	"      --nw_join_target <#>,     Set target transmitter id (long RD ID) of FT\n"
	"                                for nw_join of the PT device. Default: 0 (=any).\n";
static const char dect_shell_sett_rssi_scan_usage_str[] =
	"RSSI measurement settings:\n"
	"      --rssi_scan_time <msecs>,   Channel access: set the time (msec) that is used for\n"
	"                                  scanning per channel for RSSI measurements.\n"
	"                                  Range: [10, 2550].\n"
	"      --rssi_scan_free_th <dbm>,  Channel access: considered as free:\n"
	"                                  measured signal level <= <value>.\n"
	"                                  Set a threshold for RSSI scan free threshold (dBm).\n"
	"      --rssi_scan_busy_th <dbm>,  Channel access: considered as busy:\n"
	"                                  measured signal level > <value>.\n"
	"                                  Set a threshold for RSSI scan busy threshold (dBm).\n"
	"                                  Channel access: considered as possible:\n"
	"                                  rssi_scan_busy_th >= measured signal level >\n"
	"                                  rssi_scan_free_th\n"
	"      --rssi_scan_suitable_percent <int>,  SCAN_SUITABLE% as per spec.\n";
static const char dect_shell_sett_association_usage_str[] =
	"Association related settings\n"
	"      --max_beacon_rx_fails <int>, Set maximum number of consecutive missed cluster\n"
	"                                   beacons. Note: set to modem when creating association\n"
	"                                   and has impact when FT device is considered as out of\n"
	"                                   range or FT is turned off, resulting the automatic\n"
	"                                   disassociation of the FT device.\n"
	"                                   Value 0 means that no limit and no automatic\n"
	"				    disassociation is done.\n"
	"      --min_sensitivity <dbm>,     Set minimum RX sensitivity (dBm). Has impact\n"
	"                                   when selecting RD for association.\n"
	"                                   MIN_SENSITIVITY_LEVEL per MAC spec.\n";
static const char dect_shell_sett_cluster_usage_str1[] =
	"Cluster beacon settings\n"
	"      --cluster_beacon_period <#>,   Set cluster beacon period in ms. Possible values:\n"
	"                                     10, 50, 100, 500, 1000, 1500, 2000, 4000, 8000,\n"
	"                                     16000 and 32000\n"
	"      --cluster_max_tx_pwr <dbm>,    Set max TX power (dBm) in cluster.\n"
	"                                     Range: [-12,-8,-4,0,4,7,10,13,16,19,21,23].\n"
	"      --cluster_max_beacon_tx_pwr <dbm>, Set max beacon TX power (dBm). Range:\n"
	"                                    [-40,-30,-20,-16,-12,-8,-4,0,4,7,10,13,16,19,21,23]\n";
static const char dect_shell_sett_cluster_usage_str2[] =
	"      --cluster_ch_reselection_th <int>, CHANNEL_LOADED percent per MAC spec.\n"
	"                                     Threshold percent when an operating channel\n"
	"                                     load is so high that the RD should start\n"
	"                                     channel reselection.\n"
	"                                     Setting to zero disables the feature and FT is not\n"
	"                                     initiating channel reselection automatically.\n"
	"      --cluster_nbr_inactivity_time <ms>, Neighbor inactivity timer (in ms) that\n"
	"                                          triggers association releasing.\n"
	"                                          Use 0 to disable timer.\n";
static const char dect_shell_sett_network_beacon_usage_str[] =
	"Network beacon settings\n"
	"      --nw_beacon_period <#>,        Set network beacon period in ms. Possible values:\n"
	"                                     50, 100, 500, 1000, 1500, 2000 and 4000.\n"
	"      --nw_beacon_channel <#>,       Set channel to be used for the network beacon.\n"
	"                                     This has impact only in network_create.\n"
	"                                     Default: disabled.\n";
static const char dect_shell_sett_sec_conf_usage_str[] =
	"Security configuration settings\n"
	"      --sec_mode <none/mode_1>,      Set security mode.\n"
	"                                     Default: mode_1.\n"
	"      --sec_integ_key <key>,         Set security key (hex string). Length 16.\n"
	"                                     Example: 0123456789abcdef0123456789abcdef\n"
	"      --sec_cipher_key <key>,        Set cipher key (hex string). Length 16.\n"
	"                                     Example: 0123456789abcdef0123456789abcdef\n";

/* The following do not have short options: */
enum {
	DECT_SHELL_SETT_CMD_RESET_ALL,
	DECT_SHELL_SETT_CMD_AUTO_ACTIVATE,
	DECT_SHELL_SETT_CMD_NW_JOIN_TARGET,
	DECT_SHELL_SETT_CMD_DEV_TYPE,
	DECT_SHELL_SETT_CMD_REGION,
	DECT_SHELL_SETT_CMD_RSSI_SCAN_TIME_PER_CHANNEL,
	DECT_SHELL_SETT_CMD_RSSI_SCAN_FREE_THRESHOLD,
	DECT_SHELL_SETT_CMD_RSSI_SCAN_BUSY_THRESHOLD,
	DECT_SHELL_SETT_CMD_RSSI_SCAN_SUITABLE_PERCENT,
	DECT_SHELL_SETT_CMD_CLUSTER_BEACON_PERIOD,
	DECT_SHELL_SETT_CMD_CLUSTER_CHANNEL_LOADED_PERCENT,
	DECT_SHELL_SETT_CMD_CLUSTER_MAX_BEACON_TX_PWR,
	DECT_SHELL_SETT_CMD_CLUSTER_MAX_TX_PWR,
	DECT_SHELL_SETT_CMD_CLUSTER_NBR_INACTIVITY_TIME,
	DECT_SHELL_SETT_CMD_ASSOCIATION_MAX_CLUSTER_BEACON_RX_FAILS,
	DECT_SHELL_SETT_CMD_ASSOCIATION_MIN_SENSITIVITY,
	DECT_SHELL_SETT_CMD_NW_BEACON_PERIOD,
	DECT_SHELL_SETT_CMD_NW_BEACON_CHANNEL,
	DECT_SHELL_SETT_CMD_PWR_SAVE,
	DECT_SHELL_SETT_CMD_MAX_MCS,
	DECT_SHELL_SETT_CMD_MAX_TX_PWR,
	DECT_SHELL_SETT_CMD_SEC_MODE,
	DECT_SHELL_SETT_CMD_SEC_INTEG_KEY,
	DECT_SHELL_SETT_CMD_SEC_CIPHER_KEY,
};

/* Specifying the expected options (both long and short): */
static struct sys_getopt_option long_options_sett_cmd[] = {
	{"nw_id", sys_getopt_required_argument, 0, 'n'},
	{"tx_id", sys_getopt_required_argument, 0, 't'},
	{"band_nbr", sys_getopt_required_argument, 0, 'b'},
	{"reset", sys_getopt_no_argument, 0, DECT_SHELL_SETT_CMD_RESET_ALL},
	{"region", sys_getopt_required_argument, 0, DECT_SHELL_SETT_CMD_REGION},
	{"auto_activate", sys_getopt_required_argument, 0, DECT_SHELL_SETT_CMD_AUTO_ACTIVATE},
	{"nw_join_target", sys_getopt_required_argument, 0, DECT_SHELL_SETT_CMD_NW_JOIN_TARGET},
	{"dev_type", sys_getopt_required_argument, 0, DECT_SHELL_SETT_CMD_DEV_TYPE},
	{"power_save", sys_getopt_required_argument, 0, DECT_SHELL_SETT_CMD_PWR_SAVE},
	{"max_mcs", sys_getopt_required_argument, 0, DECT_SHELL_SETT_CMD_MAX_MCS},
	{"max_tx_pwr", sys_getopt_required_argument, 0, DECT_SHELL_SETT_CMD_MAX_TX_PWR},
	{"rssi_scan_time", sys_getopt_required_argument, 0,
	 DECT_SHELL_SETT_CMD_RSSI_SCAN_TIME_PER_CHANNEL},
	{"rssi_scan_free_th", sys_getopt_required_argument, 0,
	 DECT_SHELL_SETT_CMD_RSSI_SCAN_FREE_THRESHOLD},
	{"rssi_scan_busy_th", sys_getopt_required_argument, 0,
	 DECT_SHELL_SETT_CMD_RSSI_SCAN_BUSY_THRESHOLD},
	{"rssi_scan_suitable_percent", sys_getopt_required_argument, 0,
	 DECT_SHELL_SETT_CMD_RSSI_SCAN_SUITABLE_PERCENT},
	{"cluster_beacon_period", sys_getopt_required_argument, 0,
	 DECT_SHELL_SETT_CMD_CLUSTER_BEACON_PERIOD},
	{"cluster_max_beacon_tx_pwr", sys_getopt_required_argument, 0,
	 DECT_SHELL_SETT_CMD_CLUSTER_MAX_BEACON_TX_PWR},
	{"cluster_ch_reselection_th", sys_getopt_required_argument, 0,
	 DECT_SHELL_SETT_CMD_CLUSTER_CHANNEL_LOADED_PERCENT},
	{"cluster_max_tx_pwr", sys_getopt_required_argument, 0,
	 DECT_SHELL_SETT_CMD_CLUSTER_MAX_TX_PWR},
	{"cluster_nbr_inactivity_time", sys_getopt_required_argument, 0,
	 DECT_SHELL_SETT_CMD_CLUSTER_NBR_INACTIVITY_TIME},
	{"nw_beacon_period", sys_getopt_required_argument, 0, DECT_SHELL_SETT_CMD_NW_BEACON_PERIOD},
	{"nw_beacon_channel", sys_getopt_required_argument, 0,
	 DECT_SHELL_SETT_CMD_NW_BEACON_CHANNEL},
	{"max_beacon_rx_fails", sys_getopt_required_argument, 0,
	 DECT_SHELL_SETT_CMD_ASSOCIATION_MAX_CLUSTER_BEACON_RX_FAILS},
	{"min_sensitivity", sys_getopt_required_argument, 0,
	 DECT_SHELL_SETT_CMD_ASSOCIATION_MIN_SENSITIVITY},
	{"sec_mode", sys_getopt_required_argument, 0, DECT_SHELL_SETT_CMD_SEC_MODE},
	{"sec_integ_key", sys_getopt_required_argument, 0, DECT_SHELL_SETT_CMD_SEC_INTEG_KEY},
	{"sec_cipher_key", sys_getopt_required_argument, 0, DECT_SHELL_SETT_CMD_SEC_CIPHER_KEY},
	{0, 0, 0, 0}};

	static const char dect_shell_cluster_reconfig_usage_str[] =
	"Usage: dect cluster_reconfig [optional options]\n"
	"Options:\n"
	"  -c  <int>                          New channel on a set band.\n"
	"                                     Default: any suitable channel.\n"
	"      --cluster_beacon_period <#>,   Set cluster beacon period in ms. Possible values:\n"
	"                                     10, 50, 100, 500, 1000, 1500, 2000, 4000, 8000,\n"
	"                                     16000 and 32000.\n"
	"                                     Default: from current cluster setting.\n"
	"      --cluster_max_tx_pwr <dbm>,    Set max TX power (dBm) in cluster. Range:\n"
	"                                     [-12,-8,-4,0,4,7,10,13,16,19,21,23].\n"
	"                                     Default: from current cluster setting.\n"
	"      --cluster_max_beacon_tx_pwr <dbm>, Set max beacon TX power (dBm) Range:\n"
	"                                     [-40,-30,-20,-16,-12,-8,-4,0,4,7,10,13,16,19,21,23]."
	"                                     Default: from current cluster setting.\n";

/* Cluster reconfigure command */

/* The following do not have short options: */
enum {
	DECT_SHELL_CLUSTER_RECONFIG_CMD_CLUSTER_BEACON_PERIOD,
	DECT_SHELL_CLUSTER_RECONFIG_CMD_CLUSTER_MAX_BEACON_TX_PWR,
	DECT_SHELL_CLUSTER_RECONFIG_CMD_CLUSTER_MAX_TX_PWR,
};

/* Specifying the expected options (both long and short): */
static struct sys_getopt_option long_options_cluster_reconfig_cmd[] = {
	{"cluster_beacon_period", sys_getopt_required_argument, 0,
	 DECT_SHELL_CLUSTER_RECONFIG_CMD_CLUSTER_BEACON_PERIOD},
	{"cluster_max_beacon_tx_pwr", sys_getopt_required_argument, 0,
	 DECT_SHELL_CLUSTER_RECONFIG_CMD_CLUSTER_MAX_BEACON_TX_PWR},
	{"cluster_max_tx_pwr", sys_getopt_required_argument, 0,
	 DECT_SHELL_CLUSTER_RECONFIG_CMD_CLUSTER_MAX_TX_PWR},
	{0, 0, 0, 0}};

	static const char dect_shell_nw_beacon_start_usage_str[] =
	"Usage: dect nw_beacon_start -c <channel_nbr> [--add_channels <list>]\n\n"
	"Options:\n"
	"  -c  <int>                       Network beacon channel on a set band.\n"
	"      --add_channels <int,int,..> Additional Network beacon channels list.\n"
	"                                  At most 3 additional channels can be listed.\n"
	"                                  Channels are listed in sent Network beacon.\n ";

/* Network beacon start command */

/* The following do not have short options: */
enum {
	DECT_SHELL_NW_BEACON_START_CMD_ADD_CHANNELS,
};

/* Specifying the expected options (both long and short): */
static struct sys_getopt_option long_options_nw_beacon_start[] = {
	{"add_channels", sys_getopt_required_argument, 0,
	 DECT_SHELL_NW_BEACON_START_CMD_ADD_CHANNELS},
	{"channel", sys_getopt_required_argument, 0, 'c'},
	{0, 0, 0, 0}};

/* RX command */

static int dect_shell_rx_sockfd = -1;
#define DATA_RX_POLL_TIMEOUT_MS 1000 /* Milliseconds */
static char rx_data_buf[DECT_MTU];

#define DECT_SHELL_RX_THREAD_STACK_SIZE 1024
#define DECT_SHELL_RX_THREAD_PRIORITY	5

K_SEM_DEFINE(dect_shell_rx_thread_sem, 0, 1);

static const char dect_shell_rx_cmd_usage_str[] = "Usage: dect rx start | stop\n";

/* Associate command */

static const char dect_shell_sett_associate_usage_str[] =
	"Usage: dect associate <options>\n"
	"Options:\n"
	"  -t  --target <int>          Target long RD ID of the FT device.\n";

/* Specifying the expected options (both long and short): */
static struct sys_getopt_option long_options_associate[] = {
	{"target", sys_getopt_required_argument, 0, 't'},
	{0, 0, 0, 0}
};

/* Dissociate command */

static const char dect_shell_sett_dissociate_usage_str[] =
	"Usage: dect dissociate <options>\n"
	"Options:\n"
	"  -t  --target <int>          Target long RD ID of the FT device.\n";

/* Specifying the expected options (both long and short): */
static struct sys_getopt_option long_options_dissociate[] = {
	{"target", sys_getopt_required_argument, 0, 't'},
	{0, 0, 0, 0}
};

/* Store shell from commands for use as default in print functions */
static inline void set_shell(const struct shell *shell)
{
	if (shell != NULL) {
		context.shell = shell;
	}
}

/* Wrapper macros for print functions:
 * - If custom print functions are set (via dect_net_l2_shell_init), use those (override)
 * - Otherwise, use the shell from commands (context.shell) as default
 * - If no shell is available, fall back to printk
 */
#define dect_l2_shell_print(...)                                                                   \
	do {                                                                                       \
		if (context.custom_print_enabled && context.print_fns.print_fn) {                  \
			context.print_fns.print_fn(context.shell, ##__VA_ARGS__);                  \
		} else if (context.shell != NULL) {                                                \
			shell_print(context.shell, ##__VA_ARGS__);                                 \
		} else {                                                                           \
			printk(__VA_ARGS__);                                                       \
		}                                                                                  \
	} while (0)

#define dect_l2_shell_error(...)                                                                   \
	do {                                                                                       \
		if (context.custom_print_enabled && context.print_fns.error_fn) {                  \
			context.print_fns.error_fn(context.shell, ##__VA_ARGS__);                  \
		} else if (context.shell != NULL) {                                                \
			shell_error(context.shell, ##__VA_ARGS__);                                 \
		} else {                                                                           \
			printk(__VA_ARGS__);                                                       \
		}                                                                                  \
	} while (0)

#define dect_l2_shell_warn(...)                                                                    \
	do {                                                                                       \
		if (context.custom_print_enabled && context.print_fns.warn_fn) {                   \
			context.print_fns.warn_fn(context.shell, ##__VA_ARGS__);                   \
		} else if (context.shell != NULL) {                                                \
			shell_warn(context.shell, ##__VA_ARGS__);                                  \
		} else {                                                                           \
			printk(__VA_ARGS__);                                                       \
		}                                                                                  \
	} while (0)

#define DECT_SHELL_MGMT_EVENTS                                                                     \
	(NET_EVENT_DECT_ACTIVATE_DONE | NET_EVENT_DECT_DEACTIVATE_DONE |                           \
	 NET_EVENT_DECT_RSSI_SCAN_RESULT | NET_EVENT_DECT_RSSI_SCAN_DONE |                         \
	 NET_EVENT_DECT_SCAN_DONE | NET_EVENT_DECT_SCAN_RESULT |                                   \
	 NET_EVENT_DECT_ASSOCIATION_CHANGED | NET_EVENT_DECT_CLUSTER_INFO |                        \
	 NET_EVENT_DECT_CLUSTER_CREATED_RESULT | NET_EVENT_DECT_NEIGHBOR_LIST |                    \
	 NET_EVENT_DECT_NEIGHBOR_INFO | NET_EVENT_DECT_NW_BEACON_START_RESULT |                    \
	 NET_EVENT_DECT_NW_BEACON_STOP_RESULT | NET_EVENT_DECT_NEIGHBOR_INFO |                     \
	 NET_EVENT_DECT_NETWORK_STATUS | NET_EVENT_DECT_SINK_STATUS)

static char *net_sprint_addr(sa_family_t af, const void *addr)
{
#define NBUFS 3
	static char buf[NBUFS][NET_IPV6_ADDR_LEN];
	static int i;
	char *s = buf[++i % NBUFS];

	return net_addr_ntop(af, addr, s, NET_IPV6_ADDR_LEN);
}
#define net_sprint_ipv6_addr(_addr) net_sprint_addr(AF_INET6, _addr)

char *dect_net_l2_shell_util_mac_err_to_string(enum dect_status_values status, char *out_str_buff,
					       size_t out_str_buff_len)
{
	if (out_str_buff == NULL || out_str_buff_len == 0) {
		return NULL;
	}

	switch (status) {
	case DECT_STATUS_OK:
		strncpy(out_str_buff, "OK", out_str_buff_len);
		break;
	case DECT_STATUS_FAIL:
		strncpy(out_str_buff, "Fail", out_str_buff_len);
		break;
	case DECT_STATUS_NOT_ALLOWED:
		strncpy(out_str_buff, "Not allowed", out_str_buff_len);
		break;
	case DECT_STATUS_NO_CONFIG:
		strncpy(out_str_buff, "No config", out_str_buff_len);
		break;
	case DECT_STATUS_RD_NOT_FOUND:
		strncpy(out_str_buff, "RD not found", out_str_buff_len);
		break;
	case DECT_STATUS_TEMP_FAILURE:
		strncpy(out_str_buff, "Temporary failure", out_str_buff_len);
		break;
	case DECT_STATUS_NO_RESOURCES:
		strncpy(out_str_buff, "No resources", out_str_buff_len);
		break;
	case DECT_STATUS_NO_RESPONSE:
		strncpy(out_str_buff, "No response", out_str_buff_len);
		break;
	case DECT_STATUS_NW_REJECT:
		strncpy(out_str_buff, "Network reject", out_str_buff_len);
		break;
	case DECT_STATUS_NO_MEMORY:
		strncpy(out_str_buff, "No memory", out_str_buff_len);
		break;
	case DECT_STATUS_NO_RSSI_RESULTS:
		strncpy(out_str_buff, "No RSSI results", out_str_buff_len);
		break;
	case DECT_STATUS_OS_ERROR:
		strncpy(out_str_buff, "OS error", out_str_buff_len);
		break;
	default:
		snprintf(out_str_buff, out_str_buff_len, "Unknown (%d)", status);
		break;
	}

	return out_str_buff;
}
static bool dect_shell_util_hexstr_check(const uint8_t *data, uint16_t data_len)
{
	for (int i = 0; i < data_len; i++) {
		char ch = *(data + i);

		if ((ch < '0' || ch > '9') && (ch < 'A' || ch > 'F') && (ch < 'a' || ch > 'f')) {
			return false;
		}
	}

	return true;
}

/* Convert hex string to hex array */
int dect_shell_util_atoh(const char *ascii, uint16_t ascii_len, uint8_t *hex, uint16_t hex_len)
{
	char hex_str[3];

	if (hex == NULL || ascii == NULL) {
		return -EINVAL;
	}
	if ((ascii_len % 2) > 0) {
		return -EINVAL;
	}
	if (ascii_len > (hex_len * 2)) {
		return -EINVAL;
	}
	if (!dect_shell_util_hexstr_check(ascii, ascii_len)) {
		return -EINVAL;
	}

	hex_str[2] = '\0';
	for (int i = 0; (i * 2) < ascii_len; i++) {
		strncpy(&hex_str[0], ascii + (i * 2), 2);
		*(hex + i) = (uint8_t)strtoul(hex_str, NULL, 16);
	}

	return (ascii_len / 2);
}

/* Convert from hex array to hex string */
int dect_shell_util_htoa(const uint8_t *hex, uint16_t hex_len, char *ascii, uint16_t ascii_len)
{
	if (hex == NULL || ascii == NULL) {
		return -EINVAL;
	}
	if (ascii_len < (hex_len * 2)) {
		return -EINVAL;
	}

	for (int i = 0; i < hex_len; i++) {
		snprintf(ascii + (i * 2), ascii_len - (i * 2), "%02X", *(hex + i));
	}

	return (hex_len * 2);
}

static char *
dect_shell_util_association_reject_time_to_string(enum dect_association_reject_time reject_time,
						  char *out_str_buff, size_t out_str_buff_len)
{
	if (out_str_buff == NULL || out_str_buff_len == 0) {
		return NULL;
	}

	switch (reject_time) {
	case DECT_ASSOCIATION_REJECT_TIME_0S:
		strncpy(out_str_buff, "0 seconds", out_str_buff_len);
		break;
	case DECT_ASSOCIATION_REJECT_TIME_5S:
		strncpy(out_str_buff, "5 seconds", out_str_buff_len);
		break;
	case DECT_ASSOCIATION_REJECT_TIME_10S:
		strncpy(out_str_buff, "10 seconds", out_str_buff_len);
		break;
	case DECT_ASSOCIATION_REJECT_TIME_30S:
		strncpy(out_str_buff, "30 seconds", out_str_buff_len);
		break;
	case DECT_ASSOCIATION_REJECT_TIME_60S:
		strncpy(out_str_buff, "60 seconds", out_str_buff_len);
		break;
	case DECT_ASSOCIATION_REJECT_TIME_120S:
		strncpy(out_str_buff, "120 seconds", out_str_buff_len);
		break;
	case DECT_ASSOCIATION_REJECT_TIME_180S:
		strncpy(out_str_buff, "180 seconds", out_str_buff_len);
		break;
	case DECT_ASSOCIATION_REJECT_TIME_300S:
		strncpy(out_str_buff, "300 seconds", out_str_buff_len);
		break;
	case DECT_ASSOCIATION_REJECT_TIME_600S:
		strncpy(out_str_buff, "600 seconds", out_str_buff_len);
		break;
	default:
		snprintf(out_str_buff, out_str_buff_len, "Unknown (%d)", reject_time);
		break;
	}
	return out_str_buff;
}

static char *
dect_shell_util_association_reject_cause_to_string(enum dect_association_reject_cause reject_cause,
						   char *out_str_buff, size_t out_str_buff_len)
{
	if (out_str_buff == NULL || out_str_buff_len == 0) {
		return NULL;
	}

	switch (reject_cause) {
	case DECT_ASSOCIATION_REJECT_CAUSE_NO_RADIO_CAPACITY:
		strncpy(out_str_buff, "No radio capacity", out_str_buff_len);
		break;
	case DECT_ASSOCIATION_REJECT_CAUSE_NO_HW_CAPACITY:
		strncpy(out_str_buff, "No hardware capacity", out_str_buff_len);
		break;
	case DECT_ASSOCIATION_REJECT_CAUSE_CONFLICTED_SHORT_ID:
		strncpy(out_str_buff, "Conflicted short ID", out_str_buff_len);
		break;
	case DECT_ASSOCIATION_REJECT_CAUSE_SECURITY_NEEDED:
		strncpy(out_str_buff, "Security needed", out_str_buff_len);
		break;
	case DECT_ASSOCIATION_REJECT_CAUSE_OTHER_REASON:
		strncpy(out_str_buff, "Other reason", out_str_buff_len);
		break;
	case DECT_ASSOCIATION_NO_RESPONSE:
		strncpy(out_str_buff, "No response", out_str_buff_len);
		break;
	default:
		snprintf(out_str_buff, out_str_buff_len, "Unknown (%d)", reject_cause);
		break;
	}
	return out_str_buff;
}

static char *
dect_shell_util_association_release_cause_to_string(enum dect_association_release_cause cause,
						    char *out_str_buff, size_t out_str_buff_len)
{
	if (out_str_buff == NULL || out_str_buff_len == 0) {
		return NULL;
	}

	switch (cause) {
	case DECT_RELEASE_CAUSE_CONNECTION_TERMINATION:
		strncpy(out_str_buff, "Connection termination", out_str_buff_len);
		break;
	case DECT_RELEASE_CAUSE_MOBILITY:
		strncpy(out_str_buff, "Mobility", out_str_buff_len);
		break;
	case DECT_RELEASE_CAUSE_LONG_INACTIVITY:
		strncpy(out_str_buff, "Long inactivity", out_str_buff_len);
		break;
	case DECT_RELEASE_CAUSE_INCOMPATIBLE_CONFIGURATION:
		strncpy(out_str_buff, "Incompatible configuration", out_str_buff_len);
		break;
	case DECT_RELEASE_CAUSE_INSUFFICIENT_HW_RESOURCES:
		strncpy(out_str_buff, "Insufficient HW/memory resources", out_str_buff_len);
		break;
	case DECT_RELEASE_CAUSE_INSUFFICIENT_RADIO_RESOURCES:
		strncpy(out_str_buff, "Insufficient radio resources", out_str_buff_len);
		break;
	case DECT_RELEASE_CAUSE_BAD_RADIO_QUALITY:
		strncpy(out_str_buff, "Bad radio quality", out_str_buff_len);
		break;
	case DECT_RELEASE_CAUSE_SECURITY_ERROR:
		strncpy(out_str_buff, "Security error", out_str_buff_len);
		break;
	case DECT_RELEASE_CAUSE_OTHER_ERROR:
		strncpy(out_str_buff, "Other error", out_str_buff_len);
		break;
	case DECT_RELEASE_CAUSE_OTHER_REASON:
		strncpy(out_str_buff, "Other reason", out_str_buff_len);
		break;
	case DECT_RELEASE_CAUSE_CUSTOM_RACH_RESOURCE_FAILURE:
		strncpy(out_str_buff, "RACH resource failure", out_str_buff_len);
		break;
	default:
		snprintf(out_str_buff, out_str_buff_len, "Unknown (%d)", cause);
		break;
	}

	return out_str_buff;
}

static char *dect_shell_util_rssi_scan_result_verdict_to_string(int verdict, char *out_str_buff,
								size_t out_str_buff_len)
{
	if (out_str_buff == NULL || out_str_buff_len == 0) {
		return NULL;
	}
	size_t current_len = strlen(out_str_buff);
	size_t remaining = (current_len < out_str_buff_len) ? (out_str_buff_len - current_len) : 0;

	if (verdict == DECT_RSSI_SCAN_VERDICT_FREE) {
		snprintf(out_str_buff + current_len, remaining, "%sF%s",
			 ANSI_COLOR_GREEN, ANSI_RESET_ALL);
	} else if (verdict == DECT_RSSI_SCAN_VERDICT_POSSIBLE) {
		snprintf(out_str_buff + current_len, remaining, "%sP%s",
			 ANSI_COLOR_YELLOW, ANSI_RESET_ALL);
	} else if (verdict == DECT_RSSI_SCAN_VERDICT_BUSY) {
		snprintf(out_str_buff + current_len, remaining, "%sB%s",
			 ANSI_COLOR_RED, ANSI_RESET_ALL);
	} else {
		snprintf(out_str_buff + current_len, remaining, "%sU%s",
			 ANSI_COLOR_BLUE, ANSI_RESET_ALL);
	}
	return out_str_buff;
}

static void handle_dect_rssi_scan_result_evt(struct net_mgmt_event_callback *cb)
{
	const struct dect_rssi_scan_result_evt *evt =
		(const struct dect_rssi_scan_result_evt *)cb->info;
	struct dect_rssi_scan_result_data *entry =
		(struct dect_rssi_scan_result_data *)&evt->rssi_scan_result;
	char verdict_str[48 + ((48 * 6) * 2) + 1] = {0};
	char *tmp_ptr = verdict_str;

	verdict_str[0] = '\0';

	dect_l2_shell_print("RSSI scan result:");
	dect_l2_shell_print("  Channel:                             %d",
			    entry->channel);
	dect_l2_shell_print("  All subslots free:                   %s",
			    entry->all_subslots_free ? "yes" : "no");
	if (entry->another_cluster_detected_in_channel) {
		/* Currently only when selecting channel when creating a cluster */
		dect_l2_shell_print("  Another cluster detected in channel: %s",
				    entry->another_cluster_detected_in_channel ? "yes" : "no");
	}
	dect_l2_shell_print("  Busy percentage:                     %d%%",
			    entry->busy_percentage);
	if (!entry->all_subslots_free) {
		dect_l2_shell_print("  Free subslot count:                  %d",
				    entry->free_subslot_cnt);
		dect_l2_shell_print("  Possible subslot count:              %d",
				    entry->possible_subslot_cnt);
		dect_l2_shell_print("  Busy subslot count:                  %d",
				    entry->busy_subslot_cnt);
		dect_l2_shell_print("  Scan suitable percent:               %d%%",
				    entry->scan_suitable_percent);
		dect_l2_shell_print("  Frame subslot verdicts: "
						     "(Free=F, Possible=P, Busy=B, Unknown=U)");

		for (int i = 0; i < sizeof(entry->frame_subslot_verdicts); i++) {
			tmp_ptr = dect_shell_util_rssi_scan_result_verdict_to_string(
				entry->frame_subslot_verdicts[i], tmp_ptr, sizeof(verdict_str));
		}
		dect_l2_shell_print("    %s", verdict_str);
	}
}

static void handle_dect_scan_result_evt(struct net_mgmt_event_callback *cb)
{
	const struct dect_scan_result_evt *entry = (const struct dect_scan_result_evt *)cb->info;

	dect_l2_shell_print("Scan result:");
	dect_l2_shell_print("  Beacon type:             %s",
			    entry->beacon_type == DECT_SCAN_RESULT_TYPE_NW_BEACON ? "NW"
										  : "Cluster");
	dect_l2_shell_print("  Reception channel:       %d", entry->channel);
	dect_l2_shell_print("  Long RD ID:              %u (0x%08x)",
			    entry->transmitter_long_rd_id, entry->transmitter_long_rd_id);
	dect_l2_shell_print("  NW ID:                   %u (0x%08x)",
			    entry->network_id, entry->network_id);
	dect_l2_shell_print("  RX RSSI-2:               %ddBm",
			    entry->rx_signal_info.rssi_2);
	dect_l2_shell_print("  RX SNR:                  %ddB",
			    entry->rx_signal_info.snr);
	dect_l2_shell_print("  RX MCS index:            %d",
			    entry->rx_signal_info.mcs);
	dect_l2_shell_print(
		"  RX Transmit power:       %d (%d dBm)",
		entry->rx_signal_info.transmit_power,
		dect_utils_lib_phy_tx_power_to_dbm(entry->rx_signal_info.transmit_power));
	if (entry->beacon_type == DECT_SCAN_RESULT_TYPE_NW_BEACON) {
		dect_l2_shell_print("  Current cluster channel: %d",
				    entry->network_beacon.current_cluster_channel);
		dect_l2_shell_print("  Next cluster channel:    %d",
				    entry->network_beacon.next_cluster_channel);
		for (int i = 0; i < entry->network_beacon.num_network_beacon_channels; i++) {
			dect_l2_shell_print(
					    "  Additional network beacon channel #%d: %d", i + 1,
					    entry->network_beacon.network_beacon_channels[i]);
		}
	}
	if (entry->has_route_info) {
		dect_l2_shell_print("  Route info:");
		dect_l2_shell_print("    Route cost: %d", entry->route_info.route_cost);
		dect_l2_shell_print("    Application sequence number: %d",
			entry->route_info.application_sequence_number);
		dect_l2_shell_print("    Sink address: %u (0x%08x)",
			entry->route_info.sink_address, entry->route_info.sink_address);
	}
}

static void handle_dect_scan_done(struct net_mgmt_event_callback *cb)
{
	const struct dect_common_resp_evt *evt = (const struct dect_common_resp_evt *)cb->info;

	if (evt->status != DECT_STATUS_OK) {
		char err_str[DECT_L2_SHELL_STR_BUFF_SIZE] = {0};

		dect_net_l2_shell_util_mac_err_to_string(evt->status, err_str, sizeof(err_str));

		dect_l2_shell_warn("Scan request failed: %d (%s)", evt->status,
				   err_str);
	} else {
		dect_l2_shell_print("Scan request done");
	}
}
static void dect_shell_net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
					      uint64_t mgmt_event, struct net_if *iface)
{
	char err_str[DECT_L2_SHELL_STR_BUFF_SIZE] = {0};

	switch (mgmt_event) {
	case NET_EVENT_DECT_ACTIVATE_DONE: {
		const struct dect_common_resp_evt *evt =
			(const struct dect_common_resp_evt *)cb->info;

		if (evt->status != DECT_STATUS_OK) {
			dect_net_l2_shell_util_mac_err_to_string(evt->status, err_str,
								 sizeof(err_str));
			dect_l2_shell_warn(
				"NET_EVENT_DECT_ACTIVATE_DONE: activation failed: %d (%s)",
				evt->status, err_str);
		} else {
			dect_l2_shell_print(
					    "NET_EVENT_DECT_ACTIVATE_DONE: activation done");
		}
		break;
	}
	case NET_EVENT_DECT_DEACTIVATE_DONE: {
		const struct dect_common_resp_evt *evt =
			(const struct dect_common_resp_evt *)cb->info;

		if (evt->status != DECT_STATUS_OK) {
			dect_net_l2_shell_util_mac_err_to_string(evt->status, err_str,
								 sizeof(err_str));
			dect_l2_shell_warn(
				"NET_EVENT_DECT_DEACTIVATE_DONE: de-activation failed: %d (%s)",
				evt->status, err_str);
		} else {
			dect_l2_shell_print(
					    "NET_EVENT_DECT_DEACTIVATE_DONE: de-activation done");
		}
		break;
	}

	case NET_EVENT_DECT_RSSI_SCAN_RESULT:
		dect_l2_shell_print("NET_EVENT_DECT_RSSI_SCAN_RESULT");
		handle_dect_rssi_scan_result_evt(cb);
		break;
	case NET_EVENT_DECT_RSSI_SCAN_DONE: {
		const struct dect_common_resp_evt *evt =
			(const struct dect_common_resp_evt *)cb->info;

		if (evt->status != DECT_STATUS_OK) {
			dect_net_l2_shell_util_mac_err_to_string(evt->status, err_str,
								 sizeof(err_str));

			dect_l2_shell_warn(
					   "NET_EVENT_DECT_RSSI_SCAN_DONE: scan failed: %d (%s)",
					   evt->status, err_str);
		} else {
			dect_l2_shell_print(
					    "NET_EVENT_DECT_RSSI_SCAN_DONE: scan done");
		}
		break;
	}
	case NET_EVENT_DECT_SCAN_RESULT:
		dect_l2_shell_print("NET_EVENT_DECT_SCAN_RESULT");
		handle_dect_scan_result_evt(cb);
		break;
	case NET_EVENT_DECT_SCAN_DONE:
		dect_l2_shell_print("NET_EVENT_DECT_SCAN_DONE");
		handle_dect_scan_done(cb);
		break;
	case NET_EVENT_DECT_ASSOCIATION_CHANGED: {
		const struct dect_association_changed_evt *evt_data =
			(const struct dect_association_changed_evt *)cb->info;

		dect_l2_shell_print("NET_EVENT_DECT_ASSOCIATION_CHANGED");
		if (evt_data->association_change_type == DECT_ASSOCIATION_REQ_REJECTED) {
			dect_l2_shell_print(" DECT_ASSOCIATION_REQ_REJECTED:");
			dect_l2_shell_print(
				"  Association request failed with long RD ID:          %u",
				evt_data->long_rd_id);
			dect_l2_shell_print(
				"  Neighbor role:                                       %s",
				evt_data->neighbor_role == DECT_NEIGHBOR_ROLE_PARENT ? "Parent"
										     : "Child");
			dect_l2_shell_print(
				"  Reject cause:                                        %s (%d)",
				dect_shell_util_association_reject_cause_to_string(
					evt_data->association_req_rejected.reject_cause, err_str,
					sizeof(err_str)),
				evt_data->association_req_rejected.reject_cause);
			dect_l2_shell_print(
				"  Reject time:                                         %s sec",
				dect_shell_util_association_reject_time_to_string(
					evt_data->association_req_rejected.reject_time, err_str,
					sizeof(err_str)));
		} else if (evt_data->association_change_type == DECT_ASSOCIATION_REQ_FAILED_MDM) {
			dect_l2_shell_print(" DECT_ASSOCIATION_REQ_FAILED_MDM:");
			dect_l2_shell_print(
				"  Association request failed in modem with long RD ID: %u",
				evt_data->long_rd_id);
			dect_l2_shell_print(
				"  Neighbor role:                                       %s",
				evt_data->neighbor_role == DECT_NEIGHBOR_ROLE_PARENT ? "Parent"
										     : "Child");
			dect_l2_shell_print(
				"  Modem cause:                                         %s (%d)",
				dect_net_l2_shell_util_mac_err_to_string(
					evt_data->association_req_failed_mdm.cause, err_str,
					sizeof(err_str)),
				evt_data->association_req_failed_mdm.cause);
		} else if (evt_data->association_change_type == DECT_ASSOCIATION_CREATED) {
			dect_l2_shell_print(" DECT_ASSOCIATION_CREATED:");
			dect_l2_shell_print(
				"  Association created with long RD ID:                 %u",
				evt_data->long_rd_id);
			dect_l2_shell_print(
				"  Neighbor role:                                       %s",
				evt_data->neighbor_role == DECT_NEIGHBOR_ROLE_PARENT ? "Parent"
										     : "Child");
		} else if (evt_data->association_change_type == DECT_ASSOCIATION_RELEASED) {
			dect_l2_shell_print(" DECT_ASSOCIATION_RELEASED:");
			dect_l2_shell_print(
				"  Association released with long RD ID:                %u",
				evt_data->long_rd_id);
			dect_l2_shell_print(
				"  Neighbor role:                                       %s",
				evt_data->neighbor_role == DECT_NEIGHBOR_ROLE_PARENT ? "Parent"
										     : "Child");
			dect_l2_shell_print(
				"  Initiator:                                           %s",
				evt_data->association_released.neighbor_initiated ? "Neighbor"
										  : "Local");
			dect_l2_shell_print(
				"  Release cause:                                        %s (%d)",
				dect_shell_util_association_release_cause_to_string(
					evt_data->association_released.release_cause, err_str,
					sizeof(err_str)),
				evt_data->association_released.release_cause);
		} else {
			dect_l2_shell_error(" Unknown association change type: %d",
					    evt_data->association_change_type);
		}
		break;
	}
	case NET_EVENT_DECT_CLUSTER_CREATED_RESULT: {
		const struct dect_cluster_start_resp_evt *resp_data =
			(const struct dect_cluster_start_resp_evt *)cb->info;

		dect_l2_shell_print("NET_EVENT_DECT_CLUSTER_CREATED_RESULT");
		if (resp_data->status != DECT_STATUS_OK) {
			dect_net_l2_shell_util_mac_err_to_string(resp_data->status, err_str,
								 sizeof(err_str));

			dect_l2_shell_warn(
					   "Cluster start/reconfigure failed: %d (%s)",
					   resp_data->status, err_str);

		} else {
			dect_l2_shell_print(
					    "Cluster started/reconfigured at channel %d.",
					    resp_data->cluster_channel);
		}
		break;
	}
	case NET_EVENT_DECT_CLUSTER_INFO: {
		const struct dect_cluster_info_evt *resp_data =
			(const struct dect_cluster_info_evt *)cb->info;
		dect_l2_shell_print("NET_EVENT_DECT_CLUSTER_INFO");
		if (resp_data->status != DECT_STATUS_OK) {
			dect_l2_shell_warn("Cluster info request failed (%d)",
					   resp_data->status);
		} else {
			struct dect_rssi_scan_result_data *entry =
				(struct dect_rssi_scan_result_data *)&resp_data->status_info
					.rssi_result;

			char verdict_str[48 + ((48 * 6) * 2) + 1] = {0};
			char *tmp_ptr = verdict_str;

			dect_l2_shell_print("Cluster status information:");
			dect_l2_shell_print(
					    "  Cluster channel:                 %d",
					    resp_data->status_info.rssi_result.channel);
			dect_l2_shell_print(
					    "  Number of Association requests:  %d",
					    resp_data->status_info.num_association_requests);
			dect_l2_shell_print(
					    "  Number of Association failures:  %d",
					    resp_data->status_info.num_association_failures);
			dect_l2_shell_print(
					    "  Number of neighbors:             %d",
					    resp_data->status_info.num_neighbors);
			dect_l2_shell_print(
					    "  Number of FTPT neighbors:        %d",
					    resp_data->status_info.num_ftpt_neighbors);
			dect_l2_shell_print(
					    "  Number of received RACH PDCs:    %d",
					    resp_data->status_info.num_rach_rx_pdc);
			dect_l2_shell_print(
					    "  Number of RACH PCC CRC failures: %d",
					    resp_data->status_info.num_rach_rx_pcc_crc_failures);
			dect_l2_shell_print(
					    "  Current RSSI scan result of cluster channel:");
			dect_l2_shell_print(
					    "    Channel:                       %d",
					    resp_data->status_info.rssi_result.channel);
			dect_l2_shell_print(
					    "    Busy percentage:               %d%%",
					    resp_data->status_info.rssi_result.busy_percentage);
			dect_l2_shell_print(
					    "  All subslots free:               %s",
					    entry->all_subslots_free ? "yes" : "no");
			if (!entry->all_subslots_free) {
				dect_l2_shell_print(
						    "  Free subslot count:      %d",
						    entry->free_subslot_cnt);
				dect_l2_shell_print(
						    "  Possible subslot count:  %d",
						    entry->possible_subslot_cnt);
				dect_l2_shell_print(
						    "  Busy subslot count:      %d",
						    entry->busy_subslot_cnt);
				dect_l2_shell_print(
						    "  Scan suitable percent:   %d%%",
						    entry->scan_suitable_percent);
				dect_l2_shell_print(
						    "  Frame subslot verdicts: "
						    "(Free=F, Possible=P, Busy=B, Unknown=U)");
				for (int i = 0; i < sizeof(entry->frame_subslot_verdicts); i++) {
					tmp_ptr =
						dect_shell_util_rssi_scan_result_verdict_to_string(
							entry->frame_subslot_verdicts[i], tmp_ptr,
							sizeof(verdict_str));
				}
				dect_l2_shell_print("    %s", verdict_str);
			}
		}
		break;
	}
	case NET_EVENT_DECT_NETWORK_STATUS: {
		dect_l2_shell_print("NET_EVENT_DECT_NETWORK_STATUS:");
		const struct dect_network_status_evt *evt =
			(const struct dect_network_status_evt *)cb->info;

		switch (evt->network_status) {
		case DECT_NETWORK_STATUS_FAILURE:
			if (evt->dect_err_cause == DECT_STATUS_OS_ERROR) {
				dect_l2_shell_warn(
						   "  Network status: failure (OS error %d)",
						   evt->os_err_cause);
			} else {
				dect_net_l2_shell_util_mac_err_to_string(evt->dect_err_cause,
									 err_str, sizeof(err_str));

				dect_l2_shell_warn(
						   "  Network status: failure %d (%s)",
						   evt->dect_err_cause, err_str);
			}
			break;
		case DECT_NETWORK_STATUS_CREATED:
			dect_l2_shell_print("  Network status: created");
			break;
		case DECT_NETWORK_STATUS_REMOVED:
			dect_l2_shell_print("  Network status: removed");
			break;
		case DECT_NETWORK_STATUS_JOINED:
			dect_l2_shell_print("  Network status: joined");
			break;
		case DECT_NETWORK_STATUS_UNJOINED:
			dect_l2_shell_print("  Network status: unjoined");
			break;
		default:
			dect_l2_shell_error("  Unknown network status: %d",
					    evt->network_status);
			break;
		}
		break;
	}
	case NET_EVENT_DECT_NEIGHBOR_LIST: {
		const struct dect_neighbor_list_evt *resp_data =
			(const struct dect_neighbor_list_evt *)cb->info;

		dect_l2_shell_print("NET_EVENT_DECT_NEIGHBOR_LIST");
		if (resp_data->status != DECT_STATUS_OK) {
			dect_net_l2_shell_util_mac_err_to_string(resp_data->status, err_str,
								 sizeof(err_str));

			dect_l2_shell_warn("Neighbor list request failed: %d (%s)",
					   resp_data->status, err_str);
		} else {
			if (resp_data->neighbor_count == 0) {
				dect_l2_shell_print("  No neighbors found.");
			} else {
				dect_l2_shell_print("  Neighbor count: %d",
						    resp_data->neighbor_count);
				dect_l2_shell_print("  Neighbor list:");
				for (int i = 0; i < resp_data->neighbor_count; i++) {
					dect_l2_shell_print(
							    "    Neighbor long RD ID %u",
							    resp_data->neighbor_long_rd_ids[i]);
				}
			}
		}
		break;
	}
	case NET_EVENT_DECT_NEIGHBOR_INFO: {
		const struct dect_neighbor_info_evt *evt_data =
			(const struct dect_neighbor_info_evt *)cb->info;

		dect_l2_shell_print("NET_EVENT_DECT_NEIGHBOR_INFO");
		if (evt_data->status != DECT_STATUS_OK) {
			dect_net_l2_shell_util_mac_err_to_string(evt_data->status, err_str,
								 sizeof(err_str));
			dect_l2_shell_warn("Neighbor info request failed: %d (%s)",
					   evt_data->status, err_str);
		} else {
			dect_l2_shell_print("Neighbor status information:");
			dect_l2_shell_print(
				"  Network ID.....................................%u (0x%08x)",
				evt_data->network_id, evt_data->network_id);
			dect_l2_shell_print(
				"  Neighbor (long RD ID)..........................%u (0x%08x)",
				evt_data->long_rd_id, evt_data->long_rd_id);
			dect_l2_shell_print(
					    "  Associated.....................................%s",
					    evt_data->associated ? "true" : "false");
			dect_l2_shell_print(
					    "  FT mode........................................%s",
					    evt_data->ft_mode ? "true" : "false");
			dect_l2_shell_print(
					    "  Channel........................................%d",
					    evt_data->channel);
			dect_l2_shell_print(
				"  Time in ms since neighbor is last seen.........%u ms",
				evt_data->time_since_last_rx_ms);
			dect_l2_shell_print(
					    "  Last RX mcs....................................%u",
					    evt_data->last_rx_signal_info.mcs);
			dect_l2_shell_print(
				"  Last RX transmit power.........................%d (%d dBm)",
				evt_data->last_rx_signal_info.transmit_power,
				dect_utils_lib_phy_tx_power_to_dbm(
					evt_data->last_rx_signal_info.transmit_power));
			dect_l2_shell_print(
					    "  Last RX RSSI 2.................................%d",
					    evt_data->last_rx_signal_info.rssi_2);
			dect_l2_shell_print(
					    "  Last RX SNR....................................%d",
					    evt_data->last_rx_signal_info.snr);
			dect_l2_shell_print(
					    "  beacon_average_rx_rssi_2.......................%d",
					    evt_data->beacon_average_rx_rssi_2);
			dect_l2_shell_print(
					    "  beacon_average_rx_snr..........................%d",
					    evt_data->beacon_average_rx_snr);
			dect_l2_shell_print(
					    "  total_missed_cluster_beacons...................%u",
					    evt_data->status_info.total_missed_cluster_beacons);
			dect_l2_shell_print(
				"  current_consecutive_missed_cluster_beacons.....%u",
				evt_data->status_info.current_consecutive_missed_cluster_beacons);
			dect_l2_shell_print(
					    "  num_rx_paging..................................%u",
					    evt_data->status_info.num_rx_paging);
			dect_l2_shell_print(
					    "  average_rx_mcs.................................%u",
					    evt_data->status_info.average_rx_mcs);
			dect_l2_shell_print(
					    "  average_rx_txpower.............................%d",
					    evt_data->status_info.average_rx_txpower);
			dect_l2_shell_print(
					    "  average_rx_rssi_2..............................%d",
					    evt_data->status_info.average_rx_rssi_2);
			dect_l2_shell_print(
					    "  average_rx_snr.................................%d",
					    evt_data->status_info.average_rx_snr);
			dect_l2_shell_print(
					    "  average_tx_mcs.................................%u",
					    evt_data->status_info.average_tx_mcs);
			dect_l2_shell_print(
					    "  average_tx_txpower.............................%d",
					    evt_data->status_info.average_tx_txpower);
			dect_l2_shell_print(
					    "  num_tx_attempts................................%u",
					    evt_data->status_info.num_tx_attempts);
			dect_l2_shell_print(
					    "  num_lbt_failures...............................%u",
					    evt_data->status_info.num_lbt_failures);
			dect_l2_shell_print(
					    "  num_rx_pdc.....................................%u",
					    evt_data->status_info.num_rx_pdc);
			dect_l2_shell_print(
					    "  num_rx_pdc_crc_fail............................%u",
					    evt_data->status_info.num_rx_pdc_crc_failures);
			dect_l2_shell_print(
					    "  num_no_response................................%u",
					    evt_data->status_info.num_no_response);
			dect_l2_shell_print(
					    "  num_harq_ack...................................%u",
					    evt_data->status_info.num_harq_ack);
			dect_l2_shell_print(
					    "  num_harq_nack..................................%u",
					    evt_data->status_info.num_harq_nack);
			dect_l2_shell_print(
					    "  num_arq_retx...................................%u",
					    evt_data->status_info.num_arq_retx);
			dect_l2_shell_print(
				"  inactive_time_ms...............................%u ms",
				evt_data->status_info.inactive_time_ms);
		}
		break;
	}
	case NET_EVENT_DECT_NW_BEACON_START_RESULT: {
		const struct dect_common_resp_evt *resp_data =
			(const struct dect_common_resp_evt *)cb->info;

		dect_l2_shell_print("NET_EVENT_DECT_NW_BEACON_START_RESULT");
		if (resp_data->status != DECT_STATUS_OK) {
			dect_net_l2_shell_util_mac_err_to_string(resp_data->status, err_str,
								 sizeof(err_str));
			dect_l2_shell_warn("NW beacon start failed: %d (%s)",
					   resp_data->status, err_str);
		} else {
			dect_l2_shell_print("NW beacon started.");
		}
		break;
	}
	case NET_EVENT_DECT_NW_BEACON_STOP_RESULT: {
		const struct dect_common_resp_evt *resp_data =
			(const struct dect_common_resp_evt *)cb->info;

		dect_l2_shell_print("NET_EVENT_DECT_NW_BEACON_STOP_RESULT");
		if (resp_data->status != DECT_STATUS_OK) {
			dect_net_l2_shell_util_mac_err_to_string(resp_data->status, err_str,
								 sizeof(err_str));
			dect_l2_shell_warn("NW beacon stop failed: %d (%s)",
					   resp_data->status, err_str);
		} else {
			dect_l2_shell_print("NW beacon stopped.");
		}
		break;
	}
	case NET_EVENT_DECT_SINK_STATUS: {
		const struct dect_sink_status_evt *evt_data =
			(const struct dect_sink_status_evt *)cb->info;

		dect_l2_shell_print("NET_EVENT_DECT_SINK_STATUS");
		if (evt_data->sink_status == DECT_SINK_STATUS_CONNECTED) {
			dect_l2_shell_print(
					    "  Sink is connected, iface towards Internet %p",
					    evt_data->br_iface);
		} else {
			dect_l2_shell_print(
					    "  Sink is disconnected, iface towards Internet %p",
					    evt_data->br_iface);
		}
		break;
	}
	default:
		dect_l2_shell_error("%s: unknown event: %llu", (__func__),
				    mgmt_event);
		break;
	}
}

static void dect_shell_rssi_scan_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct dect_rssi_scan_params params;
	int long_index = 0;
	int opt;
	int ret;

	set_shell(shell); /* Store shell from command for use as default */
	if (argc < 2) {
		goto show_usage;
	}
	sys_getopt_init();

	params.band = 0;
	params.channel_count = 0;
	params.frame_count_to_scan = 2010 / 10; /* 2010 ms / 10 ms = 201 frames */

	while ((opt = sys_getopt_long(argc, argv, "c:b:h", long_options_rssi_scan,
				      &long_index)) != -1) {
		switch (opt) {
		case DECT_SHELL_RSSI_SCAN_CMD_FRAMES: {
			params.frame_count_to_scan = atoi(sys_getopt_optarg);
			if (params.frame_count_to_scan > UINT8_MAX) {
				dect_l2_shell_error(
						    "Invalid number of frames: %d (max %d)",
						    params.frame_count_to_scan, UINT8_MAX);
				goto show_usage;
			}
			break;
		}
		case 'b': {
			params.band = atoi(sys_getopt_optarg);
			break;
		}
		case 'c': {
			char *ch_string = strtok(sys_getopt_optarg, ",");

			if (ch_string == NULL) {
				params.channel_list[0] = atoi(sys_getopt_optarg);
				params.channel_count = 1;
			} else {
				while (ch_string != NULL && params.channel_count < 20) {
					params.channel_list[params.channel_count++] =
						atoi(ch_string);
					ch_string = strtok(NULL, ",");
				}
			}
			break;
		}
		case 'h':
			goto show_usage;
		case '?':
		default:
			dect_l2_shell_error("Unknown option (%s). See usage:",
					    argv[sys_getopt_optind - 1]);
			goto show_usage;
		}
	}

	if (sys_getopt_optind < argc) {
		dect_l2_shell_error("Arguments without '-' not supported: %s",
				    argv[argc - 1]);
		goto show_usage;
	}

	if ((params.band == 0) && (params.channel_count == 0)) {
		dect_l2_shell_error(
				    "Either channels or band must be given. See usage:");
		goto show_usage;
	}

	if (params.band != 0) {
		params.channel_count = 0;
	}
	ret = net_mgmt(NET_REQUEST_DECT_RSSI_SCAN, context.iface, &params, sizeof(params));
	if (ret) {
		dect_l2_shell_error("RSSI scan request failed");
		return;
	}

	dect_l2_shell_print("RSSI scan initiated.");

	return;

show_usage:
	dect_l2_shell_print("%s", dect_shell_rssi_scan_usage_str);
}

static void dect_shell_scan_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct dect_scan_params params;
	int long_index = 0;
	int ret, opt, tmp_value;

	set_shell(shell); /* Store shell from command for use as default */
	if (argc < 2) {
		goto show_usage;
	}

	sys_getopt_init();

	params.band = 0;
	params.channel_count = 0;
	params.channel_scan_time_ms = 3000;

	while ((opt = sys_getopt_long(argc, argv, "t:c:b:h", long_options_nw_scan,
				      &long_index)) != -1) {
		switch (opt) {
		case 't': {
			tmp_value = atoi(sys_getopt_optarg);
			if (tmp_value < 1 || tmp_value > 60000) {
				dect_l2_shell_error(
					"Invalid scan time: %d ms (valid range: [1, 60000])",
					tmp_value);
				goto show_usage;
			}
			params.channel_scan_time_ms = tmp_value;
			break;
		}
		case 'b': {
			params.band = atoi(sys_getopt_optarg);
			break;
		}
		case 'c': {
			char *ch_string = strtok(sys_getopt_optarg, ",");

			if (ch_string == NULL) {
				params.channel_list[0] = atoi(sys_getopt_optarg);
				params.channel_count = 1;
			} else {
				while (ch_string != NULL) {
					if (params.channel_count >= 4) {
						dect_l2_shell_error(
							"Maximum of 4 channels supported.");
						goto show_usage;
					}
					params.channel_list[params.channel_count++] =
						atoi(ch_string);
					ch_string = strtok(NULL, ",");
				}
			}
			break;
		}
		case 'h':
			goto show_usage;
		case '?':
		default:
			dect_l2_shell_error("Unknown option (%s). See usage:",
					    argv[sys_getopt_optind - 1]);
			goto show_usage;
		}
	}

	if (sys_getopt_optind < argc) {
		dect_l2_shell_error("Arguments without '-' not supported: %s",
				    argv[argc - 1]);
		goto show_usage;
	}

	if ((params.band == 0) && (params.channel_count == 0)) {
		dect_l2_shell_error(
				    "Either channels or band must be given. See usage:");
		goto show_usage;
	}

	if (params.band != 0) {
		params.channel_count = 0;
	}
	ret = net_mgmt(NET_REQUEST_DECT_SCAN, context.iface, &params, sizeof(params));
	if (ret) {
		dect_l2_shell_error("Scan request failed");
		return;
	}

	dect_l2_shell_print("Scan initiated.");

	return;

show_usage:
	dect_l2_shell_print("%s", dect_shell_scan_usage_str);
}
static int dect_shell_tx_rx_cmd_setup_socket(int *sockfd, struct sockaddr_ll *sa)
{
	int ret;

	/* Using SOCK_DGRAM instead of SOCK_RAW because dst address is
	 * passed down in a stack (in zephyr net_context.c)
	 */
	*sockfd = zsock_socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL));
	if (*sockfd < 0) {
		dect_l2_shell_error("Unable to create a socket %d (%s)", errno,
				    strerror(errno));
		return -1;
	}

	sa->sll_family = AF_PACKET;
	sa->sll_ifindex = net_if_get_by_iface(context.iface);

	/* Bind the socket */
	ret = zsock_bind(*sockfd, (struct sockaddr *)sa, sizeof(struct sockaddr_ll));
	if (ret < 0) {
		dect_l2_shell_error(
				    "Error: Unable to bind socket to the network interface:%d",
				    errno);
		zsock_close(*sockfd);
		return -1;
	}

	return 0;
}

static const char dect_shell_tx_cmd_usage_str[] =
	"Usage: dect tx <options>\n\n"
	"Options:\n"
	"  -t  --target <int>          Target long RD ID.\n"
	"  -d  --data <string>,        Data string to send.\n";

/* Specifying the expected options (both long and short): */
static struct sys_getopt_option long_options_tx[] = {
	{"target", sys_getopt_required_argument, 0, 't'},
	{"data", sys_getopt_required_argument, 0, 'd'},
	{0, 0, 0, 0}
};

#define DECT_TX_CMD_MAX_SEND_DATA_LEN DECT_MTU

static void dect_shell_tx_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct sockaddr_ll dst = {0};
	int long_index = 0;
	int opt;
	int sockfd, ret;
	char tx_data_buf[DECT_TX_CMD_MAX_SEND_DATA_LEN];
	int tx_data_len;
	uint32_t target_long_rd_id = 0;

	set_shell(shell); /* Store shell from command for use as default */
	if (argc < 2) {
		goto show_usage;
	}

	sys_getopt_init();

	memset(tx_data_buf, 0, sizeof(tx_data_buf));

	while ((opt = sys_getopt_long(argc, argv, "d:t:h", long_options_tx, &long_index)) != -1) {
		switch (opt) {
		case 'd':
			tx_data_len = strlen(sys_getopt_optarg) + 1;
			if (tx_data_len > DECT_TX_CMD_MAX_SEND_DATA_LEN) {
				dect_l2_shell_error(
						    "Data length (%d) exceeded the maximum (%d). "
						    "Given data: %s",
						    tx_data_len, DECT_TX_CMD_MAX_SEND_DATA_LEN,
						    sys_getopt_optarg);
				return;
			}
			strcpy(tx_data_buf, sys_getopt_optarg);
			break;
		case 't': {
			target_long_rd_id = (uint32_t)atoll(sys_getopt_optarg);
			break;
		}
		case 'h':
			goto show_usage;
		case '?':
		default:
			dect_l2_shell_error("Unknown option (%s). See usage:",
					    argv[sys_getopt_optind - 1]);
			goto show_usage;
		}
	}

	if (sys_getopt_optind < argc) {
		dect_l2_shell_error("Arguments without '-' not supported: %s",
				    argv[argc - 1]);
		goto show_usage;
	}

	if (target_long_rd_id == 0) {
		dect_l2_shell_error(
				    "Target long RD ID needs to be given. See usage:");
		goto show_usage;
	}

	ret = dect_shell_tx_rx_cmd_setup_socket(&sockfd, &dst);
	if (ret < 0) {
		dect_l2_shell_error(
				    "Setting socket for raw pkt transmission failed %d", ret);
		return;
	}

	target_long_rd_id = htonl(target_long_rd_id);
	memcpy(&dst.sll_addr, &target_long_rd_id, sizeof(target_long_rd_id));
	dst.sll_halen = sizeof(uint32_t);

	ret = zsock_sendto(sockfd, tx_data_buf, tx_data_len, 0, (const struct sockaddr *)&dst,
		     sizeof(struct sockaddr_ll));
	if (ret < 0) {
		dect_l2_shell_error("Unable to send data: %s", strerror(errno));
	} else {
		dect_l2_shell_print("Send initiated.");
	}
	zsock_close(sockfd);

	return;

show_usage:
	dect_l2_shell_print("%s", dect_shell_tx_cmd_usage_str);
}

static void dect_shell_rx_thread_handler(void)
{
	int recv_len, ret;
	struct zsock_pollfd fds[1];
	struct sockaddr_ll src;
	socklen_t fromlen;

	fromlen = sizeof(src);
	while (true) {
		if (dect_shell_rx_sockfd < 0) {
			/* Wait for sockets to be created */
			k_sem_take(&dect_shell_rx_thread_sem, K_FOREVER);
			continue;
		}

		fds[0].fd = dect_shell_rx_sockfd;
		fds[0].events = ZSOCK_POLLIN;
		fds[0].revents = 0;

		ret = zsock_poll(fds, 1, DATA_RX_POLL_TIMEOUT_MS);
		if (ret < 0) {
			printk("Error: poll failed %d\n", errno);
			continue;
		} else if (ret == 0) {
			continue;
		}

		recv_len = zsock_recvfrom(dect_shell_rx_sockfd, rx_data_buf, DECT_MTU, 0,
				    (struct sockaddr *)&src, &fromlen);
		if (recv_len < 0) {
			printk("Error: Unable to receive data from the network interface: %d\n",
			       errno);
			break;
		}
		rx_data_buf[recv_len] = '\0';

		/* Get source from ll addr */
		uint32_t src_long_rd_id = 0;

		src_long_rd_id = ntohl(*(uint32_t *)&src.sll_addr);

		dect_l2_shell_print(
				    "Received data (len %d, src long RD ID %u):", recv_len,
				    src_long_rd_id);
		dect_l2_shell_print("  %s", rx_data_buf);
	}
}

K_THREAD_DEFINE(dect_shell_rx_thread, DECT_SHELL_RX_THREAD_STACK_SIZE, dect_shell_rx_thread_handler,
		NULL, NULL, NULL, DECT_SHELL_RX_THREAD_PRIORITY, 0, 0);

void dect_shell_rx_stop(void)
{
	if (dect_shell_rx_sockfd >= 0) {
		zsock_close(dect_shell_rx_sockfd);
		dect_shell_rx_sockfd = -1;
	}
	k_sem_reset(&dect_shell_rx_thread_sem);
	dect_l2_shell_print("RX stopped");
}

void dect_shell_rx_start(void)
{
	struct sockaddr_ll dst = {0};
	int ret;

	if (dect_shell_rx_sockfd >= 0) {
		dect_l2_shell_error("RX already started");
		return;
	}

	ret = dect_shell_tx_rx_cmd_setup_socket(&dect_shell_rx_sockfd, &dst);
	if (ret < 0) {
		dect_l2_shell_error("Setting socket for raw pkt rcv failed %d",
				    ret);
		return;
	}
	dect_l2_shell_print("RX started");
	k_sem_give(&dect_shell_rx_thread_sem);
}

static void dect_shell_rx_cmd(const struct shell *shell, size_t argc, char **argv)
{
	set_shell(shell); /* Store shell from command for use as default */
	if (argv[1] != NULL && !strcmp(argv[1], "stop")) {
		dect_shell_rx_stop();
	} else if (argv[1] != NULL && !strcmp(argv[1], "start")) {
		dect_shell_rx_start();
	} else {
		dect_l2_shell_print("%s", dect_shell_rx_cmd_usage_str);
	}
}

static int dect_shell_associate_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct dect_associate_req_params params;
	int long_index = 0;
	int opt;
	int ret;

	set_shell(shell); /* Store shell from command for use as default */
	if (argc < 2) {
		goto show_usage;
	}
	sys_getopt_init();

	/* Set defaults */
	params.target_long_rd_id = 0;
	while ((opt = sys_getopt_long(argc, argv, "t:h", long_options_associate,
				      &long_index)) != -1) {
		switch (opt) {
		case 't': {
			params.target_long_rd_id = (uint32_t)atoll(sys_getopt_optarg);
			break;
		}
		case 'h':
			goto show_usage;
		case '?':
		default:
			dect_l2_shell_error("Unknown option (%s). See usage:",
					    argv[sys_getopt_optind - 1]);
			goto show_usage;
		}
	}

	if (sys_getopt_optind < argc) {
		dect_l2_shell_error("Arguments without '-' not supported: %s",
				    argv[argc - 1]);
		goto show_usage;
	}

	if (!params.target_long_rd_id) {
		dect_l2_shell_error(
				    "Target long RD ID needs to be given. See usage:");
		goto show_usage;
	}

	ret = net_mgmt(NET_REQUEST_DECT_ASSOCIATION, context.iface, &params, sizeof(params));
	if (ret) {
		dect_l2_shell_error("Association request failed: %d", ret);
		return -1;
	}

	return 0;

show_usage:
	dect_l2_shell_print("%s", dect_shell_sett_associate_usage_str);
	return 0;
}

static int dect_shell_dissociate_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct dect_associate_rel_params params;
	int long_index = 0;
	int opt;
	int ret;

	set_shell(shell); /* Store shell from command for use as default */
	if (argc < 2) {
		goto show_usage;
	}
	sys_getopt_init();

	/* Set defaults */
	params.target_long_rd_id = 0;
	while ((opt = sys_getopt_long(argc, argv, "t:h", long_options_dissociate,
				      &long_index)) != -1) {
		switch (opt) {
		case 't': {
			params.target_long_rd_id = (uint32_t)atoll(sys_getopt_optarg);
			break;
		}
		case 'h':
			goto show_usage;
		case '?':
		default:
			dect_l2_shell_error("Unknown option (%s). See usage:",
					    argv[sys_getopt_optind - 1]);
			goto show_usage;
		}
	}

	if (sys_getopt_optind < argc) {
		dect_l2_shell_error("Arguments without '-' not supported: %s",
				    argv[argc - 1]);
		goto show_usage;
	}

	if (!params.target_long_rd_id) {
		dect_l2_shell_error(
				    "Target long RD ID needs to be given. See usage:");
		goto show_usage;
	}

	ret = net_mgmt(NET_REQUEST_DECT_ASSOCIATION_RELEASE, context.iface, &params,
		       sizeof(params));
	if (ret) {
		dect_l2_shell_error("Association Release request failed: %d", ret);
		return -1;
	}

	return 0;

show_usage:
	dect_l2_shell_print("%s", dect_shell_sett_dissociate_usage_str);
	return 0;
}

static int dect_common_utils_settings_ms_to_mac_cluster_beacon_period_in_ms(int period_in_msecs)
{
	switch (period_in_msecs) {
	case 10:
		return DECT_CLUSTER_BEACON_PERIOD_10MS;
	case 50:
		return DECT_CLUSTER_BEACON_PERIOD_50MS;
	case 100:
		return DECT_CLUSTER_BEACON_PERIOD_100MS;
	case 500:
		return DECT_CLUSTER_BEACON_PERIOD_500MS;
	case 1000:
		return DECT_CLUSTER_BEACON_PERIOD_1000MS;
	case 1500:
		return DECT_CLUSTER_BEACON_PERIOD_1500MS;
	case 2000:
		return DECT_CLUSTER_BEACON_PERIOD_2000MS;
	case 4000:
		return DECT_CLUSTER_BEACON_PERIOD_4000MS;
	case 8000:
		return DECT_CLUSTER_BEACON_PERIOD_8000MS;
	case 16000:
		return DECT_CLUSTER_BEACON_PERIOD_16000MS;
	case 32000:
		return DECT_CLUSTER_BEACON_PERIOD_32000MS;
	default:
		return -1;
	}
}

static int dect_common_utils_settings_ms_to_mac_nw_beacon_period_in_ms(int period_in_msecs)
{
	switch (period_in_msecs) {
	case 50:
		return DECT_NW_BEACON_PERIOD_50MS;
	case 100:
		return DECT_NW_BEACON_PERIOD_100MS;
	case 500:
		return DECT_NW_BEACON_PERIOD_500MS;
	case 1000:
		return DECT_NW_BEACON_PERIOD_1000MS;
	case 1500:
		return DECT_NW_BEACON_PERIOD_1500MS;
	case 2000:
		return DECT_NW_BEACON_PERIOD_2000MS;
	case 4000:
		return DECT_NW_BEACON_PERIOD_4000MS;
	default:
		return -1;
	}
}

static char *dect_common_utils_settings_region_to_string(enum dect_settings_region region)
{
	switch (region) {
	case DECT_SETTINGS_REGION_EU:
		return "eu";
	case DECT_SETTINGS_REGION_US:
		return "us";
	case DECT_SETTINGS_REGION_GLOBAL:
		return "global";
	default:
		return "Unknown";
	}
}

static char *
dect_common_utils_settings_write_scope_to_string(enum dect_settings_cmd_params_write_scope scope)
{
	switch (scope) {
	case DECT_SETTINGS_WRITE_SCOPE_ALL:
		return "all";
	case DECT_SETTINGS_WRITE_SCOPE_AUTO_START:
		return "auto_start";
	case DECT_SETTINGS_WRITE_SCOPE_REGION:
		return "region";
	case DECT_SETTINGS_WRITE_SCOPE_DEVICE_TYPE:
		return "device_type";
	case DECT_SETTINGS_WRITE_SCOPE_IDENTITIES:
		return "identities";
	case DECT_SETTINGS_WRITE_SCOPE_TX:
		return "tx";
	case DECT_SETTINGS_WRITE_SCOPE_POWER_SAVE:
		return "power_save";
	case DECT_SETTINGS_WRITE_SCOPE_BAND_NBR:
		return "band_nbr";
	case DECT_SETTINGS_WRITE_SCOPE_RSSI_SCAN:
		return "rssi_scan";
	case DECT_SETTINGS_WRITE_SCOPE_CLUSTER:
		return "cluster";
	case DECT_SETTINGS_WRITE_SCOPE_NW_BEACON:
		return "network_beacon";
	case DECT_SETTINGS_WRITE_SCOPE_ASSOCIATION:
		return "association";
	case DECT_SETTINGS_WRITE_SCOPE_NETWORK_JOIN:
		return "network_join";
	case DECT_SETTINGS_WRITE_SCOPE_SECURITY_CONFIGURATION:
		return "security_configuration";
	default:
		return "Unknown";
	}
}

static int
dect_common_utils_settings_mac_pdu_nw_beacon_period_in_ms(enum dect_nw_beacon_period period)
{
	switch (period) {
	case DECT_NW_BEACON_PERIOD_50MS:
		return 50;
	case DECT_NW_BEACON_PERIOD_100MS:
		return 100;
	case DECT_NW_BEACON_PERIOD_500MS:
		return 500;
	case DECT_NW_BEACON_PERIOD_1000MS:
		return 1000;
	case DECT_NW_BEACON_PERIOD_1500MS:
		return 1500;
	case DECT_NW_BEACON_PERIOD_2000MS:
		return 2000;
	case DECT_NW_BEACON_PERIOD_4000MS:
		return 4000;
	default:
		return -1;
	}
}

static int dect_common_utils_settings_mac_pdu_cluster_beacon_period_in_ms(
	enum dect_cluster_beacon_period period)
{
	switch (period) {
	case DECT_CLUSTER_BEACON_PERIOD_10MS:
		return 10;
	case DECT_CLUSTER_BEACON_PERIOD_50MS:
		return 50;
	case DECT_CLUSTER_BEACON_PERIOD_100MS:
		return 100;
	case DECT_CLUSTER_BEACON_PERIOD_500MS:
		return 500;
	case DECT_CLUSTER_BEACON_PERIOD_1000MS:
		return 1000;
	case DECT_CLUSTER_BEACON_PERIOD_1500MS:
		return 1500;
	case DECT_CLUSTER_BEACON_PERIOD_2000MS:
		return 2000;
	case DECT_CLUSTER_BEACON_PERIOD_4000MS:
		return 4000;
	case DECT_CLUSTER_BEACON_PERIOD_8000MS:
		return 8000;
	case DECT_CLUSTER_BEACON_PERIOD_16000MS:
		return 16000;
	case DECT_CLUSTER_BEACON_PERIOD_32000MS:
		return 32000;
	default:
		return -1;
	}
}

static void dect_shell_sett_cmd_print(struct dect_settings *dect_sett)
{
	dect_l2_shell_print("DECT settings:");
	dect_l2_shell_print("  Network ID:                           %u (0x%08x)",
			    dect_sett->identities.network_id, dect_sett->identities.network_id);
	dect_l2_shell_print("  Transmitter Long RD ID:               %u (0x%08x)",
			    dect_sett->identities.transmitter_long_rd_id,
			    dect_sett->identities.transmitter_long_rd_id);
	dect_l2_shell_print("  Region/variant:                       %s",
			    dect_common_utils_settings_region_to_string(dect_sett->region));
	dect_l2_shell_print("  Band:                                 %d",
			    dect_sett->band_nbr);
	dect_l2_shell_print("  Auto activate:                        %s",
			    dect_sett->auto_start.activate ? "on" : "off");
	dect_l2_shell_print("  Device type:                          %s",
			    (dect_sett->device_type & DECT_DEVICE_TYPE_FT) ? "FT" : "PT");
	dect_l2_shell_print("  Max TX power:                         %d dBm",
			    dect_sett->tx.max_power_dbm);
	dect_l2_shell_print("  Max MCS:                              %d",
			    dect_sett->tx.max_mcs);
	dect_l2_shell_print("  Power save:                           %s",
			    dect_sett->power_save ? "on" : "off");

	dect_l2_shell_print("Common RSSI scanning settings:");
	dect_l2_shell_print("  ch access: RSSI scan (msecs) per ch:  %d",
			    dect_sett->rssi_scan.time_per_channel_ms);
	dect_l2_shell_print(
			    "  ch access: RSSI scan busy (dBm):      signal level > %d",
			    dect_sett->rssi_scan.busy_threshold_dbm);
	dect_l2_shell_print(
		"  ch access: RSSI scan possible (dBm):  %d >= signal level > %d",
		dect_sett->rssi_scan.busy_threshold_dbm, dect_sett->rssi_scan.free_threshold_dbm);
	dect_l2_shell_print(
			    "  ch access: RSSI scan free (dBm):      signal level <= %d",
			    dect_sett->rssi_scan.free_threshold_dbm);
	dect_l2_shell_print("  ch access: SCAN_SUITABLE%%             %d%%",
			    dect_sett->rssi_scan.scan_suitable_percent);
	dect_l2_shell_print("  Association related:");
	dect_l2_shell_print("   Max cluster beacon RX fails:         %d",
			    dect_sett->association.max_beacon_rx_failures);
	dect_l2_shell_print("  Cluster beacon:");
	dect_l2_shell_print("   Period:                              %d ms",
			    dect_common_utils_settings_mac_pdu_cluster_beacon_period_in_ms(
				    dect_sett->cluster.beacon_period));
	dect_l2_shell_print("   Max beacon TX power:                 %d dBm",
			    dect_sett->cluster.max_beacon_tx_power_dbm);
	dect_l2_shell_print("   Max cluster TX power:                %d dBm",
			    dect_sett->cluster.max_cluster_power_dbm);
	dect_l2_shell_print("   Max num of neighbors:                %d",
			    dect_sett->cluster.max_num_neighbors);
	if (dect_sett->cluster.channel_loaded_percent) {
		dect_l2_shell_print(
				    "   Channel reselection:                 enabled");
		dect_l2_shell_print("   Channel reselection threshold:       %d%%",
				    dect_sett->cluster.channel_loaded_percent);
	} else {
		dect_l2_shell_print(
				    "   Channel reselection:                 disabled");
	}
	if (dect_sett->cluster.neighbor_inactivity_disconnect_timer_ms) {
		dect_l2_shell_print(
				    "   Neighbor inactivity time:            %d ms",
				    dect_sett->cluster.neighbor_inactivity_disconnect_timer_ms);
	} else {
		dect_l2_shell_print(
				    "   Neighbor inactivity time:            timer disabled");
	}
	dect_l2_shell_print("  Network beacon:");
	dect_l2_shell_print("   Period:                              %d ms",
			    dect_common_utils_settings_mac_pdu_nw_beacon_period_in_ms(
				    dect_sett->nw_beacon.beacon_period));
	if (dect_sett->nw_beacon.channel != DECT_NW_BEACON_CHANNEL_NOT_USED) {
		dect_l2_shell_print("   Channel:                             %d",
				    dect_sett->nw_beacon.channel);
	}
	dect_l2_shell_print("  Network join:");
	if (dect_sett->network_join.target_ft_long_rd_id == DECT_SETT_NETWORK_JOIN_TARGET_FT_ANY) {
		dect_l2_shell_print(
				    "   Target FT:                           Any FT");
	} else {
		dect_l2_shell_print(
				    "   Target FT:                           %u (0x%08x)",
				    dect_sett->network_join.target_ft_long_rd_id,
				    dect_sett->network_join.target_ft_long_rd_id);
	}
	dect_l2_shell_print("   MIN_SENSITIVITY_LEVEL:               %d dBm",
			    dect_sett->association.min_sensitivity_dbm);

	dect_l2_shell_print("  Security configuration:");
	dect_l2_shell_print("   Security mode:                       %s",
			    dect_sett->sec_conf.mode == DECT_SECURITY_MODE_NONE ? "none"
										    : "mode_1");
	if (dect_sett->sec_conf.mode == DECT_SECURITY_MODE_1) {
		int sec_key_len = DECT_INTEGRITY_KEY_LENGTH * 2 + 1;
		char sec_key_str[sec_key_len];
		int err;

		/* Print security keys as hex strings */
		err = dect_shell_util_htoa(dect_sett->sec_conf.integrity_key,
					   DECT_INTEGRITY_KEY_LENGTH, sec_key_str, sec_key_len);
		if (err < 0) {
			dect_l2_shell_error(
					    "Error converting integrity key to hex string: %d",
					    err);
		} else {
			dect_l2_shell_print(
					    "   Security integrity key:              %s",
					    sec_key_str);
		}
		err = dect_shell_util_htoa(dect_sett->sec_conf.cipher_key,
					   DECT_CIPHER_KEY_LENGTH, sec_key_str, sec_key_len);
		if (err < 0) {
			dect_l2_shell_error(
					    "Error converting cipher key to hex string: %d", err);
		} else {
			dect_l2_shell_print(
					    "   Security cipher key:                 %s",
					    sec_key_str);
		}
	}
}

static void dect_shell_sett_cmd(const struct shell *shell, size_t argc, char **argv)
{
	struct dect_settings current_settings;
	struct dect_settings newsettings;

	int long_index = 0;
	int opt;
	unsigned long long tmp_value;
	int ret = 0;

	set_shell(shell); /* Store shell from command for use as default */
	if (argc < 2) {
		goto show_usage;
	}

	sys_getopt_init();

	/* We need to read current settings first because we do not mandate that user must give
	 * all of the settings in scope
	 */
	ret = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, context.iface, &current_settings,
		       sizeof(current_settings));
	if (ret) {
		dect_l2_shell_error("Cannot read current settings: %d", ret);
		return;
	}
	newsettings = current_settings;
	newsettings.cmd_params.reset_to_driver_defaults = false;
	newsettings.cmd_params.write_scope_bitmap = 0;

	while ((opt = sys_getopt_long(argc, argv, "n:t:b:rh", long_options_sett_cmd,
				      &long_index)) != -1) {
		switch (opt) {
		case 'r': {
			dect_shell_sett_cmd_print(&current_settings);
			return;
		}
		case DECT_SHELL_SETT_CMD_RESET_ALL: {
			newsettings.cmd_params.reset_to_driver_defaults = true;
			break;
		}
		case 'n': {
			tmp_value = shell_strtoull(sys_getopt_optarg, 10, &ret);
			if (ret || tmp_value > UINT32_MAX ||
			    !dect_utils_lib_32bit_network_id_validate((uint32_t)tmp_value)) {
				dect_l2_shell_error(
					"%llu (0x%08llx) is not a valid network id.\n"
					"The network ID shall be set to UINT32 value where "
					"neither\n"
					"the 8 LSB bits are 0x00 nor the 24 MSB bits "
					"are 0x000000.",
					tmp_value, tmp_value);
				return;
			}
			newsettings.identities.network_id = tmp_value;
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_IDENTITIES;
			break;
		}
		case 't': {
			tmp_value = shell_strtoull(sys_getopt_optarg, 10, &ret);
			if (ret || (tmp_value < 1 || tmp_value > (UINT32_MAX - 2))) {
				dect_l2_shell_error(
						    "Give decent tx id (range: 1-%lu)",
						    (unsigned long)(UINT32_MAX - 2));
				return;
			}
			newsettings.identities.transmitter_long_rd_id = tmp_value;
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_IDENTITIES;
			break;
		}
		case 'b': {
			tmp_value = atoi(sys_getopt_optarg);
			if (tmp_value == 1 || tmp_value == 2 || tmp_value == 4 || tmp_value == 9 ||
			    tmp_value == 22) {
				newsettings.band_nbr = tmp_value;
				newsettings.cmd_params.write_scope_bitmap |=
					DECT_SETTINGS_WRITE_SCOPE_BAND_NBR;
			} else {
				dect_l2_shell_error("Band #%llu is not supported.",
						    tmp_value);
				return;
			}
			break;
		}
		case DECT_SHELL_SETT_CMD_REGION: {
			if (!strcmp(sys_getopt_optarg, "eu")) {
				newsettings.region = DECT_SETTINGS_REGION_EU;
			} else if (!strcmp(sys_getopt_optarg, "us")) {
				newsettings.region = DECT_SETTINGS_REGION_US;
			} else if (!strcmp(sys_getopt_optarg, "global")) {
				newsettings.region = DECT_SETTINGS_REGION_GLOBAL;
			} else {
				dect_l2_shell_error("Invalid region: %s", sys_getopt_optarg);
				return;
			}
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_REGION;
			break;
		}
		case DECT_SHELL_SETT_CMD_AUTO_ACTIVATE: {
			if (!strcmp(sys_getopt_optarg, "on")) {
				newsettings.auto_start.activate = true;
			} else if (!strcmp(sys_getopt_optarg, "off")) {
				newsettings.auto_start.activate = false;
			} else {
				dect_l2_shell_error("Invalid auto_activate value: %s",
						    sys_getopt_optarg);
				return;
			}
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_AUTO_START;
			break;
		}
		case DECT_SHELL_SETT_CMD_NW_JOIN_TARGET: {
			tmp_value = shell_strtoull(sys_getopt_optarg, 10, &ret);
			if (ret || tmp_value > (UINT32_MAX - 2)) {
				dect_l2_shell_error(
					"Give decent auto start target tx id (range: 0-%lu)",
					(unsigned long)(UINT32_MAX - 2));
				return;
			}
			newsettings.network_join.target_ft_long_rd_id = tmp_value;
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_NETWORK_JOIN;
			break;
		}
		case DECT_SHELL_SETT_CMD_DEV_TYPE: {
			if (!strcmp(sys_getopt_optarg, "FT")) {
				newsettings.device_type = DECT_DEVICE_TYPE_FT;
			} else if (!strcmp(sys_getopt_optarg, "PT")) {
				newsettings.device_type = DECT_DEVICE_TYPE_PT;
			} else {
				dect_l2_shell_error("Invalid device type: %s",
						    sys_getopt_optarg);
				return;
			}
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_DEVICE_TYPE;
			break;
		}
		case DECT_SHELL_SETT_CMD_RSSI_SCAN_TIME_PER_CHANNEL: {
			tmp_value = shell_strtoul(sys_getopt_optarg, 10, &ret);
			if (ret || tmp_value < 10 || (tmp_value > 2550)) {
				dect_l2_shell_error(
						    "Give decent value (range: 10-2550)");
				return;
			}
			newsettings.rssi_scan.time_per_channel_ms = tmp_value;
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_RSSI_SCAN;
			break;
		}
		case DECT_SHELL_SETT_CMD_RSSI_SCAN_FREE_THRESHOLD: {
			long value = shell_strtol(sys_getopt_optarg, 10, &ret);

			if (ret || value >= 0 || value < INT8_MIN) {
				dect_l2_shell_error(
						    "Give decent value (range: %d...-1)", INT8_MIN);
				return;
			}
			newsettings.rssi_scan.free_threshold_dbm = value;
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_RSSI_SCAN;
			break;
		}
		case DECT_SHELL_SETT_CMD_RSSI_SCAN_BUSY_THRESHOLD: {
			long value = shell_strtol(sys_getopt_optarg, 10, &ret);

			if (ret || value >= 0 || value < INT8_MIN) {
				dect_l2_shell_error(
						    "Give decent value (range: %d...-1)", INT8_MIN);
				return;
			}
			newsettings.rssi_scan.busy_threshold_dbm = value;
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_RSSI_SCAN;
			break;
		}
		case DECT_SHELL_SETT_CMD_RSSI_SCAN_SUITABLE_PERCENT: {
			tmp_value = shell_strtoul(sys_getopt_optarg, 10, &ret);
			if (ret || tmp_value > 100) {
				dect_l2_shell_error(
						    "Give decent value (range: 0-100)");
				return;
			}
			newsettings.rssi_scan.scan_suitable_percent = tmp_value;
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_RSSI_SCAN;
			break;
		}

		case DECT_SHELL_SETT_CMD_CLUSTER_BEACON_PERIOD: {
			tmp_value = atoi(sys_getopt_optarg);
			tmp_value =
				dect_common_utils_settings_ms_to_mac_cluster_beacon_period_in_ms(
					tmp_value);
			if (tmp_value < 0) {
				dect_l2_shell_error("Invalid cluster beacon period: %s",
						    sys_getopt_optarg);
				return;
			}
			newsettings.cluster.beacon_period = tmp_value;
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_CLUSTER;
			break;
		}
		case DECT_SHELL_SETT_CMD_CLUSTER_MAX_BEACON_TX_PWR: {
			long value = shell_strtol(sys_getopt_optarg, 10, &ret);

			if (ret || value < -40 || value > 23) {
				dect_l2_shell_error("Invalid cluster beacon TX power: %s",
						    sys_getopt_optarg);
				return;
			}

			newsettings.cluster.max_beacon_tx_power_dbm = value;
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_CLUSTER;
			break;
		}
		case DECT_SHELL_SETT_CMD_CLUSTER_MAX_TX_PWR: {
			long value = shell_strtol(sys_getopt_optarg, 10, &ret);

			if (ret || value < -12 || value > 23) {
				dect_l2_shell_error("Invalid cluster max TX power: %s",
						    sys_getopt_optarg);
				return;
			}
			newsettings.cluster.max_cluster_power_dbm = value;
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_CLUSTER;
			break;
		}
		case DECT_SHELL_SETT_CMD_CLUSTER_CHANNEL_LOADED_PERCENT: {
			tmp_value = shell_strtoul(sys_getopt_optarg, 10, &ret);
			if (ret || tmp_value > 100) {
				dect_l2_shell_error(
						    "Give decent value (range: 0-100)");
				return;
			}
			newsettings.cluster.channel_loaded_percent = tmp_value;
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_CLUSTER;
			break;
		}
		case DECT_SHELL_SETT_CMD_CLUSTER_NBR_INACTIVITY_TIME: {
			tmp_value = shell_strtoul(sys_getopt_optarg, 10, &ret);
			if (ret || tmp_value >= UINT32_MAX) {
				dect_l2_shell_error(
						    "Give decent value (range: 0-%lu)",
						    (unsigned long)UINT32_MAX);
				return;
			}
			newsettings.cluster.neighbor_inactivity_disconnect_timer_ms = tmp_value;
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_CLUSTER;
			break;
		}
		case DECT_SHELL_SETT_CMD_NW_BEACON_PERIOD: {
			tmp_value = atoi(sys_getopt_optarg);
			tmp_value = dect_common_utils_settings_ms_to_mac_nw_beacon_period_in_ms(
				tmp_value);
			if (tmp_value < 0) {
				dect_l2_shell_error("Invalid network beacon period: %s",
						    sys_getopt_optarg);
				return;
			}
			newsettings.nw_beacon.beacon_period = tmp_value;
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_NW_BEACON;
			break;
		}
		case DECT_SHELL_SETT_CMD_NW_BEACON_CHANNEL: {
			tmp_value = shell_strtoul(sys_getopt_optarg, 10, &ret);
			if (ret || tmp_value > UINT16_MAX) {
				dect_l2_shell_error(
					"Invalid network beacon channel: %s, range: 0-%u. "
					"Set to %u to set as disabled.",
					sys_getopt_optarg, UINT16_MAX,
					DECT_NW_BEACON_CHANNEL_NOT_USED);
				return;
			}
			newsettings.nw_beacon.channel = tmp_value;
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_NW_BEACON;
			break;
		}
		case DECT_SHELL_SETT_CMD_ASSOCIATION_MAX_CLUSTER_BEACON_RX_FAILS: {
			tmp_value = shell_strtoul(sys_getopt_optarg, 10, &ret);
			if (ret || tmp_value > UINT8_MAX) {
				dect_l2_shell_error(
						    "Give decent value (range: 0-%d)", UINT8_MAX);
				return;
			}
			newsettings.association.max_beacon_rx_failures = tmp_value;
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_ASSOCIATION;
			break;
		}
		case DECT_SHELL_SETT_CMD_ASSOCIATION_MIN_SENSITIVITY: {
			long value = shell_strtol(sys_getopt_optarg, 10, &ret);

			if (ret || value > INT8_MAX || value < INT8_MIN) {
				dect_l2_shell_error(
						    "Give decent value (range: %d-%d)", INT8_MIN,
						    INT8_MAX);
				return;
			}
			newsettings.association.min_sensitivity_dbm = value;
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_ASSOCIATION;
			break;
		}
		case DECT_SHELL_SETT_CMD_PWR_SAVE: {
			if (!strcmp(sys_getopt_optarg, "on")) {
				newsettings.power_save = true;
			} else if (!strcmp(sys_getopt_optarg, "off")) {
				newsettings.power_save = false;
			} else {
				dect_l2_shell_error("Invalid power_save value: %s",
						    sys_getopt_optarg);
				return;
			}
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_POWER_SAVE;
			break;
		}
		case DECT_SHELL_SETT_CMD_MAX_MCS: {
			tmp_value = shell_strtoul(sys_getopt_optarg, 10, &ret);
			if (ret || tmp_value > 4) {
				dect_l2_shell_error(
						    "Give decent value (range: 0-4)");
				return;
			}
			newsettings.tx.max_mcs = tmp_value;
			newsettings.cmd_params.write_scope_bitmap |= DECT_SETTINGS_WRITE_SCOPE_TX;
			break;
		}
		case DECT_SHELL_SETT_CMD_MAX_TX_PWR: {
			long value = shell_strtol(sys_getopt_optarg, 10, &ret);

			if (ret || value < -40 || value > 23) {
				dect_l2_shell_error(
						    "Give decent value (range: -40-23)");
				return;
			}
			newsettings.tx.max_power_dbm = value;
			newsettings.cmd_params.write_scope_bitmap |= DECT_SETTINGS_WRITE_SCOPE_TX;
			break;
		}
		case DECT_SHELL_SETT_CMD_SEC_MODE: {
			if (!strcmp(sys_getopt_optarg, "none")) {
				newsettings.sec_conf.mode = DECT_SECURITY_MODE_NONE;
			} else if (!strcmp(sys_getopt_optarg, "mode_1")) {
				newsettings.sec_conf.mode = DECT_SECURITY_MODE_1;
			} else {
				dect_l2_shell_error("Invalid security mode: %s",
						    sys_getopt_optarg);
				return;
			}
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_SECURITY_CONFIGURATION;
			break;
		}
		case DECT_SHELL_SETT_CMD_SEC_INTEG_KEY: {
			if (dect_shell_util_atoh(sys_getopt_optarg, strlen(sys_getopt_optarg),
						 newsettings.sec_conf.integrity_key,
						 sizeof(newsettings.sec_conf.integrity_key)) !=
			    DECT_INTEGRITY_KEY_LENGTH) {
				dect_l2_shell_error("Invalid security integrity key: %s",
						    sys_getopt_optarg);
				return;
			}
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_SECURITY_CONFIGURATION;
			break;
		}
		case DECT_SHELL_SETT_CMD_SEC_CIPHER_KEY: {
			if (dect_shell_util_atoh(sys_getopt_optarg, strlen(sys_getopt_optarg),
						 newsettings.sec_conf.cipher_key,
						 sizeof(newsettings.sec_conf.cipher_key)) !=
			    DECT_CIPHER_KEY_LENGTH) {
				dect_l2_shell_error("Invalid security cipher key: %s",
						    sys_getopt_optarg);
				return;
			}
			newsettings.cmd_params.write_scope_bitmap |=
				DECT_SETTINGS_WRITE_SCOPE_SECURITY_CONFIGURATION;
			break;
		}

		case 'h':
			goto show_usage;
		case '?':
		default:
			dect_l2_shell_error("Unknown option (%s). See usage:",
					    argv[sys_getopt_optind - 1]);
			goto show_usage;
		}
	}

	if (sys_getopt_optind < argc) {
		dect_l2_shell_error("Arguments without '-' not supported: %s",
				    argv[argc - 1]);
		goto show_usage;
	}
	if (newsettings.cmd_params.write_scope_bitmap == 0 &&
	    !newsettings.cmd_params.reset_to_driver_defaults) {
		dect_l2_shell_error("No settings to write. See usage:");
		goto show_usage;
	}

	ret = net_mgmt(NET_REQUEST_DECT_SETTINGS_WRITE, context.iface, &newsettings,
		       sizeof(newsettings));
	if (ret) {
		dect_l2_shell_error(
			"Cannot write new settings: %d, there was a failure on scope: %s", ret,
			dect_common_utils_settings_write_scope_to_string(
				newsettings.cmd_params.failure_scope_bitmap_out));
	} else {
		dect_l2_shell_print("Settings updated.");
	}

	return;

show_usage:
	/* Split into many because of limited size of CONFIG_DESH_PRINT_BUFFER_SIZE */
	dect_l2_shell_print("%s", dect_shell_sett_common_usage_str);
	dect_l2_shell_print("%s", dect_shell_sett_auto_start_usage_str);
	dect_l2_shell_print("%s", dect_shell_sett_rssi_scan_usage_str);
	dect_l2_shell_print("%s", dect_shell_sett_association_usage_str);
	dect_l2_shell_print("%s", dect_shell_sett_cluster_usage_str1);
	dect_l2_shell_print("%s", dect_shell_sett_cluster_usage_str2);
	dect_l2_shell_print("%s", dect_shell_sett_network_beacon_usage_str);
	dect_l2_shell_print("%s", dect_shell_sett_sec_conf_usage_str);
}

static void dect_shell_cluster_start_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	struct dect_cluster_start_req_params params;

	set_shell(shell); /* Store shell from command for use as default */

	/* Is there optional channel parameter */
	if (argc >= 2) {
		int channel = shell_strtoul(argv[1], 10, &ret);

		if (ret || channel < 0 || channel > UINT16_MAX) {
			dect_l2_shell_error("Invalid channel: %s", argv[1]);
			return;
		}
		params.channel = channel;
	} else {
		params.channel = DECT_CLUSTER_CHANNEL_ANY;
	}

	ret = net_mgmt(NET_REQUEST_DECT_CLUSTER_START, context.iface, &params, sizeof(params));
	if (ret) {
		dect_l2_shell_error("Cannot start cluster: %d", ret);
	} else {
		dect_l2_shell_print("Cluster start initiated.");
	}
}

static void dect_shell_cluster_reconfig_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	struct dect_cluster_reconfig_req_params params;
	struct dect_settings current_settings;
	int long_index = 0;
	int opt, tmp_value;

	set_shell(shell); /* Store shell from command for use as default */
	if (argc < 2) {
		goto show_usage;
	}
	sys_getopt_init();

	params.channel = DECT_CLUSTER_CHANNEL_ANY;
	ret = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, context.iface, &current_settings,
		       sizeof(current_settings));
	if (ret) {
		dect_l2_shell_error("Cannot read current settings: %d", ret);
		return;
	}
	params.max_beacon_tx_power_dbm = current_settings.cluster.max_beacon_tx_power_dbm;
	params.max_cluster_power_dbm = current_settings.cluster.max_cluster_power_dbm;
	params.period = current_settings.cluster.beacon_period;

	while ((opt = sys_getopt_long(argc, argv, "c:h", long_options_cluster_reconfig_cmd,
				  &long_index)) != -1) {
		switch (opt) {
		case 'c': {
			int channel = shell_strtoul(sys_getopt_optarg, 10, &ret);

			if (ret || channel < 0 || channel > UINT16_MAX) {
				dect_l2_shell_error("Invalid channel: %s", sys_getopt_optarg);
				return;
			}
			params.channel = channel;
			break;
		}

		case DECT_SHELL_CLUSTER_RECONFIG_CMD_CLUSTER_BEACON_PERIOD: {
			tmp_value = atoi(sys_getopt_optarg);
			tmp_value =
				dect_common_utils_settings_ms_to_mac_cluster_beacon_period_in_ms(
					tmp_value);
			if (tmp_value < 0) {
				dect_l2_shell_error("Invalid cluster beacon period: %s",
						    sys_getopt_optarg);
				return;
			}
			params.period = tmp_value;
			break;
		}
		case DECT_SHELL_CLUSTER_RECONFIG_CMD_CLUSTER_MAX_BEACON_TX_PWR: {
			tmp_value = atoi(sys_getopt_optarg);
			if (tmp_value < -40 || tmp_value > 23) {
				dect_l2_shell_error("Invalid cluster beacon TX power: %s",
						    sys_getopt_optarg);
				return;
			}

			params.max_beacon_tx_power_dbm = tmp_value;
			break;
		}
		case DECT_SHELL_CLUSTER_RECONFIG_CMD_CLUSTER_MAX_TX_PWR: {
			tmp_value = atoi(sys_getopt_optarg);
			if (tmp_value < -12 || tmp_value > 23) {
				dect_l2_shell_error("Invalid cluster max TX power: %s",
						    sys_getopt_optarg);
				return;
			}

			params.max_cluster_power_dbm = tmp_value;
			break;
		}
		case 'h':
			goto show_usage;
		case '?':
		default:
			dect_l2_shell_error("Unknown option (%s). See usage:",
					    argv[sys_getopt_optind - 1]);
			goto show_usage;
		}
	}

	if (sys_getopt_optind < argc) {
		dect_l2_shell_error("Arguments without '-' not supported: %s",
				    argv[argc - 1]);
		goto show_usage;
	}

	ret = net_mgmt(NET_REQUEST_DECT_CLUSTER_RECONFIGURE, context.iface, &params,
		       sizeof(params));
	if (ret) {
		dect_l2_shell_error("Cannot reconfigure cluster: %d", ret);
	} else {
		dect_l2_shell_print("Cluster reconfiguration initiated.");
	}
	return;

show_usage:
	dect_l2_shell_print("%s", dect_shell_cluster_reconfig_usage_str);
}

static void dect_shell_cluster_info_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	set_shell(shell); /* Store shell from command for use as default */
	ret = net_mgmt(NET_REQUEST_DECT_CLUSTER_INFO, context.iface, NULL, 0);
	if (ret) {
		dect_l2_shell_error(
				    "Cannot initiate reqquest for cluster info: %d", ret);
	} else {
		dect_l2_shell_print("Cluster information request initiated.");
	}
}

static void dect_shell_nw_beacon_start_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	struct dect_nw_beacon_start_req_params params;
	int long_index = 0;
	int opt;

	set_shell(shell); /* Store shell from command for use as default */
	if (argc < 2) {
		goto show_usage;
	}
	sys_getopt_init();

	params.additional_ch_count = 0;
	params.channel = 0;

	while ((opt = sys_getopt_long(argc, argv, "c:h", long_options_nw_beacon_start,
				      &long_index)) != -1) {
		switch (opt) {
		case 'c': {
			params.channel = atoi(sys_getopt_optarg);
			break;
		}

		case DECT_SHELL_NW_BEACON_START_CMD_ADD_CHANNELS: {
			char *ch_string = strtok(sys_getopt_optarg, ",");

			if (ch_string == NULL) {
				params.additional_ch_list[0] = atoi(sys_getopt_optarg);
				params.additional_ch_count = 1;
			} else {
				while (ch_string != NULL && params.additional_ch_count < 3) {
					params.additional_ch_list[params.additional_ch_count++] =
						atoi(ch_string);
					ch_string = strtok(NULL, ",");
				}
			}
			break;
		}
		case 'h':
			goto show_usage;
		case '?':
		default:
			dect_l2_shell_error("Unknown option (%s). See usage:",
					    argv[sys_getopt_optind - 1]);
			goto show_usage;
		}
	}

	if (sys_getopt_optind < argc) {
		dect_l2_shell_error("Arguments without '-' not supported: %s",
				    argv[argc - 1]);
		goto show_usage;
	}
	if (params.channel == 0) {
		dect_l2_shell_error(
				    "Network beacon channel needs to be given. See usage:");
		goto show_usage;
	}

	ret = net_mgmt(NET_REQUEST_DECT_NW_BEACON_START, context.iface, &params, sizeof(params));
	if (ret) {
		dect_l2_shell_error("Cannot start cluster: %d", ret);
	} else {
		dect_l2_shell_print("Network beacon start initiated.");
	}
	return;

show_usage:
	dect_l2_shell_print("%s", dect_shell_nw_beacon_start_usage_str);
}

static void dect_shell_nw_beacon_stop_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	struct dect_nw_beacon_stop_req_params params;

	set_shell(shell); /* Store shell from command for use as default */
	ret = net_mgmt(NET_REQUEST_DECT_NW_BEACON_STOP, context.iface, &params, sizeof(params));
	if (ret) {
		dect_l2_shell_error("Cannot initiate network beacon stopping: %d",
				    ret);
	} else {
		dect_l2_shell_print("Network beacon stop initiated.");
	}
}

#if defined(CONFIG_MODEM_CELLULAR)
char *dect_shell_util_cellular_reg_status_to_string(enum cellular_registration_status reg_status,
						    char *out_str_buff, size_t out_str_buff_len)
{
	if (out_str_buff == NULL || out_str_buff_len == 0) {
		return NULL;
	}

	switch (reg_status) {
	case CELLULAR_REGISTRATION_NOT_REGISTERED:
		strncpy(out_str_buff, "Not registered", out_str_buff_len);
		break;
	case CELLULAR_REGISTRATION_REGISTERED_HOME:
		strncpy(out_str_buff, "Registered (Home)", out_str_buff_len);
		break;
	case CELLULAR_REGISTRATION_SEARCHING:
		strncpy(out_str_buff, "Searching", out_str_buff_len);
		break;
	case CELLULAR_REGISTRATION_DENIED:
		strncpy(out_str_buff, "Denied", out_str_buff_len);
		break;
	case CELLULAR_REGISTRATION_UNKNOWN:
		strncpy(out_str_buff, "Unknown", out_str_buff_len);
		break;
	case CELLULAR_REGISTRATION_REGISTERED_ROAMING:
		strncpy(out_str_buff, "Registered (Roaming)", out_str_buff_len);
		break;
	default:
		snprintf(out_str_buff, out_str_buff_len, "Unknown (%d)", reg_status);
		break;
	}

	return out_str_buff;
}

const struct device *modem_dev = DEVICE_DT_GET(DT_ALIAS(modem));

static void print_cellular_info(void)
{
	int rc;
	int16_t rsrp;
	char buffer[256];
	enum cellular_registration_status reg_status;

	dect_l2_shell_print("Border Router - Cellular Modem LTE status:");

	rc = cellular_get_registration_status(modem_dev, CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN,
					      &reg_status);
	if (!rc) {
		dect_l2_shell_print("  Network registration status:  %s",
				    dect_shell_util_cellular_reg_status_to_string(
					    reg_status, buffer, sizeof(buffer)));
	} else {
		dect_l2_shell_error(
				    "    Failed to get network registration status, error: %d", rc);
	}

	rc = cellular_get_signal(modem_dev, CELLULAR_SIGNAL_RSRP, &rsrp);
	if (!rc) {
		dect_l2_shell_print("  RSRP:                         %d dBm",
				    rsrp);
	}
	rc = cellular_get_signal(modem_dev, CELLULAR_SIGNAL_RSRQ, &rsrp);
	if (!rc) {
		dect_l2_shell_print("  RSRQ:                         %d dB", rsrp);
	}

	rc = cellular_get_modem_info(modem_dev, CELLULAR_MODEM_INFO_IMEI, &buffer[0],
				     sizeof(buffer));
	if (!rc) {
		dect_l2_shell_print("  IMEI:                         %s", buffer);
	} else {
		dect_l2_shell_error("  Failed to get IMEI, error: %d", rc);
	}

	rc = cellular_get_modem_info(modem_dev, CELLULAR_MODEM_INFO_MODEL_ID, &buffer[0],
				     sizeof(buffer));
	if (!rc) {
		dect_l2_shell_print("  Model:                        %s", buffer);
	} else {
		dect_l2_shell_error("  Failed to get MODEL_ID, error: %d", rc);
	}

	rc = cellular_get_modem_info(modem_dev, CELLULAR_MODEM_INFO_MANUFACTURER, &buffer[0],
				     sizeof(buffer));
	if (!rc) {
		dect_l2_shell_print("  Manufacturer:                 %s", buffer);
	} else {
		dect_l2_shell_error("  Failed to get MANUFACTURER, error: %d", rc);
	}

	rc = cellular_get_modem_info(modem_dev, CELLULAR_MODEM_INFO_FW_VERSION, &buffer[0],
				     sizeof(buffer));
	if (!rc) {
		dect_l2_shell_print("  Modem firmware version:       %s", buffer);
	} else {
		dect_l2_shell_error("  Failed to get FW_VERSION, error: %d", rc);
	}
}
#endif /* CONFIG_MODEM_CELLULAR */

static void dect_shell_status_cmd_print(struct dect_status_info *dect_status)
{
	dect_l2_shell_print("DECT NR+ status:");
	dect_l2_shell_print("  Modem FW version:             %s",
			    dect_status->fw_version_str);
	dect_l2_shell_print("  Modem activated:              %s",
			    dect_status->mdm_activated ? "yes" : "no");
	dect_l2_shell_print("  Cluster running:              %s",
			    dect_status->cluster_running ? "yes" : "no");
	if (dect_status->cluster_running) {
		dect_l2_shell_print("  Cluster channel:              %d",
				    dect_status->cluster_channel);
	}
	dect_l2_shell_print("  Network beacon running:       %s",
			    dect_status->nw_beacon_running ? "yes" : "no");
	if (dect_status->parent_count || dect_status->child_count) {
		dect_l2_shell_print("  Associations:");
	}
	if (dect_status->parent_count > 0) {
		for (int i = 0; i < dect_status->parent_count; i++) {
			dect_l2_shell_print(
					    "    Parent long RD ID:              %u (0x%08x)",
					    dect_status->parent_associations[i].long_rd_id,
					    dect_status->parent_associations[i].long_rd_id);
			dect_l2_shell_print(
				"      Local IPv6 address:           %s",
				net_sprint_ipv6_addr(
					&dect_status->parent_associations[i].local_ipv6_addr));
			if (dect_status->parent_associations[i].global_ipv6_addr_set) {
				dect_l2_shell_print(
					"      Global IPv6 address:          %s",
					net_sprint_ipv6_addr(&dect_status->parent_associations[i]
								      .global_ipv6_addr));
			}
		}
	}
	if (dect_status->child_count > 0) {
		for (int i = 0; i < dect_status->child_count; i++) {
			dect_l2_shell_print(
					    "    Child #%d long RD ID:       %u (0x%08x)", i + 1,
					    dect_status->child_associations[i].long_rd_id,
					    dect_status->child_associations[i].long_rd_id);
			dect_l2_shell_print(
				"      Local IPv6 address:       %s",
				net_sprint_ipv6_addr(
					&dect_status->child_associations[i].local_ipv6_addr));
			if (dect_status->child_associations[i].global_ipv6_addr_set) {
				dect_l2_shell_print(
					"      Global IPv6 address:      %s",
					net_sprint_ipv6_addr(&dect_status->child_associations[i]
								      .global_ipv6_addr));
			}
		}
	}
#if defined(CONFIG_NET_L2_DECT_BR)
	char ifname[CONFIG_NET_INTERFACE_NAME_LEN + 1] = {0};
	int ret;

	dect_l2_shell_print("  Border router and sink information:");
	if (dect_status->br_net_iface) {
		ret = net_if_get_name(dect_status->br_net_iface, ifname, sizeof(ifname) - 1);
		if (ret < 0) {
			dect_l2_shell_print(
					    "    Border router network interface:             %p",
					    dect_status->br_net_iface);
		} else {
			dect_l2_shell_print(
				"    Border router network interface:             %s (%p)", ifname,
				dect_status->br_net_iface);
		}
	} else {
		dect_l2_shell_print(
				    "    Border router network interface:   not set");
	}
	if (dect_status->br_global_ipv6_addr_prefix_set) {
		dect_l2_shell_print(
				    "    Border router global IPv6 address prefix/%d: %s",
				    dect_status->br_global_ipv6_addr_prefix_len * 8,
				    net_sprint_ipv6_addr(&dect_status->br_global_ipv6_addr_prefix));
		dect_l2_shell_print(
				    "      Connection to Internet should be available.");
	} else {
		dect_l2_shell_print(
				    "    Border router global IPv6 address: not set");
	}
#endif
#if defined(CONFIG_MODEM_CELLULAR)
	print_cellular_info();
#endif
}

static void dect_shell_status_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	struct dect_status_info dect_status;

	set_shell(shell); /* Store shell from command for use as default */
	ret = net_mgmt(NET_REQUEST_DECT_STATUS_INFO_GET, context.iface, &dect_status,
		       sizeof(dect_status));
	if (ret) {
		dect_l2_shell_error("Cannot get status information: %d", ret);
	} else {
		dect_shell_status_cmd_print(&dect_status);
	}
}

static void dect_shell_activate_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	set_shell(shell); /* Store shell from command for use as default */
	ret = net_mgmt(NET_REQUEST_DECT_ACTIVATE, context.iface, NULL, 0);
	if (ret) {
		dect_l2_shell_error("Cannot activate dect nr+ stack: %d", ret);
	} else {
		dect_l2_shell_print("DECT NR+ stack activation initiated.");
	}
}

static void dect_shell_deactivate_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	set_shell(shell); /* Store shell from command for use as default */
	ret = net_mgmt(NET_REQUEST_DECT_DEACTIVATE, context.iface, NULL, 0);
	if (ret) {
		dect_l2_shell_error("Cannot de-activate dect nr+ stack: %d", ret);
	} else {
		dect_l2_shell_print("DECT NR+ stack de-activation initiated.");
	}
}

static void dect_shell_neighbor_list_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	set_shell(shell); /* Store shell from command for use as default */
	ret = net_mgmt(NET_REQUEST_DECT_NEIGHBOR_LIST, context.iface, NULL, 0);
	if (ret) {
		dect_l2_shell_error("Cannot get neighbor list: %d", ret);
	} else {
		dect_l2_shell_print("Neighbor list request initiated.");
	}
}

static void dect_shell_neighbor_info_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	struct dect_neighbor_info_req_params params;

	set_shell(shell); /* Store shell from command for use as default */
	if (argc < 2) {
		dect_l2_shell_error("Alert data not provided");
		return;
	}
	int err = 0;
	int long_rd_id = shell_strtoul(argv[1], 10, &err);

	if (err) {
		dect_l2_shell_error("Invalid long RD ID: %s", argv[1]);
		return;
	}
	params.long_rd_id = long_rd_id;
	ret = net_mgmt(NET_REQUEST_DECT_NEIGHBOR_INFO, context.iface, &params, sizeof(params));
	if (ret) {
		dect_l2_shell_error("Cannot get neighbor info: %d", ret);
	} else {
		dect_l2_shell_print("Neighbor info request initiated.");
	}
}

static void dect_shell_nw_create_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	set_shell(shell); /* Store shell from command for use as default */
	ret = net_mgmt(NET_REQUEST_DECT_NETWORK_CREATE, context.iface, NULL, 0);
	if (ret) {
		dect_l2_shell_error(
				    "Cannot initiate request for creating a network: %d", ret);
	} else {
		dect_l2_shell_print("Network creation initiated.");
	}
}

static void dect_shell_nw_remove_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	set_shell(shell); /* Store shell from command for use as default */
	ret = net_mgmt(NET_REQUEST_DECT_NETWORK_REMOVE, context.iface, NULL, 0);
	if (ret) {
		dect_l2_shell_error(
				    "Cannot initiate request for removing a network: %d", ret);
	} else {
		dect_l2_shell_print("Network removal initiated.");
	}
}

static void dect_shell_nw_join_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	set_shell(shell); /* Store shell from command for use as default */
	ret = net_mgmt(NET_REQUEST_DECT_NETWORK_JOIN, context.iface, NULL, 0);
	if (ret) {
		dect_l2_shell_error(
				    "Cannot initiate request for joining a network: %d", ret);
	} else {
		dect_l2_shell_print("Network joining initiated.");
	}
}

static void dect_shell_nw_unjoin_cmd(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	set_shell(shell); /* Store shell from command for use as default */
	ret = net_mgmt(NET_REQUEST_DECT_NETWORK_UNJOIN, context.iface, NULL, 0);
	if (ret) {
		dect_l2_shell_error(
				    "Cannot initiate request unjoining from a network: %d", ret);
	} else {
		dect_l2_shell_print("Unjoining from a network initiated.");
	}
}

static void dect_shell_connect_cmd(const struct shell *shell, size_t argc, char **argv)
{
	set_shell(shell); /* Store shell from command for use as default */

#if defined(CONFIG_NET_CONNECTION_MANAGER)
	int ret;

	ret = conn_mgr_if_connect(context.iface);
	if (ret < 0) {
		dect_l2_shell_error("conn_mgr_if_connect failed: %d", ret);
		return;
	}
	dect_l2_shell_print("connect initiated.");
#else
	struct dect_settings current_settings;
	int err;

	err = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, context.iface, &current_settings,
		       sizeof(current_settings));
	if (err) {
		dect_l2_shell_error("Cannot read current settings: %d", err);
		return;
	}
	if (current_settings.device_type & DECT_DEVICE_TYPE_PT) {
		err = net_mgmt(NET_REQUEST_DECT_NETWORK_JOIN, context.iface, NULL, 0);
		if (err) {
			dect_l2_shell_error(
					    "cannot initiate request for joining "
					    "a network: %d",
					    err);
		} else {
			dect_l2_shell_print("network joining initiated.");
		}

	} else {
		__ASSERT_NO_MSG(current_settings.device_type & DECT_DEVICE_TYPE_FT);
		err = net_mgmt(NET_REQUEST_DECT_NETWORK_CREATE, context.iface, NULL, 0);
		if (err) {
			dect_l2_shell_error(
					    "cannot initiate request for creating "
					    "a network: %d",
					    err);
		} else {
			dect_l2_shell_print("network creation initiated.");
		}
	}

#endif
}

static void dect_shell_disconnect_cmd(const struct shell *shell, size_t argc, char **argv)
{
	set_shell(shell); /* Store shell from command for use as default */

#if defined(CONFIG_NET_CONNECTION_MANAGER)
	int ret = conn_mgr_if_disconnect(context.iface);

	if (ret < 0) {
		dect_l2_shell_error("Failed to initiate a disconnect: %d", ret);
		return;
	}
	dect_l2_shell_print("disconnect initiated.");
#else
	struct dect_settings current_settings;
	int ret;

	ret = net_mgmt(NET_REQUEST_DECT_SETTINGS_READ, context.iface, &current_settings,
		       sizeof(current_settings));
	if (ret) {
		dect_l2_shell_error("Cannot read current settings: %d\n", ret);
		return;
	}
	if (current_settings.device_type & DECT_DEVICE_TYPE_PT) {
		ret = net_mgmt(NET_REQUEST_DECT_NETWORK_UNJOIN, context.iface, NULL, 0);
		if (ret) {
			dect_l2_shell_error(
					    "cannot initiate request for unjoining "
					    "a network: %d\n",
					    ret);
		} else {
			dect_l2_shell_print("network unjoining initiated.\n");
		}

	} else {
		__ASSERT_NO_MSG(current_settings.device_type & DECT_DEVICE_TYPE_FT);
		ret = net_mgmt(NET_REQUEST_DECT_NETWORK_REMOVE, context.iface, NULL, 0);
		if (ret) {
			dect_l2_shell_error(
					    "cannot initiate request for removing "
					    "a network: %d\n",
					    ret);
		} else {
			dect_l2_shell_print("network removal initiated.\n");
		}
	}
#endif
}

SHELL_SUBCMD_SET_CREATE(dect_commands, (dect));

SHELL_SUBCMD_ADD((dect), activate, NULL,
		 "Activate dect nr+ stack.\n"
		 " Usage: dect activate",
		 dect_shell_activate_cmd, 1, 0);
SHELL_SUBCMD_ADD((dect), deactivate, NULL,
		 "De-activate dect nr+ stack.\n"
		 " Usage: dect deactivate",
		 dect_shell_deactivate_cmd, 1, 0);
SHELL_SUBCMD_ADD((dect), status, NULL,
		 "Get status information.\n"
		 " Usage: dect status",
		 dect_shell_status_cmd, 1, 0);
SHELL_SUBCMD_ADD((dect), rssi_scan, &dect_commands,
		 "Execute a RSSI scan.\n"
		 "[-h, --help] : Print out the help for the rssi_scan command.\n",
		 dect_shell_rssi_scan_cmd, 1, 8);
SHELL_SUBCMD_ADD((dect), scan, &dect_commands,
		 "Scan for dect nr+ beacons.\n"
		 "[-h, --help] : Print out the help for the scan command.\n",
		 dect_shell_scan_cmd, 1, 8);
SHELL_SUBCMD_ADD((dect), tx, &dect_commands,
		 "Send data to target device.\n"
		 "[-h, --help] : Print out the help for the tx command.\n",
		 dect_shell_tx_cmd, 1, 8);
SHELL_SUBCMD_ADD((dect), rx, &dect_commands,
		 "Receive DLC data.\n"
		 "[-h, --help] : Print out the help for the rx command.\n",
		 dect_shell_rx_cmd, 1, 8);
SHELL_SUBCMD_ADD((dect), associate, &dect_commands,
		 "Performs association procedure with given FT-device\n"
		 " Usage: mac associate [options: see: mac associate -h]",
		 dect_shell_associate_cmd, 1, 3);
SHELL_SUBCMD_ADD((dect), dissociate, &dect_commands,
		 "Releases association with a given FT-device\n"
		 " Usage: dect dissociate [options: see: mac associate -h]",
		 dect_shell_dissociate_cmd, 1, 3);
SHELL_SUBCMD_ADD((dect), cluster_start, NULL,
		 "Start FT's cluster based on settings.\n"
		 " Usage: dect cluster_start [channel]\n",
		 dect_shell_cluster_start_cmd, 1, 1);
SHELL_SUBCMD_ADD((dect), cluster_reconfig, NULL,
		 "Reconfigure FT's running cluster based on given parameters and/or settings.\n"
		 "[-h, --help] : Print out the help for the cluster_reconfig command.\n",
		 dect_shell_cluster_reconfig_cmd, 1, 4);
SHELL_SUBCMD_ADD((dect), cluster_info, NULL,
		 "Request for cluster information.\n"
		 " Usage: dect cluster_info\n",
		 dect_shell_cluster_info_cmd, 1, 0);
SHELL_SUBCMD_ADD((dect), nw_beacon_start, NULL,
		 "Start network beacon.\n"
		 "[-h, --help] : Print out the help for the nw_beacon_start command.\n",
		 dect_shell_nw_beacon_start_cmd, 2, 3);
SHELL_SUBCMD_ADD((dect), nw_beacon_stop, NULL,
		 "Stop network beacon.\n"
		 " Usage: dect nw_beacon_stop",
		 dect_shell_nw_beacon_stop_cmd, 1, 0);
SHELL_SUBCMD_ADD((dect), neighbor_list, NULL,
		 "Request for a neighbor list.\n"
		 " Usage: dect neighbor_list",
		 dect_shell_neighbor_list_cmd, 1, 0);
SHELL_SUBCMD_ADD((dect), neighbor_info, NULL,
		 "Request for neighbor information.\n"
		 " Usage: dect neighbor_info <neighbor_long_rd_id>\n",
		 dect_shell_neighbor_info_cmd, 2, 0);
SHELL_SUBCMD_ADD((dect), nw_create, NULL,
		 "Request to create dect nr+ network based on settings.\n"
		 " Usage: dect nw_create\n",
		 dect_shell_nw_create_cmd, 1, 0);
SHELL_SUBCMD_ADD((dect), nw_remove, NULL,
		 "Request to remove dect nr+ network.\n"
		 " Usage: dect nw_remove\n",
		 dect_shell_nw_remove_cmd, 1, 0);
SHELL_SUBCMD_ADD((dect), nw_join, NULL,
		 "Request to join into dect nr+ network based on settings.\n"
		 " Usage: dect nw_join\n",
		 dect_shell_nw_join_cmd, 1, 0);
SHELL_SUBCMD_ADD((dect), nw_unjoin, NULL,
		 "Request to unjoin from a dect nr+ network.\n"
		 " Usage: dect nw_unjoin\n",
		 dect_shell_nw_unjoin_cmd, 1, 0);
SHELL_SUBCMD_ADD((dect), connect, NULL,
		 "Request to connect into dect nr+ network based on settings.\n"
		 "Usage: dect connect\n"
		 " FT: same as nw_create, i.e. performs RSSI scan and creates cluster to free "
		 " channel at set band.\n"
		 " PT: same as nw_join, Performs network scan on a set band and associates with "
		 " FT device as set in nw_join settings.\n",
		 dect_shell_connect_cmd, 1, 0);
SHELL_SUBCMD_ADD(
	(dect), disconnect, NULL,
	"Request to disconnect from dect nr+ network.\n"
	"Usage: dect disconnect\n"
	" FT: same as nw_remove, i.e. removes cluster and network.\n"
	" PT: same as nw_unjoin, i.e. unjoins from network and releases association with FT "
	" device.\n",
	dect_shell_disconnect_cmd, 1, 0);
SHELL_SUBCMD_ADD((dect), sett, &dect_commands,
		 "Set and read common dect settings\n"
		 "[-h, --help] : Print out the help for the scan command.\n",
		 dect_shell_sett_cmd, 1, 19);

SHELL_CMD_REGISTER(dect, &dect_commands, "DECT NR+ commands", NULL);

int dect_net_l2_shell_init(const struct dect_net_l2_shell_print_fns *print_fns)
{
	if (print_fns == NULL) {
		context.custom_print_enabled = false;
		memset(&context.print_fns, 0, sizeof(context.print_fns));
	} else {
		context.custom_print_enabled = true;
		context.print_fns = *print_fns;
	}

	return 0;
}

static int dect_shell_init(void)
{
	dect_shell_rx_sockfd = -1;
	context.iface = net_if_get_by_index(net_if_get_by_name(CONFIG_DECT_MDM_DEVICE_NAME));
	if (!context.iface) {
		printk("Interface %s not found - continue\n", CONFIG_DECT_MDM_DEVICE_NAME);
	}
	context.shell = shell_backend_uart_get_ptr();
	if (!context.shell) {
		printk("Failed to get shell backend - continue\n");
	}

	net_mgmt_init_event_callback(&dect_shell_mgmt_cb, dect_shell_net_mgmt_event_handler,
				     DECT_SHELL_MGMT_EVENTS);

	net_mgmt_add_event_callback(&dect_shell_mgmt_cb);

	return 0;
}

SYS_INIT(dect_shell_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
