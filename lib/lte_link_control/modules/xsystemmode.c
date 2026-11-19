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

#define SYS_MODE_MASK_TN (LTE_LC_SYSTEM_MODE_BIT_LTEM | LTE_LC_SYSTEM_MODE_BIT_NBIOT)
#define SYS_MODE_MASK_NTN                                                                          \
	(LTE_LC_SYSTEM_MODE_BIT_NTN_NBIOT_FDD | LTE_LC_SYSTEM_MODE_BIT_NTN_NBIOT_TDD)
#define SYS_MODE_MASK_ALL (SYS_MODE_MASK_TN | SYS_MODE_MASK_NTN | LTE_LC_SYSTEM_MODE_BIT_GNSS)

/* Internal system mode value used when CONFIG_LTE_NETWORK_MODE_DEFAULT is enabled. */
#define LTE_LC_SYSTEM_MODE_DEFAULT 0

#define SYS_MODE_PREFERRED                                                                         \
	(IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M)                                                 \
		 ? LTE_LC_SYSTEM_MODE_BIT_LTEM                                                     \
	 : IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT)                                               \
		 ? LTE_LC_SYSTEM_MODE_BIT_NBIOT                                                    \
	 : IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_GPS)                                           \
		 ? (LTE_LC_SYSTEM_MODE_BIT_LTEM | LTE_LC_SYSTEM_MODE_BIT_GNSS)                     \
	 : IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NBIOT_GPS)                                           \
		 ? (LTE_LC_SYSTEM_MODE_BIT_NBIOT | LTE_LC_SYSTEM_MODE_BIT_GNSS)                    \
	 : IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT)                                         \
		 ? (LTE_LC_SYSTEM_MODE_BIT_LTEM | LTE_LC_SYSTEM_MODE_BIT_NBIOT)                    \
	 : IS_ENABLED(CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS)                                     \
		 ? (LTE_LC_SYSTEM_MODE_BIT_LTEM | LTE_LC_SYSTEM_MODE_BIT_NBIOT |                   \
		    LTE_LC_SYSTEM_MODE_BIT_GNSS)                                                   \
	 : IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NTN_NBIOT)                                           \
		 ? LTE_LC_SYSTEM_MODE_BIT_NTN_NBIOT_FDD                                            \
	 : IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NTN_NBIOT_GNSS)                                      \
		 ? (LTE_LC_SYSTEM_MODE_BIT_NTN_NBIOT_FDD | LTE_LC_SYSTEM_MODE_BIT_GNSS)            \
	 : IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NTN_NBIOT_TDD)                                       \
		 ? LTE_LC_SYSTEM_MODE_BIT_NTN_NBIOT_TDD                                            \
	 : IS_ENABLED(CONFIG_LTE_NETWORK_MODE_NTN_NBIOT_TDD_GNSS)                                  \
		 ? (LTE_LC_SYSTEM_MODE_BIT_NTN_NBIOT_TDD | LTE_LC_SYSTEM_MODE_BIT_GNSS)            \
		 : LTE_LC_SYSTEM_MODE_DEFAULT)

/* The preferred system mode to use when connecting to LTE network. Can be changed by calling
 * lte_lc_system_mode_set() or lte_lc_system_mode_mask_set().
 *
 * extern in lte_lc_modem_hooks.c
 */
uint32_t lte_lc_sys_mode_mask = SYS_MODE_PREFERRED;
/* System mode preference to set when configuring system mode. Can be changed by calling
 * lte_lc_system_mode_set() or lte_lc_system_mode_mask_set().
 *
 * extern in lte_lc_modem_hooks.c
 */
enum lte_lc_system_mode_preference lte_lc_sys_mode_pref = CONFIG_LTE_MODE_PREFERENCE_VALUE;

static const uint32_t system_mode_masks[] = {
	[LTE_LC_SYSTEM_MODE_LTEM] = LTE_LC_SYSTEM_MODE_BIT_LTEM,
	[LTE_LC_SYSTEM_MODE_NBIOT] = LTE_LC_SYSTEM_MODE_BIT_NBIOT,
	[LTE_LC_SYSTEM_MODE_GPS] = LTE_LC_SYSTEM_MODE_BIT_GNSS,
	[LTE_LC_SYSTEM_MODE_LTEM_GPS] = LTE_LC_SYSTEM_MODE_BIT_LTEM | LTE_LC_SYSTEM_MODE_BIT_GNSS,
	[LTE_LC_SYSTEM_MODE_NBIOT_GPS] = LTE_LC_SYSTEM_MODE_BIT_NBIOT | LTE_LC_SYSTEM_MODE_BIT_GNSS,
	[LTE_LC_SYSTEM_MODE_LTEM_NBIOT] = LTE_LC_SYSTEM_MODE_BIT_LTEM |
					  LTE_LC_SYSTEM_MODE_BIT_NBIOT,
	[LTE_LC_SYSTEM_MODE_LTEM_NBIOT_GPS] = LTE_LC_SYSTEM_MODE_BIT_LTEM |
					      LTE_LC_SYSTEM_MODE_BIT_NBIOT |
					      LTE_LC_SYSTEM_MODE_BIT_GNSS,
	[LTE_LC_SYSTEM_MODE_NTN_NBIOT] = LTE_LC_SYSTEM_MODE_BIT_NTN_NBIOT_FDD,
};

/* LTE preference to be passed using AT%XSYSTEMMMODE=<params>,<preference> */
static const char system_mode_preference[] = {
	[LTE_LC_SYSTEM_MODE_PREFER_AUTO] = '0',
	[LTE_LC_SYSTEM_MODE_PREFER_LTEM] = '1',
	[LTE_LC_SYSTEM_MODE_PREFER_NBIOT] = '2',
	[LTE_LC_SYSTEM_MODE_PREFER_LTEM_PLMN_PRIO] = '3',
	[LTE_LC_SYSTEM_MODE_PREFER_NBIOT_PLMN_PRIO] = '4',
};

int xsystemmode_mask_set(uint32_t mask, enum lte_lc_system_mode_preference preference)
{
	int err;
	uint32_t ntn_mask = mask & SYS_MODE_MASK_NTN;
	int gnss = (mask & LTE_LC_SYSTEM_MODE_BIT_GNSS) ? 1 : 0;

	if (mask == 0 || (mask & ~SYS_MODE_MASK_ALL)) {
		LOG_ERR("Invalid system mode requested: 0x%x", mask);
		return -EINVAL;
	}

	if (ntn_mask == SYS_MODE_MASK_NTN) {
		LOG_ERR("NTN NB-IoT FDD and TDD can not be enabled at the same time");
		return -EINVAL;
	}

	if (ntn_mask && (mask & SYS_MODE_MASK_TN)) {
		LOG_ERR("NTN NB-IoT can not be enabled together with LTE-M or NB-IoT");
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

	if (ntn_mask && preference != LTE_LC_SYSTEM_MODE_PREFER_AUTO) {
		LOG_ERR("LTE preference must be LTE_LC_SYSTEM_MODE_PREFER_AUTO with NTN NB-IoT");
		return -EINVAL;
	}

	if (ntn_mask == LTE_LC_SYSTEM_MODE_BIT_NTN_NBIOT_TDD) {
		err = nrf_modem_at_printf("AT%%XSYSTEMMODE=0,0,%d,0,0,1", gnss);
	} else if (ntn_mask == LTE_LC_SYSTEM_MODE_BIT_NTN_NBIOT_FDD) {
		err = nrf_modem_at_printf("AT%%XSYSTEMMODE=0,0,%d,0,1", gnss);
	} else {
		err = nrf_modem_at_printf("AT%%XSYSTEMMODE=%d,%d,%d,%c",
					  (mask & LTE_LC_SYSTEM_MODE_BIT_LTEM) ? 1 : 0,
					  (mask & LTE_LC_SYSTEM_MODE_BIT_NBIOT) ? 1 : 0,
					  gnss,
					  system_mode_preference[preference]);
	}
	if (err) {
		LOG_ERR("Failed to set system mode");
		return -EFAULT;
	}

	lte_lc_sys_mode_mask = mask;
	lte_lc_sys_mode_pref = preference;

	LOG_DBG("System mode set to 0x%x, preference %d", lte_lc_sys_mode_mask,
		lte_lc_sys_mode_pref);

	return 0;
}

int xsystemmode_mask_get(uint32_t *mask, enum lte_lc_system_mode_preference *preference)
{
	int err;
	uint32_t mode_mask;
	int ltem_mode = 0;
	int nbiot_mode = 0;
	int gps_mode = 0;
	int mode_preference = 0;
	int ntn_nbiot_fdd_mode = 0;
	int ntn_nbiot_tdd_mode = 0;

	if (mask == NULL) {
		return -EINVAL;
	}

	/* It's expected to have 4 to 6 arguments matched */
	err = nrf_modem_at_scanf(AT_XSYSTEMMODE_READ, "%%XSYSTEMMODE: %d,%d,%d,%d,%d,%d",
				 &ltem_mode, &nbiot_mode, &gps_mode, &mode_preference,
				 &ntn_nbiot_fdd_mode, &ntn_nbiot_tdd_mode);
	if (err < 4) {
		LOG_ERR("Failed to get system mode, error: %d", err);
		return -EFAULT;
	}

	mode_mask = (ltem_mode ? LTE_LC_SYSTEM_MODE_BIT_LTEM : 0) |
		    (nbiot_mode ? LTE_LC_SYSTEM_MODE_BIT_NBIOT : 0) |
		    (gps_mode ? LTE_LC_SYSTEM_MODE_BIT_GNSS : 0) |
		    (ntn_nbiot_fdd_mode ? LTE_LC_SYSTEM_MODE_BIT_NTN_NBIOT_FDD : 0) |
		    (ntn_nbiot_tdd_mode ? LTE_LC_SYSTEM_MODE_BIT_NTN_NBIOT_TDD : 0);

	if (mode_mask == 0) {
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

	*mask = mode_mask;

	return 0;
}

int xsystemmode_mode_set(enum lte_lc_system_mode mode,
			 enum lte_lc_system_mode_preference preference)
{
	if ((unsigned int)mode >= ARRAY_SIZE(system_mode_masks) ||
	    system_mode_masks[mode] == 0) {
		LOG_ERR("Invalid system mode requested: %d", mode);
		return -EINVAL;
	}

	return xsystemmode_mask_set(system_mode_masks[mode], preference);
}

int xsystemmode_mode_get(enum lte_lc_system_mode *mode,
			 enum lte_lc_system_mode_preference *preference)
{
	int err;
	uint32_t mask;

	if (mode == NULL) {
		return -EINVAL;
	}

	err = xsystemmode_mask_get(&mask, preference);
	if (err) {
		return err;
	}

	for (size_t i = 0; i < ARRAY_SIZE(system_mode_masks); i++) {
		if (system_mode_masks[i] == mask) {
			*mode = i;
			return 0;
		}
	}

	LOG_ERR("System mode 0x%x can not be represented as enum lte_lc_system_mode", mask);

	return -EFAULT;
}
