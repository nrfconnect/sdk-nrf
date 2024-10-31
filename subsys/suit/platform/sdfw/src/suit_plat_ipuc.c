/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "suit_plat_ipuc.h"
#include <suit_platform_internal.h>
#include <suit_memory_layout.h>
#include <suit_plat_decode_util.h>
#include <suit_sink_selector.h>
#include <suit_generic_address_streamer.h>
#include <sdfw/arbiter.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_ipuc, CONFIG_SUIT_LOG_LEVEL);

typedef struct {
	struct zcbor_string component_id;
	size_t write_offset;
	bool last_chunk_stored;

} suit_plat_ipuc_entry_t;

static suit_plat_ipuc_entry_t components[CONFIG_SUIT_IPUC_SIZE];
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

static suit_plat_ipuc_entry_t *ipuc_entry_from_handle(suit_component_t handle)
{
	struct zcbor_string *component_id = ipuc_component_id_from_handle(handle);

	if (component_id == NULL) {
		return NULL;
	}

	for (size_t i = 0; i < CONFIG_SUIT_IPUC_SIZE; i++) {
		suit_plat_ipuc_entry_t *p_entry = &components[i];

		if (suit_compare_zcbor_strings(component_id, &p_entry->component_id)) {
			return p_entry;
		}
	}

	return NULL;
}

suit_plat_err_t suit_plat_ipuc_write(suit_component_t handle, size_t offset, uintptr_t buffer,
				     size_t chunk_size, bool last_chunk)
{
	suit_plat_err_t plat_ret = SUIT_PLAT_SUCCESS;
	struct stream_sink ipuc_sink;
	bool sink_allocated = false;

	k_mutex_lock(&ipuc_mutex, K_FOREVER);
	suit_plat_ipuc_entry_t *ipuc_entry = ipuc_entry_from_handle(handle);

	if (ipuc_entry == NULL) {
		plat_ret = SUIT_PLAT_ERR_NOT_FOUND;
	}

	if (plat_ret == SUIT_PLAT_SUCCESS) {

		if (offset == 0) {
			ipuc_entry->write_offset = 0;
			ipuc_entry->last_chunk_stored = false;
		}

		if (offset != ipuc_entry->write_offset || ipuc_entry->last_chunk_stored) {
			plat_ret = SUIT_PLAT_ERR_INCORRECT_STATE;
		}
	}

	if (plat_ret == SUIT_PLAT_SUCCESS) {
		plat_ret = suit_sink_select(handle, &ipuc_sink);
		if (plat_ret != SUIT_PLAT_SUCCESS) {
			plat_ret = SUIT_PLAT_ERR_IO;
		} else {
			sink_allocated = true;
		}
	}

	if (plat_ret == SUIT_PLAT_SUCCESS) {
		if (ipuc_sink.seek == NULL) {
			plat_ret = SUIT_PLAT_ERR_IO;
		}
	}

	if (plat_ret == SUIT_PLAT_SUCCESS) {
		if (offset == 0 && ipuc_sink.erase != NULL) {
			ipuc_sink.erase(ipuc_sink.ctx);
		}

		plat_ret = ipuc_sink.seek(ipuc_sink.ctx, offset);
		if (plat_ret != SUIT_PLAT_SUCCESS) {
			plat_ret = SUIT_PLAT_ERR_IO;
		}
	}

	if (plat_ret == SUIT_PLAT_SUCCESS && chunk_size > 0) {
		plat_ret = suit_generic_address_streamer_stream((uint8_t *)buffer, chunk_size,
								&ipuc_sink);
		if (plat_ret != SUIT_PLAT_SUCCESS) {
			plat_ret = SUIT_PLAT_ERR_IO;
		}
	}

	if (plat_ret == SUIT_PLAT_SUCCESS) {
		ipuc_entry->write_offset = offset + chunk_size;
		if (last_chunk) {
			ipuc_entry->last_chunk_stored = true;
		}
	}

	if (sink_allocated) {
		if (ipuc_sink.release != NULL) {
			ipuc_sink.release(ipuc_sink.ctx);
		}
	}

	k_mutex_unlock(&ipuc_mutex);
	return plat_ret;
}

suit_plat_err_t suit_plat_ipuc_get_stored_img_size(suit_component_t handle, size_t *size)
{
	if (size == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	k_mutex_lock(&ipuc_mutex, K_FOREVER);
	suit_plat_ipuc_entry_t *ipuc_entry = ipuc_entry_from_handle(handle);

	if (ipuc_entry == NULL) {
		k_mutex_unlock(&ipuc_mutex);
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

	if (ipuc_entry->last_chunk_stored == false) {
		k_mutex_unlock(&ipuc_mutex);
		return SUIT_PLAT_ERR_INCORRECT_STATE;
	}

	*size = ipuc_entry->write_offset;
	k_mutex_unlock(&ipuc_mutex);
	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_plat_ipuc_revoke(suit_component_t handle)
{

	k_mutex_lock(&ipuc_mutex, K_FOREVER);
	suit_plat_ipuc_entry_t *ipuc_entry = ipuc_entry_from_handle(handle);

	if (ipuc_entry == NULL) {
		k_mutex_unlock(&ipuc_mutex);
		return SUIT_PLAT_ERR_NOT_FOUND;
	}

#ifdef CONFIG_SUIT_LOG_LEVEL_INF
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
	ipuc_entry->write_offset = 0;
	ipuc_entry->last_chunk_stored = false;

	k_mutex_unlock(&ipuc_mutex);
	return SUIT_PLAT_SUCCESS;
}

suit_plat_err_t suit_plat_ipuc_declare(suit_component_t handle)
{
	suit_plat_ipuc_entry_t *allocated_entry = NULL;
	struct zcbor_string *component_id = ipuc_component_id_from_handle(handle);

	if (component_id == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	k_mutex_lock(&ipuc_mutex, K_FOREVER);
	for (size_t i = 0; i < CONFIG_SUIT_IPUC_SIZE; i++) {
		suit_plat_ipuc_entry_t *p_entry = &components[i];

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

#ifdef CONFIG_SUIT_LOG_LEVEL_INF
	intptr_t slot_address = 0;
	size_t slot_size = 0;

	if (suit_plat_decode_address_size(component_id, &slot_address, &slot_size) ==
	    SUIT_PLAT_SUCCESS) {
		LOG_INF("Declaring IPUC, address: %p, size: %d bytes", (void *)slot_address,
			slot_size);
	}
#endif

	allocated_entry->component_id.value = component_id->value;
	allocated_entry->component_id.len = component_id->len;
	allocated_entry->write_offset = 0;
	allocated_entry->last_chunk_stored = false;

	k_mutex_unlock(&ipuc_mutex);
	return SUIT_PLAT_SUCCESS;
}

uintptr_t suit_plat_ipuc_sdfw_mirror_addr(size_t required_size)
{
	uintptr_t sdfw_update_area_addr = 0;
	size_t sdfw_update_area_size = 0;

	suit_memory_sdfw_update_area_info_get(&sdfw_update_area_addr, &sdfw_update_area_size);

	if (required_size == 0 || sdfw_update_area_size == 0) {

		LOG_ERR("suit_plat_ipuc_sdfw_mirror_addr, required_size: %d, sdfw_update_area_size "
			"%d",
			required_size, sdfw_update_area_size);
		return 0;
	}

	k_mutex_lock(&ipuc_mutex, K_FOREVER);
	for (size_t i = 0; i < CONFIG_SUIT_IPUC_SIZE; i++) {

		struct zcbor_string *component_id = &components[i].component_id;

		if (component_id->value == NULL) {
			continue;
		}

		suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

		intptr_t slot_address = 0;
		size_t slot_size = 0;

		if (suit_plat_decode_component_type(component_id, &component_type) !=
			    SUIT_PLAT_SUCCESS ||
		    component_type != SUIT_COMPONENT_TYPE_MEM) {
			continue;
		}

		if (suit_plat_decode_address_size(component_id, &slot_address, &slot_size) !=
		    SUIT_PLAT_SUCCESS) {
			continue;
		}

		if (slot_address == 0 || slot_size < required_size) {
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
			k_mutex_unlock(&ipuc_mutex);
			return adjusted_slot_address;
		}

		mem_params.access.owner = NRF_OWNER_RADIOCORE;
		if (arbiter_mem_access_check(&mem_params) == ARBITER_STATUS_OK) {
			/* ... or belonging to radio domain
			 */
			k_mutex_unlock(&ipuc_mutex);
			return adjusted_slot_address;
		}
	}

	k_mutex_unlock(&ipuc_mutex);
	return 0;
}
