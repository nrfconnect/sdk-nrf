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
