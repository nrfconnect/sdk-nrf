/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_plat_retrieve_manifest_domain_specific.h>
#include <suit_plat_error_convert.h>

int suit_plat_retrieve_manifest_domain_specific(struct zcbor_string *component_id,
						suit_component_type_t component_type,
						const uint8_t **envelope_str, size_t *envelope_len)
{
	(void)component_id;
	(void)component_type;
	(void)envelope_str;
	(void)envelope_len;

	/* suit_plat_retrieve will only set the return value to success if some
	 * component type in the higher layer will be processed successfully.
	 */
	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}
