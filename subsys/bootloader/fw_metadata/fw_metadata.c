/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "fw_metadata.h"

extern const u32_t _image_rom_start;
extern const u32_t _flash_used;

const struct fw_firmware_info m_firmware_info
_GENERIC_SECTION(.firmware_info)
__attribute__((used)) = {
	.magic = {FIRMWARE_INFO_MAGIC},
	.firmware_size = (u32_t)&_flash_used,
	.firmware_version = CONFIG_SB_FIRMWARE_VERSION,
	.firmware_address = (u32_t)&_image_rom_start
};
