/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <assert.h>

#include "cdc_data_event.h"

static void log_cdc_data_event(const struct event_header *eh)
{
	const struct cdc_data_event *event = cast_cdc_data_event(eh);

	EVENT_MANAGER_LOG(eh,
		"dev:%u buf:%p len:%d",
		event->dev_idx,
		event->buf,
		event->len);
}

EVENT_TYPE_DEFINE(cdc_data_event,
		  IS_ENABLED(CONFIG_BRIDGE_LOG_CDC_DATA_EVENT),
		  log_cdc_data_event,
		  NULL);
