/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(app);

int main()
{
	return GetAppTask().StartApp();
}
