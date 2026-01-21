/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <modem/lte_lc.h>
#include <modem/at_monitor.h>
#include <nrf_modem_at.h>

#include "common/event_handler_list.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

#define AT_CMD_CELLULAR_PROFILE_NOTIF_SUBSCRIBE   "AT%%CELLULARPRFL=1"
#define AT_CMD_CELLULAR_PROFILE_CONFIGURE         "AT%%CELLULARPRFL=2,%d,%d,%d"
#define AT_CMD_CELLULAR_PROFILE_REMOVE            "AT%%CELLULARPRFL=2,%d"

#ifndef CONFIG_UNITY
AT_MONITOR(lte_lc_cellular_profile_notif, "%CELLULARPRFL", on_cellularprfl);
#endif

#ifdef CONFIG_UNITY
void on_cellularprfl(const char *notif)
#else
static void on_cellularprfl(const char *notif)
#endif
{
	char *p;
	int8_t cp_id;
	struct lte_lc_evt evt;

	p = strstr(notif, "%CELLULARPRFL: ");
	if (!p) {
		return;
	}

	p += sizeof("%CELLULARPRFL: ") - 1;

	cp_id = (int8_t)strtoul(p, &p, 10);
	if (cp_id < 0 || cp_id > 1) {
		LOG_ERR("Invalid cellular profile ID: %d", cp_id);

		return;
	}

	evt.type = LTE_LC_EVT_CELLULAR_PROFILE_ACTIVE;
	evt.cellular_profile.profile_id = cp_id;

	event_handler_list_dispatch(&evt);
}

static int cellular_profile_act_is_valid(uint8_t act)
{
	uint8_t mask = LTE_LC_ACT_LTEM |
		       LTE_LC_ACT_NBIOT |
		       LTE_LC_ACT_NTN;

	if ((act == 0) || (act & ~mask)) {
		return -EINVAL;
	}

	return 0;
}

int lte_lc_cellular_profile_configure(struct lte_lc_cellular_profile *profile)
{
	int err;

	if (!profile) {
		return -EINVAL;
	}

	err = cellular_profile_act_is_valid(profile->act);
	if (err) {
		LOG_ERR("Invalid AcT bitmap for cellular profile: 0x%x", profile->act);

		return -EINVAL;
	}

	err = nrf_modem_at_printf(AT_CMD_CELLULAR_PROFILE_CONFIGURE,
				  profile->id,
				  profile->act,
				  profile->uicc);
	if (err) {
		LOG_ERR("Failed to configure cellular profile, err %d", err);

		return err;
	}

	err = nrf_modem_at_printf(AT_CMD_CELLULAR_PROFILE_NOTIF_SUBSCRIBE);
	if (err) {
		LOG_ERR("Failed to subscribe to cellular profile notifications, err %d", err);
	}

	return 0;
}

int lte_lc_cellular_profile_remove(uint8_t id)
{
	int err;

	err = nrf_modem_at_printf(AT_CMD_CELLULAR_PROFILE_REMOVE, id);
	if (err) {
		LOG_ERR("Failed to remove cellular profile, err %d", err);

		return err;
	}

	return 0;
}
