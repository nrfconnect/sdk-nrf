/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
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

LOG_MODULE_REGISTER(lte_lc_helpers, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

/* Confirm valid system mode and set Paging Time Window multiplier.
 * Multiplier is 1.28 s for LTE-M, and 2.56 s for NB-IoT, derived from
 * Figure 10.5.5.32/3GPP TS 24.008.
 */
static int get_ptw_multiplier(enum lte_lc_lte_mode lte_mode, float *ptw_multiplier)
{
	switch (lte_mode) {
	case LTE_LC_LTE_MODE_LTEM:
		*ptw_multiplier = 1.28;
		break;
	case LTE_LC_LTE_MODE_NBIOT:
		*ptw_multiplier = 2.56;
		break;
	default:
		return -ENOTCONN;
	}

	return 0;
}


static int get_edrx_value(enum lte_lc_lte_mode lte_mode, uint8_t idx, float *edrx_value)
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

	switch (lte_mode) {
	case LTE_LC_LTE_MODE_LTEM:
		multiplier = edrx_lookup_ltem[idx];
		break;
	case LTE_LC_LTE_MODE_NBIOT:
		multiplier = edrx_lookup_nbiot[idx];
		break;
	default:
		return -ENOTCONN;
	}

	*edrx_value = multiplier == 0 ? 5.12 : multiplier * 10.24;

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
bool response_is_valid(const char *response, size_t response_len,
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

/* Get network registration status from CEREG response list.
 * Returns the (positive) registration value if it's found, otherwise a negative
 * error code.
 */
static int get_nw_reg_status(struct at_param_list *list, bool is_notif)
{
	int err, reg_status;
	size_t reg_status_index = is_notif ? AT_CEREG_REG_STATUS_INDEX :
					     AT_CEREG_READ_REG_STATUS_INDEX;

	err = at_params_int_get(list, reg_status_index, &reg_status);
	if (err) {
		return err;
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
		break;
	default:
		LOG_ERR("Invalid network registration status: %d", reg_status);
		return -EINVAL;
	}

	return reg_status;
}

int parse_edrx(const char *at_response, struct lte_lc_edrx_cfg *cfg)
{
	int err, tmp_int;
	uint8_t idx;
	struct at_param_list resp_list = {0};
	char tmp_buf[5];
	size_t len = sizeof(tmp_buf) - 1;
	float ptw_multiplier;
	enum lte_lc_lte_mode lte_mode = LTE_LC_LTE_MODE_NONE;

	if ((at_response == NULL) || (cfg == NULL)) {
		return -EINVAL;
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

	err = at_params_int_get(&resp_list, AT_CEDRXP_ACTT_INDEX, &tmp_int);
	if (err) {
		LOG_ERR("Failed to get LTE mode, error: %d", err);
		goto clean_exit;
	}

	/* The acces technology indicators 4 for LTE-M and 5 for NB-IoT are
	 * specified in 3GPP 27.007 Ch. 7.41.
	 */
	lte_mode = tmp_int == 4 ? LTE_LC_LTE_MODE_LTEM :
		   tmp_int == 5 ? LTE_LC_LTE_MODE_NBIOT :
				  LTE_LC_LTE_MODE_NONE;

	/* Confirm valid system mode and set Paging Time Window multiplier.
	 * Multiplier is 1.28 s for LTE-M, and 2.56 s for NB-IoT, derived from
	 * figure 10.5.5.32/3GPP TS 24.008.
	 */
	err = get_ptw_multiplier(lte_mode, &ptw_multiplier);
	if (err) {
		LOG_WRN("Active LTE mode could not be determined");
		goto clean_exit;
	}

	err = get_edrx_value(lte_mode, idx, &cfg->edrx);
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



int parse_psm(struct at_param_list *at_params,
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


/**@brief Parses an AT command response, and returns the current RRC mode.
 *
 * @param at_response Pointer to buffer with AT response.
 * @param mode Pointer to where the RRC mode is stored.
 * @param mode_index Parameter index for mode.
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int parse_rrc_mode(const char *at_response,
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

int parse_cereg(const char *at_response,
		bool is_notif,
		enum lte_lc_nw_reg_status *reg_status,
		struct lte_lc_cell *cell,
		enum lte_lc_lte_mode *lte_mode,
		struct lte_lc_psm_cfg *psm_cfg)
{
	int err, status;
	struct at_param_list resp_list;
	char str_buf[10];
	char  response_prefix[sizeof(AT_CEREG_RESPONSE_PREFIX)] = {0};
	size_t response_prefix_len = sizeof(response_prefix);
	size_t len = sizeof(str_buf) - 1;

	err = at_params_list_init(&resp_list, AT_CEREG_PARAMS_COUNT_MAX);
	if (err) {
		LOG_ERR("Could not init AT params list, error: %d", err);
		return err;
	}

	/* Parse CEREG response and populate AT parameter list */
	err = at_parser_params_from_str(at_response,
					NULL,
					&resp_list);
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
		LOG_DBG("Not a valid CEREG response");
		goto clean_exit;
	}

	/* Get network registration status */
	status = get_nw_reg_status(&resp_list, is_notif);
	if (status < 0) {
		LOG_ERR("Could not get registration status, error: %d", status);
		err = status;
		goto clean_exit;
	}

	if (reg_status) {
		*reg_status = status;
	}

	LOG_DBG("Network registration status: %d", *reg_status);

	if (cell && (status != LTE_LC_NW_REG_UICC_FAIL) &&
	    (at_params_valid_count_get(&resp_list) > AT_CEREG_CELL_ID_INDEX)) {
		/* Parse tracking area code */
		err = at_params_string_get(
				&resp_list,
				is_notif ? AT_CEREG_TAC_INDEX :
					   AT_CEREG_READ_TAC_INDEX,
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
				is_notif ? AT_CEREG_CELL_ID_INDEX :
					   AT_CEREG_READ_CELL_ID_INDEX,
				str_buf, &len);
		if (err) {
			LOG_ERR("Could not get cell ID, error: %d", err);
			goto clean_exit;
		}

		str_buf[len] = '\0';
		cell->id = strtoul(str_buf, NULL, 16);
	} else if (cell) {
		cell->tac = UINT32_MAX;
		cell->id = UINT32_MAX;
	}

	if (lte_mode) {
		int mode;

		/* Get currently active LTE mode. */
		err = at_params_int_get(&resp_list,
				is_notif ? AT_CEREG_ACT_INDEX :
					   AT_CEREG_READ_ACT_INDEX,
				&mode);
		if (err) {
			LOG_DBG("LTE mode not found, error code: %d", err);
			*lte_mode = LTE_LC_LTE_MODE_NONE;

			/* This is not an error that should be returned, as it's
			 * expected in some situations that LTE mode is not
			 * available.
			 */
			err = 0;
		} else {
			*lte_mode = mode;

			LOG_DBG("LTE mode: %d", *lte_mode);
		}
	}

	/* Parse PSM configuration only when registered */
	if (psm_cfg && ((status == LTE_LC_NW_REG_REGISTERED_HOME) ||
	    (status == LTE_LC_NW_REG_REGISTERED_ROAMING)) &&
	     (at_params_valid_count_get(&resp_list) > AT_CEREG_TAU_INDEX)) {
		err = parse_psm(&resp_list, is_notif, psm_cfg);
		if (err) {
			LOG_ERR("Failed to parse PSM configuration, error: %d",
				err);
			goto clean_exit;
		}
	} else if (psm_cfg) {
		/* When device is not registered, PSM valies are invalid */
		psm_cfg->tau = -1;
		psm_cfg->active_time = -1;
	}

clean_exit:
	at_params_list_free(&resp_list);

	return err;
}
