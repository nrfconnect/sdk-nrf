/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SCAN_WIFI_H
#define SCAN_WIFI_H

#include <net/wifi_location_common.h>

int scan_wifi_init(void);
int scan_wifi_start(struct k_sem *wifi_scan_ready);
struct wifi_scan_info *scan_wifi_results_get(void);
int scan_wifi_cancel(void);

#endif /* SCAN_WIFI_H */
