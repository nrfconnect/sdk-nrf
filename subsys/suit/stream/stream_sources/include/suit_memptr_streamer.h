/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MEMPTR_STREAMER_H__
#define MEMPTR_STREAMER_H__

#include <suit_sink.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check if address can be streamed from readable memory.
 *
 * @param address Pointer to the payload
 *
 * @return True if address can be read from memory, false otherwise.
 */
bool suit_memptr_streamer_address_in_range(const uint8_t *address);

/**
 * @brief Stream payload from pointer to sink
 *
 * @param payload Pointer to payload - source
 * @param payload_size Size of payload
 * @param sink Pointer to sink that will write payload - target
 * @return SUIT_PLAT_SUCCESS if success otherwise error code
 */
suit_plat_err_t suit_memptr_streamer_stream(const uint8_t *payload, size_t payload_size,
					    struct stream_sink *sink);

#ifdef __cplusplus
}
#endif

#endif /* MEMPTR_STREAMER_H__ */
