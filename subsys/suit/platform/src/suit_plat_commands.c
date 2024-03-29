/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform.h>

int suit_plat_check_slot(suit_component_t component_handle, unsigned int slot)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}

/** File a report on a command result. */
int suit_plat_report(unsigned int rep_policy, struct suit_report *report)
{
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}
