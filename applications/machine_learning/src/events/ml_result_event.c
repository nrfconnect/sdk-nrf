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

	return snprintf(buf, buf_len, "%s val: %0.2f anomaly: %0.2f",
			event->label, event->value, event->anomaly);
}

static void profile_ml_result_event(struct log_event_buf *buf, const struct event_header *eh)
{
}

EVENT_INFO_DEFINE(ml_result_event,
		  ENCODE(),
		  ENCODE(),
		  profile_ml_result_event);

EVENT_TYPE_DEFINE(ml_result_event,
		  IS_ENABLED(CONFIG_ML_APP_INIT_LOG_ML_RESULT_EVENTS),
		  log_ml_result_event,
		  &ml_result_event_info);
