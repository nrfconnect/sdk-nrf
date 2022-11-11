/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "bl_storage.h"
#include <string.h>
#include <errno.h>
#include <nrf.h>
#include <assert.h>
#include <nrfx_nvmc.h>
#include <pm_config.h>

uint32_t s0_address_read(void)
{
	uint32_t addr = BL_STORAGE->s0_address;

	__DSB(); /* Because of nRF9160 Erratum 7 */
	return addr;
}

uint32_t s1_address_read(void)
{
	uint32_t addr = BL_STORAGE->s1_address;

	__DSB(); /* Because of nRF9160 Erratum 7 */
	return addr;
}

uint32_t num_public_keys_read(void)
{
	uint32_t num_pk = BL_STORAGE->num_public_keys;

	__DSB(); /* Because of nRF9160 Erratum 7 */
	return num_pk;
}

/* Value written to the invalidation token when invalidating an entry. */
#define INVALID_VAL 0xFFFF0000

static bool key_is_valid(uint32_t key_idx)
{
	bool ret = (BL_STORAGE->key_data[key_idx].valid != INVALID_VAL);

	__DSB(); /* Because of nRF9160 Erratum 7 */
	return ret;
}

int verify_public_keys(void)
{
	for (uint32_t n = 0; n < num_public_keys_read(); n++) {
		if (key_is_valid(n)) {
			for (uint32_t i = 0; i < SB_PUBLIC_KEY_HASH_LEN / 2; i++) {
				const uint16_t *hash_as_halfwords =
					(const uint16_t *)BL_STORAGE->key_data[n].hash;
				uint16_t halfword = nrfx_nvmc_otp_halfword_read(
					(uint32_t)&hash_as_halfwords[i]);
				if (halfword == 0xFFFF) {
					return -EHASHFF;
				}
			}
		}
	}
	return 0;
}

int public_key_data_read(uint32_t key_idx, uint8_t *p_buf)
{
	const volatile uint8_t *p_key;

	if (!key_is_valid(key_idx)) {
		return -EINVAL;
	}

	if (key_idx >= num_public_keys_read()) {
		return -EFAULT;
	}

	p_key = BL_STORAGE->key_data[key_idx].hash;

	/* Ensure word alignment, since the data is stored in memory region
	 * with word sized read limitation. Perform both build time and run
	 * time asserts to catch the issue as soon as possible.
	 */
	BUILD_ASSERT(offsetof(struct bl_storage_data, key_data) % 4 == 0);
	__ASSERT(((uint32_t)p_key % 4 == 0), "Key address is not word aligned");

	otp_copy32(p_buf, (volatile uint32_t *restrict)p_key, SB_PUBLIC_KEY_HASH_LEN);

	return SB_PUBLIC_KEY_HASH_LEN;
}

void invalidate_public_key(uint32_t key_idx)
{
	const volatile uint32_t *invalidation_token =
			&BL_STORAGE->key_data[key_idx].valid;

	if (*invalidation_token != INVALID_VAL) {
		/* Write if not already written. */
		__DSB(); /* Because of nRF9160 Erratum 7 */
		nrfx_nvmc_word_write((uint32_t)invalidation_token, INVALID_VAL);
	}
}
