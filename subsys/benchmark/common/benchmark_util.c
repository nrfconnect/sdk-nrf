/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include "benchmark_api.h"

void benchmark_clear_latency(struct benchmark_latency *p_latency)
{
	p_latency->sum = 0;
	p_latency->cnt = 0;
	p_latency->max = 0;
	p_latency->min = UINT32_MAX;
}

void benchmark_update_latency(struct benchmark_latency *p_latency, uint32_t latency)
{
	p_latency->cnt++;

	p_latency->sum += latency;

	if (latency < p_latency->min) {
		p_latency->min = latency;
	}

	if (latency > p_latency->max) {
		p_latency->max = latency;
	}
}
