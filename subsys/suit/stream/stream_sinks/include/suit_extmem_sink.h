/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef EXTMEM_SINK_H__
#define EXTMEM_SINK_H__

#include <suit_sink.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the extmem_sink object
 *
 * @param sink Pointer to sink_stream to be filled
 * @param dst Destination address - start of write area
 * @param size Write area size
 * @return SUIT_PLAT_SUCCESS if success otherwise error code.
 */
suit_plat_err_t suit_extmem_sink_get(struct stream_sink *sink, uint8_t *dst, size_t size);

/**
 * @brief Check if given address lays in external memory area.
 *
 * @param address Address to be checked
 * @return true If given address lays in external memory area.
 * @return false If given address is out of bounds of external memory area.
 */
bool suit_extmem_sink_is_address_supported(uint8_t *address);

#ifdef __cplusplus
}
#endif

#endif /* EXTMEM_SINK_H__ */
