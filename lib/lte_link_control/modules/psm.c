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

#include "common/event_handler_list.h"
#include "modules/psm.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

static void psm_get_work_fn(struct k_work *work_item);
K_WORK_DEFINE(psm_get_work, psm_get_work_fn);

/* Different values in the T3324 lookup table. */
#define T3324_LOOKUP_DIFFERENT_VALUES	  3
/* Different values in the T3412 extended lookup table. */
#define T3412_EXT_LOOKUP_DIFFERENT_VALUES 7
/* Maximum value for the timer value field of the timer information elements in both
 * T3324 and T3412 extended lookup tables.
 */
#define TIMER_VALUE_MAX			  31

/* String format that can be used in printf-style functions for printing a byte as a bit field. */
#define BYTE_TO_BINARY_STRING_FORMAT "%c%c%c%c%c%c%c%c"
/* Arguments for a byte in a bit field string representation. */
#define BYTE_TO_BINARY_STRING_ARGS(byte)                                                           \
	((byte)&0x80 ? '1' : '0'), ((byte)&0x40 ? '1' : '0'), ((byte)&0x20 ? '1' : '0'),           \
		((byte)&0x10 ? '1' : '0'), ((byte)&0x08 ? '1' : '0'), ((byte)&0x04 ? '1' : '0'),   \
		((byte)&0x02 ? '1' : '0'), ((byte)&0x01 ? '1' : '0')

/* Lookup table for T3412-extended timer used for periodic TAU. Unit is seconds.
 * Ref: GPRS Timer 3 in 3GPP TS 24.008 Table 10.5.163a.
 */
static const uint32_t t3412_ext_lookup[8] = {600, 3600, 36000, 2, 30, 60, 1152000, 0};

/* Lookup table for T3412 (legacy) timer used for periodic TAU. Unit is seconds.
 * Ref: GPRS Timer in 3GPP TS 24.008 Table 10.5.172.
 */
static const uint32_t t3412_lookup[8] = {2, 60, 360, 60, 60, 60, 60, 0};

/* Lookup table for T3324 timer used for PSM active time in seconds.
 * Ref: GPRS Timer 2 IE in 3GPP TS 24.008 Table 10.5.163.
 */
static const uint32_t t3324_lookup[8] = {2, 60, 360, 60, 60, 60, 60, 0};

/* Requested PSM RAT setting */
static char requested_psm_param_rat[9] = CONFIG_LTE_PSM_REQ_RAT;
/* Requested PSM RPTAU setting */
static char requested_psm_param_rptau[9] = CONFIG_LTE_PSM_REQ_RPTAU;
/* Request PSM to be disabled and timers set to default values */
static const char psm_disable[] = "AT+CPSMS=";

/* Internal enums */

enum feaconf_oper {
	FEACONF_OPER_WRITE = 0,
	FEACONF_OPER_READ = 1,
	FEACONF_OPER_LIST = 2
};

enum feaconf_feat {
	FEACONF_FEAT_PROPRIETARY_PSM = 0
};

static int feaconf_write(enum feaconf_feat feat, bool state)
{
	return nrf_modem_at_printf("AT%%FEACONF=%d,%d,%u", FEACONF_OPER_WRITE, feat, state);
}

void psm_evt_update_send(struct lte_lc_psm_cfg *psm_cfg)
{
	static struct lte_lc_psm_cfg prev_psm_cfg;
	struct lte_lc_evt evt = {0};

	/* PSM configuration update event */
	if ((psm_cfg->tau != prev_psm_cfg.tau) ||
	    (psm_cfg->active_time != prev_psm_cfg.active_time)) {
		evt.type = LTE_LC_EVT_PSM_UPDATE;

		memcpy(&prev_psm_cfg, psm_cfg, sizeof(struct lte_lc_psm_cfg));
		memcpy(&evt.psm_cfg, psm_cfg, sizeof(struct lte_lc_psm_cfg));
		event_handler_list_dispatch(&evt);
	}
}

static void psm_get_work_fn(struct k_work *work_item)
{
	int err;
	struct lte_lc_psm_cfg psm_cfg = {.active_time = -1, .tau = -1};

	err = psm_get(&psm_cfg.tau, &psm_cfg.active_time);
	if (err) {
		if (err != -EBADMSG) {
			LOG_ERR("Failed to get PSM information");
		}
		return;
	}

	psm_evt_update_send(&psm_cfg);
}

static int psm_encode_timer(char *encoded_timer_str, uint32_t requested_value,
			    const uint32_t *lookup_table, uint8_t lookup_table_size)
{
	/* Timer unit and timer value refer to the terminology used in 3GPP 24.008
	 * Ch. 10.5.7.4a and Ch. 10.5.7.3. for bits 6 to 8 and bits 1 to 5, respectively
	 */

	/* Lookup table index to the currently selected timer unit */
	uint32_t selected_index = -1;
	/* Currently calculated value for the timer, which tries to match the requested value */
	uint32_t selected_value = -1;
	/* Currently selected timer value */
	uint32_t selected_timer_value = -1;
	/* Timer unit and timer value encoded as an integer which will be converted to a string */
	uint8_t encoded_value_int = 0;

	__ASSERT_NO_MSG(encoded_timer_str != NULL);
	__ASSERT_NO_MSG(lookup_table != NULL);

	/* Search a value that is as close as possible to the requested value
	 * rounding it up to the closest possible value
	 */
	for (int i = 0; i < lookup_table_size; i++) {
		uint32_t current_timer_value = requested_value / lookup_table[i];

		if (requested_value % lookup_table[i] > 0) {
			/* Round up the time when it's not divisible by the timer unit */
			current_timer_value++;
		}

		/* Current timer unit is so small that timer value range is not enough */
		if (current_timer_value > TIMER_VALUE_MAX) {
			continue;
		}

		uint32_t current_value = lookup_table[i] * current_timer_value;

		/* Use current timer unit if current value is closer to requested value
		 * than currently selected values
		 */
		if (selected_value == -1 ||
		    current_value - requested_value < selected_value - requested_value) {
			selected_value = current_value;
			selected_index = i;
			selected_timer_value = current_timer_value;
		}
	}

	if (selected_index != -1) {
		LOG_DBG("requested_value=%d, selected_value=%d, selected_timer_unit=%d, "
			"selected_timer_value=%d, selected_index=%d",
			requested_value, selected_value, lookup_table[selected_index],
			selected_timer_value, selected_index);

		/* Selected index (timer unit) is in bits 6 to 8 */
		encoded_value_int = (selected_index << 5) | selected_timer_value;
		sprintf(encoded_timer_str, BYTE_TO_BINARY_STRING_FORMAT,
			BYTE_TO_BINARY_STRING_ARGS(encoded_value_int));
	} else {
		LOG_ERR("requested_value=%d is too big to be represented by the timer encoding",
			requested_value);
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_UNITY)
int psm_encode(char *tau_ext_str, char *active_time_str, int rptau, int rat)
#else
static int psm_encode(char *tau_ext_str, char *active_time_str, int rptau, int rat)
#endif /* CONFIG_UNITY */
{
	int ret = 0;

	LOG_DBG("TAU: %d sec, active time: %d sec", rptau, rat);

	__ASSERT_NO_MSG(tau_ext_str != NULL);
	__ASSERT_NO_MSG(active_time_str != NULL);

	if (rptau >= 0) {
		ret = psm_encode_timer(tau_ext_str, rptau, t3412_ext_lookup,
				       T3412_EXT_LOOKUP_DIFFERENT_VALUES);
	} else {
		*tau_ext_str = '\0';
		LOG_DBG("Using modem default value for RPTAU");
	}

	if (rat >= 0) {
		ret |= psm_encode_timer(active_time_str, rat, t3324_lookup,
					T3324_LOOKUP_DIFFERENT_VALUES);
	} else {
		*active_time_str = '\0';
		LOG_DBG("Using modem default value for RAT");
	}

	return ret;
}

int psm_param_set(const char *rptau, const char *rat)
{
	if ((rptau != NULL && strlen(rptau) != 8) || (rat != NULL && strlen(rat) != 8)) {
		return -EINVAL;
	}

	if (rptau != NULL) {
		strcpy(requested_psm_param_rptau, rptau);
		LOG_DBG("RPTAU set to %s", requested_psm_param_rptau);
	} else {
		*requested_psm_param_rptau = '\0';
		LOG_DBG("Using modem default value for RPTAU");
	}

	if (rat != NULL) {
		strcpy(requested_psm_param_rat, rat);
		LOG_DBG("RAT set to %s", requested_psm_param_rat);
	} else {
		*requested_psm_param_rat = '\0';
		LOG_DBG("Using modem default value for RAT");
	}

	return 0;
}

int psm_param_set_seconds(int rptau, int rat)
{
	int ret;

	ret = psm_encode(requested_psm_param_rptau, requested_psm_param_rat, rptau, rat);

	if (ret != 0) {
		*requested_psm_param_rptau = '\0';
		*requested_psm_param_rat = '\0';
	}

	LOG_DBG("RPTAU=%d (%s), RAT=%d (%s), ret=%d", rptau, requested_psm_param_rptau, rat,
		requested_psm_param_rat, ret);

	return ret;
}

int psm_req(bool enable)
{
	int err;

	LOG_DBG("enable=%d, tau=%s, rat=%s", enable, requested_psm_param_rptau,
		requested_psm_param_rat);

	if (enable) {
		if (strlen(requested_psm_param_rptau) == 8 &&
		    strlen(requested_psm_param_rat) == 8) {
			err = nrf_modem_at_printf("AT+CPSMS=1,,,\"%s\",\"%s\"",
						  requested_psm_param_rptau,
						  requested_psm_param_rat);
		} else if (strlen(requested_psm_param_rptau) == 8) {
			err = nrf_modem_at_printf("AT+CPSMS=1,,,\"%s\"", requested_psm_param_rptau);
		} else if (strlen(requested_psm_param_rat) == 8) {
			err = nrf_modem_at_printf("AT+CPSMS=1,,,,\"%s\"", requested_psm_param_rat);
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

int psm_get(int *tau, int *active_time)
{
	int err;
	struct lte_lc_psm_cfg psm_cfg;
	struct at_parser parser;
	char active_time_str[9] = {0};
	char tau_ext_str[9] = {0};
	char tau_legacy_str[9] = {0};
	int len;
	int reg_status = 0;
	static char response[160] = {0};

	if ((tau == NULL) || (active_time == NULL)) {
		return -EINVAL;
	}

	/* Format of XMONITOR AT command response:
	 * %XMONITOR: <reg_status>,[<full_name>,<short_name>,<plmn>,<tac>,<AcT>,<band>,<cell_id>,
	 * <phys_cell_id>,<EARFCN>,<rsrp>,<snr>,<NW-provided_eDRX_value>,<Active-Time>,
	 * <Periodic-TAU-ext>,<Periodic-TAU>]
	 * We need to parse the three last parameters, Active-Time, Periodic-TAU-ext and
	 * Periodic-TAU.
	 */

	response[0] = '\0';

	err = nrf_modem_at_cmd(response, sizeof(response), "AT%%XMONITOR");
	if (err) {
		LOG_ERR("AT command failed, error: %d", err);
		return -EFAULT;
	}

	err = at_parser_init(&parser, response);
	__ASSERT_NO_MSG(err == 0);

	/* Check registration status */
	err = at_parser_num_get(&parser, 1, &reg_status);
	if (err) {
		LOG_ERR("AT command parsing failed, error: %d", err);
		return -EBADMSG;
	} else if (reg_status != LTE_LC_NW_REG_REGISTERED_HOME &&
		   reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING) {
		LOG_WRN("No PSM parameters because device not registered, status: %d", reg_status);
		return -EBADMSG;
	}

	/* <Active-Time> */
	len = sizeof(active_time_str);
	err = at_parser_string_get(&parser, 14, active_time_str, &len);
	if (err) {
		LOG_ERR("AT command parsing failed, error: %d", err);
		return -EBADMSG;
	}

	/* <Periodic-TAU-ext> */
	len = sizeof(tau_ext_str);
	err = at_parser_string_get(&parser, 15, tau_ext_str, &len);
	if (err) {
		LOG_ERR("AT command parsing failed, error: %d", err);
		return -EBADMSG;
	}

	/* <Periodic-TAU> */
	len = sizeof(tau_legacy_str);
	err = at_parser_string_get(&parser, 16, tau_legacy_str, &len);
	if (err) {
		LOG_ERR("AT command parsing failed, error: %d", err);
		return -EBADMSG;
	}

	err = psm_parse(active_time_str, tau_ext_str, tau_legacy_str, &psm_cfg);
	if (err) {
		LOG_ERR("Failed to parse PSM configuration, error: %d", err);
		return -EBADMSG;
	}

	*tau = psm_cfg.tau;
	*active_time = psm_cfg.active_time;

	LOG_DBG("TAU: %d sec, active time: %d sec", *tau, *active_time);

	return 0;
}

int psm_proprietary_req(bool enable)
{
	if (feaconf_write(FEACONF_FEAT_PROPRIETARY_PSM, enable) != 0) {
		return -EFAULT;
	}

	return 0;
}

int psm_parse(const char *active_time_str, const char *tau_ext_str, const char *tau_legacy_str,
	      struct lte_lc_psm_cfg *psm_cfg)
{
	char unit_str[4] = {0};
	size_t unit_str_len = sizeof(unit_str) - 1;
	size_t lut_idx;
	uint32_t timer_unit, timer_value;

	__ASSERT_NO_MSG(active_time_str != NULL);
	__ASSERT_NO_MSG(tau_ext_str != NULL);
	__ASSERT_NO_MSG(psm_cfg != NULL);

	if (strlen(active_time_str) != 8 || strlen(tau_ext_str) != 8 ||
	    (tau_legacy_str != NULL && strlen(tau_legacy_str) != 8)) {
		return -EINVAL;
	}

	/* Parse T3412-extended (periodic TAU) timer */
	memcpy(unit_str, tau_ext_str, unit_str_len);

	lut_idx = strtoul(unit_str, NULL, 2);
	__ASSERT_NO_MSG(lut_idx < ARRAY_SIZE(t3412_ext_lookup));

	timer_unit = t3412_ext_lookup[lut_idx];
	timer_value = strtoul(tau_ext_str + unit_str_len, NULL, 2);
	psm_cfg->tau = timer_unit ? timer_unit * timer_value : -1;

	/* If T3412-extended is disabled, periodic TAU is reported using the T3412 legacy timer
	 * if the caller requests for it
	 */
	if (psm_cfg->tau == -1 && tau_legacy_str != NULL) {
		memcpy(unit_str, tau_legacy_str, unit_str_len);

		lut_idx = strtoul(unit_str, NULL, 2);
		__ASSERT_NO_MSG(lut_idx < ARRAY_SIZE(t3412_lookup));

		timer_unit = t3412_lookup[lut_idx];
		if (timer_unit == 0) {
			/* TAU must be reported either using T3412-extended or T3412 (legacy)
			 * timer, so the timer unit is expected to be valid.
			 */
			LOG_ERR("Expected valid T3412 timer unit");
			return -EINVAL;
		}
		timer_value = strtoul(tau_legacy_str + unit_str_len, NULL, 2);
		psm_cfg->tau = timer_unit * timer_value;
	}

	/* Parse active time */
	memcpy(unit_str, active_time_str, unit_str_len);

	lut_idx = strtoul(unit_str, NULL, 2);
	__ASSERT_NO_MSG(lut_idx < ARRAY_SIZE(t3324_lookup));

	timer_unit = t3324_lookup[lut_idx];
	timer_value = strtoul(active_time_str + unit_str_len, NULL, 2);
	psm_cfg->active_time = timer_unit ? timer_unit * timer_value : -1;

	LOG_DBG("TAU: %d sec, active time: %d sec", psm_cfg->tau, psm_cfg->active_time);

	return 0;
}

struct k_work *psm_work_get(void)
{
	return &psm_get_work;
}
