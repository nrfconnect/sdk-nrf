/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform_internal.h>
#include <suit_plat_decode_util.h>
#include <suit_plat_error_convert.h>
#include <suit_memptr_storage.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_plat_memptr_update_size, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_memptr_size_update(suit_component_t handle, size_t size)
{
	struct zcbor_string *component_id = NULL;
	intptr_t run_address;
	size_t component_size;
	void *impl_data = NULL;
	const uint8_t *payload_ptr = NULL;
	size_t payload_size = 0;

	int err = suit_plat_component_id_get(handle, &component_id);

	if (err != SUIT_SUCCESS) {
		LOG_ERR("Failed to get component id: %i", err);
		return err;
	}

	if (suit_plat_decode_address_size(component_id, &run_address, &component_size) !=
	    SUIT_PLAT_SUCCESS) {
		LOG_ERR("suit_plat_decode_address_size failed");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (size > component_size) {
		LOG_ERR("Input size exceeds component size");
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}

	err = suit_plat_component_impl_data_get(handle, &impl_data);
	if (err != SUIT_SUCCESS) {
		LOG_ERR("Failed to get implementation data: %i", err);
		return err;
	}

	err = suit_memptr_storage_ptr_get(impl_data, &payload_ptr, &payload_size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to get memptr: %i", err);
		return SUIT_ERR_CRASH;
	}

	payload_size = size;
	err = suit_memptr_storage_ptr_store(impl_data, payload_ptr, payload_size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to update memptr: %i", err);
		return SUIT_ERR_CRASH;
	}

	return SUIT_SUCCESS;
}
