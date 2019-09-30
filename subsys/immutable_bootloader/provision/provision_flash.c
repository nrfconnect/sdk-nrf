/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "provision.h"
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include "bootloader.h"
#include <nrf.h>
#include <assert.h>
#include <pm_config.h>

typedef struct {
	u32_t s0_address;
	u32_t s1_address;
	u32_t num_public_keys;
	u32_t pkd[1];
} provision_flash_t;

static const provision_flash_t *p_provision_data =
	(provision_flash_t *)PM_PROVISION_ADDRESS;

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

int public_key_data_read(u32_t key_idx, u32_t *p_buf, size_t buf_size)
{
	const u32_t *p_key;

	if (buf_size < CONFIG_SB_PUBLIC_KEY_HASH_LEN) {
		return -ENOMEM;
	}

	if (key_idx >= p_provision_data->num_public_keys) {
		return -EINVAL;
	}

	p_key = &(p_provision_data->pkd[key_idx *
			CONFIG_SB_PUBLIC_KEY_HASH_LEN]);

#ifdef CONFIG_SOC_NRF9160
	/* Ensure word alignment, as provision data is stored in memory region
	 * with word sized read limitation. Perform both build time and run
	 * time asserts to catch the issue as soon as possible.
	 */
	BUILD_ASSERT(CONFIG_SB_PUBLIC_KEY_HASH_LEN % 4 == 0);
	BUILD_ASSERT(offsetof(provision_flash_t, pkd) % 4 == 0);
	__ASSERT(((u32_t)p_key % 4 == 0), "Key address is not word aligned");
#endif /* CONFIG_SOC_NRF9160 */

	for (size_t i = 0; i < CONFIG_SB_PUBLIC_KEY_HASH_LEN/4; i++) {
		p_buf[i] = p_key[i];
	}

	return CONFIG_SB_PUBLIC_KEY_HASH_LEN;
}
