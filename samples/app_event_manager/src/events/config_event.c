/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "config_event.h"


static void profile_config_event(struct log_event_buf *buf,
				 const struct app_event_header *aeh)
{
	struct config_event *event = cast_config_event(aeh);

	ARG_UNUSED(event);
	nrf_profiler_log_encode_int8(buf, event->init_value1);
}

static void log_config_event(const struct app_event_header *aeh)
{
	struct config_event *event = cast_config_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "init_val_1=%d", event->init_value1);
}

APP_EVENT_INFO_DEFINE(config_event,
		  ENCODE(NRF_PROFILER_ARG_S8),
		  ENCODE("init_val_1"),
		  profile_config_event);

APP_EVENT_TYPE_DEFINE(config_event,
		  log_config_event,
		  &config_event_info,
		  APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
