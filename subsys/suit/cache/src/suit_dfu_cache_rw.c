/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <suit_dfu_cache_rw.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include "zcbor_encode.h"
#include <zephyr/sys/byteorder.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/util_macro.h>
#include <suit_flash_sink.h>

#include "suit_dfu_cache_internal.h"
#include <suit_envelope_info.h>

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

struct dfu_cache_partition_ext { /* Extended structure describing single cache partition */
	const struct device *fdev;
	size_t offset;
	size_t size;
	uint8_t *address;
	size_t eb_size;
	size_t wb_size;
	uint8_t id;
};

#define PARTITION_INIT(index, label)                                                               \
	{                                                                                          \
		.fdev = FIXED_PARTITION_DEVICE(label),                                             \
		.offset = FIXED_PARTITION_OFFSET(label),                                           \
		.size = FIXED_PARTITION_SIZE(label),                                               \
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
		.id = 0,
	},
	LISTIFY(CONFIG_SUIT_CACHE_MAX_CACHES, PARTITION_DEFINE, (), dfu_cache_partition_)};

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
 * @brief Adjust cache pool 0 location and size based on characteristics of
 * update candidate envelope
 *
 */
static suit_plat_err_t cache_0_resize(bool drop_content)
{
	struct suit_nvm_device_info device_info;
	const uint8_t *uc_env_addr = NULL;
	size_t uc_env_size = 0;

	struct dfu_cache_partition_ext *partition = &dfu_partitions_ext[0];

	suit_plat_err_t err = suit_dfu_partition_device_info_get(&device_info);

	if (err != SUIT_PLAT_SUCCESS) {
		return SUIT_PLAT_ERR_IO;
	}

	suit_dfu_cache_deinitialize();

	suit_dfu_partition_envelope_info_get(&uc_env_addr, &uc_env_size);
	partition->size = 0;
	partition->address = 0;

	if (uc_env_size > 0 && uc_env_size <= device_info.partition_size) {
		/* DFU partition is NOT empty and it is NOT full */

		partition->fdev = device_info.fdev;
		partition->eb_size = device_info.erase_block_size;
		partition->wb_size = device_info.write_block_size;

		/* Align to nearest erase block */
		size_t cache_pool_offset = DIV_ROUND_UP(device_info.partition_offset + uc_env_size,
							device_info.erase_block_size) *
					   device_info.erase_block_size;

		size_t cache_pool_size = device_info.partition_size -
					 (cache_pool_offset - device_info.partition_offset);

		if (cache_pool_size > 0) {
			partition->offset = cache_pool_offset;
			partition->size = cache_pool_size;
			partition->address = device_info.mapped_address +
					     (cache_pool_offset - device_info.partition_offset);

			LOG_INF("DFU Cache pool 0, addr: %p, size %d bytes",
				(void *)partition->address, partition->size);

			if (drop_content) {
				struct dfu_cache_pool cache_pool = {
					.address = partition->address,
					.size = partition->size,
				};

				suit_plat_err_t err =
					suit_dfu_cache_partition_is_empty(&cache_pool);
				if (err == SUIT_PLAT_ERR_NOMEM) {
					LOG_INF("DFU Cache pool 0 is not empty... Erasing");
					erase_on_sink(partition->address, partition->size);
				}
			}
		}
	}

	struct dfu_cache cache_pool_info;

	for (size_t i = 0; i < ARRAY_SIZE(dfu_partitions_ext); i++) {
		cache_pool_info.pools[i].size = dfu_partitions_ext[i].size;
		cache_pool_info.pools[i].address = dfu_partitions_ext[i].address;
	}
	cache_pool_info.pools_count = ARRAY_SIZE(dfu_partitions_ext);

	return suit_dfu_cache_initialize(&cache_pool_info);
}

suit_plat_err_t suit_dfu_cache_0_resize(void)
{
	return cache_0_resize(true);
}

suit_plat_err_t suit_dfu_cache_validate_content(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(dfu_partitions_ext); i++) {

		struct dfu_cache_partition_ext *partition = &dfu_partitions_ext[i];

		if (partition->address && partition->size) {
			LOG_INF("DFU Cache pool, id: %d, addr: %p, size %d bytes", partition->id,
				(void *)partition->address, partition->size);

			struct dfu_cache_pool cache_pool = {
				.address = partition->address,
				.size = partition->size,
			};

			suit_plat_err_t err = suit_dfu_cache_partition_is_initialized(&cache_pool);

			if (err == SUIT_PLAT_ERR_NOT_FOUND) {
				LOG_INF("DFU Cache pool, id: %d does not contain valid content",
					partition->id);

				err = suit_dfu_cache_partition_is_empty(&cache_pool);
				if (err == SUIT_PLAT_ERR_NOMEM) {
					LOG_INF("DFU Cache pool, id: %d is not empty... Erasing",
						partition->id);
					erase_on_sink(partition->address, partition->size);
				}
			} else if (err != SUIT_PLAT_SUCCESS) {
				LOG_ERR("DFU Cache pool, id: %d unavailable, err: %d",
					partition->id, err);
				return err;
			}
		}
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_dfu_cache_drop_content(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(dfu_partitions_ext); i++) {

		struct dfu_cache_partition_ext *partition = &dfu_partitions_ext[i];

		if (partition->address && partition->size) {

			struct dfu_cache_pool cache_pool = {
				.address = partition->address,
				.size = partition->size,
			};

			suit_plat_err_t err = suit_dfu_cache_partition_is_empty(&cache_pool);

			if (err == SUIT_PLAT_ERR_NOMEM) {
				LOG_INF("DFU Cache pool, id: %d is not empty... Erasing",
					partition->id);
				erase_on_sink(partition->address, partition->size);
			}
		}
	}

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_dfu_cache_rw_device_info_get(uint8_t cache_partition_id,
						  struct suit_nvm_device_info *device_info)
{

	if (device_info != NULL) {

		struct dfu_cache_partition_ext *partition = cache_partition_get(cache_partition_id);

		if (partition == NULL || partition->size == 0) {
			return SUIT_PLAT_ERR_NOT_FOUND;
		}

		device_info->fdev = partition->fdev;
		device_info->mapped_address = partition->address;
		device_info->erase_block_size = partition->eb_size;
		device_info->write_block_size = partition->wb_size;
		device_info->partition_offset = partition->offset;
		device_info->partition_size = partition->size;

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Invalid argument. NULL pointer.");
	return SUIT_PLAT_ERR_INVAL;
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

			/* Left padding area as-is to save time.
			 * It is inaccessible through the cache API.
			 */
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

		LOG_DBG("Erase area: (addr: %p, size: 0x%x)", (void *)slot->slot_address,
			part->size);
		if (erase_size < slot->eb_size) {
			LOG_ERR("Unable to erase area: (addr: %p, size: 0x%x)",
				(void *)slot->slot_address, erase_size);
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
			LOG_DBG("Restore map header (%p)", (void *)slot->slot_address);
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

/**
 * @brief Update characteristics of NVM device unavailable at compile time
 *
 * For some NVM devices, especially external Flash, some characteristics
 * like write or erase block size can be at runtime only.
 * The same applies to memory mapping.
 *
 * For sake of simplicity, for devices with varying erase block size
 * the largest erase block size is taken as reference.
 *
 * Functionality leaves NVM content intact.
 *
 */
static int preinitialize(void)
{
	for (size_t i = 1; i < ARRAY_SIZE(dfu_partitions_ext); i++) {

		/* Calculating memory-mapped address for cache pool */
		struct dfu_cache_partition_ext *partition = &dfu_partitions_ext[i];
		struct nvm_address device_offset = {
			.fdev = partition->fdev,
			.offset = partition->offset,
		};

		uintptr_t mapped_addr = 0;

		if (suit_memory_nvm_address_to_global_address(&device_offset, &mapped_addr)) {
			partition->address = (uint8_t *)mapped_addr;
		}

		/* Getting write block size. External devices provide that information on runtime */
		size_t wb_size = flash_get_write_block_size(partition->fdev);

		partition->wb_size = wb_size;

		/* Calculating the biggest erase block size */
		struct flash_pages_info info;

		if (0 == flash_get_page_info_by_offs(partition->fdev, partition->offset, &info)) {

			if (info.start_offset != partition->offset) {
				LOG_ERR("Partition for DFU cache pool %d unaligned to device erase "
					"block",
					partition->id);
			}

			size_t page_count = flash_get_page_count(partition->fdev);
			size_t eb_size = info.size;

			for (uint32_t p_idx = info.index + 1; p_idx < page_count; p_idx++) {

				if (0 ==
				    flash_get_page_info_by_idx(partition->fdev, p_idx, &info)) {
					if (info.size > eb_size) {
						LOG_WRN("Partition for DFU cache pool %d placed on "
							"device with multiple size erase blocks",
							partition->id);
						eb_size = info.size;
					}

					if (info.start_offset + info.size >=
					    partition->offset + partition->size) {
						break;
					}
				}
			}

			partition->eb_size = eb_size;

			LOG_DBG("DFU cache pool %d, Address: %p, size: %d bytes, erase block: %d, "
				"write block: %d",
				partition->id, (void *)partition->address, partition->size,
				partition->eb_size, partition->wb_size);

		} else {
			LOG_ERR("Cannot obtain initial page info for DFU cache pool %d",
				partition->id);
		}
	}

	int err = cache_0_resize(false);

	if (err != SUIT_PLAT_SUCCESS) {
		return -EIO;
	}
	return 0;
}

SYS_INIT(preinitialize, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
