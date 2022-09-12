/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FW_INFO_H__
#define FW_INFO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/linker/sections.h>
#include <string.h>
#if USE_PARTITION_MANAGER
#include <pm_config.h>
#endif
#include <fw_info_bare.h>

/** @defgroup fw_info Firmware information linker helpers and build asserts.
 * @{
 */

/** @cond
 *  Remove from doc building.
 */
#define OFFSET_CHECK(type, member, value) \
		BUILD_ASSERT(offsetof(type, member) == value, \
				#member " has wrong offset")

/* Static asserts to ensure compatibility */
OFFSET_CHECK(struct fw_info_ext_api, ext_api_len, 12);
OFFSET_CHECK(struct fw_info_ext_api, ext_api_id, 16);
OFFSET_CHECK(struct fw_info_ext_api, ext_api_flags, 20);
OFFSET_CHECK(struct fw_info_ext_api, ext_api_version, 24);
/** @endcond
 */


/* Macro for initializing struct fw_info_ext_api instances in the correct
 * linker section. Also creates a uint8_t in another section to provide a count of
 * the number of struct fw_info_ext_api instances.
 */
#define EXT_API(ext_api_name, type, name) \
	Z_GENERIC_SECTION(.ext_apis) \
	const uint8_t _CONCAT(name, _ext_api_counter) = 0xFF; \
	BUILD_ASSERT((sizeof(type) % 4) == 0, \
			"Size of EXT_API " #type " is not word-aligned"); \
	struct __packed _CONCAT(name, _t) \
	{ \
		struct fw_info_ext_api header; \
		type ext_api; \
	}; \
	Z_GENERIC_SECTION(.firmware_info.1) __attribute__((used)) \
	const struct _CONCAT(name, _t) name = { \
	.header = {\
		.magic = {EXT_API_MAGIC}, \
		.ext_api_id = CONFIG_ ## ext_api_name ## _EXT_API_ID, \
		.ext_api_flags = CONFIG_ ## ext_api_name ## _EXT_API_FLAGS, \
		.ext_api_version = CONFIG_ ## ext_api_name ## _EXT_API_VER, \
		.ext_api_len = sizeof(struct __packed _CONCAT(name, _t)), \
	}, \
	.ext_api



/** @cond
 *  Remove from doc building.
 */
/* Static asserts to ensure compatibility */
OFFSET_CHECK(struct fw_info_ext_api_request, request, 0);
OFFSET_CHECK(struct fw_info_ext_api_request, ext_api_max_version, 28);
OFFSET_CHECK(struct fw_info_ext_api_request, required, 32);
OFFSET_CHECK(struct fw_info_ext_api_request, ext_api, 36);
/** @endcond
 */

/* Decorator for struct fw_info_ext_api_request instances to place them in the
 * correct linker section. Also creates a uint8_t in another section to provide a
 * count of the number of struct fw_info_ext_api instances.
 */
#define EXT_API_REQ(name, req, type, var_name) \
	Z_GENERIC_SECTION(.ext_apis_req) \
	const uint8_t _CONCAT(var_name, _ext_api_req_counter) = 0xFF; \
	__noinit const struct __packed \
	{ \
		struct fw_info_ext_api header; \
		type ext_api; \
	} *var_name; \
	Z_GENERIC_SECTION(.firmware_info.2) \
	__attribute__((used)) \
	const struct fw_info_ext_api_request _CONCAT(var_name, _req) = \
	{ \
		.request = {\
			.magic = {EXT_API_MAGIC}, \
			.ext_api_id = CONFIG_ ## name ## _EXT_API_ID, \
			.ext_api_flags = CONFIG_ ## name ## _EXT_API_FLAGS, \
			.ext_api_version = CONFIG_ ## name ## _EXT_API_VER, \
			.ext_api_len = sizeof(struct fw_info_ext_api_request), \
		}, \
		.ext_api_max_version = CONFIG_ ## name ## _EXT_API_MAX_VER, \
		.required = req, \
		.ext_api = (void *) &var_name, \
	}


/** @cond
 *  Remove from doc building.
 */
/* Static asserts to ensure compatibility */
OFFSET_CHECK(struct fw_info, magic, 0);
OFFSET_CHECK(struct fw_info, total_size, 12);
OFFSET_CHECK(struct fw_info, size, 16);
OFFSET_CHECK(struct fw_info, version, 20);
OFFSET_CHECK(struct fw_info, address, 24);
OFFSET_CHECK(struct fw_info, boot_address, 28);
OFFSET_CHECK(struct fw_info, valid, 32);
OFFSET_CHECK(struct fw_info, reserved, 36);
OFFSET_CHECK(struct fw_info, ext_api_num, 52);
OFFSET_CHECK(struct fw_info, ext_api_request_num, 56);
OFFSET_CHECK(struct fw_info, ext_apis, 60);
BUILD_ASSERT(sizeof(struct fw_info) == offsetof(struct fw_info, ext_apis),
	"Size of fw_info must assume ext_apis is empty.");
/** @endcond
 */



/* Find the difference between the start of the current image and the address
 * from which the firmware info offset is calculated.
 */
#if defined(PM_S0_PAD_SIZE) && (PM_ADDRESS == PM_S0_IMAGE_ADDRESS)
	#define FW_INFO_VECTOR_OFFSET PM_S0_PAD_SIZE
#elif defined(PM_S1_PAD_SIZE) && (PM_ADDRESS == PM_S1_IMAGE_ADDRESS)
	#define FW_INFO_VECTOR_OFFSET PM_S1_PAD_SIZE
#elif defined(PM_MCUBOOT_PAD_SIZE) && \
		(PM_ADDRESS == PM_MCUBOOT_PRIMARY_APP_ADDRESS)
	#define FW_INFO_VECTOR_OFFSET PM_MCUBOOT_PAD_SIZE
#else
	#define FW_INFO_VECTOR_OFFSET 0
#endif


/** @cond
 *  Remove from doc building.
 */
BUILD_ASSERT(ARRAY_SIZE(fw_info_allowed_offsets) == FW_INFO_OFFSET_COUNT,
		"Mismatch in the number of allowed offsets.");
/** @endcond
 */

/* Build time check of CONFIG_FW_INFO_OFFSET. */
#if (FW_INFO_OFFSET_COUNT != 5) \
			|| ((FW_INFO_CURRENT_OFFSET) != (FW_INFO_OFFSET0) && \
			(FW_INFO_CURRENT_OFFSET) != (FW_INFO_OFFSET1) && \
			(FW_INFO_CURRENT_OFFSET) != (FW_INFO_OFFSET2) && \
			(FW_INFO_CURRENT_OFFSET) != (FW_INFO_OFFSET3) && \
			(FW_INFO_CURRENT_OFFSET) != (FW_INFO_OFFSET4))
	#error FW_INFO_OFFSET not set to one of the allowed values.
#endif


/** Expose EXT_APIs to another firmware
 *
 * Populate the other firmware's @c ext_api_in with EXT_APIs from other images.
 *
 * @note This should be called immediately before booting the other firmware
 *       since it will likely corrupt the memory of the running firmware.
 *
 * @param[in]  fwinfo   Pointer to the other firmware's information structure.
 * @param[in]  provide  If true, populate ext_api_in.
 *                      If false, only check whether requirements can be
 *                      satisfied.
 *
 * @return Whether requirements could be satisfied.
 */
bool fw_info_ext_api_provide(const struct fw_info *fwinfo, bool provide);

/**Invalidate an image by manipulating its fw_info.
 *
 * @details Invalidation happens by setting the @c valid value to INVALID_VAL.
 *
 * @note This function needs to have @kconfig{CONFIG_NRFX_NVMC} enabled.
 *
 * @param[in]  fw_info  The info structure to invalidate.
 *                      This memory will be modified directly in flash.
 */
void fw_info_invalidate(const struct fw_info *fw_info);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
