/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#include <shell/shell.h>
#include <shell/shell_uart.h>

#include <modem/at_cmd.h>
#include <modem/modem_info.h>
#include <modem/lte_lc.h>
#include <modem/pdn.h>

#include <nrf_socket.h>

#include "link_settings.h"
#include "link_shell.h"
#include "link_shell_print.h"

#include "link_shell_pdn.h"
#include "link_api.h"
#include "link.h"

#if defined(CONFIG_MOSH_SMS)
#include "sms.h"
#endif

#if defined(CONFIG_MODEM_INFO)
#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>
#endif

static bool link_subscribe_for_rsrp;

extern const struct shell *shell_global;

#if defined(CONFIG_MODEM_INFO)
/* System work queue for getting the modem info that ain't in lte connection ind: */
static struct k_work modem_info_work;

/* Work queue for signal info: */
static struct k_work modem_info_signal_work;
#define LINK_RSRP_VALUE_NOT_KNOWN -999
static int32_t modem_rsrp = LINK_RSRP_VALUE_NOT_KNOWN;

/******************************************************************************/

static void link_modem_info_work(struct k_work *unused)
{
	ARG_UNUSED(unused);

	/* Seems that 1st info read fails without this. Thus, let modem have some time: */
	k_sleep(K_MSEC(1500));

	link_api_modem_info_get_for_shell(shell_global, true);
}

/******************************************************************************/

static void link_rsrp_signal_handler(char rsrp_value)
{
	modem_rsrp = (int8_t)rsrp_value - MODEM_INFO_RSRP_OFFSET_VAL;
	k_work_submit(&modem_info_signal_work);
}

/******************************************************************************/

#define MOSH_RSRP_UPDATE_INTERVAL_IN_SECS 5
static void link_rsrp_signal_update(struct k_work *work)
{
	static uint32_t timestamp_prev;

	if ((timestamp_prev != 0) &&
	    (k_uptime_get_32() - timestamp_prev <
	     MOSH_RSRP_UPDATE_INTERVAL_IN_SECS * MSEC_PER_SEC)) {
		return;
	}

	if (link_subscribe_for_rsrp && shell_global != NULL) {
		shell_print(shell_global, "RSRP: %d", modem_rsrp);
	}
	timestamp_prev = k_uptime_get_32();
}
#endif

/******************************************************************************/

void link_init(void)
{
#if defined(CONFIG_MODEM_INFO)
	k_work_init(&modem_info_work, link_modem_info_work);
	k_work_init(&modem_info_signal_work, link_rsrp_signal_update);
	modem_info_rsrp_register(link_rsrp_signal_handler);
#endif

	/* TODO CHECK THIS */
	//shell_global = shell_backend_uart_get_ptr();

	link_sett_init(shell_global);

	link_shell_pdn_init(shell_global);

	lte_lc_register_handler(link_ind_handler);

/* With CONFIG_LWM2M_CARRIER, MoSH auto connect must be disabled
 * because LwM2M carrier lib handles that.
 */
#if !defined(CONFIG_LWM2M_CARRIER)
	if (link_sett_is_normal_mode_autoconn_enabled() == true) {
		link_func_mode_set(LTE_LC_FUNC_MODE_NORMAL);
	}
#endif
}

void link_ind_handler(const struct lte_lc_evt *const evt)
{
	char snum[64];

	switch (evt->type) {
	case LTE_LC_EVT_TAU_PRE_WARNING:
		/** Tracking Area Update pre-warning.
		 *  This event will be received a configurable amount of time before TAU is
		 *  scheduled to occur. This gives the application the opportunity to send data
		 *  over the network before the TAU happens, thus saving power by avoiding
		 *  sending data and the TAU separately.
		 */
		shell_print(shell_global, "TAU pre warning: time %lld",
			    evt->time);
		break;
	case LTE_LC_EVT_NEIGHBOR_CELL_MEAS: {
		int i;
		struct lte_lc_cells_info cells = evt->cells_info;
		struct lte_lc_cell cur_cell = cells.current_cell;

		/* Current cell: */
		if (cur_cell.id) {
			shell_print(shell_global, "Current cell:");

			shell_print(
				shell_global,
				"    ID %d, phy ID %d, MCC %d MNC %d, RSRP %d : %ddBm, RSRQ %d, TAC %d, earfcn %d, meas time %lld, TA %d",
				cur_cell.id, cur_cell.phys_cell_id,
				cur_cell.mcc, cur_cell.mnc, cur_cell.rsrp,
				cur_cell.rsrp - MODEM_INFO_RSRP_OFFSET_VAL,
				cur_cell.rsrq, cur_cell.tac, cur_cell.earfcn,
				cur_cell.measurement_time,
				cur_cell.timing_advance);
		} else {
			shell_print(shell_global,
				    "No current cell information from modem.");
		}

		if (!cells.ncells_count) {
			shell_print(shell_global,
				    "No neighbor cell information from modem.");
		}

		for (i = 0; i < cells.ncells_count; i++) {
			/* Neighbor cells: */
			shell_print(shell_global, "Neighbor cell %d", i + 1);
			shell_print(
				shell_global,
				"    phy ID %d, RSRP %d : %ddBm, RSRQ %d, earfcn %d, timediff %d",
				cells.neighbor_cells[i].phys_cell_id,
				cells.neighbor_cells[i].rsrp,
				cells.neighbor_cells[i].rsrp -
				MODEM_INFO_RSRP_OFFSET_VAL,
				cells.neighbor_cells[i].rsrq,
				cells.neighbor_cells[i].earfcn,
				cells.neighbor_cells[i].time_diff);
		}
	} break;
	case LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING:
	case LTE_LC_EVT_MODEM_SLEEP_ENTER:
	case LTE_LC_EVT_MODEM_SLEEP_EXIT:
		link_shell_print_modem_sleep_notif(shell_global, evt);
		break;
	case LTE_LC_EVT_LTE_MODE_UPDATE:
		/** The currently active LTE mode is updated. If a system mode that
		 *  enables both LTE-M and NB-IoT is configured, the modem may change
		 *  the currently active LTE mode based on the system mode preference
		 *  and network availability. This event will then indicate which
		 *  LTE mode is currently used by the modem.
		 */
		shell_print(shell_global, "Currently active system mode: %s",
			    link_shell_sysmode_currently_active_to_string(
				    evt->lte_mode, snum));
		break;
	case LTE_LC_EVT_NW_REG_STATUS:
		link_shell_print_reg_status(shell_global, evt->nw_reg_status);

#if defined(CONFIG_MODEM_INFO)
		if (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_EMERGENCY ||
		    evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ||
		    evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING) {
			k_work_submit(&modem_info_work);
		}
#endif
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		shell_print(shell_global,
			    "LTE cell changed: Cell ID: %d, Tracking area: %d",
			    evt->cell.id, evt->cell.tac);
		break;
	case LTE_LC_EVT_RRC_UPDATE:
		shell_print(shell_global, "RRC mode: %s",
			    evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			    "Connected" :
			    "Idle");
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		shell_print(
			shell_global,
			"PSM parameter update: TAU: %d, Active time: %d seconds",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintf(log_buf, sizeof(log_buf),
			       "eDRX parameter update: eDRX: %f, PTW: %f",
			       evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			shell_print(shell_global, "%s", log_buf);
		}
		break;
	}
	default:
		break;
	}
}

/******************************************************************************/

static int link_default_pdp_context_set(void)
{
	int ret;

	if (link_sett_is_defcont_enabled() == true) {
		ret = pdn_ctx_configure(0, link_sett_defcont_apn_get(),
					link_sett_defcont_pdn_family_get(),
					NULL);
		if (ret) {
			shell_error(shell_global, "pdn_ctx_configure returned err %d", ret);
			return ret;
		}
	}
	return 0;
}
static int link_default_pdp_context_auth_set(void)
{
	int ret;

	if (link_sett_is_defcontauth_enabled() == true) {
		ret = pdn_ctx_auth_set(0, link_sett_defcontauth_prot_get(),
				       link_sett_defcontauth_username_get(),
				       link_sett_defcontauth_password_get());

		if (ret) {
			shell_error(shell_global,
				    "pdn_ctx_auth_set returned err  %d", ret);
			return ret;
		}
	}
	return 0;
}

static int link_normal_mode_at_cmds_run(void)
{
	char *normal_mode_at_cmd;
	char response[MOSH_AT_CMD_RESPONSE_MAX_LEN + 1];
	int mem_slot_index = LINK_SETT_NMODEAT_MEM_SLOT_INDEX_START;
	int len;

	for (; mem_slot_index <= LINK_SETT_NMODEAT_MEM_SLOT_INDEX_END;
	     mem_slot_index++) {
		normal_mode_at_cmd =
			link_sett_normal_mode_at_cmd_str_get(mem_slot_index);
		len = strlen(normal_mode_at_cmd);
		if (len) {
			if (at_cmd_write(normal_mode_at_cmd, response,
					 sizeof(response), NULL) != 0) {
				shell_error(
					shell_global,
					"Normal mode AT-command from memory slot %d \"%s\" returned: ERROR",
					mem_slot_index, normal_mode_at_cmd);
			} else {
				shell_print(
					shell_global,
					"Normal mode AT-command from memory slot %d \"%s\" returned:\n\r %s OK",
					mem_slot_index, normal_mode_at_cmd,
					response);
			}
		}
	}

	return 0;
}

void link_rsrp_subscribe(bool subscribe)
{
	link_subscribe_for_rsrp = subscribe;
	if (shell_global != NULL) {
		if (link_subscribe_for_rsrp) {
			/* print current value right away: */
			shell_print(shell_global, "RSRP subscribed");
			if (modem_rsrp != LINK_RSRP_VALUE_NOT_KNOWN) {
				shell_print(shell_global, "RSRP: %d", modem_rsrp);
			}
		} else {
			shell_print(shell_global, "RSRP unsubscribed");
		}
	}
}

void link_ncellmeas_start(bool start)
{
	int ret;

	if (start) {
		shell_print(
			shell_global,
			"Neighbor cell measurements and reporting starting");
		ret = lte_lc_neighbor_cell_measurement();
		if (shell_global != NULL) {
			if (ret) {
				shell_error(
					shell_global,
					"lte_lc_neighbor_cell_measurement() returned err %d",
					ret);
				shell_error(
					shell_global,
					"Cannot start neigbor measurements");
			}
		}
	} else {
		ret = lte_lc_neighbor_cell_measurement_cancel();
		if (shell_global != NULL) {
			if (ret) {
				shell_error(
					shell_global,
					"lte_lc_neighbor_cell_measurement_cancel() returned err %d",
					ret);
			} else {
				shell_print(
					shell_global,
					"Neighbor cell measurements and reporting stopped");
			}
		}
	}
}

#define AT_MDM_SLEEP_NOTIF_START "AT%%XMODEMSLEEP=1,%d,%d"
#define AT_MDM_SLEEP_NOTIF_STOP "AT%XMODEMSLEEP=0"
void link_modem_sleep_notifications_subscribe(uint32_t warn_time_ms,
					       uint32_t threshold_ms)
{
	char buf_sub[48];
	int err;

	snprintk(buf_sub, sizeof(buf_sub), AT_MDM_SLEEP_NOTIF_START,
		 warn_time_ms, threshold_ms);

	err = at_cmd_write(buf_sub, NULL, 0, NULL);
	if (err) {
		shell_error(
			shell_global,
			"Cannot subscribe to modem sleep notifications, err %d",
			err);
	} else {
		shell_print(shell_global,
			    "Subscribed to modem sleep notifications");
	}
}

void link_modem_sleep_notifications_unsubscribe(void)
{
	int err;

	err = at_cmd_write(AT_MDM_SLEEP_NOTIF_STOP, NULL, 0, NULL);
	if (err) {
		shell_error(shell_global,
			    "Cannot stop modem sleep notifications, err %d",
			    err);
	} else {
		shell_print(shell_global,
			    "Unsubscribed from modem sleep notifications");
	}
}

#define AT_TAU_NOTIF_START "AT%%XT3412=1,%d,%d"
#define AT_TAU_NOTIF_STOP "AT%XT3412=0"
void link_modem_tau_notifications_subscribe(uint32_t warn_time_ms,
					     uint32_t threshold_ms)
{
	char buf_sub[48];
	int err;

	snprintk(buf_sub, sizeof(buf_sub), AT_TAU_NOTIF_START, warn_time_ms,
		 threshold_ms);

	err = at_cmd_write(buf_sub, NULL, 0, NULL);
	if (err) {
		shell_error(shell_global,
			    "Cannot subscribe to TAU notifications, err %d",
			    err);
	} else {
		shell_print(shell_global,
			    "Subscribed to TAU pre-warning notifications");
	}
}

void link_modem_tau_notifications_unsubscribe(void)
{
	int err;

	err = at_cmd_write(AT_TAU_NOTIF_STOP, NULL, 0, NULL);
	if (err) {
		shell_error(shell_global,
			    "Cannot stop TAU pre-warning notifications, err %d",
			    err);
	} else {
		shell_print(shell_global,
			    "Unsubscribed from TAU pre-warning notifications");
	}
}

int link_func_mode_set(enum lte_lc_func_mode fun)
{
	int return_value = 0;
	int sysmode;
	int lte_pref;

	switch (fun) {
	case LTE_LC_FUNC_MODE_POWER_OFF:
#if defined(CONFIG_MOSH_SMS)
		sms_unregister();
#endif
		return_value = lte_lc_power_off();
		break;
	case LTE_LC_FUNC_MODE_OFFLINE:
		return_value = lte_lc_offline();
		break;
	case LTE_LC_FUNC_MODE_NORMAL:
		/* Run custom at cmds from settings (link nmodeat -mosh command): */
		link_normal_mode_at_cmds_run();

		/* Set default context from settings (link defcont/defcontauth -mosh commands): */
		link_default_pdp_context_set();
		link_default_pdp_context_auth_set();

		/* Set saved system mode (if set) from settings (by link sysmode -mosh command): */
		sysmode = link_sett_sysmode_get();
		lte_pref = link_sett_sysmode_lte_preference_get();
		if (sysmode != LTE_LC_SYSTEM_MODE_NONE) {
			return_value =
				lte_lc_system_mode_set(sysmode, lte_pref);
			if (shell_global != NULL && return_value < 0) {
				shell_warn(
					shell_global,
					"lte_lc_system_mode_set returned error %d",
					return_value);
			}
		}

		if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
			return_value = lte_lc_normal();
		} else {
			/* TODO: why not just do lte_lc_normal() as notifications are
			 * subscribed there also nowadays?
			 */
			return_value = lte_lc_init_and_connect_async(
				link_ind_handler);
			if (return_value == -EALREADY) {
				return_value =
					lte_lc_connect_async(link_ind_handler);
			}
		}
		break;
	case LTE_LC_FUNC_MODE_DEACTIVATE_LTE:
	case LTE_LC_FUNC_MODE_ACTIVATE_LTE:
	case LTE_LC_FUNC_MODE_DEACTIVATE_GNSS:
	case LTE_LC_FUNC_MODE_ACTIVATE_GNSS:
	case LTE_LC_FUNC_MODE_DEACTIVATE_UICC:
	case LTE_LC_FUNC_MODE_ACTIVATE_UICC:
	case LTE_LC_FUNC_MODE_OFFLINE_UICC_ON:
	default:
		return_value = lte_lc_func_mode_set(fun);
		if (return_value) {
			shell_error(shell_global,
				    "lte_lc_func_mode_set returned, error %d",
				    return_value);
		}
		break;
	}

	return return_value;
}
