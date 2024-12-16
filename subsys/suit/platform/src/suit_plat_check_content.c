/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform.h>
#include <suit_plat_check_content_domain_specific.h>
#include <suit_plat_decode_util.h>
#ifdef CONFIG_SUIT_MANIFEST_VARIABLES
#include <suit_manifest_variables.h>
#endif /* CONFIG_SUIT_MANIFEST_VARIABLES */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(suit_plat_check_content, CONFIG_SUIT_LOG_LEVEL);

#ifdef CONFIG_SUIT_MANIFEST_VARIABLES
static int suit_plat_check_content_mfst_var(suit_component_t component,
					    struct zcbor_string *content)
{
	struct zcbor_string *component_id = NULL;
	suit_plat_err_t plat_ret;
	uint32_t checked_val = 0;
	uint32_t current_val = 0;
	uint32_t id;
	int ret;

	plat_ret = suit_plat_decode_content_uint32(content, &checked_val);
	if (plat_ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to decode content value: %d", plat_ret);
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}

	ret = suit_plat_component_id_get(component, &component_id);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to get component ID: %d", ret);
		return SUIT_ERR_CRASH;
	}

	plat_ret = suit_plat_decode_component_number(component_id, &id);
	if (plat_ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to decode variable ID: %d", plat_ret);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	plat_ret = suit_mfst_var_get(id, &current_val);
	switch (plat_ret) {
	case SUIT_PLAT_SUCCESS:
		if (checked_val == current_val) {
			return SUIT_SUCCESS;
		}
		return SUIT_FAIL_CONDITION;
	case SUIT_PLAT_ERR_NOT_FOUND:
		LOG_ERR("Failed to get manifest variable: ID %d not supported", id);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	case SUIT_PLAT_ERR_INVAL:
	case SUIT_PLAT_ERR_IO:
	default:
		break;
	}

	LOG_ERR("Failed to get manifest variable: %d", plat_ret);

	return SUIT_ERR_CRASH;
}
#endif /* CONFIG_SUIT_MANIFEST_VARIABLES */

int suit_plat_check_content(suit_component_t component, struct zcbor_string *content)
{
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	int ret = SUIT_SUCCESS;

	if (content == NULL) {
		return SUIT_ERR_CRASH;
	}

	/* Get destination component type based on component handle. */
	ret = suit_plat_component_type_get(component, &dst_component_type);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to decode destination component type");
		return ret;
	}

#ifdef CONFIG_SUIT_MANIFEST_VARIABLES
	if (dst_component_type == SUIT_COMPONENT_TYPE_MFST_VAR) {
		return suit_plat_check_content_mfst_var(component, content);
	}
#endif /* CONFIG_SUIT_MANIFEST_VARIABLES */
	if (suit_plat_check_content_domain_specific_is_type_supported(dst_component_type)) {
		return suit_plat_check_content_domain_specific(component, dst_component_type,
							       content);
	}

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}
