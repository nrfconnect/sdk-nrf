/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "debug.h"
#include <zephyr/types.h>
#include <bl_crypto.h>
#include "bl_crypto_internal.h"

int crypto_root_of_trust(const u8_t *public_key, const u8_t *public_key_hash,
			 const u8_t *signature, const u8_t *firmware,
			 const u32_t firmware_len)
{
	__ASSERT(public_key && public_key_hash && signature && firmware,
			"A parameter was NULL.");

	if (!verify_truncated_hash(public_key, CONFIG_SB_PUBLIC_KEY_LEN,
			public_key_hash, CONFIG_SB_PUBLIC_KEY_HASH_LEN)) {
		return -EPKHASHINV;
	}

	if (!verify_signature(firmware, firmware_len, signature, public_key)) {
		return -ESIGINV;
	}
	return 0;
}
