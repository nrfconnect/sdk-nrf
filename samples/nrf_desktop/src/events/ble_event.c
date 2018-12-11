/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <assert.h>
#include <misc/util.h>
#include <stdio.h>

#include "ble_event.h"


static const char * const state_name[] = {
#define X(name) STRINGIFY(name),
	PEER_STATE_LIST
#undef X
};

static int log_event(const struct event_header *eh, char *buf,
			size_t buf_len)
{
	struct ble_peer_event *event = cast_ble_peer_event(eh);

	static_assert(ARRAY_SIZE(state_name) == PEER_STATE_COUNT,
		      "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < PEER_STATE_COUNT);

	return snprintf(buf, buf_len, "id=%p %s", event->id,
			state_name[event->state]);
}


EVENT_TYPE_DEFINE(ble_peer_event, log_event, NULL);
