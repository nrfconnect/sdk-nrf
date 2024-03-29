/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <suit_platform.h>

LOG_MODULE_REGISTER(suit_plat_invoke, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_check_invoke(suit_component_t image_handle, struct zcbor_string *invoke_args)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}

int suit_plat_invoke(suit_component_t image_handle, struct zcbor_string *invoke_args)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}
