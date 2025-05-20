/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef FLASH_SINK_H__
#define FLASH_SINK_H__

#include <suit_sink.h>
#include <suit_memptr_storage.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the flash_sink object
 *
 * @param sink Pointer to sink_stream to be filled
 * @param dst Destination address - start of write area
 * @param size Write area size
 * @return SUIT_PLAT_SUCCESS if success otherwise error code.
 */
suit_plat_err_t suit_flash_sink_get(struct stream_sink *sink, uint8_t *dst, size_t size);

/**
 * @brief Check if given address lays in nvm
 *
 * @param address Address to be checked
 * @return true If given address lays in nvm
 * @return false If given address is out of bounds of nvm
 */
bool suit_flash_sink_is_address_supported(uint8_t *address);

/**
 * @brief Read data from the flash sink. It is an additional interface, not a part
 * of the stream_sink API. User that holds the flash_sink can readback the data during
 * streaming, as flash_sink has the access to data destination in memory.
 *
 * @param[in] sink_ctx context of the flash_sink
 * @param[in] offset Offset of flash_sink area to start reading from
 * @param[out] buf Buffer to read into
 * @param[in] size size of @a buf; data read size
 *
 * @retval SUIT_PLAT_SUCCESS on success
 * @retval SUIT_PLAT_ERR_INVAL if invalid argument passed
 * @retval SUIT_PLAT_ERR_IO on flash access fail
 */
suit_plat_err_t suit_flash_sink_readback(void *sink_ctx, size_t offset, uint8_t *buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_SINK_H__ */
