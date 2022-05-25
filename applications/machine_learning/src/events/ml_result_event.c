/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "ml_result_event.h"


static void log_ml_result_event(const struct app_event_header *aeh)
{
	const struct ml_result_event *event = cast_ml_result_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "%s val: %0.2f anomaly: %0.2f",
			event->label, event->value, event->anomaly);
}

static void log_ml_result_signin_event(const struct app_event_header *aeh)
{
	const struct ml_result_signin_event *event = cast_ml_result_signin_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "module: \"%s\" %s ml_result_event",
			module_name_get(module_id_get(event->module_idx)),
			event->state ? "signs in to" : "signs off from");
}

static void profile_ml_result_event(struct log_event_buf *buf,
				    const struct app_event_header *aeh)
{
}

static void profile_ml_result_signin_event(struct log_event_buf *buf,
					   const struct app_event_header *aeh)
{
	const struct ml_result_signin_event *event = cast_ml_result_signin_event(aeh);

	nrf_profiler_log_encode_uint32(buf, event->module_idx);
	nrf_profiler_log_encode_uint8(buf, event->state);
}

APP_EVENT_INFO_DEFINE(ml_result_event,
		  ENCODE(),
		  ENCODE(),
		  profile_ml_result_event);

APP_EVENT_TYPE_DEFINE(ml_result_event,
		  log_ml_result_event,
		  &ml_result_event_info,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_ML_APP_INIT_LOG_ML_RESULT_EVENTS,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));

APP_EVENT_INFO_DEFINE(ml_result_signin_event,
		  ENCODE(NRF_PROFILER_ARG_U32, NRF_PROFILER_ARG_U8),
		  ENCODE("module", "state"),
		  profile_ml_result_signin_event);

APP_EVENT_TYPE_DEFINE(ml_result_signin_event,
		  log_ml_result_signin_event,
		  &ml_result_signin_event_info,
		  APP_EVENT_FLAGS_CREATE(
			IF_ENABLED(CONFIG_ML_APP_INIT_LOG_ML_RESULT_SIGNIN_EVENTS,
				(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE))));
