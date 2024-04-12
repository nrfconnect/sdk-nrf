/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include "app_ui.h"
#include "app_ui_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_fmdn, LOG_LEVEL_INF);

void app_ui_request_broadcast(enum app_ui_request request)
{
	STRUCT_SECTION_FOREACH(app_ui_request_listener, listener) {
		__ASSERT_NO_MSG(listener->handler != NULL);
		listener->handler(request);
	}
}
