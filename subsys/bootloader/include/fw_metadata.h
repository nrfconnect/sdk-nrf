/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef FW_METADATA_H__
#define FW_METADATA_H__

/*
 * The package will consist of (firmware | (padding) | validation_info),
 * where the firmware contains the firmware_info at a predefined location. The
 * padding is present if the validation_info needs alignment. The
 * validation_info is not directly referenced from the firmware_info since the
 * validation_info doesn't actually have to be placed after the firmware.
 *
 * Putting the firmware info inside the firmware instead of in front of it
 * removes the need to consider the padding before the vector table of the
 * firmware. It will also likely make it easier to add all the info at compile
 * time.
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <toolchain.h>
#include <assert.h>

#define MAGIC_LEN_WORDS (CONFIG_SB_MAGIC_LEN / sizeof(u32_t))

struct __packed fw_firmware_info {
	u32_t magic[MAGIC_LEN_WORDS];
	u32_t firmware_size;      /* Size without validation_info pointer and
				   * padding.
				   */
	u32_t firmware_version;   /* Monotonically increasing version counter.*/
	u32_t firmware_address;   /* The address of the start (vector table) of
				   * the firmware.
				   */
};

#define OFFSET_CHECK(type, member, value) \
		static_assert(offsetof(type, member) == value, \
				#member " has wrong offset")

/* Static asserts to ensure compatibility */
OFFSET_CHECK(struct fw_firmware_info, magic, 0);
OFFSET_CHECK(struct fw_firmware_info, firmware_size, CONFIG_SB_MAGIC_LEN);
OFFSET_CHECK(struct fw_firmware_info, firmware_version,
	(CONFIG_SB_MAGIC_LEN + 4));
OFFSET_CHECK(struct fw_firmware_info, firmware_address,
	(CONFIG_SB_MAGIC_LEN + 8));

struct __packed fw_validation_info {
	/* Magic value */
	u32_t magic[MAGIC_LEN_WORDS];

	/* The address of the start (vector table) of the firmware. */
	u32_t firmware_address;

	/* The hash of the firmware.*/
	u8_t  firmware_hash[CONFIG_SB_HASH_LEN];

	/* Public key to be used for signature verification. This must be
	 * checked against a trusted hash.
	 */
	u8_t  public_key[CONFIG_SB_PUBLIC_KEY_LEN];

	/* Signature over the firmware as represented by the firmware_address
	 * and firmware_size in the firmware_info.
	 */
	u8_t  signature[CONFIG_SB_SIGNATURE_LEN];
};


/* Static asserts to ensure compatibility */
OFFSET_CHECK(struct fw_validation_info, magic, 0);
OFFSET_CHECK(struct fw_validation_info, firmware_address, CONFIG_SB_MAGIC_LEN);
OFFSET_CHECK(struct fw_validation_info, firmware_hash,
	(CONFIG_SB_MAGIC_LEN + 4));
OFFSET_CHECK(struct fw_validation_info, public_key,
	(CONFIG_SB_MAGIC_LEN + 4 + CONFIG_SB_HASH_LEN));
OFFSET_CHECK(struct fw_validation_info, signature,
	(CONFIG_SB_MAGIC_LEN + 4 + CONFIG_SB_HASH_LEN
	+ CONFIG_SB_SIGNATURE_LEN));

/*
 * Can be used to make the firmware discoverable in other locations, e.g. when
 * searching backwards. This struct would typically be constructed locally, so
 * it needs no version.
 */
struct __packed fw_validation_pointer {
	u32_t magic[MAGIC_LEN_WORDS];
	const struct fw_validation_info *validation_info;
};

/* Static asserts to ensure compatibility */
OFFSET_CHECK(struct fw_validation_pointer, magic, 0);
OFFSET_CHECK(struct fw_validation_pointer, validation_info,
	CONFIG_SB_MAGIC_LEN);


/* All parameters must be word-aligned */
static inline bool memeq_32(const void *expected, const void *actual, u32_t len)
{
	__ASSERT(!((u32_t)expected & 3)
	      && !((u32_t)actual & 3)
	      && !((u32_t)len & 3),
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
	if (((u32_t)expected & 3) || ((u32_t)actual & 3) || ((u32_t)len & 3)) {
		/* Parameters are not word aligned. */
		return memeq_8(expected, actual, len);
	} else {
		return memeq_32(expected, actual, len);
	}
}

/*
 * Get a pointer to the firmware_info structure inside the firmware.
 */
static inline const struct fw_firmware_info *
firmware_info_get(u32_t firmware_address) {
	u32_t finfo_addr = firmware_address + CONFIG_SB_FIRMWARE_INFO_OFFSET;
	const struct fw_firmware_info *finfo;
	const u32_t firmware_info_magic[] = {FIRMWARE_INFO_MAGIC};

	finfo = (const struct fw_firmware_info *)(finfo_addr);
	if (memeq(finfo->magic, firmware_info_magic, CONFIG_SB_MAGIC_LEN)) {
		return finfo;
	}
	return NULL;
}

/*
 * Find the validation_info at the end of the firmware.
 */
static inline const struct fw_validation_info *
validation_info_find(const struct fw_firmware_info *finfo, u32_t
		search_distance) {
	u32_t vinfo_addr = finfo->firmware_address + finfo->firmware_size;
	const struct fw_validation_info *vinfo;
	const u32_t validation_info_magic[] = {VALIDATION_INFO_MAGIC};

	for (int i = 0; i <= search_distance; i++) {
		vinfo = (const struct fw_validation_info *)(vinfo_addr + i);
		if (memeq(vinfo->magic, validation_info_magic,
					CONFIG_SB_MAGIC_LEN)) {
			return vinfo;
		}
	}
	return NULL;
}

#endif
