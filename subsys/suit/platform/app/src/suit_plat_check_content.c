/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(suit_plat_check_content, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_check_content(suit_component_t component, struct zcbor_string *content)
{
	(void) component;
	(void) content;

	return SUIT_ERR_UNSUPPORTED_COMMAND;
}
