/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <assert.h>
#include <misc/util.h>

#include "ble_event.h"


static const char * const state_name[] = {
#define X(name) STRINGIFY(name),
	PEER_STATE_LIST
#undef X
};

static void print_event(const struct event_header *eh)
{
	struct ble_peer_event *event = cast_ble_peer_event(eh);

	static_assert(ARRAY_SIZE(state_name) == PEER_STATE_COUNT,
		      "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < PEER_STATE_COUNT);

	printk("conn_id=%p %s", event->conn_id, state_name[event->state]);
}


EVENT_TYPE_DEFINE(ble_peer_event, print_event, NULL);

EVENT_TYPE_DEFINE(ble_interval_event, NULL, NULL);

