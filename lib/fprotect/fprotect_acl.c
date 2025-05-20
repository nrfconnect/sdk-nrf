/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <hal/nrf_acl.h>
#include <hal/nrf_ficr.h>
#include <nrfx_nvmc.h>
#include <errno.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/kernel.h>

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

uint32_t fprotect_is_protected(uint32_t addr)
{
	uint32_t region_idx;
	uint32_t page_size = nrf_ficr_codepagesize_get(NRF_FICR);
	uint32_t flash_size = nrf_ficr_codesize_get(NRF_FICR) * page_size;
	uint32_t block_addr = ROUND_DOWN(addr, CONFIG_FPROTECT_BLOCK_SIZE);

	find_free_region(&region_idx);

	for (int i = 0; i < region_idx; i++) {
		uint32_t prot_addr = nrf_acl_region_address_get(NRF_ACL, i);
		uint32_t prot_size = nrf_acl_region_size_get(NRF_ACL, i);
		nrf_acl_perm_t prot_perm = nrf_acl_region_perm_get(NRF_ACL, i);

		if ((prot_addr > flash_size) || (prot_size > NRF_ACL_REGION_SIZE_MAX)) {
			/* ACL is incorrectly configured! */
			k_panic();
		}

		uint32_t prot_end_addr = prot_addr + prot_size;
		uint32_t end_addr = block_addr + CONFIG_FPROTECT_BLOCK_SIZE;

		if ((prot_addr <= block_addr) && (prot_end_addr >= end_addr)) {
			return prot_perm == NRF_ACL_PERM_READ_NO_WRITE ? 1
				: prot_perm == NRF_ACL_PERM_NO_READ_WRITE ? 2
				: prot_perm == NRF_ACL_PERM_NO_READ_NO_WRITE ? 3
				: 0;
		}
	}

	return 0;
}

int fprotect_area(uint32_t start, size_t length)
{
	return fprotect_set_permission(start, length,
				       NRF_ACL_PERM_READ_NO_WRITE);
}

int fprotect_area_no_access(uint32_t start, size_t length)
{
	return fprotect_set_permission(start, length,
				       NRF_ACL_PERM_NO_READ_NO_WRITE);
}
