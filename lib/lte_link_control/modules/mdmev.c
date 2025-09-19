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

/* %MDMEV commands */
#define AT_MDMEV_ENABLE_1	"AT%%MDMEV=1"
#define AT_MDMEV_ENABLE_2	"AT%%MDMEV=2"
#define AT_MDMEV_DISABLE	"AT%%MDMEV=0"
#define AT_MDMEV_NOTIF_PREFIX	"%MDMEV: "

/* Fixed events */
#define AT_MDMEV_OVERHEATED		"ME OVERHEATED\r\n"
#define AT_MDMEV_BATTERY_LOW		"ME BATTERY LOW\r\n"
#define AT_MDMEV_SEARCH_STATUS_1	"SEARCH STATUS 1\r\n"
#define AT_MDMEV_SEARCH_STATUS_2	"SEARCH STATUS 2\r\n"
#define AT_MDMEV_RESET_LOOP		"RESET LOOP\r\n"
#define AT_MDMEV_NO_IMEI		"NO IMEI\r\n"
#define AT_MDMEV_RF_CAL_NOT_DONE	"RF CALIBRATION NOT DONE\r\n"

/* Events with values */
#define AT_MDMEV_CE_LEVEL		"PRACH CE-LEVEL %u\r\n"
#define AT_MDMEV_INVALID_BAND_CONF	"INVALID BAND CONFIGURATION %u %u %u\r\n"
#define AT_MDMEV_DETECTED_COUNTRY	"DETECTED COUNTRY %u\r\n"

AT_MONITOR(ltelc_atmon_mdmev, "%MDMEV", at_handler_mdmev);

bool mdmev_enabled;

static int mdmev_fixed_parse(const char *notif, struct lte_lc_modem_evt *modem_evt)
{
	struct event_type_map {
		enum lte_lc_modem_evt_type type;
		const char *notif;
	};
	static const struct event_type_map event_types[] = {
		{ LTE_LC_MODEM_EVT_LIGHT_SEARCH_DONE, AT_MDMEV_SEARCH_STATUS_1 },
		{ LTE_LC_MODEM_EVT_SEARCH_DONE, AT_MDMEV_SEARCH_STATUS_2 },
		{ LTE_LC_MODEM_EVT_RESET_LOOP, AT_MDMEV_RESET_LOOP },
		{ LTE_LC_MODEM_EVT_BATTERY_LOW, AT_MDMEV_BATTERY_LOW },
		{ LTE_LC_MODEM_EVT_OVERHEATED, AT_MDMEV_OVERHEATED },
		{ LTE_LC_MODEM_EVT_NO_IMEI, AT_MDMEV_NO_IMEI },
		{ LTE_LC_MODEM_EVT_RF_CAL_NOT_DONE, AT_MDMEV_RF_CAL_NOT_DONE },
		{ 0, NULL }
	};

	__ASSERT_NO_MSG(notif != NULL);
	__ASSERT_NO_MSG(modem_evt != NULL);

	for (size_t i = 0; i < ARRAY_SIZE(event_types); i++) {
		if (event_types[i].notif == NULL) {
			break;
		}

		if (strcmp(event_types[i].notif, notif) == 0) {
			modem_evt->type = event_types[i].type;

			return 0;
		}
	}

	return -ENODATA;
}

static int mdmev_value_parse(const char *notif, struct lte_lc_modem_evt *modem_evt)
{
	uint32_t value1;
	uint32_t value2;
	uint32_t value3;

	__ASSERT_NO_MSG(notif != NULL);
	__ASSERT_NO_MSG(modem_evt != NULL);

	if (sscanf(notif, AT_MDMEV_CE_LEVEL, &value1) == 1) {
		modem_evt->type = LTE_LC_MODEM_EVT_CE_LEVEL;
		modem_evt->ce_level = value1;
		return 0;
	}

	/* Default value for NTN NB-IoT when it's not supported by the modem firmware. */
	value3 = LTE_LC_BAND_CONF_STATUS_SYSTEM_NOT_SUPPORTED;

	/* At least 2 values are expected (LTE-M and NB-IoT). */
	if (sscanf(notif, AT_MDMEV_INVALID_BAND_CONF, &value1, &value2, &value3) >= 2) {
		modem_evt->type = LTE_LC_MODEM_EVT_INVALID_BAND_CONF;
		modem_evt->invalid_band_conf.status_ltem = value1;
		modem_evt->invalid_band_conf.status_nbiot = value2;
		modem_evt->invalid_band_conf.status_ntn_nbiot = value3;
		return 0;
	}

	if (sscanf(notif, AT_MDMEV_DETECTED_COUNTRY, &value1) == 1) {
		modem_evt->type = LTE_LC_MODEM_EVT_DETECTED_COUNTRY;
		modem_evt->detected_country = value1;
		return 0;
	}

	return -ENODATA;
}

static void at_handler_mdmev(const char *notif)
{
	int err;
	struct lte_lc_evt evt = {
		.type = LTE_LC_EVT_MODEM_EVENT
	};

	__ASSERT_NO_MSG(notif != NULL);

	/* Remove the notification prefix. */
	const char *start_ptr = notif + sizeof(AT_MDMEV_NOTIF_PREFIX) - 1;

	LOG_DBG("%%MDMEV notification: %.*s", (int)(strlen(start_ptr) - strlen("\r\n")), start_ptr);

	/* Try to parse fixed events. */
	err = mdmev_fixed_parse(start_ptr, &evt.modem_evt);
	if (err) {
		/* Try to parse events with values. */
		err = mdmev_value_parse(start_ptr, &evt.modem_evt);
		if (err) {
			LOG_DBG("No modem event type found: %s", notif);
			return;
		}
	}

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

	mdmev_enabled = true;

	return 0;
}

int mdmev_disable(void)
{
	mdmev_enabled = false;

	return nrf_modem_at_printf(AT_MDMEV_DISABLE) ? -EFAULT : 0;
}
