/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "ml_result_event.h"


static int log_ml_result_event(const struct event_header *eh, char *buf, size_t buf_len)
{
	const struct ml_result_event *event = cast_ml_result_event(eh);

	EVENT_MANAGER_LOG(eh, "%s val: %0.2f anomaly: %0.2f",
			event->label, event->value, event->anomaly);
	return 0;
}

static int log_ml_result_signin_event(const struct event_header *eh, char *buf, size_t buf_len)
{
	const struct ml_result_signin_event *event = cast_ml_result_signin_event(eh);

	EVENT_MANAGER_LOG(eh, "module: \"%s\" %s ml_result_event",
			module_name_get(module_id_get(event->module_idx)),
			event->state ? "signs in to" : "signs off from");
	return 0;
}

static void profile_ml_result_event(struct log_event_buf *buf, const struct event_header *eh)
{
}

static void profile_ml_result_signin_event(struct log_event_buf *buf,
					   const struct event_header *eh)
{
	const struct ml_result_signin_event *event = cast_ml_result_signin_event(eh);

	profiler_log_encode_uint32(buf, event->module_idx);
	profiler_log_encode_uint8(buf, event->state);
}

EVENT_INFO_DEFINE(ml_result_event,
		  ENCODE(),
		  ENCODE(),
		  profile_ml_result_event);

EVENT_TYPE_DEFINE(ml_result_event,
		  IS_ENABLED(CONFIG_ML_APP_INIT_LOG_ML_RESULT_EVENTS),
		  log_ml_result_event,
		  &ml_result_event_info);


EVENT_INFO_DEFINE(ml_result_signin_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U8),
		  ENCODE("module", "state"),
		  profile_ml_result_signin_event);

EVENT_TYPE_DEFINE(ml_result_signin_event,
		  IS_ENABLED(CONFIG_ML_APP_INIT_LOG_ML_RESULT_SIGNIN_EVENTS),
		  log_ml_result_signin_event,
		  &ml_result_signin_event_info);
