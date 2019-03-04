/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "provision.h"
#include <string.h>
#include <stdbool.h>
#include <generated_dts_board.h>
#include <errno.h>
#include "../bootloader.h"
typedef struct {
	u32_t s0_address;
	u32_t s1_address;
	u32_t num_public_keys;
	u8_t pkd[1];
} provision_flash_t;

static const provision_flash_t *p_provision_data =
	(provision_flash_t *)DT_FLASH_AREA_PROVISION_OFFSET;

u32_t s0_address_read(void)
{
	return p_provision_data->s0_address;
}

u32_t s1_address_read(void)
{
	return p_provision_data->s1_address;
}

u32_t num_public_keys_read(void)
{
	return p_provision_data->num_public_keys;
}

int public_key_data_read(u32_t key_idx, u8_t *p_buf, size_t buf_size)
{
	if (buf_size < CONFIG_SB_PUBLIC_KEY_HASH_LEN) {
		return -ENOMEM;
	}

	if (key_idx >= p_provision_data->num_public_keys) {
		return -EINVAL;
	}

	for (size_t i = 0; i < CONFIG_SB_PUBLIC_KEY_HASH_LEN; i++) {
		p_buf[i] = p_provision_data->pkd[(key_idx *
				CONFIG_SB_PUBLIC_KEY_HASH_LEN) + i];
	}

	return CONFIG_SB_PUBLIC_KEY_HASH_LEN;
}
