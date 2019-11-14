/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bl_validation.h>
#include <zephyr/types.h>
#include <fw_info.h>
#include <kernel.h>


#ifdef CONFIG_BL_VALIDATE_FW_EXT_API_REQUIRED
EXT_API_REQ(BL_VALIDATE_FW, 1, struct bl_validate_fw_ext_api, bl_validate_fw);

bool bl_validate_firmware(u32_t fw_dst_address, u32_t fw_src_address)
{
	return bl_validate_fw->ext_api.bl_validate_firmware(fw_dst_address,
							fw_src_address);
}


#else
#include <errno.h>
#include <sys/printk.h>
#include <toolchain.h>
#include <bl_crypto.h>
#include <provision.h>

#define PRINT(...) if (!external) printk(__VA_ARGS__)

struct __packed fw_validation_info {
	/* Magic value to verify that the struct has the correct type. */
	u32_t magic[MAGIC_LEN_WORDS];

	/* The address of the start (vector table) of the firmware. */
	u32_t address;

	/* The hash of the firmware.*/
	u8_t  hash[CONFIG_SB_HASH_LEN];

	/* Public key to be used for signature verification. This must be
	 * checked against a trusted hash.
	 */
	u8_t  public_key[CONFIG_SB_PUBLIC_KEY_LEN];

	/* Signature over the firmware as represented by the address and size in
	 * the firmware_info.
	 */
	u8_t  signature[CONFIG_SB_SIGNATURE_LEN];
};


/* Static asserts to ensure compatibility */
OFFSET_CHECK(struct fw_validation_info, magic, 0);
OFFSET_CHECK(struct fw_validation_info, address, 12);
OFFSET_CHECK(struct fw_validation_info, hash, 16);
OFFSET_CHECK(struct fw_validation_info, public_key, (16 + CONFIG_SB_HASH_LEN));
OFFSET_CHECK(struct fw_validation_info, signature, (16 + CONFIG_SB_HASH_LEN
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
OFFSET_CHECK(struct fw_validation_pointer, validation_info, 12);

static bool validation_info_check(const struct fw_validation_info *vinfo)
{
	const u32_t validation_info_magic[] = {VALIDATION_INFO_MAGIC};

	if (memcmp(vinfo->magic, validation_info_magic,
					CONFIG_FW_INFO_MAGIC_LEN) == 0) {
		return vinfo;
	}
	return NULL;
}


/* Find the validation_info at the end of the firmware. */
static const struct fw_validation_info *
validation_info_find(u32_t start_address, u32_t search_distance)
{
	const struct fw_validation_info *vinfo;

	for (int i = 0; i <= search_distance; i++) {
		vinfo = (const struct fw_validation_info *)(start_address + i);
		if (validation_info_check(vinfo)) {
			return vinfo;
		}
	}
	return NULL;
}


static bool validate_firmware(u32_t fw_dst_address, u32_t fw_src_address,
		const struct fw_info *fwinfo, bool external)
{
	/* Some key data storage backends require word sized reads, hence
	 * we need to ensure word alignment for 'key_data'
	 */
	__aligned(4) u8_t key_data[CONFIG_SB_PUBLIC_KEY_HASH_LEN];
	int retval = -EFAULT;
	int err;
	const struct fw_validation_info *fw_val_info;
	u32_t num_public_keys;
	bl_root_of_trust_verify_t rot_verify = external ?
					bl_root_of_trust_verify_external :
					bl_root_of_trust_verify;

	if (!fwinfo) {
		PRINT("NULL parameter.\n\r");
		return false;
	}

	if (!fw_info_check((u32_t)fwinfo)) {
		PRINT("Invalid firmware info format.\n\r");
		return false;
	}

	if (fwinfo->valid != CONFIG_FW_INFO_VALID_VAL) {
		PRINT("Firwmare has been invalidated: 0x%x.\n\r",
			fwinfo->valid);
		return false;
	}

	if (!(((u32_t)fwinfo >= fw_src_address)
		&& (((u32_t)fwinfo + fwinfo->total_size)
			< (fw_src_address + fwinfo->size)))) {
		PRINT("Firmware info is not within signed region.\n\r");
		return false;
	}

	if (fw_dst_address != fwinfo->address) {
		PRINT("The firmware doesn't belong at destination addr.\n\r");
		return false;
	}

	fw_val_info = validation_info_find(
		fw_src_address + fwinfo->size, 4);

	if (!fw_val_info) {
		PRINT("Could not find valid firmware validation info.\n\r");
		return false;
	}

	if (fw_val_info->address != fwinfo->address) {
		PRINT("Validation info doesn't belong to this firmware.\n\r");
		return false;
	}

	err = bl_crypto_init();
	if (err) {
		PRINT("bl_crypto_init() returned %d.\n\r", err);
		return false;
	}

	num_public_keys = num_public_keys_read();

	for (u32_t key_data_idx = 0; key_data_idx < num_public_keys;
			key_data_idx++) {
		int read_retval = public_key_data_read(key_data_idx,
				key_data, CONFIG_SB_PUBLIC_KEY_HASH_LEN);
		if (read_retval < 0) {
			if (read_retval == -EINVAL) {
				/* Invalidated key, try next key. */
				PRINT("Key %d has been invalidated.\n\r",
					key_data_idx);
				continue;
			} else {
				PRINT("public_key_data_read failed: %d.\n\r",
					read_retval);
				retval = -EFAULT;
				break;
			}
		}

		PRINT("Verifying signature against key %d.\n\r", key_data_idx);
		PRINT("Hash: 0x%02x...%02x\r\n", key_data[0],
			key_data[CONFIG_SB_PUBLIC_KEY_HASH_LEN-1]);
		retval = rot_verify(fw_val_info->public_key,
					key_data,
					fw_val_info->signature,
					(u8_t *)fw_src_address,
					fwinfo->size);

		if (retval == 0) {
			for (u32_t i = 0; i < key_data_idx; i++) {
				PRINT("Invalidating key %d.\n\r", i);
				invalidate_public_key(i);
			}
		}
		if (retval != -EHASHINV) {
			break;
		}
	}

	if (retval != 0) {
		PRINT("Firmware validation failed with error %d.\n\r",
			    retval);
		return false;
	}

	PRINT("Signature verified.\n\r");

	return true;
}


bool bl_validate_firmware(u32_t fw_dst_address, u32_t fw_src_address)
{
	return validate_firmware(fw_dst_address, fw_src_address,
				fw_info_find(fw_src_address), true);
}


bool bl_validate_firmware_local(u32_t fw_address,
				const struct fw_info *fwinfo)
{
	return validate_firmware(fw_address, fw_address, fwinfo, false);
}
#endif


#ifdef CONFIG_BL_VALIDATE_FW_EXT_API_ENABLED
EXT_API(BL_VALIDATE_FW, struct bl_validate_fw_ext_api,
						bl_validate_fw_ext_api) = {
		.bl_validate_firmware = bl_validate_firmware,
	}
};
#endif
