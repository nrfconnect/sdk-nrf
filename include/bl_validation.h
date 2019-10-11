/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef BL_VALIDATION_H__
#define BL_VALIDATION_H__

#include <stdbool.h>
#include <fw_metadata.h>

bool verify_firmware(u32_t address);

#endif /* BL_VALIDATION_H__ */
