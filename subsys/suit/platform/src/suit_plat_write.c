/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <stdbool.h>
#include <suit_platform.h>
#include <suit_plat_decode_util.h>
#include <suit_plat_error_convert.h>
#include <suit_platform_internal.h>
#include <suit_plat_write_domain_specific.h>
#ifdef CONFIG_SUIT_MANIFEST_VARIABLES
#include <suit_manifest_variables.h>
#endif /* CONFIG_SUIT_MANIFEST_VARIABLES */

LOG_MODULE_REGISTER(suit_plat_write, CONFIG_SUIT_LOG_LEVEL);

#ifdef CONFIG_SUIT_MANIFEST_VARIABLES
static int suit_plat_check_write_mfst_var(suit_component_t dst_handle, struct zcbor_string *content,
					  struct zcbor_string *manifest_component_id,
					  struct suit_encryption_info *enc_info)
{
	struct zcbor_string *component_id = NULL;
	suit_plat_err_t plat_ret;
	uint32_t id;
	uint32_t val;
	int ret;

	if (enc_info != NULL) {
		LOG_ERR("Encryption not supported for MFST_VAR components");
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}

	plat_ret = suit_plat_decode_content_uint32(content, &val);
	if (plat_ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to decode content value: %d", plat_ret);
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}

	ret = suit_plat_component_id_get(dst_handle, &component_id);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to get component ID: %d", ret);
		return SUIT_ERR_CRASH;
	}

	plat_ret = suit_plat_decode_component_number(component_id, &id);
	if (plat_ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to decode variable ID: %d", plat_ret);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

#ifdef CONFIG_SUIT_PLATFORM_VARIANT_SDFW
	ret = suit_plat_authorize_var_rw_access(manifest_component_id, id);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Write access not allowed for variable %d: %d", id, ret);
		return SUIT_ERR_UNAUTHORIZED_COMPONENT;
	}
#endif /* CONFIG_SUIT_PLATFORM_VARIANT_SDFW */

	plat_ret = suit_mfst_var_get(id, &val);
	switch (plat_ret) {
	case SUIT_PLAT_SUCCESS:
		return SUIT_SUCCESS;
	case SUIT_PLAT_ERR_NOT_FOUND:
		LOG_ERR("Failed to get manifest variable: ID %d not supported", id);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	case SUIT_PLAT_ERR_INVAL:
	case SUIT_PLAT_ERR_IO:
	default:
		break;
	}

	LOG_ERR("Failed to verify setting manifest variable: %d", plat_ret);

	return SUIT_ERR_CRASH;
}

static int suit_plat_write_mfst_var(suit_component_t dst_handle, struct zcbor_string *content,
				    struct zcbor_string *manifest_component_id,
				    struct suit_encryption_info *enc_info)
{
	struct zcbor_string *component_id = NULL;
	suit_plat_err_t plat_ret;
	uint32_t id;
	uint32_t val;
	int ret;

	/* Encryption is not supported for MFST_VAR components. */
	if (enc_info != NULL) {
		LOG_ERR("Encryption not supported for MFST_VAR components");
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}

	plat_ret = suit_plat_decode_content_uint32(content, &val);
	if (plat_ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to decode content value: %d", plat_ret);
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}

	ret = suit_plat_component_id_get(dst_handle, &component_id);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to get component ID: %d", ret);
		return SUIT_ERR_CRASH;
	}

	plat_ret = suit_plat_decode_component_number(component_id, &id);
	if (plat_ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to decode variable ID: %d", plat_ret);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

#ifdef CONFIG_SUIT_PLATFORM_VARIANT_SDFW
	ret = suit_plat_authorize_var_rw_access(manifest_component_id, id);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Write access not allowed for variable %d: %d", id, ret);
		return SUIT_ERR_UNAUTHORIZED_COMPONENT;
	}
#endif /* CONFIG_SUIT_PLATFORM_VARIANT_SDFW */

	plat_ret = suit_mfst_var_set(id, val);
	switch (plat_ret) {
	case SUIT_PLAT_SUCCESS:
		return SUIT_SUCCESS;
	case SUIT_PLAT_ERR_NOT_FOUND:
		LOG_ERR("Failed to set manifest variable: ID %d not supported", id);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	case SUIT_PLAT_ERR_SIZE:
		LOG_ERR("Failed to set manifest variable %d: Value 0x%x not compatible", id, val);
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	case SUIT_PLAT_ERR_IO:
	default:
		break;
	}

	LOG_ERR("Failed to set manifest variable: %d", plat_ret);

	return SUIT_ERR_CRASH;
}
#endif /* CONFIG_SUIT_MANIFEST_VARIABLES */

int suit_plat_check_write(suit_component_t dst_handle, struct zcbor_string *content,
			  struct zcbor_string *manifest_component_id,
			  struct suit_encryption_info *enc_info)
{
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	int ret = SUIT_SUCCESS;

	if (content == NULL) {
		return suit_plat_err_to_processor_err_convert(SUIT_PLAT_ERR_INVAL);
	}

	/* Get destination component type based on component handle. */
	ret = suit_plat_component_type_get(dst_handle, &dst_component_type);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to decode destination component type");
		return ret;
	}

#ifdef CONFIG_SUIT_MANIFEST_VARIABLES
	if (dst_component_type == SUIT_COMPONENT_TYPE_MFST_VAR) {
		return suit_plat_check_write_mfst_var(dst_handle, content, manifest_component_id,
						      enc_info);
	}
#endif /* CONFIG_SUIT_MANIFEST_VARIABLES */
	if (suit_plat_write_domain_specific_is_type_supported(dst_component_type)) {
		return suit_plat_check_write_domain_specific(
			dst_handle, dst_component_type, content, manifest_component_id, enc_info);
	}

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

int suit_plat_write(suit_component_t dst_handle, struct zcbor_string *content,
		    struct zcbor_string *manifest_component_id,
		    struct suit_encryption_info *enc_info)
{
	suit_component_type_t dst_component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	int ret = SUIT_SUCCESS;

	if (content == NULL) {
		return suit_plat_err_to_processor_err_convert(SUIT_PLAT_ERR_INVAL);
	}

	/* Get destination component type based on component handle. */
	ret = suit_plat_component_type_get(dst_handle, &dst_component_type);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Failed to decode destination component type");
		return ret;
	}

#ifdef CONFIG_SUIT_MANIFEST_VARIABLES
	if (dst_component_type == SUIT_COMPONENT_TYPE_MFST_VAR) {
		return suit_plat_write_mfst_var(dst_handle, content, manifest_component_id,
						enc_info);
	}
#endif /* CONFIG_SUIT_MANIFEST_VARIABLES */
	if (suit_plat_write_domain_specific_is_type_supported(dst_component_type)) {
		return suit_plat_write_domain_specific(dst_handle, dst_component_type, content,
						       manifest_component_id, enc_info);
	}

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}
