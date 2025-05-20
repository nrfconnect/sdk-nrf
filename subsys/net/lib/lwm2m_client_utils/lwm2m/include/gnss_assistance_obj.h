/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LWM2M_CLIENT_UTILS_GNSS_ASSISTANCE_OBJ_H__
#define LWM2M_CLIENT_UTILS_GNSS_ASSISTANCE_OBJ_H__

/* Assistance types */
#define ASSISTANCE_REQUEST_TYPE_AGNSS			0
#define ASSISTANCE_REQUEST_TYPE_PGPS			1

typedef void (*gnss_assistance_get_result_code_cb_t)(int32_t result_code);
void gnss_assistance_set_result_code_cb(gnss_assistance_get_result_code_cb_t cb);

void gnss_assistance_prepare_download(void);
void gnss_assistance_download_cancel(void);
int location_assist_agnss_alloc_buf(size_t buf_size);
bool location_assist_gnss_is_busy(void);
int location_assist_gnss_type_set(int assistance_type);
int location_assist_gnss_type_get(void);

#endif
