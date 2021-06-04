/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <shell/shell.h>

#include <modem/lte_lc.h>

#include "link.h"
#include "link_shell_print.h"

const char *
link_shell_print_sleep_type_to_string(enum lte_lc_modem_sleep_type sleep_type, char *out_str_buff)
{
	struct mapping_tbl_item const mapping_table[] = {
		{ LTE_LC_MODEM_SLEEP_PSM, "PSM" },
		{ LTE_LC_MODEM_SLEEP_RF_INACTIVITY, "RF inactivity" },
		{ LTE_LC_MODEM_SLEEP_FLIGHT_MODE, "flightmode" },
		{ -1, NULL }
	};

	return link_shell_map_to_string(mapping_table, sleep_type,
					 out_str_buff);
}

void link_shell_print_modem_sleep_notif(const struct shell *shell,
					 const struct lte_lc_evt *const evt)
{
	struct lte_lc_modem_sleep modem_sleep = evt->modem_sleep;
	char snum[64] = { 0 };
	float time_in_secs = modem_sleep.time / 1000;

	switch (evt->type) {
	case LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING:
		/** Modem sleep pre-warning
		 *  This event will be received a configurable amount of time before the modem
		 *  exits sleep. The time parameter associated with this event signifies the time
		 *  until modem exits sleep.
		 */
		shell_print(
			shell,
			"Modem sleep exit pre-warning: time: %.2f seconds, type: %s",
			time_in_secs,
			link_shell_print_sleep_type_to_string(modem_sleep.type,
							       snum));
		break;
	case LTE_LC_EVT_MODEM_SLEEP_EXIT:
		/** This event will be received when the modem exits sleep. */
		shell_print(shell,
			    "Modem sleep exit: time: %.2f seconds, type: %s",
			    time_in_secs,
			    link_shell_print_sleep_type_to_string(
				    modem_sleep.type, snum));
		break;
	case LTE_LC_EVT_MODEM_SLEEP_ENTER:
		/** This event will be received when the modem enters sleep.
		 *  The time parameter associated with this event signifies
		 *  the duration of the sleep.
		 */
		shell_print(shell,
			    "Modem sleep enter: time: %.2f seconds, type: %s",
			    time_in_secs,
			    link_shell_print_sleep_type_to_string(
				    modem_sleep.type, snum));
		break;
	default:
		shell_print(shell, "Unknown type of modem sleep event %d",
			    evt->type);
	}
}

const char *link_shell_funmode_to_string(int funmode, char *out_str_buff)
{
	struct mapping_tbl_item const mapping_table[] = {
		{ LTE_LC_FUNC_MODE_POWER_OFF, "power off" },
		{ LTE_LC_FUNC_MODE_NORMAL, "normal" },
		{ LTE_LC_FUNC_MODE_OFFLINE, "flightmode" },
		{ LTE_LC_FUNC_MODE_DEACTIVATE_LTE, "LTE off" },
		{ LTE_LC_FUNC_MODE_ACTIVATE_LTE, "LTE on" },
		{ LTE_LC_FUNC_MODE_DEACTIVATE_GNSS, "GNSS off" },
		{ LTE_LC_FUNC_MODE_ACTIVATE_GNSS, "GNSS on" },
		{ LTE_LC_FUNC_MODE_DEACTIVATE_UICC, "UICC off" },
		{ LTE_LC_FUNC_MODE_ACTIVATE_UICC, "UICC on" },
		{ LTE_LC_FUNC_MODE_OFFLINE_UICC_ON, "flightmode but UICC on" },
		{ LINK_FUNMODE_NONE, "unknown" },
		{ -1, NULL }
	};

	return link_shell_map_to_string(mapping_table, funmode, out_str_buff);
}

const char *link_shell_sysmode_to_string(int sysmode, char *out_str_buff)
{
	struct mapping_tbl_item const mapping_table[] = {
		{ LTE_LC_SYSTEM_MODE_NONE, "None" },
		{ LTE_LC_SYSTEM_MODE_LTEM, "LTE-M" },
		{ LTE_LC_SYSTEM_MODE_NBIOT, "NB-IoT" },
		{ LTE_LC_SYSTEM_MODE_LTEM_NBIOT, "LTE-M - NB-IoT" },
		{ LTE_LC_SYSTEM_MODE_GPS, "GNSS" },
		{ LTE_LC_SYSTEM_MODE_LTEM_GPS, "LTE-M - GNSS" },
		{ LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS, "LTE-M - NB-IoT - GNSS" },
		{ LTE_LC_SYSTEM_MODE_NBIOT_GPS, "NB-IoT - GNSS" },
		{ -1, NULL }
	};

	return link_shell_map_to_string(mapping_table, sysmode, out_str_buff);
}

const char *link_shell_sysmode_preferred_to_string(int sysmode_preference,
						    char *out_str_buff)
{
	struct mapping_tbl_item const mapping_table[] = {
		{ LTE_LC_SYSTEM_MODE_PREFER_AUTO,
		  "No preference, automatically selected by the modem" },
		{ LTE_LC_SYSTEM_MODE_PREFER_LTEM,
		  "LTE-M is preferred over PLMN selection" },
		{ LTE_LC_SYSTEM_MODE_PREFER_NBIOT,
		  "NB-IoT is preferred over PLMN selection" },
		{ LTE_LC_SYSTEM_MODE_PREFER_LTEM_PLMN_PRIO,
		  "LTE-M is preferred, but PLMN selection is more important" },
		{ LTE_LC_SYSTEM_MODE_PREFER_NBIOT_PLMN_PRIO,
		  "NB-IoT is preferred, but PLMN selection is more important" },
		{ -1, NULL }
	};

	return link_shell_map_to_string(mapping_table, sysmode_preference,
					 out_str_buff);
}

const char *link_shell_sysmode_currently_active_to_string(int actmode,
							   char *out_str_buff)
{
	struct mapping_tbl_item const mapping_table[] = {
		{ LTE_LC_LTE_MODE_NONE, "None" },
		{ LTE_LC_LTE_MODE_LTEM, "LTE-M" },
		{ LTE_LC_LTE_MODE_NBIOT, "NB-IoT" },
		{ -1, NULL }
	};

	return link_shell_map_to_string(mapping_table, actmode, out_str_buff);
}

void link_shell_print_reg_status(const struct shell *shell,
				  enum lte_lc_nw_reg_status reg_status)
{
	switch (reg_status) {
	case LTE_LC_NW_REG_NOT_REGISTERED:
		shell_print(shell,
			    "Network registration status: not registered");
		break;
	case LTE_LC_NW_REG_SEARCHING:
		shell_print(shell, "Network registration status: searching");
		break;
	case LTE_LC_NW_REG_REGISTRATION_DENIED:
		shell_print(shell, "Network registration status: denied");
		break;
	case LTE_LC_NW_REG_UNKNOWN:
		shell_print(shell, "Network registration status: unknown");
		break;
	case LTE_LC_NW_REG_UICC_FAIL:
		shell_print(shell, "Network registration status: UICC fail");
		break;
	case LTE_LC_NW_REG_REGISTERED_EMERGENCY:
		shell_print(
			shell,
			"Network registration status: Connected - emergency");
		break;
	case LTE_LC_NW_REG_REGISTERED_HOME:
	case LTE_LC_NW_REG_REGISTERED_ROAMING:
		shell_print(shell, "Network registration status: %s",
			    reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			    "Connected - home network" :
			    "Connected - roaming");
	default:
		break;
	}
}

const char *
link_shell_map_to_string(struct mapping_tbl_item const *mapping_table,
			  int mode, char *out_str_buff)
{
	bool found = false;
	int i;

	for (i = 0; mapping_table[i].key != -1; i++) {
		if (mapping_table[i].key == mode) {
			found = true;
			break;
		}
	}

	if (!found) {
		sprintf(out_str_buff,
			"%d (unknown value, not converted to string)", mode);
	} else {
		strcpy(out_str_buff, mapping_table[i].value_str);
	}
	return out_str_buff;
}
