/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef BT_MESH_TIME_UTIL
#define BT_MESH_TIME_UTIL

#include <zephyr/types.h>


#ifdef __cplusplus
extern "C" {
#endif

void tai_to_ts(uint64_t uptime, struct tm *timeptr);
int ts_to_tai(int64_t *uptime, struct tm *timeptr);

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_TIME_UTIL */

/** @} */
