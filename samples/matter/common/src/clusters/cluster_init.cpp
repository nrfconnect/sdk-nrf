/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cluster_init.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nrf_matter_cluster_init, CONFIG_CHIP_APP_LOG_LEVEL);

bool nrf_matter_cluster_init_run_all(void)
{
	bool result = true;

	STRUCT_SECTION_FOREACH(nrf_matter_cluster_init_entry, entry)
	{
		if (entry->callback != nullptr) {
			LOG_DBG("Initializing cluster: %s", entry->name);
			if (!entry->callback()) {
				LOG_ERR("Cluster initialization failed: %s", entry->name);
				result = false;
			}
		}
	}

	return result;
}
