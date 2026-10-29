/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <modem/lte_lc.h>
#include <nrf_modem_at.h>
#include <nrf_errno.h>

#include "modules/xdataprfl.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

int xdataprfl_set(enum lte_lc_data_profile profile)
{
	int err;

	if ((profile < LTE_LC_DATA_PROFILE_ULTRA_LOW_POWER) ||
	    (profile > LTE_LC_DATA_PROFILE_HIGH_PERFORMANCE)) {
		LOG_ERR("Invalid data profile: %d", profile);

		return -EINVAL;
	}

	/* Command syntax: AT%XDATAPRFL=<level> */

	err = nrf_modem_at_printf("AT%%XDATAPRFL=%d", profile);
	if (err < 0) {
		LOG_ERR("AT command failed, err: %d", err);

		return -EFAULT;
	} else if (err > 0) {
		/* The modem responded with "ERROR". */
		LOG_ERR("The modem rejected the data profile, error: %d", err);

		return -EBADMSG;
	}

	return 0;
}

int xdataprfl_get(enum lte_lc_data_profile *profile)
{
	int err;
	uint16_t level;

	if (profile == NULL) {
		return -EINVAL;
	}

	/* Response format: %XDATAPRFL: <level> */

	err = nrf_modem_at_scanf("AT%XDATAPRFL?", "%%XDATAPRFL: %hu", &level);
	if (err == -NRF_EBADMSG) {
		/* Nothing was matched, or the modem responded with an error. */
		return -EBADMSG;
	} else if (err < 0) {
		return -EFAULT;
	} else if (err != 1) {
		return -EBADMSG;
	}

	if (level > LTE_LC_DATA_PROFILE_HIGH_PERFORMANCE) {
		LOG_ERR("Unrecognized data profile: %hu", level);

		return -EBADMSG;
	}

	*profile = level;

	return 0;
}

int xdataprfl_init_apply(enum lte_lc_data_profile profile)
{
	int err;
	enum lte_lc_data_profile current_profile;

	err = xdataprfl_get(&current_profile);
	if (err) {
		return err;
	}

	if (current_profile == profile) {
		LOG_DBG("Data profile already set to %d", profile);

		return 0;
	}

	err = xdataprfl_set(profile);
	if (err) {
		return err;
	}

	LOG_DBG("Data profile set to %d", profile);

	return 0;
}
