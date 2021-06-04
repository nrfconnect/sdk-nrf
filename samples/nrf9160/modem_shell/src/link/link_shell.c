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

#include <modem/at_cmd.h>

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
	LINK_CMD_PSM
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

enum link_shell_ncellmeas_modes {
	LINK_NCELLMEAS_MODE_NONE = 0,
	LINK_NCELLMEAS_MODE_SINGLE
};

struct link_shell_cmd_args_t {
	enum link_shell_command command;
	enum link_shell_common_options common_option;
	enum link_shell_ncellmeas_modes ncellmeasmode;
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
	"  ncellmeas:    Start/stop neighbor cell measurements\n"
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
	"                default or with custom parameters\n";

static const char link_settings_usage_str[] =
	"Usage: link settings --read | --reset\n"
	"Options:\n"
	"  -r, --read,   Read and print current persistent settings\n"
	"      --reset,  Reset all persistent settings as their defaults\n";

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
	"Usage: link connect --apn <apn str> [--family <pdn family str>]\n"
	"Options:\n"
	"  -a, --apn, [str]    Access Point Name\n"
	"  -f, --family, [str] PDN family: 'ipv4v6', 'ipv4', 'ipv6', 'non-ip'\n"
	"\n"
	"Usage: link disconnect -I <cid>\n"
	"Options:\n"
	"  -I, --cid, [int]    Use this option to disconnect specific PDN CID\n";

static const char link_sysmode_usage_str[] =
	"Usage: link sysmode [options] | --read | --reset\n"
	"Options:\n"
	"  -r, --read,            Read system modes set in modem and by 'link sysmode'\n"
	"      --reset,           Reset the set sysmode as default\n"
	"  -m, --ltem,            Set LTE-M (LTE Cat-M1) system mode\n"
	"  -n, --nbiot,           Set NB-IoT (LTE Cat-NB1) system mode\n"
	"      --ltem_nbiot,      Set LTE-M + NB-IoT system mode\n"
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
	"Usage: link nmodeauto --read | --enable | --disable\n"
	"Options:\n"
	"  -r, --read,       Read and print current setting\n"
	"  -e, --enable,     Enable autoconnect (default)\n"
	"  -d, --disable,    Disable autoconnect\n";

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
	"Usage: link ncellmeas --single | --cancel\n"
	"Options:\n"
	"      --single, Start a neighbor cell measurement and report result\n"
	"      --cancel, Cancel/Stop neighbor cell measurement if still on going\n";

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

/******************************************************************************/

/* Following are not having short options: */
enum {
	LINK_SHELL_OPT_MEM_SLOT_1 = 1001,
	LINK_SHELL_OPT_MEM_SLOT_2,
	LINK_SHELL_OPT_MEM_SLOT_3,
	LINK_SHELL_OPT_RESET,
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
	{ "threshold", required_argument, 0, LINK_SHELL_OPT_THRESHOLD_TIME },
	{ 0, 0, 0, 0 }
};

/******************************************************************************/

static void link_shell_print_usage(const struct shell *shell,
				    struct link_shell_cmd_args_t *link_cmd_args)
{
	switch (link_cmd_args->command) {
	case LINK_CMD_SETTINGS:
		shell_print(shell, "%s", link_settings_usage_str);
		break;
	case LINK_CMD_DEFCONT:
		shell_print(shell, "%s", link_defcont_usage_str);
		break;
	case LINK_CMD_DEFCONTAUTH:
		shell_print(shell, "%s", link_defcontauth_usage_str);
		break;
	case LINK_CMD_CONNECT:
	case LINK_CMD_DISCONNECT:
		shell_print(shell, "%s", link_connect_usage_str);
		break;
	case LINK_CMD_SYSMODE:
		shell_print(shell, "%s", link_sysmode_usage_str);
		break;
	case LINK_CMD_FUNMODE:
		shell_print(shell, "%s", link_funmode_usage_str);
		break;
	case LINK_CMD_NORMAL_MODE_AT:
		shell_print(shell, "%s", link_normal_mode_at_usage_str);
		break;
	case LINK_CMD_NORMAL_MODE_AUTO:
		shell_print(shell, "%s", link_normal_mode_auto_usage_str);
		break;
	case LINK_CMD_EDRX:
		shell_print(shell, "%s", link_edrx_usage_str);
		break;
	case LINK_CMD_PSM:
		shell_print(shell, "%s", link_psm_usage_str);
		break;
	case LINK_CMD_RSRP:
		shell_print(shell, "%s", link_rsrp_usage_str);
		break;
	case LINK_CMD_NCELLMEAS:
		shell_print(shell, "%s", link_ncellmeas_usage_str);
		break;
	case LINK_CMD_MDMSLEEP:
		shell_print(shell, "%s", link_msleep_usage_str);
		break;
	case LINK_CMD_TAU:
		shell_print(shell, "%s", link_tau_usage_str);
		break;
	default:
		shell_print(shell, "%s", link_usage_str);
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
	link_cmd_args->ncellmeasmode = LINK_NCELLMEAS_MODE_NONE;
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

static void link_shell_sysmode_set(const struct shell *shell, int sysmode,
				    int lte_pref)
{
	enum lte_lc_func_mode functional_mode;
	char snum[64];
	int ret = lte_lc_system_mode_set(sysmode, lte_pref);

	if (ret < 0) {
		shell_error(shell, "Cannot set system mode to modem: %d", ret);
		ret = lte_lc_func_mode_get(&functional_mode);
		if (functional_mode != LTE_LC_FUNC_MODE_OFFLINE ||
		    functional_mode != LTE_LC_FUNC_MODE_POWER_OFF) {
			shell_warn(
				shell,
				"Requested mode couldn't set to modem. "
				"Not in flighmode nor in pwroff?");
		}
	} else {
		shell_print(shell, "System mode set successfully to modem: %s",
			    link_shell_sysmode_to_string(sysmode, snum));
	}
}

/******************************************************************************/

int link_shell_get_and_print_current_system_modes(
	const struct shell *shell, enum lte_lc_system_mode *sys_mode_current,
	enum lte_lc_system_mode_preference *sys_mode_preferred,
	enum lte_lc_lte_mode *currently_active_mode)
{
	int ret = -1;

	char snum[64];

	ret = lte_lc_system_mode_get(sys_mode_current, sys_mode_preferred);
	if (ret >= 0) {
		shell_print(shell, "Modem config for system mode: %s",
			    link_shell_sysmode_to_string(*sys_mode_current,
							  snum));
		shell_print(shell, "Modem config for LTE preference: %s",
			    link_shell_sysmode_preferred_to_string(
				    *sys_mode_preferred, snum));
	} else {
		return ret;
	}

	ret = lte_lc_lte_mode_get(currently_active_mode);
	if (ret >= 0) {
		shell_print(shell, "Currently active system mode: %s",
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
	} else {
		shell_error(shell, "Unsupported command=%s\n", argv[1]);
		ret = -EINVAL;
		goto show_usage;
	}

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
				shell_error(
					shell,
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
				shell_error(shell,
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
				shell_error(
					shell,
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
				shell_error(shell,
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
				shell_error(
					shell,
					"PDN CID (%d) must be positive integer. "
					"Default PDN context (CID=0) cannot be given.",
					pdn_cid);
				return -EINVAL;
			}
			break;
		case 'a': /* APN */
			apn_len = strlen(optarg);
			if (apn_len > LINK_APN_STR_MAX_LENGTH) {
				shell_error(
					shell,
					"APN string length %d exceeded. Maximum is %d.",
					apn_len, LINK_APN_STR_MAX_LENGTH);
				ret = -EINVAL;
				goto show_usage;
			}
			apn = optarg;
			break;
		case 'f': /* Address family */
			family = optarg;
			break;
		case 'A': /* defcont auth protocol */
			protocol = atoi(optarg);
			protocol_given = true;
			break;
		case 'U': /* defcont auth username */
			username = optarg;
			break;
		case 'P': /* defcont auth password */
			password = optarg;
			break;
		/* Options without short option: */
		case LINK_SHELL_OPT_SINGLE:
			link_cmd_args.ncellmeasmode = LINK_NCELLMEAS_MODE_SINGLE;
			break;
		case LINK_SHELL_OPT_RESET:
			link_cmd_args.common_option = LINK_COMMON_RESET;
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
				shell_error(
					shell,
					"Not a valid number for --threshold_time (milliseconds).");
				return -EINVAL;
			}
			break;
		case '?':
			goto show_usage;
		default:
			shell_error(shell, "Unknown option. See usage:");
			goto show_usage;
		}
	}

	/* Check that all mandatory args were given: */
	if (require_apn && apn == NULL) {
		shell_error(shell,
			    "Option -a | -apn MUST be given. See usage:");
		goto show_usage;
	} else if (require_pdn_cid && pdn_cid == 0) {
		shell_error(shell, "-I / --cid MUST be given. See usage:");
		goto show_usage;
	} else if (require_subscribe &&
		   link_cmd_args.common_option == LINK_COMMON_NONE) {
		shell_error(shell, "Either -s or -u MUST be given. See usage:");
		goto show_usage;
	} else if (require_option &&
		   link_cmd_args.funmode_option == LINK_FUNMODE_NONE &&
		   link_cmd_args.sysmode_option == LTE_LC_SYSTEM_MODE_NONE &&
		   link_cmd_args.ncellmeasmode == LINK_NCELLMEAS_MODE_NONE &&
		   link_cmd_args.common_option == LINK_COMMON_NONE) {
		shell_error(shell,
			    "Command needs option to be given. See usage:");
		goto show_usage;
	}

	char snum[64];

	switch (link_cmd_args.command) {
	case LINK_CMD_DEFCONT:
		if (link_cmd_args.common_option == LINK_COMMON_READ) {
			link_sett_defcont_conf_shell_print(shell);
		} else if (link_cmd_args.common_option ==
			   LINK_COMMON_ENABLE) {
			link_sett_save_defcont_enabled(true);
		} else if (link_cmd_args.common_option ==
			   LINK_COMMON_DISABLE) {
			static const char cgdcont[] = "AT+CGDCONT=0";

			if (at_cmd_write(cgdcont, NULL, 0, NULL) != 0) {
				shell_warn(
					shell,
					"ERROR from modem. Getting the initial PDP context back "
					"wasn't successful.");
				shell_warn(
					shell,
					"Please note: you might need to visit the pwroff state to "
					"make an impact to modem.");
			}
			link_sett_save_defcont_enabled(false);
			shell_print(shell,
				    "Custom default context config disabled.");
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
				shell_error(shell, "Unknown PDN family %s",
					    family);
				goto show_usage;
			} else {
				(void)link_sett_save_defcont_pdn_family(
					pdn_lib_fam);
			}
		}
		break;
	case LINK_CMD_DEFCONTAUTH:
		if (link_cmd_args.common_option == LINK_COMMON_READ) {
			link_sett_defcontauth_conf_shell_print(shell);
		} else if (link_cmd_args.common_option ==
			   LINK_COMMON_ENABLE) {
			if (link_sett_save_defcontauth_enabled(true) < 0) {
				shell_warn(shell,
					   "Cannot enable authentication.");
			}
		} else if (link_cmd_args.common_option ==
			   LINK_COMMON_DISABLE) {
			static const char cgauth[] = "AT+CGAUTH=0,0";

			if (at_cmd_write(cgauth, NULL, 0, NULL) != 0) {
				shell_warn(
					shell,
					"Disabling of auth cannot be done to modem.");
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
			shell_warn(shell,
				   "Cannot get functional mode from modem: %d",
				   ret);
		} else {
			shell_print(shell, "Modem functional mode: %s",
				    link_shell_funmode_to_string(
					    functional_mode, snum));
		}
		ret = lte_lc_nw_reg_status_get(&current_reg_status);
		if (ret >= 0) {
			link_shell_print_reg_status(shell, current_reg_status);
		} else {
			shell_error(
				shell,
				"Cannot get current registration status (%d)",
				ret);
		}
		if (current_reg_status == LTE_LC_NW_REG_REGISTERED_EMERGENCY ||
		    current_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ||
		    current_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING) {
			connected = true;
		}

		link_api_modem_info_get_for_shell(shell, connected);
		break;
	}
	case LINK_CMD_SETTINGS:
		if (link_cmd_args.common_option == LINK_COMMON_READ) {
			link_sett_all_print(shell);
		} else if (link_cmd_args.common_option == LINK_COMMON_RESET) {
			link_sett_defaults_set(shell);
			link_shell_sysmode_set(shell, SYS_MODE_PREFERRED,
						CONFIG_LTE_MODE_PREFERENCE);
		} else {
			goto show_usage;
		}
		break;
	case LINK_CMD_CONEVAL:
		link_api_coneval_read_for_shell(shell);
		break;

	case LINK_CMD_SYSMODE:
		if (link_cmd_args.common_option == LINK_COMMON_READ) {
			enum lte_lc_system_mode sys_mode_current;
			enum lte_lc_system_mode_preference sys_mode_pref_current;
			enum lte_lc_lte_mode currently_active_mode;

			ret = link_shell_get_and_print_current_system_modes(
				shell, &sys_mode_current,
				&sys_mode_pref_current, &currently_active_mode);
			if (ret < 0) {
				shell_error(
					shell,
					"Cannot read system mode of the modem: %d",
					ret);
			} else {
				enum lte_lc_system_mode sett_sys_mode;
				enum lte_lc_system_mode_preference sett_lte_pref;

				/* Print also settings stored in mosh side: */
				link_sett_sysmode_print(shell);
				sett_sys_mode = link_sett_sysmode_get();
				sett_lte_pref =
					link_sett_sysmode_lte_preference_get();
				if (sett_sys_mode != LTE_LC_SYSTEM_MODE_NONE &&
				    sett_sys_mode != sys_mode_current &&
				    sett_lte_pref != sys_mode_pref_current) {
					shell_warn(
						shell,
						"note: seems that set link sysmode and a "
						"counterparts in modem are not in synch");
					shell_warn(
						shell,
						"but no worries; requested system mode retried "
						"next time when going to normal mode");
				}
			}
		} else if (link_cmd_args.sysmode_option !=
			   LTE_LC_SYSTEM_MODE_NONE) {
			link_shell_sysmode_set(
				shell, link_cmd_args.sysmode_option,
				link_cmd_args.sysmode_lte_pref_option);

			/* Save system modem to link settings: */
			(void)link_sett_sysmode_save(
				link_cmd_args.sysmode_option,
				link_cmd_args.sysmode_lte_pref_option);

		} else if (link_cmd_args.common_option == LINK_COMMON_RESET) {
			link_shell_sysmode_set(shell, SYS_MODE_PREFERRED,
						CONFIG_LTE_MODE_PREFERENCE);

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
				shell_error(shell,
					    "Cannot get functional mode: %d",
					    ret);
			} else {
				shell_print(
					shell,
					"Functional mode read successfully: %s",
					link_shell_funmode_to_string(
						functional_mode, snum));
			}
		} else if (link_cmd_args.funmode_option !=
			   LINK_FUNMODE_NONE) {
			ret = link_func_mode_set(
				link_cmd_args.funmode_option);
			if (ret < 0) {
				shell_error(shell,
					    "Cannot set functional mode: %d",
					    ret);
			} else {
				shell_print(
					shell,
					"Functional mode set successfully: %s",
					link_shell_funmode_to_string(
						link_cmd_args.funmode_option,
						snum));
			}
		} else {
			goto show_usage;
		}
		break;
	case LINK_CMD_NORMAL_MODE_AT:
		if (link_cmd_args.common_option == LINK_COMMON_READ) {
			link_sett_normal_mode_at_cmds_shell_print(shell);
		} else if (normal_mode_at_str != NULL) {
			ret = link_sett_save_normal_mode_at_cmd_str(
				normal_mode_at_str, normal_mode_at_mem_slot);
			if (ret < 0) {
				shell_error(
					shell,
					"Cannot set normal mode AT-command: \"%s\"",
					normal_mode_at_str);
			} else {
				shell_print(
					shell,
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
			link_sett_normal_mode_autoconn_shell_print(shell);
		} else if (link_cmd_args.common_option ==
			   LINK_COMMON_ENABLE) {
			link_sett_save_normal_mode_autoconn_enabled(true);
		} else if (link_cmd_args.common_option ==
			   LINK_COMMON_DISABLE) {
			link_sett_save_normal_mode_autoconn_enabled(false);
		} else {
			goto show_usage;
		}
		break;

	case LINK_CMD_EDRX:
		if (link_cmd_args.common_option == LINK_COMMON_ENABLE) {
			char *value =
				NULL; /* Set with the defaults if not given */

			if (link_cmd_args.lte_mode == LTE_LC_LTE_MODE_NONE) {
				shell_error(
					shell,
					"LTE mode is mandatory to be given. See usage:");
				goto show_usage;
			}

			if (edrx_value_set) {
				value = edrx_value_str;
			}

			ret = lte_lc_edrx_param_set(link_cmd_args.lte_mode,
						    value);
			if (ret < 0) {
				shell_error(
					shell,
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
				shell_error(
					shell,
					"Cannot set PTW value %s, error: %d",
					((value == NULL) ? "NULL" : value),
					ret);
				return -EINVAL;
			}

			ret = lte_lc_edrx_req(true);
			if (ret < 0) {
				shell_error(shell, "Cannot enable eDRX: %d",
					    ret);
			} else {
				shell_print(shell, "eDRX enabled");
			}
		} else if (link_cmd_args.common_option ==
			   LINK_COMMON_DISABLE) {
			ret = lte_lc_edrx_req(false);
			if (ret < 0) {
				shell_error(shell, "Cannot disable eDRX: %d",
					    ret);
			} else {
				shell_print(shell, "eDRX disabled");
			}
		} else {
			shell_error(
				shell,
				"Unknown option for edrx command. See usage:");
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
				shell_error(
					shell,
					"Cannot set PSM parameters: error %d",
					ret);
				shell_error(shell, "  rptau %s, rat %s",
					    ((rptau_bit_value == NULL) ?
					     "NULL" :
					     rptau_bit_value),
					    ((rat_bit_value == NULL) ?
					     "NULL" :
					     rat_bit_value));
				return -EINVAL;
			}

			ret = lte_lc_psm_req(true);
			if (ret < 0) {
				shell_error(shell, "Cannot enable PSM: %d",
					    ret);
			} else {
				shell_print(shell, "PSM enabled");
			}
		} else if (link_cmd_args.common_option ==
			   LINK_COMMON_DISABLE) {
			ret = lte_lc_psm_req(false);
			if (ret < 0) {
				shell_error(shell, "Cannot disable PSM: %d",
					    ret);
			} else {
				shell_print(shell, "PSM disabled");
			}
		} else if (link_cmd_args.common_option == LINK_COMMON_READ) {
			int tau, active_time;

			ret = lte_lc_psm_get(&tau, &active_time);
			if (ret < 0) {
				shell_error(shell, "Cannot get PSM configs: %d",
					    ret);
			} else {
				shell_print(
					shell,
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
			shell_error(
				shell,
				"Unknown option for psm command. See usage:");
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
			link_ncellmeas_start(false);

		} else if (link_cmd_args.ncellmeasmode == LINK_NCELLMEAS_MODE_SINGLE) {
			link_ncellmeas_start(true);
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
	case LINK_CMD_CONNECT:
		ret = link_shell_pdn_connect(shell, apn, family);
		break;
	case LINK_CMD_DISCONNECT:
		ret = link_shell_pdn_disconnect(shell, pdn_cid);
		break;
	default:
		shell_error(shell, "Internal error. Unknown link command=%d",
			    link_cmd_args.command);
		ret = -EINVAL;
		break;
	}
	return ret;

show_usage:
	link_shell_print_usage(shell, &link_cmd_args);
	return ret;
}
