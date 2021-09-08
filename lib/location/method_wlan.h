/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef METHOD_WLAN_H
#define METHOD_WLAN_H

int method_wlan_location_get(const struct loc_method_config *config);
int method_wlan_init(void);
int method_wlan_cancel(void);

#endif /* METHOD_WLAN_H */
