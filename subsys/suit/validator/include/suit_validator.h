/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Validator functions for SoC-dependent properties
 *
 * @details This module implements functions for validating properties, such as addresses.
 *          The result of validating a property can be different depending on the SoC.
 *
 */

#ifndef SUIT_VALIDATOR_H__
#define SUIT_VALIDATOR_H__

#include <stdint.h>
#include <stddef.h>
#include <suit_plat_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate the location of the update candidate.
 *
 * @retval SUIT_PLAT_SUCCESS     The location is valid.
 * @retval SUIT_PLAT_ERR_ACCESS  The location is forbidden.
 */
suit_plat_err_t suit_validator_validate_update_candidate_location(const uint8_t *address,
								  size_t size);

/**
 * @brief Validate the location of a DFU cache partition.
 *
 * @retval SUIT_PLAT_SUCCESS     The location is valid.
 * @retval SUIT_PLAT_ERR_ACCESS  The location is forbidden.
 */
suit_plat_err_t suit_validator_validate_cache_pool_location(const uint8_t *address,
							       size_t size);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_VALIDATOR_H__ */
