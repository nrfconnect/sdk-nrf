/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_plat_version_domain_specific.h>
#include <suit_plat_error_convert.h>
#include <suit_plat_decode_util.h>
#include <suit.h>

#ifdef CONFIG_SUIT_STORAGE
#include <suit_storage.h>
#endif /* CONFIG_SUIT_STORAGE */

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_plat_version_sdfw, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_version_domain_specific(struct zcbor_string *component_id,
				      suit_component_type_t component_type, int *version,
				      size_t *version_len)
{
	int ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;

#ifdef CONFIG_SUIT_STORAGE
	switch (component_type) {
	case SUIT_COMPONENT_TYPE_INSTLD_MFST: {
		suit_manifest_class_id_t *class_id = NULL;
		const uint8_t *envelope_str = NULL;
		size_t envelope_len = 0;

		if (suit_plat_decode_manifest_class_id(component_id, &class_id) !=
		    SUIT_PLAT_SUCCESS) {
			LOG_ERR("Unable to decode manifest class ID");
			ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
			break;
		}

		ret = suit_storage_installed_envelope_get(class_id, &envelope_str, &envelope_len);
		if ((ret != SUIT_PLAT_SUCCESS) || (envelope_str == NULL) || (envelope_len == 0)) {
			LOG_ERR("Unable to find installed envelope (err: %d)", ret);
			ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
			break;
		}

		ret = suit_processor_get_manifest_metadata(envelope_str, envelope_len, false, NULL,
							   version, version_len, NULL, NULL, NULL);
		if (ret != SUIT_SUCCESS) {
			LOG_ERR("Unable to read manifest version (err: %d)", ret);
			return SUIT_ERR_UNSUPPORTED_PARAMETER;
		}
	} break;
	default: {
		break;
	}
	}
#endif /* CONFIG_SUIT_STORAGE */

	return ret;
}
