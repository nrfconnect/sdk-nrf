/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_validator.h>

static suit_plat_err_t validate_candidate_location_common(uintptr_t address,
							  size_t size)
{
	/* For the purpose of the "mock" in tests, set the forbidden
	 * address range to 0x0001000 - 0x0001FFF
	 */
	if (((uint32_t) address >= 0x0001000 &&  (uint32_t) address <= 0x0001FFF)
	 || ((uint32_t) address + size >= 0x0001000 &&  (uint32_t) address + size <= 0x0001FFF)) {
		return SUIT_PLAT_ERR_ACCESS;
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_validator_validate_update_candidate_location(const uint8_t *address,
								  size_t size)
{
	return validate_candidate_location_common((uintptr_t) address, size);
}

suit_plat_err_t suit_validator_validate_cache_pool_location(const uint8_t *address,
							       size_t size)
{
	return validate_candidate_location_common((uintptr_t) address, size);
}
