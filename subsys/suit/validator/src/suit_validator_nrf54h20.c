/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_validator.h>
#include <sdfw/arbiter.h>
#include <suit_memory_layout.h>

static suit_plat_err_t validate_candidate_location_common(const uint8_t *address,
							  size_t size)
{
	struct arbiter_mem_params_access mem_params = {
		.allowed_types = ARBITER_MEM_TYPE(RESERVED, FIXED, FREE),
		.access = {
				.owner = NRF_OWNER_APPLICATION,
				.permissions = ARBITER_MEM_PERM(READ, SECURE),
				.address = (uintptr_t)address,
				.size = size,
			},
	};

	if (arbiter_mem_access_check(&mem_params) == ARBITER_STATUS_OK) {
		return SUIT_PLAT_SUCCESS;
	}

	mem_params.access.owner = NRF_OWNER_RADIOCORE;

	if (arbiter_mem_access_check(&mem_params) == ARBITER_STATUS_OK) {
		return SUIT_PLAT_SUCCESS;
	}

	return SUIT_PLAT_ERR_ACCESS;
}

suit_plat_err_t suit_validator_validate_update_candidate_location(const uint8_t *address,
								  size_t size)
{
	return validate_candidate_location_common(address, size);
}

suit_plat_err_t suit_validator_validate_cache_pool_location(const uint8_t *address,
							       size_t size)
{
	if (suit_memory_global_address_range_is_in_external_memory((uintptr_t) address, size)) {
		return SUIT_PLAT_SUCCESS;
	}

	return validate_candidate_location_common(address, size);
}
