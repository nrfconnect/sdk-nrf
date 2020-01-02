/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "fw_info.h"
#include <linker/sections.h>
#include <errno.h>
#include <string.h>


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


__fw_info struct fw_info m_firmware_info =
{
	.magic = {FIRMWARE_INFO_MAGIC},
	.firmware_size = (u32_t)&_flash_used,
	.firmware_version = CONFIG_FW_INFO_FIRMWARE_VERSION,
	.firmware_address = (u32_t)&_image_rom_start,
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

