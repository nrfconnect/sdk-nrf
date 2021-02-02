/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <hal/nrf_acl.h>
#include <hal/nrf_ficr.h>
#include <errno.h>
#include <sys/__assert.h>

static uint32_t region_idx;

static int fprotect_set_permission(uint32_t start, size_t length,
				   size_t permission)
{
	__ASSERT_NO_MSG(nrf_ficr_codepagesize_get(NRF_FICR) ==
			CONFIG_FPROTECT_BLOCK_SIZE);

	if (region_idx >= ACL_REGIONS_COUNT) {
		return -ENOSPC;
	}

	if (start % nrf_ficr_codepagesize_get(NRF_FICR) != 0 ||
	    length % nrf_ficr_codepagesize_get(NRF_FICR) != 0 ||
	    length > NRF_ACL_REGION_SIZE_MAX || length == 0) {
		return -EINVAL;
	}

	nrf_acl_region_set(NRF_ACL, region_idx++, start, length, permission);

	return 0;
}

int fprotect_area(uint32_t start, size_t length)
{
	return fprotect_set_permission(start, length,
				       NRF_ACL_PERM_READ_NO_WRITE);
}

#if defined(CONFIG_FPROTECT_ENABLE_NO_ACCESS)

int fprotect_area_no_access(uint32_t start, size_t length)
{
	return fprotect_set_permission(start, length,
				       NRF_ACL_PERM_NO_READ_NO_WRITE);
}

#endif
