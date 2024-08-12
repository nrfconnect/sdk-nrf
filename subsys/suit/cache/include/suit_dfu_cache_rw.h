/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DFU_CACHE_RW_H__
#define DFU_CACHE_RW_H__

#include <stddef.h>
#include <stdint.h>
#include <zcbor_decode.h>
#include <zephyr/storage/flash_map.h>
#include <suit_dfu_cache.h>
#include <suit_memory_layout.h>

#ifdef __cplusplus
extern "C" {
#endif

/* This header is meant to be used only from APP core context.
 * It extends dfu_cache by adding to cache write and erase capabilities.
 */

struct suit_cache_slot {
	uint8_t *slot_address;
	size_t size;
	size_t size_offset;
	size_t data_offset;
	size_t eb_size;
};

/**
 * @brief Function tries to allocate slot in cache pointed by ID
 *
 * @param cache_partition_id ID of the cache partition in which slot should be allocated
 * @param slot Pointer to structure that will be filled with allocated slot info
 * @param uri URI that will be used as a key in cache cbor map
 * @param uri_size URI size
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_dfu_cache_rw_slot_create(uint8_t cache_partition_id,
					      struct suit_cache_slot *slot, const uint8_t *uri,
					      size_t uri_size);

/**
 * @brief Commits changes written to slot by updating cbor header for the cache slot
 *
 * @param slot Pointer to opened cache slot
 * @param size_used Number of bytes written
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_dfu_cache_rw_slot_close(struct suit_cache_slot *slot, size_t size_used);

/**
 * @brief Drop data written to slot and revert slot allocation
 *
 * @param slot Pointer to slot that should be dropped
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_dfu_cache_rw_slot_drop(struct suit_cache_slot *slot);

/**
 * @brief Adjust cache partition 0 location and size based on characteristics of
 * update candidate envelope
 *
 * To be executed each time new envelope is stored in the dfu partition
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_dfu_cache_0_resize(void);

/**
 * @brief Validates content of cache partitions
 *
 * Validates content of cache partitions and erases nvm partition in case of content incoherency
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_dfu_cache_validate_content(void);

/**
 * @brief Drops content of all cache partitions
 *
 * Erases nvm partitions belonging to all cache partitions
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_dfu_cache_drop_content(void);

/**
 * @brief Gets information about characteristics of cache partition
 *
 * @return suit_plat_success on success, error code otherwise.
 */
suit_plat_err_t suit_dfu_cache_rw_device_info_get(uint8_t cache_partition_id,
						  struct suit_nvm_device_info *device_info);

#ifdef __cplusplus
}
#endif

#endif /* DFU_CACHE_RW_H__ */
