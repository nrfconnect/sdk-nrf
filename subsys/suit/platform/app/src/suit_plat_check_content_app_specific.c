/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_plat_check_content_domain_specific.h>

bool suit_plat_check_content_domain_specific_is_type_supported(suit_component_type_t component_type)
{
	return false;
}

int suit_plat_check_content_domain_specific(suit_component_t component,
					    suit_component_type_t component_type,
					    struct zcbor_string *content)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}
