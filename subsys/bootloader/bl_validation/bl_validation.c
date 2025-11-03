/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bl_validation.h>
#include <zephyr/types.h>
#include <fw_info.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <bl_storage.h>

LOG_MODULE_REGISTER(bl_validation, CONFIG_SECURE_BOOT_VALIDATION_LOG_LEVEL);

#ifdef CONFIG_SB_MONOTONIC_COUNTER_ROLLBACK_PROTECTION
/* The 15 bit version is encoded into the most significant bits of
 * the 16 bit monotonic_counter, and the 1 bit slot is encoded
 * into the least significant bit.
 */
int set_monotonic_version(counter_t version, uint16_t slot)
{
	__ASSERT(version <= 0x7FFF, "version too large.\r\n");
	__ASSERT(slot <= 1, "Slot must be either 0 or 1.\r\n");
	LOG_INF("Setting monotonic counter (version: %d, slot: %d)",
		version, slot);

	uint16_t num_cnt_slots;
	int err;

	err = num_monotonic_counter_slots(BL_MONOTONIC_COUNTERS_DESC_NSIB, &num_cnt_slots);
	if (err != 0) {
		LOG_ERR("Failed during reading of the number of the counter slots, error: %d",
		       err);
		return err;
	}

	if (num_cnt_slots == 0) {
		LOG_ERR("Monotonic version counter is disabled.");
		return -EINVAL;
	}

	err = set_monotonic_counter(BL_MONOTONIC_COUNTERS_DESC_NSIB, (version << 1) | !slot);
	if (err != 0) {
		LOG_ERR("Failed during setting the monotonic counter, error: %d", err);
	}

	return err;
}

int get_monotonic_version(counter_t *version_out)
{
	counter_t monotonic_version_and_slot;
	int err;

	if (version_out == NULL) {
		return -EINVAL;
	}

	err = get_monotonic_counter(BL_MONOTONIC_COUNTERS_DESC_NSIB, &monotonic_version_and_slot);
	if (err) {
		LOG_ERR("Error getting monotonic counter");
		return err;
	}

	*version_out = monotonic_version_and_slot >> 1;

	return err;
}

int get_monotonic_slot(counter_t *slot_out)
{
	counter_t monotonic_version_and_slot;
	int err;

	if (slot_out == NULL) {
		return -EINVAL;
	}

	err = get_monotonic_counter(BL_MONOTONIC_COUNTERS_DESC_NSIB, &monotonic_version_and_slot);
	if (err) {
		return err;
	}

	uint16_t inverted_slot = monotonic_version_and_slot & 1U;

	/* We store the slot inverted to allow updating the counter from
	 * slot 1 to the favoured slot 0.
	 */
	*slot_out = inverted_slot == 0U ? 1U : 0U;

	return 0;
}
#endif /* CONFIG_SB_MONOTONIC_COUNTER_ROLLBACK_PROTECTION */

#ifndef CONFIG_BL_VALIDATE_FW_EXT_API_UNUSED
#ifdef CONFIG_BL_VALIDATE_FW_EXT_API_REQUIRED
	#define BL_VALIDATE_FW_EXT_API_REQUIRED 1
#else
	#define BL_VALIDATE_FW_EXT_API_REQUIRED 0
#endif

EXT_API_REQ(BL_VALIDATE_FW, BL_VALIDATE_FW_EXT_API_REQUIRED,
		struct bl_validate_fw_ext_api, bl_validate_fw);

bool bl_validate_firmware(uint32_t fw_dst_address, uint32_t fw_src_address)
{
#ifdef CONFIG_BL_VALIDATE_FW_EXT_API_OPTIONAL
	if (!bl_validate_firmware_available()) {
		return false;
	}
#endif
	return bl_validate_fw->ext_api.bl_validate_firmware(fw_dst_address,
							fw_src_address);
}


#else
#include <errno.h>
#include <zephyr/toolchain.h>
#include <bl_crypto.h>
#include "bl_validation_internal.h"

#if USE_PARTITION_MANAGER
#include <pm_config.h>
#endif

struct __packed fw_validation_info {
	/* Magic value to verify that the struct has the correct type. */
	uint32_t magic[MAGIC_LEN_WORDS];

	/* The address of the start (vector table) of the firmware. */
	uint32_t address;

#if defined(CONFIG_SB_VALIDATION_STRUCT_HAS_HASH)
	/* The hash of the firmware.*/
	uint8_t  hash[CONFIG_SB_HASH_LEN];
#endif

#if defined(CONFIG_SB_VALIDATION_STRUCT_HAS_PUBLIC_KEY)
	/* Public key to be used for signature verification. This must be
	 * checked against a trusted hash.
	 */
	uint8_t  public_key[CONFIG_SB_PUBLIC_KEY_LEN];
#endif

	/* Signature over the firmware as represented by the address and size in
	 * the firmware_info.
	 */
	uint8_t  signature[CONFIG_SB_SIGNATURE_LEN];
};


/* Static asserts to ensure compatibility */
OFFSET_CHECK(struct fw_validation_info, magic, 0);
OFFSET_CHECK(struct fw_validation_info, address, 12);

#if defined(CONFIG_SB_VALIDATION_STRUCT_HAS_HASH)
OFFSET_CHECK(struct fw_validation_info, hash, 16);
#if defined(CONFIG_SB_VALIDATION_STRUCT_HAS_PUBLIC_KEY)
OFFSET_CHECK(struct fw_validation_info, public_key, (16 + CONFIG_SB_HASH_LEN));
OFFSET_CHECK(struct fw_validation_info, signature, (16 + CONFIG_SB_HASH_LEN
	+ CONFIG_SB_PUBLIC_KEY_LEN));
#else
OFFSET_CHECK(struct fw_validation_info, signature, (16 + CONFIG_SB_HASH_LEN));
#endif
#else
#if defined(CONFIG_SB_VALIDATION_STRUCT_HAS_PUBLIC_KEY)
OFFSET_CHECK(struct fw_validation_info, public_key, 16);
OFFSET_CHECK(struct fw_validation_info, signature, (16 + CONFIG_SB_PUBLIC_KEY_LEN));
#else
OFFSET_CHECK(struct fw_validation_info, signature, 16);
#endif
#endif

/* Can be used to make the firmware discoverable in other locations, e.g. when
 * searching backwards. This struct would typically be constructed locally, so
 * it needs no version.
 */
struct __packed fw_validation_pointer {
	uint32_t magic[MAGIC_LEN_WORDS];
	const struct fw_validation_info *validation_info;
};

/* Static asserts to ensure compatibility */
OFFSET_CHECK(struct fw_validation_pointer, magic, 0);
OFFSET_CHECK(struct fw_validation_pointer, validation_info, 12);

static bool validation_info_check(const struct fw_validation_info *vinfo)
{
	const uint32_t validation_info_magic[] = {VALIDATION_INFO_MAGIC};

	if (memcmp(vinfo->magic, validation_info_magic,
					CONFIG_FW_INFO_MAGIC_LEN) == 0) {
		return vinfo;
	}
	return NULL;
}


/* Find the validation_info at the end of the firmware. */
static const struct fw_validation_info *
validation_info_find(uint32_t start_address, uint32_t search_distance)
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

#if defined(CONFIG_SB_VALIDATE_FW_SIGNATURE)
static bool validate_signature(const uint32_t fw_src_address, const uint32_t fw_size,
			       const struct fw_validation_info *fw_val_info,
			       bool external)
{
	int init_retval = bl_crypto_init();

	if (init_retval) {
		if (!external) {
			LOG_ERR("bl_crypto_init() returned %d.", init_retval);
		}
		return false;
	}

	init_retval = verify_public_keys();
	if (init_retval) {
		if (!external) {
			LOG_ERR("verify_public_keys() returned %d.", init_retval);
			if (init_retval == -EHASHFF) {
				LOG_INF("A public key contains 0xFFFF, which is unsupported");
			}
		}
		return false;
	}

	bl_root_of_trust_verify_t rot_verify = external ?
					bl_root_of_trust_verify_external :
					bl_root_of_trust_verify;

#if defined(CONFIG_SB_VALIDATION_STRUCT_HAS_PUBLIC_KEY)
	/* Some key data storage backends require word sized reads, hence
	 * we need to ensure word alignment for 'key_data'
	 */
	__aligned(4) uint8_t key_data[SB_PUBLIC_KEY_HASH_LEN];

	for (uint32_t key_data_idx = 0; key_data_idx < num_public_keys_read();
			key_data_idx++) {
		int read_retval = public_key_data_read(key_data_idx, key_data);

		if (read_retval != SB_PUBLIC_KEY_HASH_LEN) {
			if (read_retval == -EINVAL) {
				if (!external) {
					LOG_INF("Key %d has been invalidated, try next.",
						key_data_idx);
				}
				continue;
			} else {
				if (!external) {
					LOG_ERR("public_key_data_read failed: %d.",
						read_retval);
				}
				return false;
			}
		}

		if (!external) {
			LOG_INF("Verifying signature against key %d.", key_data_idx);
			LOG_INF("Hash: 0x%02x...%02x", key_data[0],
				key_data[SB_PUBLIC_KEY_HASH_LEN-1]);
		}
		int retval = rot_verify(fw_val_info->public_key,
					key_data,
					fw_val_info->signature,
					(const uint8_t *)fw_src_address,
					fw_size);

		if (retval == 0) {
			for (uint32_t i = 0; i < key_data_idx; i++) {
				if (!external) {
					LOG_INF("Invalidating key %d.", i);
				}
				invalidate_public_key(i);
			}
			if (!external) {
				LOG_INF("Firmware signature verified.");
			}
			return true;
		} else if (retval == -EHASHINV) {
			if (!external) {
				LOG_WRN("Public key didn't match, try next.");
			}
			continue;
		} else {
			if (!external) {
				LOG_ERR("Firmware validation failed with error %d.",
					retval);
			}
			return false;
		}
	}
#else
	int retval = rot_verify(NULL, NULL, fw_val_info->signature,
				(const uint8_t *)fw_src_address,
				fw_size);

	if (retval == 0) {
		LOG_INF("Firmware signature verified.");
		return true;
	}

	LOG_ERR("Firmware validation failed with error %d.", retval);
#endif

	if (!external) {
		LOG_ERR("Failed to validate signature.");
	}
	return false;
}


#elif defined(CONFIG_SB_VALIDATE_FW_HASH)
static bool validate_hash(const uint32_t fw_src_address, const uint32_t fw_size,
			  const struct fw_validation_info *fw_val_info,
			  bool external)
{
	int retval = bl_crypto_init();

	if (retval) {
		if (!external) {
			LOG_ERR("bl_crypto_init() returned %d.", retval);
		}
		return false;
	}

	retval = bl_sha256_verify((const uint8_t *)fw_src_address, fw_size,
			fw_val_info->hash);

	if (retval != 0) {
		if (!external) {
			LOG_ERR("Firmware validation failed with error %d.",
				retval);
		}
		return false;
	}

	if (!external) {
		LOG_INF("Firmware hash verified.");
	}

	return true;
}
#endif


static bool validate_firmware(uint32_t fw_dst_address, uint32_t fw_src_address,
			      const struct fw_info *fwinfo, bool external)
{
	const struct fw_validation_info *fw_val_info;
	const uint32_t fwinfo_address = (uint32_t)fwinfo;
	const uint32_t fwinfo_end = (fwinfo_address + fwinfo->total_size);
	const uint32_t fw_dst_end = (fw_dst_address + fwinfo->size);
	const uint32_t fw_src_end = (fw_src_address + fwinfo->size);

	if (!fwinfo) {
		if (!external) {
			LOG_ERR("NULL parameter.");
		}
		return false;
	}

	if (!fw_info_check((uint32_t)fwinfo)) {
		if (!external) {
			LOG_ERR("Invalid firmware info format.");
		}
		return false;
	}

	if (fw_dst_address != fwinfo->address) {
		if (!external) {
			LOG_ERR("The firmware doesn't belong at destination addr.");
		}
		return false;
	}

	if (!external && (fw_src_address != fw_dst_address)) {
		if (!external) {
			LOG_ERR("src and dst must be equal for local calls.");
		}
		return false;
	}

	if (fw_info_find(fw_src_address) != fwinfo) {
		if (!external) {
			LOG_ERR("Firmware info doesn't point to itself.");
		}
		return false;
	}

	if (fwinfo->valid != CONFIG_FW_INFO_VALID_VAL) {
		if (!external) {
			LOG_ERR("Firmware has been invalidated: 0x%x.",
				fwinfo->valid);
		}
		return false;
	}

	if (!external) {
		LOG_INF("Trying to get Firmware version");
	}

#ifdef CONFIG_SB_MONOTONIC_COUNTER_ROLLBACK_PROTECTION
#if defined(CONFIG_NRFX_NVMC)
	uint16_t stored_version;
#elif defined(CONFIG_NRFX_RRAMC)
	uint32_t stored_version;
#endif

	int err = get_monotonic_version(&stored_version);

	if (err) {
		if (!external) {
			LOG_ERR("Cannot read the firmware version. %d", err);
		}
		return false;
	}

	if (fwinfo->version < stored_version) {
		if (!external) {
			LOG_ERR("Firmware version (%u) is smaller than monotonic counter (%u).",
				fwinfo->version, stored_version);
		}
		return false;
	}
#endif /* CONFIG_SB_MONOTONIC_COUNTER_ROLLBACK_PROTECTION */

#if defined(PM_S0_SIZE) && defined(PM_S1_SIZE)
	BUILD_ASSERT(PM_S0_SIZE == PM_S1_SIZE,
		"B0's slots aren't the same size. Check pm.yml.");
	if ((fwinfo->size > (PM_S0_SIZE))
		|| (fwinfo->total_size > fwinfo->size)) {
		if (!external) {
			LOG_ERR("Invalid size or total_size in firmware info.");
		}
		return false;
	}
#endif

	if (!region_within(fwinfo_address, fwinfo_end,
			fw_src_address, fw_src_end)) {
		if (!external) {
			LOG_ERR("Firmware info is not within signed region.");
		}
		return false;
	}

	if (!within(fwinfo->boot_address, fw_dst_address, fw_dst_end)) {
		if (!external) {
			LOG_ERR("Boot address is not within signed region.");
		}
		return false;
	}

	/* Wait until this point to set these values as we must know that we
	 * have a valid fw_info before proceeding.
	 */
	const uint32_t stack_ptr_offset = (fwinfo->boot_address - fw_dst_address);
	const uint32_t reset_vector = ((const uint32_t *)(fw_src_address + stack_ptr_offset))[1];

	if (!within(reset_vector, fw_dst_address, fw_dst_end)) {
		if (!external) {
			LOG_ERR("Reset handler is not within signed region.");
		}
		return false;
	}

	fw_val_info = validation_info_find(fw_src_address + fwinfo->size, 4);

	if (!fw_val_info) {
		if (!external) {
			LOG_ERR("Could not find valid firmware validation info.");
		}
		return false;
	}

	if (fw_val_info->address != fwinfo->address) {
		if (!external) {
			LOG_ERR("Validation info doesn't belong to this firmware.");
		}
		return false;
	}

#if defined(CONFIG_SB_VALIDATE_FW_SIGNATURE)
	return validate_signature(fw_src_address, fwinfo->size, fw_val_info,
				external);
#elif defined(CONFIG_SB_VALIDATE_FW_HASH)
	return validate_hash(fw_src_address, fwinfo->size, fw_val_info,
				external);
#else
	#error "Validation not specified."
#endif
}


bool bl_validate_firmware(uint32_t fw_dst_address, uint32_t fw_src_address)
{
	return validate_firmware(fw_dst_address, fw_src_address,
				fw_info_find(fw_src_address), true);
}


bool bl_validate_firmware_local(uint32_t fw_address, const struct fw_info *fwinfo)
{
	return validate_firmware(fw_address, fw_address, fwinfo, false);
}

void bl_validate_housekeeping(void)
{
	bl_root_of_trust_housekeeping();
}
#endif

bool bl_validate_firmware_available(void)
{
#ifdef CONFIG_BL_VALIDATE_FW_EXT_API_OPTIONAL
	return (bl_validate_fw != NULL);
#else
	return true;
#endif
}


#ifdef CONFIG_BL_VALIDATE_FW_EXT_API_ENABLED
EXT_API(BL_VALIDATE_FW, struct bl_validate_fw_ext_api,
						bl_validate_fw_ext_api) = {
		.bl_validate_firmware = bl_validate_firmware,
	}
};
#endif
