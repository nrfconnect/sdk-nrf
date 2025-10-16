/*
 * Copyright (c) 2025 PHY Wireless, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-PHYW
 */

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include <zephyr/kernel.h>

#include "otdoa_al_log.h"

#ifdef CONFIG_LOG_MODE_MINIMAL
#error "MINIMAL"
#endif
/**
 * NB: Leave this with LOG_LEVEL_DBG.  Otherwise, calls to
 * otdoa_log_XXX() where level XXX is disabled at compile-time
 * will result in a leak in the "strdup" memory.
 * Actual log level can be adjusted using the run-time filtering
 * e.g. 'log enable inf otdoa' to set the level to INF
 */
LOG_MODULE_REGISTER(otdoa, LOG_LEVEL_DBG);

uint32_t otdoa_log_level_set(int level, const char *backend)
{
	int source_id = log_source_id_get("otdoa");

	if (source_id >= 0) {
		log_filter_set(NULL, 0, source_id, level);
	} else {
		LOG_WRN("otdoa_log_level_set(): Failed to find module otdoa");
	}
	return 0;
}

void otdoa_log(int log_level, const char *string)
{
	switch (log_level) {
	case LOG_LEVEL_ERR:
		LOG_ERR("%s", string);
		break;
	case LOG_LEVEL_WRN:
		LOG_WRN("%s", string);
		break;
	case LOG_LEVEL_INF:
		LOG_INF("%s", string);
		break;
	case LOG_LEVEL_DBG:
		LOG_DBG("%s", string);
		break;
	default:
		/* ignore the log */
		break;
	}
}

void otdoa_log_init(void)
{
	otdoa_log_level_set(LOG_LEVEL_INF, "otdoa");
}
