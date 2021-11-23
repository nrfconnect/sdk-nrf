/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <shell/shell.h>
#include <shell/shell_uart.h>
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
	LINK_CMD_RAI
};

enum link_shell_common_options {
	LINK_COMMON_NONE = 0,
	LINK_COMMON_READ,
	LINK_COMMON_ENABLE,
	LINK_COMMON_DISABLE,
	LINK_COMMON_SUBSCRIBE,
	LINK_COMMON_UNSUBSCRIBE,
	LINK_COMMON_START,
	LINK_COMMON_STOP,
	LINK_COMMON_RESET
};

struct link_shell_cmd_args_t {
	enum link_shell_command command;
	enum link_shell_common_options common_option;
	enum link_sett_modem_reset_type mreset_type;
	enum link_ncellmeas_modes ncellmeasmode;
	enum lte_lc_neighbor_search_type ncellmeas_search_type;
	enum lte_lc_func_mode funmode_option;
	enum lte_lc_system_mode sysmode_option;
	enum lte_lc_system_mode_preference sysmode_lte_pref_option;
	enum lte_lc_lte_mode lte_mode;
};

/******************************************************************************/
static const char link_usage_str[] =
	"Usage: link <subcommand> [options]\n"
	"\n"
	"Subcommands:\n"
	"  status:       Show status of the current connection (no options)\n"
	"  settings:     Option to print or reset all persistent\n"
	"                link subcmd settings.\n"
	"  coneval:      Get connection evaluation parameters (no options)\n"
	"  defcont:      Set custom default PDP context config.\n"
	"                Persistent between the sessions.\n"
	"                Effective when going to normal mode.\n"
	"  defcontauth:  Set custom authentication parameters for\n"
	"                the default PDP context. Persistent between the sessions.\n"
	"                Effective when going to normal mode.\n"
	"  connect:      Connect to given apn by creating and activating a new PDP\n"
	"                context\n"
	"  disconnect:   Disconnect from given apn by deactivating and destroying\n"
	"                a PDP context\n"
	"  rsrp:         Subscribe/unsubscribe for RSRP signal info\n"
	"  ncellmeas:    Start/cancel neighbor cell measurements\n"
	"  msleep:       Subscribe/unsubscribe for modem sleep notifications\n"
	"  tau:          Subscribe/unsubscribe for modem TAU pre-warning notifications\n"
	"  funmode:      Set/read functional modes of the modem\n"
	"  sysmode:      Set/read system modes of the modem\n"
	"                Persistent between the sessions. Effective when\n"
	"                going to normal mode.\n"
	"  nmodeat:      Set custom AT commmands that are run when going to normal\n"
	"                mode\n"
	"  nmodeauto:    Enabling/disabling of automatic connecting and going to\n"
	"                normal mode after the bootup. Persistent between the sessions.\n"
	"                Has impact after the bootup\n"
	"  edrx:         Enable/disable eDRX with default or with custom parameters\n"
	"  psm:          Enable/disable Power Saving Mode (PSM) with\n"
	"                default or with custom parameters\n"
	"  rai:          Enable/disable RAI feature. Actual use must be set for commands\n"
	"                supporting RAI. Effective when going to normal mode.\n";

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
	"  -A, --prot,  [int]  Authentication protocol: 0 (None), 1 (PAP), 2 (CHAP)\n"
	"\n"
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

static const char link_ncellmeas_usage_str[] =
	"Usage: link ncellmeas --single | --continuous | --cancel | [--search_type <type>]\n"
	"Options:\n"
	"      --single,       Start a neighbor cell measurement and report result\n"
	"      --continuous,   Start continuous neighbor cell measurement mode and report result\n"
	"      --cancel,       Cancel/Stop neighbor cell measurement if still ongoing\n"
	"      --search_type,  Used search type:\n"
	"                      'default', 'ext_light' or 'ext_comp.'\n"
	"\n"
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
	"                      performed for all supported bands.\n";

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
	LINK_SHELL_OPT_NMODE_NO_REL14,
};

/* Specifying the expected options (both long and short): */
static struct option long_options[] = {
	{ "apn", required_argument, 0, 'a' },
	{ "cid", required_argument, 0, 'I' },
	{ "family", required_argument, 0, 'f' },
	{ "subscribe", no_argument, 0, 's' },
	{ "unsubscribe", no_argument, 0, 'u' },
	{ "read", no_argument, 0, 'r' },
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
	{ "search_type", required_argument, 0, LINK_SHELL_OPT_NCELLMEAS_SEARCH_TYPE },
	{ "normal_no_rel14", no_argument, 0, LINK_SHELL_OPT_NMODE_NO_REL14 },
	{ 0, 0, 0, 0 }
};

/******************************************************************************/

static void link_shell_print_usage(struct link_shell_cmd_args_t *link_cmd_args)
{
	switch (link_cmd_args->command) {
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
	case LINK_CMD_DISCONNECT:
		mosh_print_no_format(link_connect_usage_str);
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
	default:
		mosh_print_no_format(link_usage_str);
		break;
	}
}

static void link_shell_cmd_defaults_set(struct link_shell_cmd_args_t *link_cmd_args)
{
	memset(link_cmd_args, 0, sizeof(struct link_shell_cmd_args_t));
	link_cmd_args->funmode_option = LINK_FUNMODE_NONE;
	link_cmd_args->sysmode_option = LTE_LC_SYSTEM_MODE_NONE;
	link_cmd_args->sysmode_lte_pref_option =
		LTE_LC_SYSTEM_MODE_PREFER_AUTO;
	link_cmd_args->lte_mode = LTE_LC_LTE_MODE_NONE;
	link_cmd_args->common_option = LINK_COMMON_NONE;
	link_cmd_args->mreset_type = LINK_SHELL_MODEM_FACTORY_RESET_NONE;
	link_cmd_args->ncellmeasmode = LINK_NCELLMEAS_MODE_NONE;
	link_cmd_args->ncellmeas_search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT;
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
	 LTE_LC_SYSTEM_MODE_NONE)

static void link_shell_sysmode_set(int sysmode, int lte_pref)
{
	enum lte_lc_func_mode functional_mode;
	char snum[64];
	int ret = lte_lc_system_mode_set(sysmode, lte_pref);

	if (ret < 0) {
		mosh_error("Cannot set system mode to modem: %d", ret);
		ret = lte_lc_func_mode_get(&functional_mode);
		if (functional_mode != LTE_LC_FUNC_MODE_OFFLINE ||
		    functional_mode != LTE_LC_FUNC_MODE_POWER_OFF) {
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

int link_shell(const struct shell *shell, size_t argc, char **argv)
{
	struct link_shell_cmd_args_t link_cmd_args;
	int ret = 0;
	bool require_apn = false;
	bool require_pdn_cid = false;
	bool require_subscribe = false;
	bool require_option = false;
	bool nmode_use_rel14 = true;
	char *apn = NULL;
	char *family = NULL;
	int protocol = 0;
	bool protocol_given = false;
	char *username = NULL;
	char *password = NULL;
	int pdn_cid = 0;
	int threshold_time = 0;

	link_shell_cmd_defaults_set(&link_cmd_args);

	if (argc < 2) {
		goto show_usage;
	}

	/* command = argv[0] = "link" */
	/* sub-command = argv[1]       */
	if (strcmp(argv[1], "status") == 0) {
		link_cmd_args.command = LINK_CMD_STATUS;
	} else if (strcmp(argv[1], "settings") == 0) {
		link_cmd_args.command = LINK_CMD_SETTINGS;
	} else if (strcmp(argv[1], "coneval") == 0) {
		link_cmd_args.command = LINK_CMD_CONEVAL;
	} else if (strcmp(argv[1], "rsrp") == 0) {
		require_subscribe = true;
		link_cmd_args.command = LINK_CMD_RSRP;
	} else if (strcmp(argv[1], "ncellmeas") == 0) {
		require_option = true;
		link_cmd_args.command = LINK_CMD_NCELLMEAS;
	} else if (strcmp(argv[1], "msleep") == 0) {
		require_option = true;
		link_cmd_args.command = LINK_CMD_MDMSLEEP;
	} else if (strcmp(argv[1], "tau") == 0) {
		require_subscribe = true;
		link_cmd_args.command = LINK_CMD_TAU;
	} else if (strcmp(argv[1], "connect") == 0) {
		require_apn = true;
		link_cmd_args.command = LINK_CMD_CONNECT;
	} else if (strcmp(argv[1], "disconnect") == 0) {
		require_pdn_cid = true;
		link_cmd_args.command = LINK_CMD_DISCONNECT;
	} else if (strcmp(argv[1], "defcont") == 0) {
		link_cmd_args.command = LINK_CMD_DEFCONT;
	} else if (strcmp(argv[1], "defcontauth") == 0) {
		link_cmd_args.command = LINK_CMD_DEFCONTAUTH;
	} else if (strcmp(argv[1], "funmode") == 0) {
		require_option = true;
		link_cmd_args.command = LINK_CMD_FUNMODE;
	} else if (strcmp(argv[1], "sysmode") == 0) {
		require_option = true;
		link_cmd_args.command = LINK_CMD_SYSMODE;
	} else if (strcmp(argv[1], "nmodeat") == 0) {
		link_cmd_args.command = LINK_CMD_NORMAL_MODE_AT;
	} else if (strcmp(argv[1], "nmodeauto") == 0) {
		link_cmd_args.command = LINK_CMD_NORMAL_MODE_AUTO;
	} else if (strcmp(argv[1], "edrx") == 0) {
		require_option = true;
		link_cmd_args.command = LINK_CMD_EDRX;
	} else if (strcmp(argv[1], "psm") == 0) {
		require_option = true;
		link_cmd_args.command = LINK_CMD_PSM;
	} else if (strcmp(argv[1], "rai") == 0) {
		require_option = true;
		link_cmd_args.command = LINK_CMD_RAI;
	} else {
		mosh_error("Unsupported command=%s\n", argv[1]);
		ret = -EINVAL;
		goto show_usage;
	}

	/* Reset getopt due to possible previous failures: */
	optreset = 1;

	/* We start from subcmd arguments */
	optind = 2;

	int long_index = 0;
	int opt;
	int apn_len = 0;

	char edrx_value_str[LINK_SHELL_EDRX_VALUE_STR_LENGTH + 1];
	bool edrx_value_set = false;
	char edrx_ptw_bit_str[LINK_SHELL_EDRX_PTW_STR_LENGTH + 1];
	bool edrx_ptw_set = false;

	char psm_rptau_bit_str[LINK_SHELL_PSM_PARAM_STR_LENGTH + 1];
	bool psm_rptau_set = false;
	char psm_rat_bit_str[LINK_SHELL_PSM_PARAM_STR_LENGTH + 1];
	bool psm_rat_set = false;

	char *normal_mode_at_str = NULL;
	uint8_t normal_mode_at_mem_slot = 0;

	while ((opt = getopt_long(argc, argv,
				  "a:I:f:x:w:p:t:A:P:U:su014rmngMNed",
				  long_options, &long_index)) != -1) {
		switch (opt) {
		/* RSRP: */
		case 's':
			link_cmd_args.common_option = LINK_COMMON_SUBSCRIBE;
			break;
		case 'u':
			link_cmd_args.common_option = LINK_COMMON_UNSUBSCRIBE;
			break;

		/* Modem functional modes: */
		case '0':
			link_cmd_args.funmode_option =
				LTE_LC_FUNC_MODE_POWER_OFF;
			break;
		case '1':
			link_cmd_args.funmode_option = LTE_LC_FUNC_MODE_NORMAL;
			break;
		case '4':
			link_cmd_args.funmode_option =
				LTE_LC_FUNC_MODE_OFFLINE;
			break;
		case LINK_SHELL_OPT_FUNMODE_LTEOFF:
			link_cmd_args.funmode_option =
				LTE_LC_FUNC_MODE_DEACTIVATE_LTE;
			break;
		case LINK_SHELL_OPT_FUNMODE_LTEON:
			link_cmd_args.funmode_option =
				LTE_LC_FUNC_MODE_ACTIVATE_LTE;
			break;
		case LINK_SHELL_OPT_FUNMODE_GNSSOFF:
			link_cmd_args.funmode_option =
				LTE_LC_FUNC_MODE_DEACTIVATE_GNSS;
			break;
		case LINK_SHELL_OPT_FUNMODE_GNSSON:
			link_cmd_args.funmode_option =
				LTE_LC_FUNC_MODE_ACTIVATE_GNSS;
			break;
		case LINK_SHELL_OPT_FUNMODE_UICCOFF:
			link_cmd_args.funmode_option =
				LTE_LC_FUNC_MODE_DEACTIVATE_UICC;
			break;
		case LINK_SHELL_OPT_FUNMODE_UICCON:
			link_cmd_args.funmode_option =
				LTE_LC_FUNC_MODE_ACTIVATE_UICC;
			break;
		case LINK_SHELL_OPT_FUNMODE_FLIGHTMODE_UICCON:
			link_cmd_args.funmode_option =
				LTE_LC_FUNC_MODE_OFFLINE_UICC_ON;
			break;

		/* eDRX specifics: */
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

		/* PSM specifics: */
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

		/* Modem system modes: */
		case 'm':
			link_cmd_args.sysmode_option = LTE_LC_SYSTEM_MODE_LTEM;
			link_cmd_args.lte_mode = LTE_LC_LTE_MODE_LTEM;
			break;
		case 'n':
			link_cmd_args.sysmode_option =
				LTE_LC_SYSTEM_MODE_NBIOT;
			link_cmd_args.lte_mode = LTE_LC_LTE_MODE_NBIOT;
			break;
		case 'g':
			link_cmd_args.sysmode_option = LTE_LC_SYSTEM_MODE_GPS;
			break;
		case 'M':
			link_cmd_args.sysmode_option =
				LTE_LC_SYSTEM_MODE_LTEM_GPS;
			break;
		case 'N':
			link_cmd_args.sysmode_option =
				LTE_LC_SYSTEM_MODE_NBIOT_GPS;
			break;

		/* Common options: */
		case 'e':
			link_cmd_args.common_option = LINK_COMMON_ENABLE;
			break;
		case 'd':
			link_cmd_args.common_option = LINK_COMMON_DISABLE;
			break;
		case 'r':
			link_cmd_args.common_option = LINK_COMMON_READ;
			break;
		case 'I': /* PDN CID */
			pdn_cid = atoi(optarg);
			if (pdn_cid == 0) {
				mosh_error(
					"PDN CID (%d) must be positive integer. "
					"Default PDN context (CID=0) cannot be given.",
					pdn_cid);
				return -EINVAL;
			}
			break;
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
		/* Options without short option: */
		case LINK_SHELL_OPT_SINGLE:
			link_cmd_args.ncellmeasmode = LINK_NCELLMEAS_MODE_SINGLE;
			break;
		case LINK_SHELL_OPT_CONTINUOUS:
			link_cmd_args.ncellmeasmode = LINK_NCELLMEAS_MODE_CONTINUOUS;
			break;
		case LINK_SHELL_OPT_NCELLMEAS_SEARCH_TYPE:
			link_cmd_args.ncellmeas_search_type =
				link_shell_string_to_ncellmeas_search_type(optarg);
			if (link_cmd_args.ncellmeas_search_type ==
			    MOSH_NCELLMEAS_SEARCH_TYPE_NONE) {
				mosh_error("Unknown search_type. See usage:");
				goto show_usage;
			}
			break;
		case LINK_SHELL_OPT_RESET:
			link_cmd_args.common_option = LINK_COMMON_RESET;
			break;
		case LINK_SHELL_OPT_MRESET_ALL:
			link_cmd_args.mreset_type = LINK_SHELL_MODEM_FACTORY_RESET_ALL;
			break;
		case LINK_SHELL_OPT_MRESET_USER:
			link_cmd_args.mreset_type = LINK_SHELL_MODEM_FACTORY_RESET_USER_CONFIG;
			break;
		case LINK_SHELL_OPT_START:
			link_cmd_args.common_option = LINK_COMMON_START;
			break;
		case LINK_SHELL_OPT_STOP:
			link_cmd_args.common_option = LINK_COMMON_STOP;
			break;

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

		case LINK_SHELL_OPT_SYSMODE_LTEM_NBIOT:
			link_cmd_args.sysmode_option =
				LTE_LC_SYSTEM_MODE_LTEM_NBIOT;
			break;
		case LINK_SHELL_OPT_SYSMODE_LTEM_NBIOT_GNSS:
			link_cmd_args.sysmode_option =
				LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS;
			break;

		case LINK_SHELL_OPT_SYSMODE_PREF_AUTO:
			link_cmd_args.sysmode_lte_pref_option =
				LTE_LC_SYSTEM_MODE_PREFER_AUTO;
			break;
		case LINK_SHELL_OPT_SYSMODE_PREF_LTEM:
			link_cmd_args.sysmode_lte_pref_option =
				LTE_LC_SYSTEM_MODE_PREFER_LTEM;
			break;
		case LINK_SHELL_OPT_SYSMODE_PREF_NBIOT:
			link_cmd_args.sysmode_lte_pref_option =
				LTE_LC_SYSTEM_MODE_PREFER_NBIOT;
			break;
		case LINK_SHELL_OPT_SYSMODE_PREF_LTEM_PLMN_PRIO:
			link_cmd_args.sysmode_lte_pref_option =
				LTE_LC_SYSTEM_MODE_PREFER_LTEM_PLMN_PRIO;
			break;
		case LINK_SHELL_OPT_SYSMODE_PREF_NBIOT_PLMN_PRIO:
			link_cmd_args.sysmode_lte_pref_option =
				LTE_LC_SYSTEM_MODE_PREFER_NBIOT_PLMN_PRIO;
			break;
		case LINK_SHELL_OPT_THRESHOLD_TIME:
			threshold_time = atoi(optarg);
			if (threshold_time == 0) {
				mosh_error(
					"Not a valid number for --threshold_time (milliseconds).");
				return -EINVAL;
			}
			break;
		case LINK_SHELL_OPT_NMODE_NO_REL14:
			if (link_cmd_args.command == LINK_CMD_NORMAL_MODE_AUTO) {
				/* Enable autoconnect without REL14 features */
				link_cmd_args.common_option = LINK_COMMON_ENABLE;
			} else if (link_cmd_args.command == LINK_CMD_FUNMODE) {
				/* Go to normal mode without REL14 features */
				link_cmd_args.funmode_option = LTE_LC_FUNC_MODE_NORMAL;
			}
			nmode_use_rel14 = false;
			break;
		case '?':
			goto show_usage;
		default:
			mosh_error("Unknown option. See usage:");
			goto show_usage;
		}
	}

	/* Check that all mandatory args were given: */
	if (require_apn && apn == NULL) {
		mosh_error("Option -a | -apn MUST be given. See usage:");
		goto show_usage;
	} else if (require_pdn_cid && pdn_cid == 0) {
		mosh_error("-I / --cid MUST be given. See usage:");
		goto show_usage;
	} else if (require_subscribe &&
		   link_cmd_args.common_option == LINK_COMMON_NONE) {
		mosh_error("Either -s or -u MUST be given. See usage:");
		goto show_usage;
	} else if (require_option &&
		   link_cmd_args.funmode_option == LINK_FUNMODE_NONE &&
		   link_cmd_args.sysmode_option == LTE_LC_SYSTEM_MODE_NONE &&
		   link_cmd_args.ncellmeasmode == LINK_NCELLMEAS_MODE_NONE &&
		   link_cmd_args.common_option == LINK_COMMON_NONE) {
		mosh_error("Command needs option to be given. See usage:");
		goto show_usage;
	}

	char snum[64];

	switch (link_cmd_args.command) {
	case LINK_CMD_DEFCONT:
		if (link_cmd_args.common_option == LINK_COMMON_READ) {
			link_sett_defcont_conf_shell_print();
		} else if (link_cmd_args.common_option == LINK_COMMON_ENABLE) {
			link_sett_save_defcont_enabled(true);
		} else if (link_cmd_args.common_option == LINK_COMMON_DISABLE) {
			if (nrf_modem_at_printf("AT+CGDCONT=0") != 0) {
				mosh_warn(
					"ERROR from modem. Getting the initial PDP context back "
					"wasn't successful.");
			}
			link_sett_save_defcont_enabled(false);
			mosh_print("Custom default context config disabled.");
		} else if (link_cmd_args.common_option == LINK_COMMON_NONE &&
			   apn == NULL && family == NULL) {
			goto show_usage;
		}
		if (apn != NULL) {
			(void)link_sett_save_defcont_apn(apn);
		}
		if (family != NULL) {
			enum pdn_fam pdn_lib_fam;

			ret = link_family_str_to_pdn_lib_family(&pdn_lib_fam,
								 family);
			if (ret) {
				mosh_error("Unknown PDN family %s", family);
				goto show_usage;
			} else {
				(void)link_sett_save_defcont_pdn_family(
					pdn_lib_fam);
			}
		}
		break;
	case LINK_CMD_DEFCONTAUTH:
		if (link_cmd_args.common_option == LINK_COMMON_READ) {
			link_sett_defcontauth_conf_shell_print();
		} else if (link_cmd_args.common_option ==
			   LINK_COMMON_ENABLE) {
			if (link_sett_save_defcontauth_enabled(true) < 0) {
				mosh_warn("Cannot enable authentication.");
			}
		} else if (link_cmd_args.common_option ==
			   LINK_COMMON_DISABLE) {
			if (nrf_modem_at_printf("AT+CGAUTH=0,0") != 0) {
				mosh_warn("Disabling of auth cannot be done to modem.");
			}
			link_sett_save_defcontauth_enabled(false);
		} else if (link_cmd_args.common_option == LINK_COMMON_NONE &&
			   !protocol_given && username == NULL &&
			   password == NULL) {
			goto show_usage;
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
		break;

	case LINK_CMD_STATUS: {
		enum lte_lc_nw_reg_status current_reg_status;
		enum lte_lc_func_mode functional_mode;
		bool connected = false;

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
		if (current_reg_status == LTE_LC_NW_REG_REGISTERED_EMERGENCY ||
		    current_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ||
		    current_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING) {
			connected = true;
		}

		link_api_modem_info_get_for_shell(connected);
		break;
	}
	case LINK_CMD_SETTINGS:
		if (link_cmd_args.common_option == LINK_COMMON_READ) {
			link_sett_all_print();
		} else if (link_cmd_args.common_option == LINK_COMMON_RESET ||
			   link_cmd_args.mreset_type != LINK_SHELL_MODEM_FACTORY_RESET_NONE) {
			if (link_cmd_args.common_option == LINK_COMMON_RESET) {
				link_sett_defaults_set();
				link_shell_sysmode_set(SYS_MODE_PREFERRED,
						       CONFIG_LTE_MODE_PREFERENCE);
			}
			if (link_cmd_args.mreset_type == LINK_SHELL_MODEM_FACTORY_RESET_ALL) {
				link_sett_modem_factory_reset(LINK_SHELL_MODEM_FACTORY_RESET_ALL);
			} else if (link_cmd_args.mreset_type ==
					   LINK_SHELL_MODEM_FACTORY_RESET_USER_CONFIG) {
				link_sett_modem_factory_reset(
					LINK_SHELL_MODEM_FACTORY_RESET_USER_CONFIG);
			}
		} else {
			goto show_usage;
		}
		break;
	case LINK_CMD_CONEVAL:
		link_api_coneval_read_for_shell();
		break;

	case LINK_CMD_SYSMODE:
		if (link_cmd_args.common_option == LINK_COMMON_READ) {
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
				sett_lte_pref =
					link_sett_sysmode_lte_preference_get();
				if (sett_sys_mode != LTE_LC_SYSTEM_MODE_NONE &&
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
		} else if (link_cmd_args.sysmode_option != LTE_LC_SYSTEM_MODE_NONE) {
			link_shell_sysmode_set(
				link_cmd_args.sysmode_option,
				link_cmd_args.sysmode_lte_pref_option);

			/* Save system modem to link settings: */
			(void)link_sett_sysmode_save(
				link_cmd_args.sysmode_option,
				link_cmd_args.sysmode_lte_pref_option);

		} else if (link_cmd_args.common_option == LINK_COMMON_RESET) {
			link_shell_sysmode_set(SYS_MODE_PREFERRED, CONFIG_LTE_MODE_PREFERENCE);

			(void)link_sett_sysmode_default_set();
		} else {
			goto show_usage;
		}
		break;
	case LINK_CMD_FUNMODE:
		if (link_cmd_args.common_option == LINK_COMMON_READ) {
			enum lte_lc_func_mode functional_mode;

			ret = lte_lc_func_mode_get(&functional_mode);
			if (ret) {
				mosh_error("Cannot get functional mode: %d", ret);
			} else {
				mosh_print(
					"Functional mode read successfully: %s",
					link_shell_funmode_to_string(functional_mode, snum));
			}
		} else if (link_cmd_args.funmode_option !=
			   LINK_FUNMODE_NONE) {
			ret = link_func_mode_set(link_cmd_args.funmode_option, nmode_use_rel14);
			if (ret < 0) {
				mosh_error("Cannot set functional mode: %d", ret);
			} else {
				mosh_print(
					"Functional mode set successfully: %s",
					link_shell_funmode_to_string(
						link_cmd_args.funmode_option, snum));
			}
		} else {
			goto show_usage;
		}
		break;
	case LINK_CMD_NORMAL_MODE_AT:
		if (link_cmd_args.common_option == LINK_COMMON_READ) {
			link_sett_normal_mode_at_cmds_shell_print();
		} else if (normal_mode_at_str != NULL) {
			ret = link_sett_save_normal_mode_at_cmd_str(
				normal_mode_at_str, normal_mode_at_mem_slot);
			if (ret < 0) {
				mosh_error(
					"Cannot set normal mode AT-command: \"%s\"",
					normal_mode_at_str);
			} else {
				mosh_print(
					"Normal mode AT-command \"%s\" set successfully to "
					"memory slot %d.",
					((strlen(normal_mode_at_str)) ?
					 normal_mode_at_str :
					 "<empty>"),
					normal_mode_at_mem_slot);
			}
		} else {
			goto show_usage;
		}
		break;
	case LINK_CMD_NORMAL_MODE_AUTO:
		if (link_cmd_args.common_option == LINK_COMMON_READ) {
			link_sett_normal_mode_autoconn_shell_print();
		} else if (link_cmd_args.common_option ==
			   LINK_COMMON_ENABLE) {
			link_sett_save_normal_mode_autoconn_enabled(true, nmode_use_rel14);
		} else if (link_cmd_args.common_option ==
			   LINK_COMMON_DISABLE) {
			link_sett_save_normal_mode_autoconn_enabled(false, nmode_use_rel14);
		} else {
			goto show_usage;
		}
		break;

	case LINK_CMD_EDRX:
		if (link_cmd_args.common_option == LINK_COMMON_ENABLE) {
			char *value =
				NULL; /* Set with the defaults if not given */

			if (link_cmd_args.lte_mode == LTE_LC_LTE_MODE_NONE) {
				mosh_error("LTE mode is mandatory to be given. See usage:");
				goto show_usage;
			}

			if (edrx_value_set) {
				value = edrx_value_str;
			}

			ret = lte_lc_edrx_param_set(link_cmd_args.lte_mode,
						    value);
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

			ret = lte_lc_ptw_set(link_cmd_args.lte_mode, value);
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
		} else if (link_cmd_args.common_option ==
			   LINK_COMMON_DISABLE) {
			ret = lte_lc_edrx_req(false);
			if (ret < 0) {
				mosh_error("Cannot disable eDRX: %d", ret);
			} else {
				mosh_print("eDRX disabled");
			}
		} else {
			mosh_error("Unknown option for edrx command. See usage:");
			goto show_usage;
		}
		break;
	case LINK_CMD_PSM:
		if (link_cmd_args.common_option == LINK_COMMON_ENABLE) {
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
		} else if (link_cmd_args.common_option == LINK_COMMON_DISABLE) {
			ret = lte_lc_psm_req(false);
			if (ret < 0) {
				mosh_error("Cannot disable PSM: %d", ret);
			} else {
				mosh_print("PSM disabled");
			}
		} else if (link_cmd_args.common_option == LINK_COMMON_READ) {
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
			mosh_error("Unknown option for psm command. See usage:");
			goto show_usage;
		}
		break;

	case LINK_CMD_RSRP:
		(link_cmd_args.common_option == LINK_COMMON_SUBSCRIBE) ?
		link_rsrp_subscribe(true) :
		link_rsrp_subscribe(false);
		break;
	case LINK_CMD_NCELLMEAS:
		if (link_cmd_args.common_option == LINK_COMMON_STOP) {
			link_ncellmeas_start(false,
					     LINK_NCELLMEAS_MODE_NONE,
					     LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT);

		} else if (link_cmd_args.ncellmeasmode != LINK_NCELLMEAS_MODE_NONE) {
			mosh_print("Neighbor cell measurements and reporting starting");
			link_ncellmeas_start(true,
					     link_cmd_args.ncellmeasmode,
					     link_cmd_args.ncellmeas_search_type);
		} else {
			goto show_usage;
		}
		break;
	case LINK_CMD_MDMSLEEP:
		if (link_cmd_args.common_option == LINK_COMMON_SUBSCRIBE) {
			link_modem_sleep_notifications_subscribe(
				CONFIG_LTE_LC_MODEM_SLEEP_PRE_WARNING_TIME_MS,
				((threshold_time) ?
					 threshold_time :
					 CONFIG_LTE_LC_MODEM_SLEEP_NOTIFICATIONS_THRESHOLD_MS));
		} else {
			link_modem_sleep_notifications_unsubscribe();
		}
		break;
	case LINK_CMD_TAU:
		if (link_cmd_args.common_option == LINK_COMMON_SUBSCRIBE) {
			link_modem_tau_notifications_subscribe(
				CONFIG_LTE_LC_TAU_PRE_WARNING_TIME_MS,
				((threshold_time) ?
					 threshold_time :
					 CONFIG_LTE_LC_TAU_PRE_WARNING_THRESHOLD_MS));
		} else {
			link_modem_tau_notifications_unsubscribe();
		}
		break;
	case LINK_CMD_CONNECT: {
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
				mosh_error("Uknown auth protocol %d", protocol);
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
	} break;
	case LINK_CMD_RAI:
		if (link_cmd_args.common_option == LINK_COMMON_READ) {
			link_rai_read();
		} else if (link_cmd_args.common_option == LINK_COMMON_ENABLE) {
			link_rai_enable(true);
		} else if (link_cmd_args.common_option == LINK_COMMON_DISABLE) {
			link_rai_enable(false);
		} else {
			goto show_usage;
		}
		break;
	case LINK_CMD_DISCONNECT:
		ret = link_shell_pdn_disconnect(pdn_cid);
		break;
	default:
		mosh_error("Internal error. Unknown link command=%d", link_cmd_args.command);
		ret = -EINVAL;
		break;
	}
	return ret;

show_usage:
	/* Reset getopt for another users: */
	optreset = 1;

	link_shell_print_usage(&link_cmd_args);
	return ret;
}
