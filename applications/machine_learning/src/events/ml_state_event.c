/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "ml_state_event.h"

static const char * const ml_state_name[] = {
	[ML_STATE_MODEL_RUNNING] = "MODEL_RUNNING",
	[ML_STATE_DATA_FORWARDING] = "DATA_FORWARDING",
};

static int log_ml_state_event(const struct event_header *eh, char *buf, size_t buf_len)
{
	const struct ml_state_event *event = cast_ml_state_event(eh);

	BUILD_ASSERT(ARRAY_SIZE(ml_state_name) == ML_STATE_COUNT);
	__ASSERT_NO_MSG(event->state < ML_STATE_COUNT);
	__ASSERT_NO_MSG(ml_state_name[event->state] != NULL);

	return snprintf(buf, buf_len, "state: %s", ml_state_name[event->state]);
}

EVENT_TYPE_DEFINE(ml_state_event,
		  IS_ENABLED(CONFIG_ML_APP_INIT_LOG_ML_STATE_EVENTS),
		  log_ml_state_event,
		  NULL);
