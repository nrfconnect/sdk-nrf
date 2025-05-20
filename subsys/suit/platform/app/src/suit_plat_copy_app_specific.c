/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_plat_copy_domain_specific.h>

bool suit_plat_copy_domain_specific_is_type_supported(suit_component_type_t dst_component_type,
						      suit_component_type_t src_component_type)
{
	return false;
}

int suit_plat_check_copy_domain_specific(suit_component_t dst_handle,
					 suit_component_type_t dst_component_type,
					 suit_component_t src_handle,
					 suit_component_type_t src_component_type,
					 struct zcbor_string *manifest_component_id,
					 struct suit_encryption_info *enc_info)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}

int suit_plat_copy_domain_specific(suit_component_t dst_handle,
				   suit_component_type_t dst_component_type,
				   suit_component_t src_handle,
				   suit_component_type_t src_component_type,
				   struct zcbor_string *manifest_component_id,
				   struct suit_encryption_info *enc_info)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}
