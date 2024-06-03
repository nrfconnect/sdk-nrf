/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_execution_mode.h>

static suit_execution_mode_t current_execution_mode = EXECUTION_MODE_STARTUP;

suit_execution_mode_t suit_execution_mode_get(void)
{
	return current_execution_mode;
}

suit_plat_err_t suit_execution_mode_set(suit_execution_mode_t mode)
{
	if ((mode > EXECUTION_MODE_STARTUP) && (mode <= EXECUTION_MODE_FAIL_INSTALL_NORDIC_TOP)) {
		current_execution_mode = mode;

		return SUIT_PLAT_SUCCESS;
	}

	return SUIT_PLAT_ERR_INVAL;
}
