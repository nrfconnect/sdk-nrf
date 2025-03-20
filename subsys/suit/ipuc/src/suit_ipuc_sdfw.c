/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "suit_ipuc_sdfw.h"
#include <suit_platform_internal.h>
#include <suit_memory_layout.h>
#include <suit_plat_decode_util.h>
#include <suit_generic_address_streamer.h>
#include <suit_address_streamer_selector.h>
#include <suit_ram_sink.h>
#include <suit_flash_sink.h>
#include <sdfw/arbiter.h>

#if CONFIG_SUIT_DIGEST_CACHE
#include <suit_plat_digest_cache.h>
#endif /* CONFIG_SUIT_DIGEST_CACHE */

#ifdef CONFIG_SUIT_STREAM_SINK_DIGEST
#include <suit_digest_sink.h>
#include <psa/crypto.h>
#endif /* CONFIG_SUIT_STREAM_SINK_DIGEST */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(suit_ipuc, CONFIG_SUIT_LOG_LEVEL);

#define READ_CHUNK_MAX_SIZE 16

typedef enum {
	IPUC_UNUSED,
	IPUC_SDFW_MIRROR,
	IPUC_IPC_IN_PLACE_UPDATE
} ipuc_usage_t;

typedef struct {
	struct zcbor_string component_id;
	suit_manifest_role_t role;
	ipuc_usage_t usage;
	int ipc_client_id;
	size_t write_peek_offset;
	bool last_chunk_stored;
} suit_ipuc_entry_t;

static suit_ipuc_entry_t components[CONFIG_SUIT_IPUC_SIZE];
static K_MUTEX_DEFINE(ipuc_mutex);

static struct zcbor_string *ipuc_component_id_from_handle(suit_component_t handle)
{
	struct zcbor_string *component_id = NULL;
	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	if (suit_plat_component_id_get(handle, &component_id) != SUIT_SUCCESS) {
		return NULL;
	}

	if (suit_plat_decode_component_type(component_id, &component_type) != SUIT_PLAT_SUCCESS) {
		return NULL;
	}

	if (component_type != SUIT_COMPONENT_TYPE_MEM) {
		/* The only supported component type is MEM
		 */
		return NULL;
	}

	return component_id;
}

static suit_ipuc_entry_t *ipuc_entry_from_component_id(struct zcbor_string *component_id)
{
	if (component_id == NULL) {
		return NULL;
	}

	for (size_t i = 0; i < CONFIG_SUIT_IPUC_SIZE; i++) {
		suit_ipuc_entry_t *p_entry = &components[i];

		if (suit_compare_zcbor_strings(component_id, &p_entry->component_id)) {
			return p_entry;
		}
	}

	return NULL;
}

static suit_ipuc_entry_t *ipuc_entry_from_handle(suit_component_t handle)
{
	struct zcbor_string *component_id = ipuc_component_id_from_handle(handle);

	return ipuc_entry_from_component_id(component_id);
}

static suit_plat_err_t ipuc_decode_mem_component_id(struct zcbor_string *component_id,
						    intptr_t *slot_address, size_t *slot_size)
{
	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	intptr_t decoded_slot_address = 0;
	size_t decoded_slot_size = 0;

	if (suit_plat_decode_component_type(component_id, &component_type) != SUIT_PLAT_SUCCESS ||
	    component_type != SUIT_COMPONENT_TYPE_MEM) {
		return SUIT_PLAT_ERR_UNSUPPORTED;
	}

	if (suit_plat_decode_address_size(component_id, &decoded_slot_address,
					  &decoded_slot_size) != SUIT_PLAT_SUCCESS ||
	    decoded_slot_address == 0 || decoded_slot_size == 0) {
		return SUIT_PLAT_ERR_UNSUPPORTED;
	}

	*slot_address = decoded_slot_address;
	*slot_size = decoded_slot_size;

	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_ipuc_sdfw_get_count(size_t *count)
{

	if (count == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	size_t declared_count = 0;

	k_mutex_lock(&ipuc_mutex, K_FOREVER);
	for (size_t i = 0; i < CONFIG_SUIT_IPUC_SIZE; i++) {
		suit_ipuc_entry_t *ipuc_entry = &components[i];

		if (ipuc_entry->component_id.value != NULL && ipuc_entry->component_id.len != 0) {
			declared_count++;
		}
	}
	*count = declared_count;
	k_mutex_unlock(&ipuc_mutex);
	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_ipuc_sdfw_get_info(size_t idx, struct zcbor_string *component_id,
					suit_manifest_role_t *role)
{
	if (component_id == NULL || role == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	size_t cur_idx = 0;

	k_mutex_lock(&ipuc_mutex, K_FOREVER);
	for (size_t i = 0; i < CONFIG_SUIT_IPUC_SIZE; i++) {
		suit_ipuc_entry_t *ipuc_entry = &components[i];

		if (ipuc_entry->component_id.value != NULL && ipuc_entry->component_id.len != 0) {
			if (cur_idx == idx) {
				component_id->value = ipuc_entry->component_id.value;
				component_id->len = ipuc_entry->component_id.len;
				*role = ipuc_entry->role;
				k_mutex_unlock(&ipuc_mutex);
				return SUIT_PLAT_SUCCESS;
			}
			cur_idx++;
		}
	}

	k_mutex_unlock(&ipuc_mutex);
	return SUIT_PLAT_ERR_NOT_FOUND;
}

suit_plat_err_t suit_ipuc_sdfw_write_setup(int ipc_client_id, struct zcbor_string *component_id,
					   struct zcbor_string *encryption_info,
					   struct zcbor_string *compression_info)
{
	intptr_t slot_address = 0;
	size_t slot_size = 0;

	if (component_id == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (ipuc_decode_mem_component_id(component_id, &slot_address, &slot_size) !=
	    SUIT_PLAT_SUCCESS) {
		/* Only MEM components are supported
		 */
		return SUIT_PLAT_ERR_UNSUPPORTED;
	}

	if (encryption_info != NULL && encryption_info->value != NULL &&
	    encryption_info->len != 0) {
		/* Encryption not supported yet
		 */
		return SUIT_PLAT_ERR_UNSUPPORTED;
	}

	if (compression_info != NULL && compression_info->value != NULL &&
	    compression_info->len != 0) {
		/* Compression not supported yet
		 */
		return SUIT_PLAT_ERR_UNSUPPORTED;
	}

	k_mutex_lock(&ipuc_mutex, K_FOREVER);
	suit_ipuc_entry_t *ipuc_entry = ipuc_entry_from_component_id(component_id);

	if (ipuc_entry == NULL) {
		k_mutex_unlock(&ipuc_mutex);
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	if (!(ipuc_entry->usage == IPUC_UNUSED || (ipuc_entry->usage == IPUC_IPC_IN_PLACE_UPDATE &&
						   ipuc_entry->ipc_client_id == ipc_client_id))) {
		LOG_ERR("suit_ipuc_sdfw_write_setup, IPUC address: %p, busy", (void *)slot_address);
		k_mutex_unlock(&ipuc_mutex);
		return SUIT_PLAT_ERR_INCORRECT_STATE;
	}

	/* Erasing if necessary
	 */
	uint8_t read_buffer[READ_CHUNK_MAX_SIZE];
	struct stream_sink ram_sink;

	if (suit_ram_sink_get(&ram_sink, read_buffer, READ_CHUNK_MAX_SIZE) != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Could not acquire RAM sink");
		k_mutex_unlock(&ipuc_mutex);
		return SUIT_PLAT_ERR_IO;
	}

	bool clear = true;

	for (size_t off = 0; off < slot_size && clear == true; off += READ_CHUNK_MAX_SIZE) {
		size_t cur_chunk_size = slot_size - off;

		if (cur_chunk_size > READ_CHUNK_MAX_SIZE) {
			cur_chunk_size = READ_CHUNK_MAX_SIZE;
		}

		suit_address_streamer stream_fn =
			suit_address_streamer_select_by_address((uint8_t *)(slot_address + off));
		if (stream_fn == NULL) {
			LOG_ERR("Could not find streamer for address: %p",
				(void *)(slot_address + off));
			if (ram_sink.release) {
				ram_sink.release(ram_sink.ctx);
			}
			k_mutex_unlock(&ipuc_mutex);
			return SUIT_PLAT_ERR_IO;
		}

		if (ram_sink.seek) {
			ram_sink.seek(ram_sink.ctx, 0);
		}

		if (stream_fn((uint8_t *)(slot_address + off), cur_chunk_size, &ram_sink) !=
		    SUIT_PLAT_SUCCESS) {
			LOG_ERR("Stream function has failed");
			if (ram_sink.release) {
				ram_sink.release(ram_sink.ctx);
			}
			k_mutex_unlock(&ipuc_mutex);
			return SUIT_PLAT_ERR_IO;
		}

		for (size_t ram_off = 0; ram_off < cur_chunk_size; ram_off++) {
			if (read_buffer[ram_off] != 0xFF) {
				clear = false;
				break;
			}
		}
	}

	if (ram_sink.release) {
		ram_sink.release(ram_sink.ctx);
	}

	if (!clear) {
		struct stream_sink ipuc_sink;
		bool ipuc_sink_allocated = false;

		LOG_INF("suit_ipuc_sdfw_write_setup, address: %p, size: %d bytes, erasing "
			"needed",
			(void *)slot_address, slot_size);

		if (suit_flash_sink_is_address_supported((uint8_t *)slot_address)) {
			if (suit_flash_sink_get(&ipuc_sink, (uint8_t *)slot_address, slot_size) ==
			    SUIT_PLAT_SUCCESS) {
				ipuc_sink_allocated = true;
			}
		}

		if (!ipuc_sink_allocated) {
			k_mutex_unlock(&ipuc_mutex);
			return SUIT_PLAT_ERR_IO;
		}

#if CONFIG_SUIT_DIGEST_CACHE
		suit_plat_digest_cache_remove(component_id);
#endif /* CONFIG_SUIT_DIGEST_CACHE */


		if (ipuc_sink.release) {
			ipuc_sink.release(ipuc_sink.ctx);
		}
	}

	ipuc_entry->usage = IPUC_IPC_IN_PLACE_UPDATE;
	ipuc_entry->ipc_client_id = ipc_client_id;
	ipuc_entry->write_peek_offset = 0;
	ipuc_entry->last_chunk_stored = false;

	LOG_INF("suit_ipuc_sdfw_write_setup success, IPUC address: %p", (void *)slot_address);

	k_mutex_unlock(&ipuc_mutex);
	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_ipuc_sdfw_write(int ipc_client_id, struct zcbor_string *component_id,
				     size_t offset, uintptr_t buffer, size_t chunk_size,
				     bool last_chunk)
{
	intptr_t slot_address = 0;
	size_t slot_size = 0;

	if (component_id == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (ipuc_decode_mem_component_id(component_id, &slot_address, &slot_size) !=
	    SUIT_PLAT_SUCCESS) {
		/* Only MEM components are supported
		 */
		return SUIT_PLAT_ERR_UNSUPPORTED;
	}

	k_mutex_lock(&ipuc_mutex, K_FOREVER);
	suit_ipuc_entry_t *ipuc_entry = ipuc_entry_from_component_id(component_id);

	if (ipuc_entry == NULL) {
		k_mutex_unlock(&ipuc_mutex);
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	if (ipuc_entry->usage != IPUC_IPC_IN_PLACE_UPDATE ||
	    ipuc_entry->ipc_client_id != ipc_client_id) {
		k_mutex_unlock(&ipuc_mutex);
		return SUIT_PLAT_ERR_INCORRECT_STATE;
	}

	if (ipuc_entry->last_chunk_stored) {
		k_mutex_unlock(&ipuc_mutex);
		return SUIT_PLAT_ERR_INCORRECT_STATE;
	}

	if (buffer && chunk_size > 0) {

		struct stream_sink ipuc_sink;

		if (suit_flash_sink_get(&ipuc_sink, (uint8_t *)slot_address, slot_size) !=
		    SUIT_PLAT_SUCCESS) {
			LOG_ERR("IPUC sink not allocated");
			k_mutex_unlock(&ipuc_mutex);
			return SUIT_PLAT_ERR_IO;
		}

		if ((ipuc_sink.write != NULL) && (ipuc_sink.seek != NULL)
		    && (offset > ipuc_entry->write_peek_offset)) {
			/* Erasing if necessary
			 */
			uint8_t erase_buffer[READ_CHUNK_MAX_SIZE] = {
					[0 ... READ_CHUNK_MAX_SIZE - 1] = 0xFF
			};

			ipuc_sink.seek(ipuc_sink.ctx, ipuc_entry->write_peek_offset);
			for (size_t off = ipuc_entry->write_peek_offset; off < offset;
			     off += READ_CHUNK_MAX_SIZE) {
				ipuc_sink.write(ipuc_sink.ctx, (uint8_t *)erase_buffer,
				MIN(sizeof(erase_buffer), slot_size - off));
			}
		}

		if (ipuc_sink.seek == NULL ||
		    ipuc_sink.seek(ipuc_sink.ctx, offset) != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Seek operation not supported or failed");
			if (ipuc_sink.release) {
				ipuc_sink.release(ipuc_sink.ctx);
			}
			k_mutex_unlock(&ipuc_mutex);
			return SUIT_PLAT_ERR_IO;
		}

		if (ipuc_sink.write == NULL || ipuc_sink.write(ipuc_sink.ctx, (uint8_t *)buffer,
							       chunk_size) != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Write operation not supported or failed");
			if (ipuc_sink.release) {
				ipuc_sink.release(ipuc_sink.ctx);
			}
			k_mutex_unlock(&ipuc_mutex);
			return SUIT_PLAT_ERR_IO;
		}

		size_t cur_offset = offset + chunk_size;

		if (ipuc_sink.release) {
			ipuc_sink.release(ipuc_sink.ctx);
		}

		if (ipuc_entry->write_peek_offset < cur_offset) {
			ipuc_entry->write_peek_offset = cur_offset;
		}
	}

	if (last_chunk) {
		ipuc_entry->last_chunk_stored = last_chunk;
		LOG_INF("suit_ipuc_sdfw_write, IPUC address: %p, last chunk written",
			(void *)slot_address);
	}

	k_mutex_unlock(&ipuc_mutex);
	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_ipuc_sdfw_digest_compare(struct zcbor_string *component_id,
					      enum suit_cose_alg alg_id,
					      struct zcbor_string *digest)
{
	if (component_id == NULL || digest == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	k_mutex_lock(&ipuc_mutex, K_FOREVER);
	suit_ipuc_entry_t *ipuc_entry = ipuc_entry_from_component_id(component_id);
	intptr_t slot_address = 0;
	size_t slot_size = 0;

	if (ipuc_entry == NULL) {
		k_mutex_unlock(&ipuc_mutex);
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	if (ipuc_entry->usage != IPUC_IPC_IN_PLACE_UPDATE || !ipuc_entry->last_chunk_stored) {
		k_mutex_unlock(&ipuc_mutex);
		return SUIT_PLAT_ERR_INCORRECT_STATE;
	}

	if (ipuc_decode_mem_component_id(component_id, &slot_address, &slot_size) !=
	    SUIT_PLAT_SUCCESS) {
		/* Only MEM components are supported
		 */
		k_mutex_unlock(&ipuc_mutex);
		return SUIT_PLAT_ERR_UNSUPPORTED;
	}

#ifdef CONFIG_SUIT_STREAM_SINK_DIGEST
	psa_algorithm_t psa_alg;

	if (suit_cose_sha512 == alg_id) {
		psa_alg = PSA_ALG_SHA_512;
	} else if (suit_cose_sha256 == alg_id) {
		psa_alg = PSA_ALG_SHA_256;
	} else {
		LOG_ERR("Unsupported hash algorithm: %d", alg_id);
		k_mutex_unlock(&ipuc_mutex);
		return SUIT_PLAT_ERR_UNSUPPORTED;
	}

	struct stream_sink digest_sink;

	if (suit_digest_sink_get(&digest_sink, psa_alg, digest->value) != SUIT_PLAT_SUCCESS) {

		LOG_ERR("Failed to get digest sink");
		k_mutex_unlock(&ipuc_mutex);
		return SUIT_PLAT_ERR_IO;
	}

	if (suit_generic_address_streamer_stream((uint8_t *)slot_address,
						 ipuc_entry->write_peek_offset,
						 &digest_sink) != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to stream to digest sink");
		if (digest_sink.release) {
			digest_sink.release(digest_sink.ctx);
		}
		k_mutex_unlock(&ipuc_mutex);
		return SUIT_PLAT_ERR_IO;
	}

	if (suit_digest_sink_digest_match(digest_sink.ctx) != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Digest does not match");
		if (digest_sink.release) {
			digest_sink.release(digest_sink.ctx);
		}
		k_mutex_unlock(&ipuc_mutex);
		return SUIT_PLAT_ERR_INVAL;
	}

	if (digest_sink.release) {
		digest_sink.release(digest_sink.ctx);
	}

	LOG_INF("suit_ipuc_sdfw_digest_compare success, IPUC address: %p", (void *)slot_address);

	k_mutex_unlock(&ipuc_mutex);
	return SUIT_PLAT_SUCCESS;

#else /* CONFIG_SUIT_STREAM_SINK_DIGEST */
	LOG_ERR("Digest sink not enabled, see CONFIG_SUIT_STREAM_SINK_DIGEST");
	k_mutex_unlock(&ipuc_mutex);
	return SUIT_PLAT_ERR_UNSUPPORTED;

#endif /* CONFIG_SUIT_STREAM_SINK_DIGEST */
}

suit_plat_err_t suit_ipuc_sdfw_revoke(suit_component_t handle)
{

	k_mutex_lock(&ipuc_mutex, K_FOREVER);
	suit_ipuc_entry_t *ipuc_entry = ipuc_entry_from_handle(handle);

	if (ipuc_entry == NULL) {
		k_mutex_unlock(&ipuc_mutex);
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

#if defined(CONFIG_SUIT_LOG_LEVEL_INF) || defined(CONFIG_SUIT_LOG_LEVEL_DBG)
	intptr_t slot_address = 0;
	size_t slot_size = 0;

	if (suit_plat_decode_address_size(&ipuc_entry->component_id, &slot_address, &slot_size) ==
	    SUIT_PLAT_SUCCESS) {
		LOG_INF("Revoking IPUC, address: %p, size: %d bytes", (void *)slot_address,
			slot_size);
	}
#endif

	ipuc_entry->component_id.value = NULL;
	ipuc_entry->component_id.len = 0;
	ipuc_entry->role = SUIT_MANIFEST_UNKNOWN;
	ipuc_entry->usage = IPUC_UNUSED;
	ipuc_entry->ipc_client_id = 0;
	ipuc_entry->write_peek_offset = 0;
	ipuc_entry->last_chunk_stored = false;

	k_mutex_unlock(&ipuc_mutex);
	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_ipuc_sdfw_declare(suit_component_t handle, suit_manifest_role_t role)
{
	suit_ipuc_entry_t *allocated_entry = NULL;
	struct zcbor_string *component_id = ipuc_component_id_from_handle(handle);
	intptr_t slot_address = 0;
	size_t slot_size = 0;

	if (component_id == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (ipuc_decode_mem_component_id(component_id, &slot_address, &slot_size) !=
	    SUIT_PLAT_SUCCESS) {
		/* Only MEM components are supported
		 */
		return SUIT_PLAT_ERR_UNSUPPORTED;
	}

	k_mutex_lock(&ipuc_mutex, K_FOREVER);
	for (size_t i = 0; i < CONFIG_SUIT_IPUC_SIZE; i++) {
		suit_ipuc_entry_t *p_entry = &components[i];

		if (allocated_entry == NULL && p_entry->component_id.value == NULL) {
			allocated_entry = p_entry;

		} else if (suit_compare_zcbor_strings(component_id, &p_entry->component_id)) {
			allocated_entry = p_entry;
			break;
		}
	}

	if (allocated_entry == NULL) {
		k_mutex_unlock(&ipuc_mutex);
		return SUIT_PLAT_ERR_NOMEM;
	}

	LOG_INF("Declaring IPUC, address: %p, size: %d bytes, mfst role: 0x%02X",
		(void *)slot_address, slot_size, role);

	allocated_entry->component_id.value = component_id->value;
	allocated_entry->component_id.len = component_id->len;
	allocated_entry->role = role;
	allocated_entry->usage = IPUC_UNUSED;
	allocated_entry->ipc_client_id = 0;
	allocated_entry->write_peek_offset = 0;
	allocated_entry->last_chunk_stored = false;

	k_mutex_unlock(&ipuc_mutex);
	return SUIT_PLAT_SUCCESS;
}

uintptr_t suit_ipuc_sdfw_mirror_addr(size_t required_size)
{
	uintptr_t sdfw_update_area_addr = 0;
	size_t sdfw_update_area_size = 0;

	suit_memory_sdfw_update_area_info_get(&sdfw_update_area_addr, &sdfw_update_area_size);

	if (required_size == 0 || sdfw_update_area_size == 0) {

		LOG_ERR("suit_ipuc_sdfw_mirror_addr, required_size: %d, sdfw_update_area_size "
			"%d",
			required_size, sdfw_update_area_size);
		return 0;
	}

	k_mutex_lock(&ipuc_mutex, K_FOREVER);
	for (size_t i = 0; i < CONFIG_SUIT_IPUC_SIZE; i++) {

		suit_ipuc_entry_t *ipuc_entry = &components[i];
		struct zcbor_string *component_id = &ipuc_entry->component_id;
		intptr_t slot_address = 0;
		size_t slot_size = 0;

		if (ipuc_entry->usage != IPUC_UNUSED && ipuc_entry->usage != IPUC_SDFW_MIRROR) {
			continue;
		}

		if (ipuc_decode_mem_component_id(component_id, &slot_address, &slot_size) !=
		    SUIT_PLAT_SUCCESS) {
			/* Only MEM components are supported
			 */
			continue;
		}

		if (slot_size < required_size) {
			continue;
		}

		uintptr_t adjusted_slot_address = slot_address;

		/* Adjusting mirror address if IPUC is located in address lower than
		 *  area acceptable by SDROM
		 */
		if (slot_address < sdfw_update_area_addr) {
			adjusted_slot_address = sdfw_update_area_addr;
			size_t shift = (sdfw_update_area_addr - slot_address);

			if (shift + required_size > slot_size) {
				continue;
			}
		}

		if (adjusted_slot_address + required_size >
		    sdfw_update_area_addr + sdfw_update_area_size) {
			continue;
		}

		/* clang-format off */
		struct arbiter_mem_params_access mem_params = {
			.allowed_types = ARBITER_MEM_TYPE(RESERVED, FIXED),
			.access = {
					.owner = NRF_OWNER_APPLICATION,
					.permissions = ARBITER_MEM_PERM(READ, SECURE),
					.address = adjusted_slot_address,
					.size = required_size,
				},
		};
		/* clang-format on */

		LOG_INF("Checking IPUC ownership, address: %p, size: %d bytes",
			(void *)mem_params.access.address, mem_params.access.size);

		if (arbiter_mem_access_check(&mem_params) == ARBITER_STATUS_OK) {
			/* ... belonging to app domain
			 */
#if CONFIG_SUIT_DIGEST_CACHE
			suit_plat_digest_cache_remove(component_id);
#endif /* CONFIG_SUIT_DIGEST_CACHE */
			ipuc_entry->usage = IPUC_SDFW_MIRROR;
			k_mutex_unlock(&ipuc_mutex);
			return adjusted_slot_address;
		}

		mem_params.access.owner = NRF_OWNER_RADIOCORE;
		if (arbiter_mem_access_check(&mem_params) == ARBITER_STATUS_OK) {
			/* ... or belonging to radio domain
			 */
#if CONFIG_SUIT_DIGEST_CACHE
			suit_plat_digest_cache_remove(component_id);
#endif /* CONFIG_SUIT_DIGEST_CACHE */
			ipuc_entry->usage = IPUC_SDFW_MIRROR;
			k_mutex_unlock(&ipuc_mutex);
			return adjusted_slot_address;
		}
	}

	k_mutex_unlock(&ipuc_mutex);
	return 0;
}
