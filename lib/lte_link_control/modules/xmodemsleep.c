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
#include "modules/xmodemsleep.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

/* XMODEMSLEEP command parameters. */
#define AT_XMODEMSLEEP_SUB		"AT%%XMODEMSLEEP=1,%d,%d"
#define AT_XMODEMSLEEP_PARAMS_COUNT_MAX 4
#define AT_XMODEMSLEEP_TYPE_INDEX	1
#define AT_XMODEMSLEEP_TIME_INDEX	2

AT_MONITOR(ltelc_atmon_xmodemsleep, "%XMODEMSLEEP", at_handler_xmodemsleep);

#if defined(CONFIG_UNITY)
int parse_xmodemsleep(const char *at_response, struct lte_lc_modem_sleep *modem_sleep)
#else
static int parse_xmodemsleep(const char *at_response, struct lte_lc_modem_sleep *modem_sleep)
#endif /* CONFIG_UNITY */
{
	int err;
	struct at_parser parser;
	uint16_t type;
	size_t count = 0;

	__ASSERT_NO_MSG(at_response != NULL);
	__ASSERT_NO_MSG(modem_sleep != NULL);

	err = at_parser_init(&parser, at_response);
	__ASSERT_NO_MSG(err == 0);

	err = at_parser_num_get(&parser, AT_XMODEMSLEEP_TYPE_INDEX, &type);
	if (err) {
		LOG_ERR("Could not get mode sleep type, error: %d", err);
		goto clean_exit;
	}
	modem_sleep->type = type;

	err = at_parser_cmd_count_get(&parser, &count);
	if (err) {
		LOG_ERR("Could not get XMODEMSLEEP param count, "
			"potentially malformed notification, error: %d",
			err);
		goto clean_exit;
	}

	if (count < AT_XMODEMSLEEP_PARAMS_COUNT_MAX - 1) {
		modem_sleep->time = -1;
		goto clean_exit;
	}

	err = at_parser_num_get(&parser, AT_XMODEMSLEEP_TIME_INDEX, &modem_sleep->time);
	if (err) {
		LOG_ERR("Could not get time until next modem sleep, error: %d", err);
		goto clean_exit;
	}

clean_exit:
	return err;
}

static void at_handler_xmodemsleep(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};

	__ASSERT_NO_MSG(response != NULL);

	LOG_DBG("%%XMODEMSLEEP notification");

	err = parse_xmodemsleep(response, &evt.modem_sleep);
	if (err) {
		LOG_ERR("Can't parse modem sleep pre-warning notification, error: %d", err);
		return;
	}

	/* Link controller only supports PSM, RF inactivity, limited service, flight mode
	 * and proprietary PSM modem sleep types.
	 */
	if ((evt.modem_sleep.type != LTE_LC_MODEM_SLEEP_PSM) &&
	    (evt.modem_sleep.type != LTE_LC_MODEM_SLEEP_RF_INACTIVITY) &&
	    (evt.modem_sleep.type != LTE_LC_MODEM_SLEEP_LIMITED_SERVICE) &&
	    (evt.modem_sleep.type != LTE_LC_MODEM_SLEEP_FLIGHT_MODE) &&
	    (evt.modem_sleep.type != LTE_LC_MODEM_SLEEP_PROPRIETARY_PSM)) {
		return;
	}

	/* Propagate the appropriate event depending on the parsed time parameter. */
	if (evt.modem_sleep.time == CONFIG_LTE_LC_MODEM_SLEEP_PRE_WARNING_TIME_MS) {
		evt.type = LTE_LC_EVT_MODEM_SLEEP_EXIT_PRE_WARNING;
	} else if (evt.modem_sleep.time == 0) {
		LTE_LC_TRACE(LTE_LC_TRACE_MODEM_SLEEP_EXIT);

		evt.type = LTE_LC_EVT_MODEM_SLEEP_EXIT;
	} else {
		LTE_LC_TRACE(LTE_LC_TRACE_MODEM_SLEEP_ENTER);

		evt.type = LTE_LC_EVT_MODEM_SLEEP_ENTER;
	}

	event_handler_list_dispatch(&evt);
}

int xmodemsleep_notifications_enable(void)
{
	int err;

	if (IS_ENABLED(CONFIG_LTE_LC_MODEM_SLEEP_NOTIFICATIONS)) {
		/* %XMODEMSLEEP notifications subscribe */
		err = nrf_modem_at_printf(AT_XMODEMSLEEP_SUB,
					  CONFIG_LTE_LC_MODEM_SLEEP_PRE_WARNING_TIME_MS,
					  CONFIG_LTE_LC_MODEM_SLEEP_NOTIFICATIONS_THRESHOLD_MS);
		if (err) {
			LOG_WRN("Enabling modem sleep notifications failed, error: %d", err);
			LOG_WRN("Modem sleep notifications require nRF9160 modem >= v1.3.0");
			return err;
		}
	}

	return 0;
}
