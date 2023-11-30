/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/services/fast_pair.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fast_pair, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include "fp_storage.h"
#include "fp_activation.h"

int bt_fast_pair_factory_reset(void)
{
	return fp_storage_factory_reset();
}

static int storage_init(void)
{
	if (!fp_storage_settings_loaded()) {
		LOG_ERR("Fast Pair settings shall be loaded before enabling Fast Pair");
		return -ENOPROTOOPT;
	}

	return fp_storage_init();
}

static int storage_uninit(void)
{
	return fp_storage_uninit();
}

BUILD_ASSERT(CONFIG_BT_FAST_PAIR_STORAGE_INTEGRATION_INIT_PRIORITY <
	     FP_ACTIVATION_INIT_PRIORITY_DEFAULT);

FP_ACTIVATION_MODULE_REGISTER(fp_storage, CONFIG_BT_FAST_PAIR_STORAGE_INTEGRATION_INIT_PRIORITY,
			      storage_init, storage_uninit);
