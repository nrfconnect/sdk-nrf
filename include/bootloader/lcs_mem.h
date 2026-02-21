/*
 * Copyright (c) 2026 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef LCS_MEM_H__
#define LCS_MEM_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The Life Cycle State memory backend API.
 *
 * This file defines an API to read and write into memory slots that correspond
 * to LCS values in memory.
 * The memory can be a real or emulated OTP - the memory must be writable
 * only once, naturally blocking backward LCS transitions.
 *
 * The API is designed in such a way, that the memory backed do not need to
 * include conditional statements to perform requested actions.
 * This way all of the glitch-resistant logic can be implemented in the common
 * code and the memory backend can be implemented in a simple and straightforward way.
 * The read API uses a pointer to an output variable to distinguish betweeen a
 * successful read and a failed read to detect potential tampering attempts.
 */

/**
 * The value read from memory corresponding to an LCS state when it is not entered.
 */
#define LCS_MEM_VALUE_NOT_ENTERED 0xFFFF

/**
 * @brief Query the PSA RoT provisioning Life Cycle State value.
 *
 * @param[out] value  The value read from memory corresponding to the PSA RoT provisioning state.
 *
 * @return 0 if successful, negative error code otherwise.
 */
int lcs_mem_provisioning_read(uint16_t *value);

/**
 * @brief Enter the PSA RoT provisioning Life Cycle State value.
 *
 * @param[in] value  The value to write to the memory corresponding to the PSA RoT provisioning state.
 *
 * @return 0 if successful, negative error code otherwise.
 */
int lcs_mem_provisioning_write(uint16_t value);

/**
 * @brief Query the secured Life Cycle State value.
 *
 * @param[out] value  The value read from memory corresponding to the secured state.
 *
 * @return 0 if successful, negative error code otherwise.
 */
int lcs_mem_secured_read(uint16_t *value);

/**
 * @brief Enter the secured Life Cycle State value.
 *
 * @param[in] value  The value to write to the memory corresponding to the secured state.
 *
 * @return 0 if successful, negative error code otherwise.
 */
int lcs_mem_secured_write(uint16_t value);

/**
 * @brief Query the decommissioned Life Cycle State value.
 *
 * @param[out] value  The value read from memory corresponding to the decommissioned state.
 *
 * @return 0 if successful, negative error code otherwise.
 */
int lcs_mem_decommissioned_read(uint16_t *value);

/**
 * @brief Enter the decommissioned Life Cycle State value.
 *
 * @param[in] value  The value to write to the memory corresponding to the decommissioned state.
 *
 * @return 0 if successful, negative error code otherwise.
 */
int lcs_mem_decommissioned_write(uint16_t value);

#ifdef __cplusplus
}
#endif

#endif /* LCS_MEM_H__ */
