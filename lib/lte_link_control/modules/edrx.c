/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <modem/at_monitor.h>
#include <modem/lte_lc.h>
#include <modem/lte_lc_trace.h>
#include <modem/at_parser.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>

#include "common/work_q.h"
#include "common/event_handler_list.h"
#include "modules/edrx.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

/* CEDRXP notification parameters */
#define AT_CEDRXP_ACTT_INDEX	 1
#define AT_CEDRXP_REQ_EDRX_INDEX 2
#define AT_CEDRXP_NW_EDRX_INDEX	 3
#define AT_CEDRXP_NW_PTW_INDEX	 4

/* CEDRXS command parameters */
#define AT_CEDRXS_MODE_INDEX
#define AT_CEDRXS_ACTT_WB 4
#define AT_CEDRXS_ACTT_NB 5

/* Length for eDRX and PTW values */
#define LTE_LC_EDRX_VALUE_LEN 5

/* Requested eDRX state (enabled/disabled) */
static bool requested_edrx_enable;
/* Requested eDRX setting */
static char requested_edrx_value_ltem[LTE_LC_EDRX_VALUE_LEN] = CONFIG_LTE_EDRX_REQ_VALUE_LTE_M;
static char requested_edrx_value_nbiot[LTE_LC_EDRX_VALUE_LEN] = CONFIG_LTE_EDRX_REQ_VALUE_NBIOT;
/* Requested PTW setting */
static char requested_ptw_value_ltem[LTE_LC_EDRX_VALUE_LEN] = CONFIG_LTE_PTW_VALUE_LTE_M;
static char requested_ptw_value_nbiot[LTE_LC_EDRX_VALUE_LEN] = CONFIG_LTE_PTW_VALUE_NBIOT;

/* Currently used eDRX setting as indicated by the modem */
static char edrx_value_ltem[LTE_LC_EDRX_VALUE_LEN];
static char edrx_value_nbiot[LTE_LC_EDRX_VALUE_LEN];
/* Currently used PTW setting as indicated by the modem */
static char ptw_value_ltem[LTE_LC_EDRX_VALUE_LEN];
static char ptw_value_nbiot[LTE_LC_EDRX_VALUE_LEN];

static void edrx_ptw_send_work_fn(struct k_work *work_item);
K_WORK_DEFINE(edrx_ptw_send_work, edrx_ptw_send_work_fn);

AT_MONITOR(ltelc_atmon_cedrxp, "+CEDRXP", at_handler_cedrxp);

static void lte_lc_edrx_current_values_clear(void)
{
	memset(edrx_value_ltem, 0, sizeof(edrx_value_ltem));
	memset(ptw_value_ltem, 0, sizeof(ptw_value_ltem));
	memset(edrx_value_nbiot, 0, sizeof(edrx_value_nbiot));
	memset(ptw_value_nbiot, 0, sizeof(ptw_value_nbiot));
}

static void lte_lc_edrx_values_store(enum lte_lc_lte_mode mode, char *edrx_value, char *ptw_value)
{
	switch (mode) {
	case LTE_LC_LTE_MODE_LTEM:
		strcpy(edrx_value_ltem, edrx_value);
		strcpy(ptw_value_ltem, ptw_value);
		break;
	case LTE_LC_LTE_MODE_NBIOT:
		strcpy(edrx_value_nbiot, edrx_value);
		strcpy(ptw_value_nbiot, ptw_value);
		break;
	default:
		lte_lc_edrx_current_values_clear();
		break;
	}
}

static void edrx_ptw_send_work_fn(struct k_work *work_item)
{
	int err;
	int actt[] = {AT_CEDRXS_ACTT_WB, AT_CEDRXS_ACTT_NB};

	/* Apply the configurations for both LTE-M and NB-IoT. */
	for (size_t i = 0; i < ARRAY_SIZE(actt); i++) {
		char *requested_ptw_value = (actt[i] == AT_CEDRXS_ACTT_WB)
						    ? requested_ptw_value_ltem
						    : requested_ptw_value_nbiot;
		char *ptw_value = (actt[i] == AT_CEDRXS_ACTT_WB) ? ptw_value_ltem : ptw_value_nbiot;

		if (strlen(requested_ptw_value) == 4 &&
		    strcmp(ptw_value, requested_ptw_value) != 0) {

			err = nrf_modem_at_printf("AT%%XPTW=%d,\"%s\"", actt[i],
						  requested_ptw_value);
			if (err) {
				LOG_ERR("Failed to request PTW, reported error: %d", err);
			}
		}
	}
}

#if defined(CONFIG_UNITY)
void lte_lc_edrx_on_modem_cfun(int mode, void *ctx)
#else
NRF_MODEM_LIB_ON_CFUN(lte_lc_edrx_cfun_hook, lte_lc_edrx_on_modem_cfun, NULL);

static void lte_lc_edrx_on_modem_cfun(int mode, void *ctx)
#endif /* CONFIG_UNITY */
{
	ARG_UNUSED(ctx);

	/* If eDRX is enabled and modem is powered off, subscription of unsolicited eDRX
	 * notifications must be re-newed because modem forgets that information
	 * although it stores eDRX value and PTW for both system modes.
	 */
	if (mode == LTE_LC_FUNC_MODE_POWER_OFF && requested_edrx_enable) {
		lte_lc_edrx_current_values_clear();
		/* We want to avoid sending AT commands in the callback. However,
		 * when modem is powered off, we are not expecting AT notifications
		 * that could cause an assertion or missing notification.
		 */
		edrx_request(requested_edrx_enable);
	}
}

/* Get Paging Time Window multiplier for the LTE mode.
 * Multiplier is 1.28 s for LTE-M, and 2.56 s for NB-IoT, derived from
 * Figure 10.5.5.32/3GPP TS 24.008.
 */
static void get_ptw_multiplier(enum lte_lc_lte_mode lte_mode, float *ptw_multiplier)
{
	__ASSERT_NO_MSG(ptw_multiplier != NULL);

	if (lte_mode == LTE_LC_LTE_MODE_NBIOT) {
		*ptw_multiplier = 2.56;
	} else {
		__ASSERT_NO_MSG(lte_mode == LTE_LC_LTE_MODE_LTEM);
		*ptw_multiplier = 1.28;
	}
}

static void get_edrx_value(enum lte_lc_lte_mode lte_mode, uint8_t idx, float *edrx_value)
{
	uint16_t multiplier = 0;

	/* Lookup table to eDRX multiplier values, based on T_eDRX values found
	 * in Table 10.5.5.32/3GPP TS 24.008. The actual value is
	 * (multiplier * 10.24 s), except for the first entry which is handled
	 * as a special case per note 3 in the specification.
	 */
	static const uint16_t edrx_lookup_ltem[16] = {0,  1,  2,  4,  6,   8,	10,  12,
						      14, 16, 32, 64, 128, 256, 256, 256};
	static const uint16_t edrx_lookup_nbiot[16] = {2, 2,  2,  4,  2,   8,	2,   2,
						       2, 16, 32, 64, 128, 256, 512, 1024};

	__ASSERT_NO_MSG(edrx_value != NULL);
	/* idx is parsed from 4 character bit field string so it cannot be more than 15 */
	__ASSERT_NO_MSG(idx < ARRAY_SIZE(edrx_lookup_ltem));

	if (lte_mode == LTE_LC_LTE_MODE_LTEM) {
		multiplier = edrx_lookup_ltem[idx];
	} else {
		__ASSERT_NO_MSG(lte_mode == LTE_LC_LTE_MODE_NBIOT);
		multiplier = edrx_lookup_nbiot[idx];
	}

	*edrx_value = multiplier == 0 ? 5.12 : multiplier * 10.24;
}

#if defined(CONFIG_UNITY)
int parse_edrx(const char *at_response, struct lte_lc_edrx_cfg *cfg, char *edrx_str,
		      char *ptw_str)
#else
/* Parses eDRX parameters from a +CEDRXS notification or a +CEDRXRDP response. */
static int parse_edrx(const char *at_response, struct lte_lc_edrx_cfg *cfg, char *edrx_str,
		      char *ptw_str)
#endif /* CONFIG_UNITY */
{
	int err, tmp_int;
	uint8_t idx;
	struct at_parser parser;
	char tmp_buf[5];
	size_t len = sizeof(tmp_buf);
	float ptw_multiplier;

	__ASSERT_NO_MSG(at_response != NULL);
	__ASSERT_NO_MSG(cfg != NULL);
	__ASSERT_NO_MSG(edrx_str != NULL);
	__ASSERT_NO_MSG(ptw_str != NULL);

	err = at_parser_init(&parser, at_response);
	__ASSERT_NO_MSG(err == 0);

	err = at_parser_num_get(&parser, AT_CEDRXP_ACTT_INDEX, &tmp_int);
	if (err) {
		LOG_ERR("Failed to get LTE mode, error: %d", err);
		goto clean_exit;
	}

	/* The access technology indicators 4 for LTE-M and 5 for NB-IoT are
	 * specified in 3GPP 27.007 Ch. 7.41.
	 * 0 indicates that the access technology does not currently use eDRX.
	 * Any other value is not expected, and we use 0xFFFFFFFF to represent those.
	 */
	cfg->mode = tmp_int == 0   ? LTE_LC_LTE_MODE_NONE
		    : tmp_int == 4 ? LTE_LC_LTE_MODE_LTEM
		    : tmp_int == 5 ? LTE_LC_LTE_MODE_NBIOT
				   : 0xFFFFFFFF; /* Intentionally illegal value */

	/* Check for the case where eDRX is not used. */
	if (cfg->mode == LTE_LC_LTE_MODE_NONE) {
		cfg->edrx = 0;
		cfg->ptw = 0;

		err = 0;
		goto clean_exit;
	} else if (cfg->mode == 0xFFFFFFFF) {
		err = -ENODATA;
		goto clean_exit;
	}

	err = at_parser_string_get(&parser, AT_CEDRXP_NW_EDRX_INDEX, tmp_buf, &len);
	if (err) {
		LOG_ERR("Failed to get eDRX configuration, error: %d", err);
		goto clean_exit;
	}

	/* Workaround for +CEDRXRDP response handling. The AcT-type is handled differently in the
	 * +CEDRXRDP response, so use of eDRX needs to be determined based on the eDRX value
	 * parameter.
	 */
	if (len == 0) {
		/* Network provided eDRX value is empty, eDRX is not used. */
		cfg->mode = LTE_LC_LTE_MODE_NONE;
		cfg->edrx = 0;
		cfg->ptw = 0;

		err = 0;
		goto clean_exit;
	}

	__ASSERT_NO_MSG(edrx_str != NULL);
	strcpy(edrx_str, tmp_buf);

	/* The eDRX value is a multiple of 10.24 seconds, except for the
	 * special case of idx == 0 for LTE-M, where the value is 5.12 seconds.
	 * The variable idx is used to map to the entry of index idx in
	 * Figure 10.5.5.32/3GPP TS 24.008, table for eDRX in S1 mode, and
	 * note 4 and 5 are taken into account.
	 */
	idx = strtoul(tmp_buf, NULL, 2);

	/* Get Paging Time Window multiplier for the LTE mode.
	 * Multiplier is 1.28 s for LTE-M, and 2.56 s for NB-IoT, derived from
	 * Figure 10.5.5.32/3GPP TS 24.008.
	 */
	get_ptw_multiplier(cfg->mode, &ptw_multiplier);

	get_edrx_value(cfg->mode, idx, &cfg->edrx);

	len = sizeof(tmp_buf);

	err = at_parser_string_get(&parser, AT_CEDRXP_NW_PTW_INDEX, tmp_buf, &len);
	if (err) {
		LOG_ERR("Failed to get PTW configuration, error: %d", err);
		goto clean_exit;
	}

	strcpy(ptw_str, tmp_buf);

	/* Value can be a maximum of 15, as there are 16 entries in the table
	 * for paging time window (both for LTE-M and NB1).
	 * We can use assert as only 4 bits can be received and if there would be more,
	 * the previous at_parser_string_get would fail.
	 */
	idx = strtoul(tmp_buf, NULL, 2);
	__ASSERT_NO_MSG(idx <= 15);

	/* The Paging Time Window is different for LTE-M and NB-IoT:
	 *	- LTE-M: (idx + 1) * 1.28 s
	 *	- NB-IoT (idx + 1) * 2.56 s
	 */
	idx += 1;
	cfg->ptw = idx * ptw_multiplier;

	LOG_DBG("eDRX value for %s: %d.%02d, PTW: %d.%02d",
		(cfg->mode == LTE_LC_LTE_MODE_LTEM) ? "LTE-M" : "NB-IoT", (int)cfg->edrx,
		(int)(100 * (cfg->edrx - (int)cfg->edrx)), (int)cfg->ptw,
		(int)(100 * (cfg->ptw - (int)cfg->ptw)));

clean_exit:
	return err;
}

static void at_handler_cedrxp(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};
	char edrx_value[LTE_LC_EDRX_VALUE_LEN] = {0};
	char ptw_value[LTE_LC_EDRX_VALUE_LEN] = {0};

	__ASSERT_NO_MSG(response != NULL);

	LOG_DBG("+CEDRXP notification");

	err = parse_edrx(response, &evt.edrx_cfg, edrx_value, ptw_value);
	if (err) {
		LOG_ERR("Can't parse eDRX, error: %d", err);
		return;
	}

	/* PTW must be requested after eDRX is enabled */
	lte_lc_edrx_values_store(evt.edrx_cfg.mode, edrx_value, ptw_value);
	/* Send PTW setting if eDRX is enabled, i.e., we have network mode */
	if (evt.edrx_cfg.mode != LTE_LC_LTE_MODE_NONE) {
		k_work_submit_to_queue(work_q_get(), &edrx_ptw_send_work);
	}
	evt.type = LTE_LC_EVT_EDRX_UPDATE;

	event_handler_list_dispatch(&evt);
}

int edrx_cfg_get(struct lte_lc_edrx_cfg *edrx_cfg)
{
	int err;
	char response[48];
	char edrx_value[LTE_LC_EDRX_VALUE_LEN] = {0};
	char ptw_value[LTE_LC_EDRX_VALUE_LEN] = {0};

	if (edrx_cfg == NULL) {
		return -EINVAL;
	}

	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CEDRXRDP");
	if (err) {
		LOG_ERR("Failed to request eDRX parameters, error: %d", err);
		return -EFAULT;
	}

	err = parse_edrx(response, edrx_cfg, edrx_value, ptw_value);
	if (err) {
		LOG_ERR("Failed to parse eDRX parameters, error: %d", err);
		return -EBADMSG;
	}

	lte_lc_edrx_values_store(edrx_cfg->mode, edrx_value, ptw_value);

	return 0;
}

int edrx_ptw_set(enum lte_lc_lte_mode mode, const char *ptw)
{
	char *ptw_value;

	if (mode != LTE_LC_LTE_MODE_LTEM && mode != LTE_LC_LTE_MODE_NBIOT) {
		LOG_ERR("LTE mode must be LTE-M or NB-IoT");
		return -EINVAL;
	}

	if (ptw != NULL && strlen(ptw) != 4) {
		return -EINVAL;
	}

	ptw_value = (mode == LTE_LC_LTE_MODE_LTEM) ? requested_ptw_value_ltem
						   : requested_ptw_value_nbiot;

	if (ptw != NULL) {
		strcpy(ptw_value, ptw);
		LOG_DBG("PTW set to %s for %s", ptw_value,
			(mode == LTE_LC_LTE_MODE_LTEM) ? "LTE-M" : "NB-IoT");
	} else {
		*ptw_value = '\0';
		LOG_DBG("PTW use default for %s",
			(mode == LTE_LC_LTE_MODE_LTEM) ? "LTE-M" : "NB-IoT");
	}

	return 0;
}

int edrx_request(bool enable)
{
	int err = 0;
	int actt[] = {AT_CEDRXS_ACTT_WB, AT_CEDRXS_ACTT_NB};

	LOG_DBG("enable=%d, "
		"requested_edrx_value_ltem=%s, edrx_value_ltem=%s, "
		"requested_ptw_value_ltem=%s, ptw_value_ltem=%s, ",
		enable, requested_edrx_value_ltem, edrx_value_ltem, requested_ptw_value_ltem,
		ptw_value_ltem);
	LOG_DBG("enable=%d, "
		"requested_edrx_value_nbiot=%s, edrx_value_nbiot=%s, "
		"requested_ptw_value_nbiot=%s, ptw_value_nbiot=%s",
		enable, requested_edrx_value_nbiot, edrx_value_nbiot, requested_ptw_value_nbiot,
		ptw_value_nbiot);

	requested_edrx_enable = enable;

	if (!enable) {
		err = nrf_modem_at_printf("AT+CEDRXS=3");
		if (err) {
			LOG_ERR("Failed to disable eDRX, reported error: %d", err);
			return -EFAULT;
		}
		lte_lc_edrx_current_values_clear();

		return 0;
	}

	/* Apply the configurations for both LTE-M and NB-IoT. */
	for (size_t i = 0; i < ARRAY_SIZE(actt); i++) {
		char *requested_edrx_value = (actt[i] == AT_CEDRXS_ACTT_WB)
						     ? requested_edrx_value_ltem
						     : requested_edrx_value_nbiot;
		char *edrx_value =
			(actt[i] == AT_CEDRXS_ACTT_WB) ? edrx_value_ltem : edrx_value_nbiot;

		if (strlen(requested_edrx_value) == 4) {
			if (strcmp(edrx_value, requested_edrx_value) != 0) {
				err = nrf_modem_at_printf("AT+CEDRXS=2,%d,\"%s\"", actt[i],
							  requested_edrx_value);
			} else {
				/* If current eDRX value is equal to requested value, set PTW */
				edrx_ptw_send_work_fn(NULL);
			}
		} else {
			err = nrf_modem_at_printf("AT+CEDRXS=2,%d", actt[i]);
		}

		if (err) {
			LOG_ERR("Failed to enable eDRX, reported error: %d", err);
			return -EFAULT;
		}
	}

	return 0;
}

int edrx_param_set(enum lte_lc_lte_mode mode, const char *edrx)
{
	char *edrx_value;

	if (mode != LTE_LC_LTE_MODE_LTEM && mode != LTE_LC_LTE_MODE_NBIOT) {
		LOG_ERR("LTE mode must be LTE-M or NB-IoT");
		return -EINVAL;
	}

	if (edrx != NULL && strlen(edrx) != 4) {
		return -EINVAL;
	}

	edrx_value = (mode == LTE_LC_LTE_MODE_LTEM) ? requested_edrx_value_ltem
						    : requested_edrx_value_nbiot;

	if (edrx) {
		strcpy(edrx_value, edrx);
		LOG_DBG("eDRX set to %s for %s", edrx_value,
			(mode == LTE_LC_LTE_MODE_LTEM) ? "LTE-M" : "NB-IoT");
	} else {
		*edrx_value = '\0';
		LOG_DBG("eDRX use default for %s",
			(mode == LTE_LC_LTE_MODE_LTEM) ? "LTE-M" : "NB-IoT");
	}

	return 0;
}
