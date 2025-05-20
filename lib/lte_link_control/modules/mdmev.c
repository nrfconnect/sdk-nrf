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
#include "modules/mdmev.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

/* MDMEV command parameters */
#define AT_MDMEV_ENABLE_1	 "AT%%MDMEV=1"
#define AT_MDMEV_ENABLE_2	 "AT%%MDMEV=2"
#define AT_MDMEV_DISABLE	 "AT%%MDMEV=0"
#define AT_MDMEV_RESPONSE_PREFIX "%MDMEV: "
#define AT_MDMEV_OVERHEATED	 "ME OVERHEATED\r\n"
#define AT_MDMEV_BATTERY_LOW	 "ME BATTERY LOW\r\n"
#define AT_MDMEV_SEARCH_STATUS_1 "SEARCH STATUS 1\r\n"
#define AT_MDMEV_SEARCH_STATUS_2 "SEARCH STATUS 2\r\n"
#define AT_MDMEV_RESET_LOOP	 "RESET LOOP\r\n"
#define AT_MDMEV_NO_IMEI	 "NO IMEI\r\n"
#define AT_MDMEV_CE_LEVEL_0	 "PRACH CE-LEVEL 0\r\n"
#define AT_MDMEV_CE_LEVEL_1	 "PRACH CE-LEVEL 1\r\n"
#define AT_MDMEV_CE_LEVEL_2	 "PRACH CE-LEVEL 2\r\n"
#define AT_MDMEV_CE_LEVEL_3	 "PRACH CE-LEVEL 3\r\n"

AT_MONITOR(ltelc_atmon_mdmev, "%MDMEV", at_handler_mdmev);

#if defined(CONFIG_UNITY)
int mdmev_parse(const char *at_response, enum lte_lc_modem_evt *modem_evt)
#else
static int mdmev_parse(const char *at_response, enum lte_lc_modem_evt *modem_evt)
#endif /* CONFIG_UNITY */
{
	static const char *const event_types[] = {
		[LTE_LC_MODEM_EVT_LIGHT_SEARCH_DONE] = AT_MDMEV_SEARCH_STATUS_1,
		[LTE_LC_MODEM_EVT_SEARCH_DONE] = AT_MDMEV_SEARCH_STATUS_2,
		[LTE_LC_MODEM_EVT_RESET_LOOP] = AT_MDMEV_RESET_LOOP,
		[LTE_LC_MODEM_EVT_BATTERY_LOW] = AT_MDMEV_BATTERY_LOW,
		[LTE_LC_MODEM_EVT_OVERHEATED] = AT_MDMEV_OVERHEATED,
		[LTE_LC_MODEM_EVT_NO_IMEI] = AT_MDMEV_NO_IMEI,
		[LTE_LC_MODEM_EVT_CE_LEVEL_0] = AT_MDMEV_CE_LEVEL_0,
		[LTE_LC_MODEM_EVT_CE_LEVEL_1] = AT_MDMEV_CE_LEVEL_1,
		[LTE_LC_MODEM_EVT_CE_LEVEL_2] = AT_MDMEV_CE_LEVEL_2,
		[LTE_LC_MODEM_EVT_CE_LEVEL_3] = AT_MDMEV_CE_LEVEL_3,
	};

	__ASSERT_NO_MSG(at_response != NULL);
	__ASSERT_NO_MSG(modem_evt != NULL);

	const char *start_ptr = at_response + sizeof(AT_MDMEV_RESPONSE_PREFIX) - 1;

	for (size_t i = 0; i < ARRAY_SIZE(event_types); i++) {
		if (strcmp(event_types[i], start_ptr) == 0) {
			LOG_DBG("Occurrence found: %s", event_types[i]);
			*modem_evt = i;

			return 0;
		}
	}

	LOG_DBG("No modem event type found: %s", at_response);

	return -ENODATA;
}

static void at_handler_mdmev(const char *response)
{
	int err;
	struct lte_lc_evt evt = {0};

	__ASSERT_NO_MSG(response != NULL);

	LOG_DBG("%%MDMEV notification");

	err = mdmev_parse(response, &evt.modem_evt);
	if (err) {
		LOG_ERR("Can't parse modem event notification, error: %d", err);
		return;
	}

	evt.type = LTE_LC_EVT_MODEM_EVENT;

	event_handler_list_dispatch(&evt);
}

int mdmev_enable(void)
{
	/* First try to enable both warning and informational type events, which is only supported
	 * by modem firmware versions >= 2.0.0.
	 * If that fails, try to enable the legacy set of events.
	 */
	if (nrf_modem_at_printf(AT_MDMEV_ENABLE_2)) {
		if (nrf_modem_at_printf(AT_MDMEV_ENABLE_1)) {
			return -EFAULT;
		}
	}

	return 0;
}

int mdmev_disable(void)
{
	return nrf_modem_at_printf(AT_MDMEV_DISABLE) ? -EFAULT : 0;
}
