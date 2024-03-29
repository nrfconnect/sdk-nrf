/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform.h>
#include <suit_plat_decode_util.h>
#include <suit_plat_component_compatibility.h>
#include <suit_plat_manifest_info_internal.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(suit_plat_authenticate, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_authorize_component_id(struct zcbor_string *manifest_component_id,
				     struct zcbor_string *component_id)
{
	suit_manifest_class_id_t *class_id = NULL;

	if ((manifest_component_id == NULL) || (component_id == NULL) ||
	    (manifest_component_id->value == NULL) || (manifest_component_id->len == 0) ||
	    (component_id->value == NULL) || (component_id->len == 0)) {
		return SUIT_ERR_DECODING;
	}

	/* Check if component ID is a manifest class */
	if (suit_plat_decode_manifest_class_id(manifest_component_id, &class_id) !=
	    SUIT_PLAT_SUCCESS) {
		LOG_ERR("Component ID is not a manifest class");
		return SUIT_ERR_UNAUTHORIZED_COMPONENT;
	}

	return suit_plat_component_compatibility_check(class_id, component_id);
}

int suit_plat_authorize_process_dependency(struct zcbor_string *parent_component_id,
					   struct zcbor_string *child_component_id,
					   enum suit_command_sequence seq_name)
{
	suit_manifest_class_id_t *parent_class_id = NULL;
	suit_manifest_class_id_t *child_class_id = NULL;
	suit_manifest_role_t parent_role = SUIT_MANIFEST_UNKNOWN;
	suit_manifest_role_t child_role = SUIT_MANIFEST_UNKNOWN;

	suit_plat_err_t err =
		suit_plat_decode_manifest_class_id(parent_component_id, &parent_class_id);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to parse parent manifest class ID (err: %i)", err);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	err = suit_plat_decode_manifest_class_id(child_component_id, &child_class_id);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to parse child manifest class ID (err: %i)", err);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	int ret = suit_plat_manifest_role_get(parent_class_id, &parent_role);

	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Unable to find parent manifest role (err: %i)", err);
		return ret;
	}

	ret = suit_plat_manifest_role_get(child_class_id, &child_role);
	if (ret != SUIT_SUCCESS) {
		LOG_ERR("Unable to find child manifest role (err: %i)", err);
		return ret;
	}

	/* Nordic top is allowed to fetch SCFW and SDFW manifests. */
	if ((parent_role == SUIT_MANIFEST_SEC_TOP) &&
	    ((child_role == SUIT_MANIFEST_SEC_SYSCTRL) || (child_role == SUIT_MANIFEST_SEC_SDFW))) {
		return SUIT_SUCCESS;
	}

	/* Application root is allowed to fetch any local as well as Nordic top manifest. */
	if ((parent_role == SUIT_MANIFEST_APP_ROOT) &&
	    (((child_role >= SUIT_MANIFEST_APP_LOCAL_1) &&
	      (child_role <= SUIT_MANIFEST_APP_LOCAL_3)) ||
	     ((child_role >= SUIT_MANIFEST_RAD_LOCAL_1) &&
	      (child_role <= SUIT_MANIFEST_RAD_LOCAL_2)) ||
	     (child_role == SUIT_MANIFEST_SEC_TOP))) {
		return SUIT_SUCCESS;
	}

	/* Application recovery may fetch only the radio recovery manifest. */
	if ((parent_role == SUIT_MANIFEST_APP_RECOVERY) &&
	    (child_role == SUIT_MANIFEST_RAD_RECOVERY)) {
		return SUIT_SUCCESS;
	}

	LOG_INF("Manifest dependency link unauthorized for sequence %d (err: %i)", seq_name, ret);

	return SUIT_ERR_UNAUTHORIZED_COMPONENT;
}
