/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <suit_platform.h>

LOG_MODULE_REGISTER(suit_plat_copy, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_check_copy(suit_component_t dst_handle, suit_component_t src_handle,
			 struct zcbor_string *manifest_component_id,
			 struct suit_encryption_info *enc_info)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}

int suit_plat_copy(suit_component_t dst_handle, suit_component_t src_handle,
		   struct zcbor_string *manifest_component_id,
		   struct suit_encryption_info *enc_info)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}
