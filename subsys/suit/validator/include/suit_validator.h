/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_VALIDATOR_H__
#define SUIT_VALIDATOR_H__

#include <stdint.h>
#include <stddef.h>
#include <suit_plat_err.h>

suit_plat_err_t suit_validator_validate_update_candidate_location(const uint8_t *address,
								  size_t size);

suit_plat_err_t suit_validator_validate_dfu_partition_location(const uint8_t *address,
							       size_t size);

#endif /* SUIT_VALIDATOR_H__ */
