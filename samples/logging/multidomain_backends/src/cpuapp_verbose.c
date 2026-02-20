/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include "cpuapp_verbose.h"

LOG_MODULE_REGISTER(cpuapp_verbose, LOG_LEVEL_INF);

void cpuapp_verbose_log(uint32_t seq)
{
	if ((seq % 4U) == 0U) {
		LOG_INF("cpuapp verbose info seq=%u", seq);
	}
}
