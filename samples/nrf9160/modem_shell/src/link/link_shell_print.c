/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include <zephyr/shell/shell.h>

#include <modem/lte_lc.h>

#include "link.h"
#include "link_shell_print.h"
#include "mosh_print.h"

const char *link_shell_print_sleep_time_to_string(uint64_t sleep_time, char *out_str_buff)
{
	if (sleep_time == -1) {
		strcpy(out_str_buff, "infinite");
	} else {
		sprintf(out_str_buff, "%.2f seconds", sleep_time / 1000.0);
	}

	return out_str_buff;
}

const char *
link_shell_print_sleep_type_to_string(enum lte_lc_modem_sleep_type sleep_type, char *out_str_buff)
{
	struct mapping_tbl_item const mapping_table[] = {
		{ LTE_LC_MODEM_SLEEP_PSM, "PSM" },
		{ LTE_LC_MODEM_SLEEP_RF_INACTIVITY, "RF inactivity" },
		{ LTE_LC_MODEM_SLEEP_LIMITED_SERVICE, "limited service"},
		{ LTE_LC_MODEM_SLEEP_FLIGHT_MODE, "flight mode" },
		{ LTE_LC_MODEM_SLEEP_PROPRIETARY_PSM, "proprietary PSM" },
		{ -1, NULL }
	};

	return link_shell_map_to_string(mapping_table, sleep_type, out_str_buff);
}

void link_shell_print_modem_sleep_notif(const struct lte_lc_evt *const evt)
{
	struct lte_lc_modem_sleep modem_sleep = evt->modem_sleep;
	char stime[32] = { 0 };
	char stype[64] = { 0 };

	switch (evt->type) {
	case LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING:
		/** Modem sleep pre-warning
		 *  This event will be received a configurable amount of time before the modem
		 *  exits sleep. The time parameter associated with this event signifies the time
		 *  until modem exits sleep.
		 */
		mosh_print(
			"Modem sleep exit pre-warning: Time: %s, Type: %s",
			link_shell_print_sleep_time_to_string(modem_sleep.time, stime),
			link_shell_print_sleep_type_to_string(modem_sleep.type, stype));
		break;
	case LTE_LC_EVT_MODEM_SLEEP_EXIT:
		/** This event will be received when the modem exits sleep. */
		mosh_print(
			"Modem sleep exit: Type: %s",
			link_shell_print_sleep_type_to_string(modem_sleep.type, stype));
		break;
	case LTE_LC_EVT_MODEM_SLEEP_ENTER:
		/** This event will be received when the modem enters sleep.
		 *  The time parameter associated with this event signifies
		 *  the duration of the sleep.
		 */
		mosh_print(
			"Modem sleep enter: Time: %s, Type: %s",
			link_shell_print_sleep_time_to_string(modem_sleep.time, stime),
			link_shell_print_sleep_type_to_string(modem_sleep.type, stype));
		break;
	default:
		mosh_print("Unknown type of modem sleep event %d", evt->type);
	}
}

void link_shell_print_modem_domain_event(enum lte_lc_modem_evt modem_evt)
{
	switch (modem_evt) {
	case LTE_LC_MODEM_EVT_LIGHT_SEARCH_DONE:
		mosh_print("Modem domain event: Light search done");
		break;
	case LTE_LC_MODEM_EVT_SEARCH_DONE:
		mosh_print("Modem domain event: Search done");
		break;
	case LTE_LC_MODEM_EVT_RESET_LOOP:
		mosh_print("Modem domain event: Reset loop");
		break;
	case LTE_LC_MODEM_EVT_BATTERY_LOW:
		mosh_print("Modem domain event: Battery low");
		break;
	case LTE_LC_MODEM_EVT_OVERHEATED:
		mosh_print("Modem domain event: Overheated");
		break;
	case LTE_LC_MODEM_EVT_NO_IMEI:
		mosh_print("Modem domain event: No IMEI");
		break;
	case LTE_LC_MODEM_EVT_CE_LEVEL_0:
		mosh_print("Modem domain event: CE-level 0");
		break;
	case LTE_LC_MODEM_EVT_CE_LEVEL_1:
		mosh_print("Modem domain event: CE-level 1");
		break;
	case LTE_LC_MODEM_EVT_CE_LEVEL_2:
		mosh_print("Modem domain event: CE-level 2");
		break;
	case LTE_LC_MODEM_EVT_CE_LEVEL_3:
		mosh_print("Modem domain event: CE-level 3");
		break;
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

const char *link_shell_redmob_mode_to_string(int funmode, char *out_str_buff)
{
	struct mapping_tbl_item const mapping_table[] = {
		{ LTE_LC_REDUCED_MOBILITY_DEFAULT, "default" },
		{ LTE_LC_REDUCED_MOBILITY_NORDIC, "nordic" },
		{ LTE_LC_REDUCED_MOBILITY_DISABLED, "disabled" },
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

void link_shell_print_reg_status(enum lte_lc_nw_reg_status reg_status)
{
	switch (reg_status) {
	case LTE_LC_NW_REG_NOT_REGISTERED:
		mosh_print("Network registration status: not registered");
		break;
	case LTE_LC_NW_REG_SEARCHING:
		mosh_print("Network registration status: searching");
		break;
	case LTE_LC_NW_REG_REGISTRATION_DENIED:
		mosh_print("Network registration status: denied");
		break;
	case LTE_LC_NW_REG_UNKNOWN:
		mosh_print("Network registration status: unknown");
		break;
	case LTE_LC_NW_REG_UICC_FAIL:
		mosh_print("Network registration status: UICC fail");
		break;
	case LTE_LC_NW_REG_REGISTERED_EMERGENCY:
		mosh_print("Network registration status: Connected - emergency");
		break;
	case LTE_LC_NW_REG_REGISTERED_HOME:
	case LTE_LC_NW_REG_REGISTERED_ROAMING:
		mosh_print(
			"Network registration status: %s",
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
