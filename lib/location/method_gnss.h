/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef METHOD_GNSS_H
#define METHOD_GNSS_H

int method_gnss_init(void);
void method_gnss_fix_work_fn(struct k_work *item);
int method_gnss_location_get(const struct location_method_config *config);
#if defined(CONFIG_LOCATION_DATA_DETAILS)
void method_gnss_details_get(struct location_data_details *details);
#endif

int method_gnss_cancel(void);

#endif /* METHOD_GNSS_H */
