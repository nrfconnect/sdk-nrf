/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <modem/lte_lc.h>
#include <nrf_modem_at.h>

#include "modules/xfactoryreset.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

int xfactoryreset_reset(enum lte_lc_factory_reset_type type)
{
	return nrf_modem_at_printf("AT%%XFACTORYRESET=%d", type) ? -EFAULT : 0;
}
