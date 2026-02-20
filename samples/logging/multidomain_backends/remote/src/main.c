/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "cpuflpr_verbose.h"

LOG_MODULE_REGISTER(cpuflpr_main, LOG_LEVEL_INF);

int main(void)
{
	uint32_t seq = 0U;

	LOG_INF("cpuflpr domain started (%s)", CONFIG_BOARD);
	LOG_INF("cpuflpr demo cadence: app logs every 10 seconds");

	while (1) {
		LOG_INF("cpuflpr info heartbeat seq=%u (10s cadence)", seq);
		if ((seq % 3U) == 0U) {
			LOG_WRN("cpuflpr warning checkpoint seq=%u", seq);
		}
		if ((seq % 6U) == 0U) {
			LOG_ERR("cpuflpr error checkpoint seq=%u", seq);
		}

		cpuflpr_verbose_log(seq);

		seq++;
		k_sleep(K_SECONDS(10));
	}

	return 0;
}
