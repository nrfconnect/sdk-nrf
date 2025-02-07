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

/**
 * @brief Read data from the ram sink. It is an additional interface, not a part
 * of the stream_sink API. User that holds the ram_sink can readback the data during
 * streaming, as ram_sink has the access to data destination in memory.
 *
 * @param[in] sink_ctx Context of the ram_sink
 * @param[in] offset Offset of ram_sink area to start reading from
 * @param[out] buf Buffer to read into
 * @param[in] size Size of @a buf; data read size
 *
 * @retval SUIT_PLAT_SUCCESS on success
 * @retval SUIT_PLAT_ERR_INVAL if invalid argument passed
 * @retval SUIT_PLAT_ERR_OUT_OF_BOUNDS if read performed out of bounds
 */
suit_plat_err_t suit_ram_sink_readback(void *sink_ctx, size_t offset, uint8_t *buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* RAM_SINK_H__ */
