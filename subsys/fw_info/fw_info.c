/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "fw_info.h"
#include <zephyr/linker/sections.h>
#include <zephyr/sys/util.h>
#include <zephyr/init.h>
#include <errno.h>
#include <string.h>
#include <nrfx_nvmc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>


/* These symbols are defined in linker scripts. */
extern const uint32_t __rom_region_start[];
extern const uint32_t _flash_used[];
extern const struct fw_info _firmware_info_start[];
extern const uint32_t _ext_apis_size[];
extern const uint32_t _ext_apis_req_size[];
extern const uint32_t _fw_info_images_start[];
extern const uint32_t _fw_info_images_size[];
extern const uint32_t _fw_info_size[];


Z_GENERIC_SECTION(.firmware_info) __attribute__((used))
const struct fw_info m_firmware_info =
{
	.magic = {FIRMWARE_INFO_MAGIC},
	.total_size = (uint32_t)_fw_info_size,
	.size = ((uint32_t)_flash_used),
	.version = CONFIG_FW_INFO_FIRMWARE_VERSION,
	.address = ((uint32_t)__rom_region_start),
	.boot_address = (uint32_t)__rom_region_start,
	.valid = CONFIG_FW_INFO_VALID_VAL,
	.reserved = {0, 0, 0, 0},
	.ext_api_num = (uint32_t)_ext_apis_size,
	.ext_api_request_num = (uint32_t)_ext_apis_req_size,
};


Z_GENERIC_SECTION(.fw_info_images) __attribute__((used))
const uint32_t self_image = ((uint32_t)&__rom_region_start - FW_INFO_VECTOR_OFFSET);

#define NEXT_EXT_ABI(ext_api) ((const struct fw_info_ext_api *)\
			(((const uint8_t *)(ext_api)) + (ext_api)->ext_api_len))

#define ADVANCE_EXT_API(ext_api) ((ext_api) = NEXT_EXT_ABI(ext_api))

#define ADVANCE_EXT_API_REQ(ext_api_req) ((ext_api_req) = \
			(const struct fw_info_ext_api_request *) \
			NEXT_EXT_ABI(&ext_api_req->request))

static bool ext_api_satisfies_req(const struct fw_info_ext_api * const ext_api,
		const struct fw_info_ext_api_request * const ext_api_req)
{
	const uint32_t req_id = ext_api_req->request.ext_api_id;
	const uint32_t req_flags = ext_api_req->request.ext_api_flags;
	const uint32_t req_min_version = ext_api_req->request.ext_api_version;
	const uint32_t req_max_version = ext_api_req->ext_api_max_version;

	return ((ext_api->ext_api_id == req_id)
		&&  (ext_api->ext_api_version >= req_min_version)
		&&  (ext_api->ext_api_version <  req_max_version)
		&& ((ext_api->ext_api_flags & req_flags) == req_flags));
}


const struct fw_info_ext_api_request *skip_ext_apis(
					const struct fw_info * const fw_info)
{
	const struct fw_info_ext_api *ext_api = &fw_info->ext_apis[0];

	for (uint32_t j = 0; j < fw_info->ext_api_num; j++) {
		ADVANCE_EXT_API(ext_api);
	}
	return (const struct fw_info_ext_api_request *)ext_api;
}


#ifdef CONFIG_EXT_API_PROVIDE_EXT_API_UNUSED
static const struct fw_info_ext_api *find_ext_api(
		const struct fw_info_ext_api_request *ext_api_req,
		const struct fw_info * const skip_fw_info)
{
	for (uint32_t i = 0; i < (uint32_t)_fw_info_images_size; i++) {
		const struct fw_info *fw_info =
				fw_info_find(_fw_info_images_start[i]);

		if (!fw_info || (fw_info->valid != CONFIG_FW_INFO_VALID_VAL)
		    || (fw_info == skip_fw_info)) {
			continue;
		}
		const struct fw_info_ext_api *ext_api = &fw_info->ext_apis[0];

		for (uint32_t j = 0; j < fw_info->ext_api_num; j++) {
			if (ext_api_satisfies_req(ext_api, ext_api_req)) {
				/* Found valid EXT_API. */
				return ext_api;
			}
			ADVANCE_EXT_API(ext_api);
		}
	}
	return NULL;
}


/* This is the "proper" implementation of fw_info_ext_api_provide(). If
 * CONFIG_EXT_API_PROVIDE_EXT_API_REQUIRED is defined, we want to call the
 * EXT_API instead, which happens below, in the #else clause.
 */
bool fw_info_ext_api_provide(const struct fw_info *fw_info, bool provide)
{
	if (fw_info == NULL) {
		return false;
	}

	const struct fw_info_ext_api_request *ext_api_req =
				skip_ext_apis(fw_info);

	for (uint32_t i = 0; i < fw_info->ext_api_request_num; i++) {
		const struct fw_info_ext_api *new_ext_api =
				 find_ext_api(ext_api_req, fw_info);

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

#else
#ifdef CONFIG_EXT_API_PROVIDE_EXT_API_REQUIRED
#define EXT_API_PROVIDE_EXT_API_REQUIRED 1
#else
#define EXT_API_PROVIDE_EXT_API_REQUIRED 0
#endif
EXT_API_REQ(EXT_API_PROVIDE, EXT_API_PROVIDE_EXT_API_REQUIRED,
		struct ext_api_provide_ext_api, ext_api_provide);

bool fw_info_ext_api_provide(const struct fw_info *fw_info, bool provide)
{
	if (ext_api_provide == NULL) {
		/* Can only happen if the EXT_API is optional. */
		return false;
	}

	/* Calls into the EXT_API */
	return ext_api_provide->ext_api.ext_api_provide(fw_info, provide);
}
#endif


#ifdef CONFIG_EXT_API_PROVIDE_EXT_API_ENABLED
EXT_API(EXT_API_PROVIDE, struct ext_api_provide_ext_api,
				ext_api_provide_ext_api) = {
		.ext_api_provide = fw_info_ext_api_provide,
	}
};
#endif

static int check_ext_api_requests(void)
{
	const struct fw_info_ext_api_request *ext_api_req =
			skip_ext_apis(&m_firmware_info);

	for (uint32_t i = 0; i < m_firmware_info.ext_api_request_num; i++) {
		if (fw_info_ext_api_check((uint32_t)*(ext_api_req->ext_api))
			&& ext_api_satisfies_req(*(ext_api_req->ext_api),
						ext_api_req)) {
			/* EXT_API requirement met. */
		} else if (ext_api_req->required) {
			/* EXT_API hard requirement not met. */
			printk("ERROR: Cannot fulfill EXT_API request.\r\n");
			k_panic();
		} else {
			/* EXT_API soft requirement not met. */
			printk("WARNING: Optional EXT_API request not "
				"fulfilled.\r\n");
			*ext_api_req->ext_api = NULL;
		}
		ADVANCE_EXT_API_REQ(ext_api_req);
	}

	return 0;
}

SYS_INIT(check_ext_api_requests, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);

/** Value to write to the "valid" member of fw_info to invalidate the image. */
#define INVALID_VAL 0xFFFF0000

BUILD_ASSERT((INVALID_VAL & CONFIG_FW_INFO_VALID_VAL)
			!= CONFIG_FW_INFO_VALID_VAL,
		"CONFIG_FW_INFO_VALID_VAL has been configured such that the "
		"image cannot be invalidated. Change the value so that writing "
		"INVALID_VAL has an effect.");

#ifdef CONFIG_NRFX_NVMC
void fw_info_invalidate(const struct fw_info *fw_info)
{
	/* Check if value has been written. */
	if (fw_info->valid == CONFIG_FW_INFO_VALID_VAL) {
		nrfx_nvmc_word_write((uint32_t)&(fw_info->valid), INVALID_VAL);
	}
}
#endif
