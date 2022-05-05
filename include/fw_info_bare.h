/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FW_INFO_BARE_H__
#define FW_INFO_BARE_H__

/** @defgroup fw_info_bare Firmware info structure definition and inline helpers
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>

#define MAGIC_LEN_WORDS (CONFIG_FW_INFO_MAGIC_LEN / sizeof(uint32_t))

/* The supported offsets for the fw_info struct. */
#define FW_INFO_OFFSET0 0x0
#define FW_INFO_OFFSET1 0x200
#define FW_INFO_OFFSET2 0x400
#define FW_INFO_OFFSET3 0x800
#define FW_INFO_OFFSET4 0x1000
#define FW_INFO_OFFSET_COUNT 5

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
	uint32_t magic[MAGIC_LEN_WORDS];

	/* The length of this header plus everything after this header. Must be
	 * word-aligned.
	 */
	uint32_t ext_api_len;

	/* The id of the EXT_API. */
	uint32_t ext_api_id;

	/* Flags specifying properties of the EXT_API. */
	uint32_t ext_api_flags;

	/* The version of this EXT_API. */
	uint32_t ext_api_version;
};

/* Check and provide a pointer to a fw_info_ext_api structure.
 *
 * @return pointer if valid, NULL if not.
 */
static inline const struct fw_info_ext_api *fw_info_ext_api_check(
							uint32_t ext_api_addr)
{
	const struct fw_info_ext_api *ext_api;
	const uint32_t ext_api_magic[] = {EXT_API_MAGIC};

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
	uint32_t ext_api_max_version;

	/* This EXT_API is required. I.e. having this EXT_API available is a
	 * hard requirement.
	 */
	uint32_t required;

	/* Where to place a pointer to the EXT_API. */
	const struct fw_info_ext_api **ext_api;
};


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
	uint32_t magic[MAGIC_LEN_WORDS];

	/* Total size of this fw_info struct including the EXT_API lists. */
	uint32_t total_size;

	/* Size of the firmware image code. */
	uint32_t size;

	/* Monotonically increasing version counter.*/
	uint32_t version;

	/* The address of the start of the image. */
	uint32_t address;

	/* The address of the boot point (vector table) of the firmware. */
	uint32_t boot_address;

	/* Value that can be modified to invalidate the firmware. Has the value
	 * CONFIG_FW_INFO_VALID_VAL when valid.
	 */
	uint32_t valid;

	/* Reserved values (set to 0) */
	uint32_t reserved[4];

	/* The number of EXT_APIs in the @ref ext_apis list. */
	uint32_t ext_api_num;

	/* The number of EXT_API requests in the @ref ext_apis list. */
	uint32_t ext_api_request_num;

	/* A list of @ref ext_api_num EXT_APIs followed by @ref
	 * ext_api_request_num EXT_API requests. Since the entries have
	 * different lengths, the @ref ext_api_len of an entry is used to find
	 * the next entry. To get to the EXT_API requests, first iterate over
	 * all EXT_APIs.
	 */
	const struct fw_info_ext_api ext_apis[];
};

/* Check and provide a pointer to a firmware_info structure.
 *
 * @return pointer if valid, NULL if not.
 */
static inline const struct fw_info *fw_info_check(uint32_t fw_info_addr)
{
	const struct fw_info *finfo;
	const uint32_t fw_info_magic[] = {FIRMWARE_INFO_MAGIC};

	finfo = (const struct fw_info *)(fw_info_addr);
	if (memcmp(finfo->magic, fw_info_magic, CONFIG_FW_INFO_MAGIC_LEN)
		== 0) {
		return finfo;
	}
	return NULL;
}

/* The actual fw_info struct offset accounting for any space in front of the
 * image, e.g. when there's a header.
 */
#define FW_INFO_CURRENT_OFFSET (CONFIG_FW_INFO_OFFSET + FW_INFO_VECTOR_OFFSET)

/* Array for run time usage. */
static const uint32_t fw_info_allowed_offsets[] = {
					FW_INFO_OFFSET0, FW_INFO_OFFSET1,
					FW_INFO_OFFSET2, FW_INFO_OFFSET3,
					FW_INFO_OFFSET4};


/** Search for the firmware_info structure inside the firmware.
 *
 * @param[in] firmware_address  The start of the image. The function will search
 *                              at the allowed offsets from firmware_address.
 *
 * @return  A pointer to the fw_info struct if found. Otherwise NULL.
 */
static inline const struct fw_info *fw_info_find(uint32_t firmware_address)
{
	const struct fw_info *finfo;

	for (uint32_t i = 0; i < FW_INFO_OFFSET_COUNT; i++) {
		finfo = fw_info_check(firmware_address +
						fw_info_allowed_offsets[i]);
		if (finfo) {
			return finfo;
		}
	}
	return NULL;
}

typedef bool (*fw_info_ext_api_provide_t)(const struct fw_info *fwinfo,
					bool provide);

/**
 * @brief Structure describing the EXT_API_PROVIDE EXT_API.
 */
struct ext_api_provide_ext_api {
	fw_info_ext_api_provide_t ext_api_provide;
};

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* FW_INFO_BARE_H__ */
