/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <errno.h>
#include <zephyr/net/socket.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <nrf_errno.h>
#include <nrf_modem_at.h>
#include <modem/lte_lc.h>
#include <modem/lte_lc_trace.h>
#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>
#include <modem/at_monitor.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/logging/log.h>

#include "lte_lc_helpers.h"

LOG_MODULE_REGISTER(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

/* Internal system mode value used when CONFIG_LTE_NETWORK_MODE_DEFAULT is enabled. */
#define LTE_LC_SYSTEM_MODE_DEFAULT 0xff

#define SYS_MODE_PREFERRED \
	(IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M)		? \
		LTE_LC_SYSTEM_MODE_LTEM				: \
	IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT)		? \
		LTE_LC_SYSTEM_MODE_NBIOT			: \
	IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_GPS)		? \
		LTE_LC_SYSTEM_MODE_LTEM_GPS			: \
	IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT_GPS)		? \
		LTE_LC_SYSTEM_MODE_NBIOT_GPS			: \
	IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT)		? \
		LTE_LC_SYSTEM_MODE_LTEM_NBIOT			: \
	IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS)	? \
		LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS		: \
	LTE_LC_SYSTEM_MODE_DEFAULT)

/* Internal enums */

enum feaconf_oper {
	FEACONF_OPER_WRITE = 0,
	FEACONF_OPER_READ  = 1,
	FEACONF_OPER_LIST  = 2
};

enum feaconf_feat {
	FEACONF_FEAT_PROPRIETARY_PSM = 0
};

/* Static variables */

static bool is_initialized;

/* Request eDRX to be disabled */
static const char edrx_disable[] = "AT+CEDRXS=3";
/* Default eDRX setting */
static char edrx_param_ltem[5] = CONFIG_LTE_EDRX_REQ_VALUE_LTE_M;
static char edrx_param_nbiot[5] = CONFIG_LTE_EDRX_REQ_VALUE_NBIOT;
/* Default PTW setting */
static char ptw_param_ltem[5] = CONFIG_LTE_PTW_VALUE_LTE_M;
static char ptw_param_nbiot[5] = CONFIG_LTE_PTW_VALUE_NBIOT;
/* Default PSM RAT setting */
static char psm_param_rat[9] = CONFIG_LTE_PSM_REQ_RAT;
/* Default PSM RPTAU setting */
static char psm_param_rptau[9] = CONFIG_LTE_PSM_REQ_RPTAU;
/* Request PSM to be disabled and timers set to default values */
static const char psm_disable[] = "AT+CPSMS=";
/* Enable CSCON (RRC mode) notifications */
static const char cscon[] = "AT+CSCON=1";
/* Disable RAI */
static const char rai_disable[] = "AT%%XRAI=0";
/* Default RAI setting */
static char rai_param[2] = CONFIG_LTE_RAI_REQ_VALUE;

/* Requested NCELLMEAS params */
static struct lte_lc_ncellmeas_params ncellmeas_params;
/* Sempahore value 1 means ncellmeas is not ongoing, and 0 means it's ongoing. */
K_SEM_DEFINE(ncellmeas_idle_sem, 1, 1);
/* Network attach semaphore */
static K_SEM_DEFINE(link, 0, 1);

/* System mode to use when connecting to LTE network, which can be changed in
 * two ways:
 *	- Automatically to fallback mode (if enabled) when connection to the
 *	  preferred mode is unsuccessful and times out.
 *	- By calling lte_lc_system_mode_set() and set the mode explicitly.
 */
static enum lte_lc_system_mode sys_mode_target = SYS_MODE_PREFERRED;

/* System mode preference to set when configuring system mode. */
static enum lte_lc_system_mode_preference mode_pref_target = CONFIG_LTE_MODE_PREFERENCE_VALUE;
static enum lte_lc_system_mode_preference mode_pref_current;

static const enum lte_lc_system_mode sys_mode_fallback =
#if IS_ENABLED(CONFIG_LTE_NETWORK_USE_FALLBACK)
	IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M)	?
		LTE_LC_SYSTEM_MODE_NBIOT		:
	IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT)	?
		LTE_LC_SYSTEM_MODE_LTEM			:
	IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_GPS)	?
		LTE_LC_SYSTEM_MODE_NBIOT_GPS		:
	IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT_GPS)	?
		LTE_LC_SYSTEM_MODE_LTEM_GPS		:
#endif
	LTE_LC_SYSTEM_MODE_DEFAULT;

static enum lte_lc_system_mode sys_mode_current = LTE_LC_SYSTEM_MODE_DEFAULT;

/* Parameters to be passed using AT%XSYSTEMMMODE=<params>,<preference> */
static const char *const system_mode_params[] = {
	[LTE_LC_SYSTEM_MODE_LTEM]		= "1,0,0",
	[LTE_LC_SYSTEM_MODE_NBIOT]		= "0,1,0",
	[LTE_LC_SYSTEM_MODE_GPS]		= "0,0,1",
	[LTE_LC_SYSTEM_MODE_LTEM_GPS]		= "1,0,1",
	[LTE_LC_SYSTEM_MODE_NBIOT_GPS]		= "0,1,1",
	[LTE_LC_SYSTEM_MODE_LTEM_NBIOT]		= "1,1,0",
	[LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS]	= "1,1,1",
};

/* LTE preference to be passed using AT%XSYSTEMMMODE=<params>,<preference> */
static const char system_mode_preference[] = {
	/* No LTE preference, automatically selected by the modem. */
	[LTE_LC_SYSTEM_MODE_PREFER_AUTO]		= '0',
	/* LTE-M has highest priority. */
	[LTE_LC_SYSTEM_MODE_PREFER_LTEM]		= '1',
	/* NB-IoT has highest priority. */
	[LTE_LC_SYSTEM_MODE_PREFER_NBIOT]		= '2',
	/* Equal priority, but prefer LTE-M. */
	[LTE_LC_SYSTEM_MODE_PREFER_LTEM_PLMN_PRIO]	= '3',
	/* Equal priority, but prefer NB-IoT. */
	[LTE_LC_SYSTEM_MODE_PREFER_NBIOT_PLMN_PRIO]	= '4',
};

static bool is_cellid_valid(uint32_t cellid)
{
	if (cellid == LTE_LC_CELL_EUTRAN_ID_INVALID) {
		return false;
	}

	return true;
}

AT_MONITOR(ltelc_atmon_cereg, "+CEREG", at_handler_cereg);
AT_MONITOR(ltelc_atmon_cscon, "+CSCON", at_handler_cscon);
AT_MONITOR(ltelc_atmon_cedrxp, "+CEDRXP", at_handler_cedrxp);
AT_MONITOR(ltelc_atmon_xt3412, "%XT3412", at_handler_xt3412);
AT_MONITOR(ltelc_atmon_ncellmeas, "%NCELLMEAS", at_handler_ncellmeas);
AT_MONITOR(ltelc_atmon_xmodemsleep, "%XMODEMSLEEP", at_handler_xmodemsleep);
AT_MONITOR(ltelc_atmon_mdmev, "%MDMEV", at_handler_mdmev);

static void at_handler_cereg(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};

	__ASSERT_NO_MSG(response != NULL);

	static enum lte_lc_nw_reg_status prev_reg_status =
		LTE_LC_NW_REG_NOT_REGISTERED;
	static struct lte_lc_cell prev_cell;
	static struct lte_lc_psm_cfg prev_psm_cfg;
	static enum lte_lc_lte_mode prev_lte_mode = LTE_LC_LTE_MODE_NONE;
	enum lte_lc_nw_reg_status reg_status = 0;
	struct lte_lc_cell cell = {0};
	enum lte_lc_lte_mode lte_mode;
	struct lte_lc_psm_cfg psm_cfg = {0};

	LOG_DBG("+CEREG notification: %s", response);

	err = parse_cereg(response, true, &reg_status, &cell, &lte_mode);
	if (err) {
		LOG_ERR("Failed to parse notification (error %d): %s",
			err, response);
		return;
	}

	if ((reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
	    (reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
		/* Set the network registration status to UNKNOWN if the cell ID is parsed
		 * to UINT32_MAX (FFFFFFFF) when the registration status is either home or
		 * roaming.
		 */
		if (!is_cellid_valid(cell.id)) {
			reg_status = LTE_LC_NW_REG_UNKNOWN;
		} else {
			k_sem_give(&link);
		}
	}

	switch (reg_status) {
	case LTE_LC_NW_REG_NOT_REGISTERED:
		LTE_LC_TRACE(LTE_LC_TRACE_NW_REG_NOT_REGISTERED);
		break;
	case LTE_LC_NW_REG_REGISTERED_HOME:
		LTE_LC_TRACE(LTE_LC_TRACE_NW_REG_REGISTERED_HOME);
		break;
	case LTE_LC_NW_REG_SEARCHING:
		LTE_LC_TRACE(LTE_LC_TRACE_NW_REG_SEARCHING);
		break;
	case LTE_LC_NW_REG_REGISTRATION_DENIED:
		LTE_LC_TRACE(LTE_LC_TRACE_NW_REG_REGISTRATION_DENIED);
		break;
	case LTE_LC_NW_REG_UNKNOWN:
		LTE_LC_TRACE(LTE_LC_TRACE_NW_REG_UNKNOWN);
		break;
	case LTE_LC_NW_REG_REGISTERED_ROAMING:
		LTE_LC_TRACE(LTE_LC_TRACE_NW_REG_REGISTERED_ROAMING);
		break;
	case LTE_LC_NW_REG_UICC_FAIL:
		LTE_LC_TRACE(LTE_LC_TRACE_NW_REG_UICC_FAIL);
		break;
	}

	if (event_handler_list_is_empty()) {
		return;
	}

	/* Network registration status event */
	if (reg_status != prev_reg_status) {
		prev_reg_status = reg_status;
		evt.type = LTE_LC_EVT_NW_REG_STATUS;
		evt.nw_reg_status = reg_status;

		event_handler_list_dispatch(&evt);
	}

	/* Cell update event */
	if ((cell.id != prev_cell.id) || (cell.tac != prev_cell.tac)) {
		evt.type = LTE_LC_EVT_CELL_UPDATE;

		memcpy(&prev_cell, &cell, sizeof(struct lte_lc_cell));
		memcpy(&evt.cell, &cell, sizeof(struct lte_lc_cell));
		event_handler_list_dispatch(&evt);
	}

	if (lte_mode != prev_lte_mode) {
		prev_lte_mode = lte_mode;
		evt.type = LTE_LC_EVT_LTE_MODE_UPDATE;
		evt.lte_mode = lte_mode;

		switch (lte_mode) {
		case LTE_LC_LTE_MODE_LTEM:
			LTE_LC_TRACE(LTE_LC_TRACE_LTE_MODE_UPDATE_LTEM);
			break;
		case LTE_LC_LTE_MODE_NBIOT:
			LTE_LC_TRACE(LTE_LC_TRACE_LTE_MODE_UPDATE_NBIOT);
			break;
		case LTE_LC_LTE_MODE_NONE:
			LTE_LC_TRACE(LTE_LC_TRACE_LTE_MODE_UPDATE_NONE);
			break;
		}

		event_handler_list_dispatch(&evt);
	}

	if ((reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
	    (reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
		return;
	}

	err = lte_lc_psm_get(&psm_cfg.tau, &psm_cfg.active_time);
	if (err) {
		if (err != -EBADMSG) {
			LOG_ERR("Failed to get PSM information");
		}
		return;
	}

	/* PSM configuration update event */
	if ((psm_cfg.tau != prev_psm_cfg.tau) ||
	    (psm_cfg.active_time != prev_psm_cfg.active_time)) {
		evt.type = LTE_LC_EVT_PSM_UPDATE;

		memcpy(&prev_psm_cfg, &psm_cfg, sizeof(struct lte_lc_psm_cfg));
		memcpy(&evt.psm_cfg, &psm_cfg, sizeof(struct lte_lc_psm_cfg));
		event_handler_list_dispatch(&evt);
	}
}

static void at_handler_cscon(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};

	__ASSERT_NO_MSG(response != NULL);

	LOG_DBG("+CSCON notification");

	err = parse_rrc_mode(response, &evt.rrc_mode, AT_CSCON_RRC_MODE_INDEX);
	if (err) {
		LOG_ERR("Can't parse signalling mode, error: %d", err);
		return;
	}

	if (evt.rrc_mode == LTE_LC_RRC_MODE_IDLE) {
		LTE_LC_TRACE(LTE_LC_TRACE_RRC_IDLE);
	} else if (evt.rrc_mode == LTE_LC_RRC_MODE_CONNECTED) {
		LTE_LC_TRACE(LTE_LC_TRACE_RRC_CONNECTED);
	}

	evt.type = LTE_LC_EVT_RRC_UPDATE;

	event_handler_list_dispatch(&evt);
}

static void at_handler_cedrxp(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};

	__ASSERT_NO_MSG(response != NULL);

	LOG_DBG("+CEDRXP notification");

	err = parse_edrx(response, &evt.edrx_cfg);
	if (err) {
		LOG_ERR("Can't parse eDRX, error: %d", err);
		return;
	}

	evt.type = LTE_LC_EVT_EDRX_UPDATE;

	event_handler_list_dispatch(&evt);
}

static void at_handler_xt3412(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};

	__ASSERT_NO_MSG(response != NULL);

	LOG_DBG("%%XT3412 notification");

	err = parse_xt3412(response, &evt.time);
	if (err) {
		LOG_ERR("Can't parse TAU pre-warning notification, error: %d", err);
		return;
	}

	if (evt.time != CONFIG_LTE_LC_TAU_PRE_WARNING_TIME_MS) {
		/* Only propagate TAU pre-warning notifications when the received time
		 * parameter is the duration of the set pre-warning time.
		 */
		return;
	}

	evt.type = LTE_LC_EVT_TAU_PRE_WARNING;

	event_handler_list_dispatch(&evt);
}

static void at_handler_ncellmeas_gci(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};
	const char *resp = response;

	__ASSERT_NO_MSG(response != NULL);

	int max_cell_count = ncellmeas_params.gci_count;
	struct lte_lc_cell *cells = NULL;

	LOG_DBG("%%NCELLMEAS GCI notification parsing starts");

	if (max_cell_count != 0) {
		cells = k_calloc(max_cell_count, sizeof(struct lte_lc_cell));
		if (cells == NULL) {
			LOG_ERR("Failed to allocate memory for the GCI cells");
			return;
		}
	}

	evt.cells_info.gci_cells = cells;
	err = parse_ncellmeas_gci(&ncellmeas_params, resp, &evt.cells_info);
	LOG_DBG("parse_ncellmeas_gci returned %d", err);
	switch (err) {
	case -E2BIG:
		LOG_WRN("Not all neighbor cells could be parsed");
		LOG_WRN("More cells than the configured max count of %d were found",
			CONFIG_LTE_NEIGHBOR_CELLS_MAX);
		/* Fall through */
	case 0: /* Fall through */
	case 1:
		LOG_DBG("Neighbor cell count: %d, GCI cells count: %d",
			evt.cells_info.ncells_count,
			evt.cells_info.gci_cells_count);
		evt.type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
		event_handler_list_dispatch(&evt);
		break;
	default:
		LOG_ERR("Parsing of neighbor cells failed, err: %d", err);
		break;
	}

	k_free(cells);
	k_free(evt.cells_info.neighbor_cells);
}

static void at_handler_ncellmeas(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};

	__ASSERT_NO_MSG(response != NULL);

	if (event_handler_list_is_empty()) {
		/* No need to parse the response if there is no handler
		 * to receive the parsed data.
		 */
		goto exit;
	}

	if (ncellmeas_params.search_type > LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_COMPLETE) {
		at_handler_ncellmeas_gci(response);
		goto exit;
	}

	int ncell_count = neighborcell_count_get(response);
	struct lte_lc_ncell *neighbor_cells = NULL;

	LOG_DBG("%%NCELLMEAS notification: neighbor cell count: %d", ncell_count);

	if (ncell_count != 0) {
		neighbor_cells = k_calloc(ncell_count, sizeof(struct lte_lc_ncell));
		if (neighbor_cells == NULL) {
			LOG_ERR("Failed to allocate memory for neighbor cells");
			goto exit;
		}
	}

	evt.cells_info.neighbor_cells = neighbor_cells;

	err = parse_ncellmeas(response, &evt.cells_info);

	switch (err) {
	case -E2BIG:
		LOG_WRN("Not all neighbor cells could be parsed");
		LOG_WRN("More cells than the configured max count of %d were found",
			CONFIG_LTE_NEIGHBOR_CELLS_MAX);
		/* Fall through */
	case 0: /* Fall through */
	case 1:
		evt.type = LTE_LC_EVT_NEIGHBOR_CELL_MEAS;
		event_handler_list_dispatch(&evt);
		break;
	default:
		LOG_ERR("Parsing of neighbor cells failed, err: %d", err);
		break;
	}

	if (neighbor_cells) {
		k_free(neighbor_cells);
	}
exit:
	k_sem_give(&ncellmeas_idle_sem);
}

static void at_handler_xmodemsleep(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};

	__ASSERT_NO_MSG(response != NULL);

	LOG_DBG("%%XMODEMSLEEP notification");

	err = parse_xmodemsleep(response, &evt.modem_sleep);
	if (err) {
		LOG_ERR("Can't parse modem sleep pre-warning notification, error: %d", err);
		return;
	}

	/* Link controller only supports PSM, RF inactivity, limited service, flight mode
	 * and proprietary PSM modem sleep types.
	 */
	if ((evt.modem_sleep.type != LTE_LC_MODEM_SLEEP_PSM) &&
		(evt.modem_sleep.type != LTE_LC_MODEM_SLEEP_RF_INACTIVITY) &&
		(evt.modem_sleep.type != LTE_LC_MODEM_SLEEP_LIMITED_SERVICE) &&
		(evt.modem_sleep.type != LTE_LC_MODEM_SLEEP_FLIGHT_MODE) &&
		(evt.modem_sleep.type != LTE_LC_MODEM_SLEEP_PROPRIETARY_PSM)) {
		return;
	}

	/* Propagate the appropriate event depending on the parsed time parameter. */
	if (evt.modem_sleep.time == CONFIG_LTE_LC_MODEM_SLEEP_PRE_WARNING_TIME_MS) {
		evt.type = LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING;
	} else if (evt.modem_sleep.time == 0) {
		LTE_LC_TRACE(LTE_LC_TRACE_MODEM_SLEEP_EXIT);

		evt.type = LTE_LC_EVT_MODEM_SLEEP_EXIT;
	} else {
		LTE_LC_TRACE(LTE_LC_TRACE_MODEM_SLEEP_ENTER);

		evt.type = LTE_LC_EVT_MODEM_SLEEP_ENTER;
	}

	event_handler_list_dispatch(&evt);
}

static void at_handler_mdmev(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};

	__ASSERT_NO_MSG(response != NULL);

	LOG_DBG("%%MDMEV notification");

	err = parse_mdmev(response, &evt.modem_evt);
	if (err) {
		LOG_ERR("Can't parse modem event notification, error: %d", err);
		return;
	}

	evt.type = LTE_LC_EVT_MODEM_EVENT;

	event_handler_list_dispatch(&evt);
}

static int enable_notifications(void)
{
	int err;

	/* +CEREG notifications, level 5 */
	err = nrf_modem_at_printf(AT_CEREG_5);
	if (err) {
		LOG_ERR("Failed to subscribe to CEREG notifications, error: %d", err);
		return -EFAULT;
	}

	if (IS_ENABLED(CONFIG_LTE_LC_TAU_PRE_WARNING_NOTIFICATIONS)) {
		err = nrf_modem_at_printf(AT_XT3412_SUB,
					  CONFIG_LTE_LC_TAU_PRE_WARNING_TIME_MS,
					  CONFIG_LTE_LC_TAU_PRE_WARNING_THRESHOLD_MS);
		if (err) {
			LOG_WRN("Enabling TAU pre-warning notifications failed, error: %d", err);
			LOG_WRN("TAU pre-warning notifications require nRF9160 modem >= v1.3.0");
		}
	}

	if (IS_ENABLED(CONFIG_LTE_LC_MODEM_SLEEP_NOTIFICATIONS)) {
		/* %XMODEMSLEEP notifications subscribe */
		err = nrf_modem_at_printf(AT_XMODEMSLEEP_SUB,
					  CONFIG_LTE_LC_MODEM_SLEEP_PRE_WARNING_TIME_MS,
					  CONFIG_LTE_LC_MODEM_SLEEP_NOTIFICATIONS_THRESHOLD_MS);
		if (err) {
			LOG_WRN("Enabling modem sleep notifications failed, error: %d", err);
			LOG_WRN("Modem sleep notifications require nRF9160 modem >= v1.3.0");
		}
	}

	/* +CSCON notifications */
	err = nrf_modem_at_printf(cscon);
	if (err) {
		LOG_WRN("Failed to enable RRC notifications (+CSCON), error %d", err);
		return -EFAULT;
	}

	return 0;
}

static void lte_lc_psm_default_config_set(void)
{
	if (IS_ENABLED(CONFIG_LTE_PSM_REQ_FORMAT_SECONDS)) {
		lte_lc_psm_param_set_seconds(
			CONFIG_LTE_PSM_REQ_RPTAU_SECONDS,
			CONFIG_LTE_PSM_REQ_RAT_SECONDS);
		LOG_DBG("PSM configs set from seconds: tau=%s (%ds), rat=%s (%ds)",
			psm_param_rptau, CONFIG_LTE_PSM_REQ_RPTAU_SECONDS,
			psm_param_rat, CONFIG_LTE_PSM_REQ_RAT_SECONDS);
	} else {
		__ASSERT_NO_MSG(IS_ENABLED(CONFIG_LTE_PSM_REQ_FORMAT_STRING));
		lte_lc_psm_param_set(CONFIG_LTE_PSM_REQ_RPTAU, CONFIG_LTE_PSM_REQ_RAT);
		LOG_DBG("PSM configs set from string: tau=%s, rat=%s",
			psm_param_rptau, psm_param_rat);
	}
}

static int init_and_config(void)
{
	int err;

	if (is_initialized) {
		LOG_DBG("The library is already initialized and configured");
		return 0;
	}

	err = lte_lc_system_mode_get(&sys_mode_current, &mode_pref_current);
	if (err) {
		LOG_ERR("Could not get current system mode, error: %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_LTE_NETWORK_MODE_DEFAULT)) {
		sys_mode_target = sys_mode_current;

		LOG_DBG("Default system mode is used: %d", sys_mode_current);
	}

	if ((sys_mode_current != sys_mode_target) ||
	    (mode_pref_current != mode_pref_target)) {
		err = lte_lc_system_mode_set(sys_mode_target, mode_pref_target);
		if (err) {
			LOG_ERR("Could not set system mode, error: %d", err);
			return err;
		}

		LOG_DBG("System mode (%d) and preference (%d) configured",
			sys_mode_target, mode_pref_target);
	} else {
		LOG_DBG("System mode (%d) and preference (%d) are already configured",
			sys_mode_current, mode_pref_current);
	}

	lte_lc_psm_default_config_set();

	is_initialized = true;

	return 0;
}

static int connect_lte(bool blocking)
{
	int err;
	int tries = (IS_ENABLED(CONFIG_LTE_NETWORK_USE_FALLBACK) ? 2 : 1);
	enum lte_lc_func_mode current_func_mode;
	enum lte_lc_nw_reg_status reg_status;
	static atomic_t in_progress;

	if (!is_initialized) {
		LOG_ERR("The LTE link controller is not initialized");
		return -EPERM;
	}

	/* Check if a connection attempt is already in progress */
	if (atomic_set(&in_progress, 1)) {
		return -EINPROGRESS;
	}

	err = lte_lc_nw_reg_status_get(&reg_status);
	if (err) {
		LOG_ERR("Failed to get current registration status");
		err = -EFAULT;
		goto exit;
	}

	/* Do not attempt to register with an LTE network if the device already is registered.
	 * This check is needed for blocking _connect() calls to avoid hanging for
	 * CONFIG_LTE_NETWORK_TIMEOUT seconds waiting for a semaphore that will not be given.
	 */
	if ((reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
	    (reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
		LOG_DBG("The device is already registered with an LTE network");

		err = 0;
		goto exit;
	}

	do {
		tries--;

		err = lte_lc_func_mode_get(&current_func_mode);
		if (err) {
			err = -EFAULT;
			goto exit;
		}

		/* Change the modem sys-mode only if it's not running or is meant to change */
		if (!IS_ENABLED(CONFIG_LTE_NETWORK_MODE_DEFAULT) &&
		    ((current_func_mode == LTE_LC_FUNC_MODE_POWER_OFF) ||
		     (current_func_mode == LTE_LC_FUNC_MODE_OFFLINE))) {
			err = lte_lc_system_mode_set(sys_mode_target, mode_pref_current);
			if (err) {
				err = -EFAULT;
				goto exit;
			}
		}

		/* Reset the semaphore, it may have already been given
		 * by an earlier +CEREG notification.
		 */
		k_sem_reset(&link);

		err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_NORMAL);
		if (err || !blocking) {
			goto exit;
		}

		err = k_sem_take(&link, K_SECONDS(CONFIG_LTE_NETWORK_TIMEOUT));
		if (err == -EAGAIN) {
			LOG_INF("Network connection attempt timed out");

			if (IS_ENABLED(CONFIG_LTE_NETWORK_USE_FALLBACK) &&
			    (tries > 0)) {
				if (sys_mode_target == SYS_MODE_PREFERRED) {
					sys_mode_target = sys_mode_fallback;
				} else {
					sys_mode_target = SYS_MODE_PREFERRED;
				}

				err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE);
				if (err) {
					err = -EFAULT;
					goto exit;
				}

				LOG_INF("Using fallback network mode");
			} else {
				err = -ETIMEDOUT;
			}
		} else {
			tries = 0;
		}
	} while (tries > 0);

exit:
	atomic_clear(&in_progress);

	return err;
}

static int init_and_connect(void)
{
	int err;

	err = lte_lc_init();
	if (err) {
		return err;
	}

	return connect_lte(true);
}

static int feaconf_write(enum feaconf_feat feat, bool state)
{
	return nrf_modem_at_printf("AT%%FEACONF=%d,%d,%u", FEACONF_OPER_WRITE, feat, state);
}

/* Public API */

int lte_lc_init(void)
{
	int err = init_and_config();

	return err ? -EFAULT : 0;
}

void lte_lc_register_handler(lte_lc_evt_handler_t handler)
{
	if (handler == NULL) {
		LOG_INF("NULL as a handler received: Nothing to be done.\n"
			"The handler can be deregistered using lte_lc_deregister_handler()");
		return;
	}

	event_handler_list_append_handler(handler);
}

int lte_lc_deregister_handler(lte_lc_evt_handler_t handler)
{
	if (!is_initialized) {
		LOG_ERR("Module not initialized yet");
		return -EINVAL;
	}
	if (handler == NULL) {
		LOG_ERR("Invalid handler (handler=0x%08X)", (uint32_t)handler);
		return -EINVAL;
	}

	return event_handler_list_remove_handler(handler);
}

int lte_lc_connect(void)
{
	return connect_lte(true);
}

int lte_lc_init_and_connect(void)
{
	return init_and_connect();
}

int lte_lc_connect_async(lte_lc_evt_handler_t handler)
{
	if (handler) {
		event_handler_list_append_handler(handler);
	} else if (event_handler_list_is_empty()) {
		LOG_ERR("No handler registered");
		return -EINVAL;
	}

	return connect_lte(false);
}

int lte_lc_init_and_connect_async(lte_lc_evt_handler_t handler)
{
	int err;

	err = init_and_config();
	if (err) {
		return -EFAULT;
	}

	return lte_lc_connect_async(handler);
}

int lte_lc_deinit(void)
{
	if (is_initialized) {
		is_initialized = false;

		return lte_lc_func_mode_set(LTE_LC_FUNC_MODE_POWER_OFF) ? -EFAULT : 0;
	}

	return 0;
}

int lte_lc_normal(void)
{
	return lte_lc_func_mode_set(LTE_LC_FUNC_MODE_NORMAL) ? -EFAULT : 0;
}

int lte_lc_offline(void)
{
	return lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE) ? -EFAULT : 0;
}

int lte_lc_power_off(void)
{
	return lte_lc_func_mode_set(LTE_LC_FUNC_MODE_POWER_OFF) ? -EFAULT : 0;
}

int lte_lc_psm_param_set(const char *rptau, const char *rat)
{
	if ((rptau != NULL && strlen(rptau) != 8) ||
	    (rat != NULL && strlen(rat) != 8)) {
		return -EINVAL;
	}

	if (rptau != NULL) {
		strcpy(psm_param_rptau, rptau);
		LOG_DBG("RPTAU set to %s", psm_param_rptau);
	} else {
		*psm_param_rptau = '\0';
		LOG_DBG("Using modem default value for RPTAU");
	}

	if (rat != NULL) {
		strcpy(psm_param_rat, rat);
		LOG_DBG("RAT set to %s", psm_param_rat);
	} else {
		*psm_param_rat = '\0';
		LOG_DBG("Using modem default value for RAT");
	}

	return 0;
}

int lte_lc_psm_param_set_seconds(int rptau, int rat)
{
	int ret;

	ret = encode_psm(psm_param_rptau, psm_param_rat, rptau, rat);

	if (ret != 0) {
		*psm_param_rptau = '\0';
		*psm_param_rat = '\0';
	}

	LOG_DBG("RPTAU=%d (%s), RAT=%d (%s), ret=%d",
		rptau, psm_param_rptau, rat, psm_param_rat, ret);

	return ret;
}

int lte_lc_psm_req(bool enable)
{
	int err;

	LOG_DBG("enable=%d, tau=%s, rat=%s", enable, psm_param_rptau, psm_param_rat);

	if (enable) {
		if (strlen(psm_param_rptau) == 8 &&
		    strlen(psm_param_rat) == 8) {
			err = nrf_modem_at_printf("AT+CPSMS=1,,,\"%s\",\"%s\"",
						  psm_param_rptau,
						  psm_param_rat);
		} else if (strlen(psm_param_rptau) == 8) {
			err = nrf_modem_at_printf("AT+CPSMS=1,,,\"%s\"", psm_param_rptau);
		} else if (strlen(psm_param_rat) == 8) {
			err = nrf_modem_at_printf("AT+CPSMS=1,,,,\"%s\"", psm_param_rat);
		} else {
			err = nrf_modem_at_printf("AT+CPSMS=1");
		}
	} else {
		err = nrf_modem_at_printf(psm_disable);
	}

	if (err) {
		LOG_ERR("nrf_modem_at_printf failed, reported error: %d", err);
		return -EFAULT;
	}

	return 0;
}

int lte_lc_psm_get(int *tau, int *active_time)
{
	int err;
	struct lte_lc_psm_cfg psm_cfg;
	char active_time_str[9] = {0};
	char tau_ext_str[9] = {0};
	char tau_legacy_str[9] = {0};
	static char response[160] = { 0 };
	const char ch = ',';
	char *comma_ptr;

	if ((tau == NULL) || (active_time == NULL)) {
		return -EINVAL;
	}

	/* Format of XMONITOR AT command response:
	 * %XMONITOR: <reg_status>,[<full_name>,<short_name>,<plmn>,<tac>,<AcT>,<band>,<cell_id>,
	 * <phys_cell_id>,<EARFCN>,<rsrp>,<snr>,<NW-provided_eDRX_value>,<Active-Time>,
	 * <Periodic-TAUext>,<Periodic-TAU>]
	 * We need to parse the three last parameters, Active-Time, Periodic-TAU-ext and
	 * Periodic-TAU. N.B. Periodic-TAU will not be present on modem firmwares < 1.2.0.
	 */

	response[0] = '\0';

	err = nrf_modem_at_cmd(response, sizeof(response), "AT%%XMONITOR");
	if (err) {
		LOG_ERR("AT command failed, error: %d", err);
		return -EFAULT;
	}

	comma_ptr = strchr(response, ch);
	if (!comma_ptr) {
		/* Not an AT error, thus must be that just a <reg_status> received:
		 * optional part is included in a response only when <reg_status> is 1 or 5.
		 */
		LOG_DBG("Not registered: cannot get current PSM configuration");
		return -EBADMSG;
	}

	/* Skip over first 13 fields in AT cmd response by counting delimiters (commas). */
	for (int i = 0; i < 12; i++) {
		if (comma_ptr) {
			comma_ptr = strchr(comma_ptr + 1, ch);
		} else {
			LOG_ERR("AT command parsing failed");
			return -EBADMSG;
		}
	}

	/* The last three fields of AT response looks something like this:
	 * ,"00011110","00000111","01001001"
	 * comma_ptr now points the comma before Active-Time. Discard the comma and the quote mark,
	 * hence + 2, and copy Active-Time into active_time_str. Find the next comma and repeat for
	 * Periodic-TAU-ext and so forth.
	 */

	if (comma_ptr) {
		strncpy(active_time_str, comma_ptr + 2, 8);
	} else {
		LOG_ERR("AT command parsing failed");
		return -EBADMSG;
	}

	comma_ptr = strchr(comma_ptr + 1, ch);
	if (comma_ptr) {
		strncpy(tau_ext_str, comma_ptr + 2, 8);
	} else {
		LOG_ERR("AT command parsing failed");
		return -EBADMSG;
	}

	/* It's ok not to have legacy Periodic-TAU, older FWs don't provide it. */
	comma_ptr = strchr(comma_ptr + 1, ch);
	if (comma_ptr) {
		strncpy(tau_legacy_str, comma_ptr + 2, 8);
	}

	err = parse_psm(active_time_str, tau_ext_str, tau_legacy_str, &psm_cfg);
	if (err) {
		LOG_ERR("Failed to parse PSM configuration, error: %d", err);
		return err;
	}

	*tau = psm_cfg.tau;
	*active_time = psm_cfg.active_time;

	LOG_DBG("TAU: %d sec, active time: %d sec", *tau, *active_time);

	return 0;
}

int lte_lc_proprietary_psm_req(bool enable)
{
	if (feaconf_write(FEACONF_FEAT_PROPRIETARY_PSM, enable) != 0) {
		return -EFAULT;
	}

	return 0;
}

int lte_lc_edrx_param_set(enum lte_lc_lte_mode mode, const char *edrx)
{
	char *edrx_param;

	if (mode != LTE_LC_LTE_MODE_LTEM && mode != LTE_LC_LTE_MODE_NBIOT) {
		LOG_ERR("LTE mode must be LTE-M or NB-IoT");
		return -EINVAL;
	}

	if (edrx != NULL && strlen(edrx) != 4) {
		return -EINVAL;
	}

	edrx_param = (mode == LTE_LC_LTE_MODE_LTEM) ? edrx_param_ltem :
						      edrx_param_nbiot;

	if (edrx) {
		strcpy(edrx_param, edrx);
		LOG_DBG("eDRX set to %s for %s", edrx_param,
			(mode == LTE_LC_LTE_MODE_LTEM) ? "LTE-M" : "NB-IoT");
	} else {
		*edrx_param = '\0';
		LOG_DBG("eDRX use default for %s",
			(mode == LTE_LC_LTE_MODE_LTEM) ? "LTE-M" : "NB-IoT");
	}

	return 0;
}

int lte_lc_ptw_set(enum lte_lc_lte_mode mode, const char *ptw)
{
	char *ptw_param;

	if (mode != LTE_LC_LTE_MODE_LTEM && mode != LTE_LC_LTE_MODE_NBIOT) {
		LOG_ERR("LTE mode must be LTE-M or NB-IoT");
		return -EINVAL;
	}

	if (ptw != NULL && strlen(ptw) != 4) {
		return -EINVAL;
	}

	ptw_param = (mode == LTE_LC_LTE_MODE_LTEM) ? ptw_param_ltem :
						     ptw_param_nbiot;

	if (ptw != NULL) {
		strcpy(ptw_param, ptw);
		LOG_DBG("PTW set to %s for %s", ptw_param,
			(mode == LTE_LC_LTE_MODE_LTEM) ? "LTE-M" : "NB-IoT");
	} else {
		*ptw_param = '\0';
		LOG_DBG("PTW use default for %s",
			(mode == LTE_LC_LTE_MODE_LTEM) ? "LTE-M" : "NB-IoT");
	}

	return 0;
}

int lte_lc_edrx_req(bool enable)
{
	int err;
	int actt[] = {AT_CEDRXS_ACTT_WB, AT_CEDRXS_ACTT_NB};

	if (!enable) {
		err = nrf_modem_at_printf(edrx_disable);
		if (err) {
			LOG_ERR("Failed to disable eDRX, reported error: %d", err);
			return -EFAULT;
		}

		return 0;
	}

	/* Apply the configurations for both LTE-M and NB-IoT. */
	for (size_t i = 0; i < ARRAY_SIZE(actt); i++) {
		char *edrx_param = (actt[i] == AT_CEDRXS_ACTT_WB) ?
					edrx_param_ltem : edrx_param_nbiot;
		char *ptw_param = (actt[i] == AT_CEDRXS_ACTT_WB) ?
					ptw_param_ltem : ptw_param_nbiot;

		if (strlen(edrx_param) == 4) {
			err = nrf_modem_at_printf("AT+CEDRXS=2,%d,\"%s\"", actt[i], edrx_param);
		} else {
			err = nrf_modem_at_printf("AT+CEDRXS=2,%d", actt[i]);
		}

		if (err) {
			LOG_ERR("Failed to enable eDRX, reported error: %d", err);
			return -EFAULT;
		}

		/* PTW must be requested after eDRX is enabled */
		if (strlen(ptw_param) != 4) {
			continue;
		}

		err = nrf_modem_at_printf("AT%%XPTW=%d,\"%s\"", actt[i], ptw_param);
		if (err) {
			LOG_ERR("Failed to request PTW, reported error: %d", err);
			return -EFAULT;
		}
	}

	return 0;
}

int lte_lc_edrx_get(struct lte_lc_edrx_cfg *edrx_cfg)
{
	int err;
	char response[48];

	if (edrx_cfg == NULL) {
		return -EINVAL;
	}

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CEDRXRDP");
	if (err) {
		LOG_ERR("Failed to request eDRX parameters, error: %d", err);
		return -EFAULT;
	}

	err = parse_edrx(response, edrx_cfg);
	if (err) {
		LOG_ERR("Failed to parse eDRX parameters, error: %d", err);
		return -EBADMSG;
	}

	return 0;
}

int lte_lc_rai_req(bool enable)
{
	int err;
	enum lte_lc_system_mode mode;

	err = lte_lc_system_mode_get(&mode, NULL);
	if (err) {
		return -EFAULT;
	}

	switch (mode) {
	case LTE_LC_SYSTEM_MODE_LTEM:
	case LTE_LC_SYSTEM_MODE_LTEM_GPS:
		LOG_ERR("RAI not supported for LTE-M networks");
		return -EOPNOTSUPP;
	case LTE_LC_SYSTEM_MODE_NBIOT:
	case LTE_LC_SYSTEM_MODE_NBIOT_GPS:
	case LTE_LC_SYSTEM_MODE_LTEM_NBIOT:
	case LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS:
		break;
	default:
		LOG_ERR("Unknown system mode");
		LOG_ERR("Cannot request RAI for unknown system mode");
		return -EOPNOTSUPP;
	}

	if (enable) {
		err = nrf_modem_at_printf("AT%%XRAI=%s", rai_param);
	} else {
		err = nrf_modem_at_printf(rai_disable);
	}

	if (err) {
		LOG_ERR("nrf_modem_at_printf failed, reported error: %d", err);
		return -EFAULT;
	}

	return 0;
}

int lte_lc_rai_param_set(const char *value)
{
	if (value == NULL || strlen(value) != 1) {
		return -EINVAL;
	}

	if (value[0] == '3' || value[0] == '4') {
		memcpy(rai_param, value, 2);
	} else {
		return -EINVAL;
	}

	return 0;
}

int lte_lc_nw_reg_status_get(enum lte_lc_nw_reg_status *status)
{
	int err;
	uint16_t status_tmp;
	uint32_t cell_id = 0;

	if (status == NULL) {
		return -EINVAL;
	}

	/* Read network registration status */
	err = nrf_modem_at_scanf("AT+CEREG?",
		"+CEREG: "
		"%*u,"		/* <n> */
		"%hu,"		/* <stat> */
		"%*[^,],"	/* <tac> */
		"\"%x\",",	/* <ci> */
		&status_tmp,
		&cell_id);
	if (err < 1) {
		LOG_ERR("Could not get registration status, error: %d", err);
		return -EFAULT;
	}

	if (!is_cellid_valid(cell_id)) {
		*status = LTE_LC_NW_REG_UNKNOWN;
	} else {
		*status = status_tmp;
	}

	return 0;
}

int lte_lc_system_mode_set(enum lte_lc_system_mode mode,
			   enum lte_lc_system_mode_preference preference)
{
	int err;

	switch (mode) {
	case LTE_LC_SYSTEM_MODE_LTEM:
	case LTE_LC_SYSTEM_MODE_LTEM_GPS:
	case LTE_LC_SYSTEM_MODE_NBIOT:
	case LTE_LC_SYSTEM_MODE_NBIOT_GPS:
	case LTE_LC_SYSTEM_MODE_GPS:
	case LTE_LC_SYSTEM_MODE_LTEM_NBIOT:
	case LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS:
		break;
	default:
		LOG_ERR("Invalid system mode requested: %d", mode);
		return -EINVAL;
	}

	switch (preference) {
	case LTE_LC_SYSTEM_MODE_PREFER_AUTO:
	case LTE_LC_SYSTEM_MODE_PREFER_LTEM:
	case LTE_LC_SYSTEM_MODE_PREFER_NBIOT:
	case LTE_LC_SYSTEM_MODE_PREFER_LTEM_PLMN_PRIO:
	case LTE_LC_SYSTEM_MODE_PREFER_NBIOT_PLMN_PRIO:
		break;
	default:
		LOG_ERR("Invalid LTE preference requested: %d", preference);
		return -EINVAL;
	}

	err = nrf_modem_at_printf("AT%%XSYSTEMMODE=%s,%c",
				  system_mode_params[mode],
				  system_mode_preference[preference]);
	if (err) {
		LOG_ERR("Could not send AT command, error: %d", err);
		return -EFAULT;
	}

	sys_mode_current = mode;
	sys_mode_target = mode;
	mode_pref_current = preference;
	mode_pref_target = preference;

	return 0;
}

int lte_lc_system_mode_get(enum lte_lc_system_mode *mode,
			   enum lte_lc_system_mode_preference *preference)
{
	int err;
	int mode_bitmask = 0;
	int ltem_mode = 0;
	int nbiot_mode = 0;
	int gps_mode = 0;
	int mode_preference = 0;

	if (mode == NULL) {
		return -EINVAL;
	}

	/* It's expected to have all 4 arguments matched */
	err = nrf_modem_at_scanf(AT_XSYSTEMMODE_READ, "%%XSYSTEMMODE: %d,%d,%d,%d",
				 &ltem_mode, &nbiot_mode, &gps_mode, &mode_preference);
	if (err != 4) {
		LOG_ERR("Failed to get system mode, error: %d", err);
		return -EFAULT;
	}

	mode_bitmask = (ltem_mode ? BIT(AT_XSYSTEMMODE_READ_LTEM_INDEX) : 0) |
		       (nbiot_mode ? BIT(AT_XSYSTEMMODE_READ_NBIOT_INDEX) : 0) |
		       (gps_mode ? BIT(AT_XSYSTEMMODE_READ_GPS_INDEX) : 0);

	switch (mode_bitmask) {
	case BIT(AT_XSYSTEMMODE_READ_LTEM_INDEX):
		*mode = LTE_LC_SYSTEM_MODE_LTEM;
		break;
	case BIT(AT_XSYSTEMMODE_READ_NBIOT_INDEX):
		*mode = LTE_LC_SYSTEM_MODE_NBIOT;
		break;
	case BIT(AT_XSYSTEMMODE_READ_GPS_INDEX):
		*mode = LTE_LC_SYSTEM_MODE_GPS;
		break;
	case (BIT(AT_XSYSTEMMODE_READ_LTEM_INDEX) | BIT(AT_XSYSTEMMODE_READ_GPS_INDEX)):
		*mode = LTE_LC_SYSTEM_MODE_LTEM_GPS;
		break;
	case (BIT(AT_XSYSTEMMODE_READ_NBIOT_INDEX) | BIT(AT_XSYSTEMMODE_READ_GPS_INDEX)):
		*mode = LTE_LC_SYSTEM_MODE_NBIOT_GPS;
		break;
	case (BIT(AT_XSYSTEMMODE_READ_LTEM_INDEX) | BIT(AT_XSYSTEMMODE_READ_NBIOT_INDEX)):
		*mode = LTE_LC_SYSTEM_MODE_LTEM_NBIOT;
		break;
	case (BIT(AT_XSYSTEMMODE_READ_LTEM_INDEX) |
	      BIT(AT_XSYSTEMMODE_READ_NBIOT_INDEX) |
	      BIT(AT_XSYSTEMMODE_READ_GPS_INDEX)):
		*mode = LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS;
		break;
	default:
		LOG_ERR("Invalid system mode, assuming parsing error");
		return -EFAULT;
	}

	/* Get LTE preference. */
	if (preference != NULL) {
		switch (mode_preference) {
		case 0:
			*preference = LTE_LC_SYSTEM_MODE_PREFER_AUTO;
			break;
		case 1:
			*preference = LTE_LC_SYSTEM_MODE_PREFER_LTEM;
			break;
		case 2:
			*preference = LTE_LC_SYSTEM_MODE_PREFER_NBIOT;
			break;
		case 3:
			*preference = LTE_LC_SYSTEM_MODE_PREFER_LTEM_PLMN_PRIO;
			break;
		case 4:
			*preference = LTE_LC_SYSTEM_MODE_PREFER_NBIOT_PLMN_PRIO;
			break;
		default:
			LOG_ERR("Unsupported LTE preference: %d", mode_preference);
			return -EFAULT;
		}
	}

	return 0;
}

int lte_lc_func_mode_get(enum lte_lc_func_mode *mode)
{
	int err;
	uint16_t mode_tmp;

	if (mode == NULL) {
		return -EINVAL;
	}

	/* Exactly one parameter is expected to match. */
	err = nrf_modem_at_scanf(AT_CFUN_READ, "+CFUN: %hu", &mode_tmp);
	if (err != 1) {
		LOG_ERR("AT command failed, nrf_modem_at_scanf() returned error: %d", err);
		return -EFAULT;
	}

	*mode = mode_tmp;

	return 0;
}

int lte_lc_func_mode_set(enum lte_lc_func_mode mode)
{
	int err;

	switch (mode) {
	case LTE_LC_FUNC_MODE_ACTIVATE_LTE:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_ACTIVATE_LTE);

		err = enable_notifications();
		if (err) {
			LOG_ERR("Failed to enable notifications, error: %d", err);
			return -EFAULT;
		}

		break;
	case LTE_LC_FUNC_MODE_NORMAL:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_NORMAL);

		err = enable_notifications();
		if (err) {
			LOG_ERR("Failed to enable notifications, error: %d", err);
			return -EFAULT;
		}

		break;
	case LTE_LC_FUNC_MODE_POWER_OFF:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_POWER_OFF);
		break;
	case LTE_LC_FUNC_MODE_RX_ONLY:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_RX_ONLY);
		break;
	case LTE_LC_FUNC_MODE_OFFLINE:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_OFFLINE);
		break;
	case LTE_LC_FUNC_MODE_DEACTIVATE_LTE:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_DEACTIVATE_LTE);
		break;
	case LTE_LC_FUNC_MODE_DEACTIVATE_GNSS:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_DEACTIVATE_GNSS);
		break;
	case LTE_LC_FUNC_MODE_ACTIVATE_GNSS:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_ACTIVATE_GNSS);
		break;
	case LTE_LC_FUNC_MODE_DEACTIVATE_UICC:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_DEACTIVATE_UICC);
		break;
	case LTE_LC_FUNC_MODE_ACTIVATE_UICC:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_ACTIVATE_UICC);
		break;
	case LTE_LC_FUNC_MODE_OFFLINE_UICC_ON:
		LTE_LC_TRACE(LTE_LC_TRACE_FUNC_MODE_OFFLINE_UICC_ON);
		break;
	default:
		LOG_ERR("Invalid functional mode: %d", mode);
		return -EINVAL;
	}

	err = nrf_modem_at_printf("AT+CFUN=%d", mode);
	if (err) {
		LOG_ERR("Failed to set functional mode. Please check XSYSTEMMODE.");
		return -EFAULT;
	}

	STRUCT_SECTION_FOREACH(lte_lc_cfun_cb, e) {
		LOG_DBG("CFUN monitor callback: %p", e->callback);
		e->callback(mode, e->context);
	}

	return 0;
}

int lte_lc_lte_mode_get(enum lte_lc_lte_mode *mode)
{
	int err;
	uint16_t mode_tmp;

	if (mode == NULL) {
		return -EINVAL;
	}

	err = nrf_modem_at_scanf(AT_CEREG_READ,
		"+CEREG: "
		"%*u,"		/* <n> */
		"%*u,"		/* <stat> */
		"%*[^,],"	/* <tac> */
		"%*[^,],"	/* <ci> */
		"%hu",		/* <AcT> */
		&mode_tmp);
	if (err == -NRF_EBADMSG) {
		/* The AT command was successful, but there were no matches.
		 * This is not an error, but the LTE mode is unknown.
		 */
		*mode = LTE_LC_LTE_MODE_NONE;

		return 0;
	} else if (err < 1) {
		LOG_ERR("Could not get the LTE mode, error: %d", err);
		return -EFAULT;
	}

	*mode = mode_tmp;

	switch (*mode) {
	case LTE_LC_LTE_MODE_NONE:
	case LTE_LC_LTE_MODE_LTEM:
	case LTE_LC_LTE_MODE_NBIOT:
		break;
	default:
		return -EBADMSG;
	}

	return 0;
}

int lte_lc_neighbor_cell_measurement(struct lte_lc_ncellmeas_params *params)
{
	int err;
	/* lte_lc defaults for the used params */
	struct lte_lc_ncellmeas_params used_params = {
		.search_type = LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT,
		.gci_count = 0,
	};

	__ASSERT(!IN_RANGE(
			(int)params,
			LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT,
			LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_COMPLETE),
		 "Invalid argument, API does not accept enum values directly anymore");

	if (k_sem_take(&ncellmeas_idle_sem, K_SECONDS(1)) != 0) {
		LOG_WRN("Neighbor cell measurement already in progress");
		return -EINPROGRESS;
	}

	if (params != NULL) {
		used_params = *params;
	}

	/* Starting from modem firmware v1.3.1, there is an optional parameter to specify
	 * the type of search.
	 * If the type is LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT, we therefore use the AT
	 * command without parameters to avoid error messages for older firmware version.
	 * Starting from modem firmware v1.3.4, additional CGI search types and
	 * GCI count are supported.
	 */
	if (used_params.search_type == LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_LIGHT) {
		err = nrf_modem_at_printf("AT%%NCELLMEAS=1");
	} else if (used_params.search_type == LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_COMPLETE) {
		err = nrf_modem_at_printf("AT%%NCELLMEAS=2");
	} else if (used_params.search_type == LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_DEFAULT) {
		err = nrf_modem_at_printf("AT%%NCELLMEAS=3,%d", used_params.gci_count);
	} else if (used_params.search_type == LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_LIGHT) {
		err = nrf_modem_at_printf("AT%%NCELLMEAS=4,%d", used_params.gci_count);
	} else if (used_params.search_type == LTE_LC_NEIGHBOR_SEARCH_TYPE_GCI_EXTENDED_COMPLETE) {
		err = nrf_modem_at_printf("AT%%NCELLMEAS=5,%d", used_params.gci_count);
	} else {
		/* Defaulting to use LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT */
		err = nrf_modem_at_printf("AT%%NCELLMEAS");
	}

	if (err) {
		err = -EFAULT;
		k_sem_give(&ncellmeas_idle_sem);
	} else {
		ncellmeas_params = used_params;
	}

	return err;
}

int lte_lc_neighbor_cell_measurement_cancel(void)
{
	int err = nrf_modem_at_printf(AT_NCELLMEAS_STOP);

	if (err) {
		err = -EFAULT;
	}

	k_sem_give(&ncellmeas_idle_sem);

	return err;
}

int lte_lc_conn_eval_params_get(struct lte_lc_conn_eval_params *params)
{
	int err;
	enum lte_lc_func_mode mode;
	int result;
	/* PLMN field is a string of maximum 6 characters, plus null termination. */
	char plmn_str[7] = {0};
	uint16_t rrc_state_tmp, energy_estimate_tmp, tau_trig_tmp, ce_level_tmp;

	if (params == NULL) {
		return -EINVAL;
	}

	err = lte_lc_func_mode_get(&mode);
	if (err) {
		LOG_ERR("Could not get functional mode");
		return -EFAULT;
	}

	switch (mode) {
	case LTE_LC_FUNC_MODE_NORMAL:
	case LTE_LC_FUNC_MODE_ACTIVATE_LTE:
	case LTE_LC_FUNC_MODE_RX_ONLY:
		break;
	default:
		LOG_WRN("Connection evaluation is not available in the current functional mode");
		return -EOPNOTSUPP;
	}

	/* AT%CONEVAL response format, from nRF91 AT Commands - Command Reference Guide, v1.7:
	 *
	 * %CONEVAL: <result>[,<rrc_state>,<energy_estimate>,<rsrp>,<rsrq>,<snr>,<cell_id>,<plmn>,
	 * <phys_cell_id>,<earfcn>,<band>,<tau_triggered>,<ce_level>,<tx_power>,<tx_repetitions>,
	 * <rx_repetitions>,<dl-pathloss>]
	 *
	 * In total, 17 parameters are expected to match for a successful command response.
	 */
	err = nrf_modem_at_scanf(
		AT_CONEVAL_READ,
		"%%CONEVAL: "
		"%d,"		/* <result> */
		"%hu,"		/* <rrc_state> */
		"%hu,"		/* <energy_estimate> */
		"%hd,"		/* <rsrp> */
		"%hd,"		/* <rsrq> */
		"%hd,"		/* <snr> */
		"\"%x\","	/* <cell_id> */
		"\"%6[^\"]\","	/* <plmn> */
		"%hd,"		/* <phys_cell_id> */
		"%d,"		/* <earfcn> */
		"%hd,"		/* <band> */
		"%hu,"		/* <tau_triggered> */
		"%hu,"		/* <ce_level> */
		"%hd,"		/* <tx_power> */
		"%hd,"		/* <tx_repetitions> */
		"%hd,"		/* <rx_repetitions> */
		"%hd",		/* <dl-pathloss> */
		&result, &rrc_state_tmp, &energy_estimate_tmp,
		&params->rsrp, &params->rsrq, &params->snr, &params->cell_id,
		plmn_str, &params->phy_cid, &params->earfcn, &params->band,
		&tau_trig_tmp, &ce_level_tmp, &params->tx_power,
		&params->tx_rep, &params->rx_rep, &params->dl_pathloss);
	if (err < 0) {
		LOG_ERR("AT command failed, error: %d", err);
		return -EFAULT;
	} else if (result != 0) {
		LOG_WRN("Connection evaluation failed with reason: %d", result);
		return result;
	} else if (err != 17) {
		LOG_ERR("AT command parsing failed, error: %d", err);
		return -EBADMSG;
	}

	params->rrc_state = rrc_state_tmp;
	params->energy_estimate = energy_estimate_tmp;
	params->tau_trig = tau_trig_tmp;
	params->ce_level = ce_level_tmp;

	/* Read MNC and store as integer. The MNC starts as the fourth character
	 * in the string, following three characters long MCC.
	 */
	err = string_to_int(&plmn_str[3], 10, &params->mnc);
	if (err) {
		return -EBADMSG;
	}

	/* Null-terminated MCC, read and store it. */
	plmn_str[3] = '\0';

	err = string_to_int(plmn_str, 10, &params->mcc);
	if (err) {
		return -EBADMSG;
	}

	return 0;
}

int lte_lc_modem_events_enable(void)
{
	/* First try to enable both warning and informational type events, which is only supported
	 * by modem firmware versions >= 2.0.0.
	 * If that fails, try to enable the legacy set of events.
	 */
	if (nrf_modem_at_printf(AT_MDMEV_ENABLE_2)) {
		if (nrf_modem_at_printf(AT_MDMEV_ENABLE_1)) {
			return -EFAULT;
		}
	}

	return 0;
}

int lte_lc_modem_events_disable(void)
{
	return nrf_modem_at_printf(AT_MDMEV_DISABLE) ? -EFAULT : 0;
}

int lte_lc_periodic_search_set(const struct lte_lc_periodic_search_cfg *const cfg)
{
	int err;
	char pattern_buf[4][40];

	if (!cfg || (cfg->pattern_count == 0) || (cfg->pattern_count > 4)) {
		return -EINVAL;
	}

	/* Command syntax:
	 *	AT%PERIODICSEARCHCONF=<mode>[,<loop>,<return_to_pattern>,<band_optimization>,
	 *	<pattern_1>[,<pattern_2>][,<pattern_3>][,<pattern_4>]]
	 */

	err = nrf_modem_at_printf(
		"AT%%PERIODICSEARCHCONF=0," /* Write mode */
		"%hu,"  /* <loop> */
		"%hu,"  /* <return_to_pattern> */
		"%hu,"  /* <band_optimization> */
		"%s%s" /* <pattern_1> */
		"%s%s" /* <pattern_2> */
		"%s%s" /* <pattern_3> */
		"%s",  /* <pattern_4> */
		cfg->loop, cfg->return_to_pattern, cfg->band_optimization,
		/* Pattern 1 */
		periodic_search_pattern_get(pattern_buf[0], sizeof(pattern_buf[0]),
					    &cfg->patterns[0]),
		/* Pattern 2, if configured */
		cfg->pattern_count > 1 ? "," : "",
		cfg->pattern_count > 1 ? periodic_search_pattern_get(pattern_buf[1],
						sizeof(pattern_buf[1]), &cfg->patterns[1]) : "",
		/* Pattern 3, if configured */
		cfg->pattern_count > 2 ? "," : "",
		cfg->pattern_count > 2 ? periodic_search_pattern_get(pattern_buf[2],
						sizeof(pattern_buf[2]), &cfg->patterns[2]) : "",
		/* Pattern 4, if configured */
		cfg->pattern_count > 3 ? "," : "",
		cfg->pattern_count > 3 ? periodic_search_pattern_get(pattern_buf[3],
						sizeof(pattern_buf[3]), &cfg->patterns[3]) : ""
	);
	if (err < 0) {
		/* Failure to send the AT command. */
		LOG_ERR("AT command failed, returned error code: %d", err);
		return -EFAULT;
	} else if (err > 0) {
		/* The modem responded with "ERROR". */
		LOG_ERR("The modem rejected the configuration");
		return -EBADMSG;
	}

	return 0;
}

int lte_lc_periodic_search_clear(void)
{
	int err;

	err = nrf_modem_at_printf("AT%%PERIODICSEARCHCONF=2");
	if (err < 0) {
		return -EFAULT;
	} else if (err > 0) {
		return -EBADMSG;
	}

	return 0;
}

int lte_lc_periodic_search_request(void)
{
	return nrf_modem_at_printf("AT%%PERIODICSEARCHCONF=3") ? -EFAULT : 0;
}

int lte_lc_periodic_search_get(struct lte_lc_periodic_search_cfg *const cfg)
{

	int err;
	char pattern_buf[4][40];
	uint16_t loop_tmp;

	if (!cfg) {
		return -EINVAL;
	}

	/* Response format:
	 *	%PERIODICSEARCHCONF=<loop>,<return_to_pattern>,<band_optimization>,<pattern_1>
	 *	[,<pattern_2>][,<pattern_3>][,<pattern_4>]
	 */

	err = nrf_modem_at_scanf("AT%PERIODICSEARCHCONF=1",
		"%%PERIODICSEARCHCONF: "
		"%hu," /* <loop> */
		"%hu," /* <return_to_pattern> */
		"%hu," /* <band_optimization> */
		"\"%40[^\"]\","  /* <pattern_1> */
		"\"%40[^\"]\","  /* <pattern_2> */
		"\"%40[^\"]\","  /* <pattern_3> */
		"\"%40[^\"]\"",  /* <pattern_4> */
		&loop_tmp, &cfg->return_to_pattern, &cfg->band_optimization,
		pattern_buf[0], pattern_buf[1], pattern_buf[2], pattern_buf[3]
	);
	if (err == -NRF_EBADMSG) {
		return -ENOENT;
	} else if (err < 0) {
		return -EFAULT;
	} else if (err < 4) {
		/* No pattern found */
		return -EBADMSG;
	}

	cfg->loop = loop_tmp;

	/* Pattern count is matched parameters minus 3 for loop, return_to_pattern
	 * and band_optimization.
	 */
	cfg->pattern_count = err - 3;

	if (cfg->pattern_count >= 1) {
		LOG_DBG("Pattern 1: %s", pattern_buf[0]);

		err = parse_periodic_search_pattern(pattern_buf[0], &cfg->patterns[0]);
		if (err) {
			LOG_ERR("Failed to parse periodic search pattern");
			return err;
		}
	}

	if (cfg->pattern_count >= 2) {
		LOG_DBG("Pattern 2: %s", pattern_buf[1]);

		err = parse_periodic_search_pattern(pattern_buf[1], &cfg->patterns[1]);
		if (err) {
			LOG_ERR("Failed to parse periodic search pattern");
			return err;
		}
	}

	if (cfg->pattern_count >= 3) {
		LOG_DBG("Pattern 3: %s", pattern_buf[2]);

		err = parse_periodic_search_pattern(pattern_buf[2], &cfg->patterns[2]);
		if (err) {
			LOG_ERR("Failed to parse periodic search pattern");
			return err;
		}
	}

	if (cfg->pattern_count == 4) {
		LOG_DBG("Pattern 4: %s", pattern_buf[3]);

		err = parse_periodic_search_pattern(pattern_buf[3], &cfg->patterns[3]);
		if (err) {
			LOG_ERR("Failed to parse periodic search pattern");
			return err;
		}
	}

	return 0;
}

int lte_lc_reduced_mobility_get(enum lte_lc_reduced_mobility_mode *mode)
{
	int ret;
	uint16_t mode_tmp;

	if (mode == NULL) {
		return -EINVAL;
	}

	ret = nrf_modem_at_scanf("AT%REDMOB?", "%%REDMOB: %hu", &mode_tmp);
	if (ret != 1) {
		LOG_ERR("AT command failed, nrf_modem_at_scanf() returned error: %d", ret);
		return -EFAULT;
	}

	*mode = mode_tmp;

	return 0;
}

int lte_lc_reduced_mobility_set(enum lte_lc_reduced_mobility_mode mode)
{
	int ret = nrf_modem_at_printf("AT%%REDMOB=%d", mode);

	if (ret) {
		/* Failure to send the AT command. */
		LOG_ERR("AT command failed, returned error code: %d", ret);
		return -EFAULT;
	}

	return 0;
}

int lte_lc_factory_reset(enum lte_lc_factory_reset_type type)
{
	return nrf_modem_at_printf("AT%%XFACTORYRESET=%d", type) ? -EFAULT : 0;
}
