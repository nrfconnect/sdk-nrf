/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>

#include "otperf_internal.h"
#include "otperf_session.h"

#include <openthread/ip6.h>

uint32_t otperf_packet_duration(uint32_t packet_size, uint32_t rate_in_kbps)
{
	return (uint32_t)(((uint64_t)packet_size * 8U * USEC_PER_SEC) / (rate_in_kbps * 1024U));
}

static int otperf_init(void)
{
	if (IS_ENABLED(CONFIG_OTPERF_SERVER)) {
		otperf_session_reset();
	}

	return 0;
}

SYS_INIT(otperf_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
