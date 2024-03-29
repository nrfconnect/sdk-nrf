/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_plat_retrieve_manifest_domain_specific.h>
#include <suit_plat_error_convert.h>
#include <suit_plat_decode_util.h>

#ifdef CONFIG_SUIT_STORAGE
#include <suit_storage.h>
#endif /* CONFIG_SUIT_STORAGE */

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_plat_retr_mfst_sdfw, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_retrieve_manifest_domain_specific(struct zcbor_string *component_id,
						suit_component_type_t component_type,
						const uint8_t **envelope_str, size_t *envelope_len)
{
	int ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;

#ifdef CONFIG_SUIT_STORAGE
	switch (component_type) {
	case SUIT_COMPONENT_TYPE_INSTLD_MFST: {
		suit_manifest_class_id_t *class_id;

		if (suit_plat_decode_manifest_class_id(component_id, &class_id)
		    != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Unable to decode manifest class ID");
			ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
			break;
		}

		ret = suit_storage_installed_envelope_get(class_id, envelope_str, envelope_len);
		if ((ret != SUIT_PLAT_SUCCESS) || (*envelope_str == NULL) || (*envelope_len == 0)) {
			LOG_ERR("Unable to find installed envelope (err: %d)", ret);
			ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
			break;
		}

		ret = SUIT_SUCCESS;
	} break;
	default: {
		break;
	}
	}
#endif /* CONFIG_SUIT_STORAGE */

	return ret;
}
