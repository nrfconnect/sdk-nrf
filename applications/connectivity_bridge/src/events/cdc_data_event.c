/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <assert.h>

#include "cdc_data_event.h"

static void log_cdc_data_event(const struct app_event_header *aeh)
{
	const struct cdc_data_event *event = cast_cdc_data_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh,
		"dev:%u buf:%p len:%d",
		event->dev_idx,
		event->buf,
		event->len);
}

APP_EVENT_TYPE_DEFINE(cdc_data_event,
		  log_cdc_data_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_BRIDGE_LOG_CDC_DATA_EVENT,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
