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
#include <string.h>
#if USE_PARTITION_MANAGER
#include <pm_config.h>
#endif

/** @defgroup fw_info Firmware info structure
 * @{
 */

#define MAGIC_LEN_WORDS (CONFIG_FW_INFO_MAGIC_LEN / sizeof(u32_t))

struct fw_info_ext_api;

/**@brief Function that returns an EXT_API.
 *
 * @param[in]    id      Which EXT_API to get.
 * @param[in]    index   If there are multiple EXT_APIs available with the same
 *                       ID, retrieve the different ones with this.
 * @param[out]   ext_api Pointer to the ext_api with the given id and index.
 *
 * @retval 0        Success.
 * @retval -ENOENT  id not found.
 * @retval -EBADF   index too large.
 * @retval -EFAULT  ext_api was NULL.
 */
typedef int (*fw_info_ext_api_getter)(u32_t id, u32_t index,
				const struct fw_info_ext_api **ext_api);

/**
 * This is a data structure that is placed at a specific offset inside a
 * firmware image so it can be consistently read by external parties. The
 * specific offset makes it easy to find, and the magic value at the start
 * guarantees that it contains data of a specific format.
 */
struct __packed fw_info {
	/* Magic value to verify that the struct has the correct format. */
	u32_t magic[MAGIC_LEN_WORDS];

	/* Size of the firmware image code. */
	u32_t firmware_size;

	/* Monotonically increasing version counter.*/
	u32_t firmware_version;

	/* The address of the start (vector table) of the firmware. */
	u32_t firmware_address;

	/* Where to place the getter for the EXT_API provided to this firmware.
	 */
	fw_info_ext_api_getter *ext_api_in;

	/* This firmware's EXT_API getter. */
	const fw_info_ext_api_getter ext_api_out;
};

/** @cond
 *  Remove from doc building.
 */
#define OFFSET_CHECK(type, member, value) \
		BUILD_ASSERT_MSG(offsetof(type, member) == value, \
				#member " has wrong offset")

/* Static asserts to ensure compatibility */
OFFSET_CHECK(struct fw_info, magic, 0);
OFFSET_CHECK(struct fw_info, firmware_size, CONFIG_FW_INFO_MAGIC_LEN);
OFFSET_CHECK(struct fw_info, firmware_version,
	(CONFIG_FW_INFO_MAGIC_LEN + 4));
OFFSET_CHECK(struct fw_info, firmware_address,
	(CONFIG_FW_INFO_MAGIC_LEN + 8));

/** @endcond
 */


/**
 * This struct is meant to serve as a header before a list of function pointers
 * (or something else) that constitute the actual EXT_API. How to use the
 * EXT_API, such as the signatures of all the functions in the list must be
 * unambiguous for an ID/version combination.
 */
struct __packed fw_info_ext_api {
	/* Magic value to verify that the struct has the correct format. */
	u32_t magic[MAGIC_LEN_WORDS];

	/* The id of the EXT_API. */
	u32_t ext_api_id;

	/* Flags specifying properties of the EXT_API. */
	u32_t ext_api_flags;

	/* The version of this EXT_API. */
	u32_t ext_api_version;

	/* The length of this header plus everything after this header. Must be
	 * word-aligned.
	 */
	u32_t ext_api_len;
};


#define OFFSET_CHECK_EXT_API(type, member, value) \
	BUILD_ASSERT_MSG(offsetof(type, header.member) == value, \
		"ext_api " #type " has wrong offset for header." #member)

#define EXT_API(type, name) \
	OFFSET_CHECK_EXT_API(type, magic, 0); \
	OFFSET_CHECK_EXT_API(type, ext_api_id, CONFIG_FW_INFO_MAGIC_LEN); \
	OFFSET_CHECK_EXT_API(type, ext_api_flags,\
				(CONFIG_FW_INFO_MAGIC_LEN + 4)); \
	OFFSET_CHECK_EXT_API(type, ext_api_version,\
				(CONFIG_FW_INFO_MAGIC_LEN + 8)); \
	OFFSET_CHECK_EXT_API(type, ext_api_len,\
				(CONFIG_FW_INFO_MAGIC_LEN + 12)); \
	BUILD_ASSERT_MSG((sizeof(type) % 4) == 0, \
			"ext_api " #type " is not word-aligned"); \
	extern const type name; \
	Z_GENERIC_SECTION(.ext_apis) __attribute__((used)) \
	const type * const _CONCAT(name, _ptr) = &name; \
	__attribute__((used)) \
	const type name



#define FW_INFO_EXT_API_INIT(id, flags, version, total_size) \
	{ \
		.magic = {EXT_API_MAGIC}, \
		.ext_api_id = id, \
		.ext_api_flags = flags, \
		.ext_api_version = version, \
		.ext_api_len = total_size, \
	}

/* Shorthand for declaring function that will be exposed through an ext_api.
 * This will define a function pointer type as well as declare the function.
 */
#define EXT_API_FUNCTION(retval, name, ...) \
	typedef retval (*name ## _t) (__VA_ARGS__); \
	retval name (__VA_ARGS__)


/* All parameters must be word-aligned */
static inline bool memeq_32(const void *expected, const void *actual, u32_t len)
{
	__ASSERT(!((u32_t)expected % 4)
	      && !((u32_t)actual % 4)
	      && !((u32_t)len % 4),
		"A parameter is unaligned.");
	const u32_t *expected_32 = (const u32_t *) expected;
	const u32_t *actual_32   = (const u32_t *) actual;

	for (u32_t i = 0; i < (len / sizeof(u32_t)); i++) {
		if (expected_32[i] != actual_32[i]) {
			return false;
		}
	}
	return true;
}

static inline bool memeq_8(const void *expected, const void *actual, u32_t len)
{
	const u8_t *expected_8 = (const u8_t *) expected;
	const u8_t *actual_8   = (const u8_t *) actual;

	for (u32_t i = 0; i < len; i++) {
		if (expected_8[i] != actual_8[i]) {
			return false;
		}
	}
	return true;
}

static inline bool memeq(const void *expected, const void *actual, u32_t len)
{
	if (((u32_t)expected % 4) || ((u32_t)actual % 4) || ((u32_t)len % 4)) {
		/* Parameters are not word aligned. */
		return memeq_8(expected, actual, len);
	} else {
		return memeq_32(expected, actual, len);
	}
}

/* Check and provide a pointer to a firmware_info structure.
 *
 * @return pointer if valid, NULL if not.
 */
static inline const struct fw_info *fw_info_check(u32_t fw_info_addr)
{
	const struct fw_info *finfo;
	const u32_t fw_info_magic[] = {FIRMWARE_INFO_MAGIC};

	finfo = (const struct fw_info *)(fw_info_addr);
	if (memeq(finfo->magic, fw_info_magic, CONFIG_FW_INFO_MAGIC_LEN)) {
		return finfo;
	}
	return NULL;
}


/* The supported offsets for the fw_info struct. */
#define FW_INFO_OFFSET0 0x200
#define FW_INFO_OFFSET1 0x400
#define FW_INFO_OFFSET2 0x800
#define FW_INFO_OFFSET_COUNT 3

/* Find the difference between the start of the current image and the address
 * from which the firmware info offset is calculated.
 */
#if defined(PM_S0_PAD_SIZE) && (PM_ADDRESS == PM_S0_IMAGE_ADDRESS)
	#define VECTOR_OFFSET PM_S0_PAD_SIZE
#elif defined(PM_S1_PAD_SIZE) && (PM_ADDRESS == PM_S1_IMAGE_ADDRESS)
	#define VECTOR_OFFSET PM_S1_PAD_SIZE
#elif defined(PM_MCUBOOT_PAD_SIZE) && \
		(PM_ADDRESS == PM_MCUBOOT_PRIMARY_APP_ADDRESS)
	#define VECTOR_OFFSET PM_MCUBOOT_PAD_SIZE
#else
	#define VECTOR_OFFSET 0
#endif

#define CURRENT_OFFSET (CONFIG_FW_INFO_OFFSET + VECTOR_OFFSET)

static const u32_t allowed_offsets[] = {FW_INFO_OFFSET0, FW_INFO_OFFSET1,
					FW_INFO_OFFSET2};

/** @cond
 *  Remove from doc building.
 */
BUILD_ASSERT_MSG(ARRAY_SIZE(allowed_offsets) == FW_INFO_OFFSET_COUNT,
		"Mismatch in the number of allowed offsets.");
/** @endcond
 */

#if (FW_INFO_OFFSET_COUNT != 3) || ((CURRENT_OFFSET) != (FW_INFO_OFFSET0) && \
				(CURRENT_OFFSET) != (FW_INFO_OFFSET1) && \
				(CURRENT_OFFSET) != (FW_INFO_OFFSET2))
	#error FW_INFO_OFFSET not set to one of the allowed values.
#endif

/* Search for the firmware_info structure inside the firmware. */
static inline const struct fw_info *fw_info_find(u32_t firmware_address)
{
	const struct fw_info *finfo;

	for (u32_t i = 0; i < FW_INFO_OFFSET_COUNT; i++) {
		finfo = fw_info_check(firmware_address +
						allowed_offsets[i]);
		if (finfo) {
			return finfo;
		}
	}
	return NULL;
}


/* Check a fw_info_ext_api pointer. */
static inline bool fw_info_ext_api_check(
				const struct fw_info_ext_api *ext_api_info)
{
	const u32_t ext_api_info_magic[] = {EXT_API_MAGIC};

	return memeq(ext_api_info->magic, ext_api_info_magic,
		CONFIG_FW_INFO_MAGIC_LEN);
}


/**Expose EXT_APIs to another firmware
 *
 * Populate the other firmware's @c ext_api_in with EXT_APIs from other images.
 *
 * @note This is should be called immediately before booting the other firmware
 *       since it will likely corrupt the memory of the running firmware.
 *
 * @param[in]  fwinfo  Pointer to the other firmware's information structure.
 */
void fw_info_ext_api_provide(const struct fw_info *fwinfo);

/**Get a single EXT_API.
 *
 * @param[in]    id      Which EXT_API to get.
 * @param[in]    index   If there are multiple EXT_APIs available with the same
 *                       ID, retrieve the different ones with this.
 *
 * @return The EXT_API, or NULL, if it wasn't found.
 */
const struct fw_info_ext_api *fw_info_ext_api_get(u32_t id, u32_t index);

/**Find an EXT_API based on a version range.
 *
 * @param[in]  id           The ID of the EXT_API to find.
 * @param[in]  flags        The required flags of the EXT_API to find. The
 *                          returned EXT_API may have other flags set as well.
 * @param[in]  min_version  The minimum acceptable EXT_API version.
 * @param[in]  max_version  One more than the maximum acceptable EXT_API
 *                          version.
 *
 * @return The EXT_API, or NULL if none was found.
 */
const struct fw_info_ext_api *fw_info_ext_api_find(u32_t id, u32_t flags,
					u32_t min_version, u32_t max_version);

  /** @} */

#ifdef __cplusplus
}
#endif

#endif
