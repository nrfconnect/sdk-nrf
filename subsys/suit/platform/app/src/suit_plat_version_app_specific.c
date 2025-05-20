/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_plat_version_domain_specific.h>
#ifdef CONFIG_SSF_SUIT_SERVICE_ENABLED
#include <sdfw/sdfw_services/suit_service.h>
#endif /* CONFIG_SSF_SUIT_SERVICE_ENABLED */
#include <suit_plat_decode_util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(suit_plat_version_app, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_version_domain_specific(struct zcbor_string *component_id,
				      suit_component_type_t component_type, int *version,
				      size_t *version_len)
{
	int ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;

	switch (component_type) {
#ifdef CONFIG_SSF_SUIT_SERVICE_ENABLED
	case SUIT_COMPONENT_TYPE_INSTLD_MFST: {
		suit_manifest_class_id_t *class_id = NULL;
		suit_semver_raw_t raw_version = {0};

		if (suit_plat_decode_manifest_class_id(component_id, &class_id) !=
		    SUIT_PLAT_SUCCESS) {
			LOG_ERR("Unable to decode manifest class ID");
			ret = SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
			break;
		}

		suit_ssf_err_t err = suit_get_installed_manifest_info(class_id, NULL, &raw_version,
								      NULL, NULL, NULL);

		if ((err != SUIT_SUCCESS) || (raw_version.len > *version_len)) {
			LOG_ERR("Unable to read manifest version (err: %d)", err);
			return SUIT_ERR_UNSUPPORTED_PARAMETER;
		}

		memset(version, 0, sizeof(*version) * (*version_len));

		for (size_t i = 0; i < raw_version.len; i++) {
			version[i] = raw_version.raw[i];
		}
		*version_len = raw_version.len;

		ret = SUIT_SUCCESS;
	} break;
#endif /* CONFIG_SSF_SUIT_SERVICE_ENABLED */
	default: {
		break;
	}
	}

	return ret;
}
