/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform.h>
#include <suit_mci.h>
#include <suit_plat_error_convert.h>

int suit_plat_supported_manifest_class_infos_get(suit_manifest_class_info_t *class_info,
						 size_t *size)
{
	return suit_plat_err_to_processor_err_convert(
		suit_mci_supported_manifest_class_ids_get(class_info, size));
}

int suit_plat_manifest_role_get(const suit_manifest_class_id_t *class_id,
				suit_manifest_role_t *role)
{
	(void)class_id;
	(void)role;

	/* Function used only by the application domain version of the platform */
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}
