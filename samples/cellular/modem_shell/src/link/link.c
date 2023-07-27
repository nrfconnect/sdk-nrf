/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#include <modem/modem_info.h>
#include <modem/lte_lc.h>
#include <modem/pdn.h>

#include <dk_buttons_and_leds.h>

#include <nrf_socket.h>

#include "link_settings.h"
#include "link_shell.h"
#include "link_shell_print.h"

#include "link_shell_pdn.h"
#include "link_api.h"
#include "link.h"

#include "uart/uart_shell.h"
#include "mosh_print.h"
#include "mosh_defines.h"

#include <nrf_modem_at.h>
#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>

extern bool uart_shell_disable_during_sleep_requested;
extern struct k_work_q mosh_common_work_q;
extern char mosh_at_resp_buf[MOSH_AT_CMD_RESPONSE_MAX_LEN];
extern struct k_mutex mosh_at_resp_buf_mutex;

#define PDN_CONTEXTS_MAX 11
struct pdn_activation_status_info {
	bool activated;
	uint8_t cid;
};


static bool link_subscribe_for_rsrp;

/* Work for getting the modem info that ain't in lte connection ind: */
static struct k_work registered_work;

/* Work for getting the data needed to be shown after the cell change: */
struct cell_change_data {
	struct k_work work;

	/* Data from LTE_LC_EVT_CELL_UPDATE: */

	/* E-UTRAN cell ID, range 0 - LTE_LC_CELL_EUTRAN_ID_MAX */
	uint32_t cell_id;

	/* Tracking area code. */
	uint32_t tac;
};
static struct cell_change_data cell_change_work_data;

/* Work for a signal info: */
static struct k_work modem_info_signal_work;

#define LINK_RSRP_VALUE_NOT_KNOWN -999
static int32_t modem_rsrp = LINK_RSRP_VALUE_NOT_KNOWN;

/* Work for continuous neighbor cell measurements: */
struct ncellmeas_data {
	struct k_work_delayable work;

	struct lte_lc_ncellmeas_params params;

	enum link_ncellmeas_modes mode;

	int periodic_interval;
	bool periodic_interval_given;
};
static struct ncellmeas_data ncellmeas_work_data;
static struct ncellmeas_data ncellmeas_param_data = {
	.mode = LINK_NCELLMEAS_MODE_NONE,
	.params.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT,
	.periodic_interval = 0,
	.periodic_interval_given = false,
};

static void link_ncellmeas_worker(struct k_work *work_item)
{
	struct ncellmeas_data *data = CONTAINER_OF(work_item, struct ncellmeas_data, work);

	if (data->mode == LINK_NCELLMEAS_MODE_CONTINUOUS) {
		link_ncellmeas_start(true,
				     data->mode,
				     data->params,
				     data->periodic_interval,
				     data->periodic_interval_given);
	}
}

/******************************************************************************/

static void link_api_activate_mosh_contexts(
	struct pdn_activation_status_info pdn_act_status_arr[], int size)
{
	int i, esm, ret;

	/* Check that all context created by mosh link connect are active and if not,
	 * then activate.
	 */
	for (i = 0; i < size; i++) {
		if (pdn_act_status_arr[i].activated == false &&
		    link_shell_pdn_info_is_in_list(pdn_act_status_arr[i].cid)) {
			ret = pdn_activate(pdn_act_status_arr[i].cid, &esm, NULL);
			if (ret) {
				mosh_warn(
					"Cannot reactivate ctx with CID #%d, err: %d, removing from the list",
						pdn_act_status_arr[i].cid,
						ret);
				link_shell_pdn_info_list_remove(pdn_act_status_arr[i].cid);
			}
		}
	}
}

/* ****************************************************************************/

static void link_api_get_pdn_activation_status(
	struct pdn_activation_status_info pdn_act_status_arr[], int size)
{
	char buf[16] = { 0 };
	const char *p;
	int ret;

	k_mutex_lock(&mosh_at_resp_buf_mutex, K_FOREVER);
	ret = nrf_modem_at_cmd(mosh_at_resp_buf, sizeof(mosh_at_resp_buf), "AT+CGACT?");
	if (ret) {
		mosh_error("Cannot get PDP contexts activation states, err: %d", ret);
		goto exit;
	}

	/* For each contexts, fill the activation status into given array */
	for (int i = 0; i < size; i++) {
		/* Search for a string +CGACT: <cid>,<state> */
		snprintf(buf, sizeof(buf), "+CGACT: %d,1", i);
		pdn_act_status_arr[i].cid = i;
		p = strstr(mosh_at_resp_buf, buf);
		if (p) {
			pdn_act_status_arr[i].activated = true;
		}
	}
exit:
	k_mutex_unlock(&mosh_at_resp_buf_mutex);
}

/* ****************************************************************************/

static void link_registered_work(struct k_work *unused)
{
	struct pdn_activation_status_info pdn_act_status_arr[PDN_CONTEXTS_MAX];

	ARG_UNUSED(unused);

#if defined(CONFIG_DK_LIBRARY)
	dk_set_led_on(REGISTERED_STATUS_LED);
#endif
	memset(pdn_act_status_arr, 0,
	       PDN_CONTEXTS_MAX * sizeof(struct pdn_activation_status_info));

	/* Get PDN activation status for each */
	link_api_get_pdn_activation_status(pdn_act_status_arr, PDN_CONTEXTS_MAX);

	/* Activate the deactive ones that have been connected by us */
	link_api_activate_mosh_contexts(pdn_act_status_arr, PDN_CONTEXTS_MAX);

	/* Seems that 1st info read fails without this. Thus, let modem have some time */
	k_sleep(K_MSEC(1500));

	link_api_modem_info_get_for_shell(true);
}

/******************************************************************************/

static void link_cell_change_work(struct k_work *work_item)
{
	struct cell_change_data *data = CONTAINER_OF(work_item, struct cell_change_data, work);
	struct lte_xmonitor_resp_t xmonitor_resp;
	int ret = link_api_xmonitor_read(&xmonitor_resp);

	if (ret) {
		mosh_print(
			"LTE cell changed: ID: %d, Tracking area: %d",
			data->cell_id, data->tac);
		return;
	}
	mosh_print(
		"LTE cell changed: ID: %d (0x%s), PCI %d, Tracking area: %d (0x%s), Band: %d, RSRP: %d (%ddBm), SNR: %d (%ddB)",
		xmonitor_resp.cell_id, xmonitor_resp.cell_id_str, xmonitor_resp.pci,
		xmonitor_resp.tac, xmonitor_resp.tac_str, xmonitor_resp.band,
		xmonitor_resp.rsrp, RSRP_IDX_TO_DBM(xmonitor_resp.rsrp),
		xmonitor_resp.snr, (xmonitor_resp.snr - LINK_SNR_OFFSET_VALUE));
}

/******************************************************************************/

static void link_rsrp_signal_handler(char rsrp_value)
{
	modem_rsrp = RSRP_IDX_TO_DBM(rsrp_value);
	k_work_submit_to_queue(&mosh_common_work_q, &modem_info_signal_work);
}

/******************************************************************************/

#define MOSH_RSRP_UPDATE_INTERVAL_IN_SECS 5
static void link_rsrp_signal_update(struct k_work *unused)
{
	static uint32_t timestamp_prev;

	ARG_UNUSED(unused);
	if ((timestamp_prev != 0) &&
	    (k_uptime_get_32() - timestamp_prev <
	     MOSH_RSRP_UPDATE_INTERVAL_IN_SECS * MSEC_PER_SEC)) {
		return;
	}

	if (link_subscribe_for_rsrp) {
		mosh_print("RSRP: %d", modem_rsrp);
	}
	timestamp_prev = k_uptime_get_32();
}

/******************************************************************************/

void link_init(void)
{
	k_work_init(&cell_change_work_data.work, link_cell_change_work);
	k_work_init(&registered_work, link_registered_work);
	k_work_init(&modem_info_signal_work, link_rsrp_signal_update);
	modem_info_rsrp_register(link_rsrp_signal_handler);

	k_work_init_delayable(&ncellmeas_work_data.work, link_ncellmeas_worker);

	link_sett_init();

	link_shell_pdn_init();

	lte_lc_register_handler(link_ind_handler);
	(void)lte_lc_modem_events_enable();

	if (link_sett_is_dnsaddr_enabled()) {
		(void)link_setdnsaddr(link_sett_dnsaddr_ip_get());
	}

	if (link_sett_is_normal_mode_autoconn_enabled() == true) {
		link_func_mode_set(LTE_LC_FUNC_MODE_NORMAL,
				   link_sett_is_normal_mode_autoconn_rel14_used());
	}
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
		mosh_print("TAU pre warning: time %lld", evt->time);
		break;
	case LTE_LC_EVT_NEIGHBOR_CELL_MEAS: {
		int i;
		struct lte_lc_cells_info cells = evt->cells_info;
		struct lte_lc_cell cur_cell = cells.current_cell;
		char tmp_ta_str[12];

		mosh_print("Neighbor cell measurement results:");

		/* Current cell: */
		if (cur_cell.id != LTE_LC_CELL_EUTRAN_ID_INVALID) {
			if (cur_cell.timing_advance == LTE_LC_CELL_TIMING_ADVANCE_INVALID) {
				sprintf(tmp_ta_str, "\"not valid\"");
			} else {
				sprintf(tmp_ta_str, "%d", cur_cell.timing_advance);
			}

			mosh_print("  Current cell:");
			mosh_print(
				"    ID %d, phy ID %d, MCC %d MNC %d, RSRP %d : %ddBm, RSRQ %d, TAC %d, earfcn %d, meas time %lld\n"
				"    TA %s, TA meas time %lld",
				cur_cell.id, cur_cell.phys_cell_id,
				cur_cell.mcc, cur_cell.mnc, cur_cell.rsrp,
				RSRP_IDX_TO_DBM(cur_cell.rsrp),
				cur_cell.rsrq, cur_cell.tac, cur_cell.earfcn,
				cur_cell.measurement_time,
				tmp_ta_str,
				cur_cell.timing_advance_meas_time);
		} else {
			mosh_print("  No current cell information from modem.");
		}

		if (!cells.ncells_count) {
			mosh_print("  No neighbor cell information from modem.");
		}

		for (i = 0; i < cells.ncells_count; i++) {
			/* Actual neighbor cells of a current cell */
			mosh_print("  Neighbor cell %d", i + 1);
			mosh_print(
				"    phy ID %d, RSRP %d : %ddBm, RSRQ %d, earfcn %d, timediff %d",
				cells.neighbor_cells[i].phys_cell_id,
				cells.neighbor_cells[i].rsrp,
				RSRP_IDX_TO_DBM(cells.neighbor_cells[i].rsrp),
				cells.neighbor_cells[i].rsrq,
				cells.neighbor_cells[i].earfcn,
				cells.neighbor_cells[i].time_diff);
		}

		/* Surrounding cells from GCI search types */
		for (i = 0; i < cells.gci_cells_count; i++) {
			/* GCI neighbor cells */
			if (cells.gci_cells[i].timing_advance ==
				LTE_LC_CELL_TIMING_ADVANCE_INVALID) {
				sprintf(tmp_ta_str, "\"not valid\"");
			} else {
				sprintf(tmp_ta_str, "%d", cells.gci_cells[i].timing_advance);
			}
			mosh_print("  Surrounding cell %d from GCI search", i + 1);
			mosh_print(
				"    ID %d, phy ID %d, MCC %d MNC %d, RSRP %d : %ddBm, RSRQ %d, TAC %d, earfcn %d, meas time %lld\n"
				"    TA %s, TA meas time %lld",
				cells.gci_cells[i].id, cells.gci_cells[i].phys_cell_id,
				cells.gci_cells[i].mcc, cells.gci_cells[i].mnc,
				cells.gci_cells[i].rsrp,
				RSRP_IDX_TO_DBM(cells.gci_cells[i].rsrp),
				cells.gci_cells[i].rsrq, cells.gci_cells[i].tac,
				cells.gci_cells[i].earfcn, cells.gci_cells[i].measurement_time,
				tmp_ta_str,
				cells.gci_cells[i].timing_advance_meas_time);
		}

		if (ncellmeas_param_data.mode == LINK_NCELLMEAS_MODE_CONTINUOUS &&
		    ncellmeas_param_data.periodic_interval_given) {
			/* Interval was given for continuous mode */
			ncellmeas_work_data.mode = ncellmeas_param_data.mode;
			ncellmeas_work_data.params.search_type =
				ncellmeas_param_data.params.search_type;
			ncellmeas_work_data.params.gci_count =
				ncellmeas_param_data.params.gci_count;
			ncellmeas_work_data.periodic_interval =
				ncellmeas_param_data.periodic_interval;
			ncellmeas_work_data.periodic_interval_given =
				ncellmeas_param_data.periodic_interval_given;

			k_work_schedule_for_queue(&mosh_common_work_q,
					&ncellmeas_work_data.work,
					K_SECONDS(ncellmeas_param_data.periodic_interval));
		}
	} break;
	case LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING:
		link_shell_print_modem_sleep_notif(evt);
		break;
	case LTE_LC_EVT_MODEM_SLEEP_ENTER:
	case LTE_LC_EVT_MODEM_SLEEP_EXIT:
		link_shell_print_modem_sleep_notif(evt);

		if (uart_shell_disable_during_sleep_requested) {
			uart_toggle_power_state_at_event(evt);
		}
		break;
	case LTE_LC_EVT_LTE_MODE_UPDATE:
		/** The currently active LTE mode is updated. If a system mode that
		 *  enables both LTE-M and NB-IoT is configured, the modem may change
		 *  the currently active LTE mode based on the system mode preference
		 *  and network availability. This event will then indicate which
		 *  LTE mode is currently used by the modem.
		 */
		mosh_print(
			"Currently active system mode: %s",
			link_shell_sysmode_currently_active_to_string(evt->lte_mode, snum));
		break;
	case LTE_LC_EVT_NW_REG_STATUS:
		link_shell_print_reg_status(evt->nw_reg_status);

		if (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_EMERGENCY ||
		    evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ||
		    evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING) {
			k_work_submit_to_queue(&mosh_common_work_q, &registered_work);
		} else {
#if defined(CONFIG_DK_LIBRARY)
			dk_set_led_off(REGISTERED_STATUS_LED);
#endif
		}
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		cell_change_work_data.cell_id = evt->cell.id;
		cell_change_work_data.tac = evt->cell.tac;
		k_work_submit_to_queue(&mosh_common_work_q, &cell_change_work_data.work);

		if (ncellmeas_param_data.mode == LINK_NCELLMEAS_MODE_CONTINUOUS) {
			ncellmeas_work_data.mode = ncellmeas_param_data.mode;
			ncellmeas_work_data.params.search_type =
				ncellmeas_param_data.params.search_type;
			ncellmeas_work_data.params.gci_count =
				ncellmeas_param_data.params.gci_count;
			ncellmeas_work_data.periodic_interval =
				ncellmeas_param_data.periodic_interval;
			ncellmeas_work_data.periodic_interval_given =
				ncellmeas_param_data.periodic_interval_given;

			/* Send immediately after a cell update */
			k_work_schedule_for_queue(&mosh_common_work_q, &ncellmeas_work_data.work,
						  K_SECONDS(0));
		}
		break;
	case LTE_LC_EVT_RRC_UPDATE:
		mosh_print(
			"RRC mode: %s",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" :
			"Idle");
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		mosh_print(
			"PSM parameter update: TAU: %d, Active time: %d seconds",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintf(
			log_buf, sizeof(log_buf),
			"eDRX parameter update: eDRX: %f, PTW: %f",
			evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			mosh_print("%s", log_buf);
		}
		break;
	}
	case LTE_LC_EVT_MODEM_EVENT:
		link_shell_print_modem_domain_event(evt->modem_evt);
		break;
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
			mosh_error("pdn_ctx_configure returned err %d", ret);
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
			mosh_error("pdn_ctx_auth_set returned err  %d", ret);
			return ret;
		}
	}
	return 0;
}

static int link_enable_disable_rel14_features(bool enable)
{
	int ret;

	if (enable) {
		ret = nrf_modem_at_printf("AT%%REL14FEAT=1,1,1,1,1");
	} else {
		ret = nrf_modem_at_printf("AT%%REL14FEAT=0,0,0,0,0");
	}

	if (ret < 0) {
		mosh_warn("Release 14 features %s AT-command failed, err %d",
			((enable) ? "enable" : "disable"),
			ret);
	}
	return 0;
}

static int link_normal_mode_at_cmds_run(void)
{
	char *normal_mode_at_cmd;
	int mem_slot_index = LINK_SETT_NMODEAT_MEM_SLOT_INDEX_START;
	int len;

	k_mutex_lock(&mosh_at_resp_buf_mutex, K_FOREVER);
	for (; mem_slot_index <= LINK_SETT_NMODEAT_MEM_SLOT_INDEX_END;
	     mem_slot_index++) {
		normal_mode_at_cmd =
			link_sett_normal_mode_at_cmd_str_get(mem_slot_index);
		len = strlen(normal_mode_at_cmd);
		if (len) {
			if (nrf_modem_at_cmd(mosh_at_resp_buf, sizeof(mosh_at_resp_buf), "%s",
					     normal_mode_at_cmd) != 0) {
				mosh_error(
					"Normal mode AT-command from memory slot %d \"%s\" returned: ERROR",
					mem_slot_index, normal_mode_at_cmd);
			} else {
				mosh_print(
					"Normal mode AT-command from memory slot %d \"%s\" returned:\n\r %s",
					mem_slot_index, normal_mode_at_cmd,
					mosh_at_resp_buf);
			}
		}
	}
	k_mutex_unlock(&mosh_at_resp_buf_mutex);

	return 0;
}

void link_rsrp_subscribe(bool subscribe)
{
	link_subscribe_for_rsrp = subscribe;
	if (link_subscribe_for_rsrp) {
		/* print current value right away */
		mosh_print("RSRP subscribed");
		if (modem_rsrp != LINK_RSRP_VALUE_NOT_KNOWN) {
			mosh_print("RSRP: %d", modem_rsrp);
		}
	} else {
		mosh_print("RSRP unsubscribed");
	}
}

void link_ncellmeas_start(bool start, enum link_ncellmeas_modes mode,
			  struct lte_lc_ncellmeas_params ncellmeas_params,
			  int periodic_interval,
			  bool periodic_interval_given)
{
	int ret;

	ncellmeas_param_data.mode = mode;
	ncellmeas_param_data.periodic_interval = periodic_interval;
	ncellmeas_param_data.periodic_interval_given = periodic_interval_given;
	ncellmeas_param_data.params = ncellmeas_params;

	k_work_cancel_delayable(&ncellmeas_work_data.work);

	if (start) {
		ret = lte_lc_neighbor_cell_measurement(&ncellmeas_params);
		if (ret) {
			mosh_error("lte_lc_neighbor_cell_measurement() returned err %d",
				ret);
			mosh_error("Cannot start neigbor measurements");
		}
	} else {
		ret = lte_lc_neighbor_cell_measurement_cancel();
		if (ret) {
			mosh_error(
				"lte_lc_neighbor_cell_measurement_cancel() returned err %d", ret);
		} else {
			mosh_print("Neighbor cell measurements and reporting stopped");
		}
	}
}

void link_modem_sleep_notifications_subscribe(uint32_t warn_time_ms,
					       uint32_t threshold_ms)
{
	int err;

	err = nrf_modem_at_printf("AT%%XMODEMSLEEP=1,%d,%d", warn_time_ms, threshold_ms);
	if (err) {
		mosh_error("Cannot subscribe to modem sleep notifications, err %d", err);
	} else {
		mosh_print("Subscribed to modem sleep notifications");
	}
}

void link_modem_sleep_notifications_unsubscribe(void)
{
	int err;

	err = nrf_modem_at_printf("AT%%XMODEMSLEEP=0");
	if (err) {
		mosh_error("Cannot stop modem sleep notifications, err %d", err);
	} else {
		mosh_print("Unsubscribed from modem sleep notifications");
	}
}

void link_modem_tau_notifications_subscribe(uint32_t warn_time_ms,
					     uint32_t threshold_ms)
{
	int err;

	err = nrf_modem_at_printf("AT%%XT3412=1,%d,%d", warn_time_ms, threshold_ms);
	if (err) {
		mosh_error("Cannot subscribe to TAU notifications, err %d", err);
	} else {
		mosh_print("Subscribed to TAU pre-warning notifications");
	}
}

void link_modem_tau_notifications_unsubscribe(void)
{
	int err;

	err = nrf_modem_at_printf("AT%%XT3412=0");
	if (err) {
		mosh_error("Cannot stop TAU pre-warning notifications, err %d", err);
	} else {
		mosh_print("Unsubscribed from TAU pre-warning notifications");
	}
}

int link_func_mode_set(enum lte_lc_func_mode fun, bool rel14_used)
{
	int return_value = 0;
	int sysmode;
	int lte_pref;

	switch (fun) {
	case LTE_LC_FUNC_MODE_POWER_OFF:
		return_value = lte_lc_power_off();
		break;
	case LTE_LC_FUNC_MODE_OFFLINE:
		return_value = lte_lc_offline();
		break;
	case LTE_LC_FUNC_MODE_NORMAL:
		/* Enable/disable Rel14 features before going to normal mode */
		link_enable_disable_rel14_features(rel14_used);

		/* (Re)register for rsrp notifications */
		modem_info_rsrp_register(link_rsrp_signal_handler);

		/* Run custom at cmds from settings (link nmodeat -mosh command) */
		link_normal_mode_at_cmds_run();

		/* Set default context from settings (link defcont/defcontauth -mosh commands) */
		link_default_pdp_context_set();
		link_default_pdp_context_auth_set();

		/* Set saved system mode (if set) from settings (by link sysmode -mosh command) */
		sysmode = link_sett_sysmode_get();
		lte_pref = link_sett_sysmode_lte_preference_get();
		if (sysmode != LTE_LC_SYSTEM_MODE_NONE) {
			return_value = lte_lc_system_mode_set(sysmode, lte_pref);
			if (return_value < 0) {
				mosh_warn("lte_lc_system_mode_set returned error %d", return_value);
			}
		}

		return_value = lte_lc_normal();
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
			mosh_error("lte_lc_func_mode_set returned, error %d", return_value);
		}
		break;
	}

	return return_value;
}

void link_rai_read(void)
{
	bool rai_status = false;
	int err;

	err = link_api_rai_status(&rai_status);
	if (err == 0) {
		mosh_print("Release Assistance Indication status is enabled=%d", rai_status);
	} else {
		mosh_error("Reading RAI failed with error code %d", err);
	}
}

int link_rai_enable(bool enable)
{
	int err;

	err = nrf_modem_at_printf("AT%%RAI=%d", enable);
	if (!err) {
		mosh_print(
			"Release Assistance Indication functionality set to enabled=%d.\n"
			"The change will be applied when going to normal mode for the next time.",
			enable);
	} else {
		mosh_error("RAI AT command failed, error: %d", err);
		return -EFAULT;
	}
	return 0;
}

int link_setdnsaddr(const char *ip_address)
{
	struct nrf_in_addr in4_addr;
	struct nrf_in6_addr in6_addr;
	int family = NRF_AF_INET;
	void *in_addr = NULL;
	nrf_socklen_t in_size = 0;
	int ret = 0;

	if (strlen(ip_address) > 0) {
		in_addr = &in4_addr;
		in_size = sizeof(in4_addr);
		ret = nrf_inet_pton(family, ip_address, in_addr);

		if (ret != 1) {
			family = NRF_AF_INET6;
			in_addr = &in6_addr;
			in_size = sizeof(in6_addr);
			ret = nrf_inet_pton(family, ip_address, in_addr);
		}

		if (ret != 1) {
			mosh_error("Invalid IP address: %s", ip_address);
			return -EINVAL;
		}
	}

	if (link_sett_is_dnsaddr_enabled() && ret == 1) {
		ret = nrf_setdnsaddr(family, in_addr, in_size);
		if (ret != 0) {
			mosh_error("Error setting DNS address: %d", errno);
			return -errno;
		}
	} else {
		(void)nrf_setdnsaddr(family, NULL, 0);
	}

	return 0;
}
