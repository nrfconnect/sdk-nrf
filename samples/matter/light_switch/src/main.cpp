/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include <cstdlib>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, CONFIG_MATTER_LOG_LEVEL);

int main()
{
	return GetAppTask().StartApp() == CHIP_NO_ERROR ? EXIT_SUCCESS : EXIT_FAILURE;
}
