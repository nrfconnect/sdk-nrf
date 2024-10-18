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
#include "modules/xt3412.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

AT_MONITOR(ltelc_atmon_xt3412, "%XT3412", at_handler_xt3412);

/* XT3412 command parameters */
#define AT_XT3412_SUB	     "AT%%XT3412=1,%d,%d"
#define AT_XT3412_TIME_INDEX 1
#define T3412_MAX	     35712000000

#if defined(CONFIG_UNITY)
int parse_xt3412(const char *at_response, uint64_t *time)
#else
static int parse_xt3412(const char *at_response, uint64_t *time)
#endif /* CONFIG_UNITY */
{
	int err;
	struct at_parser parser;

	__ASSERT_NO_MSG(at_response != NULL);
	__ASSERT_NO_MSG(time != NULL);

	err = at_parser_init(&parser, at_response);
	__ASSERT_NO_MSG(err == 0);

	/* Get the remaining time of T3412 from the response */
	err = at_parser_num_get(&parser, AT_XT3412_TIME_INDEX, time);
	if (err) {
		if (err == -ERANGE) {
			err = -EINVAL;
		}
		LOG_ERR("Could not get time until next TAU, error: %d", err);
		goto clean_exit;
	}

	if ((*time > T3412_MAX) || *time < 0) {
		LOG_WRN("Parsed time parameter not within valid range");
		err = -EINVAL;
	}

clean_exit:
	return err;
}

static void at_handler_xt3412(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};

	__ASSERT_NO_MSG(response != NULL);

	LOG_DBG("%%XT3412 notification");

	err = parse_xt3412(response, &evt.time);
	if (err) {
		LOG_ERR("Can't parse TAU pre-warning notification, error: %d", err);
		return;
	}

	if (evt.time != CONFIG_LTE_LC_TAU_PRE_WARNING_TIME_MS) {
		/* Only propagate TAU pre-warning notifications when the received time
		 * parameter is the duration of the set pre-warning time.
		 */
		return;
	}

	evt.type = LTE_LC_EVT_TAU_PRE_WARNING;

	event_handler_list_dispatch(&evt);
}

int xt3412_notifications_enable(void)
{
	int err;

	if (IS_ENABLED(CONFIG_LTE_LC_TAU_PRE_WARNING_NOTIFICATIONS)) {
		err = nrf_modem_at_printf(AT_XT3412_SUB, CONFIG_LTE_LC_TAU_PRE_WARNING_TIME_MS,
					  CONFIG_LTE_LC_TAU_PRE_WARNING_THRESHOLD_MS);
		if (err) {
			LOG_WRN("Enabling TAU pre-warning notifications failed, error: %d", err);
			LOG_WRN("TAU pre-warning notifications require nRF9160 modem >= v1.3.0");
			return err;
		}
	}

	return 0;
}
