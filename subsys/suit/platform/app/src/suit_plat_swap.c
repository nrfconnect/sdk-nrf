/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform.h>

int suit_plat_check_swap(suit_component_t dst_handle, suit_component_t src_handle,
			 struct zcbor_string *manifest_component_id,
			 struct suit_encryption_info *enc_info)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}

int suit_plat_swap(suit_component_t dst_handle, suit_component_t src_handle,
		   struct zcbor_string *manifest_component_id,
		   struct suit_encryption_info *enc_info)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}
