/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "suit_plat_ipuc.h"
#include <suit_platform_internal.h>
#include <suit_memory_layout.h>
#include <suit_plat_decode_util.h>
#include <sdfw/arbiter.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_ipuc, CONFIG_SUIT_LOG_LEVEL);

typedef struct {
	struct zcbor_string component_id;
	size_t write_offset;

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

int suit_plat_ipuc_revoke(suit_component_t handle)
{
	struct zcbor_string *component_id = ipuc_component_id_from_handle(handle);

	if (component_id == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	k_mutex_lock(&ipuc_mutex, K_FOREVER);
	for (size_t i = 0; i < CONFIG_SUIT_IPUC_SIZE; i++) {
		suit_plat_ipuc_entry_t *p_entry = &components[i];

		if (suit_compare_zcbor_strings(component_id, &p_entry->component_id)) {

#ifdef CONFIG_SUIT_LOG_LEVEL_INF
			intptr_t slot_address = 0;
			size_t slot_size = 0;

			if (suit_plat_decode_address_size(&p_entry->component_id, &slot_address,
							  &slot_size) == SUIT_PLAT_SUCCESS) {
				LOG_INF("Revoking IPUC, address: %p, size: %d bytes",
					(void *)slot_address, slot_size);
			}
#endif
			p_entry->component_id.value = NULL;
			p_entry->component_id.len = 0;
			p_entry->write_offset = 0;
			break;
		}
	}

	k_mutex_unlock(&ipuc_mutex);

	return SUIT_SUCCESS;
}

int suit_plat_ipuc_declare(suit_component_t handle)
{
	suit_plat_ipuc_entry_t *allocated_entry = NULL;
	struct zcbor_string *component_id = ipuc_component_id_from_handle(handle);

	if (component_id == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
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
		return SUIT_ERR_OVERFLOW;
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

	k_mutex_unlock(&ipuc_mutex);
	return SUIT_SUCCESS;
}

struct zcbor_string *suit_plat_find_sdfw_mirror_ipuc(size_t min_size)
{
	struct zcbor_string *component_id = NULL;

	k_mutex_lock(&ipuc_mutex, K_FOREVER);
	for (size_t i = 0; i < CONFIG_SUIT_IPUC_SIZE; i++) {

		component_id = &components[i].component_id;

		if (component_id->value != NULL) {

			suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

			intptr_t slot_address = 0;
			size_t slot_size = 0;

			if (suit_plat_decode_component_type(component_id, &component_type) !=
				    SUIT_PLAT_SUCCESS ||
			    component_type != SUIT_COMPONENT_TYPE_MEM) {
				continue;
			}

			if (suit_plat_decode_address_size(component_id, &slot_address,
							  &slot_size) != SUIT_PLAT_SUCCESS) {
				continue;
			}

			if (slot_address == 0 || slot_size < min_size) {
				continue;
			}

			if (suit_memory_global_address_range_is_in_nvm(slot_address, slot_size) ==
			    false) {
				/* looking for MRAM
				 */
				continue;
			}

			/* clang-format off */
			struct arbiter_mem_params_access mem_params = {
				.allowed_types = ARBITER_MEM_TYPE(RESERVED, FIXED),
				.access = {
						.owner = NRF_OWNER_APPLICATION,
						.permissions = ARBITER_MEM_PERM(READ),
						.address = slot_address,
						.size = slot_size,
					},
			};
			/* clang-format on */

			if (arbiter_mem_access_check(&mem_params) == ARBITER_STATUS_OK) {
				/* ... belonging to app domain
				 */
				k_mutex_unlock(&ipuc_mutex);
				return component_id;
			}

			mem_params.access.owner = NRF_OWNER_RADIOCORE;
			if (arbiter_mem_access_check(&mem_params) == ARBITER_STATUS_OK) {
				/* ... or belonging to radio domain
				 */
				k_mutex_unlock(&ipuc_mutex);
				return component_id;
			}
		}
	}

	k_mutex_unlock(&ipuc_mutex);
	return NULL;
}
