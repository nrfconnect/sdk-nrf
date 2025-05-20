/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef METHOD_GNSS_H
#define METHOD_GNSS_H

int method_gnss_init(void);
int method_gnss_location_get(const struct location_request_info *request);
int method_gnss_cancel(void);
int method_gnss_timeout(void);
#if defined(CONFIG_LOCATION_DATA_DETAILS)
void method_gnss_details_get(struct location_data_details *details);
#endif

#endif /* METHOD_GNSS_H */
