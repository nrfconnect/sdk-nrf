/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CPU_LOAD_MONITOR_H_
#define CPU_LOAD_MONITOR_H_

#include <stdint.h>

void cpu_load_monitor_init(void);

void cpu_load_monitor_start(void);

void cpu_load_monitor_stop(void);

void cpu_load_monitor_show(void);

void cpu_load_monitor_terminate(void);

#endif /* CPU_LOAD_MONITOR_H_ */
