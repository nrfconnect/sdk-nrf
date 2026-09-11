/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRFX_CLOCK_HFCLKAUDIO_H__
#define NRFX_CLOCK_HFCLKAUDIO_H__

#include <stdint.h>

/* Test-only declarations intentionally shadow nrfx hardware headers. */

typedef void (*nrfx_clock_hfclkaudio_event_handler_t)(void);

int nrfx_clock_hfclkaudio_init(nrfx_clock_hfclkaudio_event_handler_t event_handler);
void nrfx_clock_hfclkaudio_start(void);
void nrfx_clock_hfclkaudio_config_set(uint16_t freq_value);

#endif
