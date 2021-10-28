/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <errno.h>
#include <net/socket.h>
#include <string.h>
#include <stdio.h>
#include <device.h>
#include <nrf_errno.h>
#include <nrf_modem_at.h>
#include <modem/lte_lc.h>
#include <modem/lte_lc_trace.h>
#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>
#include <modem/at_monitor.h>
#include <logging/log.h>

#include "lte_lc_helpers.h"

LOG_MODULE_REGISTER(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

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
	LTE_LC_SYSTEM_MODE_NONE)

enum lte_lc_notif_type {
	LTE_LC_NOTIF_CEREG,
	LTE_LC_NOTIF_CSCON,
	LTE_LC_NOTIF_CEDRXP,
	LTE_LC_NOTIF_XT3412,
	LTE_LC_NOTIF_NCELLMEAS,
	LTE_LC_NOTIF_XMODEMSLEEP,
	LTE_LC_NOTIF_MDMEV,

	LTE_LC_NOTIF_COUNT,
};

/* Static variables */

static bool is_initialized;

#if defined(CONFIG_NRF_MODEM_LIB_TRACE_ENABLED)
/* Enable modem trace */
static const char mdm_trace[] = "AT%%XMODEMTRACE=1,2";
#endif

#if defined(CONFIG_LTE_LOCK_BANDS)
/* Lock LTE bands 3, 4, 13 and 20 (volatile setting) */
static const char lock_bands[] = "AT%%XBANDLOCK=2,\""CONFIG_LTE_LOCK_BAND_MASK
				 "\"";
#endif
#if defined(CONFIG_LTE_LOCK_PLMN)
/* Lock PLMN */
static const char lock_plmn[] = "AT+COPS=1,2,\""
				 CONFIG_LTE_LOCK_PLMN_STRING"\"";
#elif defined(CONFIG_LTE_UNLOCK_PLMN)
/* Unlock PLMN */
static const char unlock_plmn[] = "AT+COPS=0";
#endif
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
/* Default PSM RPATU setting */
static char psm_param_rptau[9] = CONFIG_LTE_PSM_REQ_RPTAU;
/* Request PSM to be disabled */
static const char psm_disable[] = "AT+CPSMS=";
/* Enable CSCON (RRC mode) notifications */
static const char cscon[] = "AT+CSCON=1";
/* Disable RAI */
static const char rai_disable[] = "AT%%XRAI=0";
/* Default RAI setting */
static char rai_param[2] = CONFIG_LTE_RAI_REQ_VALUE;

static const enum lte_lc_system_mode sys_mode_preferred = SYS_MODE_PREFERRED;

/* System mode to use when connecting to LTE network, which can be changed in
 * two ways:
 *	- Automatically to fallback mode (if enabled) when connection to the
 *	  preferred mode is unsuccessful and times out.
 *	- By calling lte_lc_system_mode_set() and set the mode explicitly.
 */
static enum lte_lc_system_mode sys_mode_target = SYS_MODE_PREFERRED;

/* System mode preference to set when configuring system mode. */
static enum lte_lc_system_mode_preference mode_pref_target = CONFIG_LTE_MODE_PREFERENCE;
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
	LTE_LC_SYSTEM_MODE_NONE;

static enum lte_lc_system_mode sys_mode_current = LTE_LC_SYSTEM_MODE_NONE;

/* Parameters to be passed using AT%XSYSTEMMMODE=<params>,<preference> */
static const char *const system_mode_params[] = {
	[LTE_LC_SYSTEM_MODE_NONE]		= "0,0,0",
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

#if !defined(CONFIG_NRF_MODEM_LIB_SYS_INIT) && \
	defined(CONFIG_BOARD_THINGY91_NRF9160_NS)
static const char thingy91_magpio[] = {
	"AT%%XMAGPIO=1,1,1,7,1,746,803,2,698,748,"
	"2,1710,2200,3,824,894,4,880,960,5,791,849,"
	"7,1565,1586"
};
#endif /* !CONFIG_NRF_MODEM_LIB_SYS_INIT && CONFIG_BOARD_THINGY91_NRF9160_NS */

static struct k_sem link;

static const char *const at_notifs[] = {
	[LTE_LC_NOTIF_CEREG]		= "+CEREG",
	[LTE_LC_NOTIF_CSCON]		= "+CSCON",
	[LTE_LC_NOTIF_CEDRXP]		= "+CEDRXP",
	[LTE_LC_NOTIF_XT3412]		= "%XT3412",
	[LTE_LC_NOTIF_NCELLMEAS]	= "%NCELLMEAS",
	[LTE_LC_NOTIF_XMODEMSLEEP]	= "%XMODEMSLEEP",
	[LTE_LC_NOTIF_MDMEV]		= "%MDMEV",
};

BUILD_ASSERT(ARRAY_SIZE(at_notifs) == LTE_LC_NOTIF_COUNT);

static bool is_relevant_notif(const char *notif, enum lte_lc_notif_type *type)
{
	for (size_t i = 0; i < ARRAY_SIZE(at_notifs); i++) {
		if (strncmp(at_notifs[i], notif,
			    strlen(at_notifs[i])) == 0) {
			/* The notification type matches the array index */
			*type = i;

			return true;
		}
	}

	return false;
}

static bool is_cellid_valid(uint32_t cellid)
{
	if (cellid == LTE_LC_CELL_EUTRAN_ID_INVALID) {
		return false;
	}

	return true;
}

AT_MONITOR(network, ANY, at_handler);

static void at_handler(const char *response)
{
	int err;
	bool notify = false;
	enum lte_lc_notif_type notif_type;
	struct lte_lc_evt evt = {0};

	if (response == NULL) {
		LOG_ERR("Response buffer is NULL-pointer");
		return;
	}

	/* Only proceed with parsing if notification is relevant */
	if (!is_relevant_notif(response, &notif_type)) {
		return;
	}

	switch (notif_type) {
	case LTE_LC_NOTIF_CEREG: {
		static enum lte_lc_nw_reg_status prev_reg_status =
			LTE_LC_NW_REG_NOT_REGISTERED;
		static struct lte_lc_cell prev_cell;
		static struct lte_lc_psm_cfg prev_psm_cfg;
		static enum lte_lc_lte_mode prev_lte_mode = LTE_LC_LTE_MODE_NONE;
		enum lte_lc_nw_reg_status reg_status = 0;
		struct lte_lc_cell cell = {0};
		enum lte_lc_lte_mode lte_mode;
		struct lte_lc_psm_cfg psm_cfg = {0};

		LOG_DBG("+CEREG notification: %s", log_strdup(response));

		err = parse_cereg(response, true, &reg_status, &cell, &lte_mode);
		if (err) {
			LOG_ERR("Failed to parse notification (error %d): %s",
				err, log_strdup(response));
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
		case LTE_LC_NW_REG_REGISTERED_EMERGENCY:
			LTE_LC_TRACE(LTE_LC_TRACE_NW_REG_REGISTERED_EMERGENCY);
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
		if (memcmp(&cell, &prev_cell, sizeof(struct lte_lc_cell))) {
			evt.type = LTE_LC_EVT_CELL_UPDATE;

			memcpy(&prev_cell, &cell, sizeof(struct lte_lc_cell));
			memcpy(&evt.cell, &cell, sizeof(struct lte_lc_cell));
			event_handler_list_dispatch(&evt);
		}

		if (lte_mode != prev_lte_mode) {
			prev_lte_mode = lte_mode;
			evt.type = LTE_LC_EVT_LTE_MODE_UPDATE;
			evt.lte_mode = lte_mode;

			LTE_LC_TRACE(lte_mode == LTE_LC_LTE_MODE_LTEM ?
				     LTE_LC_TRACE_LTE_MODE_UPDATE_LTEM :
				     LTE_LC_TRACE_LTE_MODE_UPDATE_NBIOT);

			event_handler_list_dispatch(&evt);
		}

		if ((reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		    (reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			return;
		}

		err = lte_lc_psm_get(&psm_cfg.tau, &psm_cfg.active_time);
		if (err) {
			LOG_ERR("Failed to get PSM information");
			return;
		}

		/* PSM configuration update event */
		if (memcmp(&psm_cfg, &prev_psm_cfg,
			   sizeof(struct lte_lc_psm_cfg))) {
			evt.type = LTE_LC_EVT_PSM_UPDATE;

			memcpy(&prev_psm_cfg, &psm_cfg,
			       sizeof(struct lte_lc_psm_cfg));
			memcpy(&evt.psm_cfg, &psm_cfg,
			       sizeof(struct lte_lc_psm_cfg));
			event_handler_list_dispatch(&evt);
		}

		break;
	}
	case LTE_LC_NOTIF_CSCON:
		LOG_DBG("+CSCON notification");

		err = parse_rrc_mode(response,
				     &evt.rrc_mode,
				     AT_CSCON_RRC_MODE_INDEX);
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
		notify = true;

		break;
	case LTE_LC_NOTIF_CEDRXP:
		LOG_DBG("+CEDRXP notification");

		err = parse_edrx(response, &evt.edrx_cfg);
		if (err) {
			LOG_ERR("Can't parse eDRX, error: %d", err);
			return;
		}

		evt.type = LTE_LC_EVT_EDRX_UPDATE;
		notify = true;

		break;
	case LTE_LC_NOTIF_XT3412:
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
		notify = true;

		break;
	case LTE_LC_NOTIF_NCELLMEAS: {
		int ncell_count = neighborcell_count_get(response);
		struct lte_lc_ncell *neighbor_cells = NULL;

		LOG_DBG("%%NCELLMEAS notification");
		LOG_DBG("Neighbor cell count: %d", ncell_count);

		if (event_handler_list_is_empty()) {
			/* No need to parse the response if there is no handler
			 * to receive the parsed data.
			 */
			return;
		}

		if (ncell_count != 0) {
			neighbor_cells = k_calloc(ncell_count, sizeof(struct lte_lc_ncell));
			if (neighbor_cells == NULL) {
				LOG_ERR("Failed to allocate memory for neighbor cells");
				return;
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

		return;
	}
	case LTE_LC_NOTIF_XMODEMSLEEP:
		LOG_DBG("%%XMODEMSLEEP notification");

		err = parse_xmodemsleep(response, &evt.modem_sleep);
		if (err) {
			LOG_ERR("Can't parse modem sleep pre-warning notification, error: %d", err);
			return;
		}

		/* Link controller only supports PSM, RF inactivity and flight mode
		 * modem sleep types.
		 */
		if ((evt.modem_sleep.type != LTE_LC_MODEM_SLEEP_PSM) &&
		    (evt.modem_sleep.type != LTE_LC_MODEM_SLEEP_RF_INACTIVITY) &&
		    (evt.modem_sleep.type != LTE_LC_MODEM_SLEEP_FLIGHT_MODE)) {
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

		notify = true;

		break;
	case LTE_LC_NOTIF_MDMEV:
		LOG_DBG("%%MDMEV notification");

		err = parse_mdmev(response, &evt.modem_evt);
		if (err) {
			LOG_ERR("Can't parse modem event notification, error: %d", err);
			return;
		}

		evt.type = LTE_LC_EVT_MODEM_EVENT;
		notify = true;

		break;
	default:
		LOG_ERR("Unrecognized notification type: %d", notif_type);
		break;
	}

	if (!event_handler_list_is_empty() && notify) {
		event_handler_list_dispatch(&evt);
	}
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
		char buf[50];

		/* AT+CSCON is supported from modem firmware v1.1.0, and will
		 * not work for older versions. If the command fails, RRC
		 * mode change notifications will not be received. This is not
		 * considered a critical error, and the error code is therefore
		 * not returned, while informative log messageas are printed.
		 */
		LOG_WRN("AT+CSCON failed (%d), RRC notifications are not enabled", err);
		LOG_WRN("AT+CSCON is supported in nRF9160 modem >= v1.1.0");

		err = nrf_modem_at_cmd(buf, sizeof(buf), "AT+CGMR");
		if (err == 0) {
			char *end = strstr(buf, "\r\nOK");

			if (end) {
				*end = '\0';
			}

			LOG_WRN("Current modem firmware version: %s", log_strdup(buf));
		}
	}

	return 0;
}

static int init_and_config(void)
{
	int err;

	if (!IS_ENABLED(CONFIG_AT_MONITOR_SYS_INIT)) {
		at_monitor_init();
	}

	if (is_initialized) {
		return -EALREADY;
	}

	k_sem_init(&link, 0, 1);

	err = lte_lc_system_mode_get(&sys_mode_current, &mode_pref_current);
	if (err) {
		LOG_ERR("Could not get current system mode, error: %d", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_LTE_NETWORK_DEFAULT)) {
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

#if !defined(CONFIG_NRF_MODEM_LIB_SYS_INIT) && \
	defined(CONFIG_BOARD_THINGY91_NRF9160_NS)
	/* Configuring MAGPIO, so that the correct antenna
	 * matching network is used for each LTE band and GPS.
	 */
	if (nrf_modem_at_printf(thingy91_magpio)) {
		return -EIO;
	}
#endif

#if defined(CONFIG_LTE_EDRX_REQ)
	/* Request configured eDRX settings to save power */
	if (lte_lc_edrx_req(true) != 0) {
		return -EIO;
	}
#endif
#if defined(CONFIG_NRF_MODEM_LIB_TRACE_ENABLED)
	if (nrf_modem_at_printf(mdm_trace)) {
		return -EIO;
	}
#endif
#if defined(CONFIG_LTE_LOCK_BANDS)
	/* Set LTE band lock (volatile setting).
	 * Has to be done every time before activating the modem.
	 */
	if (nrf_modem_at_printf(lock_bands)) {
		return -EIO;
	}
#endif
#if defined(CONFIG_LTE_LOCK_PLMN)
	/* Manually select Operator (volatile setting).
	 * Has to be done every time before activating the modem.
	 */
	if (nrf_modem_at_printf(lock_plmn)) {
		return -EIO;
	}
#elif defined(CONFIG_LTE_UNLOCK_PLMN)
	/* Automatically select Operator (volatile setting).
	 */
	if (nrf_modem_at_printf(unlock_plmn)) {
		return -EIO;
	}
#endif

	/* Listen for RRC connection mode notifications */
	err = enable_notifications();
	if (err) {
		LOG_ERR("Failed to enable notifications");
		return err;
	}

	is_initialized = true;

	return 0;
}

static int connect_lte(bool blocking)
{
	int err;
	int tries = (IS_ENABLED(CONFIG_LTE_NETWORK_USE_FALLBACK) ? 2 : 1);
	enum lte_lc_func_mode current_func_mode;

	if (!is_initialized) {
		LOG_ERR("The LTE link controller is not initialized");
		return -EPERM;
	}

	k_sem_init(&link, 0, 1);

	do {
		tries--;

		err = lte_lc_func_mode_get(&current_func_mode);
		if (err) {
			return -EFAULT;
		}

		/* Change the modem sys-mode only if it's not running or is meant to change */
		if (!IS_ENABLED(CONFIG_LTE_NETWORK_DEFAULT) &&
		    ((current_func_mode == LTE_LC_FUNC_MODE_POWER_OFF) ||
		     (current_func_mode == LTE_LC_FUNC_MODE_OFFLINE))) {
			err = lte_lc_system_mode_set(sys_mode_target, mode_pref_current);
			if (err) {
				return -EFAULT;
			}
		}

		err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_NORMAL);
		if (err || !blocking) {
			return err;
		}

		err = k_sem_take(&link, K_SECONDS(CONFIG_LTE_NETWORK_TIMEOUT));
		if (err == -EAGAIN) {
			LOG_INF("Network connection attempt timed out");

			if (IS_ENABLED(CONFIG_LTE_NETWORK_USE_FALLBACK) &&
			    (tries > 0)) {
				if (sys_mode_target == sys_mode_preferred) {
					sys_mode_target = sys_mode_fallback;
				} else {
					sys_mode_target = sys_mode_preferred;
				}

				err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE);
				if (err) {
					return -EFAULT;
				}

				LOG_INF("Using fallback network mode");
			} else {
				err = -ETIMEDOUT;
			}
		} else {
			tries = 0;
		}
	} while (tries > 0);

	return err;
}

static int init_and_connect(const struct device *unused)
{
	int err;

	err = lte_lc_init();
	if (err) {
		return err;
	}

	return connect_lte(true);
}

/* Public API */

int lte_lc_init(void)
{
	int err = init_and_config();

	if ((err == 0) || (err == -EALREADY)) {
		return err;
	}

	return -EFAULT;
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
	const struct device *x = 0;

	return init_and_connect(x);
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
		return err;
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
		LOG_DBG("RPTAU set to %s", log_strdup(psm_param_rptau));
	} else {
		*psm_param_rptau = '\0';
		LOG_DBG("RPTAU use default");
	}

	if (rat != NULL) {
		strcpy(psm_param_rat, rat);
		LOG_DBG("RAT set to %s", log_strdup(psm_param_rat));
	} else {
		*psm_param_rat = '\0';
		LOG_DBG("RAT use default");
	}

	return 0;
}

int lte_lc_psm_req(bool enable)
{
	int err;

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

	if ((tau == NULL) || (active_time == NULL)) {
		return -EINVAL;
	}

	/* We expect match on at least two parameters, <Active-Time> and <Periodic-TAU-ext>.
	 * Three matches will only happen on modem firmwares >= 1.2.0.
	 */
	err = nrf_modem_at_scanf("AT%XMONITOR",
		"%%XMONITOR: "
		"%*u,"		/* <reg_status> */
		"%*[^,],"	/* <full_name> */
		"%*[^,],"	/* <short_name> */
		"%*[^,],"	/* <plmn> */
		"%*[^,],"	/* <tac> */
		"%*u,"		/* <AcT> */
		"%*u,"		/* <band> */
		"%*[^,],"	/* <cell_id> */
		"%*u,"		/* <phys_cell_id> */
		"%*u,"		/* <EARFCN> */
		"%*u,"		/* <rsrp> */
		"%*u,"		/* <snr> */
		"%*[^,],"	/* <NW-provided_eDRX_value> */
		"\"%8[0-1]\","	/* <Active-Time> */
		"\"%8[0-1]\","	/* <Periodic-TAU-ext> */
		"\"%8[0-1]\"",	/* <Periodic-TAU>, available only for modem fw >= v1.2.0 */
		active_time_str, tau_ext_str, tau_legacy_str);
	if (err < 0) {
		LOG_ERR("AT command failed, error: %d", err);
		return -EFAULT;
	} else if (err < 2) {
		LOG_ERR("Active time and/or TAU interval is missing");
		return -EBADMSG;
	}

	err = parse_psm(active_time_str, tau_ext_str, tau_legacy_str, &psm_cfg);
	if (err) {
		LOG_ERR("Failed to parse PSM configuration, error: %d", err);
		return err;
	}

	*tau = psm_cfg.tau;
	*active_time = psm_cfg.active_time;

	LOG_DBG("TAU: %d sec, active time: %d sec\n", *tau, *active_time);

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
		LOG_DBG("eDRX set to %s for %s", log_strdup(edrx_param),
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
		LOG_DBG("PTW set to %s for %s", log_strdup(ptw_param),
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

	if (status == NULL) {
		return -EINVAL;
	}

	/* Read network registration status */
	err = nrf_modem_at_scanf("AT%XMONITOR", "%%XMONITOR: %u", status);
	if (err != 1) {
		LOG_ERR("Could not get registration status, error: %d", err);
		return -EFAULT;
	}

	return 0;
}

int lte_lc_system_mode_set(enum lte_lc_system_mode mode,
			   enum lte_lc_system_mode_preference preference)
{
	int err;

	switch (mode) {
	case LTE_LC_SYSTEM_MODE_NONE:
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
	int err, mode_bitmask = 0;
	int ltem_mode, nbiot_mode, gps_mode, mode_preference;

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
	case 0:
		*mode = LTE_LC_SYSTEM_MODE_NONE;
		break;
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

	if (sys_mode_current != *mode) {
		LOG_DBG("Current system mode updated from %d to %d",
			sys_mode_current, *mode);
		sys_mode_current = *mode;
	}

	if ((preference != NULL) && (mode_pref_current != *preference)) {
		LOG_DBG("Current system mode preference updated from %d to %d",
			mode_pref_current, *preference);
		mode_pref_current = *preference;
	}

	return 0;
}

int lte_lc_func_mode_get(enum lte_lc_func_mode *mode)
{
	int err;

	if (mode == NULL) {
		return -EINVAL;
	}

	/* Exactly one parameter is expected to match. */
	err = nrf_modem_at_scanf(AT_CFUN_READ, "+CFUN: %u", mode);
	if (err != 1) {
		LOG_ERR("AT command failed, nrf_modem_at_scanf() returned error: %d", err);
		return -EFAULT;
	}

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

	return nrf_modem_at_printf("AT+CFUN=%d", mode) ? -EFAULT : 0;
}

int lte_lc_lte_mode_get(enum lte_lc_lte_mode *mode)
{
	int err;

	if (mode == NULL) {
		return -EINVAL;
	}

	err = nrf_modem_at_scanf(AT_CEREG_READ,
		"+CEREG: "
		"%*u,"		/* <n> */
		"%*u,"		/* <stat> */
		"%*[^,],"	/* <tac> */
		"%*[^,],"	/* <ci> */
		"%u",		/* <AcT> */
		mode);
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

int lte_lc_neighbor_cell_measurement(enum lte_lc_neighbor_search_type type)
{
	int err;

	/* Starting from modem firmware v1.3.1, there is an optional parameter to specify
	 * the type of search.
	 * If the type is LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT, we therefore use the AT
	 * command without parameters to avoid error messages for older firmware version.
	 */

	if (type == LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_LIGHT) {
		err = at_cmd_write("AT%NCELLMEAS=1",  NULL, 0, NULL);
	} else if (type == LTE_LC_NEIGHBOR_SEARCH_TYPE_EXTENDED_COMPLETE) {
		err = at_cmd_write("AT%NCELLMEAS=2",  NULL, 0, NULL);
	} else {
		/* Defaulting to use LTE_LC_NEIGHBOR_SEARCH_TYPE_DEFAULT */
		err = at_cmd_write("AT%NCELLMEAS",  NULL, 0, NULL);
	}

	return err ? -EFAULT : 0;
}

int lte_lc_neighbor_cell_measurement_cancel(void)
{
	return nrf_modem_at_printf(AT_NCELLMEAS_STOP) ? -EFAULT : 0;
}

int lte_lc_conn_eval_params_get(struct lte_lc_conn_eval_params *params)
{
	int err;
	enum lte_lc_func_mode mode;
	int result;
	/* PLMN field is a string of maximum 6 characters, plus null termination. */
	char plmn_str[7] = {0};

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
		"%u,"		/* <result> */
		"%u,"		/* <rrc_state> */
		"%u,"		/* <energy_estimate> */
		"%d,"		/* <rsrp> */
		"%d,"		/* <rsrq> */
		"%u,"		/* <snr> */
		"\"%x\","	/* <cell_id> */
		"\"%6[^\"]\","	/* <plmn> */
		"%u,"		/* <phys_cell_id> */
		"%u,"		/* <earfcn> */
		"%u,"		/* <band> */
		"%u,"		/* <tau_triggered> */
		"%u,"		/* <ce_level> */
		"%d,"		/* <tx_power> */
		"%u,"		/* <tx_repetitions> */
		"%u,"		/* <rs_repetitions> */
		"%d",		/* <dl-pathloss> */
		&result, &params->rrc_state, &params->energy_estimate,
		&params->rsrp, &params->rsrq, &params->snr, &params->cell_id,
		plmn_str, &params->phy_cid, &params->earfcn, &params->band,
		&params->tau_trig, &params->ce_level, &params->tx_power,
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
	return nrf_modem_at_printf(AT_MDMEV_ENABLE) ? -EFAULT : 0;
}

int lte_lc_modem_events_disable(void)
{
	return nrf_modem_at_printf(AT_MDMEV_DISABLE) ? -EFAULT : 0;
}

#if defined(CONFIG_LTE_AUTO_INIT_AND_CONNECT)
SYS_DEVICE_DEFINE("LTE_LINK_CONTROL", init_and_connect,
		  NULL,
		  APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif /* CONFIG_LTE_AUTO_INIT_AND_CONNECT */
