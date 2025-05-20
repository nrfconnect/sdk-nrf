/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(suit_plat_devconfig, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_sequence_completed(enum suit_command_sequence seq_name,
				 struct zcbor_string *manifest_component_id,
				 const uint8_t *envelope_str, size_t envelope_len)
{
	return SUIT_SUCCESS;
}
