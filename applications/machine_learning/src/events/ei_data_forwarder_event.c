/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>

#include "ei_data_forwarder_event.h"

static const char * const ei_data_forwarder_state_name[] = {
	[EI_DATA_FORWARDER_STATE_DISABLED] = "DISABLED",
	[EI_DATA_FORWARDER_STATE_DISCONNECTED] = "DISCONNECTED",
	[EI_DATA_FORWARDER_STATE_CONNECTED] = "CONNECTED",
	[EI_DATA_FORWARDER_STATE_TRANSMITTING] = "TRANSMITTING",
};

static int log_ei_data_forwarder_event(const struct event_header *eh, char *buf, size_t buf_len)
{
	const struct ei_data_forwarder_event *event = cast_ei_data_forwarder_event(eh);

	BUILD_ASSERT(ARRAY_SIZE(ei_data_forwarder_state_name) == EI_DATA_FORWARDER_STATE_COUNT);
	__ASSERT_NO_MSG(event->state < EI_DATA_FORWARDER_STATE_COUNT);
	__ASSERT_NO_MSG(ei_data_forwarder_state_name[event->state] != NULL);

	return snprintf(buf, buf_len, "state: %s", ei_data_forwarder_state_name[event->state]);
}

EVENT_TYPE_DEFINE(ei_data_forwarder_event,
		  IS_ENABLED(CONFIG_ML_APP_INIT_LOG_EI_DATA_FORWARDER_EVENTS),
		  log_ei_data_forwarder_event,
		  NULL);
