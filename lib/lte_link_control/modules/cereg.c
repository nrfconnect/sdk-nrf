/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <modem/lte_lc.h>
#include <modem/lte_lc_trace.h>
#include <modem/at_monitor.h>
#include <modem/at_parser.h>
#include <nrf_modem_at.h>
#include <nrf_errno.h>

#include "common/work_q.h"
#include "common/event_handler_list.h"
#include "modules/cereg.h"
#include "modules/cfun.h"
#include "modules/psm.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

#define AT_CEREG_READ "AT+CEREG?"
#define AT_CEREG_5    "AT+CEREG=5"

#define AT_CEREG_REG_STATUS_INDEX   1
#define AT_CEREG_TAC_INDEX	    2
#define AT_CEREG_CELL_ID_INDEX	    3
#define AT_CEREG_ACT_INDEX	    4
#define AT_CEREG_CAUSE_TYPE_INDEX   5
#define AT_CEREG_REJECT_CAUSE_INDEX 6
#define AT_CEREG_ACTIVE_TIME_INDEX  7
#define AT_CEREG_TAU_INDEX	    8

/* Previously received LTE mode as indicated by the modem */
static enum lte_lc_lte_mode prev_lte_mode = LTE_LC_LTE_MODE_NONE;

/* Network attach semaphore */
static K_SEM_DEFINE(link, 0, 1);

AT_MONITOR(ltelc_atmon_cereg, "+CEREG", at_handler_cereg);

static bool is_cellid_valid(uint32_t cellid)
{
	if (cellid == LTE_LC_CELL_EUTRAN_ID_INVALID) {
		return false;
	}

	return true;
}

static int parse_cereg(const char *at_response, enum lte_lc_nw_reg_status *reg_status,
		       struct lte_lc_cell *cell, enum lte_lc_lte_mode *lte_mode,
		       struct lte_lc_psm_cfg *psm_cfg)
{
	int err, temp;
	struct at_parser parser;
	char str_buf[10];
	size_t len = sizeof(str_buf);
	size_t count = 0;

	__ASSERT_NO_MSG(at_response != NULL);
	__ASSERT_NO_MSG(reg_status != NULL);
	__ASSERT_NO_MSG(cell != NULL);
	__ASSERT_NO_MSG(lte_mode != NULL);
	__ASSERT_NO_MSG(psm_cfg != NULL);

	/* Initialize all return values. */
	*reg_status = LTE_LC_NW_REG_UNKNOWN;
	(void)memset(cell, 0, sizeof(struct lte_lc_cell));
	cell->id = LTE_LC_CELL_EUTRAN_ID_INVALID;
	cell->tac = LTE_LC_CELL_TAC_INVALID;
	*lte_mode = LTE_LC_LTE_MODE_NONE;
	psm_cfg->active_time = -1;
	psm_cfg->tau = -1;

	err = at_parser_init(&parser, at_response);
	__ASSERT_NO_MSG(err == 0);

	/* Get network registration status */
	err = at_parser_num_get(&parser, AT_CEREG_REG_STATUS_INDEX, &temp);
	if (err) {
		LOG_ERR("Could not get registration status, error: %d", err);
		goto clean_exit;
	}

	*reg_status = temp;
	LOG_DBG("Network registration status: %d", *reg_status);

	err = at_parser_cmd_count_get(&parser, &count);
	if (err) {
		LOG_ERR("Could not get CEREG param count, potentially malformed notification, "
			"error: %d",
			err);
		goto clean_exit;
	}

	if ((*reg_status != LTE_LC_NW_REG_UICC_FAIL) && (count > AT_CEREG_CELL_ID_INDEX)) {
		/* Parse tracking area code */
		err = at_parser_string_get(&parser, AT_CEREG_TAC_INDEX, str_buf, &len);
		if (err) {
			LOG_DBG("Could not get tracking area code, error: %d", err);
		} else {
			cell->tac = strtoul(str_buf, NULL, 16);
		}

		/* Parse cell ID */
		len = sizeof(str_buf);

		err = at_parser_string_get(&parser, AT_CEREG_CELL_ID_INDEX, str_buf, &len);
		if (err) {
			LOG_DBG("Could not get cell ID, error: %d", err);
		} else {
			cell->id = strtoul(str_buf, NULL, 16);
		}
	}

	/* Get currently active LTE mode. */
	err = at_parser_num_get(&parser, AT_CEREG_ACT_INDEX, &temp);
	if (err) {
		LOG_DBG("LTE mode not found, error code: %d", err);

		/* This is not an error that should be returned, as it's
		 * expected in some situations that LTE mode is not available.
		 */
		err = 0;
	} else {
		*lte_mode = temp;
		LOG_DBG("LTE mode: %d", *lte_mode);
	}

#if defined(CONFIG_LOG)
	int cause_type;
	int reject_cause;

	/* Log reject cause if present. */
	err = at_parser_num_get(&parser, AT_CEREG_CAUSE_TYPE_INDEX, &cause_type);
	err |= at_parser_num_get(&parser, AT_CEREG_REJECT_CAUSE_INDEX, &reject_cause);
	if (!err && cause_type == 0 /* EMM cause */) {
		LOG_WRN("Registration rejected, EMM cause: %d, Cell ID: %d, Tracking area: %d, "
			"LTE mode: %d",
			reject_cause, cell->id, cell->tac, *lte_mode);
	}
	/* Absence of reject cause is not considered an error. */
	err = 0;
#endif /* CONFIG_LOG */

#if defined(CONFIG_LTE_LC_PSM_MODULE)
	/* Check PSM parameters only if we are connected */
	if ((*reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
	    (*reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
		goto clean_exit;
	}

	char active_time_str[9] = {0};
	char tau_ext_str[9] = {0};
	int str_len;
	int err_active_time;
	int err_tau;

	str_len = sizeof(active_time_str);

	/* Get active time */
	err_active_time = at_parser_string_get(&parser, AT_CEREG_ACTIVE_TIME_INDEX, active_time_str,
					       &str_len);
	if (err_active_time) {
		LOG_DBG("Active time not found, error: %d", err_active_time);
	} else {
		LOG_DBG("Active time: %s", active_time_str);
	}

	str_len = sizeof(tau_ext_str);

	/* Get Periodic-TAU-ext */
	err_tau = at_parser_string_get(&parser, AT_CEREG_TAU_INDEX, tau_ext_str, &str_len);
	if (err_tau) {
		LOG_DBG("TAU not found, error: %d", err_tau);
	} else {
		LOG_DBG("TAU: %s", tau_ext_str);
	}

	if (err_active_time == 0 && err_tau == 0) {
		/* Legacy TAU is not requested because we do not get it from CEREG.
		 * If extended TAU is not set, TAU will be set to inactive so
		 * caller can then make its conclusions.
		 */
		err = psm_parse(active_time_str, tau_ext_str, NULL, psm_cfg);
		if (err) {
			LOG_ERR("Failed to parse PSM configuration, error: %d", err);
		}
	}
	/* The notification does not always contain PSM parameters,
	 * so this is not considered an error
	 */
	err = 0;
#endif

clean_exit:
	return err;
}

static void at_handler_cereg(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};

	__ASSERT_NO_MSG(response != NULL);

	static enum lte_lc_nw_reg_status prev_reg_status = LTE_LC_NW_REG_NOT_REGISTERED;
	static struct lte_lc_cell prev_cell;
	enum lte_lc_nw_reg_status reg_status;
	struct lte_lc_cell cell;
	enum lte_lc_lte_mode lte_mode;
	struct lte_lc_psm_cfg psm_cfg;

	LOG_DBG("+CEREG notification: %.*s", strlen(response) - strlen("\r\n"), response);

	err = parse_cereg(response, &reg_status, &cell, &lte_mode, &psm_cfg);
	if (err) {
		LOG_ERR("Failed to parse notification (error %d): %s", err, response);
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
	default:
		LOG_ERR("Unknown network registration status: %d", reg_status);
		return;
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
		switch (lte_mode) {
		case LTE_LC_LTE_MODE_LTEM:
			LTE_LC_TRACE(LTE_LC_TRACE_LTE_MODE_UPDATE_LTEM);
			break;
		case LTE_LC_LTE_MODE_NBIOT:
			LTE_LC_TRACE(LTE_LC_TRACE_LTE_MODE_UPDATE_NBIOT);
			break;
		case LTE_LC_LTE_MODE_NTN_NBIOT:
			LTE_LC_TRACE(LTE_LC_TRACE_LTE_MODE_UPDATE_NTN_NBIOT);
			break;
		case LTE_LC_LTE_MODE_NONE:
			LTE_LC_TRACE(LTE_LC_TRACE_LTE_MODE_UPDATE_NONE);
			break;
		default:
			LOG_ERR("Unknown LTE mode: %d", lte_mode);
			return;
		}

		prev_lte_mode = lte_mode;
		evt.type = LTE_LC_EVT_LTE_MODE_UPDATE;
		evt.lte_mode = lte_mode;

		event_handler_list_dispatch(&evt);
	}

	if ((reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
	    (reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
#if defined(CONFIG_LTE_LC_PSM_MODULE)
		/* Clear PSM configuration if the device is not registered */
		if (reg_status == LTE_LC_NW_REG_NOT_REGISTERED) {
			psm_cfg.tau = -1;
			psm_cfg.active_time = -1;
			psm_evt_update_send(&psm_cfg);
		}
#endif
		return;
	}

#if defined(CONFIG_LTE_LC_PSM_MODULE)
	if (psm_cfg.tau == -1) {
		/* Need to get legacy T3412 value as TAU using AT%XMONITOR.
		 *
		 * As we are in an AT notification handler that is run from the system work queue,
		 * we shall not send AT commands here because another AT command might be ongoing,
		 * and the second command will be blocked until the first one completes.
		 * Further AT notifications from the modem will gradually exhaust AT monitor
		 * library's heap, and eventually it will run out causing an assert or
		 * AT notifications not being dispatched.
		 */
		k_work_submit_to_queue(work_q_get(), psm_work_get());
		return;
	}

	psm_evt_update_send(&psm_cfg);
#endif
}

int cereg_status_get(enum lte_lc_nw_reg_status *status)
{
	int err;
	uint16_t status_tmp;
	uint32_t cell_id = 0;

	if (status == NULL) {
		return -EINVAL;
	}

	/* Read network registration status */
	err = nrf_modem_at_scanf(AT_CEREG_READ,
				 "+CEREG: "
				 "%*u,"	    /* <n> */
				 "%hu,"	    /* <stat> */
				 "%*[^,],"  /* <tac> */
				 "\"%x\",", /* <ci> */
				 &status_tmp, &cell_id);
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

int cereg_mode_get(enum lte_lc_lte_mode *mode)
{
	int err;
	uint16_t mode_tmp;

	if (mode == NULL) {
		return -EINVAL;
	}

	err = nrf_modem_at_scanf(AT_CEREG_READ,
				 "+CEREG: "
				 "%*u,"	   /* <n> */
				 "%*u,"	   /* <stat> */
				 "%*[^,]," /* <tac> */
				 "%*[^,]," /* <ci> */
				 "%hu",	   /* <AcT> */
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

int cereg_lte_connect(bool blocking)
{
	int err;
	enum lte_lc_func_mode original_func_mode;
	bool func_mode_changed = false;
	enum lte_lc_nw_reg_status reg_status;
	static atomic_t in_progress;

	/* Check if a connection attempt is already in progress */
	if (atomic_set(&in_progress, 1)) {
		LOG_WRN("Connect already in progress");
		return -EINPROGRESS;
	}

	err = cereg_status_get(&reg_status);
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

	err = cfun_mode_get(&original_func_mode);
	if (err) {
		err = -EFAULT;
		goto exit;
	}

	/* Reset the semaphore, it may have already been given by an earlier +CEREG notification. */
	k_sem_reset(&link);

	err = cfun_mode_set(LTE_LC_FUNC_MODE_NORMAL);
	if (err || !blocking) {
		goto exit;
	}

	func_mode_changed = true;

	err = k_sem_take(&link, K_SECONDS(CONFIG_LTE_NETWORK_TIMEOUT));
	if (err == -EAGAIN) {
		LOG_INF("Network connection attempt timed out");
		err = -ETIMEDOUT;
	}

exit:
	if (err && func_mode_changed) {
		/* Connecting to LTE network failed, restore original functional mode. */
		cfun_mode_set(original_func_mode);
	}

	atomic_clear(&in_progress);

	return err;
}

int cereg_notifications_enable(void)
{
	int err;

	/* +CEREG notifications, level 5 */
	err = nrf_modem_at_printf(AT_CEREG_5);
	if (err) {
		LOG_ERR("Failed to subscribe to CEREG notifications, error: %d", err);
		return -EFAULT;
	}

	return 0;
}
