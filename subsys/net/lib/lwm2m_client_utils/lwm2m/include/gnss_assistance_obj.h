/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LWM2M_CLIENT_UTILS_GNSS_ASSISTANCE_OBJ_H__
#define LWM2M_CLIENT_UTILS_GNSS_ASSISTANCE_OBJ_H__
void gnss_assistance_prepare_download(void);
int location_assist_agps_alloc_buf(size_t buf_size);
#endif
