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
#include <modem/lte_lc.h>
#include <modem/at_cmd.h>
#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>
#include <modem/at_notif.h>
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

	LTE_LC_NOTIF_COUNT,
};

/* Static variables */

static lte_lc_evt_handler_t evt_handler;
static bool is_initialized;

#if defined(CONFIG_NRF_MODEM_LIB_TRACE_ENABLED)
/* Enable modem trace */
static const char mdm_trace[] = "AT%XMODEMTRACE=1,2";
#endif

/* Subscribes to notifications with level 5 */
static const char cereg_5_subscribe[] = AT_CEREG_5;

#if defined(CONFIG_LTE_LOCK_BANDS)
/* Lock LTE bands 3, 4, 13 and 20 (volatile setting) */
static const char lock_bands[] = "AT%XBANDLOCK=2,\""CONFIG_LTE_LOCK_BAND_MASK
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
	defined(CONFIG_BOARD_THINGY91_NRF9160NS)
static const char thingy91_magpio[] = {
	"AT%XMAGPIO=1,1,1,7,1,746,803,2,698,748,"
	"2,1710,2200,3,824,894,4,880,960,5,791,849,"
	"7,1565,1586"
};
#endif /* !CONFIG_NRF_MODEM_LIB_SYS_INIT && CONFIG_BOARD_THINGY91_NRF9160NS */

static struct k_sem link;

#if defined(CONFIG_LTE_PDP_CMD)
static char cgdcont[144] = "AT+CGDCONT="CONFIG_LTE_PDP_CONTEXT;
#endif
#if defined(CONFIG_LTE_PDN_AUTH_CMD)
static char cgauth[19 + CONFIG_LTE_PDN_AUTH_LEN] =
				"AT+CGAUTH="CONFIG_LTE_PDN_AUTH;
#endif
#if defined(CONFIG_LTE_LEGACY_PCO_MODE)
static const char legacy_pco[] = "AT%XEPCO=0";
#endif

static const char *const at_notifs[] = {
	[LTE_LC_NOTIF_CEREG] = "+CEREG",
	[LTE_LC_NOTIF_CSCON] = "+CSCON",
	[LTE_LC_NOTIF_CEDRXP] = "+CEDRXP",
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

static void at_handler(void *context, const char *response)
{
	ARG_UNUSED(context);

	int err;
	bool notify = false;
	enum lte_lc_notif_type notif_type;
	struct lte_lc_evt evt;

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
		struct lte_lc_cell cell;
		enum lte_lc_lte_mode lte_mode;
		struct lte_lc_psm_cfg psm_cfg;

		LOG_DBG("+CEREG notification: %s", log_strdup(response));

		err = parse_cereg(response, true, &reg_status, &cell, &lte_mode, &psm_cfg);
		if (err) {
			LOG_ERR("Failed to parse notification (error %d): %s",
				err, log_strdup(response));
			return;
		}

		if ((reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
		    (reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			k_sem_give(&link);
		}

		if (!evt_handler) {
			return;
		}

		/* Network registration status event */
		if (reg_status != prev_reg_status) {
			prev_reg_status = reg_status;
			evt.type = LTE_LC_EVT_NW_REG_STATUS;
			evt.nw_reg_status = reg_status;

			evt_handler(&evt);
		}

		/* Cell update event */
		if (memcmp(&cell, &prev_cell, sizeof(struct lte_lc_cell))) {
			evt.type = LTE_LC_EVT_CELL_UPDATE;

			memcpy(&prev_cell, &cell, sizeof(struct lte_lc_cell));
			memcpy(&evt.cell, &cell, sizeof(struct lte_lc_cell));
			evt_handler(&evt);
		}

		if (lte_mode != prev_lte_mode) {
			prev_lte_mode = lte_mode;
			evt.type = LTE_LC_EVT_LTE_MODE_UPDATE;
			evt.lte_mode = lte_mode;

			evt_handler(&evt);
		}

		if ((reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		    (reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
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
			evt_handler(&evt);
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
	default:
		LOG_ERR("Unrecognized notification type: %d", notif_type);
		break;
	}

	if (evt_handler && notify) {
		evt_handler(&evt);
	}
}

static int enable_notifications(void)
{
	int err;

	/* +CEREG notifications, level 5 */
	err = at_cmd_write(cereg_5_subscribe, NULL, 0, NULL);
	if (err) {
		LOG_ERR("Failed to subscribe to CEREG notifications");
		return err;
	}

	/* +CSCON notifications */
	err = at_cmd_write(cscon, NULL, 0, NULL);
	if (err) {
		char buf[50];

		/* AT+CSCON is supported from modem firmware v1.1.0, and will
		 * not work for older versions. If the command fails, RRC
		 * mode change notifications will not be received. This is not
		 * considered a critical error, and the error code is therefore
		 * not returned, while informative log messageas are printed.
		 */
		LOG_WRN("%s failed (%d), RRC notifications are not enabled",
			cscon, err);
		LOG_WRN("%s is supported in nRF9160 modem >= v1.1.0", cscon);

		err = at_cmd_write("AT+CGMR", buf, sizeof(buf), NULL);
		if (err == 0) {
			LOG_WRN("Current modem firmware version: %s",
				log_strdup(buf));
		}
	}

	return 0;
}

static int init_and_config(void)
{
	int err;

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

	err = at_notif_register_handler(NULL, at_handler);
	if (err) {
		LOG_ERR("Can't register AT handler, error: %d", err);
		return err;
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
	defined(CONFIG_BOARD_THINGY91_NRF9160NS)
	/* Configuring MAGPIO, so that the correct antenna
	 * matching network is used for each LTE band and GPS.
	 */
	if (at_cmd_write(thingy91_magpio, NULL, 0, NULL) != 0) {
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
	if (at_cmd_write(mdm_trace, NULL, 0, NULL) != 0) {
		return -EIO;
	}
#endif
#if defined(CONFIG_LTE_LOCK_BANDS)
	/* Set LTE band lock (volatile setting).
	 * Has to be done every time before activating the modem.
	 */
	if (at_cmd_write(lock_bands, NULL, 0, NULL) != 0) {
		return -EIO;
	}
#endif
#if defined(CONFIG_LTE_LOCK_PLMN)
	/* Manually select Operator (volatile setting).
	 * Has to be done every time before activating the modem.
	 */
	if (at_cmd_write(lock_plmn, NULL, 0, NULL) != 0) {
		return -EIO;
	}
#elif defined(CONFIG_LTE_UNLOCK_PLMN)
	/* Automatically select Operator (volatile setting).
	 */
	if (at_cmd_write(unlock_plmn, NULL, 0, NULL) != 0) {
		return -EIO;
	}
#endif
#if defined(CONFIG_LTE_LEGACY_PCO_MODE)
	if (at_cmd_write(legacy_pco, NULL, 0, NULL) != 0) {
		return -EIO;
	}
	LOG_INF("Using legacy LTE PCO mode...");
#endif
#if defined(CONFIG_LTE_PDP_CMD)
	if (at_cmd_write(cgdcont, NULL, 0, NULL) != 0) {
		return -EIO;
	}
	LOG_INF("PDP Context: %s", log_strdup(cgdcont));
#endif
#if defined(CONFIG_LTE_PDN_AUTH_CMD)
	if (at_cmd_write(cgauth, NULL, 0, NULL) != 0) {
		return -EIO;
	}
	LOG_INF("PDN Auth: %s", log_strdup(cgauth));
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
	bool retry;

	if (!is_initialized) {
		LOG_ERR("The LTE link controller is not initialized");
		return -EPERM;
	}

	k_sem_init(&link, 0, 1);

	do {
		retry = false;

		if (!IS_ENABLED(CONFIG_LTE_NETWORK_DEFAULT)) {
			err = lte_lc_system_mode_set(sys_mode_target, mode_pref_current);
			if (err) {
				return err;
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
			    (sys_mode_target == sys_mode_preferred)) {
				sys_mode_target = sys_mode_fallback;
				retry = true;

				err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE);
				if (err) {
					return err;
				}

				LOG_INF("Using fallback network mode");
			} else {
				err = -ETIMEDOUT;
			}
		}
	} while (retry);

	return err;
}

static int init_and_connect(const struct device *unused)
{
	int ret;

	ret = init_and_config();
	if (ret) {
		return ret;
	}

	return connect_lte(true);
}

/* Public API */

int lte_lc_init(void)
{
	return init_and_config();
}

void lte_lc_register_handler(lte_lc_evt_handler_t handler)
{
	if (handler == NULL) {
		evt_handler = NULL;

		LOG_INF("Previously registered handler (%p) deregistered",
			handler);

		return;
	}

	if (evt_handler) {
		LOG_WRN("Replacing previously registered handler (%p) with %p",
			evt_handler, handler);
	}

	evt_handler = handler;

	return;
}

int lte_lc_connect(void)
{
	return connect_lte(true);
}

int lte_lc_init_and_connect(void)
{
	const struct device *x = 0;

	int err = init_and_connect(x);

	return err;
}

int lte_lc_connect_async(lte_lc_evt_handler_t handler)
{
	if (handler) {
		evt_handler = handler;
	} else if (evt_handler == NULL) {
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
		at_notif_deregister_handler(NULL, at_handler);
		return lte_lc_func_mode_set(LTE_LC_FUNC_MODE_POWER_OFF);
	}

	return 0;
}

int lte_lc_normal(void)
{
	int err;

	err = enable_notifications();
	if (err) {
		LOG_ERR("Failed to enabled notifications, error: %d", err);
		return err;
	}

	return lte_lc_func_mode_set(LTE_LC_FUNC_MODE_NORMAL);
}

int lte_lc_offline(void)
{
	return lte_lc_func_mode_set(LTE_LC_FUNC_MODE_OFFLINE);
}

int lte_lc_power_off(void)
{
	return lte_lc_func_mode_set(LTE_LC_FUNC_MODE_POWER_OFF);
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
		char psm_req[40];

		if (strlen(psm_param_rptau) == 8 &&
		    strlen(psm_param_rat) == 8) {
			snprintf(psm_req, sizeof(psm_req),
			"AT+CPSMS=1,,,\"%s\",\"%s\"",
			psm_param_rptau, psm_param_rat);
		} else if (strlen(psm_param_rptau) == 8) {
			snprintf(psm_req, sizeof(psm_req),
				"AT+CPSMS=1,,,\"%s\"",
				psm_param_rptau);
		} else if (strlen(psm_param_rat) == 8) {
			snprintf(psm_req, sizeof(psm_req),
				"AT+CPSMS=1,,,,\"%s\"",
				psm_param_rat);
		} else {
			snprintf(psm_req, sizeof(psm_req),
				"AT+CPSMS=1");
		}
		err = at_cmd_write(psm_req, NULL, 0, NULL);
	} else {
		err = at_cmd_write(psm_disable, NULL, 0, NULL);
	}

	if (err != 0) {
		return -EIO;
	}

	return 0;
}

int lte_lc_psm_get(int *tau, int *active_time)
{
	int err;
	struct at_param_list at_resp_list = {0};
	char buf[AT_CEREG_RESPONSE_MAX_LEN] = {0};
	struct lte_lc_psm_cfg psm_cfg;

	if ((tau == NULL) || (active_time == NULL)) {
		return -EINVAL;
	}

	/* Enable network registration status with PSM information */
	err = at_cmd_write(AT_CEREG_5, NULL, 0, NULL);
	if (err) {
		LOG_ERR("Could not set CEREG, error: %d", err);
		return err;
	}

	/* Read network registration status */
	err = at_cmd_write(AT_CEREG_READ, buf, sizeof(buf), NULL);
	if (err) {
		LOG_ERR("Could not get CEREG response, error: %d", err);
		return err;
	}

	err = at_params_list_init(&at_resp_list, AT_CEREG_PARAMS_COUNT_MAX);
	if (err) {
		LOG_ERR("Could not init AT params list, error: %d", err);
		return err;
	}

	err = at_parser_max_params_from_str(buf,
					    NULL,
					    &at_resp_list,
					    AT_CEREG_PARAMS_COUNT_MAX);
	if (err) {
		LOG_ERR("Could not parse AT+CEREG response, error: %d", err);
		goto parse_psm_clean_exit;
	}

	err = parse_psm(&at_resp_list, false, &psm_cfg);
	if (err) {
		LOG_ERR("Could not obtain PSM configuration");
		goto parse_psm_clean_exit;
	}

	*tau = psm_cfg.tau;
	*active_time = psm_cfg.active_time;

	LOG_DBG("TAU: %d sec, active time: %d sec\n", *tau, *active_time);

parse_psm_clean_exit:
	at_params_list_free(&at_resp_list);

	return err;
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
	char req[25];

	if (!enable) {
		err = at_cmd_write(edrx_disable, NULL, 0, NULL);
		if (err) {
			LOG_ERR("Failed to disable eDRX, error: %d", err);
		}

		return err;
	}

	/* Apply the configurations for both LTE-M and NB-IoT. */
	for (size_t i = 0; i < ARRAY_SIZE(actt); i++) {
		char *edrx_param = (actt[i] == AT_CEDRXS_ACTT_WB) ?
					edrx_param_ltem : edrx_param_nbiot;
		char *ptw_param = (actt[i] == AT_CEDRXS_ACTT_WB) ?
					ptw_param_ltem : ptw_param_nbiot;

		if (strlen(edrx_param) == 4) {
			snprintk(req, sizeof(req), "AT+CEDRXS=2,%d,\"%s\"", actt[i], edrx_param);
		} else {
			snprintk(req, sizeof(req), "AT+CEDRXS=2,%d", actt[i]);
		}

		err = at_cmd_write(req, NULL, 0, NULL);
		if (err) {
			LOG_ERR("Failed to enable eDRX, error: %d", err);
			return err;
		}

		/* PTW must be requested after eDRX is enabled */
		if (strlen(ptw_param) != 4) {
			continue;
		}

		snprintk(req, sizeof(req), "AT%%XPTW=%d,\"%s\"", actt[i], ptw_param);

		err = at_cmd_write(req, NULL, 0, NULL);
		if (err) {
			LOG_ERR("Failed to request PTW (%s), error: %d", log_strdup(req), err);
			return err;
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
		return err;
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

	return at_cmd_printf("AT%%XRAI=%s", enable ? rai_param : "0");
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

int lte_lc_pdp_context_set(enum lte_lc_pdp_type type, const char *apn,
			   bool ip4_addr_alloc, bool nslpi, bool secure_pco)
{
#if defined(CONFIG_LTE_PDP_CMD)
	static const char * const pdp_type_lut[] = {
		"IP", "IPV6", "IPV4V6"
	};

	if (apn == NULL || type > LTE_LC_PDP_TYPE_IPV4V6) {
		return -EINVAL;
	}

	snprintf(cgdcont, sizeof(cgdcont),
		"AT+CGDCONT=0,\"%s\",\"%s\",0,0,0,%d,0,0,0,%d,%d",
		pdp_type_lut[type], apn, ip4_addr_alloc, nslpi, secure_pco);

	return 0;
#else
	return -ENOTSUP;
#endif
}

int lte_lc_pdn_auth_set(enum lte_lc_pdn_auth_type auth_prot,
			const char *username, const char *password)
{
#if defined(CONFIG_LTE_PDN_AUTH_CMD)
	if (username == NULL || password == NULL ||
	    auth_prot > LTE_LC_PDN_AUTH_TYPE_CHAP) {
		return -EINVAL;
	}

	snprintf(cgauth, sizeof(cgauth), "AT+CGAUTH=0,%d,\"%s\",\"%s\"",
		 auth_prot, username, password);

	return 0;
#else
	return -ENOTSUP;
#endif
}

int lte_lc_nw_reg_status_get(enum lte_lc_nw_reg_status *status)
{
	int err;

	if (status == NULL) {
		return -EINVAL;
	}

	/* Enable network registration status with level 5 */
	err = at_cmd_printf(AT_CEREG_5);
	if (err) {
		LOG_ERR("Could not set CEREG level 5, error: %d", err);
		return err;
	}

	/* Read network registration status */
	err = at_cmd_scanf(AT_CEREG_READ, "+CEREG: %*d,%d", status);
	if (err) {
		LOG_ERR("Could not get CEREG response, error: %d", err);
		return err;
	}

	return err;
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
		LOG_ERR("Invalid system mode requested");
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
		LOG_ERR("Invalid LTE preference requested");
		return -EINVAL;
	}

	err = at_cmd_printf("AT%%XSYSTEMMODE=%s,%c",
			    system_mode_params[mode],
			    system_mode_preference[preference]);
	if (err) {
		LOG_ERR("Could not send AT command, error: %d", err);
	}

	sys_mode_current = mode;
	sys_mode_target = mode;
	mode_pref_current = preference;
	mode_pref_target = preference;

	return err;
}

int lte_lc_system_mode_get(enum lte_lc_system_mode *mode,
			   enum lte_lc_system_mode_preference *preference)
{
	int err, temp_pref, mode_bitmask = 0;
	struct at_param_list resp_list = {0};
	char response[AT_XSYSTEMMODE_RESPONSE_MAX_LEN] = {0};
	char response_prefix[sizeof(AT_XSYSTEMMODE_RESPONSE_PREFIX)] = {0};
	size_t response_prefix_len = sizeof(response_prefix);

	if (mode == NULL) {
		return -EINVAL;
	}

	err = at_cmd_write(AT_XSYSTEMMODE_READ, response, sizeof(response),
			   NULL);
	if (err) {
		LOG_ERR("Could not send AT command");
		return err;
	}

	err = at_params_list_init(&resp_list, AT_XSYSTEMMODE_PARAMS_COUNT);
	if (err) {
		LOG_ERR("Could init AT params list, error: %d", err);
		return err;
	}

	err = at_parser_max_params_from_str(response, NULL, &resp_list,
					    AT_XSYSTEMMODE_PARAMS_COUNT);
	if (err) {
		LOG_ERR("Could not parse AT response, error: %d", err);
		goto clean_exit;
	}

	/* Check if AT command response starts with %XSYSTEMMODE */
	err = at_params_string_get(&resp_list,
				   AT_RESPONSE_PREFIX_INDEX,
				   response_prefix,
				   &response_prefix_len);
	if (err) {
		LOG_ERR("Could not get response prefix, error: %d", err);
		goto clean_exit;
	}

	if (!response_is_valid(response_prefix, response_prefix_len,
			       AT_XSYSTEMMODE_RESPONSE_PREFIX)) {
		LOG_ERR("Invalid XSYSTEMMODE response");
		err = -EIO;
		goto clean_exit;
	}

	/* We skip the first parameter, as that's the response prefix,
	 * "%XSYSTEMMODE:" in this case."
	 * The last parameter sets the preferred mode, and is not implemented
	 * yet on the modem side, so we ignore it.
	 */
	for (size_t i = 1; i < AT_XSYSTEMMODE_PARAMS_COUNT - 1; i++) {
		int param;

		err = at_params_int_get(&resp_list, i, &param);
		if (err) {
			LOG_ERR("Could not parse mode parameter, err: %d", err);
			goto clean_exit;
		}

		mode_bitmask = param ? mode_bitmask | BIT(i) : mode_bitmask;
	}

	/* Get LTE preference. */
	if (preference != NULL) {
		err = at_params_int_get(&resp_list, AT_XSYSTEMMODE_READ_PREFERENCE_INDEX,
					&temp_pref);
		if (err) {
			LOG_ERR("Could not parse LTE preference parameter, err: %d", err);
			goto clean_exit;
		}

		switch (temp_pref) {
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
			LOG_ERR("Unsupported LTE preference: %d", temp_pref);
			err = -EFAULT;
			goto clean_exit;
		}
	}

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
	case (BIT(AT_XSYSTEMMODE_READ_LTEM_INDEX) |
	      BIT(AT_XSYSTEMMODE_READ_GPS_INDEX)):
		*mode = LTE_LC_SYSTEM_MODE_LTEM_GPS;
		break;
	case (BIT(AT_XSYSTEMMODE_READ_NBIOT_INDEX) |
	      BIT(AT_XSYSTEMMODE_READ_GPS_INDEX)):
		*mode = LTE_LC_SYSTEM_MODE_NBIOT_GPS;
		break;
	case (BIT(AT_XSYSTEMMODE_READ_LTEM_INDEX) |
	      BIT(AT_XSYSTEMMODE_READ_NBIOT_INDEX)):
		*mode = LTE_LC_SYSTEM_MODE_LTEM_NBIOT;
		break;
	case (BIT(AT_XSYSTEMMODE_READ_LTEM_INDEX) |
	      BIT(AT_XSYSTEMMODE_READ_NBIOT_INDEX) |
	      BIT(AT_XSYSTEMMODE_READ_GPS_INDEX)):
		*mode = LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS;
		break;
	default:
		LOG_ERR("Invalid system mode, assuming parsing error");
		err = -EFAULT;
		goto clean_exit;
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

clean_exit:
	at_params_list_free(&resp_list);

	return err;
}

int lte_lc_func_mode_get(enum lte_lc_func_mode *mode)
{
	if (mode == NULL) {
		return -EINVAL;
	}

	return at_cmd_scanf(AT_CFUN_READ, "+CFUN: %d", mode);
}

int lte_lc_func_mode_set(enum lte_lc_func_mode mode)
{
	switch (mode) {
	case LTE_LC_FUNC_MODE_POWER_OFF:
	case LTE_LC_FUNC_MODE_NORMAL:
	case LTE_LC_FUNC_MODE_OFFLINE:
	case LTE_LC_FUNC_MODE_DEACTIVATE_LTE:
	case LTE_LC_FUNC_MODE_ACTIVATE_LTE:
	case LTE_LC_FUNC_MODE_DEACTIVATE_GNSS:
	case LTE_LC_FUNC_MODE_ACTIVATE_GNSS:
	case LTE_LC_FUNC_MODE_OFFLINE_UICC_ON: {
		return at_cmd_printf("AT+CFUN=%d", mode);
	}
	default:
		LOG_ERR("Invalid functional mode: %d", mode);
		return -EINVAL;
	}
}

int lte_lc_lte_mode_get(enum lte_lc_lte_mode *mode)
{
	int err;
	char buf[AT_CEREG_RESPONSE_MAX_LEN] = {0};

	/* Read network registration status */
	err = at_cmd_write(AT_CEREG_READ, buf, sizeof(buf), NULL);
	if (err) {
		LOG_ERR("Could not get CEREG response, error: %d", err);
		return err;
	}

	err = parse_cereg(buf, false, NULL, NULL, mode, NULL);
	if (err) {
		LOG_ERR("Could not parse registration status, err: %d", err);
		return err;
	}

	return 0;
}

#if defined(CONFIG_LTE_AUTO_INIT_AND_CONNECT)
SYS_DEVICE_DEFINE("LTE_LINK_CONTROL", init_and_connect,
		  device_pm_control_nop,
		  APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif /* CONFIG_LTE_AUTO_INIT_AND_CONNECT */
