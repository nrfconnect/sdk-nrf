/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <suit_generic_address_streamer.h>
#include <suit_address_streamer_selector.h>

LOG_MODULE_REGISTER(suit_generic_address_streamer, CONFIG_SUIT_LOG_LEVEL);

suit_plat_err_t suit_generic_address_streamer_stream(const uint8_t *payload, size_t payload_size,
						     struct stream_sink *sink)
{
	if ((payload != NULL) && (sink != NULL) && (sink->write != NULL) && (payload_size > 0)) {
		suit_address_streamer streamer = suit_address_streamer_select_by_address(payload);

		if (streamer == NULL) {
			LOG_ERR("Streamer source not found for address: %p", (void *)payload);
			return SUIT_PLAT_ERR_NOT_FOUND;
		} else {
			return streamer(payload, payload_size, sink);
		}
	}

	LOG_ERR("Invalid arguments");
	return SUIT_PLAT_ERR_INVAL;
}
