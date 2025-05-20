/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FPROTECT_H_
#define FPROTECT_H_

#include <stdint.h>
#include <string.h>
#include <zephyr/types.h>

/**
 * @file
 * @defgroup fprotect Hardware flash write protection.
 * @{
 *
 * @brief API for write protection of flash areas using Hardware peripheral
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Protect flash area against writes.
 *
 * @param[in]  start   Start of range to protect.
 * @param[in]  length  Length in bytes of range to protect.
 *
 * @retval 0        On success.
 * @retval -EINVAL  If any of the argument are incorrect.
 * @retval -EFAULT  If the protection was not applied properly.
 * @retval -ENOSPC  If function is called too many times. Applies to
 *                  devices where there is a limited number of configuration
 *                  registers which are used for all address ranges.
 */
int fprotect_area(uint32_t start, size_t length);

/**
 * @brief Protect flash area against reads/writes.
 *
 * @param[in]  start   Start of range to protect.
 * @param[in]  length  Length in bytes of range to protect.
 *
 * @retval 0        On success.
 * @retval -EINVAL  If any of the argument are incorrect.
 * @retval -EFAULT  If the protection was not applied properly.
 * @retval -ENOSPC  If function is called too many times. Applies to
 *                  devices where there is a limited number of configuration
 *                  registers which are used for all address ranges.
 */
int fprotect_area_no_access(uint32_t start, size_t length);

/**
 * @brief Check whether a block has already been protected.
 *
 * NB: Only supported on HW platforms with ACL (CONFIG_HAS_HW_NRF_ACL).
 *
 * @param[in]  addr  The address to check. The block containing this address
 *                   will be checked.
 *
 * @retval 0   If not protected
 * @retval 1   If only write protected
 * @retval 2   If only read protected
 * @retval 3   If write and read protected
 */
uint32_t fprotect_is_protected(uint32_t addr);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* FPROTECT_H_ */
