/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef RAM_SINK_H__
#define RAM_SINK_H__

#include <suit_sink.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the ram_sink object
 *
 * @param sink Pointer to sink_stream to be filled
 * @param dst Destination address - start of write area
 * @param size Write area size
 * @return SUIT_PLAT_SUCCESS if success otherwise error code.
 */
suit_plat_err_t suit_ram_sink_get(struct stream_sink *sink, uint8_t *dst, size_t size);

/**
 * @brief Check if given address lays in assigned RAM
 *
 * @param address Address to be checked
 * @return true If given address lays in RAM
 * @return false If given address is out of bounds of assigned RAM
 */
bool suit_ram_sink_is_address_supported(uint8_t *address);

#ifdef __cplusplus
}
#endif

#endif /* RAM_SINK_H__ */
