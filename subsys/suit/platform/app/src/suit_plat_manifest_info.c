/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform.h>
#include <suit_plat_error_convert.h>
#include <suit_plat_decode_util.h>
#include <suit_metadata.h>
#include <sdfw/sdfw_services/suit_service.h>

static int supported_manifest_class_infos_get(const suit_ssf_manifest_class_info_t **class_info,
					      size_t *out_size)
{
	static suit_ssf_manifest_class_info_t
		manifest_class_infos_list[CONFIG_MAX_NUMBER_OF_MANIFEST_CLASS_IDS] = {0};
	static size_t size = CONFIG_MAX_NUMBER_OF_MANIFEST_CLASS_IDS;
	static bool initialized;

	suit_manifest_role_t manifest_roles_list[CONFIG_MAX_NUMBER_OF_MANIFEST_CLASS_IDS] = {0};

	if (!initialized) {
		suit_ssf_err_t ret = suit_get_supported_manifest_roles(manifest_roles_list, &size);

		if (ret != SUIT_PLAT_SUCCESS) {
			return suit_plat_err_to_processor_err_convert(ret);
		}

		for (size_t i = 0; i < size; i++) {
			ret = suit_get_supported_manifest_info(manifest_roles_list[i],
							       &manifest_class_infos_list[i]);

			if (ret != SUIT_PLAT_SUCCESS) {
				return suit_plat_err_to_processor_err_convert(ret);
			}
		}

		initialized = true;
	}

	if (NULL == class_info || NULL == out_size) {
		return SUIT_ERR_CRASH;
	}

	if (*out_size < size) {
		return SUIT_ERR_CRASH;
	}

	*class_info = manifest_class_infos_list;

	*out_size = size;

	return SUIT_SUCCESS;
}

int suit_plat_supported_manifest_class_infos_get(suit_manifest_class_info_t *class_info,
						 size_t *size)
{
	const suit_ssf_manifest_class_info_t *manifest_class_infos_list = NULL;

	if (NULL == class_info || NULL == size) {
		return SUIT_ERR_CRASH;
	}

	int ret = supported_manifest_class_infos_get(&manifest_class_infos_list, size);

	if (ret != SUIT_SUCCESS) {
		return ret;
	}

	for (size_t i = 0; i < *size; i++) {
		class_info[i].role = manifest_class_infos_list[i].role;
		class_info[i].class_id = &manifest_class_infos_list[i].class_id;
		class_info[i].vendor_id = &manifest_class_infos_list[i].vendor_id;
	}

	return SUIT_SUCCESS;
}

int suit_plat_manifest_role_get(const suit_manifest_class_id_t *class_id,
				suit_manifest_role_t *role)
{
	const suit_ssf_manifest_class_info_t *manifest_class_infos_list = NULL;
	size_t size = CONFIG_MAX_NUMBER_OF_MANIFEST_CLASS_IDS;

	if (role == NULL) {
		return SUIT_ERR_CRASH;
	}

	int ret = supported_manifest_class_infos_get(&manifest_class_infos_list, &size);

	if (ret != SUIT_PLAT_SUCCESS) {
		return ret;
	}

	for (size_t i = 0; i < size; i++) {
		if (suit_metadata_uuid_compare(class_id, &manifest_class_infos_list[i].class_id) ==
		    SUIT_PLAT_SUCCESS) {
			*role = manifest_class_infos_list[i].role;
			return SUIT_SUCCESS;
		}
	}

	*role = SUIT_MANIFEST_UNKNOWN;

	return SUIT_ERR_CRASH;
}
