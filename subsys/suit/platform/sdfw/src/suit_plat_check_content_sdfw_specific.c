/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform.h>
#include <suit_platform_internal.h>
#include <suit_plat_decode_util.h>
#include <suit_memptr_storage.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(suit_plat_check_content, CONFIG_SUIT_LOG_LEVEL);

static int suit_plat_check_content_mem_mapped(suit_component_t component,
					      struct zcbor_string *content)
{
	void *impl_data = NULL;
	int err = suit_plat_component_impl_data_get(component, &impl_data);

	if (err != SUIT_SUCCESS) {
		LOG_ERR("Failed to get implementation data: %d", err);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	const uint8_t *data = NULL;
	size_t size = 0;
	const uint8_t *content_data = content->value;

	err = suit_memptr_storage_ptr_get((memptr_storage_handle_t)impl_data, &data, &size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to get memptr ptr: %d", err);
		return SUIT_ERR_CRASH;
	}

	if (size != content->len) {
		return SUIT_FAIL_CONDITION;
	}

	uint8_t residual = 0;

	for (size_t i = 0; i < size; i++) {
		residual |= content_data[i] ^ data[i];
	}

	if (residual != 0) {
		return SUIT_FAIL_CONDITION;
	}

	return SUIT_SUCCESS;
}

bool suit_plat_check_content_domain_specific_is_type_supported(suit_component_type_t component_type)
{
	switch (component_type) {
	case SUIT_COMPONENT_TYPE_MEM:
		return true;
	default:
		break;
	}

	return false;
}

int suit_plat_check_content_domain_specific(suit_component_t component,
					    suit_component_type_t component_type,
					    struct zcbor_string *content)
{
	int err = SUIT_ERR_CRASH;

	switch (component_type) {
	case SUIT_COMPONENT_TYPE_UNSUPPORTED: {
		LOG_ERR("Unsupported component type");
		err = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		break;
	}
	case SUIT_COMPONENT_TYPE_MEM: {
		err = suit_plat_check_content_mem_mapped(component, content);
		break;
	}
	default: {
		LOG_ERR("Unhandled component type: %d", component_type);
		err = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		break;
	}
	}

	return err;
}
