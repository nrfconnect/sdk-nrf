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

#include <nrf_modem_at.h>
#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>
#include <net/nrf_cloud.h>

#include "location_utils.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

#define AT_CMD_PDP_ACT_READ "AT+CGACT?"
#define MODEM_PARAM_STR_MAX_LEN 16

/* For having a numeric constant in scanf string length */
#define L_(x) #x
#define L(x) L_(x)

char jwt_buf[600];

bool location_utils_is_default_pdn_active(void)
{
	char at_response_str[128];
	const char *p;
	int err;
	bool is_active = false;

	err = nrf_modem_at_cmd(at_response_str, sizeof(at_response_str), AT_CMD_PDP_ACT_READ);
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

int location_utils_modem_params_read(struct location_utils_modem_params_info *modem_params)
{
	/* Parsed strings include double quotes */
	char plmn_str[MODEM_PARAM_STR_MAX_LEN + 1] = { 0 };
	char tac_str[MODEM_PARAM_STR_MAX_LEN + 1] = { 0 };
	char cell_id_str[MODEM_PARAM_STR_MAX_LEN + 1] = { 0 };
	int err = 0;

	__ASSERT_NO_MSG(modem_params != NULL);

	err = nrf_modem_at_scanf(
		"AT%XMONITOR",
		"%%XMONITOR: "
		"%*d"                                 /* <reg_status>: ignored */
		",%*[^,]"                             /* <full_name>: ignored */
		",%*[^,]"                             /* <short_name>: ignored */
		",%"L(MODEM_PARAM_STR_MAX_LEN)"[^,]"  /* <plmn> */
		",%"L(MODEM_PARAM_STR_MAX_LEN)"[^,]"  /* <tac> */
		",%*d"                                /* <AcT>: ignored */
		",%*d"                                /* <band>: ignored */
		",%"L(MODEM_PARAM_STR_MAX_LEN)"[^,]"  /* <cell_id> */
		",%d",                                /* <phys_cell_id> */
		plmn_str, tac_str, cell_id_str, &modem_params->phys_cell_id);

	if (err <= 2) {
		LOG_ERR("Cannot get modem parameters, err %d", err);
	} else {
		/* Indicate success to the caller with zero return value */
		err = 0;

		/* Read MNC and store as integer. The MNC starts as the fifth character
		 * in the string, following double quote and three characters long MCC.
		 */
		modem_params->mnc = strtol(&plmn_str[4], NULL, 10);

		/* Null-terminated MCC, read and store it. */
		plmn_str[4] = '\0';

		modem_params->mcc = strtol(plmn_str + 1, NULL, 10);

		/* <tac> */
		modem_params->tac = strtol(tac_str + 1, NULL, 16);

		/* <cell_id> */
		modem_params->cell_id = strtol(cell_id_str + 1, NULL, 16);

		LOG_DBG("parsed modem parameters: "
			"mcc %d, mnc %d, tac %d (string: %s), cell_id %d (string: %s) phys_cell_id %d",
			modem_params->mcc, modem_params->mnc, modem_params->tac,
			log_strdup(tac_str), modem_params->cell_id, log_strdup(cell_id_str),
			modem_params->phys_cell_id);
	}
	return err;
}

const char *location_utils_nrf_cloud_jwt_generate(void)
{
	int err = nrf_cloud_jwt_generate(0, jwt_buf, sizeof(jwt_buf));

	if (err) {
		LOG_ERR("Failed to generate JWT, error: %d", err);
		return NULL;
	}

	return jwt_buf;
}
