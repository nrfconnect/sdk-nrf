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

void tai_to_ts(const struct bt_mesh_time_tai *tai, struct tm *timeptr);
int ts_to_tai(struct bt_mesh_time_tai *tai, struct tm *timeptr);

#ifdef __cplusplus
}
#endif

#endif /* BT_MESH_TIME_UTIL */

/** @} */
