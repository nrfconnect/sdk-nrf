/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(execution_context, LOG_LEVEL_INF);

static int log_application_execution_context(void)
{
	LOG_INF("Now running: application");

	return 0;
}

SYS_INIT(log_application_execution_context, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
