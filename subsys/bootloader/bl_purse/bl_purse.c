/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "bl_purse.h"
#include <string.h>
#include <errno.h>
#include <nrf.h>
#include <assert.h>
#include <pm_config.h>
#include <nrfx_nvmc.h>


struct provision_flash {
	u32_t s0_address;
	u32_t s1_address;
	u32_t num_public_keys;
	struct {
		u32_t valid;
		u8_t hash[CONFIG_SB_PUBLIC_KEY_HASH_LEN];
	} key_data[1];
};


#if defined(CONFIG_SOC_NRF9160) || defined(CONFIG_SOC_NRF5340_CPUAPP)
static const struct provision_flash *p_provision_data =
	(struct provision_flash *)&(NRF_UICR_S->OTP[0]);

#elif defined(PM_PROVISION_ADDRESS)
static const struct provision_flash *p_provision_data =
	(struct provision_flash *)PM_PROVISION_ADDRESS;

#else
#error "Provision data location unknown."

#endif


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


/* Value written to the invalidation token when invalidating an entry. */
#define INVALID_VAL 0xFFFF0000

int public_key_data_read(u32_t key_idx, u8_t *p_buf, size_t buf_size)
{
	const u8_t *p_key;

	if (p_provision_data->key_data[key_idx].valid == INVALID_VAL) {
		return -EINVAL;
	}

	if (buf_size < CONFIG_SB_PUBLIC_KEY_HASH_LEN) {
		return -ENOMEM;
	}

	if (key_idx >= num_public_keys_read()) {
		return -EFAULT;
	}

	p_key = p_provision_data->key_data[key_idx].hash;

	/* Ensure word alignment, as provision data is stored in memory region
	 * with word sized read limitation. Perform both build time and run
	 * time asserts to catch the issue as soon as possible.
	 */
	BUILD_ASSERT(CONFIG_SB_PUBLIC_KEY_HASH_LEN % 4 == 0);
	BUILD_ASSERT(offsetof(struct provision_flash, key_data) % 4 == 0);
	__ASSERT(((u32_t)p_key % 4 == 0), "Key address is not word aligned");

	memcpy(p_buf, p_key, CONFIG_SB_PUBLIC_KEY_HASH_LEN);

	return CONFIG_SB_PUBLIC_KEY_HASH_LEN;
}

void invalidate_public_key(u32_t key_idx)
{
	const u32_t *invalidation_token =
			&p_provision_data->key_data[key_idx].valid;

	if (*invalidation_token != INVALID_VAL) {
		/* Write if not already written. */
		nrfx_nvmc_word_write((u32_t)invalidation_token, INVALID_VAL);
	}
}
