/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <hal/nrf_acl.h>
#include <errno.h>

int fprotect_area(u32_t start, size_t length)
{
	static u32_t region_idx;

	if (region_idx >= ACL_REGIONS_COUNT) {
		return -ENOSPC;
	}

	if (start  % NRF_FICR->CODEPAGESIZE != 0 ||
	    length % NRF_FICR->CODEPAGESIZE != 0 ||
	    length > NRF_ACL_REGION_SIZE_MAX ||
	    length == 0) {
		return -EINVAL;
	}

	nrf_acl_region_set(NRF_ACL, region_idx++, start, length,
			   NRF_ACL_PERM_READ_NO_WRITE);

	return 0;
}
