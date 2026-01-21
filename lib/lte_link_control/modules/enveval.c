/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
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
#include <nrf_modem_at.h>

#include "common/event_handler_list.h"
#include "common/helpers.h"
#include "modules/cfun.h"
#include "modules/enveval.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

/* ENVEVAL AT commands */
#define AT_ENVEVAL		"AT%%ENVEVAL=%d,%s"
#define AT_ENVEVAL_CANCEL	"AT%%ENVEVALSTOP"

/** "MCCMNC" (including quotes) + comma or null termination */
#define PLMN_ENTRY_SIZE	9

AT_MONITOR(ltelc_atmon_enveval, "%ENVEVAL", at_handler_enveval);

static void at_handler_enveval(const char *notif)
{
	int ret;
	int earfcn;
	uint32_t cell_id;
	uint16_t rrc_state, energy_estimate, tau_trig, ce_level;
	int16_t rsrp, rsrq, snr, phys_cell_id, band, tx_power, tx_rep, rx_rep, dl_pathloss;
	uint8_t status;
	/* PLMN field is a string of maximum 6 characters + null termination. */
	char plmn_str[7] = {0};
	struct lte_lc_conn_eval_params *plmn_result;
	static uint8_t result_count;
	static struct lte_lc_conn_eval_params results[CONFIG_LTE_LC_ENV_EVAL_MAX_PLMN_COUNT];
	static bool env_eval_in_progress;

	__ASSERT_NO_MSG(notif != NULL);

	/* Initialize buffer if this is the first result */
	if (!env_eval_in_progress) {
		result_count = 0;
		memset(&results, 0, sizeof(results));
		env_eval_in_progress = true;
	}

	/* %ENVEVAL notification format (same as %CONEVAL):
	 *
	 * %ENVEVAL: <result>[,<rrc_state>,<energy_estimate>,<rsrp>,<rsrq>,<snr>,<cell_id>,<plmn>,
	 * <phys_cell_id>,<earfcn>,<band>,<tau_triggered>,<ce_level>,<tx_power>,<tx_repetitions>,
	 * <rx_repetitions>,<dl-pathloss>]
	 *
	 * In total, 17 parameters are expected to match for a successful notification parsing.
	 * If only 1 parameter is present (%ENVEVAL: <result>), it indicates end of evaluation.
	 */
	ret = sscanf(notif,
		     "%%ENVEVAL: "
		     "%hhu,"		/* <result> */
		     "%hu,"		/* <rrc_state> */
		     "%hu,"		/* <energy_estimate> */
		     "%hd,"		/* <rsrp> */
		     "%hd,"		/* <rsrq> */
		     "%hd,"		/* <snr> */
		     "\"%x\","		/* <cell_id> */
		     "\"%6[^\"]\","	/* <plmn> */
		     "%hd,"		/* <phys_cell_id> */
		     "%d,"		/* <earfcn> */
		     "%hd,"		/* <band> */
		     "%hu,"		/* <tau_triggered> */
		     "%hu,"		/* <ce_level> */
		     "%hd,"		/* <tx_power> */
		     "%hd,"		/* <tx_repetitions> */
		     "%hd,"		/* <rx_repetitions> */
		     "%hd",		/* <dl-pathloss> */
		     &status, &rrc_state, &energy_estimate, &rsrp, &rsrq, &snr, &cell_id, plmn_str,
		     &phys_cell_id, &earfcn, &band, &tau_trig, &ce_level, &tx_power, &tx_rep,
		     &rx_rep, &dl_pathloss);

	if (ret == 17) {
		/* Successfully parsed full notification - this is a PLMN result */

		/* Check if we have space for more results */
		if (result_count >= CONFIG_LTE_LC_ENV_EVAL_MAX_PLMN_COUNT) {
			LOG_ERR("Maximum number of PLMN results reached, ignoring notification");
			return;
		}

		/* Get pointer to the next available result slot */
		plmn_result = &results[result_count];

		plmn_result->rrc_state = (enum lte_lc_rrc_mode)rrc_state;
		plmn_result->energy_estimate = (enum lte_lc_energy_estimate)energy_estimate;
		plmn_result->tau_trig = (enum lte_lc_tau_triggered)tau_trig;
		plmn_result->ce_level = (enum lte_lc_ce_level)ce_level;
		plmn_result->earfcn = earfcn;
		plmn_result->dl_pathloss = dl_pathloss;
		plmn_result->rsrp = rsrp;
		plmn_result->rsrq = rsrq;
		plmn_result->tx_rep = tx_rep;
		plmn_result->rx_rep = rx_rep;
		plmn_result->phy_cid = phys_cell_id;
		plmn_result->band = band;
		plmn_result->snr = snr;
		plmn_result->tx_power = tx_power;
		plmn_result->cell_id = cell_id;

		/* Parse PLMN string: MCC (3 digits) + MNC (2-3 digits) */
		if (strlen(plmn_str) < 5) {
			LOG_ERR("Invalid PLMN string length: %zu", strlen(plmn_str));
			return;
		}

		/* Read MNC and store as integer. The MNC starts as the fourth character
		 * in the string, following three characters long MCC.
		 */
		ret = string_to_int(&plmn_str[3], 10, &plmn_result->mnc);
		if (ret) {
			LOG_ERR("Could not parse MNC from PLMN string");
			return;
		}

		/* Null-terminate MCC, read and store it */
		plmn_str[3] = '\0';

		ret = string_to_int(plmn_str, 10, &plmn_result->mcc);
		if (ret) {
			LOG_ERR("Could not parse MCC from PLMN string");
			return;
		}

		result_count++;

		LOG_DBG("Environment evaluation result %u received for %03d%02d",
			result_count,
			plmn_result->mcc, plmn_result->mnc);
	} else if (ret == 1) {
		/* Only result parameter parsed - this indicates end of evaluation */
		LOG_DBG("Environment evaluation completed with status: %d", status);
		/* Send accumulated results */
		event_handler_list_dispatch(
			&(struct lte_lc_evt) {
				.type = LTE_LC_EVT_ENV_EVAL_RESULT,
				.env_eval_result.status = status,
				.env_eval_result.result_count = result_count,
				.env_eval_result.results = results
			});
		env_eval_in_progress = false;
	} else {
		/* Failed to parse notification */
		LOG_ERR("%%ENVEVAL parsing failed, error: %d", ret);
	}
}

int env_eval(struct lte_lc_env_eval_params *params)
{
	int ret;
	enum lte_lc_func_mode mode;

	if (params == NULL || params->plmn_list == NULL) {
		return -EINVAL;
	}

	if (params->plmn_count == 0 ||
	    params->plmn_count > CONFIG_LTE_LC_ENV_EVAL_MAX_PLMN_COUNT) {
		LOG_ERR("Invalid PLMN count, check CONFIG_LTE_LC_ENV_EVAL_MAX_PLMN_COUNT");
		return -EINVAL;
	}

	ret = cfun_mode_get(&mode);
	if (ret) {
		LOG_ERR("Could not get functional mode");
		return -EFAULT;
	}

	switch (mode) {
	case LTE_LC_FUNC_MODE_RX_ONLY:
		break;
	default:
		LOG_ERR("Environment evaluation is only available in RX only mode");
		return -EOPNOTSUPP;
	}

	/* Construct PLMN string from params structure */
	char plmn_string[PLMN_ENTRY_SIZE * CONFIG_LTE_LC_ENV_EVAL_MAX_PLMN_COUNT] = {0};
	size_t offset = 0;

	for (size_t i = 0; i < params->plmn_count; i++) {
		if (params->plmn_list[i].mcc < 0 || params->plmn_list[i].mcc > 999 ||
		    params->plmn_list[i].mnc < 0 || params->plmn_list[i].mnc > 999) {
			return -EINVAL;
		}

		if (i > 0) {
			/* Add comma separator between PLMNs */
			__ASSERT_NO_MSG(offset < sizeof(plmn_string) - 1);
			plmn_string[offset] = ',';
			offset++;
		}

		/* Add quoted PLMN: "MCCMNC" */
		int written = snprintf(&plmn_string[offset], sizeof(plmn_string) - offset,
				      "\"%03d%02d\"",
				      params->plmn_list[i].mcc, params->plmn_list[i].mnc);
		__ASSERT_NO_MSG(written >= 0 && written < (int)(sizeof(plmn_string) - offset));

		offset += written;
	}

	/* Null-terminate the string */
	__ASSERT_NO_MSG(offset < sizeof(plmn_string));
	plmn_string[offset] = '\0';

	ret = nrf_modem_at_printf(AT_ENVEVAL, params->eval_type, plmn_string);
	if (ret) {
		LOG_ERR("AT command failed, error: %d", ret);
		return -EFAULT;
	}

	return 0;
}

int env_eval_cancel(void)
{
	int ret;

	ret = nrf_modem_at_printf(AT_ENVEVAL_CANCEL);
	if (ret) {
		LOG_ERR("AT command failed, error: %d", ret);
		return -EFAULT;
	}

	return 0;
}
