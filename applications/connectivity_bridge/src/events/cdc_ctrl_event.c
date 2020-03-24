/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <assert.h>

#include "cdc_ctrl_event.h"

static int log_cdc_ctrl_event(const struct event_header *eh, char *buf,
				  size_t buf_len)
{
	const struct cdc_ctrl_event *event = cast_cdc_ctrl_event(eh);

	return snprintf(
		buf,
		buf_len,
		"cmd:%d",
		event->cmd);
}

EVENT_TYPE_DEFINE(cdc_ctrl_event,
		  IS_ENABLED(CONFIG_BRIDGE_LOG_CDC_CTRL_EVENT),
		  log_cdc_ctrl_event,
		  NULL);
