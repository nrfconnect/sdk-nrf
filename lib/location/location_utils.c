/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <string.h>

#if defined(CONFIG_ARCH_POSIX) && defined(CONFIG_EXTERNAL_LIBC)
#include <time.h>
#else
#include <zephyr/posix/time.h>
#endif

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

#else /* CONFIG_NRF_MODEM_LIB */

bool location_utils_is_lte_available(void)
{
	return false;
}

#endif /* CONFIG_NRF_MODEM_LIB */

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
