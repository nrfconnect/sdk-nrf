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

LOG_MODULE_REGISTER(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

#define LC_MAX_READ_LENGTH			128
#define AT_CMD_SIZE(x)				(sizeof(x) - 1)
#define AT_RESPONSE_PREFIX_INDEX		0
#define AT_CFUN_READ				"AT+CFUN?"
#define AT_CFUN_RESPONSE_PREFIX			"+CFUN"
#define AT_CFUN_MODE_INDEX			1
#define AT_CFUN_PARAMS_COUNT			2
#define AT_CFUN_RESPONSE_MAX_LEN		20
#define AT_CEREG_5				"AT+CEREG=5"
#define AT_CEREG_READ				"AT+CEREG?"
#define AT_CEREG_RESPONSE_PREFIX		"+CEREG"
#define AT_CEREG_PARAMS_COUNT_MAX		10
#define AT_CEREG_REG_STATUS_INDEX		1
#define AT_CEREG_READ_REG_STATUS_INDEX		2
#define AT_CEREG_TAC_INDEX			2
#define AT_CEREG_READ_TAC_INDEX			3
#define AT_CEREG_CELL_ID_INDEX			3
#define AT_CEREG_READ_CELL_ID_INDEX		4
#define AT_CEREG_ACTIVE_TIME_INDEX		7
#define AT_CEREG_READ_ACTIVE_TIME_INDEX		8
#define AT_CEREG_TAU_INDEX			8
#define AT_CEREG_READ_TAU_INDEX			9
#define AT_CEREG_RESPONSE_MAX_LEN		80
#define AT_XSYSTEMMODE_READ			"AT%XSYSTEMMODE?"
#define AT_XSYSTEMMODE_RESPONSE_PREFIX		"%XSYSTEMMODE"
#define AT_XSYSTEMMODE_PROTO			"AT%%XSYSTEMMODE=%d,%d,%d,%d"
/* The indices are for the set command. Add 1 for the read command indices. */
#define AT_XSYSTEMMODE_LTEM_INDEX		0
#define AT_XSYSTEMMODE_NBIOT_INDEX		1
#define AT_XSYSTEMMODE_GPS_INDEX		2
#define AT_XSYSTEMMODE_PARAMS_COUNT		5
#define AT_XSYSTEMMODE_RESPONSE_MAX_LEN		30
/* CEDRXS command parameters */
#define AT_CEDRXS_MODE_INDEX
#define AT_CEDRXS_ACTT_WB			4
#define AT_CEDRXS_ACTT_NB			5
/* CEDRXP notification parameters */
#define AT_CEDRXP_PARAMS_COUNT_MAX		5
#define AT_CEDRXP_ACTT_INDEX			1
#define AT_CEDRXP_REQ_EDRX_INDEX		2
#define AT_CEDRXP_NW_EDRX_INDEX			3
#define AT_CEDRXP_NW_PTW_INDEX			4
/* CSCON command parameters */
#define AT_CSCON_RESPONSE_PREFIX		"+CSCON"
#define AT_CSCON_PARAMS_COUNT_MAX		4
#define AT_CSCON_RRC_MODE_INDEX			1
#define AT_CSCON_READ_RRC_MODE_INDEX		2

#define SYS_MODE_PREFERRED \
	(IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M)	? \
		LTE_LC_SYSTEM_MODE_LTEM			: \
	IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT)	? \
		LTE_LC_SYSTEM_MODE_NBIOT		: \
	IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_GPS)	? \
		LTE_LC_SYSTEM_MODE_LTEM_GPS		: \
	IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT_GPS)	? \
		LTE_LC_SYSTEM_MODE_NBIOT_GPS		: \
	LTE_LC_SYSTEM_MODE_NONE)

/* Forward declarations */
static int parse_nw_reg_status(const char *at_response,
			       enum lte_lc_nw_reg_status *status,
			       size_t reg_status_index);
static int parse_rrc_mode(const char *at_response,
			  enum lte_lc_rrc_mode *mode,
			  size_t mode_index);
static int parse_edrx(const char *at_response,
		      struct lte_lc_edrx_cfg *cfg);
static bool response_is_valid(const char *response, size_t response_len,
			      const char *check);
static int parse_psm_cfg(struct at_param_list *at_params,
			 bool is_notif,
			 struct lte_lc_psm_cfg *psm_cfg);

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
static char edrx_param[5] = CONFIG_LTE_EDRX_REQ_VALUE;
/* Default PTW setting */
static char ptw_param[5] = CONFIG_LTE_PTW_VALUE;
/* Default PSM RAT setting */
static char psm_param_rat[9] = CONFIG_LTE_PSM_REQ_RAT;
/* Default PSM RPATU setting */
static char psm_param_rptau[9] = CONFIG_LTE_PSM_REQ_RPTAU;
/* Request PSM to be disabled */
static const char psm_disable[] = "AT+CPSMS=";
/* Set the modem to power off mode */
static const char power_off[] = "AT+CFUN=0";
/* Set the modem to Normal mode */
static const char normal[] = "AT+CFUN=1";
/* Set the modem to Offline mode */
static const char offline[] = "AT+CFUN=4";
/* Enable CSCON (RRC mode) notifications */
static const char cscon[] = "AT+CSCON=1";
/* Disable RAI */
static const char rai_disable[] = "AT+%XRAI=0";
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

/* Parameters to be passed when using AT%XSYSTEMMMODE=<params> */
static const char *const system_mode_params[] = {
	[LTE_LC_SYSTEM_MODE_LTEM]	= "1,0,0,0",
	[LTE_LC_SYSTEM_MODE_NBIOT]	= "0,1,0,0",
	[LTE_LC_SYSTEM_MODE_GPS]	= "0,0,1,0",
	[LTE_LC_SYSTEM_MODE_LTEM_GPS]	= "1,0,1,0",
	[LTE_LC_SYSTEM_MODE_NBIOT_GPS]	= "0,1,1,0",
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

enum lte_lc_notif_type {
	LTE_LC_NOTIF_CEREG,
	LTE_LC_NOTIF_CSCON,
	LTE_LC_NOTIF_CEDRXP,

	LTE_LC_NOTIF_COUNT,
};

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

static int parse_cereg(const char *notification,
		       enum lte_lc_nw_reg_status *reg_status,
		       struct lte_lc_cell *cell,
		       struct lte_lc_psm_cfg *psm_cfg)
{
	int err, status;
	struct at_param_list resp_list;
	char str_buf[10];
	size_t len = sizeof(str_buf) - 1;

	err = at_params_list_init(&resp_list, AT_CEREG_PARAMS_COUNT_MAX);
	if (err) {
		LOG_ERR("Could not init AT params list, error: %d", err);
		return err;
	}

	/* Parse CEREG response and populate AT parameter list */
	err = at_parser_params_from_str(notification,
					NULL,
					&resp_list);
	if (err) {
		LOG_ERR("Could not parse AT+CEREG response, error: %d", err);
		goto clean_exit;
	}

	/* Parse network registration status */
	err = at_params_int_get(&resp_list,
				AT_CEREG_REG_STATUS_INDEX,
				&status);
	if (err) {
		LOG_ERR("Could not get registration status, error: %d", err);
		goto clean_exit;
	}

	*reg_status = status;

	if ((*reg_status != LTE_LC_NW_REG_UICC_FAIL) &&
	    (at_params_valid_count_get(&resp_list) > AT_CEREG_CELL_ID_INDEX)) {
		/* Parse tracking area code */
		err = at_params_string_get(&resp_list,
					AT_CEREG_TAC_INDEX,
					str_buf, &len);
		if (err) {
			LOG_ERR("Could not get tracking area code, error: %d", err);
			goto clean_exit;
		}

		str_buf[len] = '\0';
		cell->tac = strtoul(str_buf, NULL, 16);

		/* Parse cell ID */
		len = sizeof(str_buf) - 1;

		err = at_params_string_get(&resp_list,
					AT_CEREG_CELL_ID_INDEX,
					str_buf, &len);
		if (err) {
			LOG_ERR("Could not get cell ID, error: %d", err);
			goto clean_exit;
		}

		str_buf[len] = '\0';
		cell->id = strtoul(str_buf, NULL, 16);
	} else {
		cell->tac = UINT32_MAX;
		cell->id = UINT32_MAX;
	}

	/* Parse PSM configuration only when registered */
	if (((*reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
	    (*reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) &&
	     (at_params_valid_count_get(&resp_list) > AT_CEREG_TAU_INDEX)) {
		err = parse_psm_cfg(&resp_list, true, psm_cfg);
		if (err) {
			LOG_ERR("Failed to parse PSM configuration, error: %d",
				err);
			goto clean_exit;
		}
	} else {
		/* When device is not registered, PSM valies are invalid */
		psm_cfg->tau = -1;
		psm_cfg->active_time = -1;
	}

clean_exit:
	at_params_list_free(&resp_list);

	return err;
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
		enum lte_lc_nw_reg_status reg_status = 0;
		struct lte_lc_cell cell;
		struct lte_lc_psm_cfg psm_cfg;

		LOG_DBG("+CEREG notification: %s", log_strdup(response));

		err = parse_cereg(response, &reg_status, &cell, &psm_cfg);
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

static int parse_psm_cfg(struct at_param_list *at_params,
			 bool is_notif,
			 struct lte_lc_psm_cfg *psm_cfg)
{
	int err;
	size_t tau_idx = is_notif ? AT_CEREG_TAU_INDEX :
				    AT_CEREG_READ_TAU_INDEX;
	size_t active_time_idx = is_notif ? AT_CEREG_ACTIVE_TIME_INDEX :
					    AT_CEREG_READ_ACTIVE_TIME_INDEX;
	char timer_str[9] = {0};
	char unit_str[4] = {0};
	size_t timer_str_len = sizeof(timer_str) - 1;
	size_t unit_str_len = sizeof(unit_str) - 1;
	size_t lut_idx;
	uint32_t timer_unit, timer_value;

	/* Lookup table for T3324 timer used for PSM active time in seconds.
	 * Ref: GPRS Timer 2 IE in 3GPP TS 24.008 Table 10.5.163/3GPP TS 24.008.
	 */
	static const uint32_t t3324_lookup[8] = {2, 60, 600, 60, 60, 60, 60, 0};

	/* Lookup table for T3412 timer used for periodic TAU. Unit is seconds.
	 * Ref: GPRS Timer 3 in 3GPP TS 24.008 Table 10.5.163a/3GPP TS 24.008.
	 */
	static const uint32_t t3412_lookup[8] = {600, 3600, 36000, 2, 30, 60,
					      1152000, 0};

	/* Parse periodic TAU string */
	err = at_params_string_get(at_params,
				   tau_idx,
				   timer_str,
				   &timer_str_len);
	if (err) {
		LOG_ERR("Could not get TAU, error: %d", err);
		return err;
	}

	memcpy(unit_str, timer_str, unit_str_len);

	lut_idx = strtoul(unit_str, NULL, 2);
	if (lut_idx > (ARRAY_SIZE(t3412_lookup) - 1)) {
		LOG_ERR("Unable to parse periodic TAU string");
		err = -EINVAL;
		return err;
	}

	timer_unit = t3412_lookup[lut_idx];
	timer_value = strtoul(timer_str + unit_str_len, NULL, 2);
	psm_cfg->tau = timer_unit ? timer_unit * timer_value : -1;

	/* Parse active time string */
	err = at_params_string_get(at_params,
				   active_time_idx,
				   timer_str,
				   &timer_str_len);
	if (err) {
		LOG_ERR("Could not get TAU, error: %d", err);
		return err;
	}

	memcpy(unit_str, timer_str, unit_str_len);

	lut_idx = strtoul(unit_str, NULL, 2);
	if (lut_idx > (ARRAY_SIZE(t3324_lookup) - 1)) {
		LOG_ERR("Unable to parse active time string");
		err = -EINVAL;
		return err;
	}

	timer_unit = t3324_lookup[lut_idx];
	timer_value = strtoul(timer_str + unit_str_len, NULL, 2);
	psm_cfg->active_time = timer_unit ? timer_unit * timer_value : -1;

	LOG_DBG("TAU: %d sec, active time: %d sec\n",
		psm_cfg->tau, psm_cfg->active_time);

	return 0;
}

static int w_lte_lc_init(void)
{
	int err;

	if (is_initialized) {
		return -EALREADY;
	}

	k_sem_init(&link, 0, 1);

	err = lte_lc_system_mode_get(&sys_mode_current);
	if (err) {
		LOG_ERR("Could not get current system mode, error: %d", err);
		return err;
	}

	err = at_notif_register_handler(NULL, at_handler);
	if (err) {
		LOG_ERR("Can't register AT handler, error: %d", err);
		return err;
	}

	if (sys_mode_current != sys_mode_target) {
		err = lte_lc_system_mode_set(sys_mode_target);
		if (err) {
			LOG_ERR("Could not set system mode, error: %d", err);
			return err;
		}
	} else {
		LOG_DBG("Preferred system mode (%d) is already configured",
			sys_mode_current);
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
	if (at_cmd_write(cereg_5_subscribe, NULL, 0, NULL) != 0) {
		return -EIO;
	}

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

	is_initialized = true;

	return 0;
}

static int w_lte_lc_connect(bool blocking)
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

		err = lte_lc_system_mode_set(sys_mode_target);
		if (err) {
			return err;
		}

		err = lte_lc_normal();
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

				err = lte_lc_offline();
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

static int w_lte_lc_init_and_connect(const struct device *unused)
{
	int ret;

	ret = w_lte_lc_init();
	if (ret) {
		return ret;
	}

	return w_lte_lc_connect(true);
}

/* lte lc Init wrapper */
int lte_lc_init(void)
{
	return w_lte_lc_init();
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

/* lte lc Connect wrapper */
int lte_lc_connect(void)
{
	return w_lte_lc_connect(true);
}

/* lte lc Init and connect wrapper */
int lte_lc_init_and_connect(void)
{
	const struct device *x = 0;

	int err = w_lte_lc_init_and_connect(x);

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

	return w_lte_lc_connect(false);
}

int lte_lc_init_and_connect_async(lte_lc_evt_handler_t handler)
{
	int err;

	err = w_lte_lc_init();
	if (err) {
		return err;
	}

	return lte_lc_connect_async(handler);
}

int lte_lc_offline(void)
{
	if (at_cmd_write(offline, NULL, 0, NULL) != 0) {
		return -EIO;
	}

	return 0;
}

int lte_lc_power_off(void)
{
	if (at_cmd_write(power_off, NULL, 0, NULL) != 0) {
		return -EIO;
	}

	return 0;
}

int lte_lc_deinit(void)
{
	if (is_initialized) {
		is_initialized = false;
		at_notif_deregister_handler(NULL, at_handler);
		return lte_lc_power_off();
	}

	return 0;
}

int lte_lc_normal(void)
{
	if (at_cmd_write(normal, NULL, 0, NULL) != 0) {
		return -EIO;
	}

	return 0;
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

	err = parse_psm_cfg(&at_resp_list, false, &psm_cfg);
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

int lte_lc_edrx_param_set(const char *edrx)
{
	if (edrx != NULL && strlen(edrx) != 4) {
		return -EINVAL;
	}

	if (edrx != NULL) {
		strcpy(edrx_param, edrx);
		LOG_DBG("eDRX set to %s", log_strdup(edrx_param));
	} else {
		*edrx_param = '\0';
		LOG_DBG("eDRX use default");
	}

	return 0;
}

int lte_lc_ptw_set(const char *ptw)
{
	if (ptw != NULL && strlen(ptw) != 4) {
		return -EINVAL;
	}

	if (ptw != NULL) {
		strcpy(ptw_param, ptw);
		LOG_DBG("PTW set to %s", log_strdup(ptw_param));
	} else {
		*ptw_param = '\0';
		LOG_DBG("PTW use default");
	}

	return 0;
}

int lte_lc_edrx_req(bool enable)
{
	int err, actt;
	char req[25];

	if (sys_mode_current == LTE_LC_SYSTEM_MODE_NONE) {
		err = lte_lc_system_mode_get(&sys_mode_current);
		if (err) {
			return err;
		}
	}

	switch (sys_mode_current) {
	case LTE_LC_SYSTEM_MODE_LTEM:
	case LTE_LC_SYSTEM_MODE_LTEM_GPS:
		actt = AT_CEDRXS_ACTT_WB;
		break;
	case LTE_LC_SYSTEM_MODE_NBIOT:
	case LTE_LC_SYSTEM_MODE_NBIOT_GPS:
		actt = AT_CEDRXS_ACTT_NB;
		break;
	default:
		LOG_ERR("Cannot request eDRX for this system mode (%d)",
			sys_mode_current);
		return -EOPNOTSUPP;
	}

	if (enable) {
		if (strlen(edrx_param) == 4) {
			snprintf(req, sizeof(req),
				"AT+CEDRXS=2,%d,\"%s\"", actt, edrx_param);
		} else {
			snprintf(req, sizeof(req),
				"AT+CEDRXS=2,%d", actt);
		}
		err = at_cmd_write(req, NULL, 0, NULL);
	} else {
		err = at_cmd_write(edrx_disable, NULL, 0, NULL);
	}
	if (err) {
		LOG_ERR("Failed to %s eDRX, error: %d",
			enable ? "enable" : "disable", err);
		return err;
	}

	/* PTW must be requested after eDRX is enabled */
	if (enable) {
		if (strlen(ptw_param) == 4) {
			snprintf(req, sizeof(req),
				"AT%%XPTW=%d,\"%s\"", actt, ptw_param);
		} else {
			snprintf(req, sizeof(req),
				"AT%%XPTW=%d", actt);
		}
		err = at_cmd_write(req, NULL, 0, NULL);
		if (err) {
			LOG_ERR("Failed to request PTW (%s), error: %d",
				log_strdup(req), err);
			return err;
		}
	}

	return 0;
}

int lte_lc_rai_req(bool enable)
{
	int err;
	enum lte_lc_system_mode mode;

	err = lte_lc_system_mode_get(&mode);
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

	if (enable) {
		char rai_req[10];

		snprintf(rai_req, sizeof(rai_req), "AT%%XRAI=%s", rai_param);
		err = at_cmd_write(rai_req, NULL, 0, NULL);
	} else {
		err = at_cmd_write(rai_disable, NULL, 0, NULL);
	}

	return err;
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

/**@brief Helper function to check if a response is what was expected
 *
 * @param response Pointer to response prefix
 * @param response_len Length of the response to be checked
 * @param check The buffer with "truth" to verify the response against,
 *		for example "+CEREG"
 *
 * @return True if the provided buffer and check are equal, false otherwise.
 */
static bool response_is_valid(const char *response, size_t response_len,
			      const char *check)
{
	if ((response == NULL) || (check == NULL)) {
		LOG_ERR("Invalid pointer provided");
		return false;
	}

	if ((response_len < strlen(check)) ||
	    (memcmp(response, check, response_len) != 0)) {
		return false;
	}

	return true;
}

/**@brief Parses an AT command response, and returns the current network
 *	  registration status if it's available in the string.
 *
 * @param at_response Pointer to buffer with AT response.
 * @param status Pointer to where the registration status is stored.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
static int parse_nw_reg_status(const char *at_response,
			       enum lte_lc_nw_reg_status *status,
			       size_t reg_status_index)
{
	int err, reg_status;
	struct at_param_list resp_list = {0};
	char  response_prefix[sizeof(AT_CEREG_RESPONSE_PREFIX)] = {0};
	size_t response_prefix_len = sizeof(response_prefix);

	if ((at_response == NULL) || (status == NULL)) {
		return -EINVAL;
	}

	err = at_params_list_init(&resp_list, AT_CEREG_PARAMS_COUNT_MAX);
	if (err) {
		LOG_ERR("Could not init AT params list, error: %d", err);
		return err;
	}

	/* Parse CEREG response and populate AT parameter list */
	err = at_parser_max_params_from_str(at_response,
					    NULL,
					    &resp_list,
					    AT_CEREG_PARAMS_COUNT_MAX);
	if (err) {
		LOG_ERR("Could not parse AT+CEREG response, error: %d", err);
		goto clean_exit;
	}

	/* Check if AT command response starts with +CEREG */
	err = at_params_string_get(&resp_list,
				   AT_RESPONSE_PREFIX_INDEX,
				   response_prefix,
				   &response_prefix_len);
	if (err) {
		LOG_ERR("Could not get response prefix, error: %d", err);
		goto clean_exit;
	}

	if (!response_is_valid(response_prefix, response_prefix_len,
			       AT_CEREG_RESPONSE_PREFIX)) {
		/* The unsolicited response is not a CEREG response, ignore it.
		 */
		goto clean_exit;
	}

	/* Get the network registration status parameter from the response */
	err = at_params_int_get(&resp_list, reg_status_index,
				&reg_status);
	if (err) {
		LOG_ERR("Could not get registration status, error: %d", err);
		goto clean_exit;
	}

	/* Check if the parsed value maps to a valid registration status */
	switch (reg_status) {
	case LTE_LC_NW_REG_NOT_REGISTERED:
	case LTE_LC_NW_REG_REGISTERED_HOME:
	case LTE_LC_NW_REG_SEARCHING:
	case LTE_LC_NW_REG_REGISTRATION_DENIED:
	case LTE_LC_NW_REG_UNKNOWN:
	case LTE_LC_NW_REG_REGISTERED_ROAMING:
	case LTE_LC_NW_REG_REGISTERED_EMERGENCY:
	case LTE_LC_NW_REG_UICC_FAIL:
		*status = reg_status;
		LOG_DBG("Network registration status: %d", reg_status);
		break;
	default:
		LOG_ERR("Invalid network registration status: %d", reg_status);
		err = -EIO;
	}

clean_exit:
	at_params_list_free(&resp_list);

	return err;
}

/**@brief Parses an AT command response, and returns the current RRC mode.
 *
 * @param at_response Pointer to buffer with AT response.
 * @param mode Pointer to where the RRC mode is stored.
 * @param mode_index Parameter index for mode.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
static int parse_rrc_mode(const char *at_response,
			  enum lte_lc_rrc_mode *mode,
			  size_t mode_index)
{
	int err, temp_mode;
	struct at_param_list resp_list = {0};

	err = at_params_list_init(&resp_list, AT_CSCON_PARAMS_COUNT_MAX);
	if (err) {
		LOG_ERR("Could not init AT params list, error: %d", err);
		return err;
	}

	/* Parse CSCON response and populate AT parameter list */
	err = at_parser_params_from_str(at_response,
					NULL,
					&resp_list);
	if (err) {
		LOG_ERR("Could not parse +CSCON response, error: %d", err);
		goto clean_exit;
	}

	/* Get the RRC mode from the response */
	err = at_params_int_get(&resp_list, mode_index, &temp_mode);
	if (err) {
		LOG_ERR("Could not get signalling mode, error: %d", err);
		goto clean_exit;
	}

	/* Check if the parsed value maps to a valid registration status */
	if (temp_mode == 0) {
		*mode = LTE_LC_RRC_MODE_IDLE;
	} else if (temp_mode == 1) {
		*mode = LTE_LC_RRC_MODE_CONNECTED;
	} else {
		LOG_ERR("Invalid signalling mode: %d", temp_mode);
		err = -EINVAL;
	}

clean_exit:
	at_params_list_free(&resp_list);

	return err;
}

/* Confirm valid system mode and set Paging Time Window multiplier.
 * Multiplier is 1.28 s for LTE-M, and 2.56 s for NB-IoT, derived from
 * Figure 10.5.5.32/3GPP TS 24.008.
 */
static int get_ptw_multiplier(float *ptw_multiplier)
{
	switch (sys_mode_current) {
	case LTE_LC_SYSTEM_MODE_LTEM: /* Fall through */
	case LTE_LC_SYSTEM_MODE_LTEM_GPS:
		*ptw_multiplier = 1.28;
		break;
	case LTE_LC_SYSTEM_MODE_NBIOT: /* Fall through */
	case LTE_LC_SYSTEM_MODE_NBIOT_GPS:
		*ptw_multiplier = 2.56;
		break;
	case LTE_LC_SYSTEM_MODE_GPS: /* Fall through */
	case LTE_LC_SYSTEM_MODE_NONE: /* Fall through */
	default:
		LOG_ERR("No LTE connection available in this system mode");
		return -ENOTCONN;
	}

	return 0;
}

static int get_edrx_value(uint8_t idx, float *edrx_value)
{
	uint16_t multiplier = 0;

	/* Lookup table to eDRX multiplier values, based on T_eDRX values found
	 * in Table 10.5.5.32/3GPP TS 24.008. The actual value is
	 * (multiplier * 10.24 s), except for the first entry which is handled
	 * as a special case per note 3 in the specification.
	 */
	static const uint16_t edrx_lookup_ltem[16] = {
		0, 1, 2, 4, 6, 8, 10, 12, 14, 16, 32, 64, 128, 256, 256, 256
	};
	static const uint16_t edrx_lookup_nbiot[16] = {
		2, 2, 2, 4, 2, 8, 2, 2, 2, 16, 32, 64, 128, 256, 512, 1024
	};

	if ((edrx_value == NULL) || (idx > ARRAY_SIZE(edrx_lookup_ltem) - 1)) {
		return -EINVAL;
	}

	switch (sys_mode_current) {
	case LTE_LC_SYSTEM_MODE_LTEM: /* Fall through */
	case LTE_LC_SYSTEM_MODE_LTEM_GPS:
		multiplier = edrx_lookup_ltem[idx];
		break;
	case LTE_LC_SYSTEM_MODE_NBIOT: /* Fall through */
	case LTE_LC_SYSTEM_MODE_NBIOT_GPS:
		multiplier = edrx_lookup_nbiot[idx];
		break;
	case LTE_LC_SYSTEM_MODE_GPS: /* Fall through */
	case LTE_LC_SYSTEM_MODE_NONE: /* Fall through */
	default:
		LOG_ERR("No LTE connection available in this system mode");
		return -ENOTCONN;
	}

	*edrx_value = multiplier == 0 ? 5.12 : multiplier * 10.24;

	return 0;
}

/**@brief Parses an AT command response, and returns the current eDRX settings.
 *
 * @note It's assumed that the network only reports valid eDRX values when
 *	 in each mode (LTE-M and NB1). There's no sanity-check of these values.
 *
 * @param at_response Pointer to buffer with AT response.
 * @param cfg Pointer to where the eDRX configuration is stored.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
static int parse_edrx(const char *at_response,
		      struct lte_lc_edrx_cfg *cfg)
{
	int err;
	uint8_t idx;
	struct at_param_list resp_list = {0};
	char tmp_buf[5];
	size_t len = sizeof(tmp_buf) - 1;
	float ptw_multiplier;

	if ((at_response == NULL) || (cfg == NULL)) {
		return -EINVAL;
	}

	/* Confirm valid system mode and set Paging Time Window multiplier.
	 * Multiplier is 1.28 s for LTE-M, and 2.56 s for NB-IoT, derived from
	 * figure 10.5.5.32/3GPP TS 24.008.
	 */
	err = get_ptw_multiplier(&ptw_multiplier);
	if (err) {
		return err;
	}

	err = at_params_list_init(&resp_list, AT_CEDRXP_PARAMS_COUNT_MAX);
	if (err) {
		LOG_ERR("Could not init AT params list, error: %d", err);
		return err;
	}

	/* Parse CEDRXP response and populate AT parameter list */
	err = at_parser_params_from_str(at_response,
					NULL,
					&resp_list);
	if (err) {
		LOG_ERR("Could not parse +CEDRXP response, error: %d", err);
		goto clean_exit;
	}

	err = at_params_string_get(&resp_list, AT_CEDRXP_NW_EDRX_INDEX,
				   tmp_buf, &len);
	if (err) {
		LOG_ERR("Failed to get eDRX configuration, error: %d", err);
		goto clean_exit;
	}

	tmp_buf[len] = '\0';

	/* The eDRX value is a multiple of 10.24 seconds, except for the
	 * special case of idx == 0 for LTE-M, where the value is 5.12 seconds.
	 * The variable idx is used to map to the entry of index idx in
	 * Figure 10.5.5.32/3GPP TS 24.008, table for eDRX in S1 mode, and
	 * note 4 and 5 are taken into account.
	 */
	idx = strtoul(tmp_buf, NULL, 2);

	err = get_edrx_value(idx, &cfg->edrx);
	if (err) {
		LOG_ERR("Failed to get eDRX value, error; %d", err);
		goto clean_exit;
	}

	len = sizeof(tmp_buf) - 1;

	err = at_params_string_get(&resp_list, AT_CEDRXP_NW_PTW_INDEX,
				   tmp_buf, &len);
	if (err) {
		LOG_ERR("Failed to get PTW configuration, error: %d", err);
		goto clean_exit;
	}

	tmp_buf[len] = '\0';

	/* Value can be a maximum of 15, as there are 16 entries in the table
	 * for paging time window (both for LTE-M and NB1).
	 */
	idx = strtoul(tmp_buf, NULL, 2);
	if (idx > 15) {
		LOG_ERR("Invalid PTW lookup index: %d", idx);
		err = -EINVAL;
		goto clean_exit;
	}

	/* The Paging Time Window is different for LTE-M and NB-IoT:
	 *	- LTE-M: (idx + 1) * 1.28 s
	 *	- NB-IoT (idx + 1) * 2.56 s
	 */
	idx += 1;
	cfg->ptw = idx * ptw_multiplier;

	LOG_DBG("eDRX value: %d.%02d, PTW: %d.%02d",
		(int)cfg->edrx,
		(int)(100 * (cfg->edrx - (int)cfg->edrx)),
		(int)cfg->ptw,
		(int)(100 * (cfg->ptw - (int)cfg->ptw)));

clean_exit:
	at_params_list_free(&resp_list);

	return err;
}

int lte_lc_nw_reg_status_get(enum lte_lc_nw_reg_status *status)
{
	int err;
	char buf[AT_CEREG_RESPONSE_MAX_LEN] = {0};

	if (status == NULL) {
		return -EINVAL;
	}

	/* Enable network registration status with level 5 */
	err = at_cmd_write(AT_CEREG_5, NULL, 0, NULL);
	if (err) {
		LOG_ERR("Could not set CEREG level 5, error: %d", err);
		return err;
	}

	/* Read network registration status */
	err = at_cmd_write(AT_CEREG_READ, buf, sizeof(buf), NULL);
	if (err) {
		LOG_ERR("Could not get CEREG response, error: %d", err);
		return err;
	}

	err = parse_nw_reg_status(buf, status, AT_CEREG_READ_REG_STATUS_INDEX);
	if (err) {
		LOG_ERR("Could not parse registration status, err: %d", err);
		return err;
	}

	return err;
}

int lte_lc_system_mode_set(enum lte_lc_system_mode mode)
{
	int err, len;
	char cmd[50];

	switch (mode) {
	case LTE_LC_SYSTEM_MODE_NONE:
		LOG_DBG("No system mode set");
		return 0;
	case LTE_LC_SYSTEM_MODE_LTEM:
	case LTE_LC_SYSTEM_MODE_LTEM_GPS:
	case LTE_LC_SYSTEM_MODE_NBIOT:
	case LTE_LC_SYSTEM_MODE_NBIOT_GPS:
	case LTE_LC_SYSTEM_MODE_GPS:
		break;
	default:
		LOG_ERR("Invalid system mode requested");
		return -EINVAL;
	}

	len = snprintk(cmd, sizeof(cmd), "AT%%XSYSTEMMODE=%s",
		       system_mode_params[mode]);
	if (len < 0) {
		LOG_ERR("Could not construct system mode command");
		return -EFAULT;
	}

	LOG_DBG("Sending AT command to set system mode: %s", log_strdup(cmd));

	err = at_cmd_write(cmd, NULL, 0, NULL);
	if (err) {
		LOG_ERR("Could not send AT command, error: %d", err);
	}

	sys_mode_current = mode;
	sys_mode_target = mode;

	return err;
}

int lte_lc_system_mode_get(enum lte_lc_system_mode *mode)
{
	int err, bitmask = 0;
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

		bitmask = param ? bitmask | BIT(i) : bitmask;
	}

	/* When checking the bitmask, we need to add 1 to the indices,
	 * as the response prefix is also counted as a parameter.
	 */
	switch (bitmask) {
	case 0:
		*mode = LTE_LC_SYSTEM_MODE_NONE;
		break;
	case BIT(AT_XSYSTEMMODE_LTEM_INDEX + 1):
		*mode = LTE_LC_SYSTEM_MODE_LTEM;
		break;
	case BIT(AT_XSYSTEMMODE_NBIOT_INDEX + 1):
		*mode = LTE_LC_SYSTEM_MODE_NBIOT;
		break;
	case BIT(AT_XSYSTEMMODE_GPS_INDEX + 1):
		*mode = LTE_LC_SYSTEM_MODE_GPS;
		break;
	case (BIT(AT_XSYSTEMMODE_LTEM_INDEX + 1) |
	      BIT(AT_XSYSTEMMODE_GPS_INDEX + 1)):
		*mode = LTE_LC_SYSTEM_MODE_LTEM_GPS;
		break;
	case (BIT(AT_XSYSTEMMODE_NBIOT_INDEX + 1) |
	      BIT(AT_XSYSTEMMODE_GPS_INDEX + 1)):
		*mode = LTE_LC_SYSTEM_MODE_NBIOT_GPS;
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

clean_exit:
	at_params_list_free(&resp_list);

	return err;
}

int lte_lc_func_mode_get(enum lte_lc_func_mode *mode)
{
	int err, resp_mode;
	struct at_param_list resp_list = {0};
	char response[AT_CFUN_RESPONSE_MAX_LEN] = {0};
	char response_prefix[sizeof(AT_CFUN_RESPONSE_PREFIX)] = {0};
	size_t response_prefix_len = sizeof(response_prefix);

	if (mode == NULL) {
		return -EINVAL;
	}

	err = at_cmd_write(AT_CFUN_READ, response, sizeof(response), NULL);
	if (err) {
		LOG_ERR("Could not send AT command");
		return err;
	}

	err = at_params_list_init(&resp_list, AT_CFUN_PARAMS_COUNT);
	if (err) {
		LOG_ERR("Could init AT params list, error: %d", err);
		return err;
	}

	err = at_parser_max_params_from_str(response, NULL, &resp_list,
					    AT_CFUN_PARAMS_COUNT);
	if (err) {
		LOG_ERR("Could not parse AT response, error: %d", err);
		goto clean_exit;
	}

	/* Check if AT command response starts with +CFUN */
	err = at_params_string_get(&resp_list,
				   AT_RESPONSE_PREFIX_INDEX,
				   response_prefix,
				   &response_prefix_len);
	if (err) {
		LOG_ERR("Could not get response prefix, error: %d", err);
		goto clean_exit;
	}

	if (!response_is_valid(response_prefix, response_prefix_len,
			       AT_CFUN_RESPONSE_PREFIX)) {
		LOG_ERR("Invalid CFUN response");
		err = -EIO;
		goto clean_exit;
	}

	err = at_params_int_get(&resp_list, AT_CFUN_MODE_INDEX, &resp_mode);
	if (err) {
		LOG_ERR("Could not parse mode parameter, err: %d", err);
		goto clean_exit;
	}

	*mode = resp_mode;

clean_exit:
	at_params_list_free(&resp_list);

	return err;
}

#if defined(CONFIG_LTE_AUTO_INIT_AND_CONNECT)
SYS_DEVICE_DEFINE("LTE_LINK_CONTROL", w_lte_lc_init_and_connect,
		  device_pm_control_nop,
		  APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif /* CONFIG_LTE_AUTO_INIT_AND_CONNECT */
