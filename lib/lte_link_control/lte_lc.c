/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
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
#define AT_CEREG_ACTIVE_TIME_INDEX		8
#define AT_CEREG_TAU_INDEX			9
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
/* Parameter values for AT+CEDRXS command. */
#define AT_CEDRXS_ACTT_WB			4
#define AT_CEDRXS_ACTT_NB			5
#define AT_CSCON_RESPONSE_PREFIX		"+CSCON"
#define AT_CSCON_PARAMS_COUNT_MAX		4
#define AT_CSCON_RRC_MODE_INDEX			1
#define AT_CSCON_READ_RRC_MODE_INDEX		2

/* Forward declarations */
static int parse_nw_reg_status(const char *at_response,
			       enum lte_lc_nw_reg_status *status,
			       size_t reg_status_index);
static int parse_rrc_mode(const char *at_response,
			  enum lte_lc_rrc_mode *mode,
			  size_t mode_index);
static bool response_is_valid(const char *response, size_t response_len,
			      const char *check);

static lte_lc_evt_handler_t evt_handler;

/* Lookup table for T3324 timer used for PSM active time. Unit is seconds.
 * Ref: GPRS Timer 2 IE in 3GPP TS 24.008 Table 10.5.163/3GPP TS 24.008.
 */
static const u32_t t3324_lookup[8] = {2, 60, 600, 60, 60, 60, 60, 0};

/* Lookup table for T3412 timer used for periodic TAU. Unit is seconds.
 * Ref: GPRS Timer 3 IE in 3GPP TS 24.008 Table 10.5.163a/3GPP TS 24.008.
 */
static const u32_t t3412_lookup[8] = {600, 3600, 36000, 2, 30, 60,
				      1152000, 0};

#if defined(CONFIG_BSD_LIBRARY_TRACE_ENABLED)
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
/* Request modem to go to power saving mode */
static const char psm_req[] = "AT+CPSMS=1,,,\""CONFIG_LTE_PSM_REQ_RPTAU
			      "\",\""CONFIG_LTE_PSM_REQ_RAT"\"";

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

static const enum lte_lc_system_mode sys_mode_preferred =
	IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M)	?
		LTE_LC_SYSTEM_MODE_LTEM			:
	IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT)	?
		LTE_LC_SYSTEM_MODE_NBIOT		:
	IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_GPS)	?
		LTE_LC_SYSTEM_MODE_LTEM_GPS		:
	IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT_GPS)	?
		LTE_LC_SYSTEM_MODE_NBIOT_GPS		:
	LTE_LC_SYSTEM_MODE_NONE;

static const enum lte_lc_system_mode sys_mode_fallback =
#if IS_ENABLED(CONFIG_LTE_LC_USE_FALLBACK)
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

/* Parameters to be passed when using AT%XSYSTEMMMODE=<params> */
static const char *system_mode_params[] = {
	[LTE_LC_SYSTEM_MODE_LTEM]	= "1,0,0,0",
	[LTE_LC_SYSTEM_MODE_NBIOT]	= "0,1,0,0",
	[LTE_LC_SYSTEM_MODE_GPS]	= "0,0,1,0",
	[LTE_LC_SYSTEM_MODE_LTEM_GPS]	= "1,0,1,0",
	[LTE_LC_SYSTEM_MODE_NBIOT_GPS]	= "0,1,1,0",
};

#if defined(CONFIG_LWM2M_CARRIER) && !defined(CONFIG_GPS_USE_SIM)
#if defined(CONFIG_BOARD_NRF9160_PCA20035NS)
	const char *const lwm2m_ant_cfg[] = {
			"AT%XMAGPIO=1,1,1,7,1,746,803,2,698,748,"
			"2,1710,2200,3,824,894,4,880,960,5,791,849,"
			"7,1574,1577",
			"AT%XCOEX0=1,1,1570,1580"};
#elif defined(CONFIG_BOARD_NRF9160_PCA10090NS)
	const char *const lwm2m_ant_cfg[] = {
			"AT\%XMAGPIO=1,0,0,1,1,1574,1577",
			"AT\%XCOEX0=1,1,1570,1580"};
#endif
#endif

static struct k_sem link;

#if defined(CONFIG_LTE_PDP_CMD) && defined(CONFIG_LTE_PDP_CONTEXT)
static const char cgdcont[] = "AT+CGDCONT="CONFIG_LTE_PDP_CONTEXT;
#endif
#if defined(CONFIG_LTE_PDN_AUTH_CMD) && defined(CONFIG_LTE_PDN_AUTH)
static const char cgauth[] = "AT+CGAUTH="CONFIG_LTE_PDN_AUTH;
#endif
#if defined(CONFIG_LTE_LEGACY_PCO_MODE)
static const char legacy_pco[] = "AT%XEPCO=0";
#endif

enum lte_lc_notif_type {
	LTE_LC_NOTIF_CEREG,
	LTE_LC_NOTIF_CSCON,

	LTE_LC_NOTIF_COUNT,
};

static const char *relevant_at_notifs[] = {
	[LTE_LC_NOTIF_CEREG] = "+CEREG",
	[LTE_LC_NOTIF_CSCON] = "+CSCON",
};

BUILD_ASSERT(ARRAY_SIZE(relevant_at_notifs) == LTE_LC_NOTIF_COUNT);

static bool is_relevant_notif(const char *notif, enum lte_lc_notif_type *type)
{
	for (size_t i = 0; i < ARRAY_SIZE(relevant_at_notifs); i++) {
		if (strncmp(relevant_at_notifs[i], notif,
			    strlen(relevant_at_notifs[i])) == 0) {
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
	if(!is_relevant_notif(response, &notif_type)) {
		return;
	}

	switch (notif_type) {
	case LTE_LC_NOTIF_CEREG:
		LOG_DBG("+CEREG notification");

		err = parse_nw_reg_status(response,
					  &evt.nw_reg_status,
					  AT_CEREG_REG_STATUS_INDEX);
		if (err) {
			LOG_ERR("Can't parse network registration, error: %d",
				err);
			return;
		}

		evt.type = LTE_LC_EVT_NW_REG_STATUS;
		notify = true;

		if ((evt.nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME) ||
		    (evt.nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			k_sem_give(&link);
		}

		break;
	case LTE_LC_NOTIF_CSCON: {
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
	}
	default:
		LOG_ERR("Unrecognized notification type: %d", notif_type);
		break;
	}

	if (evt_handler && notify) {
		evt_handler(&evt);
	}
}

static int w_lte_lc_init(void)
{
	int err;

	if (is_initialized) {
		return -EALREADY;
	}

	err = at_notif_register_handler(NULL, at_handler);
	if (err) {
		LOG_ERR("Can't register AT handler, error: %d", err);
		return err;
	}

	err = lte_lc_system_mode_set(sys_mode_preferred);
	if (err) {
		LOG_ERR("Could not set system mode, error: %d", err);
		return err;
	}

#if defined(CONFIG_LWM2M_CARRIER) && !defined(CONFIG_GPS_USE_SIM) && \
    (defined(CONFIG_BOARD_NRF9160_PCA20035NS) || \
     defined(CONFIG_BOARD_NRF9160_PCA10090NS))
	/* Configuring MAGPIO/COEX, so that the correct antenna
	 * matching network is used for each LTE band and GPS.
	 */
	for (size_t i = 0; i < ARRAY_SIZE(lwm2m_ant_cfg); i++) {
		if (at_cmd_write(lwm2m_ant_cfg[i], NULL, 0, NULL) != 0) {
			return -EIO;
		}
	}
#endif

#if defined(CONFIG_LTE_EDRX_REQ)
	/* Request configured eDRX settings to save power */
	if (lte_lc_edrx_req(true) != 0) {
		return -EIO;
	}
#endif
#if defined(CONFIG_BSD_LIBRARY_TRACE_ENABLED)
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
		LOG_ERR("%s failed, error: %d", log_strdup(cscon), err);
		return err;
	}

	return 0;
}

static int w_lte_lc_connect(void)
{
	int err;
	enum lte_lc_system_mode current_network_mode = sys_mode_preferred;
	bool retry;

	k_sem_init(&link, 0, 1);

	do {
		retry = false;

		err = lte_lc_system_mode_set(current_network_mode);
		if (err) {
			return err;
		}

		err = lte_lc_normal();
		if (err) {
			return err;
		}

		err = k_sem_take(&link, K_SECONDS(CONFIG_LTE_NETWORK_TIMEOUT));
		if (err == -EAGAIN) {
			LOG_INF("Network connection attempt timed out");

			if (IS_ENABLED(CONFIG_LTE_NETWORK_USE_FALLBACK) &&
			    (current_network_mode == sys_mode_preferred)) {
				current_network_mode = sys_mode_fallback;
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

static int w_lte_lc_init_and_connect(struct device *unused)
{
	int ret;

	ret = w_lte_lc_init();
	if (ret) {
		return ret;
	}

	return w_lte_lc_connect();
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
	return w_lte_lc_connect();
}

/* lte lc Init and connect wrapper */
int lte_lc_init_and_connect(void)
{
	struct device *x = 0;

	int err = w_lte_lc_init_and_connect(x);

	return err;
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

int lte_lc_normal(void)
{
	if (at_cmd_write(normal, NULL, 0, NULL) != 0) {
		return -EIO;
	}

	return 0;
}

int lte_lc_psm_req(bool enable)
{
	if (at_cmd_write(enable ? psm_req : psm_disable,
			 NULL, 0, NULL) != 0) {
		return -EIO;
	}

	return 0;
}

int lte_lc_psm_get(int *tau, int *active_time)
{
	int err;
	struct at_param_list at_resp_list = {0};
	char buf[AT_CEREG_RESPONSE_MAX_LEN] = {0};
	char timer_str[9] = {0};
	char unit_str[4] = {0};
	size_t timer_str_len = sizeof(timer_str) - 1;
	size_t unit_str_len = sizeof(unit_str) - 1;
	size_t index;
	u32_t timer_unit, timer_value;

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

	/* Parse periodic TAU string */
	err = at_params_string_get(&at_resp_list,
				   AT_CEREG_TAU_INDEX,
				   timer_str,
				   &timer_str_len);
	if (err) {
		LOG_ERR("Could not get TAU, error: %d", err);
		goto parse_psm_clean_exit;
	}

	memcpy(unit_str, timer_str, unit_str_len);

	index = strtoul(unit_str, NULL, 2);
	if (index > (ARRAY_SIZE(t3412_lookup) - 1)) {
		LOG_ERR("Unable to parse periodic TAU string");
		err = -EINVAL;
		goto parse_psm_clean_exit;
	}

	timer_unit = t3412_lookup[index];
	timer_value = strtoul(timer_str + unit_str_len, NULL, 2);
	*tau = timer_unit ? timer_unit * timer_value : -1;

	/* Parse active time string */
	err = at_params_string_get(&at_resp_list,
				   AT_CEREG_ACTIVE_TIME_INDEX,
				   timer_str,
				   &timer_str_len);
	if (err) {
		LOG_ERR("Could not get TAU, error: %d", err);
		goto parse_psm_clean_exit;
	}

	memcpy(unit_str, timer_str, unit_str_len);

	index = strtoul(unit_str, NULL, 2);
	if (index > (ARRAY_SIZE(t3324_lookup) - 1)) {
		LOG_ERR("Unable to parse active time string");
		err = -EINVAL;
		goto parse_psm_clean_exit;
	}

	timer_unit = t3324_lookup[index];
	timer_value = strtoul(timer_str + unit_str_len, NULL, 2);
	*active_time = timer_unit ? timer_unit * timer_value : -1;

	LOG_DBG("TAU: %d sec, active time: %d sec\n", *tau, *active_time);

parse_psm_clean_exit:
	at_params_list_free(&at_resp_list);

	return err;
}

int lte_lc_edrx_req(bool enable)
{
	char edrx_req[25];
	int err, actt;
	enum lte_lc_system_mode mode;

	err = lte_lc_system_mode_get(&mode);
	if (err) {
		return err;
	}

	switch (mode) {
	case LTE_LC_SYSTEM_MODE_LTEM:
	case LTE_LC_SYSTEM_MODE_LTEM_GPS:
		actt = AT_CEDRXS_ACTT_WB;
		break;
	case LTE_LC_SYSTEM_MODE_NBIOT:
	case LTE_LC_SYSTEM_MODE_NBIOT_GPS:
		actt = AT_CEDRXS_ACTT_NB;
		break;
	default:
		LOG_ERR("Unknown system mode");
		LOG_ERR("Cannot request eDRX for unknown system mode");
		return -EOPNOTSUPP;
	}

	snprintf(edrx_req, sizeof(edrx_req),
		 "AT+CEDRXS=1,%d,\""CONFIG_LTE_EDRX_REQ_VALUE"\"", actt);

	err = at_cmd_write(enable ? edrx_req : edrx_disable, NULL, 0, NULL);
	if (err) {
		return err;
	}

	return 0;
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
		return - EFAULT;
	}

	LOG_DBG("Sending AT command to set system mode: %s", log_strdup(cmd));

	err = at_cmd_write(cmd, NULL, 0, NULL);
	if (err) {
		LOG_ERR("Could not send AT command, error: %d", err);
	}

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
	 */
	for (size_t i = 1; i < AT_XSYSTEMMODE_PARAMS_COUNT; i++) {
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
		break;
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
DEVICE_DECLARE(lte_link_control);
DEVICE_AND_API_INIT(lte_link_control, "LTE_LINK_CONTROL",
		    w_lte_lc_init_and_connect, NULL, NULL, APPLICATION,
		    CONFIG_APPLICATION_INIT_PRIORITY, NULL);
#endif /* CONFIG_LTE_AUTO_INIT_AND_CONNECT */
