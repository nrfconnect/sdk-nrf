/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef FPROTECT_H_
#define FPROTECT_H_

#include <stdint.h>
#include <string.h>

/**
 * @brief Protect flash area against writes.
 *
 * @param[in]  start   Start of range to protect.
 * @param[in]  length  Length in bytes of range to protect.
 *
 * @retval 0        On success.
 * @retval -EINVAL  If any of the argument are incorrect.
 * @retval -ENOSPC  If function is called too many times. Applies to
 *                  devices where there is a limited number of configuration
 *                  registers which are used for all address ranges.
 */
int fprotect_area(u32_t start, size_t length);

#endif /* FPROTECT_H_ */
