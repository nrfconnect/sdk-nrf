/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include "cpuflpr_verbose.h"

LOG_MODULE_REGISTER(cpuflpr_verbose, LOG_LEVEL_INF);

void cpuflpr_verbose_log(uint32_t seq)
{
	if ((seq % 4U) == 0U) {
		LOG_INF("cpuflpr verbose info seq=%u", seq);
	}
}
