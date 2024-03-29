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
 * @brief Initialize cache in R/W mode for APP context
 *
 * @note dfu_cache structure is initialized based on dfu_partitions_ext which in turn is
 *       initialized using SUIT cache partitions information from Device Tree.
 *
 * @param addr  Address of the envelope in DFU partition
 * @param size  Size of the envelope
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_dfu_cache_rw_initialize(void *addr, size_t size);

/**
 * @brief Deinitialize SUIT cache. This will also erase all the cache partitions.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_dfu_cache_rw_deinitialize(void);

/**
 * @brief Retrieve information about the address and size of a given cache
 *        partition.
 *
 * @param cache_partition_id ID of the cache partition
 * @param address address of the cache partition
 * @param size size of the cache partition
 *
 * @return SUIT_PLAT_SUCCESS in case of success
 * @return SUIT_PLAT_ERR_NOT_FOUND if the given partition does not exist
 */
suit_plat_err_t suit_dfu_cache_rw_partition_info_get(uint8_t cache_partition_id,
						     const uint8_t **address, size_t *size);

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
					      struct suit_cache_slot *slot,
					      const uint8_t *uri, size_t uri_size);

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

#ifdef __cplusplus
}
#endif

#endif /* DFU_CACHE_RW_H__ */
