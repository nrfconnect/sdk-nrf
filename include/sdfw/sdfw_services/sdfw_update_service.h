/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SSF_SDFW_UPDATE_SERVICE_H__
#define SSF_SDFW_UPDATE_SERVICE_H__

#include <stdint.h>

#include <sdfw/sdfw_services/ssf_errno.h>

/* Error code in response from server if an error occurred when performing the request. */
#define SSF_SDFW_UPDATE_FAILED 0x3dfc0001

/**
 * @brief       Trigger an SDFW update request by pointing to an SDFW update blob.
 *
 * @param[in]   blob_addr       Start address of the SDFW update blob.
 *
 * @return      Return value from `ssf_client_send_request`.
 */
int ssf_sdfw_update(uintptr_t blob_addr);

#endif /* SSF_SDFW_UPDATE_SERVICE_H__ */
