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
#include <nrf_modem_at.h>

#include "common/event_handler_list.h"
#include "common/helpers.h"
#include "modules/cfun.h"
#include "modules/coneval.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

/* CONEVAL command parameters */
#define AT_CONEVAL_READ			  "AT%CONEVAL"
#define AT_CONEVAL_PARAMS_MAX		  19
#define AT_CONEVAL_RESULT_INDEX		  1
#define AT_CONEVAL_RRC_STATE_INDEX	  2
#define AT_CONEVAL_ENERGY_ESTIMATE_INDEX  3
#define AT_CONEVAL_RSRP_INDEX		  4
#define AT_CONEVAL_RSRQ_INDEX		  5
#define AT_CONEVAL_SNR_INDEX		  6
#define AT_CONEVAL_CELL_ID_INDEX	  7
#define AT_CONEVAL_PLMN_INDEX		  8
#define AT_CONEVAL_PHYSICAL_CELL_ID_INDEX 9
#define AT_CONEVAL_EARFCN_INDEX		  10
#define AT_CONEVAL_BAND_INDEX		  11
#define AT_CONEVAL_TAU_TRIGGERED_INDEX	  12
#define AT_CONEVAL_CE_LEVEL_INDEX	  13
#define AT_CONEVAL_TX_POWER_INDEX	  14
#define AT_CONEVAL_TX_REPETITIONS_INDEX	  15
#define AT_CONEVAL_RX_REPETITIONS_INDEX	  16
#define AT_CONEVAL_DL_PATHLOSS_INDEX	  17

int coneval_params_get(struct lte_lc_conn_eval_params *params)
{
	int err;
	enum lte_lc_func_mode mode;
	int result;
	/* PLMN field is a string of maximum 6 characters, plus null termination. */
	char plmn_str[7] = {0};
	uint16_t rrc_state_tmp, energy_estimate_tmp, tau_trig_tmp, ce_level_tmp;

	if (params == NULL) {
		return -EINVAL;
	}

	err = cfun_mode_get(&mode);
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
	err = nrf_modem_at_scanf(AT_CONEVAL_READ,
				 "%%CONEVAL: "
				 "%d,"		/* <result> */
				 "%hu,"		/* <rrc_state> */
				 "%hu,"		/* <energy_estimate> */
				 "%hd,"		/* <rsrp> */
				 "%hd,"		/* <rsrq> */
				 "%hd,"		/* <snr> */
				 "\"%x\","	/* <cell_id> */
				 "\"%6[^\"]\"," /* <plmn> */
				 "%hd,"		/* <phys_cell_id> */
				 "%d,"		/* <earfcn> */
				 "%hd,"		/* <band> */
				 "%hu,"		/* <tau_triggered> */
				 "%hu,"		/* <ce_level> */
				 "%hd,"		/* <tx_power> */
				 "%hd,"		/* <tx_repetitions> */
				 "%hd,"		/* <rx_repetitions> */
				 "%hd",		/* <dl-pathloss> */
				 &result, &rrc_state_tmp, &energy_estimate_tmp, &params->rsrp,
				 &params->rsrq, &params->snr, &params->cell_id, plmn_str,
				 &params->phy_cid, &params->earfcn, &params->band, &tau_trig_tmp,
				 &ce_level_tmp, &params->tx_power, &params->tx_rep, &params->rx_rep,
				 &params->dl_pathloss);
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

	params->rrc_state = rrc_state_tmp;
	params->energy_estimate = energy_estimate_tmp;
	params->tau_trig = tau_trig_tmp;
	params->ce_level = ce_level_tmp;

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
