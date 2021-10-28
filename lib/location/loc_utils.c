/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr.h>
#include <logging/log.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <nrf_modem_at.h>
#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>

#include "loc_utils.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

#define AT_CMD_PDP_ACT_READ "AT+CGACT?"

bool loc_utils_is_default_pdn_active(void)
{
	char at_response_str[128];
	const char *p;
	int err;
	bool is_active = false;

	err = nrf_modem_at_cmd(at_response_str, sizeof(at_response_str), "%s", AT_CMD_PDP_ACT_READ);
	if (err) {
		LOG_ERR("Cannot get PDP contexts activation states, err: %d", err);
		return false;
	}

	/* Search for a string +CGACT: <cid>,<state> */
	p = strstr(at_response_str, "+CGACT: 0,1");
	if (p) {
		is_active = true;
	}
	return is_active;
}

int loc_utils_modem_params_read(struct loc_utils_modem_params_info *modem_params)
{
	char plmn_str[16] = { 0 };
	char tac_str[16] = { 0 };
	char cell_id_str[16] = { 0 };
	int err = 0;

	err = nrf_modem_at_scanf("AT%XMONITOR",
				 "%%XMONITOR: "
				 "%*d"          /* <reg_status>: ignored */
				 ",\"%*[^\"]\"" /* <full_name>: ignored */
				 ",\"%*[^\"]\"" /* <short_name>: ignored */
				 ",\"%[^\"]\""  /* <plmn> */
				 ",\"%[^\"]\""  /* <tac> */
				 ",%*d"         /* <AcT>: ignored */
				 ",%*d"         /* <band>: ignored */
				 ",\"%[^\"]\""  /* <cell_id> */
				 ",%d",         /* <phys_cell_id> */
				 plmn_str, tac_str, cell_id_str, &modem_params->phys_cell_id);
	if (err < 0) {
		LOG_ERR("Cannot get modem parameters, err %d", err);
	} else {
		/* Read MNC and store as integer. The MNC starts as the fourth character
		 * in the string, following three characters long MCC.
		 */
		modem_params->mnc = strtol(&plmn_str[3], NULL, 10);

		/* Null-terminated MCC, read and store it. */
		plmn_str[3] = '\0';

		modem_params->mcc = strtol(plmn_str, NULL, 10);

		/* <tac> */
		modem_params->tac = strtol(tac_str, NULL, 16);

		/* <cell_id> */
		modem_params->cell_id = strtol(cell_id_str, NULL, 16);

		LOG_DBG("parsed modem parameters: "
			"mcc %d, mnc %d, tac %d (string: %s), cell_id %d (string: %s) phys_cell_id %d",
			modem_params->mcc, modem_params->mnc, modem_params->tac,
			log_strdup(tac_str), modem_params->cell_id, log_strdup(cell_id_str),
			modem_params->phys_cell_id);

	}
	return err;
}
