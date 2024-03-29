/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform_internal.h>
#include <zephyr/logging/log.h>
#include <suit_plat_decode_util.h>
#include <suit_plat_retrieve_manifest_domain_specific.h>
#include <suit_memory_layout.h>

#ifdef CONFIG_SUIT_MEMPTR_STORAGE
#include <suit_memptr_storage.h>
#endif /* CONFIG_SUIT_MEMPTR_STORAGE */

LOG_MODULE_REGISTER(suit_plat_retr_mfst, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_retrieve_manifest(suit_component_t component_handle, const uint8_t **envelope_str,
				size_t *envelope_len)
{
	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	int ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	struct zcbor_string *component_id;

	if ((envelope_str == NULL) || (envelope_len == NULL)) {
		return SUIT_ERR_CRASH;
	}

	/* Verify that the component ID has the correct format. */
	if (suit_plat_component_id_get(component_handle, &component_id) != SUIT_SUCCESS) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (suit_plat_decode_component_type(component_id, &component_type) != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Error decoding component type");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	switch (component_type) {
#ifdef CONFIG_SUIT_MEMPTR_STORAGE
	case SUIT_COMPONENT_TYPE_CAND_MFST: {
		memptr_storage_handle_t handle = NULL;

		ret = suit_plat_component_impl_data_get(component_handle, &handle);
		if (ret != SUIT_SUCCESS) {
			LOG_ERR("Unable to get data for manifest candidate (err: %d)", ret);
			ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
			break;
		}

		ret = suit_memptr_storage_ptr_get(handle, envelope_str, envelope_len);
		if ((ret != SUIT_PLAT_SUCCESS) || (*envelope_str == NULL) || (*envelope_len == 0)) {
			LOG_ERR("Unable to fetch pointer to manifest candidate"
				"(memptr storage err: %d)",
				ret);
			ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
			break;
		}

		if (suit_memory_global_address_is_in_external_memory((uintptr_t)*envelope_str) ==
		    true) {
			LOG_ERR("Manifest candidate is in external memory - this is not supported");
			ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
			break;
		}

		ret = SUIT_SUCCESS;
	} break;
#endif /* CONFIG_SUIT_MEMPTR_STORAGE */
	default:
		ret = suit_plat_retrieve_manifest_domain_specific(component_id, component_type,
								  envelope_str, envelope_len);
		break;
	}

	return ret;
}
