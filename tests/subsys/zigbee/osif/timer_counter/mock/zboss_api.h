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

/**
 *   Callback function typedef.
 *   Callback is function planned to execute by another function.
 *
 *   @param  param - callback parameter - usually, but not always, ref to packet buf
 *
 *   See any sample
 */
typedef void (*zb_callback_t)(zb_uint8_t param);

/**
 *   Callback function with 2 parameters typedef.
 *   Callback is function planned to execute by another function.
 *
 *   @param  param - callback parameter - usually, but not always, ref to packet buf
 *   @param  cb_param - additional 2-byte callback parameter, user data.
 *
 *   See any sample
 */
typedef void (*zb_callback2_t)(zb_uint8_t param, zb_uint16_t cb_param);

void zb_osif_busy_loop_delay(zb_uint32_t count);

/**
 *  Get current transceiver time value in usec
 */
zb_time_t osif_transceiver_time_get(void);

/**
 *  Get current transceiver time value in usec
 */
zb_uint64_t osif_transceiver_time_get_long(void);
