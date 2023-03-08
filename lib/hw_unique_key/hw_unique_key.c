/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <nrf_cc3xx_platform_kmu.h>
#include <nrf_cc3xx_platform_ctr_drbg.h>
#include <hw_unique_key.h>
#include "hw_unique_key_internal.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>


#ifdef CONFIG_HW_UNIQUE_KEY_RANDOM
void hw_unique_key_write_random(void)
{
	uint8_t rand_bytes[HUK_SIZE_BYTES * ARRAY_SIZE(huk_slots)];
	uint8_t zeros[sizeof(rand_bytes)] = {0};
	size_t olen;
	uint8_t pers_str[] = "HW Unique Key";
	int err, err2;
	nrf_cc3xx_platform_ctr_drbg_context_t ctx = {0};

	if (hw_unique_key_are_any_written()) {
		printk("One or more keys already set. Cannot overwrite\r\n");
		k_panic();
	}

	err = nrf_cc3xx_platform_ctr_drbg_init(&ctx, pers_str, sizeof(pers_str) - 1);
	if (err != 0) {
		printk("The RNG setup failed with error code: %d\r\n", err);
		k_panic();
	}

	err = nrf_cc3xx_platform_ctr_drbg_get(&ctx, rand_bytes, sizeof(rand_bytes), &olen);
	err2 = nrf_cc3xx_platform_ctr_drbg_free(&ctx);

	if (err != 0 || olen != sizeof(rand_bytes)) {
		printk("The RNG call failed with error code: %d or wrong size %d\r\n", err, olen);
		k_panic();
	}

	if (err2 != 0) {
		printk("Could not free nrf_cc3xx_platform_ctr_drbg context: %d\r\n", err2);
		k_panic();
	}

	for (int i = 0; i < ARRAY_SIZE(huk_slots); i++) {
		hw_unique_key_write(huk_slots[i], rand_bytes + (HUK_SIZE_BYTES * i));
	}

	memset(rand_bytes, 0, sizeof(rand_bytes));

	if (memcmp(rand_bytes, zeros, sizeof(rand_bytes)) != 0) {
		printk("The key bytes weren't correctly deleted from RAM.\r\n");
		k_panic();
	}

	for (int i = 0; i < ARRAY_SIZE(huk_slots); i++) {
		if (!hw_unique_key_is_written(huk_slots[i])) {
			printk("One or more keys not set correctly\r\n");
			k_panic();
		}
	}
}
#endif /* CONFIG_HW_UNIQUE_KEY_RANDOM */

bool hw_unique_key_are_any_written(void)
{
	for (int i = 0; i < ARRAY_SIZE(huk_slots); i++) {
		if (hw_unique_key_is_written(huk_slots[i])) {
			return true;
		}
	}
	return false;
}

int hw_unique_key_derive_key(enum hw_unique_key_slot kmu_slot,
	const uint8_t *context, size_t context_size,
	uint8_t const *label, size_t label_size,
	uint8_t *output, uint32_t output_size)
{
	if (!hw_unique_key_is_written(kmu_slot)) {
		return -ERR_HUK_MISSING;
	}

	return nrf_cc3xx_platform_kmu_shadow_key_derive(
		kmu_slot, HUK_SIZE_BYTES * 8,
		label, label_size, context, context_size, output, output_size);
}
