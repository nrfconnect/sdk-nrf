/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <hal/nrf_acl.h>
#include <hal/nrf_ficr.h>
#include <errno.h>
#include <sys/__assert.h>
#include <kernel.h>

/* Find the first unused ACL region. */
static int find_free_region(uint32_t *region_idx)
{
	static uint32_t idx;

	while (nrf_acl_region_perm_get(NRF_ACL, idx) != 0) {
		idx++;
		if (idx >= ACL_REGIONS_COUNT) {
			*region_idx = idx;
			return -ENOSPC;
		}
	}
	*region_idx = idx;
	return 0;
}

static int fprotect_set_permission(uint32_t start, size_t length,
				   size_t permission)
{
	__ASSERT_NO_MSG(nrf_ficr_codepagesize_get(NRF_FICR) ==
			CONFIG_FPROTECT_BLOCK_SIZE);

	uint32_t region_idx;
	int result = find_free_region(&region_idx);

	if (result != 0) {
		return result;
	}

	if (start % nrf_ficr_codepagesize_get(NRF_FICR) != 0 ||
	    length % nrf_ficr_codepagesize_get(NRF_FICR) != 0 ||
	    length > NRF_ACL_REGION_SIZE_MAX || length == 0) {
		return -EINVAL;
	}

	nrf_acl_region_set(NRF_ACL, region_idx, start, length, permission);

	if ((nrf_acl_region_address_get(NRF_ACL, region_idx) != start)
		|| (nrf_acl_region_size_get(NRF_ACL, region_idx) != length)
		|| (nrf_acl_region_perm_get(NRF_ACL, region_idx) != permission)) {
		return -EFAULT;
	}

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
