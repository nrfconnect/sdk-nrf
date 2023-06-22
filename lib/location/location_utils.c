/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/posix/time.h>

#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>
#include <net/nrf_cloud.h>
#include <modem/location.h>

#include "location_utils.h"

LOG_MODULE_DECLARE(location, CONFIG_LOCATION_LOG_LEVEL);

#define AT_CMD_PDP_ACT_READ "AT+CGACT?"
#define MODEM_PARAM_STR_MAX_LEN 16

/* For having a numeric constant in scanf string length */
#define L_(x) #x
#define L(x) L_(x)

char jwt_buf[600];

#if defined(CONFIG_NRF_MODEM_LIB)
#include <nrf_modem_at.h>

bool location_utils_is_lte_available(void)
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

	/* Check whether a PDN bearer is active by searching for a string +CGACT: <cid>,<state> */
	p = strstr(at_response_str, "+CGACT: 0,1");
	if (p) {
		/* If it is, then LTE networking is likely available. */
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
			tac_str, modem_params->cell_id, cell_id_str,
			modem_params->phys_cell_id);
	}
	return err;
}

#else /* CONFIG_NRF_MODEM_LIB */

bool location_utils_is_lte_available(void)
{
	return false;
}

int location_utils_modem_params_read(struct location_utils_modem_params_info *modem_params)
{
	return 0;
}

#endif /* CONFIG_NRF_MODEM_LIB*/

const char *location_utils_nrf_cloud_jwt_generate(void)
{
	int err = nrf_cloud_jwt_generate(0, jwt_buf, sizeof(jwt_buf));

	if (err) {
		LOG_ERR("Failed to generate JWT, error: %d", err);
		return NULL;
	}

	return jwt_buf;
}

void location_utils_systime_to_location_datetime(struct location_datetime *datetime)
{
	struct timespec tp;
	struct tm ltm = { 0 };

	__ASSERT_NO_MSG(datetime != NULL);

	clock_gettime(CLOCK_REALTIME, &tp);
	gmtime_r(&tp.tv_sec, &ltm);

	/* System time should have been set when date_time lib is in use */
	if (IS_ENABLED(CONFIG_DATE_TIME)) {
		datetime->valid = true;
	} else {
		datetime->valid = false;
	}

	/* Relative to 1900, as per POSIX */
	datetime->year = 1900 + ltm.tm_year;

	/* Range is 0-11, as per POSIX */
	datetime->month = ltm.tm_mon + 1;

	datetime->day = ltm.tm_mday;
	datetime->hour = ltm.tm_hour;
	datetime->minute = ltm.tm_min;
	datetime->second = ltm.tm_sec;
	datetime->ms = tp.tv_nsec / 1000000;
}
