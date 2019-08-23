/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef BL_VALIDATION_H__
#define BL_VALIDATION_H__

#include <stdbool.h>
#include <fw_metadata.h>

bool bl_validate_firmware_local(u32_t fw_address,
				const struct fw_firmware_info *fw_info);

#endif /* BL_VALIDATION_H__ */
