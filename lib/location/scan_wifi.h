/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SCAN_WIFI_H
#define SCAN_WIFI_H

#include <modem/location.h>
#include <net/wifi_location_common.h>

int scan_wifi_init(void);
int scan_wifi_validate_params(const struct location_method_config *config);
void scan_wifi_execute(const struct location_wifi_config *config, struct k_sem *wifi_scan_ready);
struct wifi_scan_info *scan_wifi_results_get(void);
int scan_wifi_cancel(void);
#if defined(CONFIG_LOCATION_DATA_DETAILS)
void scan_wifi_details_get(struct location_data_details *details);
#endif

#endif /* SCAN_WIFI_H */
