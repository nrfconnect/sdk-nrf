/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef FW_INFO_H__
#define FW_INFO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <stddef.h>
#include <toolchain.h>
#include <sys/util.h>
#include <sys/__assert.h>
#include <linker/sections.h>
#include <string.h>
#if USE_PARTITION_MANAGER
#include <pm_config.h>
#endif

/** @defgroup fw_info Firmware info structure
 * @{
 */

#define MAGIC_LEN_WORDS (CONFIG_FW_INFO_MAGIC_LEN / sizeof(u32_t))


/**
 * This struct serves as a header before an EXT_API.
 *
 * @details This header contains basic metadata about the EXT_API: A unique
 *          identifier, version and flags to be used for compatibility matching.
 *          The payload that comes after this header is completely
 *          implementation dependent for each EXT_API.
 *
 * @note How to use the EXT_API, such as the signatures of all the functions in
 *       the list must be unambiguous for an ID/version combination.
 */
struct __packed fw_info_ext_api {
	/* Magic value to verify that the struct has the correct format.
	 * The magic value will change whenever the format changes.
	 */
	u32_t magic[MAGIC_LEN_WORDS];

	/* The length of this header plus everything after this header. Must be
	 * word-aligned.
	 */
	u32_t ext_api_len;

	/* The id of the EXT_API. */
	u32_t ext_api_id;

	/* Flags specifying properties of the EXT_API. */
	u32_t ext_api_flags;

	/* The version of this EXT_API. */
	u32_t ext_api_version;
};

/** @cond
 *  Remove from doc building.
 */
#define OFFSET_CHECK(type, member, value) \
		BUILD_ASSERT_MSG(offsetof(type, member) == value, \
				#member " has wrong offset")

/* Static asserts to ensure compatibility */
OFFSET_CHECK(struct fw_info_ext_api, ext_api_len, 12);
OFFSET_CHECK(struct fw_info_ext_api, ext_api_id, 16);
OFFSET_CHECK(struct fw_info_ext_api, ext_api_flags, 20);
OFFSET_CHECK(struct fw_info_ext_api, ext_api_version, 24);
/** @endcond
 */


/* Macro for initializing struct fw_info_ext_api instances in the correct
 * linker section. Also creates a u8_t in another section to provide a count of
 * the number of struct fw_info_ext_api instances.
 */
#define EXT_API(ext_api_name, type, name) \
	Z_GENERIC_SECTION(.ext_apis) \
	const u8_t _CONCAT(name, _ext_api_counter) = 0xFF; \
	BUILD_ASSERT_MSG((sizeof(type) % 4) == 0, \
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


/* Check and provide a pointer to a fw_info_ext_api structure.
 *
 * @return pointer if valid, NULL if not.
 */
static inline const struct fw_info_ext_api *fw_info_ext_api_check(
							u32_t ext_api_addr)
{
	const struct fw_info_ext_api *ext_api;
	const u32_t ext_api_magic[] = {EXT_API_MAGIC};

	ext_api = (const struct fw_info_ext_api *)(ext_api_addr);
	if (memcmp(ext_api->magic, ext_api_magic, CONFIG_FW_INFO_MAGIC_LEN)
		== 0) {
		return ext_api;
	}
	return NULL;
}


/**
 * A struct that is used to request an EXT_API.
 *
 * @details It contains a pointer to a non-initialized pointer that a previous
 *          boot stage will populate with a pointer to a compliant EXT_API. The
 *          EXT_API could be located in the boot stage itself, or in a third
 *          image. An EXT_API fulfills a request if the ID matches, all flags in
 *          the request are set in the EXT_API, and the version falls between
 *          the minimum and maximum (inclusive).
 *
 *          The request is placed in the list of requests in the fw_info struct.
 *
 * @note If `required` is true, the image making the request will not function
 *       unless it has access to the EXT_API.
 *
 */
struct __packed fw_info_ext_api_request {
	/* The requested EXT_API. This struct defines the requested ID as well
	 * as the minimum required version and the flags that must be set.
	 */
	struct fw_info_ext_api request;

	/* The maximum accepted version. */
	u32_t ext_api_max_version;

	/* This EXT_API is required. I.e. having this EXT_API available is a
	 * hard requirement.
	 */
	u32_t required;

	/* Where to place a pointer to the EXT_API. */
	const struct fw_info_ext_api **ext_api;
};

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
 * correct linker section. Also creates a u8_t in another section to provide a
 * count of the number of struct fw_info_ext_api instances.
 */
#define EXT_API_REQ(name, req, type, var_name) \
	Z_GENERIC_SECTION(.ext_apis_req) \
	const u8_t _CONCAT(var_name, _ext_api_req_counter) = 0xFF; \
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


/**
 * The top level firmware info struct.
 *
 * @details This is a data structure that is placed at a specific offset inside
 *          a firmware image so it can be consistently read by external parties.
 *          The specific offset makes it easy to find, and the magic value at
 *          the start guarantees that it contains data of a specific format.
 */
struct __packed fw_info {
	/* Magic value to verify that the struct has the correct format.
	 * The magic value will change whenever the format changes.
	 */
	u32_t magic[MAGIC_LEN_WORDS];

	/* Total size of this fw_info struct including the EXT_API lists. */
	u32_t total_size;

	/* Size of the firmware image code. */
	u32_t size;

	/* Monotonically increasing version counter.*/
	u32_t version;

	/* The address of the start of the image. */
	u32_t address;

	/* The address of the boot point (vector table) of the firmware. */
	u32_t boot_address;

	/* Value that can be modified to invalidate the firmware. Has the value
	 * CONFIG_FW_INFO_VALID_VAL when valid.
	 */
	u32_t valid;

	/* Reserved values (set to 0) */
	u32_t reserved[4];

	/* The number of EXT_APIs in the @ref ext_apis list. */
	u32_t ext_api_num;

	/* The number of EXT_API requests in the @ref ext_apis list. */
	u32_t ext_api_request_num;

	/* A list of @ref ext_api_num EXT_APIs followed by @ref
	 * ext_api_request_num EXT_API requests. Since the entries have
	 * different lengths, the @ref ext_api_len of an entry is used to find
	 * the next entry. To get to the EXT_API requests, first iterate over
	 * all EXT_APIs.
	 */
	const struct fw_info_ext_api ext_apis[];
};

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
BUILD_ASSERT_MSG(sizeof(struct fw_info) == offsetof(struct fw_info, ext_apis),
	"Size of fw_info must assume ext_apis is empty.");
/** @endcond
 */

/* Check and provide a pointer to a firmware_info structure.
 *
 * @return pointer if valid, NULL if not.
 */
static inline const struct fw_info *fw_info_check(u32_t fw_info_addr)
{
	const struct fw_info *finfo;
	const u32_t fw_info_magic[] = {FIRMWARE_INFO_MAGIC};

	finfo = (const struct fw_info *)(fw_info_addr);
	if (memcmp(finfo->magic, fw_info_magic, CONFIG_FW_INFO_MAGIC_LEN)
		== 0) {
		return finfo;
	}
	return NULL;
}


/* The supported offsets for the fw_info struct. */
#define FW_INFO_OFFSET0 0x0
#define FW_INFO_OFFSET1 0x200
#define FW_INFO_OFFSET2 0x400
#define FW_INFO_OFFSET3 0x800
#define FW_INFO_OFFSET4 0x1000
#define FW_INFO_OFFSET_COUNT 5

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

/* The actual fw_info struct offset accounting for any space in front of the
 * image, e.g. when there's a header.
 */
#define FW_INFO_CURRENT_OFFSET (CONFIG_FW_INFO_OFFSET + FW_INFO_VECTOR_OFFSET)

/* Array for run time usage. */
static const u32_t fw_info_allowed_offsets[] = {
					FW_INFO_OFFSET0, FW_INFO_OFFSET1,
					FW_INFO_OFFSET2, FW_INFO_OFFSET3,
					FW_INFO_OFFSET4};

/** @cond
 *  Remove from doc building.
 */
BUILD_ASSERT_MSG(ARRAY_SIZE(fw_info_allowed_offsets) == FW_INFO_OFFSET_COUNT,
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


/** Search for the firmware_info structure inside the firmware.
 *
 * @param[in] firmware_address  The start of the image. The function will search
 *                              at the allowed offsets from firmware_address.
 *
 * @return  A pointer to the fw_info struct if found. Otherwise NULL.
 */
static inline const struct fw_info *fw_info_find(u32_t firmware_address)
{
	const struct fw_info *finfo;

	for (u32_t i = 0; i < FW_INFO_OFFSET_COUNT; i++) {
		finfo = fw_info_check(firmware_address +
						fw_info_allowed_offsets[i]);
		if (finfo) {
			return finfo;
		}
	}
	return NULL;
}


/** Expose EXT_APIs to another firmware
 *
 * Populate the other firmware's @c ext_api_in with EXT_APIs from other images.
 *
 * @note This is should be called immediately before booting the other firmware
 *       since it will likely corrupt the memory of the running firmware.
 *
 * @param[in]  fw_info  Pointer to the other firmware's information structure.
 * @param[in]  provide  If true, populate ext_api_in.
 *                      If false, only check whether requirements can be
 *                      satisfied.
 *
 * @return Whether requirements could be satisified.
 */
bool fw_info_ext_api_provide(const struct fw_info *fwinfo, bool provide);

/**Invalidate an image by manipulating its fw_info.
 *
 * @details Invalidation happens by setting the @c valid value to 0x0.
 *
 * @note This function needs to have CONFIG_NRF_NVMC enabled.
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
