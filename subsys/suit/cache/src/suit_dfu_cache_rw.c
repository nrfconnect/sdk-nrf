/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <suit_dfu_cache_rw.h>
#include <zephyr/drivers/flash.h>
#include <suit_plat_mem_util.h>
#include <zephyr/logging/log.h>
#include "zcbor_encode.h"
#include <zephyr/sys/byteorder.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/util_macro.h>
#include <suit_flash_sink.h>

#include "suit_dfu_cache_internal.h"
#include <suit_memory_layout.h>

LOG_MODULE_REGISTER(suit_cache_rw, CONFIG_SUIT_LOG_LEVEL);

#ifndef BSWAP_32
#define BSWAP_32 __bswap_32
#endif

#ifndef BSWAP_16
#define BSWAP_16 __bswap_16
#endif

#define SUCCESS		      0
#define INDEFINITE_MAP_HEADER 0xBF

/* BF
 * tstr_header - max 9 bytes 0x7b UTF-8 string (eight-byte uint64_t for n, and then n bytes follow)
 * CONFIG_SUIT_MAX_URI_LENGTH
 * 5A 0 0 0 0
 * FF
 */
#define MIN_VALID_PARTITION_SIZE (CONFIG_SUIT_MAX_URI_LENGTH + 16)

#define ENCODING_OUTPUT_BUFFER_LENGTH CACHE_METADATA_MAX_LENGTH

/* Adding 9 bytes for max length tstr header*/
#define MAX_URI_ENCODE_BUFFER_LENGTH (CONFIG_SUIT_MAX_URI_LENGTH + 9)

extern struct dfu_cache dfu_cache;
static bool prevent_second_init;

struct dfu_cache_partition_ext { /* Extended structure describing single cache partition */
	const struct device *fdev;
	size_t size;
	uint8_t *address;
	size_t eb_size;
	uint8_t id;
};

#define PARTITION_INIT(index, label)                                                               \
	{                                                                                          \
		.fdev = FIXED_PARTITION_DEVICE(label),                                             \
		.address = (uint8_t *)FIXED_PARTITION_OFFSET(label),                               \
		.size = FIXED_PARTITION_SIZE(label),                                               \
		.eb_size = CONFIG_SUIT_CACHEX_ERASE_BLOCK_SIZE,                                    \
		.id = index,                                                                       \
	},

#define INDEX_IN_RAGE(index) IN_RANGE(index, 1, (CONFIG_SUIT_CACHE_MAX_CACHES - 1))
#define PARTITION_IS_USABLE(label)                                                                 \
	UTIL_AND(FIXED_PARTITION_EXISTS(label),                                                    \
		 DT_NODE_HAS_STATUS(DT_MTD_FROM_FIXED_PARTITION(DT_NODELABEL(label)), okay))

#define PARTITION_INIT_IF_INDEX_OK(label, index)                                                   \
	IF_ENABLED(UTIL_BOOL(INDEX_IN_RANGE(index)), (PARTITION_INIT(index, label)))

#define PARTITION_DEFINE_(index, label)                                                            \
	IF_ENABLED(PARTITION_IS_USABLE(label), (PARTITION_INIT_IF_INDEX_OK(label, index)))

#define PARTITION_DEFINE(index, prefix) PARTITION_DEFINE_(index, prefix##index)

static struct dfu_cache_partition_ext dfu_partitions_ext[] = {
	{
		.fdev = FIXED_PARTITION_DEVICE(dfu_partition),
		.address = (uint8_t *)FIXED_PARTITION_OFFSET(dfu_partition),
		.size = FIXED_PARTITION_SIZE(dfu_partition),
		.eb_size = FLASH_AREA_ERASE_BLOCK_SIZE(dfu_partition),
		.id = 0,
	},
	LISTIFY(CONFIG_SUIT_CACHE_MAX_CACHES, PARTITION_DEFINE, (), dfu_cache_partition_)};

static suit_plat_err_t partition_initialize(struct dfu_cache_partition_ext *part, bool force_erase);
static suit_plat_err_t cache_0_update(uint8_t *address, size_t size);
static struct dfu_cache_partition_ext *cache_partition_get(uint8_t partition_id);
static suit_plat_err_t slot_in_cache_partition_allocate(const struct zcbor_string *uri,
							struct suit_cache_slot *slot,
							struct dfu_cache_partition_ext *part);
static suit_plat_err_t cache_free_space_check(struct dfu_cache_partition_ext *part,
					      struct suit_cache_slot *slot);
static suit_plat_err_t erase_on_sink(uint8_t *address, size_t size);

/**
 * @brief Get cache partition of specified id
 *
 * @param partition_id Integer from partition label from dts.
 *		For example if partition label is dfu_cache_partition_3 than 3 is partition id.
 * @return struct dfu_cache_partition_ext* In case of success pointer to partition or
 *		NULL if requested partition was not found
 */
static struct dfu_cache_partition_ext *cache_partition_get(uint8_t partition_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(dfu_partitions_ext); i++) {
		if (dfu_partitions_ext[i].id == partition_id) {
			return &dfu_partitions_ext[i];
		}
	}

	return NULL;
}

/**
 * @brief Get the cache partition containing offset
 *
 * @param offset Offset in desired partition
 * @return struct dfu_cache_partition_ext* or NULL in case of error
 */
static struct dfu_cache_partition_ext *cache_partition_get_by_address(uint8_t *address)
{
	for (size_t i = 0; i < ARRAY_SIZE(dfu_partitions_ext); i++) {
		if ((address >= dfu_partitions_ext[i].address) &&
		    (address < (dfu_partitions_ext[i].address + dfu_partitions_ext[i].size))) {
			return &dfu_partitions_ext[i];
		}
	}

	return NULL;
}

static bool translate_partition_start_address(struct dfu_cache_partition_ext *part)
{
	struct nvm_address addr = {
		.fdev = part->fdev,
		.offset = (uintptr_t)part->address,
	};

	uintptr_t ret;

	if (!suit_memory_nvm_address_to_global_address(&addr, &ret)) {
		LOG_ERR("Address not translated: %p(dev):%lX", (void *)addr.fdev, addr.offset);
		return false;
	}

	LOG_DBG("Address translated: %p(dev):%lX -> %p", (void *)addr.fdev, addr.offset,
		(void *)ret);
	part->address = (uint8_t *)ret;

	return true;
}

static bool setup_partition_addresses(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(dfu_partitions_ext); i++) {
		if (!translate_partition_start_address(&dfu_partitions_ext[i])) {
			return false;
		}
	}

	return true;
}

suit_plat_err_t suit_dfu_cache_rw_initialize(void *addr, size_t size)
{
	if (prevent_second_init) {
		return SUIT_PLAT_ERR_INCORRECT_STATE;
	}
	prevent_second_init = true;

	suit_plat_err_t ret = cache_0_update(addr, size);

	if (ret != SUIT_PLAT_SUCCESS) {
		return ret;
	}

	dfu_cache.pools_count = ARRAY_SIZE(dfu_partitions_ext);

	for (size_t i = 1; i < ARRAY_SIZE(dfu_partitions_ext); i++) {
		/* Check if partition already has valid cache and if not initialize */
		bool force_erase = IS_ENABLED(CONFIG_SUIT_CACHEX_ERASE_ON_INIT);

		ret = partition_initialize(&dfu_partitions_ext[i], force_erase);

		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_WRN("suit_cache_%u initialization failed: %i", dfu_partitions_ext[i].id,
				ret);
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(dfu_partitions_ext); i++) {
		dfu_cache.pools[i].size = dfu_partitions_ext[i].size;
		dfu_cache.pools[i].address = dfu_partitions_ext[i].address;
		LOG_INF("Found partition %d: id: %u, (addr: %p, size: 0x%x)", i,
			dfu_partitions_ext[i].id, (void *)dfu_cache.pools[i].address,
			dfu_cache.pools[i].size);
	}

	return suit_dfu_cache_initialize(&dfu_cache);
}

suit_plat_err_t suit_dfu_cache_rw_deinitialize(void)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;

	suit_dfu_cache_deinitialize();
	suit_dfu_cache_clear(&dfu_cache);

	/* Erase all cache partitions */
	for (size_t i = 0; i < ARRAY_SIZE(dfu_partitions_ext); i++) {
		suit_plat_err_t ret2;

		struct dfu_cache_pool cache_pool = {
			.address = dfu_partitions_ext[i].address,
			.size = dfu_partitions_ext[i].size,
		};

		ret2 = suit_dfu_cache_partition_is_empty(&cache_pool);

		if (ret2 == SUIT_PLAT_ERR_NOMEM) {
			ret2 = erase_on_sink(dfu_partitions_ext[i].address,
					     dfu_partitions_ext[i].size);
		}

		if (ret2 != SUIT_PLAT_SUCCESS) {
			ret = ret2;
		}
	}

	/* Reset cache pool 0 to its initial state */
	dfu_partitions_ext[0].address = (uint8_t *)FIXED_PARTITION_OFFSET(dfu_partition);
	dfu_partitions_ext[0].size = FIXED_PARTITION_SIZE(dfu_partition);
	if (!translate_partition_start_address(&dfu_partitions_ext[0])) {
		return SUIT_PLAT_ERR_OUT_OF_BOUNDS;
	}

	prevent_second_init = false;

	return ret;
}

suit_plat_err_t suit_dfu_cache_rw_partition_info_get(uint8_t cache_partition_id,
						     const uint8_t **address, size_t *size)
{
	struct dfu_cache_partition_ext *partition = cache_partition_get(cache_partition_id);

	if (partition == NULL) {
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	*address = partition->address;
	*size = partition->size;

	return SUIT_PLAT_SUCCESS;
}

/**
 * @brief Write to nvm via flash_sink
 *
 * @param address Target address
 * @param data Data to be written
 * @param size Size of data to be written
 * @return suit_plat_err_t SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
static suit_plat_err_t write_to_sink(uint8_t *address, uint8_t *data, size_t size)
{
	struct stream_sink sink;

	suit_plat_err_t ret = suit_flash_sink_get(&sink, address, size);

	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Getting flash_sink failed. %i", ret);
		return SUIT_PLAT_ERR_IO;
	}

	ret = sink.write(sink.ctx, data, size);
	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Writing to sink failed. %i", ret);

		if (sink.release(sink.ctx) != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Sink release failed");
		}

		return SUIT_PLAT_ERR_IO;
	}

	ret = sink.release(sink.ctx);
	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Sink release failed");
		return ret;
	}

	return SUIT_PLAT_SUCCESS;
}

/**
 * @brief Use flash_sink to erase nvm region
 *
 * @param offset Target offset in NVM
 * @param size Size of region to be erased
 * @return suit_plat_err_t SUIT_PLAT_SUCCESS in case of success, otherwise error code
 */
static suit_plat_err_t erase_on_sink(uint8_t *address, size_t size)
{
	struct stream_sink sink;

	LOG_DBG("Erasing memory: %p(size:%u)", (void *)address, size);

	suit_plat_err_t ret = suit_flash_sink_get(&sink, address, size);

	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Getting flash_sink failed. %i", ret);
		return SUIT_PLAT_ERR_IO;
	}

	ret = sink.erase(sink.ctx);
	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Erasing on sink failed. %i", ret);

		if (sink.release(sink.ctx) != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Sink release failed");
		}

		return SUIT_PLAT_ERR_IO;
	}

	ret = sink.release(sink.ctx);
	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Sink release failed");
		return ret;
	}

	return SUIT_PLAT_SUCCESS;
}

/**
 * @brief Creates indefinite map at given offset by adding appropriate header and end marker
 *
 * @param part Pointer to cache partition structure
 * @param force_erase Erase the cache partition, if it's not already erased.
 * @return SUIT_PLAT_SUCCESS on success, otherwise error code
 */
static suit_plat_err_t partition_initialize(struct dfu_cache_partition_ext *part, bool force_erase)
{
	struct dfu_cache_pool cache_pool = {
		.address = part->address,
		.size = part->size,
	};

	if (part != NULL) {
		if (part->size >= MIN_VALID_PARTITION_SIZE) {
			LOG_INF("Partition %u: address(%p) size(%u)", part->id,
				(void *)part->address, part->size);

			suit_plat_err_t ret = suit_dfu_cache_partition_is_empty(&cache_pool);

			if (ret == SUIT_PLAT_ERR_CRASH) {
				return ret;
			}

			if (ret == SUIT_PLAT_ERR_NOMEM && force_erase) {
				LOG_INF("Erasing partition %p(size:%d) on init",
					(void *)part->address, part->size);
				ret = erase_on_sink(part->address, part->size);

				if (ret != SUIT_PLAT_SUCCESS) {
					return ret;
				}
			} else if (ret == SUIT_PLAT_ERR_NOMEM) {
				ret = suit_dfu_cache_partition_is_initialized(&cache_pool);

				if (ret == SUIT_PLAT_SUCCESS) {
					return ret;
				}

				LOG_INF("Erasing partition %p(size:%d) on init",
					(void *)part->address, part->size);
				ret = erase_on_sink(part->address, part->size);

				if (ret != SUIT_PLAT_SUCCESS) {
					return ret;
				}
			}

			return SUIT_PLAT_SUCCESS;
		}

		LOG_ERR("Partition too small to fit CBOR headers (%d)", part->size);
		return SUIT_PLAT_ERR_NOMEM;
	}

	LOG_ERR("Invalid argument.");
	return SUIT_PLAT_ERR_INVAL;
}

/**
 * @brief Check size of available free space in given cache and get allocable slot info
 *
 * @param cache Pointer to structure with information about single cache
 * @param slot Pointer to structure that will contain allocable slot info
 * @return SUIT_PLAT_SUCCESS on success, otherwise error code
 */
static suit_plat_err_t cache_free_space_check(struct dfu_cache_partition_ext *part,
					      struct suit_cache_slot *slot)
{
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;
	uintptr_t part_tmp_address;
	bool needs_erase = false;

	struct dfu_cache_pool cache_pool = {
		.address = part->address,
		.size = part->size,
	};

	if ((part != NULL) && (slot != NULL)) {
		part_tmp_address = (uintptr_t)part->address;
		if (suit_dfu_cache_partition_is_empty(&cache_pool) != SUIT_PLAT_SUCCESS) {
			ret = suit_dfu_cache_partition_find_free_space(
				&cache_pool, &part_tmp_address, &needs_erase);
			if (ret == SUIT_PLAT_ERR_CBOR_DECODING) {
				part_tmp_address = (uintptr_t)part->address;
				ret = SUIT_PLAT_SUCCESS;
			}
		}

		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Free space was not found: %d", ret);
			return ret;
		}

		LOG_INF("partition address %p", (void *)part->address);
		LOG_INF("partition size: %X", part->size);
		LOG_INF("partition tmp address: %lX", part_tmp_address);

		/* Subtract additional 1 byte to account for required indefinite map end marker
		 * which needs to fit within cache partition boundary.
		 */
		slot->size = (((uintptr_t)part->address + part->size) > part_tmp_address)
				     ? (size_t)part->address + part->size - part_tmp_address - 1
				     : 0;

		if ((part->address == slot->slot_address) && (slot->size > 0)) {
			/* This is a first slot at the beginning of the partition so we have to
			 * take into account required indefinite map header that will be added.
			 * We subtract its size.
			 */
			slot->size--;
		}

		slot->slot_address = (uint8_t *)part_tmp_address;

		if (!needs_erase) {
			return SUIT_PLAT_SUCCESS;
		}

		/* Clear corrupted slot */
		if (suit_dfu_cache_rw_slot_drop(slot) != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Clearing corrupted cache pool failed");
			return SUIT_PLAT_ERR_CRASH;
		}

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Invalid argument. NULL pointer.");
	return SUIT_PLAT_ERR_INVAL;
}

/**
 * @brief If possible allocate new slot in given cache partition
 *
 * @param uri URI that will be used as a key in cache map
 * @param slot Pointer to structure containing allocated slot inforamation
 * @param cache_index Index of cache in which slot shall be allocated
 * @return SUIT_PLAT_SUCCESS on success, otherwise error code
 */
static suit_plat_err_t slot_in_cache_partition_allocate(const struct zcbor_string *uri,
							struct suit_cache_slot *slot,
							struct dfu_cache_partition_ext *part)
{
	size_t encoded_size = 0;
	uint8_t output[ENCODING_OUTPUT_BUFFER_LENGTH];
	uint8_t *output_ptr = output;
	zcbor_state_t states[3];

	if ((uri != NULL) && (slot != NULL) && (part != NULL)) {
		if (uri->len > CONFIG_SUIT_MAX_URI_LENGTH) {
			LOG_ERR("URI longer than defined maximum CONFIG_SUIT_MAX_URI_LENGTH: %u",
				CONFIG_SUIT_MAX_URI_LENGTH);
			return SUIT_PLAT_ERR_NOMEM;
		}

		/* Check if uri is not a duplicate */
		const uint8_t *payload = NULL;
		size_t payload_size = 0;

		suit_plat_err_t ret =
			suit_dfu_cache_search(uri->value, uri->len, &payload, &payload_size);

		if (ret == SUIT_PLAT_SUCCESS) {
			/* Key URI is a duplicate */
			LOG_ERR("Key URI already exists.");
			return SUIT_PLAT_ERR_EXISTS;
		}

		/* Check how much free space is in given cache pool*/
		ret = cache_free_space_check(part, slot);

		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Insufficient free space in cache");
			return ret;
		}

		if (slot->slot_address == part->address) {
			output[0] = INDEFINITE_MAP_HEADER;
			output_ptr++;
		}

		zcbor_new_state(states, sizeof(states) / sizeof(zcbor_state_t), output_ptr,
				MAX_URI_ENCODE_BUFFER_LENGTH, 1, NULL, 0);

		if (!zcbor_tstr_encode(states, uri)) {
			return SUIT_PLAT_ERR_CRASH;
		}

		encoded_size = (size_t)states[0].payload - (size_t)output;

		/* 0x5A - byte string (four-byte uint32_t for n, and then n bytes follow) */
		output[encoded_size++] = 0x5A;
		slot->size_offset = encoded_size;

		/* Fill 4 size bytes to 0xFF so that they can be written later during slot closing
		 */
		memset(&output[encoded_size], 0xFF, 4);
		encoded_size += 4;

		if (slot->size < encoded_size) {
			LOG_ERR("Not enough free space in slot to write header.");
			return SUIT_PLAT_ERR_NOMEM;
		}

		ret = write_to_sink(slot->slot_address, output, encoded_size);
		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Writing slot header failed. %i", ret);
			return SUIT_PLAT_ERR_IO;
		}

		slot->data_offset = encoded_size;

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Invalid argument.");
	return SUIT_PLAT_ERR_INVAL;
}

/**
 * @brief Update size and offset of suit_cache_0 partition based of envelope location
 *
 * @param address  DFU partition address
 * @param size     DFU partition size
 *
 * @return SUIT_PLAT_SUCCESS on success, otherwise error code
 */
static suit_plat_err_t cache_0_update(uint8_t *address, size_t size)

{
	if ((address == NULL) || (size == 0)) {
		LOG_WRN("Initialized with empty DFU partition");
		dfu_partitions_ext[0].size = 0;
		return SUIT_PLAT_SUCCESS;
	}

	uint8_t *cache_0_end = dfu_partitions_ext[0].address + dfu_partitions_ext[0].size;

	/* Check if update address is in dfu_partition range */
	if ((address < dfu_partitions_ext[0].address) || (address >= cache_0_end)) {
		LOG_ERR("Envelope address doesn't match dfu_partition");
		return SUIT_PLAT_ERR_INVAL;
	}

	if (((uint8_t *)address + size > cache_0_end)) {
		LOG_ERR("Envelope exceeds dfu_partition");
		return SUIT_PLAT_ERR_INVAL;
	}

	uint8_t *tmp_offset = address + size;
	/* Align to nearest erase block */
	tmp_offset =
		(uint8_t *)(DIV_ROUND_UP((uintptr_t)tmp_offset, dfu_partitions_ext[0].eb_size) *
			    dfu_partitions_ext[0].eb_size);

	if (tmp_offset >= cache_0_end) {
		LOG_WRN("No free space for cache");
		dfu_partitions_ext[0].size = 0;
	} else {
		/* Set offset to the end of the envelope */
		dfu_partitions_ext[0].address = tmp_offset;

		/* Calculate remaining free space in dfu_partition */
		dfu_partitions_ext[0].size = cache_0_end - tmp_offset;
	}

	if (dfu_partitions_ext[0].size > 0) {
		bool force_erase = IS_ENABLED(CONFIG_SUIT_CACHE0_ERASE_ON_ENVELOPE_STORED);

		return partition_initialize(&dfu_partitions_ext[0], force_erase);
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_dfu_cache_rw_slot_create(uint8_t cache_partition_id,
					      struct suit_cache_slot *slot, const uint8_t *uri,
					      size_t uri_size)
{
	if ((slot != NULL) && (uri != NULL) && (uri_size > 0)) {
		struct zcbor_string tmp_uri = {.value = uri, .len = uri_size};

		if (uri[uri_size - 1] == '\0') {
			tmp_uri.len--;
		}

		struct dfu_cache_partition_ext *part = cache_partition_get(cache_partition_id);

		if (part == NULL) {
			LOG_ERR("Partition not found");
			return SUIT_PLAT_ERR_NOT_FOUND;
		}

		slot->eb_size = part->eb_size;
		suit_plat_err_t ret = slot_in_cache_partition_allocate(&tmp_uri, slot, part);

		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Allocating slot in cache failed: %d", ret);
			return ret;
		}

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Invalid argument. NULL pointer.");
	return SUIT_PLAT_ERR_INVAL;
}

suit_plat_err_t suit_dfu_cache_rw_slot_close(struct suit_cache_slot *slot, size_t size_used)
{
	if ((slot != NULL) && (slot->size >= size_used)) {
		uint32_t tmp = BSWAP_32(size_used);
		size_t tmp_size = sizeof(uint32_t);
		size_t end_address = (size_t)slot->slot_address + slot->data_offset + size_used;

		/* Update byte string size */
		if (write_to_sink(slot->slot_address + slot->size_offset, (uint8_t *)&tmp,
				  tmp_size) != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Updating cache slot size in header failed.");
			return SUIT_PLAT_ERR_IO;
		}

		struct dfu_cache_partition_ext *part =
			cache_partition_get_by_address(slot->slot_address);
		if (part == NULL) {
			LOG_ERR("Couldn't find partition matching slot offset");
			return SUIT_PLAT_ERR_IO;
		}

		tmp_size = (slot->data_offset + size_used);
		size_t padding_size = ROUND_UP(tmp_size, slot->eb_size) - tmp_size;

		/* Minimal size of an entry in the map is 2:
		 * 0x60 - empty uri ""
		 * 0x40 - empty byte string h''
		 */
		if (padding_size == 1) {
			padding_size += slot->eb_size;
		}

		if ((size_used + padding_size) > slot->size) {
			LOG_ERR("Padding (header + bytes) would overflow slot boundaries");
			return SUIT_PLAT_ERR_NOMEM;
		}

		LOG_DBG("Number of padding bytes required: %u", padding_size);

		if (padding_size > 0) {
			/* Assumed worst case scenario is that padding size is not bigger than
			 * uint16
			 */
			uint8_t header[] = {0x60, 0, 0, 0};
			size_t header_size = 0;

			if (padding_size <= 23) {
				header_size = 2;
				padding_size -= header_size;
				header[1] =
					0x40 +
					padding_size; /* byte string (0x00..0x17 bytes follow) */
			} else if (padding_size <= UINT16_MAX) {
				header_size = 4;
				padding_size -= header_size;
				/* byte string (two-byte uint16_t for n, and then n bytes follow) */
				header[1] = 0x59;
				*(uint16_t *)(&header[2]) = BSWAP_16(padding_size);
			} else {
				LOG_ERR("Number of required padding bytes exceeds assumed max size "
					"0xFFFF");
				return SUIT_PLAT_ERR_INVAL;
			}

			if (write_to_sink((uint8_t *)end_address, header, header_size)) {
				LOG_ERR("Writing CBOR cache slot header for padding failed.");
				return SUIT_PLAT_ERR_IO;
			}

			end_address += header_size;

			tmp_size = 1;
			for (size_t i = 0; i < padding_size; i++) {
				if (write_to_sink((uint8_t *)(end_address + i), &(uint8_t){0},
						  tmp_size)) {
					LOG_ERR("Writing padding byte failed.");
					return SUIT_PLAT_ERR_IO;
				}
			}

			end_address += padding_size;
		}

		/* To be used as end marker */
		tmp = 0xFFFFFFFF;
		tmp_size = 1;

		/* Add indefinite map, end marker 0xFF */
		if (write_to_sink((uint8_t *)end_address, (uint8_t *)&tmp, tmp_size) !=
		    SUIT_PLAT_SUCCESS) {
			LOG_ERR("Writing CBOR map end marker to cache partition failed.");
			return SUIT_PLAT_ERR_IO;
		}

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Invalid argument. NULL pointer or invalid size.");
	return SUIT_PLAT_ERR_INVAL;
}

suit_plat_err_t suit_dfu_cache_rw_slot_drop(struct suit_cache_slot *slot)
{
	LOG_INF("DROPPING SLOT");
	uint8_t map_header;

	if (slot != NULL) {
		struct dfu_cache_partition_ext *part =
			cache_partition_get_by_address(slot->slot_address);

		if (part == NULL) {
			LOG_ERR("Couldn't find partition matching slot offset");
			return SUIT_PLAT_ERR_IO;
		}

		uint8_t *erase_address = slot->slot_address;
		size_t erase_size = (part->address + part->size) - slot->slot_address;
		size_t write_size = 1;

		LOG_INF("Erase area: (addr: %p, size: 0x%x)", slot->slot_address, part->size);
		if (erase_size < slot->eb_size) {
			LOG_ERR("Unable to erase area: (addr: %p, size: 0x%x)", slot->slot_address,
				erase_size);
			return SUIT_PLAT_ERR_IO;
		}

		suit_plat_err_t ret = suit_dfu_cache_memcpy(
			&map_header, (uintptr_t)slot->slot_address, sizeof(map_header));

		if (ret > 0) {
			LOG_ERR("Read from cache failed: %d", ret);
			return ret;
		}

		bool add_map_header = false;

		if (map_header == INDEFINITE_MAP_HEADER) {
			add_map_header = true;
		}

		ret = erase_on_sink(erase_address, erase_size);
		if (ret != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Erasing cache failed: %i", ret);
			return SUIT_PLAT_ERR_IO;
		}

		if (add_map_header) {
			LOG_DBG("Restore map header (%p)", slot->slot_address);
			int ret = write_to_sink(erase_address, &(uint8_t){INDEFINITE_MAP_HEADER},
						write_size);
			if (ret != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Unable to restore slot after erase: %i", ret);
				return SUIT_PLAT_ERR_IO;
			}
		}

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Invalid argument. NULL pointer.");
	return SUIT_PLAT_ERR_INVAL;
}

static int preinitialize(void)
{
	if (!setup_partition_addresses()) {
		return -EIO;
	}

	return 0;
}

SYS_INIT(preinitialize, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
