/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DFU_CACHE_INTERNAL_H__
#define DFU_CACHE_INTERNAL_H__

#include <stddef.h>
#include <stdint.h>
#include <zcbor_decode.h>
#include <zephyr/storage/flash_map.h>
#include <suit_dfu_cache.h>
#include <zephyr/autoconf.h>

/* Adding 5 bytes for bstring header and 1 byte for indefinite map header and
 * 9 bytes for tstr
 */
#define CACHE_METADATA_MAX_LENGTH (CONFIG_SUIT_MAX_URI_LENGTH + 5 + 1 + 9)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback to be passed to the @ref suit_dfu_cache_partition_slot_foreach function
 *
 * @param cache_pool  Pointer to the SUIT cache pool structure.
 * @param state  zcbor state of the current slot.
 * @param uri  URI of the current slot
 * @param payload_offset  Offset of the payload. May be located in external storage area.
 * @param payload_size  Size of the payload.
 * @param ctx  Additional callback context.
 *
 * @return True continues iteration, false causes the caller to stop subsequent iterations.
 */
typedef bool (*partition_slot_foreach_cb)(struct dfu_cache_pool *cache_pool, zcbor_state_t *state,
					  const struct zcbor_string *uri, uintptr_t payload_offset,
					  size_t payload_size, void *ctx);

/**
 * @brief Iterates over cache slots and executes a provided callback.
 *
 * The foreach operation reads the URI of each slot in the cache partition, acquires the
 * payload address and size and executes a provided callback. The callback may be NULL.
 * In such case, the function can be used to validate that the cache partition is not
 * corrupted. If the callback returns false, further iteration is stopped and the
 * function returns success.
 *
 * @param cache_pool  Pointer to the SUIT cache structure.
 * @param cb  Callback pointer. May be NULL.
 * @param ctx  Additional callback context.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_dfu_cache_partition_slot_foreach(struct dfu_cache_pool *cache_pool,
						      partition_slot_foreach_cb cb, void *ctx);

/**
 * @brief Check if cache partition is initialized.
 *
 * @param cache_pool  Pointer to the SUIT cache pool structure.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_dfu_cache_partition_is_initialized(struct dfu_cache_pool *cache_pool);

/**
 * @brief Check if cache partition content is erased.
 *
 * @param cache_pool  Pointer to the SUIT cache structure.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, SUIT_PLAT_ERR_NOMEM if partition is not empty,
 *         otherwise an error code
 */
suit_plat_err_t suit_dfu_cache_partition_is_empty(struct dfu_cache_pool *cache_pool);

/**
 * @brief Find a free slot in the cache partition.
 *
 * @param cache_pool  Pointer to the SUIT cache pool structure.
 * @param address  Return value pointing to free space within the cache partition.
 * @param needs_erase  Indicates that the memory must be erased before it's written to.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_dfu_cache_partition_find_free_space(struct dfu_cache_pool *cache_pool,
							 uintptr_t *address,
							 bool *needs_erase);

/**
 * @brief Memcpy-like helper for writing streamable data into a memory buffer.
 *
 * @param destination  Destination address.
 * @param source  Source address that can map into MCU RAM/ROM memory or into external memory
 * region.
 * @param size  Read size.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_dfu_cache_memcpy(uint8_t *destination, uintptr_t source, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* DFU_CACHE_INTERNAL_H__ */
