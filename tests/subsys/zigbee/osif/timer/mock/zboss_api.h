/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zb_types.h>

/* stubs */
#define ZB_BEACON_INTERVAL_USEC 15360
#define ZB_TIMER_GET()
#define ZB_TIME_BEACON_INTERVAL_TO_USEC(t) 0

void zb_osif_busy_loop_delay(zb_uint32_t count);
