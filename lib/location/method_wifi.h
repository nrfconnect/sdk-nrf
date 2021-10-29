/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef METHOD_WIFI_H
#define METHOD_WIFI_H

int method_wifi_location_get(const struct location_method_config *config);
int method_wifi_init(void);
int method_wifi_cancel(void);

#endif /* METHOD_WIFI_H */
