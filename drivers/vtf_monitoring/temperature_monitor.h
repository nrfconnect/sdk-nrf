/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _TEMPERATURE_MONITOR_H_
#define _TEMPERATURE_MONITOR_H_

#include <drivers/vtf_monitoring/vtf_monitoring.h>

/**
 * Bring up the sensor and schedule the first periodic read.
 */
int die_temp_init(void);

/**
 * Return the latest die-temperature sample.
 */
int die_temp_sample(struct vtf_sample *out);

#endif /* _TEMPERATURE_MONITOR_H_ */
