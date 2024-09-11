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

#include "modules/redmob.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

int redmob_get(enum lte_lc_reduced_mobility_mode *mode)
{
	int ret;
	uint16_t mode_tmp;

	if (mode == NULL) {
		return -EINVAL;
	}

	ret = nrf_modem_at_scanf("AT%REDMOB?", "%%REDMOB: %hu", &mode_tmp);
	if (ret != 1) {
		LOG_ERR("AT command failed, nrf_modem_at_scanf() returned error: %d", ret);
		return -EFAULT;
	}

	*mode = mode_tmp;

	return 0;
}

int redmob_set(enum lte_lc_reduced_mobility_mode mode)
{
	int ret = nrf_modem_at_printf("AT%%REDMOB=%d", mode);

	if (ret) {
		/* Failure to send the AT command. */
		LOG_ERR("AT command failed, returned error code: %d", ret);
		return -EFAULT;
	}

	return 0;
}
