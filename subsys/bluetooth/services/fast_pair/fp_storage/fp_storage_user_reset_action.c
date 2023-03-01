/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include "fp_storage_manager.h"

__weak int bt_fast_pair_factory_reset_user_action_perform(void)
{
	/* intentionally left empty */
	return 0;
}

__weak void bt_fast_pair_factory_reset_user_action_prepare(void)
{
	/* intentionally left empty */
}

FP_STORAGE_MANAGER_MODULE_REGISTER(bt_fast_pair_factory_reset_user_action,
				   bt_fast_pair_factory_reset_user_action_perform,
				   bt_fast_pair_factory_reset_user_action_prepare);
