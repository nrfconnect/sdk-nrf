/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef METHOD_CLOUD_LOCATION_H
#define METHOD_CLOUD_LOCATION_H

int method_cloud_location_get(const struct location_request_info *request);
int method_cloud_location_init(void);
int method_cloud_location_cancel(void);
#if defined(CONFIG_LOCATION_DATA_DETAILS)
void method_cloud_location_details_get(struct location_data_details *details);
#endif

#endif /* METHOD_CLOUD_LOCATION_H */
