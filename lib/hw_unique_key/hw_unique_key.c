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
#include <sys/util.h>


#ifdef CONFIG_HW_UNIQUE_KEY_RANDOM
void hw_unique_key_write_random(void)
{
	uint8_t rand_bytes[HUK_SIZE_BYTES * ARRAY_SIZE(huk_slots)];
	size_t olen;
	uint8_t pers_str[] = "HW Unique Key";
	int err, err2;
	nrf_cc3xx_platform_ctr_drbg_context_t ctx = {0};

	for (int i = 0; i < ARRAY_SIZE(huk_slots); i++) {
		if (hw_unique_key_is_written(huk_slots[i])) {
			HUK_PRINT("One or more keys already set. Cannot overwrite\n\r");
			HUK_PANIC();
		}
	}

	err = nrf_cc3xx_platform_ctr_drbg_init(&ctx, pers_str, sizeof(pers_str) - 1);

	if (err != 0) {
		HUK_PRINT("The RNG setup failed with error code: %d\n\r", err);
		HUK_PANIC();
	}

	err = nrf_cc3xx_platform_ctr_drbg_get(&ctx, rand_bytes, sizeof(rand_bytes), &olen);

	err2 = nrf_cc3xx_platform_ctr_drbg_free(&ctx);

	if (err != 0 || olen != sizeof(rand_bytes)) {
		HUK_PRINT("The RNG call failed with error code: %d\n\r", err);
		HUK_PANIC();
	}

	if (err2 != 0) {
		HUK_PRINT("Could not free nrf_cc3xx_platform_ctr_drbg context: %d\n\r", err2);
	}

	for (int i = 0; i < ARRAY_SIZE(huk_slots); i++) {
		hw_unique_key_write(huk_slots[i], rand_bytes + (HUK_SIZE_BYTES * i));
	}

	memset(rand_bytes, sizeof(rand_bytes), 0);

	if (!hw_unique_key_are_any_written()) {
		HUK_PRINT("One or more keys not set correctly.\n\r");
		HUK_PANIC();
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
