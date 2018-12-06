/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <nrf_acl.h>

void fprotect_area(u32_t start, size_t length)
{
	nrf_acl_region_set(NRF_ACL, 0, start, length,
			   NRF_ACL_PERM_READ_NO_WRITE);
}
