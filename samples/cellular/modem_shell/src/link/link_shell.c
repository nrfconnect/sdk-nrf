/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <unistd.h>
#include <getopt.h>

#include <nrf_modem_at.h>

#include "mosh_print.h"
#include "link.h"
#include "link_api.h"
#include "link_shell.h"
#include "link_shell_print.h"
#include "link_shell_pdn.h"
#include "link_settings.h"
#include "net_utils.h"

#define LINK_SHELL_EDRX_VALUE_STR_LENGTH 4
#define LINK_SHELL_EDRX_PTW_STR_LENGTH 4
#define LINK_SHELL_PSM_PARAM_STR_LENGTH 8

enum link_shell_command {
	LINK_CMD_STATUS = 0,
	LINK_CMD_SETTINGS,
	LINK_CMD_CONEVAL,
	LINK_CMD_DEFCONT,
	LINK_CMD_DEFCONTAUTH,
	LINK_CMD_RSRP,
	LINK_CMD_SEARCH,
	LINK_CMD_NCELLMEAS,
	LINK_CMD_MDMSLEEP,
	LINK_CMD_TAU,
	LINK_CMD_CONNECT,
	LINK_CMD_DISCONNECT,
	LINK_CMD_FUNMODE,
	LINK_CMD_SYSMODE,
	LINK_CMD_NORMAL_MODE_AT,
	LINK_CMD_NORMAL_MODE_AUTO,
	LINK_CMD_EDRX,
	LINK_CMD_PSM,
	LINK_CMD_RAI,
	LINK_CMD_DNSADDR,
	LINK_CMD_REDMOB,
};

enum link_shell_common_options {
	LINK_COMMON_NONE = 0,
	LINK_COMMON_READ,
	LINK_COMMON_WRITE,
	LINK_COMMON_ENABLE,
	LINK_COMMON_DISABLE,
	LINK_COMMON_SUBSCRIBE,
	LINK_COMMON_UNSUBSCRIBE,
	LINK_COMMON_START,
	LINK_COMMON_STOP,
	LINK_COMMON_RESET
};

/******************************************************************************/
static const char link_settings_usage_str[] =
	"Usage: link settings --read | --reset | --mreset_all | --mreset_user\n"
	"Options:\n"
	"  -r, --read,        Read and print current persistent settings\n"
	"      --reset,       Reset all persistent settings to defaults\n"
	"      --mreset_all,  Reset all modem settings to defaults\n"
	"      --mreset_user, Reset modem user configurable settings to defaults\n";

static const char link_defcont_usage_str[] =
	"Usage: link defcont --enable [options] | --disable | --read\n"
	"Options:\n"
	"  -r, --read,         Read and print current config\n"
	"  -d, --disable,      Disable custom config for default PDP context\n"
	"  -e, --enable,       Enable custom config for default PDP context\n"
	"  -a, --apn, [str]    Set default Access Point Name\n"
	"  -f, --family, [str] Address family: 'ipv4v6' (default), 'ipv4', 'ipv6',\n"
	"                      'non-ip'\n";

static const char link_defcontauth_usage_str[] =
	"Usage: link defcontauth --enable [options] | --disable | --read\n"
	"Options:\n"
	"  -r, --read,         Read and print current config\n"
	"  -d, --disable,      Disable custom config for default PDP context\n"
	"  -e, --enable,       Enable custom config for default PDP context\n"
	"  -U, --uname, [str]  Username\n"
	"  -P, --pword, [str]  Password\n"
	"  -A, --prot, [int]   Authentication protocol (Default: 0 (None), 1 (PAP),\n"
	"                      2 (CHAP)\n";

static const char link_connect_usage_str[] =
	"Usage: link connect --apn <apn str> [--family <pdn family str>] [auth options]\n"
	"Options:\n"
	"  -a, --apn, [str]    Access Point Name\n"
	"  -f, --family, [str] PDN family: 'ipv4v6', 'ipv4', 'ipv6', 'non-ip'\n"
	"Optional authentication options:\n"
	"  -U, --uname, [str]  Username\n"
	"  -P, --pword, [str]  Password\n"
	"  -A, --prot,  [int]  Authentication protocol: 0 (None), 1 (PAP), 2 (CHAP)\n";

static const char link_disconnect_usage_str[] =
	"Usage: link disconnect -I <cid>\n"
	"Options:\n"
	"  -I, --cid, [int]    Use this option to disconnect specific PDN CID\n";

static const char link_sysmode_usage_str[] =
	"Usage: link sysmode [options] | --read | --reset\n"
	"Options:\n"
	"  -r, --read,             Read system modes set in modem and by 'link sysmode'\n"
	"      --reset,            Reset the set sysmode as default\n"
	"  -m, --ltem,             Set LTE-M (LTE Cat-M1) system mode\n"
	"  -n, --nbiot,            Set NB-IoT (LTE Cat-NB1) system mode\n"
	"      --ltem_nbiot,       Set LTE-M + NB-IoT system mode\n"
	"  -g, --gnss,             Set GNSS system mode\n"
	"  -M, --ltem_gnss,        Set LTE-M + GNSS system mode\n"
	"  -N, --nbiot_gnss,       Set NB-IoT + GNSS system mode\n"
	"      --ltem_nbiot_gnss,  Set LTE-M + NB-IoT + GNSS system mode\n"
	"\n"
	"Additional LTE mode preference that can be optionally given\n"
	"and might make an impact with multimode system modes in modem,\n"
	" i.e. with --ltem_nbiot or --ltem_nbiot_gnss\n"
	"      --pref_auto,            auto, selected by modem (set as default if not\n"
	"                              given)\n"
	"      --pref_ltem,            LTE-M is preferred over PLMN selection\n"
	"      --pref_nbiot,           NB-IoT is preferred over PLMN selection\n"
	"      --pref_ltem_plmn_prio,  LTE-M is preferred, but PLMN selection is more\n"
	"                              important\n"
	"      --pref_nbiot_plmn_prio, NB-IoT is preferred, but PLMN selection is more\n"
	"                              important\n";

static const char link_funmode_usage_str[] =
	"Usage: link funmode [option] | --read\n"
	"Options:\n"
	"  -r, --read,              Read modem functional mode\n"
	"  -0, --pwroff,            Set modem power off\n"
	"  -1, --normal,            Set modem normal mode\n"
	"      --normal_no_rel14,   Set modem normal mode without setting Release 14 features\n"
	"  -4, --flightmode,        Set modem offline.\n"
	"      --lteoff,            Deactivates LTE without shutting down GNSS services.\n"
	"      --lteon,             Activates LTE without changing GNSS.\n"
	"      --gnssoff,           Deactivates GNSS without shutting down LTE services.\n"
	"      --gnsson,            Activates GNSS without changing LTE.\n"
	"      --uiccoff,           Deactivates UICC.\n"
	"      --uiccon,            Activates UICC.\n"
	"      --flightmode_uiccon, Sets the device to flight mode without shutting down\n"
	"                           UICC.\n";

static const char link_normal_mode_at_usage_str[] =
	"Usage: link nmodeat --read | --mem<1-3>\n"
	"Options:\n"
	"  -r, --read,       Read all set custom normal mode at commands\n"
	"      --mem[1-3],   Set at cmd to given memory slot,\n"
	"                    Example: \"link nmodeat --mem1 \"at%xbandlock=2,\\\"100\\\"\"\"\n"
	"                    To clear the given memslot by given the empty string:\n"
	"                    \"link nmodeat --mem2 \"\"\"\n";

static const char link_normal_mode_auto_usage_str[] =
	"Usage: link nmodeauto --read | --enable | --enable_no_rel14 | --disable\n"
	"Options:\n"
	"  -r, --read,            Read and print current setting\n"
	"  -e, --enable,          Enable autoconnect (default)\n"
	"      --enable_no_rel14, Enable autoconnect without setting Release 14 features\n"
	"  -d, --disable,         Disable autoconnect\n";

static const char link_edrx_usage_str[] =
	"Usage: link edrx --enable --ltem|--nbiot [options] | --disable\n"
	"Options:\n"
	"  -d, --disable,          Disable eDRX\n"
	"  -e, --enable,           Enable eDRX\n"
	"  -m, --ltem,             Set for LTE-M (LTE Cat-M1) system mode\n"
	"  -n, --nbiot,            Set for NB-IoT (LTE Cat-NB1) system mode\n"
	"  -x, --edrx_value, [str] Sets custom eDRX value to be requested when\n"
	"                          enabling eDRX with -e option.\n"
	"  -w, --ptw, [str]        Sets custom Paging Time Window value to be\n"
	"                          requested when enabling eDRX -e option.\n";

static const char link_psm_usage_str[] =
	"Usage: link psm --enable [options] | --disable | --read\n"
	"Options:\n"
	"  -r, --read,         Read PSM config\n"
	"  -d, --disable,      Disable PSM\n"
	"  -e, --enable,       Enable PSM\n"
	"  -p, --rptau, [str]  Sets custom requested periodic TAU value to be requested\n"
	"                      when enabling PSM -e option.\n"
	"  -t, --rat, [str]    Sets custom requested active time (RAT) value to be\n"
	"                      requested when enabling PSM -e option.\n";

static const char link_rsrp_usage_str[] =
	"Usage: link rsrp --subscribe | --unsubscribe\n"
	"Options:\n"
	"  -s, --subscribe,    Subscribe for RSRP info\n"
	"  -u, --unsubscribe,  Unsubscribe for RSRP info\n";

static const char link_search_usage_str[] =
	"Usage: link search --read | --write | --clear | --start\n"
	"\n"
	"Options:\n"
	"  -r, --read,   Read modem search configuration\n"
	"      --clear,  Clear modem search configuration\n"
	"      --write,  Write modem search configuration.\n"
	"                Use --search_cfg option to define the configuration and\n"
	"                any combination of 1 to 4 --search_pattern_range or\n"
	"                --search_pattern_table options.\n"
	"                Default values will be used if no additional parameters are provided.\n"
	"      --search_cfg, [str]\n"
	"                Search configuration in the following format (3 numbers):\n"
	"                  <loop>,<return_to_pattern>,<band_optimization>\n"
	"      --search_pattern_range, [str]\n"
	"                Range type of search pattern in the following format (4 numbers):\n"
	"                  <initial_sleep>,<final_sleep>,<time_to_final_sleep>,<pattern_end_point>\n"
	"                Set <time_to_final_sleep> to -1 if it is not given.\n"
	"      --search_pattern_table, [str]\n"
	"                Table type of search pattern in the following format (1 to 5 numbers):\n"
	"                  <val1>[,<val2>][,<val3>][,<val4>][,<val5>]\n"
	"      --start,  Start modem search request. This is an extra request outside of\n"
	"                the periodic requests. However, the search is performed only\n"
	"                when the modem is in sleep state between periodic searches.\n"
	"\n"
	"For more details on the individual parameters referred to with <param>, see:\n"
	"https://infocenter.nordicsemi.com/index.jsp?topic=%2Fref_at_commands%2FREF%2Fat_commands%2Fnw_service%2Fperiodicsearchconf_set.html\n";

static const char link_ncellmeas_usage_str[] =
	"Usage: link ncellmeas --single | --continuous [--interval <interval_in_secs>] | --cancel\n"
	"                      [--search_type <type>]\n"
	"Options:\n"
	"   --single,          Start a neighbor cell measurement and report result\n"
	"   --continuous,      Start continuous neighbor cell measurement mode and report result.\n"
	"                      New cell measurement is done everytime when current cell changes.\n"
	"   --cancel,          Cancel/Stop neighbor cell measurement if still ongoing\n"
	"   --search_type,     Used search type:\n"
	"                      'default', 'ext_light', 'ext_comp', 'gci_default', 'gci_ext_light'\n"
	"                      and 'gci_ext_comp'.\n"
	"Options for GCI search_types:\n"
	"   --gci_count, [int] Result notification for GCI (Global Cell Id) search types from\n"
	"                      gci_default to gci_ext_comp include Cell ID, PLMN and TAC for\n"
	"                      up to <gci_count> cells, and optionally list of neighbor cell\n"
	"                      measurement results related to current cell. Default 5. Range 2-15.\n"
	"Options for continuous mode:\n"
	"   --interval, [int]  Interval can be given in seconds. In addition to continuous mode\n"
	"                      functionality, new cell measurement is done in every interval.\n"
	"\n"
	"Search types explained:\n"
	"                      Default: The modem searches the network it is registered to\n"
	"                      based on previous cell history. For modem firmware\n"
	"                      versions < 1.3.1, this is the only valid option.\n"
	"\n"
	"                      Extended light: the modem starts with the same\n"
	"                      search method as the default type. If needed, it continues to\n"
	"                      search by measuring the radio conditions and makes assumptions on\n"
	"                      where networks might be deployed, i.e. a light search.\n"
	"\n"
	"                      Extended complete: the modem follows the same\n"
	"                      procedure as for type_ext_light, but will continue to perform\n"
	"                      a complete search instead of a light search, and the search is\n"
	"                      performed for all supported bands.\n"
	"\n"
	"                      GCI search, default. Modem searches EARFCNs\n"
	"                      based on previous cell history. Supported with modem firmware\n"
	"                      versions >= 1.3.4.\n"
	"\n"
	"                      GCI search, extended light. Modem starts with the same search\n"
	"                      method than in search_type gci_default.\n"
	"                      If less than <gci_count> cells were found, modem continues by\n"
	"                      performing light search on bands that are valid for the area of\n"
	"                      the current ITU-T region. Supported with modem firmware versions\n"
	"                      >= 1.3.4.\n"
	"\n"
	"                      GCI search, extended complete. Modem starts with the same search\n"
	"                      method than in search_type gci_default.\n"
	"                      If less than <gci_count> cells were found, modem performs complete\n"
	"                      search on all supported bands. Supported with modem firmware\n"
	"                      versions >= 1.3.4.\n";

static const char link_msleep_usage_str[] =
	"Usage: link msleep --subscribe [options] | --unsubscribe\n"
	"Options:\n"
	"  -u, --unsubscribe,     Unsubscribe for modem sleep notifications\n"
	"  -s, --subscribe,       Subscribe for modem sleep notifications\n"
	"      --threshold, [int] Shortest value of the duration of the scheduled modem\n"
	"                         sleep that triggers a notification. In milliseconds.\n"
	"Related static configs:\n"
	"  warn_time              Time before modem exits sleep that a pre-warning\n"
	"                         is to be received. Default value from\n"
	"                         CONFIG_LTE_LC_MODEM_SLEEP_PRE_WARNING_TIME_MS.\n";

static const char link_tau_usage_str[] =
	"Usage: link tau --subscribe [options] | --unsubscribe\n"
	"Options:\n"
	"  -u, --unsubscribe,     Unsubscribe for TAU pre-warning notifications\n"
	"  -s, --subscribe,       Subscribe for TAU pre-warning notifications\n"
	"      --threshold, [int] Minimum value of the given T3412 timer that will\n"
	"                         trigger TAU pre-warnings. In milliseconds.\n"
	"Related static configs:\n"
	"  warn_time              Time before a TAU that a pre-warning is to be received.\n"
	"                         Default value from\n"
	"                         CONFIG_LTE_LC_TAU_PRE_WARNING_TIME_MS.\n";

static const char link_rai_usage_str[] =
	"Usage: link rai --enable | --disable\n"
	"Options:\n"
	"  -r, --read,         Read current RAI status\n"
	"  -d, --disable,      Disable RAI\n"
	"  -e, --enable,       Enable RAI\n";

static const char link_dnsaddr_usage_str[] =
	"Usage: link dnsaddr [options] | --read | --enable | --disable\n"
	"Options:\n"
	"  -r, --read,         Read and print current config\n"
	"  -d, --disable,      Disable manual DNS server address\n"
	"  -e, --enable,       Enable manual DNS server address\n"
	"  -i, --ipaddr, [str] DNS server IP address\n";

static const char link_redmob_usage_str[] =
	"Usage: link redmob [options] | --read\n"
	"Options:\n"
	"  -r, --read,         Read and print current mode\n"
	"  -d, --disable,      Disable reduced mobility mode\n"
	"      --default,      Enable default reduced mobility mode\n"
	"      --nordic,       Enable Nordic proprietary reduced mobility mode\n";

/******************************************************************************/

/* Following are not having short options: */
enum {
	LINK_SHELL_OPT_MEM_SLOT_1 = 1001,
	LINK_SHELL_OPT_MEM_SLOT_2,
	LINK_SHELL_OPT_MEM_SLOT_3,
	LINK_SHELL_OPT_RESET,
	LINK_SHELL_OPT_MRESET_ALL,
	LINK_SHELL_OPT_MRESET_USER,
	LINK_SHELL_OPT_SYSMODE_LTEM_NBIOT,
	LINK_SHELL_OPT_SYSMODE_LTEM_NBIOT_GNSS,
	LINK_SHELL_OPT_SYSMODE_PREF_AUTO,
	LINK_SHELL_OPT_SYSMODE_PREF_LTEM,
	LINK_SHELL_OPT_SYSMODE_PREF_NBIOT,
	LINK_SHELL_OPT_SYSMODE_PREF_LTEM_PLMN_PRIO,
	LINK_SHELL_OPT_SYSMODE_PREF_NBIOT_PLMN_PRIO,
	LINK_SHELL_OPT_FUNMODE_LTEOFF,
	LINK_SHELL_OPT_FUNMODE_LTEON,
	LINK_SHELL_OPT_FUNMODE_GNSSOFF,
	LINK_SHELL_OPT_FUNMODE_GNSSON,
	LINK_SHELL_OPT_FUNMODE_UICCOFF,
	LINK_SHELL_OPT_FUNMODE_UICCON,
	LINK_SHELL_OPT_FUNMODE_FLIGHTMODE_UICCON,
	LINK_SHELL_OPT_THRESHOLD_TIME,
	LINK_SHELL_OPT_START,
	LINK_SHELL_OPT_STOP,
	LINK_SHELL_OPT_SINGLE,
	LINK_SHELL_OPT_CONTINUOUS,
	LINK_SHELL_OPT_NCELLMEAS_SEARCH_TYPE,
	LINK_SHELL_OPT_NCELLMEAS_CONTINUOUS_INTERVAL_TIME,
	LINK_SHELL_OPT_NCELLMEAS_GCI_COUNT,
	LINK_SHELL_OPT_SEARCH_CFG,
	LINK_SHELL_OPT_SEARCH_PATTERN_RANGE,
	LINK_SHELL_OPT_SEARCH_PATTERN_TABLE,
	LINK_SHELL_OPT_NMODE_NO_REL14,
	LINK_SHELL_OPT_WRITE,
	LINK_SHELL_OPT_REDMOB_DEFAULT,
	LINK_SHELL_OPT_REDMOB_NORDIC,
};

/* Specifying the expected options (both long and short): */
static struct option long_options[] = {
	{ "apn", required_argument, 0, 'a' },
	{ "cid", required_argument, 0, 'I' },
	{ "family", required_argument, 0, 'f' },
	{ "ipaddr", required_argument, 0, 'i' },
	{ "subscribe", no_argument, 0, 's' },
	{ "unsubscribe", no_argument, 0, 'u' },
	{ "read", no_argument, 0, 'r' },
	{ "write", no_argument, 0, LINK_SHELL_OPT_WRITE },
	{ "pwroff", no_argument, 0, '0' },
	{ "normal", no_argument, 0, '1' },
	{ "flightmode", no_argument, 0, '4' },
	{ "lteoff", no_argument, 0, LINK_SHELL_OPT_FUNMODE_LTEOFF },
	{ "lteon", no_argument, 0, LINK_SHELL_OPT_FUNMODE_LTEON },
	{ "gnssoff", no_argument, 0, LINK_SHELL_OPT_FUNMODE_GNSSOFF },
	{ "gnsson", no_argument, 0, LINK_SHELL_OPT_FUNMODE_GNSSON },
	{ "uiccoff", no_argument, 0, LINK_SHELL_OPT_FUNMODE_UICCOFF },
	{ "uiccon", no_argument, 0, LINK_SHELL_OPT_FUNMODE_UICCON },
	{ "flightmode_uiccon", no_argument, 0, LINK_SHELL_OPT_FUNMODE_FLIGHTMODE_UICCON },
	{ "ltem", no_argument, 0, 'm' },
	{ "nbiot", no_argument, 0, 'n' },
	{ "gnss", no_argument, 0, 'g' },
	{ "ltem_gnss", no_argument, 0, 'M' },
	{ "nbiot_gnss", no_argument, 0, 'N' },
	{ "enable", no_argument, 0, 'e' },
	{ "enable_no_rel14", no_argument, 0, LINK_SHELL_OPT_NMODE_NO_REL14 },
	{ "disable", no_argument, 0, 'd' },
	{ "edrx_value", required_argument, 0, 'x' },
	{ "ptw", required_argument, 0, 'w' },
	{ "prot", required_argument, 0, 'A' },
	{ "pword", required_argument, 0, 'P' },
	{ "uname", required_argument, 0, 'U' },
	{ "rptau", required_argument, 0, 'p' },
	{ "rat", required_argument, 0, 't' },
	{ "mem1", required_argument, 0, LINK_SHELL_OPT_MEM_SLOT_1 },
	{ "mem2", required_argument, 0, LINK_SHELL_OPT_MEM_SLOT_2 },
	{ "mem3", required_argument, 0, LINK_SHELL_OPT_MEM_SLOT_3 },
	{ "reset", no_argument, 0, LINK_SHELL_OPT_RESET },
	{ "clear", no_argument, 0, LINK_SHELL_OPT_RESET },
	{ "mreset_all", no_argument, 0, LINK_SHELL_OPT_MRESET_ALL },
	{ "mreset_user", no_argument, 0, LINK_SHELL_OPT_MRESET_USER },
	{ "ltem_nbiot", no_argument, 0, LINK_SHELL_OPT_SYSMODE_LTEM_NBIOT },
	{ "ltem_nbiot_gnss", no_argument, 0, LINK_SHELL_OPT_SYSMODE_LTEM_NBIOT_GNSS },
	{ "pref_auto", no_argument, 0, LINK_SHELL_OPT_SYSMODE_PREF_AUTO },
	{ "pref_ltem", no_argument, 0, LINK_SHELL_OPT_SYSMODE_PREF_LTEM },
	{ "pref_nbiot", no_argument, 0, LINK_SHELL_OPT_SYSMODE_PREF_NBIOT },
	{ "pref_ltem_plmn_prio", no_argument, 0, LINK_SHELL_OPT_SYSMODE_PREF_LTEM_PLMN_PRIO },
	{ "pref_nbiot_plmn_prio", no_argument, 0, LINK_SHELL_OPT_SYSMODE_PREF_NBIOT_PLMN_PRIO },
	{ "start", no_argument, 0, LINK_SHELL_OPT_START },
	{ "stop", no_argument, 0, LINK_SHELL_OPT_STOP },
	{ "cancel", no_argument, 0, LINK_SHELL_OPT_STOP },
	{ "single", no_argument, 0, LINK_SHELL_OPT_SINGLE },
	{ "continuous", no_argument, 0, LINK_SHELL_OPT_CONTINUOUS },
	{ "threshold", required_argument, 0, LINK_SHELL_OPT_THRESHOLD_TIME },
	{ "interval", required_argument, 0, LINK_SHELL_OPT_NCELLMEAS_CONTINUOUS_INTERVAL_TIME },
	{ "gci_count", required_argument, 0, LINK_SHELL_OPT_NCELLMEAS_GCI_COUNT },
	{ "search_type", required_argument, 0, LINK_SHELL_OPT_NCELLMEAS_SEARCH_TYPE },
	{ "search_cfg", required_argument, 0, LINK_SHELL_OPT_SEARCH_CFG },
	{ "search_pattern_range", required_argument, 0, LINK_SHELL_OPT_SEARCH_PATTERN_RANGE },
	{ "search_pattern_table", required_argument, 0, LINK_SHELL_OPT_SEARCH_PATTERN_TABLE },
	{ "normal_no_rel14", no_argument, 0, LINK_SHELL_OPT_NMODE_NO_REL14 },
	{ "default", no_argument, 0, LINK_SHELL_OPT_REDMOB_DEFAULT },
	{ "nordic", no_argument, 0, LINK_SHELL_OPT_REDMOB_NORDIC },
	{ 0, 0, 0, 0 }
};

static const char short_options[] = "a:I:f:i:x:w:p:t:A:P:U:su014rmngMNed";

/******************************************************************************/

bool link_shell_msleep_notifications_subscribed;

static void link_shell_print_usage(enum link_shell_command command)
{
	switch (command) {
	case LINK_CMD_SETTINGS:
		mosh_print_no_format(link_settings_usage_str);
		break;
	case LINK_CMD_DEFCONT:
		mosh_print_no_format(link_defcont_usage_str);
		break;
	case LINK_CMD_DEFCONTAUTH:
		mosh_print_no_format(link_defcontauth_usage_str);
		break;
	case LINK_CMD_CONNECT:
		mosh_print_no_format(link_connect_usage_str);
		break;
	case LINK_CMD_DISCONNECT:
		mosh_print_no_format(link_disconnect_usage_str);
		break;
	case LINK_CMD_SYSMODE:
		mosh_print_no_format(link_sysmode_usage_str);
		break;
	case LINK_CMD_FUNMODE:
		mosh_print_no_format(link_funmode_usage_str);
		break;
	case LINK_CMD_NORMAL_MODE_AT:
		mosh_print_no_format(link_normal_mode_at_usage_str);
		break;
	case LINK_CMD_NORMAL_MODE_AUTO:
		mosh_print_no_format(link_normal_mode_auto_usage_str);
		break;
	case LINK_CMD_EDRX:
		mosh_print_no_format(link_edrx_usage_str);
		break;
	case LINK_CMD_PSM:
		mosh_print_no_format(link_psm_usage_str);
		break;
	case LINK_CMD_RSRP:
		mosh_print_no_format(link_rsrp_usage_str);
		break;
	case LINK_CMD_SEARCH:
		mosh_print_no_format(link_search_usage_str);
		break;
	case LINK_CMD_NCELLMEAS:
		mosh_print_no_format(link_ncellmeas_usage_str);
		break;
	case LINK_CMD_MDMSLEEP:
		mosh_print_no_format(link_msleep_usage_str);
		break;
	case LINK_CMD_TAU:
		mosh_print_no_format(link_tau_usage_str);
		break;
	case LINK_CMD_RAI:
		mosh_print_no_format(link_rai_usage_str);
		break;
	case LINK_CMD_DNSADDR:
		mosh_print_no_format(link_dnsaddr_usage_str);
		break;
	case LINK_CMD_REDMOB:
		mosh_print_no_format(link_redmob_usage_str);
		break;
	default:
		break;
	}
}

/******************************************************************************/

/* From lte_lc.c, and TODO: to be updated if something is added */
#define SYS_MODE_PREFERRED					   \
	(IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M)              ?  \
	 LTE_LC_SYSTEM_MODE_LTEM                         :	   \
	 IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT)               ? \
	 LTE_LC_SYSTEM_MODE_NBIOT                        :	   \
	 IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_GPS)           ? \
	 LTE_LC_SYSTEM_MODE_LTEM_GPS                     :	   \
	 IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT_GPS)           ? \
	 LTE_LC_SYSTEM_MODE_NBIOT_GPS                    :	   \
	 IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT)         ? \
	 LTE_LC_SYSTEM_MODE_LTEM_NBIOT                   :	   \
	 IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS)     ? \
	 LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS               :	   \
	 LINK_SYSMODE_NONE)

static void link_shell_sysmode_set(int sysmode, int lte_pref)
{
	enum lte_lc_func_mode functional_mode;
	char snum[64];
	int ret = lte_lc_system_mode_set(sysmode, lte_pref);

	if (ret < 0) {
		mosh_error("Cannot set system mode to modem: %d", ret);
		ret = lte_lc_func_mode_get(&functional_mode);
		if (ret == 0 &&
		    (functional_mode != LTE_LC_FUNC_MODE_OFFLINE &&
		     functional_mode != LTE_LC_FUNC_MODE_POWER_OFF)) {
			mosh_warn(
				"Requested mode couldn't set to modem. "
				"Not in flighmode nor in pwroff?");
		}
	} else {
		mosh_print(
			"System mode set successfully to modem: %s",
			link_shell_sysmode_to_string(sysmode, snum));
	}
}

/******************************************************************************/

#define MOSH_NCELLMEAS_SEARCH_TYPE_NONE 0xFF

static enum lte_lc_neighbor_search_type
	link_shell_string_to_ncellmeas_search_type(const char *search_type_str)
{
	enum lte_lc_neighbor_search_type search_type = MOSH_NCELLMEAS_SEARCH_TYPE_NONE;

	if (strcmp(search_type_str, "default") == 0) {
		search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT;
	} else if (strcmp(search_type_str, "ext_light") == 0) {
		search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_LIGHT;
	} else if (strcmp(search_type_str, "ext_comp") == 0) {
		search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_COMPLETE;
	} else if (strcmp(search_type_str, "gci_default") == 0) {
		search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_DEFAULT;
	} else if (strcmp(search_type_str, "gci_ext_light") == 0) {
		search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_LIGHT;
	} else if (strcmp(search_type_str, "gci_ext_comp") == 0) {
		search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_COMPLETE;
	}

	return search_type;
}

/******************************************************************************/

int link_shell_get_and_print_current_system_modes(
	enum lte_lc_system_mode *sys_mode_current,
	enum lte_lc_system_mode_preference *sys_mode_preferred,
	enum lte_lc_lte_mode *currently_active_mode)
{
	int ret = -1;

	char snum[64];

	ret = lte_lc_system_mode_get(sys_mode_current, sys_mode_preferred);
	if (ret >= 0) {
		mosh_print(
			"Modem config for system mode: %s",
			link_shell_sysmode_to_string(*sys_mode_current, snum));
		mosh_print(
			"Modem config for LTE preference: %s",
			link_shell_sysmode_preferred_to_string(*sys_mode_preferred, snum));
	} else {
		return ret;
	}

	ret = lte_lc_lte_mode_get(currently_active_mode);
	if (ret >= 0) {
		mosh_print(
			"Currently active system mode: %s",
			link_shell_sysmode_currently_active_to_string(
				*currently_active_mode, snum));
	}
	return ret;
}

static void link_shell_getopt_common_option(int opt, enum link_shell_common_options *common_option)
{
	switch (opt) {
	case 's':
		*common_option = LINK_COMMON_SUBSCRIBE;
		break;
	case 'u':
		*common_option = LINK_COMMON_UNSUBSCRIBE;
		break;
	case 'e':
		*common_option = LINK_COMMON_ENABLE;
		break;
	case 'd':
		*common_option = LINK_COMMON_DISABLE;
		break;
	case 'r':
		*common_option = LINK_COMMON_READ;
		break;
	case LINK_SHELL_OPT_WRITE:
		*common_option = LINK_COMMON_WRITE;
		break;
	case LINK_SHELL_OPT_RESET:
		*common_option = LINK_COMMON_RESET;
		break;
	case LINK_SHELL_OPT_START:
		*common_option = LINK_COMMON_START;
		break;
	case LINK_SHELL_OPT_STOP:
		*common_option = LINK_COMMON_STOP;
		break;
	default:
		break;
	}
}

static enum link_shell_common_options link_shell_getopt_common(int argc, char *const argv[])
{
	enum link_shell_common_options common_option = LINK_COMMON_NONE;

	int long_index = 0;
	int opt;

	/* Reset getopt due to possible previous failures */
	optreset = 1;
	/* We start from subcmd arguments */
	optind = 1;

	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		link_shell_getopt_common_option(opt, &common_option);
	}
	return common_option;
}

static int link_shell_connect(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	char *apn = NULL;
	int apn_len = 0;
	char *family = NULL;
	char *ip_address = NULL;
	int protocol = 0;
	bool protocol_given = false;
	char *username = NULL;
	char *password = NULL;

	if (argc < 2) {
		goto show_usage;
	}

	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		switch (opt) {
		case 'a': /* APN */
			apn_len = strlen(optarg);
			if (apn_len > LINK_APN_STR_MAX_LENGTH) {
				mosh_error(
					"APN string length %d exceeded. Maximum is %d.",
					apn_len, LINK_APN_STR_MAX_LENGTH);
				return -EINVAL;
			}
			apn = optarg;
			break;
		case 'f': /* Address family */
			family = optarg;
			break;
		case 'i': /* IP address */
			ip_address = optarg;
			break;
		case 'A': /* auth protocol */
			protocol = atoi(optarg);
			protocol_given = true;
			break;
		case 'U': /* auth username */
			username = optarg;
			break;
		case 'P': /* auth password */
			password = optarg;
			break;
		case '?':
			goto show_usage;
		default:
			mosh_error("Unknown option. See usage:");
			goto show_usage;
		}
	}

	/* Check that all mandatory args were given */
	if (apn == NULL) {
		mosh_error("Option -a | -apn MUST be given. See usage:");
		goto show_usage;
	}

	struct link_shell_pdn_auth auth_params;
	struct link_shell_pdn_auth *auth_params_ptr = NULL;

	if ((protocol_given &&
		(username == NULL || password == NULL)) ||
		((username != NULL || password != NULL) &&
		!protocol_given)) {
		mosh_error("When setting authentication, all auth options must be given");
		goto show_usage;
	} else {
		enum pdn_auth method;

		ret = link_shell_pdn_auth_prot_to_pdn_lib_method_map(
			protocol, &method);
		if (ret) {
			mosh_error("Unknown auth protocol %d", protocol);
			goto show_usage;
		}
		auth_params.method = method;
		auth_params.user = username;
		auth_params.password = password;
		if (protocol_given && username != NULL &&
			password != NULL) {
			auth_params_ptr = &auth_params;
		}
	}

	ret = link_shell_pdn_connect(apn, family, auth_params_ptr);

	return ret;

show_usage:
	link_shell_print_usage(LINK_CMD_CONNECT);
	return ret;
}

static int link_shell_coneval(const struct shell *shell, size_t argc, char **argv)
{
	link_api_coneval_read_for_shell();

	return 0;
}

static int link_shell_defcont(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	enum link_shell_common_options common_option = LINK_COMMON_NONE;
	int apn_len = 0;
	char *apn = NULL;
	char *family = NULL;

	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		link_shell_getopt_common_option(opt, &common_option);

		switch (opt) {
		case 'a': /* APN */
			apn_len = strlen(optarg);
			if (apn_len > LINK_APN_STR_MAX_LENGTH) {
				mosh_error(
					"APN string length %d exceeded. Maximum is %d.",
					apn_len, LINK_APN_STR_MAX_LENGTH);
				return -EINVAL;
			}
			apn = optarg;
			break;
		case 'f': /* Address family */
			family = optarg;
			break;
		default:
			break;
		}
	}

	if (common_option == LINK_COMMON_READ) {
		link_sett_defcont_conf_shell_print();
	} else if (common_option == LINK_COMMON_ENABLE) {
		link_sett_save_defcont_enabled(true);
	} else if (common_option == LINK_COMMON_DISABLE) {
		if (nrf_modem_at_printf("AT+CGDCONT=0") != 0) {
			mosh_warn(
				"ERROR from modem. Getting the initial PDP context back "
				"wasn't successful.");
		}
		link_sett_save_defcont_enabled(false);
		mosh_print("Custom default context config disabled.");
	} else if (apn == NULL && family == NULL) {
		link_shell_print_usage(LINK_CMD_DEFCONT);
	}

	if (apn != NULL) {
		(void)link_sett_save_defcont_apn(apn);
	}
	if (family != NULL) {
		enum pdn_fam pdn_lib_fam;

		ret = link_family_str_to_pdn_lib_family(&pdn_lib_fam, family);
		if (ret) {
			mosh_error("Unknown PDN family %s", family);
			link_shell_print_usage(LINK_CMD_DEFCONT);
		} else {
			(void)link_sett_save_defcont_pdn_family(pdn_lib_fam);
		}
	}

	return 0;
}

static int link_shell_defcontauth(const struct shell *shell, size_t argc, char **argv)
{
	enum link_shell_common_options common_option = LINK_COMMON_NONE;
	int protocol = 0;
	bool protocol_given = false;
	char *username = NULL;
	char *password = NULL;

	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		link_shell_getopt_common_option(opt, &common_option);

		switch (opt) {
		case 'A': /* auth protocol */
			protocol = atoi(optarg);
			protocol_given = true;
			break;
		case 'U': /* auth username */
			username = optarg;
			break;
		case 'P': /* auth password */
			password = optarg;
			break;
		default:
			break;
		}
	}

	if (common_option == LINK_COMMON_READ) {
		link_sett_defcontauth_conf_shell_print();
	} else if (common_option == LINK_COMMON_ENABLE) {
		if (link_sett_save_defcontauth_enabled(true) < 0) {
			mosh_warn("Cannot enable authentication.");
		}
	} else if (common_option == LINK_COMMON_DISABLE) {
		if (nrf_modem_at_printf("AT+CGAUTH=0,0") != 0) {
			mosh_warn("Disabling of auth cannot be done to modem.");
		}
		link_sett_save_defcontauth_enabled(false);
	} else if (!protocol_given && username == NULL && password == NULL) {

		link_shell_print_usage(LINK_CMD_DEFCONTAUTH);
	}

	if (protocol_given) {
		(void)link_sett_save_defcontauth_prot(protocol);
	}
	if (username != NULL) {
		(void)link_sett_save_defcontauth_username(username);
	}
	if (password != NULL) {
		(void)link_sett_save_defcontauth_password(password);
	}

	return 0;
}

static int link_shell_disconnect(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	int pdn_cid = 0;

	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		switch (opt) {
		case 'I': /* PDN CID */
			pdn_cid = atoi(optarg);
			if (pdn_cid <= 0) {
				mosh_error(
					"PDN CID (%d) must be positive integer. "
					"Default PDN context (CID=0) cannot be given.",
					pdn_cid);
				return -EINVAL;
			}
			break;
		default:
			break;
		}
	}

	if (pdn_cid == 0) {
		link_shell_print_usage(LINK_CMD_DISCONNECT);
		return 0;
	}

	ret = link_shell_pdn_disconnect(pdn_cid);

	return ret;
}

static int link_shell_dnsaddr(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	enum link_shell_common_options common_option = LINK_COMMON_NONE;
	char *ip_address = NULL;

	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		link_shell_getopt_common_option(opt, &common_option);

		switch (opt) {
		case 'i':
			ip_address = optarg;
			break;
		default:
			break;
		}
	}

	if (common_option == LINK_COMMON_READ) {
		link_sett_dnsaddr_conf_shell_print();
		return 0;
	}

	if (ip_address && strlen(ip_address) > 0 &&
		net_utils_ip_string_is_valid(ip_address) == false) {

		mosh_error("Invalid IP address: %s", ip_address);
		return -EINVAL;
	}

	if (common_option == LINK_COMMON_ENABLE) {
		link_sett_save_dnsaddr_enabled(true);
	} else if (common_option == LINK_COMMON_DISABLE) {
		link_sett_save_dnsaddr_enabled(false);
	} else if (ip_address == NULL) {
		link_shell_print_usage(LINK_CMD_DNSADDR);
	}

	ret = link_setdnsaddr(ip_address ? ip_address : link_sett_dnsaddr_ip_get());

	if (ip_address) {
		link_sett_save_dnsaddr_ip(ip_address);
	}

	return ret;
}

static int link_shell_edrx(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	enum link_shell_common_options common_option = LINK_COMMON_NONE;
	enum lte_lc_lte_mode lte_mode = LTE_LC_LTE_MODE_NONE;
	char edrx_value_str[LINK_SHELL_EDRX_VALUE_STR_LENGTH + 1];
	bool edrx_value_set = false;
	char edrx_ptw_bit_str[LINK_SHELL_EDRX_PTW_STR_LENGTH + 1];
	bool edrx_ptw_set = false;

	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		link_shell_getopt_common_option(opt, &common_option);

		switch (opt) {
		case 'm':
			lte_mode = LTE_LC_LTE_MODE_LTEM;
			break;
		case 'n':
			lte_mode = LTE_LC_LTE_MODE_NBIOT;
			break;
		case 'x': /* drx_value */
			if (strlen(optarg) ==
			    LINK_SHELL_EDRX_VALUE_STR_LENGTH) {
				strcpy(edrx_value_str, optarg);
				edrx_value_set = true;
			} else {
				mosh_error(
					"eDRX value string length must be %d.",
					LINK_SHELL_EDRX_VALUE_STR_LENGTH);
				return -EINVAL;
			}
			break;
		case 'w': /* Paging Time Window */
			if (strlen(optarg) == LINK_SHELL_EDRX_PTW_STR_LENGTH) {
				strcpy(edrx_ptw_bit_str, optarg);
				edrx_ptw_set = true;
			} else {
				mosh_error(
					"PTW string length must be %d.",
					LINK_SHELL_EDRX_PTW_STR_LENGTH);
				return -EINVAL;
			}
			break;
		default:
			break;
		}
	}

	if (common_option == LINK_COMMON_ENABLE) {
		char *value = NULL; /* Set with the defaults if not given */

		if (lte_mode == LTE_LC_LTE_MODE_NONE) {
			mosh_error("LTE mode is mandatory to be given. See usage:");
			link_shell_print_usage(LINK_CMD_EDRX);
			return -EINVAL;
		}

		if (edrx_value_set) {
			value = edrx_value_str;
		}

		ret = lte_lc_edrx_param_set(lte_mode, value);
		if (ret < 0) {
			mosh_error(
				"Cannot set eDRX value %s, error: %d",
				((value == NULL) ? "NULL" : value),
				ret);
			return -EINVAL;
		}
		value = NULL; /* Set with the defaults if not given */
		if (edrx_ptw_set) {
			value = edrx_ptw_bit_str;
		}

		ret = lte_lc_ptw_set(lte_mode, value);
		if (ret < 0) {
			mosh_error(
				"Cannot set PTW value %s, error: %d",
				((value == NULL) ? "NULL" : value),
				ret);
			return -EINVAL;
		}

		ret = lte_lc_edrx_req(true);
		if (ret < 0) {
			mosh_error("Cannot enable eDRX: %d", ret);
		} else {
			mosh_print("eDRX enabled");
		}
	} else if (common_option == LINK_COMMON_DISABLE) {
		ret = lte_lc_edrx_req(false);
		if (ret < 0) {
			mosh_error("Cannot disable eDRX: %d", ret);
		} else {
			mosh_print("eDRX disabled");
		}
	} else {
		link_shell_print_usage(LINK_CMD_EDRX);
	}

	return 0;
}

static int link_shell_funmode(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	bool nmode_use_rel14 = true;
	enum lte_lc_func_mode funmode_option = LINK_FUNMODE_NONE;
	enum lte_lc_func_mode functional_mode;
	enum link_shell_common_options common_option = LINK_COMMON_NONE;
	char snum[64];

	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		link_shell_getopt_common_option(opt, &common_option);

		switch (opt) {
		case '0':
			funmode_option = LTE_LC_FUNC_MODE_POWER_OFF;
			break;
		case '1':
			funmode_option = LTE_LC_FUNC_MODE_NORMAL;
			break;
		case '4':
			funmode_option = LTE_LC_FUNC_MODE_OFFLINE;
			break;
		case LINK_SHELL_OPT_FUNMODE_LTEOFF:
			funmode_option = LTE_LC_FUNC_MODE_DEACTIVATE_LTE;
			break;
		case LINK_SHELL_OPT_FUNMODE_LTEON:
			funmode_option = LTE_LC_FUNC_MODE_ACTIVATE_LTE;
			break;
		case LINK_SHELL_OPT_FUNMODE_GNSSOFF:
			funmode_option = LTE_LC_FUNC_MODE_DEACTIVATE_GNSS;
			break;
		case LINK_SHELL_OPT_FUNMODE_GNSSON:
			funmode_option = LTE_LC_FUNC_MODE_ACTIVATE_GNSS;
			break;
		case LINK_SHELL_OPT_FUNMODE_UICCOFF:
			funmode_option = LTE_LC_FUNC_MODE_DEACTIVATE_UICC;
			break;
		case LINK_SHELL_OPT_FUNMODE_UICCON:
			funmode_option = LTE_LC_FUNC_MODE_ACTIVATE_UICC;
			break;
		case LINK_SHELL_OPT_FUNMODE_FLIGHTMODE_UICCON:
			funmode_option = LTE_LC_FUNC_MODE_OFFLINE_UICC_ON;
			break;
		case LINK_SHELL_OPT_NMODE_NO_REL14:
			funmode_option = LTE_LC_FUNC_MODE_NORMAL;
			nmode_use_rel14 = false;
			break;
		default:
			break;
		}
	}

	if (common_option == LINK_COMMON_READ) {
		ret = lte_lc_func_mode_get(&functional_mode);
		if (ret) {
			mosh_error("Cannot get functional mode: %d", ret);
		} else {
			mosh_print(
				"Functional mode read successfully: %s",
				link_shell_funmode_to_string(functional_mode, snum));
		}
	} else if (funmode_option != LINK_FUNMODE_NONE) {
		ret = link_func_mode_set(funmode_option, nmode_use_rel14);
		if (ret < 0) {
			mosh_error("Cannot set functional mode: %d", ret);
		} else {
			mosh_print(
				"Functional mode set successfully: %s",
				link_shell_funmode_to_string(funmode_option, snum));
		}
	} else {
		link_shell_print_usage(LINK_CMD_FUNMODE);
	}

	return 0;
}

static int link_shell_msleep(const struct shell *shell, size_t argc, char **argv)
{
	enum link_shell_common_options common_option = LINK_COMMON_NONE;
	int threshold_time = 0;

	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		link_shell_getopt_common_option(opt, &common_option);

		switch (opt) {
		case LINK_SHELL_OPT_THRESHOLD_TIME:
			threshold_time = atoi(optarg);
			if (threshold_time <= 0) {
				mosh_error(
					"Not a valid number for --threshold_time (milliseconds).");
				return -EINVAL;
			}
			break;
		default:
			break;
		}
	}

	if (common_option == LINK_COMMON_SUBSCRIBE) {
		link_modem_sleep_notifications_subscribe(
			CONFIG_LTE_LC_MODEM_SLEEP_PRE_WARNING_TIME_MS,
			((threshold_time) ?
				threshold_time :
				CONFIG_LTE_LC_MODEM_SLEEP_NOTIFICATIONS_THRESHOLD_MS));
	} else if (common_option == LINK_COMMON_UNSUBSCRIBE) {
		link_modem_sleep_notifications_unsubscribe();
	} else {
		link_shell_print_usage(LINK_CMD_MDMSLEEP);
	}

	return 0;
}

static int link_shell_ncellmeas(const struct shell *shell, size_t argc, char **argv)
{
	struct lte_lc_ncellmeas_params ncellmeas_params = {
		.gci_count = 5,
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT,
	};
	enum link_shell_common_options common_option = LINK_COMMON_NONE;
	enum link_ncellmeas_modes ncellmeasmode = LINK_NCELLMEAS_MODE_NONE;
	enum lte_lc_neighbor_search_type ncellmeas_search_type =
		LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT;
	int periodic_time = 0;
	bool periodic_time_given = false;
	int gci_count;

	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		link_shell_getopt_common_option(opt, &common_option);

		switch (opt) {
		case LINK_SHELL_OPT_SINGLE:
			ncellmeasmode = LINK_NCELLMEAS_MODE_SINGLE;
			break;
		case LINK_SHELL_OPT_CONTINUOUS:
			ncellmeasmode = LINK_NCELLMEAS_MODE_CONTINUOUS;
			break;
		case LINK_SHELL_OPT_NCELLMEAS_SEARCH_TYPE:
			ncellmeas_search_type = link_shell_string_to_ncellmeas_search_type(optarg);
			if (ncellmeas_search_type == MOSH_NCELLMEAS_SEARCH_TYPE_NONE) {
				mosh_error("Unknown search_type. See usage:");
				link_shell_print_usage(LINK_CMD_NCELLMEAS);
			}
			break;
		case LINK_SHELL_OPT_NCELLMEAS_GCI_COUNT:
			gci_count = atoi(optarg);
			if (gci_count <= 0) {
				mosh_error("Not a valid number for --gci_count.");
				return -EINVAL;
			}
			ncellmeas_params.gci_count = gci_count;
			break;
		case LINK_SHELL_OPT_NCELLMEAS_CONTINUOUS_INTERVAL_TIME: {
			char *end_ptr;

			periodic_time = strtol(optarg, &end_ptr, 10);
			if (end_ptr == optarg || periodic_time < 0) {
				mosh_error("Not a valid number for --interval (seconds).");
				return -EINVAL;
			}
			periodic_time_given = true;
			break;
		}
		default:
			break;
		}
	}

	if (common_option == LINK_COMMON_STOP) {
		link_ncellmeas_start(false, LINK_NCELLMEAS_MODE_NONE, ncellmeas_params, 0, false);
	} else if (ncellmeasmode != LINK_NCELLMEAS_MODE_NONE) {
		mosh_print("Neighbor cell measurements and reporting starting");
		ncellmeas_params.search_type = ncellmeas_search_type;
		link_ncellmeas_start(
			true, ncellmeasmode, ncellmeas_params, periodic_time, periodic_time_given);
	} else {
		link_shell_print_usage(LINK_CMD_NCELLMEAS);
	}

	return 0;
}

static int link_shell_nmodeat(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	enum link_shell_common_options common_option = LINK_COMMON_NONE;
	char *normal_mode_at_str = NULL;
	uint8_t normal_mode_at_mem_slot = 0;

	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		link_shell_getopt_common_option(opt, &common_option);

		switch (opt) {
		case LINK_SHELL_OPT_MEM_SLOT_1:
			normal_mode_at_str = optarg;
			normal_mode_at_mem_slot = 1;
			break;
		case LINK_SHELL_OPT_MEM_SLOT_2:
			normal_mode_at_str = optarg;
			normal_mode_at_mem_slot = 2;
			break;
		case LINK_SHELL_OPT_MEM_SLOT_3:
			normal_mode_at_str = optarg;
			normal_mode_at_mem_slot = 3;
			break;
		default:
			break;
		}
	}

	if (common_option == LINK_COMMON_READ) {
		link_sett_normal_mode_at_cmds_shell_print();
	} else if (normal_mode_at_str != NULL) {
		ret = link_sett_save_normal_mode_at_cmd_str(
			normal_mode_at_str, normal_mode_at_mem_slot);
		if (ret < 0) {
			mosh_error("Cannot set normal mode AT-command: \"%s\"", normal_mode_at_str);
		} else {
			mosh_print(
				"Normal mode AT-command \"%s\" set successfully to memory slot %d.",
				((strlen(normal_mode_at_str)) ?
					normal_mode_at_str :
					"<empty>"),
				normal_mode_at_mem_slot);
		}
	} else {
		link_shell_print_usage(LINK_CMD_NORMAL_MODE_AT);
	}

	return 0;
}

static int link_shell_nmodeauto(const struct shell *shell, size_t argc, char **argv)
{
	enum link_shell_common_options common_option = LINK_COMMON_NONE;
	bool nmode_use_rel14 = true;

	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		link_shell_getopt_common_option(opt, &common_option);

		switch (opt) {
		case LINK_SHELL_OPT_NMODE_NO_REL14:
			common_option = LINK_COMMON_ENABLE;
			nmode_use_rel14 = false;
			break;
		default:
			break;
		}
	}

	if (common_option == LINK_COMMON_READ) {
		link_sett_normal_mode_autoconn_shell_print();
	} else if (common_option == LINK_COMMON_ENABLE) {
		link_sett_save_normal_mode_autoconn_enabled(true, nmode_use_rel14);
	} else if (common_option == LINK_COMMON_DISABLE) {
		link_sett_save_normal_mode_autoconn_enabled(false, nmode_use_rel14);
	} else {
		link_shell_print_usage(LINK_CMD_NORMAL_MODE_AUTO);
	}

	return 0;
}

static int link_shell_psm(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	enum link_shell_common_options common_option = LINK_COMMON_NONE;
	char psm_rptau_bit_str[LINK_SHELL_PSM_PARAM_STR_LENGTH + 1];
	bool psm_rptau_set = false;
	char psm_rat_bit_str[LINK_SHELL_PSM_PARAM_STR_LENGTH + 1];
	bool psm_rat_set = false;

	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		link_shell_getopt_common_option(opt, &common_option);

		switch (opt) {
		case 'p': /* rptau */
			if (strlen(optarg) ==
			    LINK_SHELL_PSM_PARAM_STR_LENGTH) {
				strcpy(psm_rptau_bit_str, optarg);
				psm_rptau_set = true;
			} else {
				mosh_error(
					"RPTAU bit string length must be %d.",
					LINK_SHELL_PSM_PARAM_STR_LENGTH);
				return -EINVAL;
			}
			break;
		case 't': /* rat */
			if (strlen(optarg) ==
			    LINK_SHELL_PSM_PARAM_STR_LENGTH) {
				strcpy(psm_rat_bit_str, optarg);
				psm_rat_set = true;
			} else {
				mosh_error(
					"RAT bit string length must be %d.",
					LINK_SHELL_PSM_PARAM_STR_LENGTH);
				return -EINVAL;
			}
			break;
		default:
			break;
		}
	}

	if (common_option == LINK_COMMON_ENABLE) {
		/* Set with the defaults if not given */
		char *rptau_bit_value = NULL;
		char *rat_bit_value = NULL;

		if (psm_rptau_set) {
			rptau_bit_value = psm_rptau_bit_str;
		}

		if (psm_rat_set) {
			rat_bit_value = psm_rat_bit_str;
		}

		ret = lte_lc_psm_param_set(rptau_bit_value,
						rat_bit_value);
		if (ret < 0) {
			mosh_error("Cannot set PSM parameters: error %d", ret);
			mosh_error(
				"  rptau %s, rat %s",
				((rptau_bit_value == NULL) ? "NULL" : rptau_bit_value),
				((rat_bit_value == NULL) ? "NULL" : rat_bit_value));
			return -EINVAL;
		}

		ret = lte_lc_psm_req(true);
		if (ret < 0) {
			mosh_error("Cannot enable PSM: %d", ret);
		} else {
			mosh_print("PSM enabled");
		}
	} else if (common_option == LINK_COMMON_DISABLE) {
		ret = lte_lc_psm_req(false);
		if (ret < 0) {
			mosh_error("Cannot disable PSM: %d", ret);
		} else {
			mosh_print("PSM disabled");
		}
	} else if (common_option == LINK_COMMON_READ) {
		int tau, active_time;

		ret = lte_lc_psm_get(&tau, &active_time);
		if (ret < 0) {
			mosh_error("Cannot get PSM configs: %d", ret);
		} else {
			mosh_print(
				"PSM config: TAU %d %s, active time %d %s",
				tau,
				(tau == -1) ? "(timer deactivated)" :
				"seconds",
				active_time,
				(active_time == -1) ?
				"(timer deactivated)" :
				"seconds");
		}
	} else {
		link_shell_print_usage(LINK_CMD_PSM);
	}

	return 0;
}

static int link_shell_rai(const struct shell *shell, size_t argc, char **argv)
{
	enum link_shell_common_options common_option = LINK_COMMON_NONE;

	common_option = link_shell_getopt_common(argc, argv);

	if (common_option == LINK_COMMON_READ) {
		link_rai_read();
	} else if (common_option == LINK_COMMON_ENABLE) {
		link_rai_enable(true);
	} else if (common_option == LINK_COMMON_DISABLE) {
		link_rai_enable(false);
	} else {
		link_shell_print_usage(LINK_CMD_RAI);
	}

	return 0;
}

static int link_shell_redmob(const struct shell *shell, size_t argc, char **argv)
{
	int ret;
	enum link_shell_common_options common_option = LINK_COMMON_NONE;
	enum lte_lc_reduced_mobility_mode redmob_mode = LINK_REDMOB_NONE;
	char snum[10];

	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		link_shell_getopt_common_option(opt, &common_option);

		switch (opt) {
		case LINK_SHELL_OPT_REDMOB_DEFAULT:
			redmob_mode = LTE_LC_REDUCED_MOBILITY_DEFAULT;
			break;
		case LINK_SHELL_OPT_REDMOB_NORDIC:
			redmob_mode = LTE_LC_REDUCED_MOBILITY_NORDIC;
			break;
		default:
			break;
		}
	}

	if (common_option == LINK_COMMON_DISABLE) {
		redmob_mode = LTE_LC_REDUCED_MOBILITY_DISABLED;
	}
	if (common_option == LINK_COMMON_READ) {
		enum lte_lc_reduced_mobility_mode mode;

		ret = lte_lc_reduced_mobility_get(&mode);
		if (ret) {
			mosh_error("Cannot get reduced mobility mode: %d", ret);
		} else {
			mosh_print(
				"Reduced mobility mode read successfully: %s",
				link_shell_redmob_mode_to_string(mode, snum));
		}
	} else if (redmob_mode != LINK_REDMOB_NONE) {
		ret = lte_lc_reduced_mobility_set(redmob_mode);
		if (ret) {
			mosh_error("Cannot set reduced mobility mode: %d", ret);
		} else {
			mosh_print(
				"Reduced mobility mode set successfully: %s",
				link_shell_redmob_mode_to_string(redmob_mode, snum));
		}
	} else {
		link_shell_print_usage(LINK_CMD_REDMOB);
	}

	return 0;
}

static int link_shell_rsrp(const struct shell *shell, size_t argc, char **argv)
{
	enum link_shell_common_options common_option = LINK_COMMON_NONE;

	common_option = link_shell_getopt_common(argc, argv);

	if (common_option == LINK_COMMON_SUBSCRIBE) {
		link_rsrp_subscribe(true);
	} else if (common_option == LINK_COMMON_UNSUBSCRIBE) {
		link_rsrp_subscribe(false);
	} else {
		link_shell_print_usage(LINK_CMD_RSRP);
	}

	return 0;
}

static int link_shell_search(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	enum link_shell_common_options common_option = LINK_COMMON_NONE;
	struct lte_lc_periodic_search_cfg search_cfg = { 0 };
	bool search_cfg_given = false;

	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		link_shell_getopt_common_option(opt, &common_option);

		switch (opt) {
		case LINK_SHELL_OPT_SEARCH_CFG: {
			int loop_integer = 0;

			ret = sscanf(
				optarg,
				"%d,%hd,%hd",
				&loop_integer,
				&search_cfg.return_to_pattern,
				&search_cfg.band_optimization);

			if (ret < 0) {
				mosh_error("Reading --search_cfg option failed: %d", ret);
				return -EINVAL;
			} else if (ret != 3) {
				mosh_error(
					"3 values should be given for --search_cfg option "
					"but %d given", ret);
				return -EINVAL;
			}

			if (loop_integer != 0 && loop_integer != 1) {
				mosh_error(
					"1st parameter of --search_cfg option must be 0 or 1 "
					"but %d given", loop_integer);
				return -EINVAL;
			}

			search_cfg.loop = loop_integer;
			search_cfg_given = true;
			break;
		}
		case LINK_SHELL_OPT_SEARCH_PATTERN_RANGE: {
			int index = search_cfg.pattern_count;

			ret = sscanf(
				optarg,
				"%hd,%hd,%hd,%hd",
				&search_cfg.patterns[index].range.initial_sleep,
				&search_cfg.patterns[index].range.final_sleep,
				&search_cfg.patterns[index].range.time_to_final_sleep,
				&search_cfg.patterns[index].range.pattern_end_point);

			if (ret < 0) {
				mosh_error("Reading --search_pattern_range option failed: %d", ret);
				return -EINVAL;
			} else if (ret != 4) {
				mosh_error(
					"4 values should be given for --search_pattern_range "
					"option but %d given", ret);
				return -EINVAL;
			}
			search_cfg.patterns[index].type = LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE;
			search_cfg.pattern_count++;
			break;
		}
		case LINK_SHELL_OPT_SEARCH_PATTERN_TABLE: {
			int index = search_cfg.pattern_count;

			search_cfg.patterns[index].table.val_2 = -1;
			search_cfg.patterns[index].table.val_3 = -1;
			search_cfg.patterns[index].table.val_4 = -1;
			search_cfg.patterns[index].table.val_5 = -1;

			ret = sscanf(
				optarg,
				"%d,%d,%d,%d,%d",
				&search_cfg.patterns[index].table.val_1,
				&search_cfg.patterns[index].table.val_2,
				&search_cfg.patterns[index].table.val_3,
				&search_cfg.patterns[index].table.val_4,
				&search_cfg.patterns[index].table.val_5);

			if (ret < 0) {
				mosh_error("Reading --search_pattern_table option failed: %d", ret);
				return -EINVAL;
			} else if (ret > 5) {
				mosh_error(
					"1 to 5 values should be given for --search_pattern_table "
					"option but %d given", ret);
				return -EINVAL;
			}
			search_cfg.patterns[index].type = LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE;
			search_cfg.pattern_count++;
			break;
		}
		default:
			break;
		}
	}

	if (common_option == LINK_COMMON_READ) {

		ret = lte_lc_periodic_search_get(&search_cfg);
		if (ret) {
			mosh_error("Reading search configuration failed: %d", ret);
			return ret;
		}
		mosh_print("Search configuration:");
		mosh_print("  Loop: %s", search_cfg.loop ? "True" : "False");
		mosh_print("  Return to pattern: %u", search_cfg.return_to_pattern);
		mosh_print("  Band optimization: %u", search_cfg.band_optimization);
		mosh_print("  Pattern count: %d", search_cfg.pattern_count);
		for (int i = 0; i < search_cfg.pattern_count; i++) {
			struct lte_lc_periodic_search_pattern *pattern = &search_cfg.patterns[i];

			mosh_print("  [%d] Type: %s", i,
				(pattern->type == LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE) ?
				"Range" : "Table");

			if (pattern->type == LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE) {
				mosh_print("  [%d] Initial sleep: %u", i,
					pattern->range.initial_sleep);
				mosh_print("  [%d] Final sleep: %u", i,
					pattern->range.final_sleep);
				mosh_print("  [%d] Time to final sleep: %d", i,
					pattern->range.time_to_final_sleep);
				mosh_print("  [%d] Pattern endpoint: %d", i,
					pattern->range.pattern_end_point);
			} else if (pattern->type == LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE) {
				mosh_print("  [%d] Value 1: %d", i, pattern->table.val_1);
				mosh_print("  [%d] Value 2: %d", i, pattern->table.val_2);
				mosh_print("  [%d] Value 3: %d", i, pattern->table.val_3);
				mosh_print("  [%d] Value 4: %d", i, pattern->table.val_4);
				mosh_print("  [%d] Value 5: %d", i, pattern->table.val_5);
			} else {
				mosh_error("Unknown pattern");
			}
		}
	} else if (common_option == LINK_COMMON_WRITE) {

		if (!search_cfg_given) {
			mosh_print("No --search_cfg option given, using default configuration");

			/* Using search parameters for normal power level mentioned in
			 * https://infocenter.nordicsemi.com/index.jsp?topic=%2Fref_at_commands%2FREF%2Fat_commands%2Fnw_service%2Fperiodicsearchconf_set.html
			 */
			search_cfg.loop = false;
			search_cfg.return_to_pattern = 0;
			search_cfg.band_optimization = 1;
			search_cfg.pattern_count = 2;

			search_cfg.patterns[0].type = LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE;
			search_cfg.patterns[0].range.initial_sleep = 10;
			search_cfg.patterns[0].range.final_sleep = 40;
			search_cfg.patterns[0].range.time_to_final_sleep = 5;
			search_cfg.patterns[0].range.pattern_end_point = 15;

			search_cfg.patterns[1].type = LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE;
			search_cfg.patterns[1].table.val_1 = 60;
			search_cfg.patterns[1].table.val_2 = 90;
			search_cfg.patterns[1].table.val_3 = 300;
			search_cfg.patterns[1].table.val_4 = -1;
			search_cfg.patterns[1].table.val_5 = -1;

		} else if (search_cfg.pattern_count == 0) {
			mosh_error(
				"No --search_pattern_range or --search_pattern_table "
				"options given");
			return -EINVAL;
		}

		ret = lte_lc_periodic_search_set(&search_cfg);
		if (ret) {
			mosh_error("Writing search configuration failed: %d", ret);
		} else {
			mosh_print("Writing search configuration succeeded");
		}

	} else if (common_option == LINK_COMMON_START) {
		ret = lte_lc_periodic_search_request();
		if (ret) {
			mosh_error("Starting search failed: %d", ret);
		} else {
			mosh_print("Search requested");
		}
	} else if (common_option == LINK_COMMON_RESET) {
		ret = lte_lc_periodic_search_clear();
		if (ret) {
			mosh_error("Clearing search configuration failed: %d", ret);
		} else {
			mosh_print("Search configuration cleared");
		}
	} else {
		link_shell_print_usage(LINK_CMD_SEARCH);
	}

	return 0;
}

static int link_shell_settings(const struct shell *shell, size_t argc, char **argv)
{
	enum link_shell_common_options common_option = LINK_COMMON_NONE;
	enum lte_lc_factory_reset_type mreset_type = LTE_LC_FACTORY_RESET_INVALID;

	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		link_shell_getopt_common_option(opt, &common_option);

		switch (opt) {
		case LINK_SHELL_OPT_MRESET_ALL:
			mreset_type = LTE_LC_FACTORY_RESET_ALL;
			break;
		case LINK_SHELL_OPT_MRESET_USER:
			mreset_type = LTE_LC_FACTORY_RESET_USER;
			break;
		default:
			break;
		}
	}

	if (common_option == LINK_COMMON_READ) {
		link_sett_all_print();
	} else if (common_option == LINK_COMMON_RESET ||
		   mreset_type != LTE_LC_FACTORY_RESET_INVALID) {
		if (common_option == LINK_COMMON_RESET) {
			link_sett_defaults_set();
			if (SYS_MODE_PREFERRED != LINK_SYSMODE_NONE) {
				link_shell_sysmode_set(SYS_MODE_PREFERRED,
						       CONFIG_LTE_MODE_PREFERENCE);
			}
		}
		if (mreset_type == LTE_LC_FACTORY_RESET_ALL) {
			link_sett_modem_factory_reset(LTE_LC_FACTORY_RESET_ALL);
		} else if (mreset_type == LTE_LC_FACTORY_RESET_USER) {
			link_sett_modem_factory_reset(LTE_LC_FACTORY_RESET_USER);
		}
	} else {
		link_shell_print_usage(LINK_CMD_SETTINGS);
	}

	return 0;
}

static int link_shell_status(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	enum lte_lc_nw_reg_status current_reg_status;
	enum lte_lc_func_mode functional_mode;
	bool connected = false;
	char snum[64];

	ret = lte_lc_func_mode_get(&functional_mode);
	if (ret) {
		mosh_warn("Cannot get functional mode from modem: %d", ret);
	} else {
		mosh_print(
			"Modem functional mode: %s",
			link_shell_funmode_to_string(functional_mode, snum));
	}
	ret = lte_lc_nw_reg_status_get(&current_reg_status);
	if (ret >= 0) {
		link_shell_print_reg_status(current_reg_status);
	} else {
		mosh_error("Cannot get current registration status (%d)", ret);
	}
	if (current_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ||
	    current_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING) {
		connected = true;
	}

	link_api_modem_info_get_for_shell(connected);
	return 0;
}

static int link_shell_sysmode(const struct shell *shell, size_t argc, char **argv)
{
	int ret = 0;
	enum lte_lc_system_mode sysmode_option = LINK_SYSMODE_NONE;
	enum lte_lc_system_mode_preference sysmode_lte_pref_option = LTE_LC_SYSTEM_MODE_PREFER_AUTO;
	enum link_shell_common_options common_option = LINK_COMMON_NONE;

	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		link_shell_getopt_common_option(opt, &common_option);

		switch (opt) {
		case 'm':
			sysmode_option = LTE_LC_SYSTEM_MODE_LTEM;
			break;
		case 'n':
			sysmode_option = LTE_LC_SYSTEM_MODE_NBIOT;
			break;
		case 'g':
			sysmode_option = LTE_LC_SYSTEM_MODE_GPS;
			break;
		case 'M':
			sysmode_option = LTE_LC_SYSTEM_MODE_LTEM_GPS;
			break;
		case 'N':
			sysmode_option = LTE_LC_SYSTEM_MODE_NBIOT_GPS;
			break;
		case LINK_SHELL_OPT_SYSMODE_LTEM_NBIOT:
			sysmode_option = LTE_LC_SYSTEM_MODE_LTEM_NBIOT;
			break;
		case LINK_SHELL_OPT_SYSMODE_LTEM_NBIOT_GNSS:
			sysmode_option = LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS;
			break;
		case LINK_SHELL_OPT_SYSMODE_PREF_AUTO:
			sysmode_lte_pref_option = LTE_LC_SYSTEM_MODE_PREFER_AUTO;
			break;
		case LINK_SHELL_OPT_SYSMODE_PREF_LTEM:
			sysmode_lte_pref_option = LTE_LC_SYSTEM_MODE_PREFER_LTEM;
			break;
		case LINK_SHELL_OPT_SYSMODE_PREF_NBIOT:
			sysmode_lte_pref_option = LTE_LC_SYSTEM_MODE_PREFER_NBIOT;
			break;
		case LINK_SHELL_OPT_SYSMODE_PREF_LTEM_PLMN_PRIO:
			sysmode_lte_pref_option = LTE_LC_SYSTEM_MODE_PREFER_LTEM_PLMN_PRIO;
			break;
		case LINK_SHELL_OPT_SYSMODE_PREF_NBIOT_PLMN_PRIO:
			sysmode_lte_pref_option = LTE_LC_SYSTEM_MODE_PREFER_NBIOT_PLMN_PRIO;
			break;
		default:
			break;
		}
	}

	if (common_option == LINK_COMMON_READ) {
		enum lte_lc_system_mode sys_mode_current;
		enum lte_lc_system_mode_preference sys_mode_pref_current;
		enum lte_lc_lte_mode currently_active_mode;

		ret = link_shell_get_and_print_current_system_modes(
			&sys_mode_current, &sys_mode_pref_current, &currently_active_mode);
		if (ret < 0) {
			mosh_error("Cannot read system mode of the modem: %d", ret);
		} else {
			enum lte_lc_system_mode sett_sys_mode;
			enum lte_lc_system_mode_preference sett_lte_pref;

			/* Print also settings stored in mosh side: */
			link_sett_sysmode_print();
			sett_sys_mode = link_sett_sysmode_get();
			sett_lte_pref = link_sett_sysmode_lte_preference_get();
			if (sett_sys_mode != LINK_SYSMODE_NONE &&
				sett_sys_mode != sys_mode_current &&
				sett_lte_pref != sys_mode_pref_current) {
				mosh_warn(
					"note: seems that set link sysmode and a "
					"counterparts in modem are not in synch");
				mosh_warn(
					"but no worries; requested system mode retried "
					"next time when going to normal mode");
			}
		}
	} else if (sysmode_option != LINK_SYSMODE_NONE) {
		link_shell_sysmode_set(sysmode_option, sysmode_lte_pref_option);

		/* Save system modem to link settings: */
		(void)link_sett_sysmode_save(sysmode_option, sysmode_lte_pref_option);

	} else if (common_option == LINK_COMMON_RESET) {
		if (SYS_MODE_PREFERRED != LINK_SYSMODE_NONE) {
			link_shell_sysmode_set(SYS_MODE_PREFERRED, CONFIG_LTE_MODE_PREFERENCE);
		}

		(void)link_sett_sysmode_default_set();
	} else {
		link_shell_print_usage(LINK_CMD_SYSMODE);
	}

	return 0;
}

static int link_shell_tau(const struct shell *shell, size_t argc, char **argv)
{
	enum link_shell_common_options common_option = LINK_COMMON_NONE;
	int threshold_time = 0;

	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;
	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		link_shell_getopt_common_option(opt, &common_option);

		switch (opt) {
		case LINK_SHELL_OPT_THRESHOLD_TIME:
			threshold_time = atoi(optarg);
			if (threshold_time <= 0) {
				mosh_error(
					"Not a valid number for --threshold_time (milliseconds).");
				return -EINVAL;
			}
			break;
		default:
			break;
		}
	}

	if (common_option == LINK_COMMON_SUBSCRIBE) {
		link_modem_tau_notifications_subscribe(
			CONFIG_LTE_LC_TAU_PRE_WARNING_TIME_MS,
			((threshold_time) ?
				threshold_time :
				CONFIG_LTE_LC_TAU_PRE_WARNING_THRESHOLD_MS));
	} else if (common_option == LINK_COMMON_UNSUBSCRIBE) {
		link_modem_tau_notifications_unsubscribe();
	} else {
		link_shell_print_usage(LINK_CMD_TAU);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_link,
	SHELL_CMD_ARG(
		coneval, NULL,
		"Get connection evaluation parameters (no options).",
		link_shell_coneval, 1, 0),
	SHELL_CMD_ARG(
		connect, NULL,
		"Connect to given apn by creating and activating a new PDP context.",
		link_shell_connect, 0, 10),
	SHELL_CMD_ARG(
		defcont, NULL,
		"Set custom default PDP context config. "
		"Persistent between the sessions. "
		"Effective when going to normal mode.",
		link_shell_defcont, 0, 10),
	SHELL_CMD_ARG(
		defcontauth, NULL,
		"Set custom authentication parameters for the default PDP context. "
		"Persistent between the sessions. "
		"Effective when going to normal mode.",
		link_shell_defcontauth, 0, 10),
	SHELL_CMD_ARG(
		disconnect, NULL,
		"Disconnect from given apn by deactivating and destroying a PDP context.",
		link_shell_disconnect, 0, 10),
	SHELL_CMD_ARG(
		dnsaddr, NULL,
		"Set a manual DNS server address. "
		"The manual DNS server address will be used if the mobile network "
		"operator does not provide any DNS addresses, or if the name "
		"resolution using DNS server(s) provided by mobile network operator "
		"fails for given domain name. The manual DNS address does not "
		"override the mobile network operator provided DNS addresses.",
		link_shell_dnsaddr, 0, 10),
	SHELL_CMD_ARG(
		edrx, NULL,
		"Enable/disable eDRX with default or with custom parameters.",
		link_shell_edrx, 0, 10),
	SHELL_CMD_ARG(
		funmode, NULL,
		"Set/read functional modes of the modem.",
		link_shell_funmode, 0, 10),
	SHELL_CMD_ARG(
		msleep, NULL,
		"Subscribe/unsubscribe for modem sleep notifications.",
		link_shell_msleep, 0, 10),
	SHELL_CMD_ARG(
		ncellmeas, NULL,
		"Start/cancel neighbor cell measurements.",
		link_shell_ncellmeas, 0, 10),
	SHELL_CMD_ARG(
		nmodeat, NULL,
		"Set custom AT commmands that are run when going to normal mode.",
		link_shell_nmodeat, 0, 10),
	SHELL_CMD_ARG(
		nmodeauto, NULL,
		"Enabling/disabling of automatic connecting and going to normal mode after "
		"the bootup. Persistent between the sessions. Has impact after the bootup.",
		link_shell_nmodeauto, 0, 10),
	SHELL_CMD_ARG(
		psm, NULL,
		"Enable/disable Power Saving Mode (PSM) with default or with custom parameters.",
		link_shell_psm, 0, 10),
	SHELL_CMD_ARG(
		rai, NULL,
		"Enable/disable RAI feature. Actual use must be set for commands "
		"supporting RAI. Effective when going to normal mode.",
		link_shell_rai, 0, 10),
	SHELL_CMD_ARG(
		redmob, NULL,
		"Set/read reduced mobility modes of the modem.",
		link_shell_redmob, 0, 10),
	SHELL_CMD_ARG(
		rsrp, NULL,
		"Subscribe/unsubscribe for RSRP signal info.",
		link_shell_rsrp, 0, 10),
	SHELL_CMD_ARG(
		search, NULL,
		"Read/write periodic search parameters or start search operation.",
		link_shell_search, 0, 10),
	SHELL_CMD_ARG(
		settings, NULL,
		"Option to print or reset all persistent link subcmd settings.",
		link_shell_settings, 0, 10),
	SHELL_CMD_ARG(
		status, NULL,
		"Show status of the current connection (no options).",
		link_shell_status, 1, 0),
	SHELL_CMD_ARG(
		sysmode, NULL,
		"Set/read functional modes of the modem.",
		link_shell_sysmode, 0, 10),
	SHELL_CMD_ARG(
		tau, NULL,
		"Subscribe/unsubscribe for modem TAU pre-warning notifications.",
		link_shell_tau, 0, 10),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(
	link, &sub_link,
	"Commands for LTE link controlling and status information.",
	NULL);
