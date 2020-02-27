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

/* Forward declarations */
static int parse_nw_reg_status(const char *at_response,
			       enum lte_lc_nw_reg_status *status,
			       size_t reg_status_index);
static bool response_is_valid(const char *response, size_t response_len,
			      const char *check);

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

#if defined(CONFIG_LTE_NETWORK_MODE_NBIOT)
/* Preferred network mode: Narrowband-IoT */
static const char nw_mode_preferred[] = "AT%XSYSTEMMODE=0,1,0,0";
/* Fallback network mode: LTE-M */
static const char nw_mode_fallback[] = "AT%XSYSTEMMODE=1,0,0,0";
#elif defined(CONFIG_LTE_NETWORK_MODE_NBIOT_GPS)
/* Preferred network mode: Narrowband-IoT and GPS */
static const char nw_mode_preferred[] = "AT%XSYSTEMMODE=0,1,1,0";
/* Fallback network mode: LTE-M and GPS*/
static const char nw_mode_fallback[] = "AT%XSYSTEMMODE=1,0,1,0";
#elif defined(CONFIG_LTE_NETWORK_MODE_LTE_M)
/* Preferred network mode: LTE-M */
static const char nw_mode_preferred[] = "AT%XSYSTEMMODE=1,0,0,0";
/* Fallback network mode: Narrowband-IoT */
static const char nw_mode_fallback[] = "AT%XSYSTEMMODE=0,1,0,0";
#elif defined(CONFIG_LTE_NETWORK_MODE_LTE_M_GPS)
/* Preferred network mode: LTE-M and GPS*/
static const char nw_mode_preferred[] = "AT%XSYSTEMMODE=1,0,1,0";
/* Fallback network mode: Narrowband-IoT and GPS */
static const char nw_mode_fallback[] = "AT%XSYSTEMMODE=0,1,1,0";
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

void at_handler(void *context, const char *response)
{
	ARG_UNUSED(context);

	int err;
	enum lte_lc_nw_reg_status status;

	if (response == NULL) {
		LOG_ERR("Response buffer is NULL-pointer");
		return;
	}

	err = parse_nw_reg_status(response, &status, AT_CEREG_REG_STATUS_INDEX);
	if (err) {
		LOG_ERR("Could not get network registration status");
		return;
	}

	if ((status == LTE_LC_NW_REG_REGISTERED_HOME) ||
	    (status == LTE_LC_NW_REG_REGISTERED_ROAMING)) {
		k_sem_give(&link);
	}
}

static int w_lte_lc_init(void)
{
	if (at_cmd_write(nw_mode_preferred, NULL, 0, NULL) != 0) {
		return -EIO;
	}

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

	return 0;
}

static int w_lte_lc_connect(void)
{
	int err, rc;
	const char *current_network_mode = nw_mode_preferred;
	bool retry;

	k_sem_init(&link, 0, 1);

	rc = at_notif_register_handler(NULL, at_handler);
	if (rc != 0) {
		LOG_ERR("Can't register handler rc=%d", rc);
		return rc;
	}

	do {
		retry = false;

		LOG_DBG("Network mode: %s", log_strdup(current_network_mode));

		if (at_cmd_write(current_network_mode, NULL, 0, NULL) != 0) {
			err = -EIO;
			goto exit;
		}

		if (at_cmd_write(normal, NULL, 0, NULL) != 0) {
			err = -EIO;
			goto exit;
		}

		err = k_sem_take(&link, K_SECONDS(CONFIG_LTE_NETWORK_TIMEOUT));
		if (err == -EAGAIN) {
			LOG_INF("Network connection attempt timed out");

			if (IS_ENABLED(CONFIG_LTE_NETWORK_USE_FALLBACK) &&
			    (current_network_mode == nw_mode_preferred)) {
				current_network_mode = nw_mode_fallback;
				retry = true;

				if (at_cmd_write(offline, NULL, 0, NULL) != 0) {
					err = -EIO;
					goto exit;
				}

				LOG_INF("Using fallback network mode");
			} else {
				err = -ETIMEDOUT;
			}
		}
	} while (retry);

exit:
	rc = at_notif_deregister_handler(NULL, at_handler);
	if (rc != 0) {
		LOG_ERR("Can't de-register handler rc=%d", rc);
	}

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
	int len, err = 0;
	u8_t mode_params[4] = {0};
	char cmd[sizeof(AT_XSYSTEMMODE_PROTO)];

	switch (mode) {
	case LTE_LC_SYSTEM_MODE_NONE:
		LOG_DBG("No system mode set");
		goto exit;
	case LTE_LC_SYSTEM_MODE_LTEM:
		mode_params[AT_XSYSTEMMODE_LTEM_INDEX] = 1;
		break;
	case LTE_LC_SYSTEM_MODE_LTEM_GPS:
		mode_params[AT_XSYSTEMMODE_LTEM_INDEX] = 1;
		mode_params[AT_XSYSTEMMODE_GPS_INDEX] = 1;
		break;
	case LTE_LC_SYSTEM_MODE_NBIOT:
		mode_params[AT_XSYSTEMMODE_NBIOT_INDEX] = 1;
		break;
	case LTE_LC_SYSTEM_MODE_NBIOT_GPS:
		mode_params[AT_XSYSTEMMODE_NBIOT_INDEX] = 1;
		mode_params[AT_XSYSTEMMODE_GPS_INDEX] = 1;
		break;
	case LTE_LC_SYSTEM_MODE_GPS:
		mode_params[AT_XSYSTEMMODE_GPS_INDEX] = 1;
		break;
	default:
		LOG_ERR("Invalid system mode requested");
		err = -EINVAL;
		goto exit;
	}

	/* System mode array is populated, proceed to compile the AT command */
	len = snprintf(cmd, sizeof(AT_XSYSTEMMODE_PROTO), AT_XSYSTEMMODE_PROTO,
		       mode_params[0], mode_params[1], mode_params[2],
		       mode_params[3]);

	LOG_DBG("Sending command: %s", log_strdup(cmd));

	err = at_cmd_write(cmd, NULL, 0, NULL);
	if (err) {
		LOG_ERR("Could not send AT command, err: %d", err);
	}

exit:
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
