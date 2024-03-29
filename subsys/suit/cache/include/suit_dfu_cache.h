/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DFU_CACHE_H__
#define DFU_CACHE_H__

#include <stddef.h>
#include <stdint.h>
#include <zcbor_decode.h>
#include <suit_plat_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/* This header has no restrictions on the context in which it can be used.
 * Contains functions allowing searching and reading from cache.
 *
 *  ___________________________    ___________________________    ___________________________
 * |        dfu_partition      |  |   dfu_cache_partition_1   |  |   dfu_cache_partition_3   |
 * |  _______________________  |  |  _______________________  |  |  _______________________  |
 * | |        Envelope       | |  | |    dfu_cache_pool_1   | |  | |    dfu_cache_pool_3   | |
 * | |                       | |  | | (Indefinite CBOR map) | |  | | (Indefinite CBOR map) | |
 * | |                       | |  | | _____________________ | |  | | _____________________ | |
 * | |                       | |  | ||    DFU CACHE SLOT   || |  | ||    DFU CACHE SLOT   || |
 * | |                       | |  | || (entry in CBOR map) || |  | || (entry in CBOR map) || |
 * | |                       | |  | || ___________________ || |  | || ___________________ || |
 * | |                       | |  | |||    UNIQUE URI_0   ||| |  | |||    UNIQUE URI_2   ||| |
 * | |                       | |  | |||___________________||| |  | |||___________________||| |
 * | |                       | |  | || ___________________ || |  | || ___________________ || |
 * | |_______________________| |  | |||       DATA        ||| |  | |||       DATA        ||| |
 * |  _______________________  |  | |||___________________||| |  | |||___________________||| |
 * | |    dfu_cache_pool_0   | |  | ||_____________________|| |  | ||_____________________|| |
 * | | (Indefinite CBOR map) | |  | |                       | |  | |                       | |
 * | | _____________________ | |  | | _____________________ | |  | | _____________________ | |
 * | ||    DFU CACHE SLOT   || |  | ||    DFU CACHE SLOT   || |  | ||    DFU CACHE SLOT   || |
 * | || (entry in CBOR map) || |  | || (entry in CBOR map) || |  | || (entry in CBOR map) || |
 * | || ___________________ || |  | || ___________________ || |  | || ___________________ || |
 * | |||    UNIQUE URI_4   ||| |  | |||    UNIQUE URI_1   ||| |  | |||    UNIQUE URI_3   ||| |
 * | |||___________________||| |  | |||___________________||| |  | |||___________________||| |
 * | || ___________________ || |  | || ___________________ || |  | || ___________________ || |
 * | |||       DATA        ||| |  | |||       DATA        ||| |  | |||       DATA        ||| |
 * | |||___________________||| |  | |||___________________||| |  | |||___________________||| |
 * | ||_____________________|| |  | ||_____________________|| |  | ||_____________________|| |
 * | |_______________________| |  | |_______________________| |  | |_______________________| |
 * |___________________________|  |___________________________|  |___________________________|
 */

/* DFU cache is meant as a storage used during update process. It can be used to fetch
 * payloads from external sources. DFU cache consists of cache partitions that can be defined
 * across all available nonvolatile memories. Each cache partition can hold multiple images. DFU
 * cache slot represents area occupied by single image with its identifier (URI stored as CBOR
 * tstring). In short, DFU cache consists of partitions, partition contains cache pool (CBOR map),
 * cache pool contains slots and one slot holds one "image". As a rule, dfu_cache_pool_0 is
 * predefined and occupies free space on the DFU partition, right after the envelope. If there's no
 * envelope in DFU partition, suit_cache_0 initialization will fail and it will be unavailable.
 * suit_cache_0 size is variable. Other caches can be defined by the user via device tree and must
 * follow the naming convention: dfu_cache_partition_N where N is a ordinal number starting at 1.
 * It is allowed for the ordinals to skip numbers in the sequence as in example above.
 *
 * Slot allocation creates initial entry in cache map with tstr uri as key, followed by header
 *		byte 0x5A and 4 bytes of size. Size is left unwritten.
 * Slot closing takes size of data that was written and updates byte string size with that value.
 *		The indefinite end tag (0xFF), which determines the end of the partition, is written
 *		at the end.
 *
 * suit_cache_slot: Describes new allocated/open slot in cache
 *		- slot_offset: Absolute offset of the slot in the memory. Where the slot header
 *			       starts.
 *		- size: It follows how much data is written and that value is used to update size in
 *			cache
 *		- slot header when the slot is being closed
 *		- size_offset: offset from slot_offset, points to where size value in header should
 *			       be written
 *		- data_offset: offset from slot_offset, points to where data begins in slot
 *		- fdev: pointer to driver associated with cache partition on which slot lays
 *
 * dfu_cache_pool: Describes single cache pool
 *		- size: Maximum size of cache, SUIT cache partition size
 *		- offset: offset were cache begins
 *
 * dfu_cache: Contains list of all registered cache partitions - cache pool
 *		- pools_count: Number of registered cache partitions
 *		- pools: List containing registered partitions
 */

/* IPC contract */
struct dfu_cache_pool { /* Structure describing single cache partition */
	size_t size;
	uint8_t *address;
};

struct dfu_cache {
	uint8_t pools_count;
	struct dfu_cache_pool pools[CONFIG_SUIT_CACHE_MAX_CACHES];
};

/**
 * @brief Initializes SUIT cache in readonly mode for use in contexts other than APP
 *
 * @param cache  Pointer to the SUIT cache structure.
 *
 * @return SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_dfu_cache_initialize(struct dfu_cache *cache);

/**
 * @brief Deinitialize SUIT cache
 */
void suit_dfu_cache_deinitialize(void);

/**
 * @brief Clear SUIT cache.
 *
 * @param cache  Pointer to the SUIT cache structure.
 */
void suit_dfu_cache_clear(struct dfu_cache *cache);

/**
 * @brief Search registered caches for data with equal uri key.
 *
 * @param uri Pointer to URI key value to look for.
 * @param uri_size URI size
 * @param payload Return pointer to data object if found.
 * @param payload_size Return pointer to size of returned payload.
 * @return suit_plat_err_t SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
suit_plat_err_t suit_dfu_cache_search(const uint8_t *uri, size_t uri_size, const uint8_t **payload,
				      size_t *payload_size);

#ifdef __cplusplus
}
#endif

#endif /* DFU_CACHE_H__ */
