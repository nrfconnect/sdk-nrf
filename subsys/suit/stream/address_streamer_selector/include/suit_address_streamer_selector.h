/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef SUIT_ADDRESS_STREAMER_SELECTOR_H__
#define SUIT_ADDRESS_STREAMER_SELECTOR_H__

#include <suit_sink.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Stream data into the sink.
 *
 * @param payload  Global memory address pointing to the payload.
 * @param payload_size  Size of the data to stream.
 * @param sink  Sink object to write the data to.
 *
 * @return 0 if success, error code otherwise.
 */
typedef suit_plat_err_t (*suit_address_streamer)(const uint8_t *payload, size_t payload_size,
						 struct stream_sink *sink);

/**
 * @brief Get sink based on address
 *
 * @param address Address to stream data from
 * @param ifaces Null-terminated list of source interfaces
 *
 * @return int 0 in case of success, otherwise error code
 */
suit_address_streamer suit_address_streamer_select_by_address(const uint8_t *address);

#ifdef __cplusplus
}
#endif

#endif /* SUIT_ADDRESS_STREAMER_SELECTOR_H__ */
