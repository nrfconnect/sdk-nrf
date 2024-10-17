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

#include "modules/xsystemmode.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

#define AT_XSYSTEMMODE_READ "AT%XSYSTEMMODE?"

/* The indices are for the set command. Add 1 for the read command indices. */
#define AT_XSYSTEMMODE_READ_LTEM_INDEX	     1
#define AT_XSYSTEMMODE_READ_NBIOT_INDEX	     2
#define AT_XSYSTEMMODE_READ_GPS_INDEX	     3
#define AT_XSYSTEMMODE_READ_PREFERENCE_INDEX 4

/* Internal system mode value used when CONFIG_LTE_NETWORK_MODE_DEFAULT is enabled. */
#define LTE_LC_SYSTEM_MODE_DEFAULT 0xff

#define SYS_MODE_PREFERRED                                                                         \
	(IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M)	       ? LTE_LC_SYSTEM_MODE_LTEM           \
	 : IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT)	       ? LTE_LC_SYSTEM_MODE_NBIOT          \
	 : IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_GPS)       ? LTE_LC_SYSTEM_MODE_LTEM_GPS       \
	 : IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT_GPS)       ? LTE_LC_SYSTEM_MODE_NBIOT_GPS      \
	 : IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT)     ? LTE_LC_SYSTEM_MODE_LTEM_NBIOT     \
	 : IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS) ? LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS \
							       : LTE_LC_SYSTEM_MODE_DEFAULT)

/* The preferred system mode to use when connecting to LTE network. Can be changed by calling
 * lte_lc_system_mode_set().
 *
 * extern in lte_lc_modem_hooks.c
 */
enum lte_lc_system_mode lte_lc_sys_mode = SYS_MODE_PREFERRED;
/* System mode preference to set when configuring system mode. Can be changed by calling
 * lte_lc_system_mode_set().
 *
 * extern in lte_lc_modem_hooks.c
 */
enum lte_lc_system_mode_preference lte_lc_sys_mode_pref = CONFIG_LTE_MODE_PREFERENCE_VALUE;

/* Parameters to be passed using AT%XSYSTEMMMODE=<params>,<preference> */
static const char *const system_mode_params[] = {
	[LTE_LC_SYSTEM_MODE_LTEM] = "1,0,0",
	[LTE_LC_SYSTEM_MODE_NBIOT] = "0,1,0",
	[LTE_LC_SYSTEM_MODE_GPS] = "0,0,1",
	[LTE_LC_SYSTEM_MODE_LTEM_GPS] = "1,0,1",
	[LTE_LC_SYSTEM_MODE_NBIOT_GPS] = "0,1,1",
	[LTE_LC_SYSTEM_MODE_LTEM_NBIOT] = "1,1,0",
	[LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS] = "1,1,1",
};

/* LTE preference to be passed using AT%XSYSTEMMMODE=<params>,<preference> */
static const char system_mode_preference[] = {
	/* No LTE preference, automatically selected by the modem. */
	[LTE_LC_SYSTEM_MODE_PREFER_AUTO] = '0',
	/* LTE-M has highest priority. */
	[LTE_LC_SYSTEM_MODE_PREFER_LTEM] = '1',
	/* NB-IoT has highest priority. */
	[LTE_LC_SYSTEM_MODE_PREFER_NBIOT] = '2',
	/* Equal priority, but prefer LTE-M. */
	[LTE_LC_SYSTEM_MODE_PREFER_LTEM_PLMN_PRIO] = '3',
	/* Equal priority, but prefer NB-IoT. */
	[LTE_LC_SYSTEM_MODE_PREFER_NBIOT_PLMN_PRIO] = '4',
};

int xsystemmode_mode_set(enum lte_lc_system_mode mode,
			 enum lte_lc_system_mode_preference preference)
{
	int err;

	switch (mode) {
	case LTE_LC_SYSTEM_MODE_LTEM:
	case LTE_LC_SYSTEM_MODE_LTEM_GPS:
	case LTE_LC_SYSTEM_MODE_NBIOT:
	case LTE_LC_SYSTEM_MODE_NBIOT_GPS:
	case LTE_LC_SYSTEM_MODE_GPS:
	case LTE_LC_SYSTEM_MODE_LTEM_NBIOT:
	case LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS:
		break;
	default:
		LOG_ERR("Invalid system mode requested: %d", mode);
		return -EINVAL;
	}

	switch (preference) {
	case LTE_LC_SYSTEM_MODE_PREFER_AUTO:
	case LTE_LC_SYSTEM_MODE_PREFER_LTEM:
	case LTE_LC_SYSTEM_MODE_PREFER_NBIOT:
	case LTE_LC_SYSTEM_MODE_PREFER_LTEM_PLMN_PRIO:
	case LTE_LC_SYSTEM_MODE_PREFER_NBIOT_PLMN_PRIO:
		break;
	default:
		LOG_ERR("Invalid LTE preference requested: %d", preference);
		return -EINVAL;
	}

	err = nrf_modem_at_printf("AT%%XSYSTEMMODE=%s,%c", system_mode_params[mode],
				  system_mode_preference[preference]);
	if (err) {
		LOG_ERR("Could not send AT command, error: %d", err);
		return -EFAULT;
	}

	lte_lc_sys_mode = mode;
	lte_lc_sys_mode_pref = preference;

	LOG_DBG("System mode set to %d, preference %d", lte_lc_sys_mode, lte_lc_sys_mode_pref);

	return 0;
}

int xsystemmode_mode_get(enum lte_lc_system_mode *mode,
			 enum lte_lc_system_mode_preference *preference)
{
	int err;
	int mode_bitmask = 0;
	int ltem_mode = 0;
	int nbiot_mode = 0;
	int gps_mode = 0;
	int mode_preference = 0;

	if (mode == NULL) {
		return -EINVAL;
	}

	/* It's expected to have all 4 arguments matched */
	err = nrf_modem_at_scanf(AT_XSYSTEMMODE_READ, "%%XSYSTEMMODE: %d,%d,%d,%d", &ltem_mode,
				 &nbiot_mode, &gps_mode, &mode_preference);
	if (err != 4) {
		LOG_ERR("Failed to get system mode, error: %d", err);
		return -EFAULT;
	}

	mode_bitmask = (ltem_mode ? BIT(AT_XSYSTEMMODE_READ_LTEM_INDEX) : 0) |
		       (nbiot_mode ? BIT(AT_XSYSTEMMODE_READ_NBIOT_INDEX) : 0) |
		       (gps_mode ? BIT(AT_XSYSTEMMODE_READ_GPS_INDEX) : 0);

	switch (mode_bitmask) {
	case BIT(AT_XSYSTEMMODE_READ_LTEM_INDEX):
		*mode = LTE_LC_SYSTEM_MODE_LTEM;
		break;
	case BIT(AT_XSYSTEMMODE_READ_NBIOT_INDEX):
		*mode = LTE_LC_SYSTEM_MODE_NBIOT;
		break;
	case BIT(AT_XSYSTEMMODE_READ_GPS_INDEX):
		*mode = LTE_LC_SYSTEM_MODE_GPS;
		break;
	case (BIT(AT_XSYSTEMMODE_READ_LTEM_INDEX) | BIT(AT_XSYSTEMMODE_READ_GPS_INDEX)):
		*mode = LTE_LC_SYSTEM_MODE_LTEM_GPS;
		break;
	case (BIT(AT_XSYSTEMMODE_READ_NBIOT_INDEX) | BIT(AT_XSYSTEMMODE_READ_GPS_INDEX)):
		*mode = LTE_LC_SYSTEM_MODE_NBIOT_GPS;
		break;
	case (BIT(AT_XSYSTEMMODE_READ_LTEM_INDEX) | BIT(AT_XSYSTEMMODE_READ_NBIOT_INDEX)):
		*mode = LTE_LC_SYSTEM_MODE_LTEM_NBIOT;
		break;
	case (BIT(AT_XSYSTEMMODE_READ_LTEM_INDEX) | BIT(AT_XSYSTEMMODE_READ_NBIOT_INDEX) |
	      BIT(AT_XSYSTEMMODE_READ_GPS_INDEX)):
		*mode = LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS;
		break;
	default:
		LOG_ERR("Invalid system mode, assuming parsing error");
		return -EFAULT;
	}

	/* Get LTE preference. */
	if (preference != NULL) {
		switch (mode_preference) {
		case 0:
			*preference = LTE_LC_SYSTEM_MODE_PREFER_AUTO;
			break;
		case 1:
			*preference = LTE_LC_SYSTEM_MODE_PREFER_LTEM;
			break;
		case 2:
			*preference = LTE_LC_SYSTEM_MODE_PREFER_NBIOT;
			break;
		case 3:
			*preference = LTE_LC_SYSTEM_MODE_PREFER_LTEM_PLMN_PRIO;
			break;
		case 4:
			*preference = LTE_LC_SYSTEM_MODE_PREFER_NBIOT_PLMN_PRIO;
			break;
		default:
			LOG_ERR("Unsupported LTE preference: %d", mode_preference);
			return -EFAULT;
		}
	}

	return 0;
}
