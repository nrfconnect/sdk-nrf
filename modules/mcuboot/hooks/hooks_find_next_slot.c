/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <../../bootutil/src/bootutil_priv.h>
#ifdef CONFIG_NRF_MCUBOOT_BOOT_REQUEST
#include <bootutil/boot_request.h>
#endif /* CONFIG_NRF_MCUBOOT_BOOT_REQUEST */

/**
 * Finds the preferred slot containing the image based on bootloader requests.
 */
int boot_find_next_slot_hook(struct boot_loader_state *state, uint8_t image,
			     enum boot_slot *active_slot)
{
#ifdef CONFIG_NRF_MCUBOOT_BOOT_REQUEST
	enum boot_slot slot = BOOT_SLOT_NONE;

	if (active_slot == NULL) {
		return BOOT_HOOK_REGULAR;
	}

	slot = boot_request_get_preferred_slot(image);
	if (slot != BOOT_SLOT_NONE) {
		if (state->slot_usage[image].slot_available[slot]) {
			*active_slot = slot;
			return 0;
		}
	}
#endif /* CONFIG_NRF_MCUBOOT_BOOT_REQUEST */

	return BOOT_HOOK_REGULAR;
}
