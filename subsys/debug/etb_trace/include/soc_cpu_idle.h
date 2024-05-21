/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

void etb_trace_on_idle_exit(void);

#define SOC_ON_EXIT_CPU_IDLE \
	etb_trace_on_idle_exit();
