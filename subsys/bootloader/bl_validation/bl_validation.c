/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bl_validation.h>
#include <zephyr/types.h>
#include <errno.h>
#include <misc/printk.h>
#include <fw_metadata.h>
#include <bl_crypto.h>
#include <provision.h>

struct __packed fw_validation_info {
	/* Magic value to verify that the struct has the correct type. */
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
OFFSET_CHECK(struct fw_validation_info, firmware_address, CONFIG_FW_MAGIC_LEN);
OFFSET_CHECK(struct fw_validation_info, firmware_hash,
	(CONFIG_FW_MAGIC_LEN + 4));
OFFSET_CHECK(struct fw_validation_info, public_key,
	(CONFIG_FW_MAGIC_LEN + 4 + CONFIG_SB_HASH_LEN));
OFFSET_CHECK(struct fw_validation_info, signature,
	(CONFIG_FW_MAGIC_LEN + 4 + CONFIG_SB_HASH_LEN
	+ CONFIG_SB_SIGNATURE_LEN));

/* Can be used to make the firmware discoverable in other locations, e.g. when
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
	CONFIG_FW_MAGIC_LEN);


/* Find the validation_info at the end of the firmware. */
static inline const struct fw_validation_info *
validation_info_find(const struct fw_firmware_info *finfo,
			u32_t search_distance)
{
	u32_t vinfo_addr = finfo->firmware_address + finfo->firmware_size;
	const struct fw_validation_info *vinfo;
	const u32_t validation_info_magic[] = {VALIDATION_INFO_MAGIC};

	for (int i = 0; i <= search_distance; i++) {
		vinfo = (const struct fw_validation_info *)(vinfo_addr + i);
		if (memeq(vinfo->magic, validation_info_magic,
					CONFIG_FW_MAGIC_LEN)) {
			return vinfo;
		}
	}
	return NULL;
}

bool verify_firmware(u32_t address)
{
	/* Some key data storage backends require word sized reads, hence
	 * we need to ensure word alignment for 'key_data'
	 */
	u32_t key_data[CONFIG_SB_PUBLIC_KEY_HASH_LEN/4];
	int retval = -EFAULT;
	int err;
	const struct fw_firmware_info *fw_info;
	const struct fw_validation_info *fw_ver_info;

	fw_info = fw_find_firmware_info(address);

	if (!fw_info) {
		printk("Could not find valid firmware info inside "
				    "firmware. Aborting boot!\n\r");
		return false;
	}

	fw_ver_info = validation_info_find(fw_info, 4);

	if (!fw_ver_info) {
		printk("Could not find valid firmware validation "
			  "info trailing firmware. Aborting boot!\n\r");
		return false;
	}

	err = bl_crypto_init();
	if (err) {
		printk("bl_crypto_init() returned %d. Aborting boot!\n\r", err);
		return false;
	}

	u32_t num_public_keys = num_public_keys_read();

	for (u32_t key_data_idx = 0; key_data_idx < num_public_keys;
			key_data_idx++) {
		if (public_key_data_read(key_data_idx, &key_data[0],
				CONFIG_SB_PUBLIC_KEY_HASH_LEN) < 0) {
			retval = -EFAULT;
			break;
		}
		retval = bl_root_of_trust_verify(fw_ver_info->public_key,
					(u8_t *)key_data,
					fw_ver_info->signature,
					(u8_t *)fw_ver_info->firmware_address,
					fw_info->firmware_size);
		if (retval != -ESIGINV) {
			break;
		}
	}

	if (retval != 0) {
		printk("Firmware validation failed with error %d. "
			    "Aborting boot!\n\r",
			    retval);
		return false;
	}

	return true;
}
