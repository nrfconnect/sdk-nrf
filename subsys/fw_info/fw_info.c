/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "fw_info.h"
#include <linker/sections.h>
#include <errno.h>
#include <string.h>
#include <nrfx_nvmc.h>


extern const u32_t _image_rom_start;
extern const u32_t _flash_used;
extern const struct fw_info _firmware_info_start[];
extern const struct fw_info_ext_api * const _ext_apis_start[];
extern const u32_t _ext_apis_size;
__noinit fw_info_ext_api_getter ext_api_getter_in;

int ext_api_getter(u32_t id, u32_t index, const struct fw_info_ext_api **out)
{
	if (!out) {
		return -EFAULT;
	}

	bool id_found = false;

	for (u32_t i = 0; i < (u32_t)&_ext_apis_size; i++) {
		const struct fw_info_ext_api *ext_api = _ext_apis_start[i];

		if (ext_api->ext_api_id == id) {
			id_found = true;
			if (index-- == 0) {
				*out = ext_api;
				return 0;
			}
		}
	}
	return id_found ? -EBADF : -ENOENT;
}

Z_GENERIC_SECTION(.firmware_info) __attribute__((used))
const struct fw_info m_firmware_info =
{
	.magic = {FIRMWARE_INFO_MAGIC},
	.size = (u32_t)&_flash_used,
	.version = CONFIG_FW_INFO_FIRMWARE_VERSION,
	.address = (u32_t)&_image_rom_start,
	.valid = CONFIG_FW_INFO_VALID_VAL,
	.ext_api_in = &ext_api_getter_in,
	.ext_api_out = &ext_api_getter,
};


void fw_info_ext_api_provide(const struct fw_info *fwinfo)
{
	if (fwinfo->ext_api_in != NULL) {
		*(fwinfo->ext_api_in) = &ext_api_getter;
	}
}


const struct fw_info_ext_api *fw_info_ext_api_find(u32_t id, u32_t flags,
					u32_t min_version, u32_t max_version)
{
	for (u32_t i = 0; i < 1000; i++)
	{
		const struct fw_info_ext_api *ext_api;
		int ret = ext_api_getter_in(id, 0, &ext_api);
		if (ret) {
			return NULL;
	 	}
		if (fw_info_ext_api_check(ext_api)
		&&  (ext_api->ext_api_version >= min_version)
		&&  (ext_api->ext_api_version <  max_version)
		&& ((ext_api->ext_api_flags & flags) == flags))
		{
			/* Found valid ext_api. */
			return ext_api;
		}
	}
	return NULL;
}

/** Value to write to the "valid" member of fw_info to invalidate the image. */
#define INVALID_VAL 0xFFFF0000

BUILD_ASSERT_MSG((INVALID_VAL & CONFIG_FW_INFO_VALID_VAL) \
			!= CONFIG_FW_INFO_VALID_VAL, \
		"CONFIG_FW_INFO_VALID_VAL has been configured such that the "
		"image cannot be invalidated. Change the value so that writing "
		"INVALID_VAL has an effect.");

#ifdef CONFIG_NRFX_NVMC
void fw_info_invalidate(const struct fw_info *fw_info)
{
	/* Check if value has been written. */
	if (fw_info->valid == CONFIG_FW_INFO_VALID_VAL) {
		nrfx_nvmc_word_write((u32_t)&(fw_info->valid), INVALID_VAL);
	}
}
#endif
