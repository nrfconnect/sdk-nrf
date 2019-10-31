/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "fw_info.h"
#include <linker/sections.h>
#include <sys/util.h>
#include <init.h>
#include <errno.h>
#include <string.h>
#include <nrfx_nvmc.h>


/* These symbols are defined in linker scripts. */
extern const u32_t _image_rom_start[];
extern const u32_t _flash_used[];
extern const struct fw_info _firmware_info_start[];
extern const u32_t _ext_apis_size[];
extern const u32_t _ext_apis_req_size[];
extern const u32_t _fw_info_images_start[];
extern const u32_t _fw_info_images_size[];
extern const u32_t _fw_info_size[];


Z_GENERIC_SECTION(.firmware_info) __attribute__((used))
const struct fw_info m_firmware_info =
{
	.magic = {FIRMWARE_INFO_MAGIC},
	.total_size = (u32_t)_fw_info_size,
	.size = ((u32_t)_flash_used),
	.version = CONFIG_FW_INFO_FIRMWARE_VERSION,
	.address = ((u32_t)_image_rom_start),
	.boot_address = (u32_t)_image_rom_start,
	.valid = CONFIG_FW_INFO_VALID_VAL,
	.reserved = {0, 0, 0, 0},
	.ext_api_num = (u32_t)_ext_apis_size,
	.ext_api_request_num = (u32_t)_ext_apis_req_size,
};


Z_GENERIC_SECTION(.fw_info_images) __attribute__((used))
const u32_t self_image = ((u32_t)&_image_rom_start - FW_INFO_VECTOR_OFFSET);

#define NEXT_EXT_ABI(ext_api) ((const struct fw_info_ext_api *)\
			(((const u8_t *)(ext_api)) + (ext_api)->ext_api_len))

#define ADVANCE_EXT_API(ext_api) ((ext_api) = NEXT_EXT_ABI(ext_api))

#define ADVANCE_EXT_API_REQ(ext_api_req) ((ext_api_req) = \
			(const struct fw_info_ext_api_request *) \
			NEXT_EXT_ABI(&ext_api_req->request))

static bool ext_api_satisfies_req(const struct fw_info_ext_api * const ext_api,
		const struct fw_info_ext_api_request * const ext_api_req)
{
	const u32_t req_id = ext_api_req->request.ext_api_id;
	const u32_t req_flags = ext_api_req->request.ext_api_flags;
	const u32_t req_min_version = ext_api_req->request.ext_api_version;
	const u32_t req_max_version = ext_api_req->ext_api_max_version;

	return ((ext_api->ext_api_id == req_id)
		&&  (ext_api->ext_api_version >= req_min_version)
		&&  (ext_api->ext_api_version <  req_max_version)
		&& ((ext_api->ext_api_flags & req_flags) == req_flags));
}

static const struct fw_info_ext_api *find_ext_api(
		const struct fw_info_ext_api_request *ext_api_req)
{
	for (u32_t i = 0; i < (u32_t)_fw_info_images_size; i++) {
		const struct fw_info *fw_info =
				fw_info_find(_fw_info_images_start[i]);

		if (!fw_info || (fw_info->valid != CONFIG_FW_INFO_VALID_VAL)) {
			continue;
		}
		const struct fw_info_ext_api *ext_api = &fw_info->ext_apis[0];

		for (u32_t j = 0; j < fw_info->ext_api_num; j++) {
			if (ext_api_satisfies_req(ext_api, ext_api_req)) {
				/* Found valid EXT_API. */
				return ext_api;
			}
			ADVANCE_EXT_API(ext_api);
		}
	}
	return NULL;
}


static const struct fw_info_ext_api_request *skip_ext_apis(
					const struct fw_info * const fw_info)
{
	const struct fw_info_ext_api *ext_api = &fw_info->ext_apis[0];
	for (u32_t j = 0; j < fw_info->ext_api_num; j++) {
		ADVANCE_EXT_API(ext_api);
	}
	return (const struct fw_info_ext_api_request *)ext_api;
}


bool fw_info_ext_api_provide(const struct fw_info *fw_info, bool provide)
{
	const struct fw_info_ext_api_request *ext_api_req =
				skip_ext_apis(fw_info);

	for (u32_t i = 0; i < fw_info->ext_api_request_num; i++) {
		const struct fw_info_ext_api *new_ext_api =
				 find_ext_api(ext_api_req);

		if (provide) {
			/* Provide ext_api, or NULL. */
			*ext_api_req->ext_api = new_ext_api;
		}
		if (!new_ext_api && ext_api_req->required) {
			/* Requirement not met */
			return false;
		}
		ADVANCE_EXT_API_REQ(ext_api_req);
	}
	return true;
}


static int check_ext_api_requests(struct device *dev)
{
	(void)dev;

	const struct fw_info_ext_api_request *ext_api_req =
			skip_ext_apis(&m_firmware_info);

	for (u32_t i = 0; i < m_firmware_info.ext_api_request_num; i++) {
		if (fw_info_ext_api_check((u32_t)*(ext_api_req->ext_api))
			&& ext_api_satisfies_req(*(ext_api_req->ext_api),
						ext_api_req)) {
			/* EXT_API requirement met. */
		} else if (ext_api_req->required) {
			/* EXT_API hard requirement not met. */
			k_panic();
		} else {
			/* EXT_API soft requirement not met. */
			*ext_api_req->ext_api = NULL;
		}
		ADVANCE_EXT_API_REQ(ext_api_req);
	}

	return 0;
}

SYS_INIT(check_ext_api_requests, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);

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
