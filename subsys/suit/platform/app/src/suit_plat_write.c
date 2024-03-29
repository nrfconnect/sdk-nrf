/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <suit_platform.h>

LOG_MODULE_REGISTER(suit_plat_write, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_check_write(suit_component_t dst_handle, struct zcbor_string *content)
{
	LOG_ERR("SUIT directive write is not supported for app");
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}

int suit_plat_write(suit_component_t dst_handle, struct zcbor_string *content)
{
	LOG_ERR("SUIT directive write is not supported for app");
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}
