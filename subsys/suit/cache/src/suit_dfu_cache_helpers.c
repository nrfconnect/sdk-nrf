/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <suit_dfu_cache.h>
#include <zephyr/logging/log.h>
#include "zcbor_encode.h"
#include <zephyr/sys/util_macro.h>

#include <suit_address_streamer_selector.h>
#include "suit_dfu_cache_internal.h"
#include "suit_ram_sink.h"
#include "zcbor_noncanonical_decode.h"

LOG_MODULE_REGISTER(dfu_cache_helpers, CONFIG_SUIT_LOG_LEVEL);

suit_plat_err_t suit_dfu_cache_partition_slot_foreach(struct dfu_cache_pool *cache_pool,
						      partition_slot_foreach_cb cb, void *ctx)
{
	suit_plat_err_t err = SUIT_PLAT_SUCCESS;
	zcbor_state_t states[4];
	struct zcbor_string uri;
	struct zcbor_string_fragment data_fragment;
	bool first_iteration = true;
	bool result = true;
	uintptr_t current_offset = 0;
	uint8_t partition_header_storage[CACHE_METADATA_MAX_LENGTH];

	LOG_DBG("Foreach on cache %p(size:%u)", (void *)cache_pool->address, cache_pool->size);

	zcbor_new_decode_state(states, ZCBOR_ARRAY_SIZE(states), NULL, 0, 1, NULL, 0);

	do {
		uintptr_t current_address = (uintptr_t)cache_pool->address + current_offset;
		size_t cache_remaining_size = cache_pool->size - current_offset;
		size_t read_size = MIN(sizeof(partition_header_storage), cache_remaining_size);

		if (cache_remaining_size == 0) {
			/* If the end of cache partition is reached and the last element was
			 * decoded correctly, end with a success (even though no end of marker
			 * is present). Handling such a case separately is necessary, as padding
			 * always ends at an address aligned to the erase block size, and the
			 * partition size also should aligned to the eb size, thus not leaving
			 * space for the single byte end of map marker.
			 */
			break;
		}

		err = suit_dfu_cache_memcpy(partition_header_storage, current_address, read_size);

		if (err != SUIT_PLAT_SUCCESS) {
			break;
		}

		LOG_HEXDUMP_DBG(partition_header_storage, MIN(read_size, 16), "Memory content");

		/* The CBOR on the cache is processed in parts.
		 * The partition_header_storage is large enough to store the indefinite map header
		 * at the beginning of the cache partition, an URI TSTR, and a BSTR header of the
		 * payload.
		 *
		 * The zcbor state is first initialized to where the indefinite map header
		 * is supposed to be. If there is not indefinite map header tag, then the
		 * partition is corrupted and no further processing is done.
		 *
		 * At the start of each iteration, it is detected if the end of the map was reached
		 * by checking if the next byte is 0xFF. For an indefinite length map, this is
		 * the map end indicator, for a definite length map, this means that the rest of
		 * the memory is erased, so the map is considered to be ended.
		 * If the end of the map is detected, the loop is exited and the function succeeds.
		 *
		 * In second step the TSTR is parsed. If a correct TSTR is found, we move to the
		 * next step. Otherwise we bail.
		 *
		 * In third step the BSTR header is decoded. The zcbor_bstr_start_decode_fragment
		 * pushes a zcbor backup. Since we won't be decoding any further BSTR fragments,
		 * the backup is immediately restored (zcbor_process_backup).
		 *
		 * If correct URI and payload was found, a user-provided callback is executed.
		 *
		 * Lastly, we calculate the next address at which the next URI TSTR is supposed
		 * to be and repeat the whole process.
		 */

		zcbor_update_state(states, partition_header_storage, read_size);
		if (first_iteration) {
			result = zcbor_noncanonical_map_start_decode(states);
			first_iteration = false;
			if (!result) {
				err = SUIT_PLAT_ERR_CBOR_DECODING;
			}
		}

		if (partition_header_storage[0] == 0xFF) {
			break;
		}

		if (result) {
			result = zcbor_tstr_decode(states, &uri);
		}

		if (result) {
			LOG_HEXDUMP_DBG(uri.value, uri.len, "Currently iterated URI");
			result = zcbor_noncanonical_bstr_start_decode_fragment(states,
									       &data_fragment);
			result = result &&
				 zcbor_process_backup(states,
						      ZCBOR_FLAG_RESTORE | ZCBOR_FLAG_CONSUME |
							      ZCBOR_FLAG_KEEP_PAYLOAD,
						      ZCBOR_MAX_ELEM_COUNT);
		}

		if (!result) {
			err = SUIT_PLAT_ERR_CBOR_DECODING;
			break;
		}

		off_t bstr_data_offset = data_fragment.fragment.value - partition_header_storage;

		if (data_fragment.total_len > cache_remaining_size - bstr_data_offset) {
			/* The bstr data would exceed the remaining size of the cache partition */
			err = SUIT_PLAT_ERR_CBOR_DECODING;
			break;
		}

		if (cb) {
			uintptr_t data_address = current_address + bstr_data_offset;

			result = cb(cache_pool, states, &uri, data_address, data_fragment.total_len,
				    ctx);
		}

		current_offset += (data_fragment.total_len + bstr_data_offset);
	} while (result);

	return err;
}

suit_plat_err_t suit_dfu_cache_partition_is_initialized(struct dfu_cache_pool *partition)
{
	suit_plat_err_t err;

	if (partition != NULL) {
		err = suit_dfu_cache_partition_slot_foreach(partition, NULL, NULL);
		if (err == SUIT_PLAT_ERR_CBOR_DECODING) {
			return SUIT_PLAT_ERR_NOT_FOUND;
		}

		return SUIT_PLAT_SUCCESS;
	}

	LOG_ERR("Invalid argument.");
	return SUIT_PLAT_ERR_INVAL;
}

suit_plat_err_t suit_dfu_cache_partition_is_empty(struct dfu_cache_pool *cache_pool)
{
	uint8_t *address = cache_pool->address;
	size_t remaining = cache_pool->size;
	uint8_t buffer[128];
	const size_t chunk_size = sizeof(buffer);
	suit_plat_err_t ret = SUIT_PLAT_SUCCESS;

	while (remaining > 0) {
		size_t read_size = MIN(chunk_size, remaining);

		ret = suit_dfu_cache_memcpy(buffer, (uintptr_t)address, read_size);
		if (ret != SUIT_PLAT_SUCCESS) {
			break;
		}

		for (int i = 0; i < read_size; i++) {
			if (buffer[i] != 0xFF) {
				return SUIT_PLAT_ERR_NOMEM;
			}
		}

		remaining -= read_size;
	}

	return ret;
}

static bool find_free_address(struct dfu_cache_pool *cache_pool, zcbor_state_t *state,
			      const struct zcbor_string *uri, uintptr_t payload_offset,
			      size_t payload_size, void *ctx)
{
	uintptr_t *ret = ctx;
	*ret = payload_offset + payload_size;

	return true;
}

suit_plat_err_t suit_dfu_cache_partition_find_free_space(struct dfu_cache_pool *partition,
							 uintptr_t *free_address, bool *needs_erase)
{
	uintptr_t address = (uintptr_t)partition->address;
	suit_plat_err_t ret;
	uint8_t partition_buffer[2];

	*free_address = 0;
	*needs_erase = false;

	ret = suit_dfu_cache_partition_slot_foreach(partition, find_free_address, &address);

	if (ret == SUIT_PLAT_SUCCESS) {
		ret = suit_dfu_cache_memcpy(partition_buffer, address, sizeof(partition_buffer));
	}

	if (ret == SUIT_PLAT_SUCCESS) {
		if (partition_buffer[0] == 0xBF) {
			address++;
		}
		if (partition_buffer[1] != 0xFF) {
			*needs_erase = true;
		}
		*free_address = address;
	}

	return ret;
}

suit_plat_err_t suit_dfu_cache_memcpy(uint8_t *destination, uintptr_t source, size_t size)
{
	suit_plat_err_t ret;
	suit_address_streamer stream_fn;
	struct stream_sink sink;

	if (size == 0) {
		return SUIT_PLAT_SUCCESS;
	}

	ret = suit_ram_sink_get(&sink, destination, size);
	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Could not acquire RAM sink: %d", ret);
		return ret;
	}

	stream_fn = suit_address_streamer_select_by_address((uint8_t *)source);
	if (stream_fn == NULL) {
		LOG_ERR("Could not find streamer for address: %p", (void *)source);
		ret = SUIT_PLAT_ERR_IO;
	}

	if (ret == SUIT_PLAT_SUCCESS) {
		ret = stream_fn((uint8_t *)source, size, &sink);
	}

	if (sink.release) {
		suit_plat_err_t ret2 = sink.release(sink.ctx);

		if (ret2 != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Failed to release sink: %d", ret2);
		}

		/* Prevent result of sink.release from obscuring previous errors. */
		ret = ret == SUIT_PLAT_SUCCESS ? ret2 : ret;
	}

	return ret;
}
