/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
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
#include <modem/lte_lc.h>
#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>
#include <zephyr/logging/log.h>

#include "lte_lc_helpers.h"

LOG_MODULE_REGISTER(lte_lc_helpers, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

static K_MUTEX_DEFINE(list_mtx);

/**@brief List element for event handler list. */
struct event_handler {
	sys_snode_t          node;
	lte_lc_evt_handler_t handler;
};

static sys_slist_t handler_list;

/**
 * @brief Find the handler from the event handler list.
 *
 * @return The node or NULL if not found and its previous node in @p prev_out.
 */
static struct event_handler *event_handler_list_find_node(struct event_handler **prev_out,
							  lte_lc_evt_handler_t handler)
{
	struct event_handler *prev = NULL, *curr;

	SYS_SLIST_FOR_EACH_CONTAINER(&handler_list, curr, node) {
		if (curr->handler == handler) {
			*prev_out = prev;
			return curr;
		}
		prev = curr;
	}
	return NULL;
}

/**@brief Test if the handler list is empty. */
bool event_handler_list_is_empty(void)
{
	return sys_slist_is_empty(&handler_list);
}

/**@brief Add the handler in the event handler list if not already present. */
int event_handler_list_append_handler(lte_lc_evt_handler_t handler)
{
	struct event_handler *to_ins;

	k_mutex_lock(&list_mtx, K_FOREVER);

	/* Check if handler is already registered. */
	if (event_handler_list_find_node(&to_ins, handler) != NULL) {
		LOG_DBG("Handler already registered. Nothing to do");
		k_mutex_unlock(&list_mtx);
		return 0;
	}

	/* Allocate memory and fill. */
	to_ins = (struct event_handler *)k_malloc(sizeof(struct event_handler));
	if (to_ins == NULL) {
		k_mutex_unlock(&list_mtx);
		return -ENOBUFS;
	}
	memset(to_ins, 0, sizeof(struct event_handler));
	to_ins->handler = handler;

	/* Insert handler in the list. */
	sys_slist_append(&handler_list, &to_ins->node);
	k_mutex_unlock(&list_mtx);
	return 0;
}

/**@brief Remove the handler from the event handler list if registered. */
int event_handler_list_remove_handler(lte_lc_evt_handler_t handler)
{
	struct event_handler *curr, *prev = NULL;

	k_mutex_lock(&list_mtx, K_FOREVER);

	/* Check if the handler is registered before removing it. */
	curr = event_handler_list_find_node(&prev, handler);
	if (curr == NULL) {
		LOG_WRN("Handler not registered. Nothing to do");
		k_mutex_unlock(&list_mtx);
		return 0;
	}

	/* Remove the handler from the list. */
	sys_slist_remove(&handler_list, &prev->node, &curr->node);
	k_free(curr);

	k_mutex_unlock(&list_mtx);
	return 0;
}

/**@brief dispatch events. */
void event_handler_list_dispatch(const struct lte_lc_evt *const evt)
{
	struct event_handler *curr, *tmp;

	if (event_handler_list_is_empty()) {
		return;
	}

	k_mutex_lock(&list_mtx, K_FOREVER);

	/* Dispatch events to all registered handlers */
	LOG_DBG("Dispatching events:");
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&handler_list, curr, tmp, node) {
		LOG_DBG(" - handler=0x%08X", (uint32_t)curr->handler);
		curr->handler(evt);
	}
	LOG_DBG("Done");

	k_mutex_unlock(&list_mtx);
}


/* Converts integer on string format to integer type.
 * Returns zero on success, otherwise negative error on failure.
 */
static int string_param_to_int(struct at_param_list *resp_list,
			       size_t idx, int *output, int base)
{
	int err;
	char str_buf[16];
	size_t len = sizeof(str_buf);

	err = at_params_string_get(resp_list, idx, str_buf, &len);
	if (err) {
		return err;
	}

	str_buf[len] = '\0';

	if (string_to_int(str_buf, base, output)) {
		return -ENODATA;
	}

	return 0;
}

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

/* Counts the frequency of a character in a null-terminated string. */
static uint32_t get_char_frequency(const char *str, char c)
{
	uint32_t count = 0;

	do {
		if (*str == c) {
			count++;
		}
	} while (*(str++) != '\0');

	return count;
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

int string_to_int(const char *str_buf, int base, int *output)
{
	int temp;
	char *end_ptr;

	errno = 0;
	temp = strtol(str_buf, &end_ptr, base);

	if (end_ptr == str_buf || *end_ptr != '\0' ||
	    ((temp == LONG_MAX || temp == LONG_MIN) && errno == ERANGE)) {
		return -ENODATA;
	}

	*output = temp;

	return 0;
}

int parse_edrx(const char *at_response, struct lte_lc_edrx_cfg *cfg)
{
	int err, tmp_int;
	uint8_t idx;
	struct at_param_list resp_list = {0};
	char tmp_buf[5];
	size_t len = sizeof(tmp_buf) - 1;
	float ptw_multiplier;

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

	err = at_params_int_get(&resp_list, AT_CEDRXP_ACTT_INDEX, &tmp_int);
	if (err) {
		LOG_ERR("Failed to get LTE mode, error: %d", err);
		goto clean_exit;
	}

	/* The access technology indicators 4 for LTE-M and 5 for NB-IoT are
	 * specified in 3GPP 27.007 Ch. 7.41.
	 * 0 indicates that the current cell doesn't use eDRX.
	 * Any other value is not expected, and we use 0xFFFFFFFF to represent those.
	 */
	cfg->mode = tmp_int == 0 ? LTE_LC_LTE_MODE_NONE :
		    tmp_int == 4 ? LTE_LC_LTE_MODE_LTEM :
		    tmp_int == 5 ? LTE_LC_LTE_MODE_NBIOT :
				   0xFFFFFFFF; /* Intentionally illegal value */

	/* Check for the case where the current cell does not use eDRX */
	if (cfg->mode == LTE_LC_LTE_MODE_NONE) {
		cfg->edrx = 0;
		cfg->ptw = 0;

		return 0;
	} else if (cfg->mode == 0xFFFFFFFF) {
		return -ENODATA;
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

	/* Confirm valid system mode and set Paging Time Window multiplier.
	 * Multiplier is 1.28 s for LTE-M, and 2.56 s for NB-IoT, derived from
	 * figure 10.5.5.32/3GPP TS 24.008.
	 */
	err = get_ptw_multiplier(cfg->mode, &ptw_multiplier);
	if (err) {
		LOG_WRN("Active LTE mode could not be determined");
		goto clean_exit;
	}

	err = get_edrx_value(cfg->mode, idx, &cfg->edrx);
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

	LOG_DBG("eDRX value for %s: %d.%02d, PTW: %d.%02d",
		(cfg->mode == LTE_LC_LTE_MODE_LTEM) ? "LTE-M" : "NB-IoT",
		(int)cfg->edrx,
		(int)(100 * (cfg->edrx - (int)cfg->edrx)),
		(int)cfg->ptw,
		(int)(100 * (cfg->ptw - (int)cfg->ptw)));

clean_exit:
	at_params_list_free(&resp_list);

	return err;
}

int parse_psm(const char *active_time_str, const char *tau_ext_str,
	      const char *tau_legacy_str, struct lte_lc_psm_cfg *psm_cfg)
{
	char unit_str[4] = {0};
	size_t unit_str_len = sizeof(unit_str) - 1;
	size_t lut_idx;
	uint32_t timer_unit, timer_value;

	/* Lookup table for T3324 timer used for PSM active time in seconds.
	 * Ref: GPRS Timer 2 IE in 3GPP TS 24.008 Table 10.5.163/3GPP TS 24.008.
	 */
	static const uint32_t t3324_lookup[8] = {2, 60, 360, 60, 60, 60, 60, 0};

	/* Lookup table for T3412-extended timer used for periodic TAU. Unit is seconds.
	 * Ref: GPRS Timer 3 in 3GPP TS 24.008 Table 10.5.163a/3GPP TS 24.008.
	 */
	static const uint32_t t3412_ext_lookup[8] = {600, 3600, 36000, 2, 30, 60, 1152000, 0};

	/* Lookup table for T3412 (legacy) timer used for periodic TAU. Unit is seconds.
	 * Ref: GPRS Timer in 3GPP TS 24.008 Table 10.5.172/3GPP TS 24.008.
	 */
	static const uint32_t t3412_lookup[8] = {2, 60, 360, 60, 60, 60, 60, 0};

	if ((strlen(active_time_str) != 8) || (strlen(tau_ext_str) != 8)) {
		return -EINVAL;
	}

	/* Parse T3412-extended (periodic TAU) timer */
	memcpy(unit_str, tau_ext_str, unit_str_len);

	lut_idx = strtoul(unit_str, NULL, 2);
	if (lut_idx > (ARRAY_SIZE(t3412_ext_lookup) - 1)) {
		LOG_ERR("Unable to parse periodic TAU string (T3412 extended)");
		return -EINVAL;
	}

	timer_unit = t3412_ext_lookup[lut_idx];
	timer_value = strtoul(tau_ext_str + unit_str_len, NULL, 2);
	psm_cfg->tau = timer_unit ? timer_unit * timer_value : -1;

	/* If T3412-extended is disabled, there's a chance that the network
	 * only reports the T3412 (legacy) timer. We therefore needs to check
	 * that as well. The legacy timer is only reported by modem fe >= 1.2.0.
	 */
	if ((psm_cfg->tau == -1) && tau_legacy_str && (strlen(tau_legacy_str) == 8)) {
		memcpy(unit_str, tau_legacy_str, unit_str_len);

		lut_idx = strtoul(unit_str, NULL, 2);
		if (lut_idx > (ARRAY_SIZE(t3412_lookup) - 1)) {
			LOG_ERR("Unable to parse periodic TAU string (T3412)");
			return -EINVAL;
		}

		timer_unit = t3412_lookup[lut_idx];
		timer_value = strtoul(tau_legacy_str + unit_str_len, NULL, 2);
		psm_cfg->tau = timer_unit ? timer_unit * timer_value : -1;
	}

	/* Parse active time */
	memcpy(unit_str, active_time_str, unit_str_len);

	lut_idx = strtoul(unit_str, NULL, 2);
	if (lut_idx > (ARRAY_SIZE(t3324_lookup) - 1)) {
		LOG_ERR("Unable to parse active time string");
		return -EINVAL;
	}

	timer_unit = t3324_lookup[lut_idx];
	timer_value = strtoul(active_time_str + unit_str_len, NULL, 2);
	psm_cfg->active_time = timer_unit ? timer_unit * timer_value : -1;

	LOG_DBG("TAU: %d sec, active time: %d sec",
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
		enum lte_lc_lte_mode *lte_mode)
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

		LOG_DBG("Network registration status: %d", *reg_status);
	}


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
		cell->id = LTE_LC_CELL_EUTRAN_ID_INVALID;
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

clean_exit:
	at_params_list_free(&resp_list);

	return err;
}

int parse_xt3412(const char *at_response, uint64_t *time)
{
	int err;
	struct at_param_list resp_list = {0};

	if (time == NULL || at_response == NULL) {
		return -EINVAL;
	}

	err = at_params_list_init(&resp_list, AT_XT3412_PARAMS_COUNT_MAX);
	if (err) {
		LOG_ERR("Could not init AT params list, error: %d", err);
		return err;
	}

	/* Parse XT3412 response and populate AT parameter list */
	err = at_parser_params_from_str(at_response, NULL, &resp_list);
	if (err) {
		LOG_ERR("Could not parse %%XT3412 response, error: %d", err);
		goto clean_exit;
	}

	/* Get the remaining time of T3412 from the response */
	err = at_params_int64_get(&resp_list, AT_XT3412_TIME_INDEX, time);
	if (err) {
		LOG_ERR("Could not get time until next TAU, error: %d", err);
		goto clean_exit;
	}

	if ((*time > T3412_MAX) || *time < 0) {
		LOG_WRN("Parsed time parameter not within valid range");
		err = -EINVAL;
	}

clean_exit:
	at_params_list_free(&resp_list);
	return err;
}

uint32_t neighborcell_count_get(const char *at_response)
{
	uint32_t comma_count, ncell_elements, ncell_count;

	if (at_response == NULL) {
		LOG_ERR("at_response is NULL, can't get cell count");
		return 0;
	}

	comma_count = get_char_frequency(at_response, ',');
	if (comma_count < AT_NCELLMEAS_PRE_NCELLS_PARAMS_COUNT) {
		return 0;
	}

	/* Add one, as there's no comma after the last element. */
	ncell_elements = comma_count - (AT_NCELLMEAS_PRE_NCELLS_PARAMS_COUNT - 1) + 1;
	ncell_count = ncell_elements / AT_NCELLMEAS_N_PARAMS_COUNT;

	return ncell_count;
}

/* Parse NCELLMEAS notification and put information into struct lte_lc_cells_info.
 *
 * Returns 0 on successful cell measurements and population of struct.
 *	     The current cell information is valid if the current cell ID is
 *	     not set to LTE_LC_CELL_EUTRAN_ID_INVALID.
 *	     The ncells_count indicates how many neighbor cells were parsed
 *	     into the neighbor_cells array.
 * Returns 1 on measurement failure
 * Returns -E2BIG if not all cells were parsed due to memory limitations
 * Returns otherwise a negative error code.
 */
int parse_ncellmeas(const char *at_response, struct lte_lc_cells_info *cells)
{
	int err, status, tmp, len;
	struct at_param_list resp_list;
	char  response_prefix[sizeof(AT_NCELLMEAS_RESPONSE_PREFIX)] = {0};
	size_t response_prefix_len = sizeof(response_prefix);
	char tmp_str[7];
	bool incomplete = false;
	/* Count the actual number of parameters in the AT response before
	 * allocating heap for it. This may save quite a bit of heap as the
	 * worst case scenario is 96 elements.
	 * 3 is added to account for the parameters that do not have a trailng
	 * comma.
	 */
	size_t param_count = get_char_frequency(at_response, ',') + 3;

	cells->ncells_count = 0;
	cells->current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;

	err = at_params_list_init(&resp_list, param_count);
	if (err) {
		LOG_ERR("Could not init AT params list, error: %d", err);
		return err;
	}

	err = at_parser_params_from_str(at_response,
					NULL,
					&resp_list);
	if (err && err != -E2BIG) {
		LOG_ERR("Could not parse AT%%NCELLMEAS response, error: %d", err);
		goto clean_exit;
	} else if (err == -E2BIG) {
		incomplete = true;
	}

	err = at_params_string_get(&resp_list,
				   AT_RESPONSE_PREFIX_INDEX,
				   response_prefix,
				   &response_prefix_len);
	if (err) {
		LOG_ERR("Could not get response prefix, error: %d", err);
		goto clean_exit;
	}

	if (!response_is_valid(response_prefix, response_prefix_len,
			       AT_NCELLMEAS_RESPONSE_PREFIX)) {
		/* The unsolicited response is not a NCELLMEAS response, ignore it. */
		LOG_DBG("Not a valid NCELLMEAS response");
		goto clean_exit;
	}

	/* Status code. */
	err = at_params_int_get(&resp_list, AT_NCELLMEAS_STATUS_INDEX, &status);
	if (err) {
		goto clean_exit;
	}

	if (status != AT_NCELLMEAS_STATUS_VALUE_SUCCESS) {
		err = 1;
		goto clean_exit;
	}

	/* Current cell ID. */
	err = string_param_to_int(&resp_list, AT_NCELLMEAS_CELL_ID_INDEX, &tmp, 16);
	if (err) {
		goto clean_exit;
	}

	if (tmp > LTE_LC_CELL_EUTRAN_ID_MAX) {
		tmp = LTE_LC_CELL_EUTRAN_ID_INVALID;
	}
	cells->current_cell.id = tmp;

	/* PLMN */
	len = sizeof(tmp_str);

	err = at_params_string_get(&resp_list, AT_NCELLMEAS_PLMN_INDEX,
				   tmp_str, &len);
	if (err) {
		goto clean_exit;
	}

	tmp_str[len] = '\0';

	/* Read MNC and store as integer. The MNC starts as the fourth character
	 * in the string, following three characters long MCC.
	 */
	err = string_to_int(&tmp_str[3], 10, &cells->current_cell.mnc);
	if (err) {
		goto clean_exit;
	}

	/* Null-terminated MCC, read and store it. */
	tmp_str[3] = '\0';

	err = string_to_int(tmp_str, 10, &cells->current_cell.mcc);
	if (err) {
		goto clean_exit;
	}

	/* Tracking area code. */
	err = string_param_to_int(&resp_list, AT_NCELLMEAS_TAC_INDEX, &tmp, 16);
	if (err) {
		goto clean_exit;
	}

	cells->current_cell.tac = tmp;

	/* Timing advance */
	err = at_params_int_get(&resp_list, AT_NCELLMEAS_TIMING_ADV_INDEX,
				&tmp);
	if (err) {
		goto clean_exit;
	}

	cells->current_cell.timing_advance = tmp;

	/* EARFCN */
	err = at_params_int_get(&resp_list, AT_NCELLMEAS_EARFCN_INDEX,
				&cells->current_cell.earfcn);
	if (err) {
		goto clean_exit;
	}

	/* Physical cell ID. */
	err = at_params_short_get(&resp_list, AT_NCELLMEAS_PHYS_CELL_ID_INDEX,
				&cells->current_cell.phys_cell_id);
	if (err) {
		goto clean_exit;
	}

	/* RSRP */
	err = at_params_int_get(&resp_list, AT_NCELLMEAS_RSRP_INDEX, &tmp);
	if (err) {
		goto clean_exit;
	}

	cells->current_cell.rsrp = tmp;

	/* RSRQ */
	err = at_params_int_get(&resp_list, AT_NCELLMEAS_RSRQ_INDEX, &tmp);
	if (err) {
		goto clean_exit;
	}

	cells->current_cell.rsrq = tmp;

	/* Measurement time. */
	err = at_params_int64_get(&resp_list, AT_NCELLMEAS_MEASUREMENT_TIME_INDEX,
				  &cells->current_cell.measurement_time);
	if (err) {
		goto clean_exit;
	}

	/* Neighbor cell count. */
	cells->ncells_count = neighborcell_count_get(at_response);

	/* Starting from modem firmware v1.3.1, timing advance measurement time
	 * information is added as the last parameter in the response.
	 */
	size_t ta_meas_time_index = AT_NCELLMEAS_PRE_NCELLS_PARAMS_COUNT +
			cells->ncells_count * AT_NCELLMEAS_N_PARAMS_COUNT;

	if (at_params_valid_count_get(&resp_list) > ta_meas_time_index) {
		err = at_params_int64_get(&resp_list, ta_meas_time_index,
					  &cells->current_cell.timing_advance_meas_time);
		if (err) {
			goto clean_exit;
		}
	} else {
		cells->current_cell.timing_advance_meas_time = 0;
	}

	if ((cells->ncells_count == 0) || (cells->neighbor_cells == NULL)) {
		goto clean_exit;
	}

	/* Neighboring cells. */
	for (size_t i = 0; i < cells->ncells_count; i++) {
		size_t start_idx = AT_NCELLMEAS_PRE_NCELLS_PARAMS_COUNT +
				   i * AT_NCELLMEAS_N_PARAMS_COUNT;

		/* EARFCN */
		err = at_params_int_get(&resp_list,
					start_idx + AT_NCELLMEAS_N_EARFCN_INDEX,
					&cells->neighbor_cells[i].earfcn);
		if (err) {
			goto clean_exit;
		}

		/* Physical cell ID. */
		err = at_params_short_get(&resp_list,
					  start_idx + AT_NCELLMEAS_N_PHYS_CELL_ID_INDEX,
					  &cells->neighbor_cells[i].phys_cell_id);
		if (err) {
			goto clean_exit;
		}

		/* RSRP */
		err = at_params_int_get(&resp_list,
					start_idx + AT_NCELLMEAS_N_RSRP_INDEX,
					&tmp);
		if (err) {
			goto clean_exit;
		}

		cells->neighbor_cells[i].rsrp = tmp;

		/* RSRQ */
		err = at_params_int_get(&resp_list,
					start_idx + AT_NCELLMEAS_N_RSRQ_INDEX,
					&tmp);
		if (err) {
			goto clean_exit;
		}

		cells->neighbor_cells[i].rsrq = tmp;

		/* Time difference. */
		err = at_params_int_get(&resp_list,
					start_idx + AT_NCELLMEAS_N_TIME_DIFF_INDEX,
					&cells->neighbor_cells[i].time_diff);
		if (err) {
			goto clean_exit;
		}
	}

	if (incomplete) {
		err = -E2BIG;
	}

clean_exit:
	at_params_list_free(&resp_list);

	return err;
}

int parse_ncellmeas_gci(struct lte_lc_ncellmeas_params *params,
	const char *at_response, struct lte_lc_cells_info *cells)
{
	struct at_param_list resp_list;
	struct lte_lc_ncell *ncells = NULL;
	int err, status, tmp_int, len;
	int16_t tmp_short;
	char response_prefix[sizeof(AT_NCELLMEAS_RESPONSE_PREFIX)] = {0};
	size_t response_prefix_len = sizeof(response_prefix);
	char tmp_str[7];
	bool incomplete = false;
	int curr_index;
	size_t i = 0, j = 0, k = 0;

	/* Count the actual number of parameters in the AT response before
	 * allocating heap for it. This may save quite a bit of heap as the
	 * worst case scenario is 96 elements.
	 * 3 is added to account for the parameters that do not have a trailing
	 * comma.
	 */
	size_t param_count = get_char_frequency(at_response, ',') + 3;

	/* Fill the defaults */
	cells->gci_cells_count = 0;
	cells->ncells_count = 0;
	cells->current_cell.id = LTE_LC_CELL_EUTRAN_ID_INVALID;

	for (i = 0; i < params->gci_count; i++) {
		cells->gci_cells[i].id = LTE_LC_CELL_EUTRAN_ID_INVALID;
		cells->gci_cells[i].timing_advance = LTE_LC_CELL_TIMING_ADVANCE_INVALID;
	}

	/*
	 * Response format for GCI search types:
	 * High level:
	 * status[,
	 *	GCI_cell_info1,neighbor_count1[,neighbor_cell1_1,neighbor_cell1_2...],
	 *	GCI_cell_info2,neighbor_count2[,neighbor_cell2_1,neighbor_cell2_2...]...]
	 *
	 * Detailed:
	 * %NCELLMEAS: status
	 * [,<cell_id>,<plmn>,<tac>,<ta>,<ta_meas_time>,<earfcn>,<phys_cell_id>,<rsrp>,<rsrq>,
	 *		<meas_time>,<serving>,<neighbor_count>
	 *	[,<n_earfcn1>,<n_phys_cell_id1>,<n_rsrp1>,<n_rsrq1>,<time_diff1>]
	 *	[,<n_earfcn2>,<n_phys_cell_id2>,<n_rsrp2>,<n_rsrq2>,<time_diff2>]...],
	 *  <cell_id>,<plmn>,<tac>,<ta>,<ta_meas_time>,<earfcn>,<phys_cell_id>,<rsrp>,<rsrq>,
	 *		<meas_time>,<serving>,<neighbor_count>
	 *	[,<n_earfcn1>,<n_phys_cell_id1>,<n_rsrp1>,<n_rsrq1>,<time_diff1>]
	 *	[,<n_earfcn2>,<n_phys_cell_id2>,<n_rsrp2>,<n_rsrq2>,<time_diff2>]...]...
	 */

	err = at_params_list_init(&resp_list, param_count);
	if (err) {
		LOG_ERR("Could not init AT params list for cell_list, error: %d", err);
		goto clean_exit;
	}

	err = at_parser_params_from_str(at_response, NULL, &resp_list);
	if (err && err != -E2BIG) {
		LOG_ERR("Could not parse AT%%NCELLMEAS response, error: %d", err);
		goto clean_exit;
	} else if (err == -E2BIG) {
		/* Returns -E2BIG if the buffers set by CONFIG_LTE_NEIGHBOR_CELLS_MAX
		 * are too small for the modem response. The associated data is still valid,
		 * but not complete.
		 */
		incomplete = true;
		LOG_WRN("E2BIG was returned, continue. param_count %d, err %d, str %s",
			param_count, err, at_response);
	}

	err = at_params_string_get(&resp_list,
				   AT_RESPONSE_PREFIX_INDEX,
				   response_prefix,
				   &response_prefix_len);
	if (err) {
		LOG_ERR("Could not get response prefix, error: %d", err);
		goto clean_exit;
	}

	if (!response_is_valid(response_prefix, response_prefix_len,
			       AT_NCELLMEAS_RESPONSE_PREFIX)) {
		/* The unsolicited response is not a NCELLMEAS response, ignore it. */
		LOG_ERR("Not a valid NCELLMEAS response");
		goto clean_exit;
	}

	/* Status code. */
	curr_index = AT_NCELLMEAS_STATUS_INDEX;
	err = at_params_int_get(&resp_list, curr_index, &status);
	if (err) {
		LOG_DBG("Cannot parse NCELLMEAS status");
		goto clean_exit;
	}

	if (status == AT_NCELLMEAS_STATUS_VALUE_FAIL) {
		err = 1;
		LOG_DBG("NCELLMEAS status %d", status);
		goto clean_exit;
	} else if (status == AT_NCELLMEAS_STATUS_VALUE_INCOMPLETE) {
		LOG_WRN("NCELLMEAS measurements interrupted; results incomplete");
	}

	/* Go through the cells. */
	for (i = 0; curr_index < (param_count - (AT_NCELLMEAS_GCI_CELL_PARAMS_COUNT + 1)) &&
			i < params->gci_count; i++) {
		struct lte_lc_cell parsed_cell;
		bool is_serving_cell;
		uint8_t parsed_ncells_count;

		/* <cell_id>  */
		curr_index++;
		err = string_param_to_int(&resp_list, curr_index, &tmp_int, 16);
		if (err) {
			LOG_ERR("Could not parse cell_id, index %d, i %d error: %d",
				curr_index, i, err);
			goto clean_exit;
		}

		if (tmp_int > LTE_LC_CELL_EUTRAN_ID_MAX) {
			LOG_WRN("cell_id = %d which is > LTE_LC_CELL_EUTRAN_ID_MAX; "
				"marking invalid", tmp_int);
			tmp_int = LTE_LC_CELL_EUTRAN_ID_INVALID;
		}
		parsed_cell.id = tmp_int;

		/* <plmn> */
		len = sizeof(tmp_str);

		curr_index++;
		err = at_params_string_get(&resp_list, curr_index, tmp_str, &len);
		if (err) {
			LOG_ERR("Could not parse plmn, error: %d", err);
			goto clean_exit;
		}
		/* A successful call to `at_params_string_get` guarantees `len` to be set to
		 * a value lower than the totalt size of `tmp_str`.
		 */
		tmp_str[len] = '\0';

		/* Read MNC and store as integer. The MNC starts as the fourth character
		 * in the string, following three characters long MCC.
		 */
		err = string_to_int(&tmp_str[3], 10, &parsed_cell.mnc);
		if (err) {
			LOG_ERR("string_to_int, error: %d", err);
			goto clean_exit;
		}

		/* Null-terminated MCC, read and store it. */
		tmp_str[3] = '\0';

		err = string_to_int(tmp_str, 10, &parsed_cell.mcc);
		if (err) {
			LOG_ERR("string_to_int, error: %d", err);
			goto clean_exit;
		}

		/* <tac> */
		curr_index++;
		err = string_param_to_int(&resp_list, curr_index, &tmp_int, 16);
		if (err) {
			LOG_ERR("Could not parse tracking_area_code in i %d, error: %d", i, err);
			goto clean_exit;
		}
		parsed_cell.tac = tmp_int;

		/* <ta> */
		curr_index++;
		err = at_params_int_get(&resp_list, curr_index, &tmp_int);
		if (err) {
			LOG_ERR("Could not parse timing_advance, error: %d", err);
			goto clean_exit;
		}
		parsed_cell.timing_advance = tmp_int;

		/* <ta_meas_time> */
		curr_index++;
		err = at_params_int64_get(&resp_list, curr_index,
					  &parsed_cell.timing_advance_meas_time);
		if (err) {
			LOG_ERR("Could not parse timing_advance_meas_time, error: %d", err);
			goto clean_exit;
		}

		/* <earfcn> */
		curr_index++;
		err = at_params_int_get(&resp_list, curr_index, &parsed_cell.earfcn);
		if (err) {
			LOG_ERR("Could not parse earfcn, error: %d", err);
			goto clean_exit;
		}

		/* <phys_cell_id> */
		curr_index++;
		err = at_params_short_get(&resp_list, curr_index, &parsed_cell.phys_cell_id);
		if (err) {
			LOG_ERR("Could not parse phys_cell_id, error: %d", err);
			goto clean_exit;
		}

		/* <rsrp> */
		curr_index++;
		err = at_params_short_get(&resp_list, curr_index, &parsed_cell.rsrp);
		if (err) {
			LOG_ERR("Could not parse rsrp, error: %d", err);
			goto clean_exit;
		}

		/* <rsrq> */
		curr_index++;
		err = at_params_short_get(&resp_list, curr_index, &parsed_cell.rsrq);
		if (err) {
			LOG_ERR("Could not parse rsrq, error: %d", err);
			goto clean_exit;
		}

		/* <meas_time> */
		curr_index++;
		err = at_params_int64_get(&resp_list, curr_index, &parsed_cell.measurement_time);
		if (err) {
			LOG_ERR("Could not parse meas_time, error: %d", err);
			goto clean_exit;
		}

		/* <serving> */
		curr_index++;
		err = at_params_short_get(&resp_list, curr_index, &tmp_short);
		if (err) {
			LOG_ERR("Could not parse serving, error: %d", err);
			goto clean_exit;
		}
		is_serving_cell = tmp_short;

		/* <neighbor_count> */
		curr_index++;
		err = at_params_short_get(&resp_list, curr_index, &tmp_short);
		if (err) {
			LOG_ERR("Could not parse neighbor_count, error: %d", err);
			goto clean_exit;
		}
		parsed_ncells_count = tmp_short;

		if (is_serving_cell) {
			int to_be_parsed_ncell_count = 0;

			/* This the current/serving cell.
			 * In practice the <neighbor_count> is always 0 for other than
			 * the serving cell, i.e. no neigbour cell list is available.
			 * Thus, handle neighbor cells only for the serving cell.
			 */
			cells->current_cell = parsed_cell;
			if (parsed_ncells_count != 0) {
				/* Allocate room for the parsed neighbor info. */
				if (parsed_ncells_count > CONFIG_LTE_NEIGHBOR_CELLS_MAX) {
					to_be_parsed_ncell_count = CONFIG_LTE_NEIGHBOR_CELLS_MAX;
					incomplete = true;
					LOG_WRN("Cutting response, because received neigbor cell"
						" count is bigger than configured max: %d",
							CONFIG_LTE_NEIGHBOR_CELLS_MAX);

				} else {
					to_be_parsed_ncell_count = parsed_ncells_count;
				}
				ncells = k_calloc(
						to_be_parsed_ncell_count,
						sizeof(struct lte_lc_ncell));
				if (ncells == NULL) {
					LOG_WRN("Failed to allocate memory for the ncells"
						" (continue)");
					continue;
				}
				cells->neighbor_cells = ncells;
				cells->ncells_count = to_be_parsed_ncell_count;
			}

			/* Parse neighbors */
			for (j = 0; j < to_be_parsed_ncell_count; j++) {
				/* <n_earfcn[j]> */
				curr_index++;
				err = at_params_int_get(&resp_list,
							curr_index,
							&cells->neighbor_cells[j].earfcn);
				if (err) {
					LOG_ERR("Could not parse n_earfcn, error: %d", err);
					goto clean_exit;
				}

				/* <n_phys_cell_id[j]> */
				curr_index++;
				err = at_params_short_get(&resp_list,
							  curr_index,
							  &cells->neighbor_cells[j].phys_cell_id);
				if (err) {
					LOG_ERR("Could not parse n_phys_cell_id, error: %d", err);
					goto clean_exit;
				}

				/* <n_rsrp[j]> */
				curr_index++;
				err = at_params_int_get(&resp_list, curr_index, &tmp_int);
				if (err) {
					LOG_ERR("Could not parse n_rsrp, error: %d", err);
					goto clean_exit;
				}
				cells->neighbor_cells[j].rsrp = tmp_int;

				/* <n_rsrq[j]> */
				curr_index++;
				err = at_params_int_get(&resp_list, curr_index, &tmp_int);
				if (err) {
					LOG_ERR("Could not parse n_rsrq, error: %d", err);
					goto clean_exit;
				}
				cells->neighbor_cells[j].rsrq = tmp_int;

				/* <time_diff[j]> */
				curr_index++;
				err = at_params_int_get(&resp_list,
							curr_index,
							&cells->neighbor_cells[j].time_diff);
				if (err) {
					LOG_ERR("Could not parse time_diff, error: %d", err);
					goto clean_exit;
				}
			}
		} else {
			cells->gci_cells[k] = parsed_cell;
			cells->gci_cells_count++; /* Increase count for non-serving GCI cell */
			k++;
		}
	}

	if (incomplete) {
		err = -E2BIG;
		LOG_ERR("Buffer is too small; results incomplete: %d", err);
	}

clean_exit:
	at_params_list_free(&resp_list);

	return err;
}

int parse_xmodemsleep(const char *at_response, struct lte_lc_modem_sleep *modem_sleep)
{
	int err;
	struct at_param_list resp_list = {0};
	uint16_t type;

	if (modem_sleep == NULL || at_response == NULL) {
		return -EINVAL;
	}

	err = at_params_list_init(&resp_list, AT_XMODEMSLEEP_PARAMS_COUNT_MAX);
	if (err) {
		LOG_ERR("Could not init AT params list, error: %d", err);
		return err;
	}

	/* Parse XMODEMSLEEP response and populate AT parameter list */
	err = at_parser_params_from_str(at_response, NULL, &resp_list);
	if (err) {
		LOG_ERR("Could not parse %%XMODEMSLEEP response, error: %d", err);
		goto clean_exit;
	}

	err = at_params_unsigned_short_get(&resp_list, AT_XMODEMSLEEP_TYPE_INDEX, &type);
	if (err) {
		LOG_ERR("Could not get mode sleep type, error: %d", err);
		goto clean_exit;
	}
	modem_sleep->type = type;

	/* If the time parameter is not present sleep time is considered infinite. */
	if (at_params_valid_count_get(&resp_list) < AT_XMODEMSLEEP_PARAMS_COUNT_MAX - 1) {
		modem_sleep->time = -1;
		goto clean_exit;
	}

	err = at_params_int64_get(&resp_list, AT_XMODEMSLEEP_TIME_INDEX, &modem_sleep->time);
	if (err) {
		LOG_ERR("Could not get time until next modem sleep, error: %d", err);
		goto clean_exit;
	}

clean_exit:
	at_params_list_free(&resp_list);
	return err;
}

int parse_mdmev(const char *at_response, enum lte_lc_modem_evt *modem_evt)
{
	static const char *const event_types[] = {
		[LTE_LC_MODEM_EVT_LIGHT_SEARCH_DONE] = AT_MDMEV_SEARCH_STATUS_1,
		[LTE_LC_MODEM_EVT_SEARCH_DONE] = AT_MDMEV_SEARCH_STATUS_2,
		[LTE_LC_MODEM_EVT_RESET_LOOP] = AT_MDMEV_RESET_LOOP,
		[LTE_LC_MODEM_EVT_BATTERY_LOW] = AT_MDMEV_BATTERY_LOW,
		[LTE_LC_MODEM_EVT_OVERHEATED] = AT_MDMEV_OVERHEATED,
	};

	if (at_response == NULL || modem_evt == NULL) {
		return -EINVAL;
	}

	if (!response_is_valid(at_response, sizeof(AT_MDMEV_RESPONSE_PREFIX) - 1,
			       AT_MDMEV_RESPONSE_PREFIX)) {
		LOG_ERR("Invalid MDMEV response");
		return -EIO;
	}

	const char *start_ptr = at_response + sizeof(AT_MDMEV_RESPONSE_PREFIX) - 1;

	for (size_t i = 0; i < ARRAY_SIZE(event_types); i++) {
		if (strcmp(event_types[i], start_ptr) == 0) {
			LOG_DBG("Occurrence found: %s", event_types[i]);
			*modem_evt = i;

			return 0;
		}
	}

	LOG_DBG("No modem event type found: %s", at_response);

	return -ENODATA;
}

char *periodic_search_pattern_get(char *const buf, size_t buf_size,
				  const struct lte_lc_periodic_search_pattern *const pattern)
{
	int err;

	if (pattern->type == LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE) {
		/* Range format:
		 * "<type>,<initial_sleep>,<final_sleep>,[<time_to_final_sleep>],
		 *  <pattern_end_point>"
		 */
		if (pattern->range.time_to_final_sleep != -1) {
			err = snprintk(buf, buf_size, "\"0,%u,%u,%u,%u\"",
				       pattern->range.initial_sleep, pattern->range.final_sleep,
				       pattern->range.time_to_final_sleep,
				       pattern->range.pattern_end_point);
			if (err < 0 || err >= buf_size) {
				goto error;
			}
		} else {
			err = snprintk(buf, buf_size, "\"0,%u,%u,,%u\"",
				       pattern->range.initial_sleep, pattern->range.final_sleep,
				       pattern->range.pattern_end_point);
			if (err < 0 || err >= buf_size) {
				goto error;
			}
		}
	} else if (pattern->type == LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE) {
		/* Table format: "<type>,<val1>[,<val2>][,<val3>][,<val4>][,<val5>]". */
		if (pattern->table.val_2 == -1) {
			err = snprintk(buf, buf_size, "\"1,%u\"", pattern->table.val_1);
			if (err < 0 || err >= buf_size) {
				goto error;
			}
		} else if (pattern->table.val_3 == -1) {
			err = snprintk(buf, buf_size, "\"1,%u,%u\"",
				       pattern->table.val_1, pattern->table.val_2);
			if (err < 0 || err >= buf_size) {
				goto error;
			}
		} else if (pattern->table.val_4 == -1) {
			err = snprintk(buf, buf_size, "\"1,%u,%u,%u\"",
				       pattern->table.val_1, pattern->table.val_2,
				       pattern->table.val_3);
			if (err < 0 || err >= buf_size) {
				goto error;
			}
		} else if (pattern->table.val_5 == -1) {
			err = snprintk(buf, buf_size, "\"1,%u,%u,%u,%u\"",
				       pattern->table.val_1, pattern->table.val_2,
				       pattern->table.val_3, pattern->table.val_4);
			if (err < 0 || err >= buf_size) {
				goto error;
			}
		} else {
			err = snprintk(buf, buf_size, "\"1,%u,%u,%u,%u,%u\"",
				       pattern->table.val_1, pattern->table.val_2,
				       pattern->table.val_3, pattern->table.val_4,
				       pattern->table.val_5);
			if (err < 0 || err >= buf_size) {
				goto error;
			}
		}
	} else {
		LOG_WRN("Unrecognized periodic search pattern type");
		buf[0] = '\0';
	}

	return buf;

error:
	LOG_ERR("An error occurred, the pattern string is empty. Error code %d", err);
	buf[0] = '\0';

	return buf;
}

int parse_periodic_search_pattern(const char *const pattern_str,
				  struct lte_lc_periodic_search_pattern *pattern)
{
	int err;
	int values[5];
	size_t param_count;

	err = sscanf(pattern_str, "%d,%u,%u,%u,%u,%u",
		(int *)&pattern->type, &values[0], &values[1], &values[2], &values[3], &values[4]);
	if (err < 1) {
		LOG_ERR("Unrecognized pattern type");
		return -EBADMSG;
	}

	param_count = err;

	if ((pattern->type == LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE) &&
	    (param_count >= 3)) {
		/* The 'time_to_final_sleep' parameter is optional and may not always be present.
		 * If that's the case, there will be only 3 matches, and we need a
		 * workaround to get the 'pattern_end_point' value.
		 */
		if (param_count == 3) {
			param_count = sscanf(pattern_str, "%*u,%*u,%*u,,%u", &values[3]);
			if (param_count != 1) {
				LOG_ERR("Could not find 'pattern_end_point' value");
				return -EBADMSG;
			}

			values[2] = -1;
		}

		pattern->range.initial_sleep = values[0];
		pattern->range.final_sleep = values[1];
		pattern->range.time_to_final_sleep = values[2];
		pattern->range.pattern_end_point = values[3];
	} else if ((pattern->type == LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE) &&
		   (param_count >= 2)) {
		/* Populate optional parameters only if matched, otherwise set
		 * to disabled, -1.
		 */
		pattern->table.val_1 = values[0];
		pattern->table.val_2 = param_count > 2 ? values[1] : -1;
		pattern->table.val_3 = param_count > 3 ? values[2] : -1;
		pattern->table.val_4 = param_count > 4 ? values[3] : -1;
		pattern->table.val_5 = param_count > 5 ? values[4] : -1;
	} else {
		LOG_DBG("No valid pattern found");
		return -EBADMSG;
	}

	return 0;
}
