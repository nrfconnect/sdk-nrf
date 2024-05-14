/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <suit_address_streamer_selector.h>
#include <suit_plat_err.h>

#if IS_ENABLED(CONFIG_SUIT_STREAM_SOURCE_EXTMEM)
#include <suit_extmem_streamer.h>
#endif

#if IS_ENABLED(CONFIG_SUIT_STREAM_SOURCE_FLASH)
#include <suit_flash_streamer.h>
#endif

#if IS_ENABLED(CONFIG_SUIT_STREAM_SOURCE_MEMPTR)
#include <suit_memptr_streamer.h>
#endif

LOG_MODULE_REGISTER(suit_plat_source_selector, CONFIG_SUIT_LOG_LEVEL);

/**
 * @brief Check if stream source handles given address.
 *
 * @param address Global memory address pointing to the payload.
 *
 * @return True if the source can stream data from the address, false otherwise.
 */
typedef bool (*suit_address_check)(const uint8_t *address);

/* Stream interface object. */
struct stream_iface {
	suit_address_streamer stream;
	suit_address_check address_check;
};

const struct stream_iface ifaces[] = {
#if IS_ENABLED(CONFIG_SUIT_STREAM_SOURCE_FLASH)
	{
		.stream = suit_flash_streamer_stream,
		.address_check = suit_flash_streamer_address_in_range
	},
#endif
#if IS_ENABLED(CONFIG_SUIT_STREAM_SOURCE_MEMPTR)
	{
		.stream = suit_memptr_streamer_stream,
		.address_check = suit_memptr_streamer_address_in_range
	},
#endif
#if IS_ENABLED(CONFIG_SUIT_STREAM_SOURCE_EXTMEM)
	{
		.stream = suit_extmem_streamer_stream,
		.address_check = suit_extmem_streamer_address_in_range
	},
#endif
};

suit_address_streamer suit_address_streamer_select_by_address(const uint8_t *address)
{
	for (int i = 0; i < ARRAY_SIZE(ifaces); i++) {
		if (ifaces[i].address_check((uint8_t *)address)) {
			return ifaces[i].stream;
		}
	}
	LOG_ERR("No streamer found for address %p", (void *) address);

	return NULL;
}
