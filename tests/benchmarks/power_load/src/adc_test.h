/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>
#include <dmm.h>

#define ADC_THREAD_STACKSIZE (512)
#define ADC_THREAD_PRIORITY  (1)
#define ADC_THREAD_SLEEP     (50)

#define ADC_BUFFER_MAX_SIZE 16

#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),
